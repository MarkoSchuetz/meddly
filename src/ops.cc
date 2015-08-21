
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
    Implementation of operation framework.

    Actual operations are in separate files, in operations/ directory.
*/

#include "defines.h"
// #include "compute_table.h"

// #define DEBUG_CLEANUP
// #define DEBUG_FINALIZE
// #define DEBUG_FINALIZE_SPLIT

namespace MEDDLY {
  extern settings meddlySettings;

  /*
      Convert from array of linked lists to contiguous array with indexes.
  
      Idea:
      traverse the array in order.
      everything after this point is still "linked lists".
      everything before this point is contiguous, but
        it is possible that one of the lists points here.
        if so, the next pointer holds the "forwarding address".
  */
  template <typename TYPE>
  void defragLists(TYPE* data, int* next, int* Lists, int NL)
  {
    int P = 0;  // "this point"
    for (int i=NL; i; i--) {
      int L = Lists[i];
      Lists[i] = P;
      while (L>=0) {
        // if L<P then there must be a forwarding address; follow it
        while (L<P) L = next[L];
        // ok, we're at the right slot now
        int nxt = next[L];
        if (L != P) {
          // element L belongs in slot P, swap them...
          SWAP(data[L], data[P]);
          next[L] = next[P];
          // ...and set up the forwarding address
          next[P] = L;
        }
        P++;
        L = nxt;
      }
    } // for k
    Lists[0] = P;
  }
}

// ******************************************************************
// *                         opname methods                         *
// ******************************************************************

int MEDDLY::opname::next_index;

MEDDLY::opname::opname(const char* n)
{
  name = n;
  index = next_index;
  next_index++;
}

MEDDLY::opname::~opname()
{
  // library must be closing
}

// ******************************************************************
// *                      unary_opname methods                      *
// ******************************************************************

MEDDLY::unary_opname::unary_opname(const char* n) : opname(n)
{
}

MEDDLY::unary_opname::~unary_opname()
{
}

MEDDLY::unary_operation* 
MEDDLY::unary_opname::buildOperation(expert_forest* ar, expert_forest* rs) const
{
  throw error(error::TYPE_MISMATCH);  
}

MEDDLY::unary_operation* 
MEDDLY::unary_opname::buildOperation(expert_forest* ar, opnd_type res) const
{
  throw error(error::TYPE_MISMATCH);
}


// ******************************************************************
// *                     binary_opname  methods                     *
// ******************************************************************

MEDDLY::binary_opname::binary_opname(const char* n) : opname(n)
{
}

MEDDLY::binary_opname::~binary_opname()
{
}

// ******************************************************************
// *                   specialized_opname methods                   *
// ******************************************************************

MEDDLY::specialized_opname::arguments::arguments()
{
  setAutoDestroy(true);
}

MEDDLY::specialized_opname::arguments::~arguments()
{
}


MEDDLY::specialized_opname::specialized_opname(const char* n) : opname(n)
{
}

MEDDLY::specialized_opname::~specialized_opname()
{
}

// ******************************************************************
// *                    numerical_opname methods                    *
// ******************************************************************

MEDDLY::numerical_opname::numerical_args
::numerical_args(const dd_edge &xi, const dd_edge &a, const dd_edge &yi)
 : x_ind(xi), A(a), y_ind(yi)
{
}

MEDDLY::numerical_opname::numerical_args::~numerical_args()
{
}


MEDDLY::numerical_opname::numerical_opname(const char* n)
 : specialized_opname(n)
{
}

MEDDLY::numerical_opname::~numerical_opname()
{
}

// ******************************************************************
// *                                                                *
// *                    satpregen_opname methods                    *
// *                                                                *
// ******************************************************************


MEDDLY::satpregen_opname::satpregen_opname(const char* n)
 : specialized_opname(n)
{
}

MEDDLY::satpregen_opname::~satpregen_opname()
{
}


