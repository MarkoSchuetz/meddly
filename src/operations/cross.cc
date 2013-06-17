
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "../defines.h"
#include "cross.h"

// #define TRACE_ALL_OPS
// #define DEBUG_CROSS

namespace MEDDLY {
  class cross_bool;
  class cross_opname;
};

// ******************************************************************
// *                                                                *
// *                        cross_bool class                        *
// *                                                                *
// ******************************************************************

class MEDDLY::cross_bool : public binary_operation {
    static const int INPTR_INDEX = 0;
    static const int LEVEL_INDEX = 1;
    static const int OPNDA_INDEX = 2;
    static const int OPNDB_INDEX = 3;
    static const int RESLT_INDEX = 4;
  public:
    cross_bool(const binary_opname* oc, expert_forest* a1,
      expert_forest* a2, expert_forest* res);

    virtual bool isStaleEntry(const int* entryData);
    virtual void discardEntry(const int* entryData);
    virtual void showEntry(FILE* strm, const int *entryData) const;
    virtual void compute(const dd_edge& a, const dd_edge& b, dd_edge &c);

    long compute_pr(int in, int ht, long a, long b);
    long compute_un(int ht, long a, long b);
};

MEDDLY::cross_bool::cross_bool(const binary_opname* oc, expert_forest* a1,
  expert_forest* a2, expert_forest* res) 
: binary_operation(oc, 4, 1, a1, a2, res)
{
  // data[INPTR_INDEX] : inpointer
  // data[LEVEL_INDEX] : level
  // data[OPNDA_INDEX] : a
  // data[OPNDB_INDEX] : b
  // data[RESLT_INDEX] : c
}

bool MEDDLY::cross_bool::isStaleEntry(const int* data)
{
  // data[0] is the level number
  return arg1F->isStale(data[OPNDA_INDEX]) ||
         arg2F->isStale(data[OPNDB_INDEX]) ||
         resF->isStale(data[RESLT_INDEX]);
}

void MEDDLY::cross_bool::discardEntry(const int* data)
{
  // data[0] is the level number
  arg1F->uncacheNode(data[OPNDA_INDEX]);
  arg2F->uncacheNode(data[OPNDB_INDEX]);
  resF->uncacheNode(data[RESLT_INDEX]);
}

void
MEDDLY::cross_bool ::showEntry(FILE* strm, const int *data) const
{
  fprintf(strm, "[%s(in: %d, level: %d, %d, %d): %d]", 
    getName(), data[0], data[1], data[2], data[3], data[4]
  );
}

void
MEDDLY::cross_bool::compute(const dd_edge &a, const dd_edge &b, dd_edge &c)
{
  int L = arg1F->getDomain()->getNumVariables();
  long cnode = compute_un(L, a.getNode(), b.getNode());
  c.set(cnode, 0);
}

long MEDDLY::cross_bool::compute_un(int k, long a, long b)
{
#ifdef DEBUG_CROSS
  printf("calling compute_un(%d, %d, %d)\n", k, a, b);
#endif
  MEDDLY_DCASSERT(k>=0);
  if (0==a || 0==b) return 0;
  if (0==k) {
    return resF->getTerminalNode(
      arg1F->getBoolean(a)
    );
  }

  // check compute table
  CTsrch.key(INPTR_INDEX) = -1;
  CTsrch.key(LEVEL_INDEX) = k;
  CTsrch.key(OPNDA_INDEX) = a;
  CTsrch.key(OPNDB_INDEX) = b;
  const int* cacheFind = CT->find(CTsrch);
  if (cacheFind) {
    return resF->linkNode(cacheFind[RESLT_INDEX]);
  }

  // build new result node
  int resultSize = resF->getLevelSize(k);
  node_builder& nb = resF->useNodeBuilder(k, resultSize);

  // Initialize node reader
  node_reader* A = (arg1F->getNodeLevel(a) < k) 
    ? arg1F->initRedundantReader(k, a, true)
    : arg1F->initNodeReader(a, true);

  // recurse
  for (int i=0; i<resultSize; i++) {
    nb.d(i) = compute_pr(i, -k, A->d(i), b);
  }

  // cleanup node reader
  node_reader::recycle(A);

  // reduce, save in compute table
  long c = resF->createReducedNode(-1, nb);

  compute_table::temp_entry &entry = CT->startNewEntry(this);
  entry.key(INPTR_INDEX) = -1;
  entry.key(LEVEL_INDEX) = k;
  entry.key(OPNDA_INDEX) = arg1F->cacheNode(a);
  entry.key(OPNDB_INDEX) = arg2F->cacheNode(b);
  entry.result(0) = resF->cacheNode(c);
  CT->addEntry();

#ifdef TRACE_ALL_OPS
  printf("computed %s(%d, %d, %d) = %d\n", getName(), k, a, b, c);
#endif

  return c;
}

