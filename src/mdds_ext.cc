
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



// TODO: Completed implementing ALL functions in mdds_ext.h"
// TODO: need to call node_manager:: functions?
//
// TODO: Test createNode variant to build a sparse node for:
//       mdds (done), mxd, evmdd, mtmdd

#include "mdds_ext.h"


#if 0
bool isMtmdd(const expert_forest* f)
{
  return !f->isForRelations() &&
         f->getRangeType() == forest::INTEGER &&
         f->getEdgeLabeling() == forest::MULTI_TERMINAL;
}

bool isMdd(const expert_forest* f)
{
  return !f->isForRelations() &&
         f->getRangeType() == forest::BOOLEAN &&
         f->getEdgeLabeling() == forest::MULTI_TERMINAL;
}

bool isMxd(const expert_forest* f)
{
  return f->isForRelations() &&
         f->getRangeType() == forest::BOOLEAN &&
         f->getEdgeLabeling() == forest::MULTI_TERMINAL;
}

bool isEvmdd(const expert_forest* f)
{
  return !f->isForRelations() &&
         f->getRangeType() == forest::INTEGER &&
         f->getEdgeLabeling() == forest::EVPLUS;
}
#endif


// ********************************** MTMDDs **********************************

mtmdd_node_manager::mtmdd_node_manager(domain *d, forest::range_type t)
: node_manager(d, false, t,
      forest::MULTI_TERMINAL, forest::FULLY_REDUCED,
      forest::FULL_OR_SPARSE_STORAGE, OPTIMISTIC_DELETION)
{ }


mtmdd_node_manager::~mtmdd_node_manager()
{ }


int mtmdd_node_manager::reduceNode(int p)
{
  DCASSERT(isActiveNode(p));

  if (isReducedNode(p)) return p; 

  DCASSERT(!isTerminalNode(p));
  DCASSERT(isFullNode(p));
  DCASSERT(getInCount(p) == 1);

#if 0
  validateIncounts();
#endif

  int size = getFullNodeSize(p);
  int* ptr = getFullNodeDownPtrs(p);
  int node_level = getNodeLevel(p);

  decrTempNodeCount(node_level);

#ifdef DEVELOPMENT_CODE
  int node_height = getNodeHeight(p);
  for (int i=0; i<size; i++) {
    assert(isReducedNode(ptr[i]));
    assert(getNodeHeight(ptr[i]) < node_height);
  }
#endif

  // quick scan: is this node zero?
  int nnz = 0;
  int truncsize = 0;
  for (int i = 0; i < size; ++i) {
    if (0 != ptr[i]) {
      nnz++;
      truncsize = i;
    }
  }
  truncsize++;

  if (0 == nnz) {
    // duplicate of 0
    deleteTempNode(p);
#ifdef TRACE_REDUCE
    printf("\tReducing %d, got 0\n", p);
#endif
    return 0;
  }

  // check for possible reductions
  if (reductionRule == forest::FULLY_REDUCED) {
    if (nnz == getLevelSize(node_level)) {
      int i = 1;
      for ( ; i < size && ptr[i] == ptr[0]; i++);
      if (i == size ) {
        // for all i, ptr[i] == ptr[0]
        int temp = sharedCopy(ptr[0]);
        deleteTempNode(p);
        return temp;
      }
    }
  } else {
    // Quasi-reduced -- no identity reduction for MDDs/MTMDDs
    DCASSERT(reductionRule == forest::QUASI_REDUCED);

    // ensure than all downpointers are pointing to nodes exactly one
    // level below or zero.
    for (int i = 0; i < size; ++i)
    {
      if (ptr[i] == 0) continue;
      if (getNodeLevel(ptr[i]) != (node_level - 1)) {
        int temp = ptr[i];
        ptr[i] = buildQuasiReducedNodeAtLevel(node_level - 1, ptr[i]);
        unlinkNode(temp);
      }
      DCASSERT(ptr[i] == 0 || (getNodeLevel(ptr[i]) == node_level - 1));
    }
  }

  // check unique table
  int q = find(p);
  if (getNull() != q) {
    // duplicate found
#ifdef TRACE_REDUCE
    printf("\tReducing %d, got %d\n", p, q);
#endif
    deleteTempNode(p);
    return sharedCopy(q);
  }

  // insert into unique table
  insert(p);

#ifdef TRACE_REDUCE
  printf("\tReducing %d: unique, compressing\n", p);
#endif

  if (!areSparseNodesEnabled())
    nnz = size;

  // right now, tie goes to truncated full.
  if (2*nnz < truncsize) {
    // sparse is better; convert
    int newoffset = getHole(node_level, 4+2*nnz, true);
    // can't rely on previous ptr, re-point to p
    int* full_ptr = getNodeAddress(p);
    int* sparse_ptr = getAddress(node_level, newoffset);
    // copy first 2 integers: incount, next
    sparse_ptr[0] = full_ptr[0];
    sparse_ptr[1] = full_ptr[1];
    // size
    sparse_ptr[2] = -nnz;
    // copy index into address[]
    sparse_ptr[3 + 2*nnz] = p;
    // get pointers to the new sparse node
    int* indexptr = sparse_ptr + 3;
    int* downptr = indexptr + nnz;
    ptr = full_ptr + 3;
    // copy downpointers
    for (int i=0; i<size; i++, ++ptr) {
      if (*ptr) {
        *indexptr = i;
        *downptr = *ptr;
        ++indexptr;
        ++downptr;
      }
    }
    // trash old node
#ifdef MEMORY_TRACE
    int saved_offset = getNodeOffset(p);
    setNodeOffset(p, newoffset);
    makeHole(node_level, saved_offset, 4 + size);
#else
    makeHole(node_level, getNodeOffset(p), 4 + size);
    setNodeOffset(p, newoffset);
#endif
    // address[p].cache_count does not change
  } else {
    // full is better
    if (truncsize<size) {
      // truncate the trailing 0s
      int newoffset = getHole(node_level, 4+truncsize, true);
      // can't rely on previous ptr, re-point to p
      int* full_ptr = getNodeAddress(p);
      int* trunc_ptr = getAddress(node_level, newoffset);
      // copy first 2 integers: incount, next
      trunc_ptr[0] = full_ptr[0];
      trunc_ptr[1] = full_ptr[1];
      // size
      trunc_ptr[2] = truncsize;
      // copy index into address[]
      trunc_ptr[3 + truncsize] = p;
      // elements
      memcpy(trunc_ptr + 3, full_ptr + 3, truncsize * sizeof(int));
      // trash old node
#ifdef MEMORY_TRACE
      int saved_offset = getNodeOffset(p);
      setNodeOffset(p, newoffset);
      makeHole(node_level, saved_offset, 4 + size);
#else
      makeHole(node_level, getNodeOffset(p), 4 + size);
      setNodeOffset(p, newoffset);
#endif
      // address[p].cache_count does not change
    }
  }

  // address[p].cache_count does not change
  DCASSERT(getCacheCount(p) == 0);
  // Sanity check that the hash value is unchanged
  DCASSERT(find(p) == p);

#if 0
  validateIncounts();
#endif

  return p;
}


int mtmdd_node_manager::createNode(int k, int index, int dptr)
{
  DCASSERT(index >= -1);

#if 1
  if (index > -1 && getLevelSize(k) <= index) {
    //printf("level: %d, curr size: %d, index: %d, ",
    //k, getLevelSize(k), index);
    expertDomain->enlargeVariableBound(k, index + 1);
    //printf("new size: %d\n", getLevelSize(k));
  }
#endif


  if (dptr == 0) return 0;
  if (index == -1) {
    // all downpointers should point to dptr
    if (reductionRule == forest::FULLY_REDUCED) return sharedCopy(dptr);
    int curr = createTempNodeMaxSize(k, false);
    setAllDownPtrsWoUnlink(curr, dptr);
    return reduceNode(curr);
  }

#if 0

  // a single downpointer points to dptr
  int curr = createTempNode(k, index + 1);
  setDownPtrWoUnlink(curr, index, dptr);
  return reduceNode(curr);

#else

  // a single downpointer points to dptr
  if (nodeStorage == FULL_STORAGE ||
      (nodeStorage == FULL_OR_SPARSE_STORAGE && index < 2)) {
    // Build a full node
    int curr = createTempNode(k, index + 1);
    setDownPtrWoUnlink(curr, index, dptr);
    return reduceNode(curr);
  }
  else {
    DCASSERT (nodeStorage == SPARSE_STORAGE ||
        (nodeStorage == FULL_OR_SPARSE_STORAGE && index >= 2));
    // Build a sparse node
    int p = createTempNode(k, 2);
    int* nodeData = getNodeAddress(p);
    // For sparse nodes, size is -ve
    nodeData[2] = -1;
    // indexes followed by downpointers -- here we have one index and one dptr
    nodeData[3] = index;
    nodeData[4] = sharedCopy(dptr);
    // search in unique table
    int q = find(p);
    if (getNull() == q) {
      // no duplicate found; insert into unique table
      insert(p);
      DCASSERT(getCacheCount(p) == 0);
      DCASSERT(find(p) == p);
    }
    else {
      // duplicate found; discard this node and return the duplicate
      // revert to full temp node before discarding
      nodeData[2] = 2;
      nodeData[3] = 0;
      nodeData[4] = 0;
      unlinkNode(dptr);
      deleteTempNode(p);
      p = sharedCopy(q);
    }
    decrTempNodeCount(k);
    return p;
  }

#endif

}