void
MEDDLY::satpregen_opname::pregen_relation
::setForests(forest* inf, forest* mxd, forest* outf)
{
  insetF = inf;
  outsetF = outf;
  mxdF = smart_cast <MEDDLY::expert_forest*>(mxd);
  if (0==insetF || 0==outsetF || 0==mxdF) throw error(error::MISCELLANEOUS);

  // Check for same domain
  if (  
    (insetF->getDomain() != mxdF->getDomain()) || 
    (outsetF->getDomain() != mxdF->getDomain()) 
  )
    throw error(error::DOMAIN_MISMATCH);

  // for now, anyway, inset and outset must be same forest
  if (insetF != outsetF)
    throw error(error::FOREST_MISMATCH);

  // Check forest types
  if (
    insetF->isForRelations()    ||
    !mxdF->isForRelations()     ||
    outsetF->isForRelations()   ||
    (insetF->getRangeType() != mxdF->getRangeType())        ||
    (outsetF->getRangeType() != mxdF->getRangeType())       ||
    (insetF->getEdgeLabeling() != forest::MULTI_TERMINAL)   ||
    (outsetF->getEdgeLabeling() != forest::MULTI_TERMINAL)  ||
    (mxdF->getEdgeLabeling() != forest::MULTI_TERMINAL)     ||
    (outsetF->getEdgeLabeling() != forest::MULTI_TERMINAL)
  )
    throw error(error::TYPE_MISMATCH);

  // Forests are good; set number of variables
  K = mxdF->getDomain()->getNumVariables();
}

MEDDLY::satpregen_opname::pregen_relation
::pregen_relation(forest* inf, forest* mxd, forest* outf, int nevents)
{
  setForests(inf, mxd, outf);

  num_events = nevents;
  if (num_events) {
    events = new node_handle[num_events];
    next = new int[num_events];
  } else {
    events = 0;
    next = 0;
  }
  last_event = -1;

  level_index = new int[K+1];
  for (int k=0; k<=K; k++) level_index[k] = -1;   // null pointer
}

MEDDLY::satpregen_opname::pregen_relation
::pregen_relation(forest* inf, forest* mxd, forest* outf)
{
  setForests(inf, mxd, outf);

  events = new node_handle[K+1];
  for (int k=0; k<=K; k++) events[k] = 0;

  next = 0;
  level_index = 0;

  num_events = -1;
  last_event = -1;
}


MEDDLY::satpregen_opname::pregen_relation
::~pregen_relation()
{
  if (events && mxdF) {
    for (int k=0; k<=K; k++) if(events[k]) mxdF->unlinkNode(events[k]);
  }
  delete[] events;
  delete[] next;
  delete[] level_index;
}

void
MEDDLY::satpregen_opname::pregen_relation
::addToRelation(const dd_edge &r)
{
  MEDDLY_DCASSERT(mxdF);

  if (r.getForest() != mxdF)  throw error(error::FOREST_MISMATCH);

  int k = r.getLevel(); 
  if (0==k) return;
  if (k<0) k = -k;   

  if (0==level_index) {
    // relation is "by levels"

    if (0==events[k]) {
      events[k] = mxdF->linkNode(r.getNode());
    } else {
      // already have something at this level; perform the union
      dd_edge tmp(mxdF);
      tmp.set(events[k]); // does not increase incoming count
      tmp += r;
      events[k] = mxdF->linkNode(tmp.getNode());
      // tmp should be destroyed here
    }

  } else {
    // relation is "by events"

    if (isFinalized())              throw error(error::MISCELLANEOUS);
    if (last_event+1 >= num_events) throw error(error::OVERFLOW);

    last_event++;

    events[last_event] = mxdF->linkNode(r.getNode());
    next[last_event] = level_index[k];
    level_index[k] = last_event;
  }
}


