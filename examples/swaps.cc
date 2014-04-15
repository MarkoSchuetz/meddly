
// $Id$

/*
    Meddly: Multi-terminal and Edge-valued Decision Diagram LibrarY.
    Copyright (C) 2009, Iowa State University Research Foundation, Inc.

    This library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published 
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
    Simple model of an array of (unique) integers,
    with operations to swap neighboring elements.
*/

#include "meddly.h"
#include "meddly_expert.h"
#include "timer.h"

using namespace MEDDLY;

// #define DUMP_NSF
// #define DUMP_REACHABLE

// Helpers

inline double factorial(int n)
{
  double f = 1.0;
  for (int i=2; i<=n; i++) {
    f *= i;
  }
  return f;
}

//
//
// Encoding based on the obvious: each state variable is
// one of the array elements.
//
//

/*
    Builds a next-state relation for exchanging two variables.
    The forest must be IDENTITY REDUCED.
*/
void Exchange(int va, int vb, int N, dd_edge &answer)
{
  expert_forest* EF = (expert_forest*) answer.getForest(); 

  /* We're doing this BY HAND which means a 4 levels of nodes */

  node_builder& na = EF->useNodeBuilder(va, N);
  for (int ia=0; ia<N; ia++) {
    node_builder& nap = EF->useNodeBuilder(-va, N);
    for (int ja=0; ja<N; ja++) {
      
      // WANT vb == va' and vb' == va, so...

      // Make a singleton for vb' == va (index ia)
      node_builder& nbp = EF->useSparseBuilder(-vb, 1);
      nbp.i(0) = ia;
      nbp.d(0) = EF->handleForValue(1);

      // Make a singleton for vb == va' (index ja)
      node_builder& nb = EF->useSparseBuilder(vb, 1);
      nb.i(0) = ja;
      nb.d(0) = EF->createReducedNode(ja, nbp);

      nap.d(ja) = EF->createReducedNode(-1, nb);
    } // for ja
    na.d(ia) = EF->createReducedNode(ia, nap);
  } // for ia

  answer.set(EF->createReducedNode(-1, na), 0);
}

/*
    Build next-state relation, using "array values" state variables.
    The forest must be IDENTITY REDUCED.
*/
void ValueNSF(int N, dd_edge &answer)
{
  dd_edge temp(answer);
  answer.set(0, 0);

  for (int i=1; i<N; i++) {
    Exchange(i+1, i, N, temp);
    answer += temp;
  }
}

//
//
// Alternate encoding: each state variable is one of the unique integers,
// and we store its position in the array.
//
//

/*
    Builds a next-state relation for exchanging two positions.
    The forest must be IDENTITY REDUCED.
*/
void AltExchange(int pa, int pb, int N, int K, dd_edge &answer)
{
  expert_forest* EF = (expert_forest*) answer.getForest(); 

  /*
      Do the same thing at every level:
        if the position is pa, change it to pb.
        if the position is pb, change it to pa.
        otherwise, no change.
  */
  node_handle bottom = EF->handleForValue(1);

  for (int k=1; k<=K; k++) {
    node_builder& nk = EF->useNodeBuilder(k, N);

    for (int i=0; i<N; i++) {
      if (pa == i) {
        node_builder& nkp = EF->useSparseBuilder(-k, 1);
        nkp.i(0) = pb;
        nkp.d(0) = bottom;
        nk.d(i) = EF->createReducedNode(i, nkp);
        continue;
      }
      if (pb == i) {
        node_builder& nkp = EF->useSparseBuilder(-k, 1);
        nkp.i(0) = pa;
        nkp.d(0) = bottom;
        nk.d(i) = EF->createReducedNode(i, nkp);
        continue;
      }
      nk.d(i) = bottom;
    } // for i

    bottom = EF->createReducedNode(-1, nk);
  }

  answer.set(bottom, 0);
}

/*
    Build next-state relation, using "array positions" state variables.
    The forest must be IDENTITY REDUCED.
*/
void PositionNSF(int N, dd_edge &answer)
{
  dd_edge temp(answer);
  answer.set(0, 0);

  for (int i=1; i<N; i++) {
    AltExchange(i-1, i, N, N, temp);
    answer += temp;
  }
}


//
//
// I/O crud
//
//