void mtmdd_node_manager::createEdge(const int* v, int term, dd_edge& e)
{
  // construct the edge bottom-up
  const int* h2l_map = expertDomain->getHeightsToLevelsMap();
  int h_sz = expertDomain->getNumVariables() + 1;
  DCASSERT(isTerminalNode(term));
  int result = term;
  int curr = 0;
  for (int i=1; i<h_sz; i++) {
    curr = createNode(h2l_map[i], v[h2l_map[i]], result);
    unlinkNode(result);
    result = curr;
  }
  e.set(result, 0, getNodeLevel(result));
  // e.show(stderr, 2);
}


forest::error mtmdd_node_manager::createEdge(const int* const* vlist,
    const int* terms, int N, dd_edge& e)
{
  if (e.getForest() != this) return forest::INVALID_OPERATION;
  if (vlist == 0 || terms == 0 || N <= 0) return forest::INVALID_VARIABLE;

  createEdge(vlist[0], getTerminalNode(terms[0]), e);
  if (N > 1) {
    dd_edge curr(this);
    for (int i=1; i<N; i++) {
      createEdge(vlist[i], getTerminalNode(terms[i]), curr);
      e += curr;
    }
  }

  return forest::SUCCESS;
}


forest::error mtmdd_node_manager::createEdge(int term, dd_edge& e)
{
  if (e.getForest() != this) return forest::INVALID_OPERATION;
  term = getTerminalNode(term);

  if (reductionRule == forest::FULLY_REDUCED) {
    e.set(term, 0, domain::TERMINALS);
    return forest::SUCCESS;
  }

  // construct the edge bottom-up
  const int* h2l_map = expertDomain->getHeightsToLevelsMap();
  int h_sz = expertDomain->getNumVariables() + 1;
  int result = term;
  int curr = 0;
  for (int i=1; i<h_sz; i++) {
    curr = createTempNodeMaxSize(h2l_map[i], false);
    setAllDownPtrsWoUnlink(curr, result);
    unlinkNode(result);
    result = reduceNode(curr);
  }
  e.set(result, 0, getNodeLevel(result));

  return forest::SUCCESS;
}


forest::error mtmdd_node_manager::evaluate(const dd_edge &f,
    const int* vlist, int &term) const
{
  // assumption: vlist does not contain any special values (-1, -2, etc).
  // vlist contains a single element.
  term = f.getNode();
  const int* h2l_map = expertDomain->getHeightsToLevelsMap();
  while (!isTerminalNode(term)) {
    term = getDownPtr(term, vlist[h2l_map[getNodeHeight(term)]]);
  }
  term = getInteger(term);
  return forest::SUCCESS;
}


void mtmdd_node_manager::normalizeAndReduceNode(int& p, int& ev)
{
  assert(false);
}


forest::error mtmdd_node_manager::createEdge(const int* const* vlist, int N,
    dd_edge &e)
{
  return forest::INVALID_OPERATION;
}


forest::error mtmdd_node_manager::createEdge(const int* const* vlist,
    const float* terms, int N, dd_edge& e)
{
  if (e.getForest() != this) return forest::INVALID_OPERATION;
  if (vlist == 0 || terms == 0 || N <= 0) return forest::INVALID_VARIABLE;

  createEdge(vlist[0], getTerminalNode(terms[0]), e);
  if (N > 1) {
    dd_edge curr(this);
    for (int i=1; i<N; i++) {
      createEdge(vlist[i], getTerminalNode(terms[i]), curr);
      e += curr;
    }
  }

  return forest::SUCCESS;
}


forest::error mtmdd_node_manager::createEdge(const int* const* vlist,
    const int* const* vplist, int N, dd_edge &e)
{
  return forest::INVALID_OPERATION;
}


forest::error mtmdd_node_manager::createEdge(const int* const* vlist,
    const int* const* vplist, const int* terms, int N, dd_edge &e)
{
  return forest::INVALID_OPERATION;
}


forest::error mtmdd_node_manager::createEdge(const int* const* vlist,
    const int* const* vplist, const float* terms, int N, dd_edge &e)
{
  return forest::INVALID_OPERATION;
}


forest::error mtmdd_node_manager::createEdge(bool val, dd_edge &e)
{
  return forest::INVALID_OPERATION;
}


forest::error mtmdd_node_manager::createEdge(float term, dd_edge& e)
{
  if (e.getForest() != this) return forest::INVALID_OPERATION;
  int termNode = getTerminalNode(term);

  if (reductionRule == forest::FULLY_REDUCED) {
    e.set(termNode, 0, domain::TERMINALS);
    return forest::SUCCESS;
  }

  // construct the edge bottom-up
  const int* h2l_map = expertDomain->getHeightsToLevelsMap();
  int h_sz = expertDomain->getNumVariables() + 1;
  int result = termNode;
  int curr = 0;
  for (int i=1; i<h_sz; i++) {
    curr = createTempNodeMaxSize(h2l_map[i], false);
    setAllDownPtrsWoUnlink(curr, result);
    unlinkNode(result);
    result = reduceNode(curr);
  }
  e.set(result, 0, getNodeLevel(result));

  return forest::SUCCESS;
}


forest::error mtmdd_node_manager::evaluate(const dd_edge &f, const int* vlist,
    bool &term) const
{
  return forest::INVALID_OPERATION;
}


forest::error mtmdd_node_manager::evaluate(const dd_edge &f,
    const int* vlist, float &term) const
{
  // assumption: vlist does not contain any special values (-1, -2, etc).
  // vlist contains a single element.
  int node = f.getNode();
  const int* h2l_map = expertDomain->getHeightsToLevelsMap();
  while (!isTerminalNode(node)) {
    node = getDownPtr(node, vlist[h2l_map[getNodeHeight(node)]]);
  }
  term = getReal(node);
  return forest::SUCCESS;
}


forest::error mtmdd_node_manager::evaluate(const dd_edge& f, const int* vlist,
    const int* vplist, bool &term) const
{
  return forest::INVALID_OPERATION;
}


forest::error mtmdd_node_manager::evaluate(const dd_edge& f, const int* vlist,
    const int* vplist, int &term) const
{
  return forest::INVALID_OPERATION;
}


forest::error mtmdd_node_manager::evaluate(const dd_edge& f, const int* vlist,
    const int* vplist, float &term) const
{
  return forest::INVALID_OPERATION;
}


// *********************************** MXDs *********************************** 


mxd_node_manager::mxd_node_manager(domain *d)
: node_manager(d, true, forest::BOOLEAN,
      forest::MULTI_TERMINAL, forest::IDENTITY_REDUCED,
      forest::FULL_OR_SPARSE_STORAGE, OPTIMISTIC_DELETION)
{ }


mxd_node_manager::~mxd_node_manager()
{ }


