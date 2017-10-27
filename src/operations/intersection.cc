
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
#include "intersection.h"
#include "apply_base.h"

namespace MEDDLY {
  class inter_mdd;
  class inter_mxd;
  class inter_max_evplus;

  class inter_opname;
};

// ******************************************************************
// *                                                                *
// *                        inter_mdd  class                        *
// *                                                                *
// ******************************************************************

class MEDDLY::inter_mdd : public generic_binary_mdd {
  public:
    inter_mdd(const binary_opname* opcode, expert_forest* arg1,
      expert_forest* arg2, expert_forest* res);

  protected:
    virtual bool checkTerminals(node_handle a, node_handle b, node_handle& c);
};

MEDDLY::inter_mdd::inter_mdd(const binary_opname* opcode, 
  expert_forest* arg1, expert_forest* arg2, expert_forest* res)
  : generic_binary_mdd(opcode, arg1, arg2, res)
{
  operationCommutes();
}

bool MEDDLY::inter_mdd::checkTerminals(node_handle a, node_handle b, node_handle& c)
{
  if (a == 0 || b == 0) {
    c = 0;
    return true;
  }
  if (a==-1 && b==-1) {
    c = -1;
    return true;
  }
  if (a == -1) {
    if (arg2F == resF) {
      c = resF->linkNode(b);
      return true;
    } else {
      return false;
    }
  }
  if (a == b) {
    if (arg1F == arg2F && arg1F == resF) {
      c = resF->linkNode(b);
      return true;
    } else {
      return false;
    }
  }
  if (b == -1) {
    if (arg1F == resF) {
      c = resF->linkNode(a);
      return true;
    } else {
      return false;
    }
  }
  return false;
}



// ******************************************************************
// *                                                                *
// *                        inter_mxd  class                        *
// *                                                                *
// ******************************************************************

class MEDDLY::inter_mxd : public generic_binary_mxd {
  public:
    inter_mxd(const binary_opname* opcode, expert_forest* arg1,
      expert_forest* arg2, expert_forest* res);

  protected:
    virtual bool checkTerminals(node_handle a, node_handle b, node_handle& c);
};

MEDDLY::inter_mxd::inter_mxd(const binary_opname* opcode, 
  expert_forest* arg1, expert_forest* arg2, expert_forest* res)
  : generic_binary_mxd(opcode, arg1, arg2, res)
{
  operationCommutes();
}

bool MEDDLY::inter_mxd::checkTerminals(node_handle a, node_handle b, node_handle& c)
{
  if (a == 0 || b == 0) {
    c = 0;
    return true;
  }
  if (a==-1 && b==-1) {
    c = -1;
    return true;
  }
  if (a == b) {
    if (arg1F == arg2F && arg1F == resF) {
      c = resF->linkNode(b);
      return true;
    } else {
      return false;
    }
  }
  return false;
}

// ******************************************************************
// *                                                                *
// *                     inter_max_evplus  class                    *
// *                                                                *
// ******************************************************************

class MEDDLY::inter_max_evplus : public generic_binary_evplus {
  public:
    inter_max_evplus(const binary_opname* opcode, expert_forest* arg1,
      expert_forest* arg2, expert_forest* res);

  protected:
    virtual compute_table::search_key* findResult(long aev, node_handle a,
      long bev, node_handle b, long& cev, node_handle &c);
    virtual void saveResult(compute_table::search_key* key,
      long aev, node_handle a, long bev, node_handle b, long cev, node_handle c);

    virtual bool checkTerminals(long aev, node_handle a, long bev, node_handle b,
        long& cev, node_handle& c);
};

MEDDLY::inter_max_evplus::inter_max_evplus(const binary_opname* opcode,
  expert_forest* arg1, expert_forest* arg2, expert_forest* res)
  : generic_binary_evplus(opcode, arg1, arg2, res)
{
  operationCommutes();
}

