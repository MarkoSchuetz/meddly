
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



// TODO: Testing

#include <map>
#include "defines.h"
#include "hash_stream.h"

// ******************************************************************
// *                                                                *
// *                                                                *
// *                     unpacked_node  methods                     *
// *                                                                *
// *                                                                *
// ******************************************************************

MEDDLY::unpacked_node::unpacked_node()
{
  parent = 0;
  extra_unhashed = 0;
  ext_uh_alloc = 0;
  ext_uh_size = 0;
  extra_hashed = 0;
  ext_h_alloc = 0;
  ext_h_size = 0;
  down = 0;
  index = 0;
  edge = 0;
  is_extensible = false;
  alloc = 0;
  ealloc = 0;
  size = 0;
  nnzs = 0;
  level = 0;
#ifdef DEVELOPMENT_CODE
  has_hash = false;
#endif
}

MEDDLY::unpacked_node::~unpacked_node()
{
  clear();
}

void MEDDLY::unpacked_node::clear()
{
  free(extra_unhashed);
  free(extra_hashed);
  free(down);
  free(index);
  free(edge);
  down = 0;
  index = 0;
  edge = 0;
  is_extensible = false;
  alloc = 0;
  ealloc = 0;
  size = 0;
  nnzs = 0;
  level = 0;
}

/*
  Initializers 

  Extensible nodes
        + Every node at level k, where level k represents an extensible
          variable, is represented by an extensible node.
        + Whether a node is extensible or not is determined by querying
          the corresponding level's property.
        + The last downpointer in an extensible node is considered to
          repeat for all indices till +infinity.
*/

void MEDDLY::unpacked_node::initRedundant(const expert_forest *f, int k, 
  node_handle node, bool full)
{
  MEDDLY_DCASSERT(f);
  MEDDLY_DCASSERT(0==f->edgeBytes());
  int nsize = f->isExtensibleLevel(k) ? 1 : f->getLevelSize(k);
  bind_to_forest(f, k, nsize, full);
  for (int i=0; i<nsize; i++) {
    down[i] = node;
  }
  if (!full) {
    for (int i=0; i<nsize; i++) index[i] = i;
    nnzs = nsize;
  }
  is_extensible = f->isExtensibleLevel(k);
}

void MEDDLY::unpacked_node::initRedundant(const expert_forest *f, int k, 
  float ev, node_handle node, bool full)
{
  MEDDLY_DCASSERT(f);
  MEDDLY_DCASSERT(sizeof(float)==f->edgeBytes());
  int nsize = f->getLevelSize(k);
  bind_to_forest(f, k, nsize, full);
  for (int i=0; i<nsize; i++) {
    down[i] = node;
    ((float*)edge)[i] = ev;
  }
  if (!full) {
    for (int i=0; i<nsize; i++) index[i] = i;
    nnzs = nsize;
  }
}

void MEDDLY::unpacked_node::initRedundant(const expert_forest *f, int k, 
  int ev, node_handle node, bool full)
{
  MEDDLY_DCASSERT(f);
//  MEDDLY_DCASSERT(sizeof(int)==f->edgeBytes());
  MEDDLY_DCASSERT(sizeof(long)==f->edgeBytes());
  int nsize = f->getLevelSize(k);
  bind_to_forest(f, k, nsize, full);
  for (int i=0; i<nsize; i++) {
    down[i] = node;
    ((int*)edge)[i] = ev;
  }
  if (!full) {
    for (int i=0; i<nsize; i++) index[i] = i;
    nnzs = nsize;
  }
}

void MEDDLY::unpacked_node::initRedundant(const expert_forest *f, int k,
  long ev, node_handle node, bool full)
{
  MEDDLY_DCASSERT(f);
  MEDDLY_DCASSERT((sizeof ev)==f->edgeBytes());
  int nsize = f->getLevelSize(k);
  bind_to_forest(f, k, nsize, full);
  for (int i=0; i<nsize; i++) {
    down[i] = node;
    ((long*)edge)[i] = ev;
  }
  if (!full) {
    for (int i=0; i<nsize; i++) index[i] = i;
    nnzs = nsize;
  }
}

void MEDDLY::unpacked_node::initIdentity(const expert_forest *f, int k, 
  int i, node_handle node, bool full)
{
  MEDDLY_DCASSERT(f);
  MEDDLY_DCASSERT(0==f->edgeBytes());
  int nsize = f->getLevelSize(k);
  if (full) {
    bind_to_forest(f, k, nsize, full);
    memset(down, 0, nsize * sizeof(node_handle));
    down[i] = node;
  } else {
    bind_to_forest(f, k, 1, full);
    nnzs = 1;
    down[0] = node;
    index[0] = i;
  }
}