int mxd_node_manager::reduceNode(int p)
{
  DCASSERT(isActiveNode(p));
  assert(reductionRule == forest::IDENTITY_REDUCED);

  if (isReducedNode(p)) return p; 

  DCASSERT(!isTerminalNode(p));
  DCASSERT(isFullNode(p));
  DCASSERT(getInCount(p) == 1);

#if 0
  validateIncounts();
#endif

  int size = getFullNodeSize(p);
  int* ptr = getFullNodeDownPtrs(p);
  int node_level = getNodeLevel(p);

  decrTempNodeCount(node_level);

#ifdef DEVELOPMENT_CODE
  int node_height = getNodeHeight(p);
  if (isUnprimedNode(p)) {
    // unprimed node
    for (int i=0; i<size; i++) {
      assert(isReducedNode(ptr[i]));
      assert(ptr[i] == 0 || (getNodeLevel(ptr[i]) == -node_level));
    }
  } else {
    // primed node
    for (int i=0; i<size; i++) {
      assert(isReducedNode(ptr[i]));
      assert(getNodeHeight(ptr[i]) < node_height);
      assert(isTerminalNode(ptr[i]) || isUnprimedNode(ptr[i]));
    }
  }
#endif

  // quick scan: is this node zero?
  int nnz = 0;
  int truncsize = 0;
  for (int i = 0; i < size; ++i) {
    if (0 != ptr[i]) {
      nnz++;
      truncsize = i;
    }
  }
  truncsize++;

  if (0 == nnz) {
    // duplicate of 0
    deleteTempNode(p);
#ifdef TRACE_REDUCE
    printf("\tReducing %d, got 0\n", p);
#endif
    return 0;
  }

  // check for possible reductions
  if (isUnprimedNode(p) && nnz == getLevelSize(node_level)) {
    // check for identity matrix node
    int temp = getDownPtr(ptr[0], 0);
    if (temp != 0) {
      bool ident = true;
      for (int i = 0; i < size && ident; i++) {
        if (!singleNonZeroAt(ptr[i], temp, i)) ident = false;
      }
      if (ident) {
        // passed all tests for identity matrix node
        temp = sharedCopy(temp);
        deleteTempNode(p);
        return temp;
      }
    }
  }

  // check unique table
  int q = find(p);
  if (getNull() != q) {
    // duplicate found
#ifdef TRACE_REDUCE
    printf("\tReducing %d, got %d\n", p, q);
#endif
    deleteTempNode(p);
    return sharedCopy(q);
  }

  // insert into unique table
  insert(p);

#ifdef TRACE_REDUCE
  printf("\tReducing %d: unique, compressing\n", p);
#endif

  if (!areSparseNodesEnabled())
    nnz = size;

  // right now, tie goes to truncated full.
  if (2*nnz < truncsize) {
    // sparse is better; convert
    int newoffset = getHole(node_level, 4+2*nnz, true);
    // can't rely on previous ptr, re-point to p
    int* full_ptr = getNodeAddress(p);
    int* sparse_ptr = getAddress(node_level, newoffset);
    // copy first 2 integers: incount, next
    sparse_ptr[0] = full_ptr[0];
    sparse_ptr[1] = full_ptr[1];
    // size
    sparse_ptr[2] = -nnz;
    // copy index into address[]
    sparse_ptr[3 + 2*nnz] = p;
    // get pointers to the new sparse node
    int* indexptr = sparse_ptr + 3;
    int* downptr = indexptr + nnz;
    ptr = full_ptr + 3;
    // copy downpointers
    for (int i=0; i<size; i++, ++ptr) {
      if (*ptr) {
        *indexptr = i;
        *downptr = *ptr;
        ++indexptr;
        ++downptr;
      }
    }
    // trash old node
#ifdef MEMORY_TRACE
    int saved_offset = getNodeOffset(p);
    setNodeOffset(p, newoffset);
    makeHole(node_level, saved_offset, 4 + size);
#else
    makeHole(node_level, getNodeOffset(p), 4 + size);
    setNodeOffset(p, newoffset);
#endif
    // address[p].cache_count does not change
  } else {
    // full is better
    if (truncsize<size) {
      // truncate the trailing 0s
      int newoffset = getHole(node_level, 4+truncsize, true);
      // can't rely on previous ptr, re-point to p
      int* full_ptr = getNodeAddress(p);
      int* trunc_ptr = getAddress(node_level, newoffset);
      // copy first 2 integers: incount, next
      trunc_ptr[0] = full_ptr[0];
      trunc_ptr[1] = full_ptr[1];
      // size
      trunc_ptr[2] = truncsize;
      // copy index into address[]
      trunc_ptr[3 + truncsize] = p;
      // elements
      memcpy(trunc_ptr + 3, full_ptr + 3, truncsize * sizeof(int));
      // trash old node
#ifdef MEMORY_TRACE
      int saved_offset = getNodeOffset(p);
      setNodeOffset(p, newoffset);
      makeHole(node_level, saved_offset, 4 + size);
#else
      makeHole(node_level, getNodeOffset(p), 4 + size);
      setNodeOffset(p, newoffset);
#endif
      // address[p].cache_count does not change
    }
  }

  // address[p].cache_count does not change
  DCASSERT(getCacheCount(p) == 0);
  // Sanity check that the hash value is unchanged
  DCASSERT(find(p) == p);

#if 0
  validateIncounts();
#endif

  return p;
}


int mxd_node_manager::createNode(int k, int index, int dptr)
{
  DCASSERT(index >= 0 && dptr >= -1);

  if (dptr == 0) return 0;

  // a single downpointer points to dptr
  if (nodeStorage == FULL_STORAGE ||
      (nodeStorage == FULL_OR_SPARSE_STORAGE && index < 2)) {
    // Build a full node
    int curr = createTempNode(k, index + 1);
    setDownPtrWoUnlink(curr, index, dptr);
    return reduceNode(curr);
  }
  else {
    DCASSERT (nodeStorage == SPARSE_STORAGE ||
        (nodeStorage == FULL_OR_SPARSE_STORAGE && index >= 2));
    // Build a sparse node
    int p = createTempNode(k, 2);
    int* nodeData = getNodeAddress(p);
    // For sparse nodes, size is -ve
    nodeData[2] = -1;
    // indexes followed by downpointers -- here we have one index and one dptr
    nodeData[3] = index;
    nodeData[4] = sharedCopy(dptr);
    // search in unique table
    int q = find(p);
    if (getNull() == q) {
      // no duplicate found; insert into unique table
      insert(p);
      DCASSERT(getCacheCount(p) == 0);
      DCASSERT(find(p) == p);
    }
    else {
      // duplicate found; discard this node and return the duplicate
      // revert to full temp node before discarding
      nodeData[2] = 2;
      nodeData[3] = 0;
      nodeData[4] = 0;
      unlinkNode(dptr);
      deleteTempNode(p);
      p = sharedCopy(q);
    }
    decrTempNodeCount(k);
    return p;
  }
}

int mxd_node_manager::createNode(int k, int index1, int index2, int dptr)
{
  DCASSERT(reductionRule == forest::IDENTITY_REDUCED);

  DCASSERT((index1 >= 0 && index2 >= 0) ||
           (index1 >= -2 && index2 >= -2 && index1 == index2));


  if (index1 == -2) {
    // "don't change"
    return sharedCopy(dptr);
  } else if (index1 == -1) {
    // represents "don't care"
    // assume index2 == -1
    int p = createTempNodeMaxSize(-k, false);
    setAllDownPtrsWoUnlink(p, dptr);
    p = reduceNode(p);
    int curr = createTempNodeMaxSize(k, false);
    setAllDownPtrsWoUnlink(curr, p);
    unlinkNode(p);
    return reduceNode(curr);
  } else {
    // normal case: build -k level node, build +k level node
#if 0
    int p = createTempNode(-k, index2 + 1);
    setDownPtrWoUnlink(p, index2, dptr);
    p = reduceNode(p);
    int curr = createTempNode(k, index1 + 1);
    setDownPtrWoUnlink(curr, index1, p);
    unlinkNode(p);
    return reduceNode(curr);
#else
    int p = createNode(-k, index2, dptr);
    int curr = createNode(k, index1, p);
    unlinkNode(p);
    return curr;
#endif
  }
}


void mxd_node_manager::createEdge(const int* v, const int* vp,  dd_edge& e)
{
  // construct the edge bottom-up
  const int* h2l_map = expertDomain->getHeightsToLevelsMap();
  int h_sz = expertDomain->getNumVariables() + 1;
  int curr = getTerminalNode(true);
  int prev = curr;
  for (int i=1; i<h_sz; i++) {
    prev = curr;
    curr = createNode(h2l_map[i], v[h2l_map[i]], vp[h2l_map[i]], prev);
    unlinkNode(prev);
  }
  e.set(curr, 0, getNodeLevel(curr));
}


forest::error mxd_node_manager::createEdge(const int* const* vlist,
    const int* const* vplist, int N, dd_edge& e)
{
  if (e.getForest() != this) return forest::INVALID_OPERATION;
  if (vlist == 0 || vplist == 0 || N <= 0) return forest::INVALID_VARIABLE;

  createEdge(vlist[0], vplist[0], e);
  if (N > 1) {
    dd_edge curr(this);
    for (int i=1; i<N; i++) {
      createEdge(vlist[i], vplist[i], curr);
      e += curr;
    }
  }

  return forest::SUCCESS;
}


forest::error mxd_node_manager::createEdge(bool val, dd_edge &e)
{
  if (!val) {
    e.set(getTerminalNode(false), 0, domain::TERMINALS);
  }
  else {
    DCASSERT(val);
    // construct the edge bottom-up
    const int* h2l_map = expertDomain->getHeightsToLevelsMap();
    int h_sz = expertDomain->getNumVariables() + 1;
    int curr = getTerminalNode(true);
    int prev = curr;
    for (int i=1; i<h_sz; i++) {
      prev = curr;
      curr = createNode(h2l_map[i], -1, -1, prev);
      unlinkNode(prev);
    }
    e.set(curr, 0, getNodeLevel(curr));
  }
  return forest::SUCCESS;
}


forest::error mxd_node_manager::evaluate(const dd_edge& f, const int* vlist,
    const int* vplist, bool &term) const
{
  // assumption: vlist and vplist do not contain any special values
  // (-1, -2, etc). vlist and vplist contains a single element.
  int node = f.getNode();
  const int* h2l_map = expertDomain->getHeightsToLevelsMap();
  int level = 0;
  while (!isTerminalNode(node)) {
    level = h2l_map[getNodeHeight(node)];
    node = getDownPtr(node, vlist[level]);
    node = getDownPtr(node, vplist[level]);
  }
  term = getBoolean(node);
  return forest::SUCCESS;
}


void mxd_node_manager::normalizeAndReduceNode(int& p, int& ev)
{
  assert(false);
}


forest::error mxd_node_manager::createEdge(const int* const* vlist, int N,
    dd_edge &e)
{
  return forest::INVALID_OPERATION;
}


forest::error mxd_node_manager::createEdge(const int* const* vlist,
    const int* terms, int N, dd_edge &e)
{
  return forest::INVALID_OPERATION;
}


forest::error mxd_node_manager::createEdge(const int* const* vlist,
    const float* terms, int n, dd_edge &e)
{
  return forest::INVALID_OPERATION;
}


