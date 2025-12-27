/**
 * @file slp.cc
 * @brief Implementation of Straightline Program (SLP) interpreter
 * 
 * This file implements the MaxArgs() and Interp() methods for all AST nodes.
 * MaxArgs() recursively traverses the AST to find the maximum number of
 * arguments in any print statement. Interp() interprets and executes the
 * program, updating the symbol table as it goes.
 */

#include "straightline/slp.h"

#include <iostream>

namespace A {
int A::CompoundStm::MaxArgs() const {
    // Maximum is the max of both statements
    int maxArgs1 = stm1->MaxArgs();
    int maxArgs2 = stm2->MaxArgs();
    return maxArgs1 > maxArgs2 ? maxArgs1: maxArgs2;
}

Table *A::CompoundStm::Interp(Table *t) const {
  // Execute stm1 first, then stm2 with updated table
  return stm2->Interp(stm1->Interp(t));
}

int A::AssignStm::MaxArgs() const {
  // Maximum is determined by the expression
  return exp->MaxArgs();
}

Table *A::AssignStm::Interp(Table *t) const {
  // Evaluate expression and update table with new binding
  return t->Update(id, exp->Interp(t)->i);
}

int A::PrintStm::MaxArgs() const {
  // Maximum is either the number of expressions here, or max from nested prints
  int numExps = exps->NumExps();
  int maxArgs = exps->MaxArgs();
  return numExps > maxArgs ? numExps : maxArgs;
}

Table *A::PrintStm::Interp(Table *t) const {
  // Print all expressions in sequence
  if (exps->NumExps() != 1) {
      // Multiple expressions: print first with space, recurse on tail
      PairExpList* pairExpList = (PairExpList*)exps;
      std::cout << pairExpList->exp->Interp(t)->i << ' ';
      PrintStm* newPrintStm = new PrintStm(pairExpList->tail);
      return newPrintStm->Interp(t);
  } else {
      // Single expression: print with newline
      std::cout << exps->Interp(t)->i << std::endl;
      return t;
  }
}

int A::IdExp::MaxArgs() const {
    // Identifiers don't contain print statements
    return 0;
}

IntAndTable* A::IdExp::Interp(Table* t) const {
    // Look up variable value, table unchanged
    return new IntAndTable(t->Lookup(id), t);
}

int A::NumExp::MaxArgs() const {
    // Integer literals don't contain print statements
    return 0;
}

IntAndTable* A::NumExp::Interp(Table* t) const {
    // Return literal value, table unchanged
    return new IntAndTable(num, t);
}

int A::OpExp::MaxArgs() const {
    // Maximum is the max of both operands
    int leftArgs = left->MaxArgs();
    int rightArgs = right->MaxArgs();
    return leftArgs > rightArgs ? leftArgs : rightArgs;
}

IntAndTable* A::OpExp::Interp(Table* t) const {
    // Evaluate both operands and apply operator
    // Note: This assumes expressions don't modify the table (true for pure expressions)
    int leftValue = left->Interp(t)->i;
    int rightValue = right->Interp(t)->i;
    switch(oper) {
        case PLUS: return new IntAndTable(leftValue + rightValue, t);
        case MINUS: return new IntAndTable(leftValue - rightValue, t);
        case TIMES: return new IntAndTable(leftValue * rightValue, t);
        case DIV: return new IntAndTable(leftValue / rightValue, t);
    }
}

int A::EseqExp::MaxArgs() const {
    // Maximum is the max of statement and expression
    int stmArgs = stm->MaxArgs();
    int expArgs = exp->MaxArgs();
    return stmArgs > expArgs ? stmArgs : expArgs;
}

IntAndTable* A::EseqExp::Interp(Table* t) const {
    // Execute statement first, then evaluate expression with updated table
    Table* newTable = stm->Interp(t);
    return new IntAndTable(exp->Interp(newTable)->i, newTable);
}


int A::PairExpList::MaxArgs() const {
    // Maximum is the max of head and tail
    int expArgs = exp->MaxArgs();
    int tailArgs = tail->MaxArgs();
    return expArgs > tailArgs ? expArgs : tailArgs;
}

int A::PairExpList::NumExps() const {
    // Count is 1 (head) plus count of tail
    return tail->NumExps() + 1;
}

IntAndTable* A::PairExpList::Interp(Table* t) const {
    // Return value of first expression (used by PrintStm)
    return new IntAndTable(exp->Interp(t)->i, t);
}

int A::LastExpList::MaxArgs() const {
    // Maximum is determined by the single expression
    return exp->MaxArgs();
}

int A::LastExpList::NumExps() const {
    // Singleton list has exactly one expression
    return 1;
}

IntAndTable* A::LastExpList::Interp(Table* t) const {
    // Return value of the single expression
    return new IntAndTable(exp->Interp(t)->i, t);
}


int Table::Lookup(const std::string &key) const {
  // Search linked list for matching variable name
  if (id == key) {
    return value;
  } else if (tail != nullptr) {
    return tail->Lookup(key);
  } else {
    // Variable not found - should not happen in valid programs
    assert(false);
  }
}

Table *Table::Update(const std::string &key, int val) const {
  // Create new table node with updated binding (functional update)
  // New binding shadows any previous binding with same name
  return new Table(key, val, this);
}
}  // namespace A