void MEDDLY::unpacked_node::initIdentity(const expert_forest *f, int k, 
  int i, int ev, node_handle node, bool full)
{
  MEDDLY_DCASSERT(f);
  MEDDLY_DCASSERT(sizeof(int)==f->edgeBytes());
  int nsize = f->getLevelSize(k);
  if (full) {
    bind_to_forest(f, k, nsize, full);
    memset(down, 0, nsize * sizeof(node_handle));
    memset(edge, 0, nsize * sizeof(int));
    down[i] = node;
    ((int*)edge)[i] = ev;
  } else {
    bind_to_forest(f, k, 1, full);
    nnzs = 1;
    down[0] = node;
    ((int*)edge)[0] = ev;
    index[0] = i;
  }
}

void MEDDLY::unpacked_node::initIdentity(const expert_forest *f, int k, 
  int i, long ev, node_handle node, bool full)
{
  MEDDLY_DCASSERT(f);
  MEDDLY_DCASSERT((sizeof ev)==f->edgeBytes());
  int nsize = f->getLevelSize(k);
  if (full) {
    bind_to_forest(f, k, nsize, full);
    memset(down, 0, nsize * sizeof(node_handle));
    memset(edge, 0, nsize * sizeof(long));
    down[i] = node;
    ((long*)edge)[i] = ev;
  } else {
    bind_to_forest(f, k, 1, full);
    nnzs = 1;
    down[0] = node;
    ((long*)edge)[0] = ev;
    index[0] = i;
  }
}

void MEDDLY::unpacked_node::initIdentity(const expert_forest *f, int k,
  int i, float ev, node_handle node, bool full)
{
  MEDDLY_DCASSERT(f);
  MEDDLY_DCASSERT(sizeof(float)==f->edgeBytes());
  int nsize = f->getLevelSize(k);
  if (full) {
    bind_to_forest(f, k, nsize, full);
    memset(down, 0, nsize * sizeof(node_handle));
    memset(edge, 0, nsize * sizeof(float));
    down[i] = node;
    ((float*)edge)[i] = ev;
  } else {
    bind_to_forest(f, k, 1, full);
    nnzs = 1;
    down[0] = node;
    ((float*)edge)[0] = ev;
    index[0] = i;
  }
}

/*
  Usage
*/

void MEDDLY::unpacked_node::show(output &s, bool details) const
{
  int stop;
  if (isSparse()) {
    if (details) s << "nnzs: " << long(nnzs) << (isExtensible()? "*": "") << " ";
    s << "down: (";
    stop = nnzs;
  } else {
    if (details) s << "size: " << long(size) << (isExtensible()? "*": "") << " ";
    s << "down: [";
    stop = size;
  }

  for (int z=0; z<stop; z++) {
    if (isSparse()) {
      if (z) s << ", ";
      s << long(i(z)) << ":";
    } else {
      if (z) s.put('|');
    }
    if (parent->edgeBytes()) {
      s.put('<');
      parent->showEdgeValue(s, eptr(z));
      s.put(", ");
    }
    if (parent->isTerminalNode(d(z))) {
      parent->showTerminal(s, d(z));
    } else {
      s.put(long(d(z)));
    }
    if (parent->edgeBytes()) s.put('>');
  }

  if (isExtensible()) s.put('*');

  if (isSparse()) {
    s.put(')');
  } else {
    s.put(']');
  }

  // show extra header stuff
  if (ext_uh_size) {
    parent->showUnhashedHeader(s, extra_unhashed);
  }
  if (ext_h_size) {
    parent->showHashedHeader(s, extra_hashed);
  }
}

void MEDDLY::unpacked_node::write(output &s, const node_handle* map) const
{
  int stop;
  if (isSparse()) {
    s.put(long(-nnzs));
    stop = nnzs;
  } else {
    s.put(long(size));
    stop = size;
  }

  //
  // write indexes (sparse only)
  //
  if (isSparse()) {
    s.put('\n');
    s.put('\t');
    for (int z=0; z<nnzs; z++) {
      s.put(' ');
      s.put(long(i(z)));
    }
  }

  //
  // write down pointers
  //
  s.put('\n');
  s.put('\t');
  for (int z=0; z<stop; z++) {
    s.put(' ');
    if (parent->isTerminalNode(d(z))) {
      parent->writeTerminal(s, d(z));
    } else {
      s.put(long( map ? map[d(z)] : d(z) ));
    }
  }

  // 
  // write edges
  //
  if (parent->edgeBytes()) {
    s.put('\n');
    s.put('\t');
    for (int z=0; z<stop; z++) {
      s.put(' ');
      parent->showEdgeValue(s, eptr(z));
    }
  }
  s.put('\n');


  // write extra header stuff
  // this goes LAST so we can read it into a built node
  if (ext_uh_size) {
    parent->writeUnhashedHeader(s, extra_unhashed);
  }
  if (ext_h_size) {
    parent->writeHashedHeader(s, extra_hashed);
  }

}