forest::error mxd_node_manager::createEdge(const int* const* vlist,
    const int* const* vplist, const int* terms, int N, dd_edge &e)
{
  return forest::INVALID_OPERATION;
}


forest::error mxd_node_manager::createEdge(const int* const* vlist,
    const int* const* vplist, const float* terms, int N, dd_edge &e)
{
  return forest::INVALID_OPERATION;
}


forest::error mxd_node_manager::createEdge(int val, dd_edge &e)
{
  return forest::INVALID_OPERATION;
}


forest::error mxd_node_manager::createEdge(float val, dd_edge &e)
{
  return forest::INVALID_OPERATION;
}


forest::error mxd_node_manager::evaluate(const dd_edge &f, const int* vlist,
    bool &term) const
{
  return forest::INVALID_OPERATION;
}


forest::error mxd_node_manager::evaluate(const dd_edge &f, const int* vlist,
    int &term) const
{
  return forest::INVALID_OPERATION;
}


forest::error mxd_node_manager::evaluate(const dd_edge &f, const int* vlist,
    float &term) const
{
  return forest::INVALID_OPERATION;
}


forest::error mxd_node_manager::evaluate(const dd_edge& f, const int* vlist,
    const int* vplist, int &term) const
{
  return forest::INVALID_OPERATION;
}


forest::error mxd_node_manager::evaluate(const dd_edge& f, const int* vlist,
    const int* vplist, float &term) const
{
  return forest::INVALID_OPERATION;
}


// ******************************** MTMXDs ******************************* 


mtmxd_node_manager::mtmxd_node_manager(domain *d, forest::range_type t)
: node_manager(d, true, t,
      forest::MULTI_TERMINAL, forest::IDENTITY_REDUCED,
      forest::FULL_OR_SPARSE_STORAGE, OPTIMISTIC_DELETION)
{ }


mtmxd_node_manager::~mtmxd_node_manager()
{ }


int mtmxd_node_manager::reduceNode(int p)
{
  DCASSERT(isActiveNode(p));
  assert(reductionRule == forest::IDENTITY_REDUCED);

  if (isReducedNode(p)) return p; 

  DCASSERT(!isTerminalNode(p));
  DCASSERT(isFullNode(p));
  DCASSERT(getInCount(p) == 1);

#if 0
  validateIncounts();
#endif

  int size = getFullNodeSize(p);
  int* ptr = getFullNodeDownPtrs(p);
  int node_level = getNodeLevel(p);

  decrTempNodeCount(node_level);

#ifdef DEVELOPMENT_CODE
  int node_height = getNodeHeight(p);
  if (isUnprimedNode(p)) {
    // unprimed node
    for (int i=0; i<size; i++) {
      assert(isReducedNode(ptr[i]));
      assert(ptr[i] == 0 || (getNodeLevel(ptr[i]) == -node_level));
    }
  } else {
    // primed node
    for (int i=0; i<size; i++) {
      assert(isReducedNode(ptr[i]));
      assert(getNodeHeight(ptr[i]) < node_height);
      assert(isTerminalNode(ptr[i]) || isUnprimedNode(ptr[i]));
    }
  }
#endif

  // quick scan: is this node zero?
  int nnz = 0;
  int truncsize = 0;
  for (int i = 0; i < size; ++i) {
    if (0 != ptr[i]) {
      nnz++;
      truncsize = i;
    }
  }
  truncsize++;

  if (0 == nnz) {
    // duplicate of 0
    deleteTempNode(p);
#ifdef TRACE_REDUCE
    printf("\tReducing %d, got 0\n", p);
#endif
    return 0;
  }

  // check for possible reductions
  if (isUnprimedNode(p) && nnz == getLevelSize(node_level)) {
    // check for identity matrix node
    int temp = getDownPtr(ptr[0], 0);
    if (temp != 0) {
      bool ident = true;
      for (int i = 0; i < size && ident; i++) {
        if (!singleNonZeroAt(ptr[i], temp, i)) ident = false;
      }
      if (ident) {
        // passed all tests for identity matrix node
        temp = sharedCopy(temp);
        deleteTempNode(p);
        return temp;
      }
    }
  }

  // check unique table
  int q = find(p);
  if (getNull() != q) {
    // duplicate found
#ifdef TRACE_REDUCE
    printf("\tReducing %d, got %d\n", p, q);
#endif
    deleteTempNode(p);
    return sharedCopy(q);
  }

  // insert into unique table
  insert(p);

#ifdef TRACE_REDUCE
  printf("\tReducing %d: unique, compressing\n", p);
#endif

  if (!areSparseNodesEnabled())
    nnz = size;

  // right now, tie goes to truncated full.
  if (2*nnz < truncsize) {
    // sparse is better; convert
    int newoffset = getHole(node_level, 4+2*nnz, true);
    // can't rely on previous ptr, re-point to p
    int* full_ptr = getNodeAddress(p);
    int* sparse_ptr = getAddress(node_level, newoffset);
    // copy first 2 integers: incount, next
    sparse_ptr[0] = full_ptr[0];
    sparse_ptr[1] = full_ptr[1];
    // size
    sparse_ptr[2] = -nnz;
    // copy index into address[]
    sparse_ptr[3 + 2*nnz] = p;
    // get pointers to the new sparse node
    int* indexptr = sparse_ptr + 3;
    int* downptr = indexptr + nnz;
    ptr = full_ptr + 3;
    // copy downpointers
    for (int i=0; i<size; i++, ++ptr) {
      if (*ptr) {
        *indexptr = i;
        *downptr = *ptr;
        ++indexptr;
        ++downptr;
      }
    }
    // trash old node
#ifdef MEMORY_TRACE
    int saved_offset = getNodeOffset(p);
    setNodeOffset(p, newoffset);
    makeHole(node_level, saved_offset, 4 + size);
#else
    makeHole(node_level, getNodeOffset(p), 4 + size);
    setNodeOffset(p, newoffset);
#endif
    // address[p].cache_count does not change
  } else {
    // full is better
    if (truncsize<size) {
      // truncate the trailing 0s
      int newoffset = getHole(node_level, 4+truncsize, true);
      // can't rely on previous ptr, re-point to p
      int* full_ptr = getNodeAddress(p);
      int* trunc_ptr = getAddress(node_level, newoffset);
      // copy first 2 integers: incount, next
      trunc_ptr[0] = full_ptr[0];
      trunc_ptr[1] = full_ptr[1];
      // size
      trunc_ptr[2] = truncsize;
      // copy index into address[]
      trunc_ptr[3 + truncsize] = p;
      // elements
      memcpy(trunc_ptr + 3, full_ptr + 3, truncsize * sizeof(int));
      // trash old node
#ifdef MEMORY_TRACE
      int saved_offset = getNodeOffset(p);
      setNodeOffset(p, newoffset);
      makeHole(node_level, saved_offset, 4 + size);
#else
      makeHole(node_level, getNodeOffset(p), 4 + size);
      setNodeOffset(p, newoffset);
#endif
      // address[p].cache_count does not change
    }
  }

  // address[p].cache_count does not change
  DCASSERT(getCacheCount(p) == 0);
  // Sanity check that the hash value is unchanged
  DCASSERT(find(p) == p);

#if 0
  validateIncounts();
#endif

  return p;
}


int mtmxd_node_manager::createNode(int k, int index, int dptr)
{
  DCASSERT(index >= 0 && isValidNodeIndex(dptr));

  if (dptr == 0) return 0;

  // a single downpointer points to dptr
  if (nodeStorage == FULL_STORAGE ||
      (nodeStorage == FULL_OR_SPARSE_STORAGE && index < 2)) {
    // Build a full node
    int curr = createTempNode(k, index + 1);
    setDownPtrWoUnlink(curr, index, dptr);
    return reduceNode(curr);
  }
  else {
    DCASSERT (nodeStorage == SPARSE_STORAGE ||
        (nodeStorage == FULL_OR_SPARSE_STORAGE && index >= 2));
    // Build a sparse node
    int p = createTempNode(k, 2);
    int* nodeData = getNodeAddress(p);
    // For sparse nodes, size is -ve
    nodeData[2] = -1;
    // indexes followed by downpointers -- here we have one index and one dptr
    nodeData[3] = index;
    nodeData[4] = sharedCopy(dptr);
    // search in unique table
    int q = find(p);
    if (getNull() == q) {
      // no duplicate found; insert into unique table
      insert(p);
      DCASSERT(getCacheCount(p) == 0);
      DCASSERT(find(p) == p);
    }
    else {
      // duplicate found; discard this node and return the duplicate
      // revert to full temp node before discarding
      nodeData[2] = 2;
      nodeData[3] = 0;
      nodeData[4] = 0;
      unlinkNode(dptr);
      deleteTempNode(p);
      p = sharedCopy(q);
    }
    decrTempNodeCount(k);
    return p;
  }
}