void
MEDDLY::satpregen_opname::pregen_relation
::splitMxd(splittingOption split)
{
  if (split == None) return;
  if (split == MonolithicSplit) {
    unionLevels();
    split = SplitOnly;
  }

  // For each level k, starting from the top level
  //    MXD(k) is the matrix diagram at level k.
  //    Calculate ID(k), the intersection of the diagonals of MXD(k).
  //    Subtract ID(k) from MXD(k).
  //    Add ID(k) to level(ID(k)).

#ifdef DEBUG_FINALIZE_SPLIT
  printf("Splitting events in finalize()\n");
  printf("events array: [");
  for (int i=0; i<=K; i++) {
    if (i) printf(", ");
    printf("%d", events[i]);
  }
  printf("]\n");
#endif

  // Initialize operations
  binary_operation* mxdUnion = getOperation(UNION, mxdF, mxdF, mxdF);
  MEDDLY_DCASSERT(mxdUnion);

  binary_operation* mxdIntersection =
    getOperation(INTERSECTION, mxdF, mxdF, mxdF);
  MEDDLY_DCASSERT(mxdIntersection);

  binary_operation* mxdDifference = getOperation(DIFFERENCE, mxdF, mxdF, mxdF);
  MEDDLY_DCASSERT(mxdDifference);

  for (int k = K; k > 1; k--) {
    if (0 == events[k]) continue;

    MEDDLY_DCASSERT(ABS(mxdF->getNodeLevel(events[k])) <= k);

    // Initialize readers
    node_reader* Mu = isLevelAbove(k, mxdF->getNodeLevel(events[k]))
      ? mxdF->initRedundantReader(k, events[k], true)
      : mxdF->initNodeReader(events[k], true);
    node_reader* Mp = node_reader::useReader();

    bool first = true;
    node_handle maxDiag = 0;

    // Read "rows"
    for (int i = 0; i < Mu->getSize(); i++) {
      // Initialize column reader
      if (isLevelAbove(-k, mxdF->getNodeLevel(Mu->d(i)))) {
        mxdF->initIdentityReader(*Mp, -k, i, Mu->d(i), true);
      } else {
        mxdF->initNodeReader(*Mp, Mu->d(i), true);
      }

      // Intersect along the diagonal
      if (first) {
        maxDiag = mxdF->linkNode(Mp->d(i));
        first = false;
      } else {
        node_handle nmd = mxdIntersection->compute(maxDiag, Mp->d(i));
        mxdF->unlinkNode(maxDiag);
        maxDiag = nmd;
      }
    } // for i

    // Cleanup
    node_reader::recycle(Mp);
    node_reader::recycle(Mu);

    if (0 == maxDiag) {
#ifdef DEBUG_FINALIZE_SPLIT
      printf("splitMxd: event %d, maxDiag %d\n", events[k], maxDiag);
#endif
      continue;
    }

    // Subtract maxDiag from events[k]
    // Do this only for SplitOnly. Other cases are handled later.
    if (split == SplitOnly) {
      int tmp = events[k];
      events[k] = mxdDifference->compute(events[k], maxDiag);
      mxdF->unlinkNode(tmp);
#ifdef DEBUG_FINALIZE_SPLIT
      printf("SplitOnly: event %d = event %d - maxDiag %d\n",
          events[k], tmp, maxDiag);
#endif
    } 

    // Add maxDiag to events[level(maxDiag)]
    int maxDiagLevel = ABS(mxdF->getNodeLevel(maxDiag));
    int tmp = events[maxDiagLevel];
    events[maxDiagLevel] = mxdUnion->compute(maxDiag, events[maxDiagLevel]);
    mxdF->unlinkNode(tmp);
    mxdF->unlinkNode(maxDiag);

    // Subtract events[maxDiagLevel] from events[k].
    // Do this only for SplitSubtract. SplitSubtractAll is handled later.
    if (split == SplitSubtract) {
      int tmp = events[k];
      events[k] = mxdDifference->compute(events[k], events[maxDiagLevel]);
      mxdF->unlinkNode(tmp);
#ifdef DEBUG_FINALIZE_SPLIT
      printf("SplitSubtract: event %d = event %d - event[maxDiagLevel] %d\n",
          events[k], tmp, maxDiag);
#endif
    }

  } // for k

  if (split == SplitSubtractAll) {
    // Subtract event[i] from all event[j], where j > i.
    for (int i = 1; i < K; i++) {
      for (int j = i + 1; j <= K; j++) {
        if (events[i] && events[j]) {
          int tmp = events[j];
          events[j] = mxdDifference->compute(events[j], events[i]);
          mxdF->unlinkNode(tmp);
#ifdef DEBUG_FINALIZE_SPLIT
          printf("SplitSubtractAll: event %d = event %d - event %d\n",
              events[j], tmp, events[i]);
#endif
        }
      }
    }
  }

#ifdef DEBUG_FINALIZE_SPLIT
  printf("After splitting events in finalize()\n");
  printf("events array: [");
  for (int i=0; i<=K; i++) {
    if (i) printf(", ");
    printf("%d", events[i]);
  }
  printf("]\n");
#endif
}