MEDDLY::compute_table::search_key* MEDDLY::inter_max_evplus::findResult(long aev, node_handle a,
  long bev, node_handle b, long& cev, node_handle &c)
{
  compute_table::search_key* CTsrch = useCTkey();
  MEDDLY_DCASSERT(CTsrch);
  CTsrch->reset();
  if (can_commute && a > b) {
    CTsrch->write(0L);
    CTsrch->writeNH(b);
    CTsrch->write(aev - bev);
    CTsrch->writeNH(a);
  } else {
    CTsrch->write(0L);
    CTsrch->writeNH(a);
    CTsrch->write(bev - aev);
    CTsrch->writeNH(b);
  }
  compute_table::search_result &cacheFind = CT->find(CTsrch);
  if (!cacheFind) return CTsrch;
  cacheFind.read(cev);
  c = resF->linkNode(cacheFind.readNH());
  if (c != 0) {
    cev += (a > b ? bev : aev);
  }
  else {
    MEDDLY_DCASSERT(cev == 0);
  }
  doneCTkey(CTsrch);
  return 0;
}

void MEDDLY::inter_max_evplus::saveResult(compute_table::search_key* key,
  long aev, node_handle a, long bev, node_handle b, long cev, node_handle c)
{
  arg1F->cacheNode(a);
  arg2F->cacheNode(b);
  compute_table::entry_builder &entry = CT->startNewEntry(key);
  if (c == 0) {
    entry.writeResult(0L);
  }
  else {
    entry.writeResult(cev - (a > b ? bev : aev));
  }
  entry.writeResultNH(resF->cacheNode(c));
  CT->addEntry();
}

bool MEDDLY::inter_max_evplus::checkTerminals(long aev, node_handle a, long bev, node_handle b,
    long& cev, node_handle& c)
{
  if (a == 0 || b == 0) {
    cev = 0;
    c = 0;
    return true;
  }
  if (arg1F->isTerminalNode(a) && bev >= aev) {
    if (arg2F == resF) {
      cev = bev;
      c = resF->linkNode(b);
      return true;
    }
    else {
      return false;
    }
  }
  if (arg2F->isTerminalNode(b) && aev >= bev) {
    if (arg1F == resF) {
      cev = aev;
      c = resF->linkNode(a);
      return true;
    }
    else {
      return false;
    }
  }
  if (a == b) {
    if (arg1F == arg2F && arg2F == resF) {
      cev = MAX(aev, bev);
      c = resF->linkNode(a);
      return true;
    }
    else {
      return false;
    }
  }
  return false;
}

// ******************************************************************
// *                                                                *
// *                       inter_opname class                       *
// *                                                                *
// ******************************************************************

class MEDDLY::inter_opname : public binary_opname {
  public:
    inter_opname();
    virtual binary_operation* buildOperation(expert_forest* a1, 
      expert_forest* a2, expert_forest* r) const;
};

MEDDLY::inter_opname::inter_opname()
 : binary_opname("Intersection")
{
}

MEDDLY::binary_operation* 
MEDDLY::inter_opname::buildOperation(expert_forest* a1, expert_forest* a2, 
  expert_forest* r) const
{
  if (0==a1 || 0==a2 || 0==r) return 0;

  if (  
    (a1->getDomain() != r->getDomain()) || 
    (a2->getDomain() != r->getDomain()) 
  )
    throw error(error::DOMAIN_MISMATCH, __FILE__, __LINE__);

  if (
    (a1->isForRelations() != r->isForRelations()) ||
    (a2->isForRelations() != r->isForRelations()) ||
    (a1->getEdgeLabeling() != r->getEdgeLabeling()) ||
    (a2->getEdgeLabeling() != r->getEdgeLabeling())
  )
    throw error(error::TYPE_MISMATCH, __FILE__, __LINE__);

  if (r->getEdgeLabeling() == forest::MULTI_TERMINAL) {
    if (r->isForRelations())
      return new inter_mxd(this, a1, a2, r);
    else
      return new inter_mdd(this, a1, a2, r);
  }

  if (r->getEdgeLabeling() == forest::EVPLUS) {
    if (r->isForRelations()) {
      throw error(error::NOT_IMPLEMENTED);
    }
    else {
      return new inter_max_evplus(this, a1, a2, r);
    }
  }

  throw error(error::NOT_IMPLEMENTED, __FILE__, __LINE__);
}

// ******************************************************************
// *                                                                *
// *                           Front  end                           *
// *                                                                *
// ******************************************************************

MEDDLY::binary_opname* MEDDLY::initializeIntersection()
{
  return new inter_opname;
}