int mtmxd_node_manager::createNode(int k, int index1, int index2, int dptr)
{
  DCASSERT(reductionRule == forest::IDENTITY_REDUCED);

  DCASSERT((index1 >= 0 && index2 >= 0) ||
           (index1 >= -2 && index2 >= -2 && index1 == index2));


  if (index1 == -2) {
    // "don't change"
    return sharedCopy(dptr);
  } else if (index1 == -1) {
    // represents "don't care"
    // assume index2 == -1
    int p = createTempNodeMaxSize(-k, false);
    setAllDownPtrsWoUnlink(p, dptr);
    p = reduceNode(p);
    int curr = createTempNodeMaxSize(k, false);
    setAllDownPtrsWoUnlink(curr, p);
    unlinkNode(p);
    return reduceNode(curr);
  } else {
    // normal case: build -k level node, build +k level node
#if 0
    int p = createTempNode(-k, index2 + 1);
    setDownPtrWoUnlink(p, index2, dptr);
    p = reduceNode(p);
    int curr = createTempNode(k, index1 + 1);
    setDownPtrWoUnlink(curr, index1, p);
    unlinkNode(p);
    return reduceNode(curr);
#else
    int p = createNode(-k, index2, dptr);
    int curr = createNode(k, index1, p);
    unlinkNode(p);
    return curr;
#endif
  }
}


void mtmxd_node_manager::createEdge(const int* v, const int* vp, int term,
    dd_edge& e)
{
  // construct the edge bottom-up
  const int* h2l_map = expertDomain->getHeightsToLevelsMap();
  int h_sz = expertDomain->getNumVariables() + 1;
  DCASSERT(isTerminalNode(term));
  int curr = term;
  int prev = curr;
  for (int i=1; i<h_sz; i++) {
    prev = curr;
    curr = createNode(h2l_map[i], v[h2l_map[i]], vp[h2l_map[i]], prev);
    unlinkNode(prev);
  }
  e.set(curr, 0, getNodeLevel(curr));
}


forest::error mtmxd_node_manager::createEdge(const int* const* vlist,
    const int* const* vplist, const int* terms, int N, dd_edge& e)
{
  DCASSERT(getRangeType() == forest::INTEGER);
  if (e.getForest() != this) return forest::INVALID_OPERATION;
  if (vlist == 0 || vplist == 0 || terms == 0 || N <= 0)
    return forest::INVALID_VARIABLE;

  createEdge(vlist[0], vplist[0], getTerminalNode(terms[0]), e);
  if (N > 1) {
    dd_edge curr(this);
    for (int i=1; i<N; i++) {
      createEdge(vlist[i], vplist[i], getTerminalNode(terms[i]), curr);
      e += curr;
    }
  }

  return forest::SUCCESS;
}


forest::error mtmxd_node_manager::createEdge(const int* const* vlist,
    const int* const* vplist, const float* terms, int N, dd_edge& e)
{
  DCASSERT(getRangeType() == forest::REAL);
  if (e.getForest() != this) return forest::INVALID_OPERATION;
  if (vlist == 0 || vplist == 0 || terms == 0 || N <= 0)
    return forest::INVALID_VARIABLE;

  createEdge(vlist[0], vplist[0], getTerminalNode(terms[0]), e);
  if (N > 1) {
    dd_edge curr(this);
    for (int i=1; i<N; i++) {
      createEdge(vlist[i], vplist[i], getTerminalNode(terms[i]), curr);
      e += curr;
    }
  }

  return forest::SUCCESS;
}


int mtmxd_node_manager::createEdge(int dptr)
{
  DCASSERT(isTerminalNode(dptr));
  if (dptr == 0) return dptr;

  // construct the edge bottom-up
  const int* h2l_map = expertDomain->getHeightsToLevelsMap();
  int h_sz = expertDomain->getNumVariables() + 1;
  int curr = dptr;
  int prev = 0;
  for (int i=1; i<h_sz; i++) {
    prev = curr;
    curr = createNode(h2l_map[i], -1, -1, prev);
    unlinkNode(prev);
  }
  return curr;
}


forest::error mtmxd_node_manager::createEdge(int val, dd_edge &e)
{
  DCASSERT(getRangeType() == forest::INTEGER);
  if (e.getForest() != this) return forest::INVALID_OPERATION;

  int node = createEdge(getTerminalNode(val));
  e.set(node, 0, getNodeLevel(node));
  return forest::SUCCESS;
}


forest::error mtmxd_node_manager::createEdge(float val, dd_edge &e)
{
  DCASSERT(getRangeType() == forest::REAL);
  if (e.getForest() != this) return forest::INVALID_OPERATION;

  int node = createEdge(getTerminalNode(val));
  e.set(node, 0, getNodeLevel(node));
  return forest::SUCCESS;
}


forest::error mtmxd_node_manager::evaluate(const dd_edge& f, const int* vlist,
    const int* vplist, int &term) const
{
  DCASSERT(getRangeType() == forest::INTEGER);
  // assumption: vlist and vplist do not contain any special values
  // (-1, -2, etc). vlist and vplist contains a single element.
  int node = f.getNode();
  const int* h2l_map = expertDomain->getHeightsToLevelsMap();
  int level = 0;
  while (!isTerminalNode(node)) {
    level = h2l_map[getNodeHeight(node)];
    node = getDownPtr(node, vlist[level]);
    node = getDownPtr(node, vplist[level]);
  }
  term = getInteger(node);
  return forest::SUCCESS;
}


forest::error mtmxd_node_manager::evaluate(const dd_edge& f, const int* vlist,
    const int* vplist, float &term) const
{
  DCASSERT(getRangeType() == forest::REAL);
  // assumption: vlist and vplist do not contain any special values
  // (-1, -2, etc). vlist and vplist contains a single element.
  int node = f.getNode();
  const int* h2l_map = expertDomain->getHeightsToLevelsMap();
  int level = 0;
  while (!isTerminalNode(node)) {
    level = h2l_map[getNodeHeight(node)];
    node = getDownPtr(node, vlist[level]);
    node = getDownPtr(node, vplist[level]);
  }
  term = getReal(node);
  return forest::SUCCESS;
}


void mtmxd_node_manager::normalizeAndReduceNode(int& p, int& ev)
{
  assert(false);
}


forest::error mtmxd_node_manager::createEdge(const int* const* vlist, int N,
    dd_edge &e)
{
  return forest::INVALID_OPERATION;
}


forest::error mtmxd_node_manager::createEdge(const int* const* vlist,
    const int* terms, int N, dd_edge &e)
{
  return forest::INVALID_OPERATION;
}


forest::error mtmxd_node_manager::createEdge(const int* const* vlist,
    const float* terms, int n, dd_edge &e)
{
  return forest::INVALID_OPERATION;
}


forest::error mtmxd_node_manager::createEdge(const int* const* vlist,
    const int* const* vplist, int N, dd_edge &e)
{
  return forest::INVALID_OPERATION;
}


forest::error mtmxd_node_manager::createEdge(bool val, dd_edge &e)
{
  return forest::INVALID_OPERATION;
}


forest::error mtmxd_node_manager::evaluate(const dd_edge &f, const int* vlist,
    bool &term) const
{
  return forest::INVALID_OPERATION;
}


forest::error mtmxd_node_manager::evaluate(const dd_edge &f, const int* vlist,
    int &term) const
{
  return forest::INVALID_OPERATION;
}


forest::error mtmxd_node_manager::evaluate(const dd_edge &f, const int* vlist,
    float &term) const
{
  return forest::INVALID_OPERATION;
}


forest::error mtmxd_node_manager::evaluate(const dd_edge& f, const int* vlist,
    const int* vplist, bool &term) const
{
  return forest::INVALID_OPERATION;
}


// ********************************** EVMDDs ********************************** 


evmdd_node_manager::evmdd_node_manager(domain *d, range_type t)
: node_manager(d, false, t,
      forest::EVPLUS, forest::FULLY_REDUCED,
      forest::FULL_OR_SPARSE_STORAGE, OPTIMISTIC_DELETION)
{
  assert(t == forest::INTEGER);
#ifdef ALT_EVMDD
  assert(false);
#endif
}


evmdd_node_manager::~evmdd_node_manager()
{ }