void
MEDDLY::satpregen_opname::pregen_relation
::unionLevels()
{
  if (K < 1) return;

  binary_operation* mxdUnion = getOperation(UNION, mxdF, mxdF, mxdF);
  MEDDLY_DCASSERT(mxdUnion);

  node_handle u = events[1];
  events[1] = 0;

  for (int k = 2; k <= K; k++) {
    if (0 == events[k]) continue;
    node_handle temp = mxdUnion->compute(u, events[k]);
    mxdF->unlinkNode(events[k]);
    events[k] = 0;
    mxdF->unlinkNode(u);
    u = temp;
  }

  events[mxdF->getNodeLevel(u)] = u;
}


void
MEDDLY::satpregen_opname::pregen_relation
::finalize(splittingOption split)
{
  if (0==level_index) {
    // by levels
    switch (split) {
      case SplitOnly: printf("Split: SplitOnly\n"); break;
      case SplitSubtract: printf("Split: SplitSubtract\n"); break;
      case SplitSubtractAll: printf("Split: SplitSubtractAll\n"); break;
      case MonolithicSplit: printf("Split: MonolithicSplit\n"); break;
      default: printf("Split: None\n");
    }
    splitMxd(split);
    if (split != None && split != MonolithicSplit) {
#ifdef DEBUG_FINALIZE_SPLIT
      // Union the elements, and then re-run.
      // Result must be the same as before.
      node_handle* old_events = new node_handle[K+1];
      for(int k = 0; k <= K; k++) {
        old_events[k] = events[k];
        if (old_events[k]) mxdF->linkNode(old_events[k]);
      }
      splitMxd(MonolithicSplit);
      binary_operation* mxdDifference =
        getOperation(DIFFERENCE, mxdF, mxdF, mxdF);
      MEDDLY_DCASSERT(mxdDifference);
      for(int k = 0; k <= K; k++) {
        if (old_events[k] != events[k]) {
          node_handle diff1 = mxdDifference->compute(old_events[k], events[k]);
          node_handle diff2 = mxdDifference->compute(events[k], old_events[k]);
          printf("error at level %d, n:k %d:%d, %d:%d\n",
              k,
              diff1, mxdF->getNodeLevel(diff1),
              diff2, mxdF->getNodeLevel(diff2)
              );
          mxdF->unlinkNode(diff1);
          mxdF->unlinkNode(diff2);
        }
        if (old_events[k]) mxdF->unlinkNode(old_events[k]);
      }

      delete [] old_events;
#endif
    }
    return; 
  }

#ifdef DEBUG_FINALIZE
  printf("Finalizing pregen relation\n");
  printf("%d events total\n", last_event+1);
  printf("events array: [");
  for (int i=0; i<=last_event; i++) {
    if (i) printf(", ");
    printf("%d", events[i]);
  }
  printf("]\n");
  printf("next array: [");
  for (int i=0; i<=last_event; i++) {
    if (i) printf(", ");
    printf("%d", next[i]);
  }
  printf("]\n");
  printf("level_index array: [%d", level_index[1]);
  for (int i=2; i<=K; i++) {
    printf(", %d", level_index[i]);
  }
  printf("]\n");
#endif

  // convert from array of linked lists to contiguous array.
  defragLists(events, next, level_index, K);

  // done with next pointers
  delete[] next;
  next = 0;

#ifdef DEBUG_FINALIZE
  printf("\nAfter finalization\n");
  printf("events array: [");
  for (int i=0; i<=last_event; i++) {
    if (i) printf(", ");
    printf("%d", events[i]);
  }
  printf("]\n");
  printf("level_index array: [%d", level_index[1]);
  for (int i=2; i<=K; i++) {
    printf(", %d", level_index[i]);
  }
  printf("]\n");
#endif
}


// ******************************************************************
// *                                                                *
// *                     satotf_opname  methods                     *
// *                                                                *
// ******************************************************************

MEDDLY::satotf_opname::satotf_opname(const char* n)
 : specialized_opname(n)
{
}

MEDDLY::satotf_opname::~satotf_opname()
{
}

// ============================================================

MEDDLY::satotf_opname::subfunc::subfunc(int* v, int nv)
{
  vars = v;
  num_vars = nv;
}

MEDDLY::satotf_opname::subfunc::~subfunc()
{
  delete[] vars;
}

// ============================================================