void MEDDLY::unpacked_node
::resize(int ns)
{
  size = ns;
  nnzs = ns;
  if (size > alloc) {
    int nalloc = ((ns/8)+1)*8;
    MEDDLY_DCASSERT(nalloc > ns);
    MEDDLY_DCASSERT(nalloc>0);
    MEDDLY_DCASSERT(nalloc>alloc);
    down = (node_handle*) realloc(down, nalloc*sizeof(node_handle));
    if (0==down) throw error(error::INSUFFICIENT_MEMORY, __FILE__, __LINE__);
    index = (int*) realloc(index, nalloc*sizeof(int));
    if (0==index) throw error(error::INSUFFICIENT_MEMORY, __FILE__, __LINE__);
    alloc = nalloc;
  }
  if (edge_bytes * size > ealloc) {
    int nalloc = ((edge_bytes * size)/8+1)*8;
    MEDDLY_DCASSERT(nalloc>0);
    MEDDLY_DCASSERT(nalloc>ealloc);
    edge = realloc(edge, nalloc);
    if (0==edge) throw error(error::INSUFFICIENT_MEMORY, __FILE__, __LINE__);
    ealloc = nalloc;
  }
}

void MEDDLY::unpacked_node::bind_to_forest(const expert_forest* f,
    int k, int ns, bool full)
{
  parent = f;
  level = k;
  is_full = full;
  markAsNotExtensible();
  edge_bytes = f->edgeBytes();
  resize(ns);

  // Allocate headers
  ext_h_size = parent->hashedHeaderBytes();
  if (ext_h_size > ext_h_alloc) {
    ext_h_alloc = ((ext_h_size/8)+1)*8;
    MEDDLY_DCASSERT(ext_h_alloc > ext_h_size);
    MEDDLY_DCASSERT(ext_h_alloc>0);
    extra_hashed =  realloc(extra_hashed, ext_h_alloc);
    if (0==extra_hashed) throw error(error::INSUFFICIENT_MEMORY, __FILE__, __LINE__);
  }

  ext_uh_size = parent->unhashedHeaderBytes();
  if (ext_uh_size > ext_uh_alloc) {
    ext_uh_alloc = ((ext_uh_size/8)+1)*8;
    MEDDLY_DCASSERT(ext_uh_alloc > ext_uh_size);
    MEDDLY_DCASSERT(ext_uh_alloc>0);
    extra_unhashed =  realloc(extra_unhashed, ext_uh_alloc);
    if (0==extra_unhashed) throw error(error::INSUFFICIENT_MEMORY, __FILE__, __LINE__);
  }
}


/*
void MEDDLY::unpacked_node
::resize_header(int extra_bytes)
{
  ext_h_size = extra_bytes;
  if (ext_h_size > ext_h_alloc) {
    ext_h_alloc = ((ext_h_size/8)+1)*8;
    MEDDLY_DCASSERT(ext_h_alloc > ext_h_size);
    MEDDLY_DCASSERT(ext_h_alloc>0);
    extra_hashed =  realloc(extra_hashed, ext_h_alloc);
    if (0==extra_hashed) throw error(error::INSUFFICIENT_MEMORY, __FILE__, __LINE__);
  }
}
*/

void MEDDLY::unpacked_node::computeHash()
{
  MEDDLY_DCASSERT(!has_hash);
  trim();

  hash_stream s;
  s.start(0);

  if (ext_h_size) {
    s.push(extra_hashed, ext_h_size);
  }
  
  if (isSparse()) {
    if (parent->areEdgeValuesHashed()) {
      for (int z=0; z<nnzs; z++) {
        MEDDLY_DCASSERT(!parent->isTransparentEdge(d(z), eptr(z)));
        s.push(i(z), d(z));
        s.push(eptr(z), parent->edgeBytes());
      }
    } else {
      for (int z=0; z<nnzs; z++) {
        MEDDLY_DCASSERT(d(z)!=parent->getTransparentNode());
        s.push(i(z), d(z));
      }
    }
  } else {
    if (parent->areEdgeValuesHashed()) {
      for (int n=0; n<size; n++) {
        if (!parent->isTransparentEdge(d(n), eptr(n))) {
          s.push(n, d(n));
          s.push(eptr(n), parent->edgeBytes());
        }
      }
    } else {
      for (int n=0; n<size; n++) {
        if (d(n)!=parent->getTransparentNode()) {
          s.push(n, d(n));
        }
      }
    }
  }

  h = s.finish();
#ifdef DEVELOPMENT_CODE
  has_hash = true;
#endif
}