void evmdd_node_manager::normalizeAndReduceNode(int& p, int& ev)
{
  DCASSERT(getEdgeLabeling() == forest::EVPLUS);

  DCASSERT(isActiveNode(p));

  if (isReducedNode(p)) {
#ifdef ALT_EVMDD
    if (p == 0) ev = 0;
#else
    if (p == 0) ev = INF;
#endif
    return;
  }

  DCASSERT(!isTerminalNode(p));
  DCASSERT(isFullNode(p));
  DCASSERT(getInCount(p) == 1);

#if 0
  validateIncounts();
#endif

  const int size = getFullNodeSize(p);
  int *dptr = getFullNodeDownPtrs(p);
  int *eptr = getFullNodeEdgeValues(p);
  const int node_level = getNodeLevel(p);

  decrTempNodeCount(node_level);

#ifdef DEVELOPMENT_CODE
  const int node_height = getNodeHeight(p);
  for (int i=0; i<size; i++) {
    assert(isReducedNode(dptr[i]));
    assert(getNodeHeight(dptr[i]) < node_height);
#ifdef ALT_EVMDD
#else
    assert((dptr[i] == 0 && eptr[i] == INF) ||
        (dptr[i] != 0 && eptr[i] != INF));
#endif
  }
#endif

  // quick scan: is this node zero?
  // ALT_EVMDD: truncated index means:
  //     EVPLUS: going to 0 with edge-value 0
  //     EVTIMES: going to 0 with edge-value 1
  // find min for normalizing later
  int nnz = 0;
  int truncsize = 0;

#ifdef ALT_EVMDD
  DCASSERT(getEdgeLabeling() == forest::EVPLUS);
  int min = eptr[0];
  int nz = 0;
  // find min
  for (int i = 0; i < size; i++) {
    if (eptr[i] == 0) nz++;
    if (eptr[i] < min) min = eptr[i];
  }
  // normalize
  /* HERE */
  if (min != 0) {
    if 
  }
  truncsize++;
#else
  int min = INF;
  for (int i = 0; i < size; i++) {
    if (0 != dptr[i]) {
      nnz++;
      truncsize = i;
      DCASSERT(eptr[i] != INF);
      if (eptr[i] < min) min = eptr[i];
    }
  }
  truncsize++;
#endif

  if (0 == nnz) {
    // duplicate of 0
    deleteTempNode(p);
#ifdef TRACE_REDUCE
    printf("\tReducing %d, got 0\n", p);
#endif
    p = 0;
    ev = INF;
    return;
  }

  // normalize -- there should be atleast one i s.t. eptr[i] == 0
  DCASSERT(min != INF);
  for (int i = 0; i < size; i++) {
    if (0 != dptr[i]) {
      eptr[i] -= min;
      DCASSERT(eptr[i] >= 0);
    } // else eptr[i] == INF
  }

  // after normalizing, residual edge-value (i.e. min) is pushed up
  // nothing needs to be added ev after this step
  ev += min;

  // check for possible reductions
  if (reductionRule == forest::FULLY_REDUCED &&
      nnz == getLevelSize(node_level) && eptr[0] == 0) {
    // if downpointers are the same and ev are same (i.e. 0 after
    // normalizing), eliminate node
    int i = 1;
    for ( ; i < size && dptr[i] == dptr[0] && eptr[i] == 0; i++);
    if (i == size ) {
      // for all i, dptr[i] == dptr[0] and eptr[i] == 0
      int temp = sharedCopy(dptr[0]);
      deleteTempNode(p);
      p = temp;
      return;
    }
  }

  // check unique table
  int q = find(p);
  if (getNull() != q) {
    // duplicate found
#ifdef TRACE_REDUCE
    printf("\tReducing %d, got %d\n", p, q);
#endif
    linkNode(q);
    deleteTempNode(p);
    p = q;
    return;
  }

  // insert into unique table
  insert(p);

#ifdef TRACE_REDUCE
  printf("\tReducing %d: unique, compressing\n", p);
#endif

  if (!areSparseNodesEnabled())
    nnz = size;

  // right now, tie goes to truncated full.
  if (3*nnz < 2*truncsize) {
    // sparse is better; convert
    int newoffset = getHole(node_level, 4+3*nnz, true);
    // can't rely on previous dptr, re-point to p
    int* full_ptr = getNodeAddress(p);
    int* sparse_ptr = getAddress(node_level, newoffset);
    // copy first 2 integers: incount, next
    sparse_ptr[0] = full_ptr[0];
    sparse_ptr[1] = full_ptr[1];
    // size
    sparse_ptr[2] = -nnz;
    // copy index into address[]
    sparse_ptr[3 + 3*nnz] = p;
    // get pointers to the new sparse node
    int* indexptr = sparse_ptr + 3;
    int* downptr = indexptr + nnz;
    int* edgeptr = downptr + nnz;
    dptr = full_ptr + 3;
    eptr = dptr + size;
    // copy downpointers
    for (int i=0; i<size; i++, ++dptr, ++eptr) {
      if (0 != *dptr) {
        *indexptr = i;
        *downptr = *dptr;
        *edgeptr = *eptr;
        ++indexptr;
        ++downptr;
        ++edgeptr;
      }
    }
    // trash old node
#ifdef MEMORY_TRACE
    int saved_offset = getNodeOffset(p);
    setNodeOffset(p, newoffset);
    makeHole(node_level, saved_offset, 4 + 2 * size);
#else
    makeHole(node_level, getNodeOffset(p), 4 + 2 * size);
    setNodeOffset(p, newoffset);
#endif
    // address[p].cache_count does not change
  } else {
    // full is better
    if (truncsize < size) {
      // truncate the trailing 0s
      int newoffset = getHole(node_level, 4+2*truncsize, true);
      // can't rely on previous ptr, re-point to p
      int* full_ptr = getNodeAddress(p);
      int* trunc_ptr = getAddress(node_level, newoffset);
      // copy first 2 integers: incount, next
      trunc_ptr[0] = full_ptr[0];
      trunc_ptr[1] = full_ptr[1];
      // size
      trunc_ptr[2] = truncsize;
      // copy index into address[]
      trunc_ptr[3 + 2 * truncsize] = p;
      // elements
      memcpy(trunc_ptr + 3, full_ptr + 3, truncsize * sizeof(int));
      // edge values
      memcpy(trunc_ptr + 3 + truncsize, full_ptr + 3 + size,
          truncsize * sizeof(int));
      // trash old node
#ifdef MEMORY_TRACE
      int saved_offset = getNodeOffset(p);
      setNodeOffset(p, newoffset);
      makeHole(node_level, saved_offset, 4 + 2 * size);
#else
      makeHole(node_level, getNodeOffset(p), 4 + 2 * size);
      setNodeOffset(p, newoffset);
#endif
      // address[p].cache_count does not change
    }
  }

  // address[p].cache_count does not change
  DCASSERT(getCacheCount(p) == 0);
  // Sanity check that the hash value is unchanged
  DCASSERT(find(p) == p);

#if 0
  validateIncounts();
#endif

  return;
}


void evmdd_node_manager::createNode(int k, int index, int dptr, int ev,
    int& res, int& resEv)
{
  DCASSERT(dptr >= -1 && ev >= 0);
  DCASSERT(index >= -1);

  if (index == -1) {
    // if all edge values are the same for a evmdd node, it is
    // equivalent to say that they are all 0 (since we subtract the minimum
    // anyway).
    if (reductionRule == forest::FULLY_REDUCED) {
      res = sharedCopy(dptr);
      resEv = ev;
      return;
    }
    res = createTempNodeMaxSize(k, false);
    setAllDownPtrsWoUnlink(res, dptr);
    setAllEdgeValues(res, ev);
    resEv = 0;
    normalizeAndReduceNode(res, resEv);
    return;
  }

  // a single downpointer points to dptr

#if 0

  res = createTempNode(k, index + 1);
  setDownPtrWoUnlink(res, index, dptr);
  setEdgeValue(res, index, ev);
  resEv = 0;
  normalizeAndReduceNode(res, resEv);

#else

  if (nodeStorage == FULL_STORAGE ||
      (nodeStorage == FULL_OR_SPARSE_STORAGE && index < 2)) {
    // Build a full node
    res = createTempNode(k, index + 1);
    setDownPtrWoUnlink(res, index, dptr);
    setEdgeValue(res, index, ev);
    resEv = 0;
    normalizeAndReduceNode(res, resEv);
  }
  else {
    DCASSERT (nodeStorage == SPARSE_STORAGE ||
        (nodeStorage == FULL_OR_SPARSE_STORAGE && index >= 2));
    // Build a sparse node
    createSparseNode(k, index, dptr, ev, res, resEv);
    // res, resEv set by createSparseNode(..); do not call normalizeAndReduce
  }

#endif

}


void evmdd_node_manager::createSparseNode(int k, int index, int dptr, int ev,
    int& res, int& resEv)
{
  DCASSERT(k != 0);

  if (isTimeToGc()) { garbageCollect(); }

  DCASSERT(isValidLevel(k));
  CHECK_RANGE(0, index, getLevelSize(k));

  // get a location in address[] to store the node
  int p = getFreeNode(k);

#ifdef DEBUG_MDD_SET
  printf("%s: k: %d, index: %d, new p: %d\n", __func__, k, index, p);
  fflush(stdout);
#endif

  // fill in the location with p's address info
  address[p].level = k;
  address[p].offset = getHole(k, 7 /*4 + 3*/, true);
  address[p].cache_count = 0;

#ifdef DEBUG_MDD_SET
  printf("%s: offset: %d\n", __func__, address[p].offset);
  fflush(stdout);
#endif

  int* foo = level[mapLevel(k)].data + address[p].offset;
  foo[0] = 1;                   // #incoming
  foo[1] = getTempNodeId();
  foo[2] = -1;                  // size
  foo[3] = index;               // index
  foo[4] = sharedCopy(dptr);    // downpointer
  foo[5] = 0;                   // this is the only ev, has to be zero
                                // set resEv = ev
  foo[6] = p;                   // pointer to this node in the address array

  resEv += ev;

#ifdef TRACK_DELETIONS
  cout << "Creating node " << p << "\n";
  cout.flush();
#endif

  incrNodesActivatedSinceGc();

  // search in unique table
  int q = find(p);
  if (getNull() == q) {
    // no duplicate found; insert into unique table
    insert(p);
    DCASSERT(getCacheCount(p) == 0);
    DCASSERT(find(p) == p);
    res = p;
  }
  else {
    // duplicate found; discard this node and return the duplicate
    unlinkNode(dptr);
    // code from deleteTempNode(p) adapted to work here
    {
      makeHole(k, getNodeOffset(p), 7);
      freeNode(p);
      if (level[mapLevel(k)].compactLevel) compactLevel(k);
    }
    res = sharedCopy(q);
  }
}


