
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



// TODO: Testing

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
  alloc = 0;
  ealloc = 0;
  size = 0;
  nnzs = 0;
  level = 0;
}

/*
  Initializers 
*/

void MEDDLY::unpacked_node::initRedundant(const expert_forest *f, int k, 
  node_handle node, bool full)
{
  MEDDLY_DCASSERT(f);
  MEDDLY_DCASSERT(0==f->edgeBytes());
  int nsize = f->getLevelSize(k);
  bind_to_forest(f, k, nsize, full);
  for (int i=0; i<nsize; i++) {
    down[i] = node;
  }
  if (!full) {
    for (int i=0; i<nsize; i++) index[i] = i;
    nnzs = nsize;
  }
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
  MEDDLY_DCASSERT(sizeof(int)==f->edgeBytes());
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

void MEDDLY::unpacked_node
::show(output &s, const expert_forest* parent, bool verb) const
{
  int stop;
  if (isSparse()) {
    if (verb) s << "nnzs: " << long(size) << " ";
    s << "down: (";
    stop = nnzs;
  } else {
    if (verb) s << "size: " << long(size) << " ";
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

  if (isSparse()) {
    s.put(')');
  } else {
    s.put(']');
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
    if (0==down) throw error(error::INSUFFICIENT_MEMORY);
    index = (int*) realloc(index, nalloc*sizeof(int));
    if (0==index) throw error(error::INSUFFICIENT_MEMORY);
    alloc = nalloc;
  }
  if (edge_bytes * size > ealloc) {
    int nalloc = ((edge_bytes * size)/8+1)*8;
    MEDDLY_DCASSERT(nalloc>0);
    MEDDLY_DCASSERT(nalloc>ealloc);
    edge = realloc(edge, nalloc);
    if (0==edge) throw error(error::INSUFFICIENT_MEMORY);
    ealloc = nalloc;
  }
}

void MEDDLY::unpacked_node::bind_to_forest(const expert_forest* f, int k, int ns, bool full)
{
  parent = f;
  level = k;
  is_full = full;
  edge_bytes = f->edgeBytes();
  resize(ns);

  // Allocate headers
  ext_h_size = parent->hashedHeaderBytes();
  if (ext_h_size > ext_h_alloc) {
    ext_h_alloc = ((ext_h_size/8)+1)*8;
    MEDDLY_DCASSERT(ext_h_alloc > ext_h_size);
    MEDDLY_DCASSERT(ext_h_alloc>0);
    extra_hashed =  realloc(extra_hashed, ext_h_alloc);
    if (0==extra_hashed) throw error(error::INSUFFICIENT_MEMORY);
  }

  ext_uh_size = parent->unhashedHeaderBytes();
  if (ext_uh_size > ext_uh_alloc) {
    ext_uh_alloc = ((ext_uh_size/8)+1)*8;
    MEDDLY_DCASSERT(ext_uh_alloc > ext_uh_size);
    MEDDLY_DCASSERT(ext_uh_alloc>0);
    extra_unhashed =  realloc(extra_unhashed, ext_uh_alloc);
    if (0==extra_unhashed) throw error(error::INSUFFICIENT_MEMORY);
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
    if (0==extra_hashed) throw error(error::INSUFFICIENT_MEMORY);
  }
}
*/

void MEDDLY::unpacked_node::computeHash()
{
#ifdef DEVELOPMENT_CODE
  MEDDLY_DCASSERT(!has_hash);
#endif
  
  hash_stream s;
  s.start(level);

  if (ext_h_size) {
    s.push(extra_hashed, ext_h_size);
  }
  
  if (isSparse()) {
    if (parent->areEdgeValuesHashed()) {
      for (int z=0; z<nnzs; z++) {
        MEDDLY_DCASSERT(d(z)!=parent->getTransparentNode());
        s.push(i(z), d(z), ei(z));
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
        if (d(n)!=parent->getTransparentNode()) {
          s.push(n, d(n), ei(n));
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

// ******************************************************************
// *                                                                *
// *                                                                *
// *                      node_builder methods                      *
// *                                                                *
// *                                                                *
// ******************************************************************

#ifdef USE_NODE_BUILDERS

MEDDLY::node_builder::node_builder()
{
  parent = 0;
  extra_hashed = 0;
  hhbytes = 0;
  extra_unhashed = 0;
  down = 0;
  indexes = 0;
  edge = 0;
#ifdef DEVELOPMENT_CODE
  has_hash = false;
#endif
}

MEDDLY::node_builder::~node_builder()
{
  free(extra_hashed);
  free(extra_unhashed);
  free(down);
  free(indexes);
  free(edge);
}

void MEDDLY::node_builder::init(int k, const expert_forest* p)
{
  MEDDLY_DCASSERT(p);
  parent = p;
  level = k;
  hhbytes = parent->hashedHeaderBytes();
  if (hhbytes) {
    extra_hashed = malloc(hhbytes);
    if (0==extra_hashed)
      throw error(error::INSUFFICIENT_MEMORY);
  }
  if (parent->unhashedHeaderBytes()) {
    extra_unhashed = malloc(parent->unhashedHeaderBytes());
    if (0==extra_unhashed)
      throw error(error::INSUFFICIENT_MEMORY);
  }
  size = 0;
  alloc = 0;
  lock = false;
  edge_bytes = parent->edgeBytes();
}

void MEDDLY::node_builder::show(output &s, bool verb) const
{
  if (isSparse()) {
    if (verb) s << "nnzs: " << long(size) << " ";
    s << "down: (";
  } else {
    if (verb) s << "size: " << long(size) << " ";
    s << "down: [";
  }

  for (int z=0; z<size; z++) {
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

  if (isSparse()) {
    s.put(')');
  } else {
    s.put(']');
  }
}

void MEDDLY::node_builder::computeHash()
{
#ifdef DEVELOPMENT_CODE
  MEDDLY_DCASSERT(!has_hash);
#endif
  
  hash_stream s;
  s.start(level);

  if (hhbytes) {
    s.push(extra_hashed, hhbytes);
  }

  if (isSparse()) {
    if (parent->areEdgeValuesHashed()) {
      for (int z=0; z<size; z++) {
        MEDDLY_DCASSERT(d(z)!=parent->getTransparentNode());
        s.push(i(z), d(z), ei(z));
      }
    } else {
      for (int z=0; z<size; z++) {
        MEDDLY_DCASSERT(d(z)!=parent->getTransparentNode());
        s.push(i(z), d(z));
      }
    }
  } else {
	node_handle tv=parent->getTransparentNode();
    if (parent->areEdgeValuesHashed()) {
      for (int n=0; n<size; n++) {
        if (d(n)!=tv) {
          s.push(n, d(n), ei(n));
        }
      }
    } else {
      for (int n=0; n<size; n++) {
        if (d(n)!=tv) {
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


void MEDDLY::node_builder::enlarge()
{
  if (size <= alloc) return;
  alloc = ((size / 8)+1) * 8;
  MEDDLY_DCASSERT(alloc > size);
  down = (node_handle*) realloc(down, alloc * sizeof(node_handle));
  if (0==down) throw error(error::INSUFFICIENT_MEMORY);
  indexes = (int*) realloc(indexes, alloc * sizeof(int));
  if (0==indexes) throw error(error::INSUFFICIENT_MEMORY);
  if (parent->edgeBytes()>0) {
    edge = realloc(edge, alloc * parent->edgeBytes());
    if (0==edge) throw error(error::INSUFFICIENT_MEMORY);
  }
}

#endif

// ******************************************************************
// *                                                                *
// *                                                                *
// *                      node_storage methods                      *
// *                                                                *
// *                                                                *
// ******************************************************************

MEDDLY::node_storage::node_storage()
{
  parent = 0;
  stats = 0;
  counts = 0;
  nexts = 0;
}

MEDDLY::node_storage::~node_storage()
{
  // nothing, derived classes must handle everything
}

void MEDDLY::node_storage::initForForest(expert_forest* f)
{
  MEDDLY_DCASSERT(0==parent);
  parent = f;
  stats = &parent->changeStats();
  localInitForForest(f);
}

void MEDDLY::node_storage
::writeNode(output &s, node_address, const node_handle*) const
{
  throw error(error::NOT_IMPLEMENTED);
}

#ifndef INLINED_COUNT
MEDDLY::node_handle
MEDDLY::node_storage::getCountOf(node_address addr) const
{
  MEDDLY_DCASSERT(counts);
  MEDDLY_DCASSERT(addr > 0);
  return counts[addr];
}

void
MEDDLY::node_storage::setCountOf(node_address addr, MEDDLY::node_handle c)
{
  MEDDLY_DCASSERT(counts);
  MEDDLY_DCASSERT(addr > 0);
  counts[addr] = c;
}

MEDDLY::node_handle
MEDDLY::node_storage::incCountOf(node_address addr)
{
  MEDDLY_DCASSERT(counts);
  MEDDLY_DCASSERT(addr > 0);
  return ++counts[addr];
}
;

MEDDLY::node_handle
MEDDLY::node_storage::decCountOf(node_address addr)
{
  MEDDLY_DCASSERT(counts);
  MEDDLY_DCASSERT(addr > 0);
  return --counts[addr];
}
#endif

#ifndef INLINED_NEXT
MEDDLY::node_handle
MEDDLY::node_storage::getNextOf(node_address addr) const
{
  MEDDLY_DCASSERT(nexts);
  MEDDLY_DCASSERT(addr > 0);
  return nexts[addr];
}

void
MEDDLY::node_storage::setNextOf(node_address addr, MEDDLY::node_handle n)
{
  MEDDLY_DCASSERT(nexts);
  MEDDLY_DCASSERT(addr > 0);
  nexts[addr] = n;
}
#endif


void MEDDLY::node_storage::dumpInternal(output &s, unsigned flags) const
{
  dumpInternalInfo(s);
  s << "Data array by record:\n";
  for (node_address a=1; a > 0; ) {
    s.flush();
    a = dumpInternalNode(s, a, flags);
  } // for a
  dumpInternalTail(s);
  s.flush();
}

void MEDDLY::node_storage::localInitForForest(const expert_forest* f)
{
  // default - do nothing
}
