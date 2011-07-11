
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
#include "minus.h"
#include "apply_base.h"

namespace MEDDLY {
  class minus_mdd;
  class minus_mxd;
  class minus_evplus;
  class minus_evtimes;

  class minus_opname;
};

// ******************************************************************
// *                                                                *
// *                        minus_mdd  class                        *
// *                                                                *
// ******************************************************************

class MEDDLY::minus_mdd : public generic_binary_mdd {
  public:
    minus_mdd(const binary_opname* opcode, expert_forest* arg1,
      expert_forest* arg2, expert_forest* res);

  protected:
    virtual bool checkTerminals(int a, int b, int& c);
};

MEDDLY::minus_mdd::minus_mdd(const binary_opname* opcode, 
  expert_forest* arg1, expert_forest* arg2, expert_forest* res)
  : generic_binary_mdd(opcode, arg1, arg2, res)
{
}

bool MEDDLY::minus_mdd::checkTerminals(int a, int b, int& c)
{
  if (arg1F->isTerminalNode(a) && arg2F->isTerminalNode(b)) {
    if (resF->getRangeType() == forest::INTEGER) {
      c = resF->getTerminalNode(arg1F->getInteger(a) - arg2F->getInteger(b));
    } else {
      DCASSERT(resF->getRangeType() == forest::REAL);
      c = resF->getTerminalNode(arg1F->getReal(a) - arg2F->getReal(b));
    }
    return true;
  }
  if (0==b) {
    if (arg1F == resF) {
      c = arg1F->linkNode(a);
      return true;
    }
  }
  return false;
}



// ******************************************************************
// *                                                                *
// *                        minus_mxd  class                        *
// *                                                                *
// ******************************************************************

class MEDDLY::minus_mxd : public generic_binary_mxd {
  public:
    minus_mxd(const binary_opname* opcode, expert_forest* arg1,
      expert_forest* arg2, expert_forest* res);

  protected:
    virtual bool checkTerminals(int a, int b, int& c);
};

MEDDLY::minus_mxd::minus_mxd(const binary_opname* opcode, 
  expert_forest* arg1, expert_forest* arg2, expert_forest* res)
  : generic_binary_mxd(opcode, arg1, arg2, res)
{
}

bool MEDDLY::minus_mxd::checkTerminals(int a, int b, int& c)
{
  if (arg1F->isTerminalNode(a) && arg2F->isTerminalNode(b)) {
    if (resF->getRangeType() == forest::INTEGER) {
      c = resF->getTerminalNode(arg1F->getInteger(a) - arg2F->getInteger(b));
    } else {
      DCASSERT(resF->getRangeType() == forest::REAL);
      c = resF->getTerminalNode(arg1F->getReal(a) - arg2F->getReal(b));
    }
    return true;
  }
  return false;
}


// ******************************************************************
// *                                                                *
// *                       minus_evplus class                       *
// *                                                                *
// ******************************************************************

class MEDDLY::minus_evplus : public generic_binary_evplus {
  public:
    minus_evplus(const binary_opname* opcode, expert_forest* arg1,
      expert_forest* arg2, expert_forest* res);

  protected:
    virtual bool checkTerminals(int aev, int a, int bev, int b, 
      int& cev, int& c);
};

MEDDLY::minus_evplus::minus_evplus(const binary_opname* opcode, 
  expert_forest* arg1, expert_forest* arg2, expert_forest* res)
  : generic_binary_evplus(opcode, arg1, arg2, res)
{
}

bool MEDDLY::minus_evplus::checkTerminals(int aev, int a, int bev, int b,
  int& cev, int& c)
{
  if (a == -1 && b == -1) {
    c = -1; cev = aev - bev;
    return true;
  }
  return false;
}



// ******************************************************************
// *                                                                *
// *                      minus_evtimes  class                      *
// *                                                                *
// ******************************************************************

class MEDDLY::minus_evtimes : public generic_binary_evtimes {
  public:
    minus_evtimes(const binary_opname* opcode, expert_forest* arg1,
      expert_forest* arg2, expert_forest* res);

  protected:
    virtual bool checkTerminals(float aev, int a, float bev, int b, 
      float& cev, int& c);
};

MEDDLY::minus_evtimes::minus_evtimes(const binary_opname* opcode, 
  expert_forest* arg1, expert_forest* arg2, expert_forest* res)
  : generic_binary_evtimes(opcode, arg1, arg2, res)
{
}

bool MEDDLY::minus_evtimes::checkTerminals(float aev, int a, 
  float bev, int b, float& cev, int& c)
{
  if (a == -1 && b == -1) {
    c = -1; cev = aev - bev;
    return true;
  }
  return false;
}



// ******************************************************************
// *                                                                *
// *                       minus_opname class                       *
// *                                                                *
// ******************************************************************

class MEDDLY::minus_opname : public binary_opname {
  public:
    minus_opname();
    virtual binary_operation* buildOperation(expert_forest* a1, 
      expert_forest* a2, expert_forest* r) const;
};

MEDDLY::minus_opname::minus_opname()
 : binary_opname("Minus")
{
}

MEDDLY::binary_operation* 
MEDDLY::minus_opname::buildOperation(expert_forest* a1, expert_forest* a2, 
  expert_forest* r) const
{
  if (0==a1 || 0==a2 || 0==r) return 0;

  if (  
    (a1->getDomain() != r->getDomain()) || 
    (a2->getDomain() != r->getDomain()) 
  )
    throw error(error::DOMAIN_MISMATCH);

  if (
    (a1->isForRelations() != r->isForRelations()) ||
    (a2->isForRelations() != r->isForRelations()) ||
    (a1->getRangeType() != r->getRangeType()) ||
    (a2->getRangeType() != r->getRangeType()) ||
    (a1->getEdgeLabeling() != r->getEdgeLabeling()) ||
    (a2->getEdgeLabeling() != r->getEdgeLabeling()) ||
    (r->getRangeType() == forest::BOOLEAN)
  )
    throw error(error::TYPE_MISMATCH);

  if (r->getEdgeLabeling() == forest::MULTI_TERMINAL) {
    if (r->isForRelations())
      return new minus_mxd(this, a1, a2, r);
    else
      return new minus_mdd(this, a1, a2, r);
  }

  if (r->getEdgeLabeling() == forest::EVPLUS)
    return new minus_evplus(this, a1, a2, r);

  if (r->getEdgeLabeling() == forest::EVTIMES)
    return new minus_evtimes(this, a1, a2, r);

  throw error(error::NOT_IMPLEMENTED);
}

// ******************************************************************
// *                                                                *
// *                           Front  end                           *
// *                                                                *
// ******************************************************************

MEDDLY::binary_opname* MEDDLY::initializeMinus(const settings &s)
{
  return new minus_opname;
}