MEDDLY::satotf_opname::event::event(subfunc** p, int np)
{
  pieces = p;
  num_pieces = np;
}

MEDDLY::satotf_opname::event::~event()
{
  for (int i=0; i<num_pieces; i++) delete pieces[i];
  delete[] pieces;
}

void MEDDLY::satotf_opname::event::rebuild(dd_edge &e)
{
  throw error(error::NOT_IMPLEMENTED);
}

// ============================================================

MEDDLY::satotf_opname::otf_relation::otf_relation(forest* inmdd, 
  forest* mxd, forest* outmdd, event** E, int ne)
{
  // Set forests
  insetF = inmdd;
  mxdF = mxd;
  outsetF = outmdd;

  if (0==insetF || 0==outsetF || 0==mxdF) throw error(error::MISCELLANEOUS);

  // Check for same domain
  if (  
    (insetF->getDomain() != mxdF->getDomain()) || 
    (outsetF->getDomain() != mxdF->getDomain()) 
  )
    throw error(error::DOMAIN_MISMATCH);

  // for now, anyway, inset and outset must be same forest
  if (insetF != outsetF)
    throw error(error::FOREST_MISMATCH);

  // Check forest types
  if (
    insetF->isForRelations()    ||
    !mxdF->isForRelations()     ||
    outsetF->isForRelations()   ||
    (insetF->getRangeType() != mxdF->getRangeType())        ||
    (outsetF->getRangeType() != mxdF->getRangeType())       ||
    (insetF->getEdgeLabeling() != forest::MULTI_TERMINAL)   ||
    (outsetF->getEdgeLabeling() != forest::MULTI_TERMINAL)  ||
    (mxdF->getEdgeLabeling() != forest::MULTI_TERMINAL)     ||
    (outsetF->getEdgeLabeling() != forest::MULTI_TERMINAL)
  )
    throw error(error::TYPE_MISMATCH);

  // Forests are good; set number of variables
  K = mxdF->getDomain()->getNumVariables();

  // Events
  events = E;
  num_events = ne;

  // preprocess events
  // (1) determine total size of pieces array
  int piecesLength = 0;
  for (int i=0; i<num_events; i++) if (events[i]) {
    const subfunc* const* PL = events[i]->getPieces();
    for (int p=events[i]->getNumPieces()-1; p>=0; p--) {
      piecesLength += PL[p]->getNumVars();
    }
  }
  // (2) build empty list for each level
  piecesForLevel = new int[K+1];
  for (int k=0; k<=K; k++) piecesForLevel[k] = -1;
  // (3) for each piece, add it to the appropriate lists
  pieces = new subfunc*[piecesLength];
  int* nextPiece = new int[piecesLength];
  int pptr = 0;
  for (int i=0; i<num_events; i++) if (events[i]) {
    subfunc** PL = events[i]->getPieces();
    for (int p=events[i]->getNumPieces()-1; p>=0; p--) {
      for (int v=PL[p]->getNumVars()-1; v>=0; v--) {
        int k = PL[p]->getVars()[v];
        // add PL[p] to list k
        pieces[pptr] = PL[p];
        nextPiece[pptr] = piecesForLevel[k];
        piecesForLevel[k] = pptr;
        pptr++;
      } // for v
      piecesLength += PL[p]->getNumVars();
    } // for p
  } // for i
  // (4) defragment lists
  defragLists(pieces, nextPiece, piecesForLevel, K);
  // (5) cleanup
  delete[] nextPiece;

  // TBD - debug here
}

MEDDLY::satotf_opname::otf_relation::~otf_relation()
{
  for (int i=0; i<num_events; i++) delete events[i];
  delete[] events;

  // DO NOT delete pieces[i] pointers, they're copies
  delete[] pieces;
  delete[] piecesForLevel;
}

// ******************************************************************
// *                       operation  methods                       *
// ******************************************************************