void evmdd_node_manager::createEdge(const int* v, int term, dd_edge &e)
{
  // construct the edge bottom-up
  const int* h2l_map = expertDomain->getHeightsToLevelsMap();
  int h_sz = expertDomain->getNumVariables() + 1;
  int prev = getTerminalNode(false);
  int prevEv = 0;
  int curr = getTerminalNode(true);
  int currEv = term;
  for (int i=1; i<h_sz; i++) {
    prev = curr;
    prevEv = currEv;
    createNode(h2l_map[i], v[h2l_map[i]], prev, prevEv, curr, currEv);
    unlinkNode(prev);
  }
  e.set(curr, currEv, getNodeLevel(curr));
}

forest::error evmdd_node_manager::createEdge(const int* const* vlist,
    const int* terms, int N, dd_edge &e)
{
  if (e.getForest() != this) return forest::INVALID_OPERATION;
  if (vlist == 0 || terms == 0 || N <= 0) return forest::INVALID_VARIABLE;

  createEdge(vlist[0], terms[0], e);
  dd_edge curr(this);
  for (int i=1; i<N; i++) {
    createEdge(vlist[i], terms[i], curr);
    e += curr;
  }

  return forest::SUCCESS;
}


forest::error evmdd_node_manager::createEdge(int term, dd_edge &e)
{
  if (e.getForest() != this) return forest::INVALID_OPERATION;
  if (!isTerminalNode(term)) return forest::INVALID_VARIABLE;

  if (reductionRule == forest::FULLY_REDUCED) {
    e.set(getTerminalNode(true), term, domain::TERMINALS);
    return forest::SUCCESS;
  }

  // construct the edge bottom-up
  const int* h2l_map = expertDomain->getHeightsToLevelsMap();
  int h_sz = expertDomain->getNumVariables() + 1;
  int prev = getTerminalNode(false);
  int prevEv = 0;
  int curr = getTerminalNode(true);
  int currEv = term;
  for (int i=1; i<h_sz; i++) {
    prev = curr;
    prevEv = currEv;
    createNode(h2l_map[i], -1, prev, prevEv, curr, currEv);
    unlinkNode(prev);
  }
  e.set(curr, currEv, getNodeLevel(curr));
  return forest::SUCCESS;
}


forest::error evmdd_node_manager::evaluate(const dd_edge &f, const int* vlist,
    int &term) const
{
  if (f.getForest() != this) return forest::INVALID_OPERATION;
  if (vlist == 0) return forest::INVALID_VARIABLE;

  // assumption: vlist does not contain any special values (-1, -2, etc).
  // vlist contains a single element.
  int node = f.getNode();
  const int* h2l_map = expertDomain->getHeightsToLevelsMap();
  int dummy_index = 0;
  int ev = 0;

  term = f.getValue();
  while (!isTerminalNode(node)) {
    dummy_index = ev = 0;
    getDownAndEdgeValueAfterIndex(node, vlist[h2l_map[getNodeHeight(node)]],
        dummy_index, node, ev);
    term += ev;
  }
  return forest::SUCCESS;
}


int evmdd_node_manager::reduceNode(int p)
{
  assert(false);
  return 0;
}


forest::error evmdd_node_manager::createEdge(const int* const* vlist, int N,
    dd_edge &e)
{
  return forest::INVALID_OPERATION;
}


forest::error evmdd_node_manager::createEdge(const int* const* vlist,
    const float* terms, int n, dd_edge &e)
{
  return forest::INVALID_OPERATION;
}


forest::error evmdd_node_manager::createEdge(const int* const* vlist,
    const int* const* vplist, int N, dd_edge &e)
{
  return forest::INVALID_OPERATION;
}


forest::error evmdd_node_manager::createEdge(const int* const* vlist,
    const int* const* vplist, const int* terms, int N, dd_edge &e)
{
  return forest::INVALID_OPERATION;
}


forest::error evmdd_node_manager::createEdge(const int* const* vlist,
    const int* const* vplist, const float* terms, int N, dd_edge &e)
{
  return forest::INVALID_OPERATION;
}


forest::error evmdd_node_manager::createEdge(bool val, dd_edge &e)
{
  return forest::INVALID_OPERATION;
}


forest::error evmdd_node_manager::createEdge(float val, dd_edge &e)
{
  return forest::INVALID_OPERATION;
}


forest::error evmdd_node_manager::evaluate(const dd_edge &f, const int* vlist,
    bool &term) const
{
  return forest::INVALID_OPERATION;
}


forest::error evmdd_node_manager::evaluate(const dd_edge &f, const int* vlist,
    float &term) const
{
  return forest::INVALID_OPERATION;
}


forest::error evmdd_node_manager::evaluate(const dd_edge& f, const int* vlist,
    const int* vplist, bool &term) const
{
  return forest::INVALID_OPERATION;
}


forest::error evmdd_node_manager::evaluate(const dd_edge& f, const int* vlist,
    const int* vplist, int &term) const
{
  return forest::INVALID_OPERATION;
}


forest::error evmdd_node_manager::evaluate(const dd_edge& f, const int* vlist,
    const int* vplist, float &term) const
{
  return forest::INVALID_OPERATION;
}


// *********************************** MDDs ***********************************

mdd_node_manager::mdd_node_manager(domain *d)
: node_manager(d, false, forest::BOOLEAN,
      forest::MULTI_TERMINAL, forest::FULLY_REDUCED,
      forest::FULL_OR_SPARSE_STORAGE, OPTIMISTIC_DELETION)
{ }


mdd_node_manager::~mdd_node_manager()
{ }


int mdd_node_manager::reduceNode(int p)
{
  DCASSERT(isActiveNode(p));

  if (isReducedNode(p)) return p; 

  DCASSERT(!isTerminalNode(p));
  DCASSERT(isFullNode(p));
  DCASSERT(getInCount(p) == 1);

#if 0
  validateIncounts();
#endif

  int size = getFullNodeSize(p);
  int* ptr = getFullNodeDownPtrs(p);
  int node_level = getNodeLevel(p);

  decrTempNodeCount(node_level);

#ifdef DEVELOPMENT_CODE
  int node_height = getNodeHeight(p);
  for (int i=0; i<size; i++) {
    assert(isReducedNode(ptr[i]));
    assert(getNodeHeight(ptr[i]) < node_height);
  }
#endif

  // quick scan: is this node zero?
  int nnz = 0;
  int truncsize = 0;
  for (int i = 0; i < size; ++i) {
    if (0 != ptr[i]) {
      nnz++;
      truncsize = i;
    }
  }
  truncsize++;

  if (0 == nnz) {
    // duplicate of 0
    deleteTempNode(p);
#ifdef TRACE_REDUCE
    printf("\tReducing %d, got 0\n", p);
#endif
    return 0;
  }

  // check for possible reductions
  if (reductionRule == forest::FULLY_REDUCED) {
    if (nnz == getLevelSize(node_level)) {
      int i = 1;
      for ( ; i < size && ptr[i] == ptr[0]; i++);
      if (i == size ) {
        // for all i, ptr[i] == ptr[0]
        int temp = sharedCopy(ptr[0]);
        deleteTempNode(p);
        return temp;
      }
    }
  } else {
    // Quasi-reduced -- no identity reduction for MDDs/MTMDDs
    assert(reductionRule == forest::QUASI_REDUCED);

    // ensure than all downpointers are pointing to nodes exactly one
    // level below or zero.
    for (int i = 0; i < size; ++i)
    {
      if (ptr[i] == 0) continue;
      if (getNodeLevel(ptr[i]) != (node_level - 1)) {
        int temp = ptr[i];
        ptr[i] = buildQuasiReducedNodeAtLevel(node_level - 1, ptr[i]);
        unlinkNode(temp);
      }
      assert(ptr[i] == 0 || (getNodeLevel(ptr[i]) == node_level - 1));
    }
  }

  // check unique table
  int q = find(p);
  if (getNull() != q) {
    // duplicate found
#ifdef TRACE_REDUCE
    printf("\tReducing %d, got %d\n", p, q);
#endif
    deleteTempNode(p);
    return sharedCopy(q);
  }

  // insert into unique table
  insert(p);

#ifdef TRACE_REDUCE
  printf("\tReducing %d: unique, compressing\n", p);
#endif

  if (!areSparseNodesEnabled())
    nnz = size;

  // right now, tie goes to truncated full.
  if (2*nnz < truncsize) {
    // sparse is better; convert
    int newoffset = getHole(node_level, 4+2*nnz, true);
    // can't rely on previous ptr, re-point to p
    int* full_ptr = getNodeAddress(p);
    int* sparse_ptr = getAddress(node_level, newoffset);
    // copy first 2 integers: incount, next
    sparse_ptr[0] = full_ptr[0];
    sparse_ptr[1] = full_ptr[1];
    // size
    sparse_ptr[2] = -nnz;
    // copy index into address[]
    sparse_ptr[3 + 2*nnz] = p;
    // get pointers to the new sparse node
    int* indexptr = sparse_ptr + 3;
    int* downptr = indexptr + nnz;
    ptr = full_ptr + 3;
    // copy downpointers
    for (int i=0; i<size; i++, ++ptr) {
      if (*ptr) {
        *indexptr = i;
        *downptr = *ptr;
        ++indexptr;
        ++downptr;
      }
    }
    // trash old node
#ifdef MEMORY_TRACE
    int saved_offset = getNodeOffset(p);
    setNodeOffset(p, newoffset);
    makeHole(node_level, saved_offset, 4 + size);
#else
    makeHole(node_level, getNodeOffset(p), 4 + size);
    setNodeOffset(p, newoffset);
#endif
    // address[p].cache_count does not change
  } else {
    // full is better
    if (truncsize<size) {
      // truncate the trailing 0s
      int newoffset = getHole(node_level, 4+truncsize, true);
      // can't rely on previous ptr, re-point to p
      int* full_ptr = getNodeAddress(p);
      int* trunc_ptr = getAddress(node_level, newoffset);
      // copy first 2 integers: incount, next
      trunc_ptr[0] = full_ptr[0];
      trunc_ptr[1] = full_ptr[1];
      // size
      trunc_ptr[2] = truncsize;
      // copy index into address[]
      trunc_ptr[3 + truncsize] = p;
      // elements
      memcpy(trunc_ptr + 3, full_ptr + 3, truncsize * sizeof(int));
      // trash old node
#ifdef MEMORY_TRACE
      int saved_offset = getNodeOffset(p);
      setNodeOffset(p, newoffset);
      makeHole(node_level, saved_offset, 4 + size);
#else
      makeHole(node_level, getNodeOffset(p), 4 + size);
      setNodeOffset(p, newoffset);
#endif
      // address[p].cache_count does not change
    }
  }

  // address[p].cache_count does not change
  DCASSERT(getCacheCount(p) == 0);
  // Sanity check that the hash value is unchanged
  DCASSERT(find(p) == p);

#if 0
  validateIncounts();
#endif

  return p;
}