long MEDDLY::cross_bool::compute_pr(int in, int k, long a, long b)
{
#ifdef DEBUG_CROSS
  printf("calling compute_pr(%d, %d, %d, %d)\n", in, k, a, b);
#endif
  MEDDLY_DCASSERT(k<0);
  if (0==a || 0==b) return 0;

  // check compute table
  CTsrch.key(INPTR_INDEX) = in;
  CTsrch.key(LEVEL_INDEX) = k;
  CTsrch.key(OPNDA_INDEX) = a;
  CTsrch.key(OPNDB_INDEX) = b;
  const int* cacheFind = CT->find(CTsrch);
  if (cacheFind) {
    return resF->linkNode(cacheFind[RESLT_INDEX]);
  }

  // build new result node
  int resultSize = resF->getLevelSize(k);
  node_builder& nb = resF->useNodeBuilder(k, resultSize);

  // Initialize node reader
  node_reader* B = (arg2F->getNodeLevel(b) < -k) 
    ? arg2F->initRedundantReader(-k, b, true)
    : arg2F->initNodeReader(b, true);

  // recurse
  for (int i=0; i<resultSize; i++) {
    nb.d(i) = compute_un(-(k+1), a, B->d(i));
  }

  // cleanup node reader
  node_reader::recycle(B);

  // reduce, save in compute table
  long c = resF->createReducedNode(in, nb);

  compute_table::temp_entry &entry = CT->startNewEntry(this);
  entry.key(INPTR_INDEX) = in;
  entry.key(LEVEL_INDEX) = k;
  entry.key(OPNDA_INDEX) = arg1F->cacheNode(a);
  entry.key(OPNDB_INDEX) = arg2F->cacheNode(b);
  entry.result(0) = resF->cacheNode(c);
  CT->addEntry();

#ifdef TRACE_ALL_OPS
  printf("computed %s((%d), %d, %d, %d) = %d\n", getName(), in, k, a, b, c);
#endif

  return c;
}


// ******************************************************************
// *                                                                *
// *                       cross_opname class                       *
// *                                                                *
// ******************************************************************

class MEDDLY::cross_opname : public binary_opname {
  public:
    cross_opname();
    virtual binary_operation* buildOperation(expert_forest* a1, 
      expert_forest* a2, expert_forest* r) const;
};

MEDDLY::cross_opname::cross_opname()
 : binary_opname("Cross")
{
}

MEDDLY::binary_operation* 
MEDDLY::cross_opname::buildOperation(expert_forest* a1, expert_forest* a2, 
  expert_forest* r) const
{
  if (0==a1 || 0==a2 || 0==r) return 0;

  if (  
    (a1->getDomain() != r->getDomain()) || 
    (a2->getDomain() != r->getDomain()) 
  )
    throw error(error::DOMAIN_MISMATCH);

  if (
    a1->isForRelations()  ||
    (a1->getRangeType() != forest::BOOLEAN) ||
    (a1->getEdgeLabeling() != forest::MULTI_TERMINAL) ||
    a2->isForRelations()  ||
    (a2->getRangeType() != forest::BOOLEAN) ||
    (a2->getEdgeLabeling() != forest::MULTI_TERMINAL) ||
    (!r->isForRelations())  ||
    (r->getRangeType() != forest::BOOLEAN) ||
    (r->getEdgeLabeling() != forest::MULTI_TERMINAL)
  )
    throw error(error::TYPE_MISMATCH);

  return new cross_bool(this, a1, a2, r);
}

// ******************************************************************
// *                                                                *
// *                           Front  end                           *
// *                                                                *
// ******************************************************************

MEDDLY::binary_opname* MEDDLY::initializeCross(const settings &s)
{
  return new cross_opname;
}