MEDDLY::operation::operation(const opname* n, int kl, int al)
{
#ifdef DEBUG_CLEANUP
  fprintf(stdout, "Creating operation %p\n", this);
  fflush(stdout);
#endif
  MEDDLY_DCASSERT(kl>=0);
  MEDDLY_DCASSERT(al>=0);
  theOpName = n;
  key_length = kl;
  ans_length = al;
  is_marked_for_deletion = false;
  next = 0;
  discardStaleHits = true;

  // 
  // assign an index to this operation
  //
  if (free_list>=0) {
    oplist_index = free_list;
    free_list = op_holes[free_list];
  } else {
    if (list_size >= list_alloc) {
      int nla = list_alloc + 256;
      op_list = (operation**) realloc(op_list, nla * sizeof(void*));
      op_holes = (int*) realloc(op_holes, nla * sizeof(int));
      if (0==op_list || 0==op_holes) throw error(error::INSUFFICIENT_MEMORY);
      for (int i=list_size; i<list_alloc; i++) {
        op_list[i] = 0;
        op_holes[i] = -1;
      }
      list_alloc = nla;
    }
    oplist_index = list_size;
    list_size++;
  }
  op_list[oplist_index] = this;

  if (key_length) { // this op uses the CT
    // 
    // Initialize CT 
    //
    if (Monolithic_CT)
      CT = Monolithic_CT;
    else {
      const compute_table_style* CTsty = meddlySettings.ctSettings.style;
      MEDDLY_DCASSERT(CTsty);
      CT = CTsty->create(meddlySettings.ctSettings, this);
    }

    //
    // Initialize CT search structure
    //
    // CTsrch = CT->initializeSearchKey(this);

  } else {
    MEDDLY_DCASSERT(0==ans_length);
    CT = 0;
    // CTsrch = 0;
  }
  CT_free_keys = 0;
}

MEDDLY::operation::~operation()
{
#ifdef DEBUG_CLEANUP
  fprintf(stdout, "Deleting operation %p %s\n", this, getName());
  fflush(stdout);
#endif

  while (CT_free_keys) {
    compute_table::search_key* next = CT_free_keys->next;
    delete CT_free_keys;
    CT_free_keys = next;
  }

  // delete CTsrch;
  if (CT && (CT!=Monolithic_CT)) delete CT;
  // delete next;  // Seriously, WTF?
  if (oplist_index >= 0) {
    MEDDLY_DCASSERT(op_list[oplist_index] == this);
    op_list[oplist_index] = 0;
    op_holes[oplist_index] = free_list;
    free_list = oplist_index;
  }
#ifdef DEBUG_CLEANUP
  fprintf(stdout, "Deleted operation %p %s\n", this, getName());
  fflush(stdout);
#endif
}

void MEDDLY::operation::removeStalesFromMonolithic()
{
  if (Monolithic_CT) Monolithic_CT->removeStales();
}

void MEDDLY::operation::markForDeletion()
{
#ifdef DEBUG_CLEANUP
  fprintf(stdout, "Marking operation %p %s for deletion\n", this, getName());
  fflush(stdout);
#endif
  if (is_marked_for_deletion) return;
  is_marked_for_deletion = true;
  if (CT && CT->isOperationTable()) CT->removeStales();
}

void MEDDLY::operation::destroyAllOps() 
{
  for (int i=0; i<list_size; i++) delete op_list[i];
  free(op_list);
  free(op_holes);
  op_list = 0;
  op_holes = 0;
  list_size = 0;
  list_alloc = 0;
  free_list = -1;
}

void MEDDLY::operation::removeStaleComputeTableEntries()
{
  if (CT) CT->removeStales();
}

void MEDDLY::operation::removeAllComputeTableEntries()
{
#ifdef DEBUG_CLEANUP
  fprintf(stdout, "Removing entries for operation %p %s\n", this, getName());
  fflush(stdout);
#endif
  if (is_marked_for_deletion) return;
  is_marked_for_deletion = true;
  if (CT) CT->removeStales();
  is_marked_for_deletion = false;
#ifdef DEBUG_CLEANUP
  fprintf(stdout, "Removed entries for operation %p %s\n", this, getName());
  fflush(stdout);
#endif
}

void MEDDLY::operation::showMonolithicComputeTable(output &s, int verbLevel)
{
  if (Monolithic_CT) Monolithic_CT->show(s, verbLevel);
}

void MEDDLY::operation::showAllComputeTables(output &s, int verbLevel)
{
  if (Monolithic_CT) {
    Monolithic_CT->show(s, verbLevel);
    return;
  }
  for (int i=0; i<list_size; i++) 
    if (op_list[i]) {
      op_list[i]->showComputeTable(s, verbLevel);
    }
}

