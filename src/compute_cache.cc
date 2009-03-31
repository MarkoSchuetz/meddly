
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



#include "../src/compute_cache.h"


const float expansionFactor = 1.25;


compute_cache::compute_cache()
: nodes(0), nodeCount(1024), lastNode(-1),
  data(0), dataCount(1024), lastData(-1),
  recycledNodes(-1), recycledFront(0),
  ht(0), hits(0), pings(0)
{
  // initialize node and data arrays
  nodes = (cache_entry *) malloc(nodeCount * sizeof(cache_entry));
  assert(nodes != NULL);
  for (int i = 0; i < nodeCount; ++i)
  {
    nodes[i].owner = 0;
    nodes[i].next = getNull();
    setDataOffset(nodes[i], -1);
  }
  data = (int *) malloc(dataCount * sizeof(int));
  assert(data != NULL);
  memset(data, 0, dataCount * sizeof(int));
  
  // create new hash table
  ht = new hash_table<compute_cache>(this);
  fsht = 0;
}


compute_cache::~compute_cache()
{
  // delete hash table
  if (ht) { delete ht; ht = 0; }
  if (fsht) { delete fsht; fsht = 0; }

  // go through all nodes and call discardEntry for each valid node
  cache_entry* end = nodes + lastNode + 1;
  for (cache_entry* curr = nodes; curr != end; ++curr)
  {
    if (!isFreeNode(*curr)) {
      DCASSERT(curr->dataOffset != -1);
      curr->owner->op->discardEntry(curr->owner, getDataAddress(*curr));
    }
    // note: we are not recycling the nodes here since we are just going
    // to delete this structure
  }

  // free data and nodes arrays
  free(data);
  free(nodes);
}


bool compute_cache::setPolicy(bool chaining, unsigned maxSize)
{
  // some data is already in cache; abort
  if (lastData > -1) return false;

  // delete existing hash tables
  if (fsht != 0) { delete fsht; fsht = 0; }
  if (ht != 0) { delete ht; ht = 0; }

  if (chaining) {
    // create hash table with chaining
    ht = new hash_table<compute_cache>(this, maxSize);
  }
  else {
    // create hash table with no chaining
    fsht = new fixed_size_hash_table<compute_cache>(this, maxSize);
  }
  return true;
}


void compute_cache::show(FILE* s, int h) const
{
  nodes[h].owner->op->showEntry(nodes[h].owner, s, getDataAddress(nodes[h]));
}


void compute_cache::show(FILE *s, bool verbose) const
{ 
  fprintf(s, "  Number of slots:  %d\n", nodeCount);
  fprintf(s, "  Memory usage:     %d\n",
      dataCount * sizeof(int) + nodeCount * sizeof(cache_entry));
  fprintf(s, "    Nodes[]:        %d\n", nodeCount * sizeof(cache_entry));
  fprintf(s, "    Data[]:         %d\n", dataCount * sizeof(int));
  fprintf(s, "  Pings:            %d\n", pings);
  fprintf(s, "  Hits:             %d\n", hits);
  fprintf(s, "Internal hash table info:\n");
  assert(ht == 0 || fsht == 0);
  if (ht != 0) ht->show(s, verbose);
  if (fsht != 0) fsht->show(s, verbose);
}


void compute_cache::expandNodes()
{
  assert(nodeCount != 0);
  int newNodeCount = int(nodeCount * expansionFactor);
  cache_entry* tempNodes =
    (cache_entry *) realloc(nodes, newNodeCount * sizeof(cache_entry));
  assert(tempNodes != NULL);
  nodes = tempNodes;
  for (int i = nodeCount; i < newNodeCount; ++i)
  {
    nodes[i].owner = 0;
    // nodes[i].data = 0;
    nodes[i].next = getNull();
    setDataOffset(nodes[i], -1);
  }
  nodeCount = newNodeCount;
}


void compute_cache::expandData()
{
  int newDataCount = int(dataCount * expansionFactor);
  data = (int *) realloc(data, newDataCount * sizeof(int));
  assert(data != NULL);
  memset(data + dataCount, 0, (newDataCount - dataCount) * sizeof(int));
  dataCount = newDataCount;
}


void compute_cache::removeStales()
{
  assert(ht != 0 || fsht != 0);
  if (ht) ht->removeStaleEntries();
  if (fsht) fsht->removeStaleEntries();
}

int compute_cache::getNumEntries() const
{
  assert(ht != 0 || fsht != 0);
  return (ht)? ht->getEntriesCount(): fsht->getEntriesCount();
}