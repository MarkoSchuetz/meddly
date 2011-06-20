
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
#include "compute_table.h"

// ******************************************************************
// *                         opname methods                         *
// ******************************************************************

int MEDDLY::opname::next_index;
MEDDLY::opname* MEDDLY::opname::list;

MEDDLY::opname::opname(const char* n)
{
  name = n;
  index = next_index;
  next_index++;
  next = list;
  list = this;
}

MEDDLY::opname::~opname()
{
  // library must be closing
  delete next;
}

// ******************************************************************
// *                       operation  methods                       *
// ******************************************************************

MEDDLY::operation::operation(const opname* n)
{
  theOpName = n;
  key_length = 0;
  ans_length = 0;
  if (useMonolithicCT)
    CT = Monolithic_CT;
  else {
    compute_table::settings s;
    CT = createOperationTable(s, this); 
  }
  is_marked_for_deletion = false;
  next = 0;
}

MEDDLY::operation::~operation()
{
  if (CT && CT->isOperationTable()) delete CT;
  delete next;
}

void MEDDLY::operation::removeStalesFromMonolithic()
{
  if (Monolithic_CT) Monolithic_CT->removeStales();
}

void MEDDLY::operation::markForDeletion()
{
  is_marked_for_deletion = true;
  if (CT && CT->isOperationTable()) CT->removeStales();

  // TBD: remove from list
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
MEDDLY::unary_opname::buildOperation(const forest* ar, const forest* rs) const
{
  throw error(error::TYPE_MISMATCH);  
}

MEDDLY::unary_operation* 
MEDDLY::unary_opname::buildOperation(const forest* ar, opnd_type res) const
{
  throw error(error::TYPE_MISMATCH);
}


// ******************************************************************
// *                    unary_operation  methods                    *
// ******************************************************************

MEDDLY::unary_operation::unary_operation(const unary_opname* code, 
  expert_forest* arg, expert_forest* res) : operation(code)
{
  argF = arg;
  resultType = FOREST;
  resF = res;

  argFslot = argF->registerOperation(this);
  resFslot = resF->registerOperation(this);
}

MEDDLY::unary_operation::unary_operation(const unary_opname* code,
  expert_forest* arg, opnd_type res) : operation(code)
{
  argF = arg;
  resultType = res;
  resF = 0;

  argFslot = argF->registerOperation(this);
  resFslot = -1;
}

MEDDLY::unary_operation::~unary_operation()
{
  argF->unregisterOperation(this, argFslot);
  if (resF) resF->unregisterOperation(this, resFslot);
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