void MEDDLY::operation::showComputeTable(output &s, int verbLevel) const
{
  if (CT) CT->show(s, verbLevel);
}

// ******************************************************************
// *                    unary_operation  methods                    *
// ******************************************************************

MEDDLY::unary_operation::unary_operation(const unary_opname* code, int kl, 
  int al, expert_forest* arg, expert_forest* res) : operation(code, kl, al)
{
  argF = arg;
  resultType = FOREST;
  resF = res;

  registerInForest(argF);
  registerInForest(resF);

  setAnswerForest(resF);
}

MEDDLY::unary_operation::unary_operation(const unary_opname* code, int kl, 
  int al, expert_forest* arg, opnd_type res) : operation(code, kl, al)
{
  argF = arg;
  resultType = res;
  resF = 0;

  registerInForest(argF);

  setAnswerForest(0);
}

MEDDLY::unary_operation::~unary_operation()
{
  unregisterInForest(argF);
  unregisterInForest(resF);
}

void MEDDLY::unary_operation::compute(const dd_edge &arg, dd_edge &res)
{
  throw error(error::TYPE_MISMATCH);
}

void MEDDLY::unary_operation::compute(const dd_edge &arg, long &res)
{
  throw error(error::TYPE_MISMATCH);
}

void MEDDLY::unary_operation::compute(const dd_edge &arg, double &res)
{
  throw error(error::TYPE_MISMATCH);
}

void MEDDLY::unary_operation::compute(const dd_edge &arg, ct_object &c)
{
  throw error(error::TYPE_MISMATCH);
}

// ******************************************************************
// *                    binary_operation methods                    *
// ******************************************************************

MEDDLY::binary_operation::binary_operation(const binary_opname* op, int kl, 
  int al, expert_forest* arg1, expert_forest* arg2, expert_forest* res)
: operation(op, kl, al)
{
  arg1F = arg1;
  arg2F = arg2;
  resF = res;

  registerInForest(arg1F);
  registerInForest(arg2F);
  registerInForest(resF);

  setAnswerForest(resF);
  can_commute = false;
}

MEDDLY::binary_operation::~binary_operation()
{
  unregisterInForest(arg1F);
  unregisterInForest(arg2F);
  unregisterInForest(resF);
}

MEDDLY::node_handle 
MEDDLY::binary_operation::compute(node_handle a, node_handle b)
{
  throw error(error::WRONG_NUMBER);
}

MEDDLY::node_handle 
MEDDLY::binary_operation::compute(int k, node_handle a, node_handle b)
{
  throw error(error::WRONG_NUMBER);
}

void MEDDLY::binary_operation::compute(int av, node_handle ap,
  int bv, node_handle bp, int &cv, node_handle &cp)
{
  throw error(error::WRONG_NUMBER);
}

void MEDDLY::binary_operation::compute(float av, node_handle ap,
  float bv, node_handle bp, float &cv, node_handle &cp)
{
  throw error(error::WRONG_NUMBER);
}

// ******************************************************************
// *                 specialized_operation  methods                 *
// ******************************************************************

MEDDLY::
specialized_operation::
specialized_operation(const specialized_opname* op, int kl, int al) 
 : operation(op, kl, al)
{
}

MEDDLY::specialized_operation::~specialized_operation()
{
}

void MEDDLY::specialized_operation::compute(const dd_edge &arg, dd_edge &res)
{
  throw error(error::TYPE_MISMATCH);
}

void MEDDLY::specialized_operation::compute(const dd_edge &ar1, 
  const dd_edge &ar2, dd_edge &res)
{
  throw error(error::TYPE_MISMATCH);
}

void MEDDLY::specialized_operation::compute(double* y, const double* x)
{
  throw error(error::TYPE_MISMATCH);
}


// ******************************************************************
// *                     op_initializer methods                     *
// ******************************************************************

MEDDLY::op_initializer::op_initializer(op_initializer* bef)
{
  refcount = 1;
  before = bef;
}

MEDDLY::op_initializer::~op_initializer()
{
  MEDDLY_DCASSERT(0==refcount);
  delete before;
}

void MEDDLY::op_initializer::initChain(const settings &s)
{
  if (before) before->initChain(s);
  init(s);
}

void MEDDLY::op_initializer::cleanupChain()
{
  cleanup();
  if (before) before->cleanupChain();
}