void printStats(const char* who, const forest* f)
{
  printf("%s stats:\n", who);
  const expert_forest* ef = (expert_forest*) f;
  ef->reportStats(stdout, "\t",
    expert_forest::HUMAN_READABLE_MEMORY  |
    expert_forest::BASIC_STATS | expert_forest::EXTRA_STATS |
    expert_forest::STORAGE_STATS | expert_forest::HOLE_MANAGER_STATS
  );
  fflush(stdout);
}

int usage(const char* who)
{
  /* Strip leading directory, if any: */
  const char* name = who;
  for (const char* ptr=who; *ptr; ptr++) {
    if ('/' == *ptr) name = ptr+1;
  }
  printf("\nUsage: %s nnnn (-bfs) (-dfs)\n\n", name);
  printf("\tnnnn: number of parts\n");
  printf("\t-bfs: use traditional iterations\n");
  printf("\t-dfs: use saturation (default)\n\n");
  printf("\t-alt: use alternate description\n");
  return 1;
}

int main(int argc, const char** argv)
{
  timer watch;
  int N = -1;
  char method = 'd';
  bool alternate = false;

  /*
    Process arguments
  */
  for (int i=1; i<argc; i++) {
    if (strcmp("-bfs", argv[i])==0) {
      method = 'b';
      continue;
    }
    if (strcmp("-dfs", argv[i])==0) {
      method = 'd';
      continue;
    }
    if (strcmp("-alt", argv[i])==0) {
      alternate = true;
      continue;
    }
    N = atoi(argv[i]);
  } // for i

  if (N<0) return usage(argv[0]);

  MEDDLY::initialize();

  printf("+-------------------------------------------+\n");
  printf("|   Initializing swaps model for N = %-4d   |\n", N);
  printf("+-------------------------------------------+\n");
  fflush(stdout);

  /*
     Initialize domain
  */
  int* sizes = new int[N];
  for (int i=0; i<N; i++) sizes[i] = N;
  domain* D = createDomainBottomUp(sizes, N);
  delete[] sizes;

  /*
     Build next-state function
  */
  forest* mxd = D->createForest(1, forest::BOOLEAN, forest::MULTI_TERMINAL);
  dd_edge nsf(mxd);

  watch.note_time();
  if (alternate) {
    PositionNSF(N, nsf);
  } else {
    ValueNSF(N, nsf);
  }
  watch.note_time();

  printf("Next-state function construction took %.4f seconds\n",
          watch.get_last_interval()/1000000.0);
  printf("Next-state function MxD has\n\t%d nodes\n\t\%d edges\n",
    nsf.getNodeCount(), nsf.getEdgeCount());

#ifdef DUMP_NSF
  printf("Next-state function:\n");
  nsf.show(stdout, 2);
#endif

  printStats("MxD", mxd);

  /*
     Build initial state
  */
  int* initial = new int[N+1];
  initial[0] = 0;
  for (int i=1; i<=N; i++) initial[i] = i-1;
  forest::policies p(false);
  forest* mdd = D->createForest(0, forest::BOOLEAN, forest::MULTI_TERMINAL, p);
  dd_edge init_state(mdd);
  mdd->createEdge(&initial, 1, init_state);
  delete[] initial;
  
  /*
      Build reachable states
  */
  watch.note_time();
  dd_edge reachable(mdd);
  switch (method) {
    case 'b':
        printf("Building reachability set using traditional algorithm\n");
        fflush(stdout);
        apply(REACHABLE_STATES_BFS, init_state, nsf, reachable);
        break;

    case 'd':
        printf("Building reachability set using saturation\n");
        fflush(stdout);
        apply(REACHABLE_STATES_DFS, init_state, nsf, reachable);
        break;

    default:
        printf("Error - unknown method\n");
        exit(2);
  };
  watch.note_time();
  printf("Reachability set construction took %.4f seconds\n",
          watch.get_last_interval()/1000000.0);
  printf("Reachability set MDD has\n\t%d nodes\n\t\%d edges\n",
    reachable.getNodeCount(), reachable.getEdgeCount());
  fflush(stdout);
//  mdd->garbageCollect();

#ifdef DUMP_REACHABLE
  printf("Reachable states:\n");
  reachable.show(stdout, 2);
#endif

  printStats("MDD", mdd);
  operation::showAllComputeTables(stdout, 1);
  fflush(stdout);

  /*
      Determine cardinality
  */
  double c;
  apply(CARDINALITY, reachable, c);
  printf("Counted (approx) %g reachable states\n", c);
  printf("(There should be %g states)\n", factorial(N));

  /*
      Cleanup
  */
  MEDDLY::cleanup();
  return 0;
}