int mdd_node_manager::createNode(int k, int index, int dptr)
{
  DCASSERT(index >= -1 && dptr >= -1);
  DCASSERT(false == getBoolean(0));

#if 1
  if (index > -1 && getLevelSize(k) <= index) {
    //printf("level: %d, curr size: %d, index: %d, ",
    //k, getLevelSize(k), index);
    expertDomain->enlargeVariableBound(k, index + 1);
    //printf("new size: %d\n", getLevelSize(k));
  }
#endif

  if (dptr == 0) return 0;
  if (index == -1) {
    // all downpointers should point to dptr
    if (reductionRule == forest::FULLY_REDUCED) return sharedCopy(dptr);
    int curr = createTempNodeMaxSize(k, false);
    setAllDownPtrsWoUnlink(curr, dptr);
    return reduceNode(curr);
  }

#if 0

  // a single downpointer points to dptr
  int curr = createTempNode(k, index + 1);
  setDownPtrWoUnlink(curr, index, dptr);
  return reduceNode(curr);

#else

  // a single downpointer points to dptr
  if (nodeStorage == FULL_STORAGE ||
      (nodeStorage == FULL_OR_SPARSE_STORAGE && index < 2)) {
    // Build a full node
    int curr = createTempNode(k, index + 1);
    setDownPtrWoUnlink(curr, index, dptr);
    return reduceNode(curr);
  }
  else {
    DCASSERT (nodeStorage == SPARSE_STORAGE ||
        (nodeStorage == FULL_OR_SPARSE_STORAGE && index >= 2));
    // Build a sparse node
    int p = createTempNode(k, 2);
    int* nodeData = getNodeAddress(p);
    // For sparse nodes, size is -ve
    nodeData[2] = -1;
    // indexes followed by downpointers -- here we have one index and one dptr
    nodeData[3] = index;
    nodeData[4] = sharedCopy(dptr);
    // search in unique table
    int q = find(p);
    if (getNull() == q) {
      // no duplicate found; insert into unique table
      insert(p);
      DCASSERT(getCacheCount(p) == 0);
      DCASSERT(find(p) == p);
    }
    else {
      // duplicate found; discard this node and return the duplicate
      // revert to full temp node before discarding
      nodeData[2] = 2;
      nodeData[3] = 0;
      nodeData[4] = 0;
      unlinkNode(dptr);
      deleteTempNode(p);
      p = sharedCopy(q);
    }
    decrTempNodeCount(k);
    return p;
  }

#endif

}


void mdd_node_manager::createEdge(const int* v, int term, dd_edge& e)
{
  // construct the edge bottom-up
  const int* h2l_map = expertDomain->getHeightsToLevelsMap();
  int h_sz = expertDomain->getNumVariables() + 1;
  int result = term;
  int curr = 0;
  for (int i=1; i<h_sz; i++) {
    curr = createNode(h2l_map[i], v[h2l_map[i]], result);
    unlinkNode(result);
    result = curr;
  }
  e.set(result, 0, getNodeLevel(result));
}


forest::error mdd_node_manager::createEdge(const int* const* vlist, int N,
    dd_edge& e)
{
  if (e.getForest() != this) return forest::INVALID_OPERATION;
  if (vlist == 0 || N <= 0) return forest::INVALID_VARIABLE;

  int trueNode = getTerminalNode(true);
  createEdge(vlist[0], trueNode, e);
  if (N > 1) {
    dd_edge curr(this);
    for (int i=1; i<N; i++) {
      createEdge(vlist[i], trueNode, curr);
      e += curr;
    }
  }

  return forest::SUCCESS;
}


forest::error mdd_node_manager::createEdge(bool term, dd_edge& e)
{
  if (e.getForest() != this) return forest::INVALID_OPERATION;
  if (term == false) return forest::INVALID_VARIABLE;

  if (reductionRule == forest::FULLY_REDUCED) {
    e.set(getTerminalNode(true), 0, domain::TERMINALS);
    return forest::SUCCESS;
  }

  // construct the edge bottom-up
  const int* h2l_map = expertDomain->getHeightsToLevelsMap();
  int h_sz = expertDomain->getNumVariables() + 1;
  int result = getTerminalNode(true);
  int curr = getTerminalNode(false);
  for (int i=1; i<h_sz; i++) {
    curr = createTempNodeMaxSize(h2l_map[i], false);
    setAllDownPtrsWoUnlink(curr, result);
    unlinkNode(result);
    result = reduceNode(curr);
  }
  e.set(result, 0, getNodeLevel(result));
  return forest::SUCCESS;
}


forest::error mdd_node_manager::evaluate(const dd_edge &f, const int* vlist,
    bool &term) const
{
  // assumption: vlist does not contain any special values (-1, -2, etc).
  // vlist contains a single element.
  int node = f.getNode();
  const int* h2l_map = expertDomain->getHeightsToLevelsMap();
  while (!isTerminalNode(node)) {
    node = getDownPtr(node, vlist[h2l_map[getNodeHeight(node)]]);
  }
  term = getBoolean(node);
  return forest::SUCCESS;
}


void mdd_node_manager::normalizeAndReduceNode(int& p, int& ev)
{
  assert(false);
}


forest::error mdd_node_manager::createEdge(const int* const* vlist,
    const int* terms, int N, dd_edge &e)
{
  return forest::INVALID_OPERATION;
}


forest::error mdd_node_manager::createEdge(const int* const* vlist,
    const float* terms, int n, dd_edge &e)
{
  return forest::INVALID_OPERATION;
}


forest::error mdd_node_manager::createEdge(const int* const* vlist,
    const int* const* vplist, int N, dd_edge &e)
{
  return forest::INVALID_OPERATION;
}


forest::error mdd_node_manager::createEdge(const int* const* vlist,
    const int* const* vplist, const int* terms, int N, dd_edge &e)
{
  return forest::INVALID_OPERATION;
}


forest::error mdd_node_manager::createEdge(const int* const* vlist,
    const int* const* vplist, const float* terms, int N, dd_edge &e)
{
  return forest::INVALID_OPERATION;
}


forest::error mdd_node_manager::createEdge(int val, dd_edge &e)
{
  return forest::INVALID_OPERATION;
}


forest::error mdd_node_manager::createEdge(float val, dd_edge &e)
{
  return forest::INVALID_OPERATION;
}


forest::error mdd_node_manager::evaluate(const dd_edge &f, const int* vlist,
    int &term) const
{
  return forest::INVALID_OPERATION;
}


forest::error mdd_node_manager::evaluate(const dd_edge &f, const int* vlist,
    float &term) const
{
  return forest::INVALID_OPERATION;
}


forest::error mdd_node_manager::evaluate(const dd_edge& f, const int* vlist,
    const int* vplist, bool &term) const
{
  return forest::INVALID_OPERATION;
}


forest::error mdd_node_manager::evaluate(const dd_edge& f, const int* vlist,
    const int* vplist, int &term) const
{
  return forest::INVALID_OPERATION;
}


forest::error mdd_node_manager::evaluate(const dd_edge& f, const int* vlist,
    const int* vplist, float &term) const
{
  return forest::INVALID_OPERATION;
}