// check is the node is written in order,
// if not rearrange it in ascending order of indices.
void MEDDLY::unpacked_node::sort()
{
  if (!isSparse()) return;

  int k = 1;
  for (k = 1; k < getNNZs() && i(k-1) < i(k) ; k++);
  if (k == getNNZs()) return; // already sorted
  
  // sort from (k-1) to (nnz-1)
  --k;
  std::map<int, int> sorter;
  for (int m = k; m < getNNZs(); m++) {
    sorter[i(m)] = m;
  }

  // allocate new arrays for index, node handles and edge-values
  node_handle* old_down = down;
  int* old_index = index;
  void* old_edge = edge;
  int old_nnzs = nnzs;

  down = 0;
  index = 0;
  edge = 0;
  size = 0;
  nnzs = 0;
  alloc = 0;
  ealloc = 0;
  resize(old_nnzs);

  // copy into new arrays
  memcpy(down, old_down, sizeof(node_handle) * k);
  memcpy(index, old_index, sizeof(int) * k);
  memcpy(edge, old_edge, edge_bytes * k);

  for (std::map<int, int>::iterator s_iter = sorter.begin();
    s_iter != sorter.end(); s_iter++, k++) {
    int old_location = s_iter->second;
    index[k] = old_index[old_location];
    down[k] = old_down[old_location];
    memcpy((char*)edge + (k * edge_bytes),
        (char*)old_edge + (old_location * edge_bytes), edge_bytes);
  }

  free(old_down);
  free(old_index);
  free(old_edge);
}

// remove all edges starting at the given index
void MEDDLY::unpacked_node::trim()
{
  if (!isExtensible()) return;
  if (isTrim()) return;

  // If extensible edge is transparent, mark the node as not-extensible and return
  if (d((isSparse()? getNNZs() : getSize()) - 1) == parent->getTransparentNode()) {
    markAsNotExtensible();
    return;
  }

  MEDDLY_DCASSERT(isExtensible() && !isTrim());

  if (isSparse()) {
    int z = getNNZs()-1;
    while (z > 0 && (i(z-1)+1) == i(z) && d(z-1) == d(z)) {
      const_cast<expert_forest*>(parent)->unlinkNode(d(z));
      z--;
    }
    if (z != (getNNZs() - 1)) {
      // node is smaller than before, shrink it to the correct size.
      shrinkSparse(z+1);
    }
  } else {
    int z = getSize()-1;
    while (z > 0 && d(z-1) == d(z)) {
      const_cast<expert_forest*>(parent)->unlinkNode(d(z));
      z--;
    }
    if (z != (getSize()-1)) {
      shrinkFull(z+1);
    }
  }

  MEDDLY_DCASSERT(isExtensible() && isTrim());
}

// checks if the node is has no trailing redundant edges
bool MEDDLY::unpacked_node::isTrim() const
{
  if (!isExtensible()) return true;

  if (isSparse()) {
    int nnz = getNNZs();
    return (nnz < 2 || i(nnz-1) != (i(nnz-2)+1) || d(nnz-1) != d(nnz-2));
  } else {
    int size = getSize();
    return (size < 2 || d(size-1) != d(size-2));
  }
}

// checks if the node indices are in ascending order
bool MEDDLY::unpacked_node::isSorted() const
{
  if (!isSparse()) return true;

  for (int z = 1; z < getNNZs(); z++) {
    if (i(z-1) >= i(z)) return false;
  }

  return true;
}


// ******************************************************************
// *                                                                *
// *                                                                *
// *                   node_storage_style methods                   *
// *                                                                *
// *                                                                *
// ******************************************************************

MEDDLY::node_storage_style::node_storage_style(const char* n)
{
  name = n;
}

MEDDLY::node_storage_style::~node_storage_style()
{
}

// ******************************************************************
// *                                                                *
// *                                                                *
// *                      node_storage methods                      *
// *                                                                *
// *                                                                *
// ******************************************************************

MEDDLY::node_storage::node_storage(const char* n, expert_forest* f)
{
  style_name = n;
  parent = f;
}

MEDDLY::node_storage::~node_storage()
{
  // nothing, derived classes must handle everything
}

void MEDDLY::node_storage::dumpInternal(output &s, unsigned flags) const
{
  dumpInternalInfo(s);
  s << "Data array by record:\n";
  for (node_address a=firstNodeAddress(); a > 0; ) {
    s.flush();
    a = dumpInternalNode(s, a, flags);
  } // for a
  dumpInternalTail(s);
  s.flush();
}

