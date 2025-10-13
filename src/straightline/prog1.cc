/**
 * @file prog1.cc
 * @brief Implementation of example straightline programs for Lab 1
 *
 * This file implements factory functions that create example SLP abstract syntax trees.
 * These programs demonstrate increasingly complex SLP features and are used for testing
 * the SLP interpreter.
 */

#include "straightline/prog1.h"

/**
 * @brief Creates the main example program: a = 5 + 3; b = (print(a, a-1), 10*a); print b;
 *
 * This program demonstrates:
 * - Variable assignment with arithmetic expression
 * - Expression sequences (eseq) for side effects
 * - Print statements with multiple expressions
 * - Compound statements for sequencing
 *
 * @return A statement representing the complete program
 */
A::Stm *Prog() {
  // a = 5 + 3;
  A::Stm *ass_stm1 = new A::AssignStm(
      "a", new A::OpExp(new A::NumExp(5), A::PLUS, new A::NumExp(3)));

  // b = (print(a, a-1), 10*a);
  A::Exp *op_exp1 = new A::OpExp(new A::IdExp("a"), A::MINUS, new A::NumExp(1));

  auto exp_list1 = new A::PairExpList(new A::IdExp("a"), new A::LastExpList(op_exp1));
  A::Stm *pr_stm1 = new A::PrintStm(exp_list1);

  A::Exp *op_exp2 = new A::OpExp(new A::NumExp(10), A::TIMES, new A::IdExp("a"));
  A::Stm *ass_stm2 = new A::AssignStm("b", new A::EseqExp(pr_stm1, op_exp2));

  // print b;
  A::Stm *pr_stm2 = new A::PrintStm(new A::LastExpList(new A::IdExp("b")));

  // b = (print(a, a-1), 10*a); print b;
  A::Stm *com_stm = new A::CompoundStm(ass_stm2, pr_stm2);

  return new A::CompoundStm(ass_stm1, com_stm);
}

/**
 * @brief Creates a progressively more complex program
 *
 * This program extends the basic program with additional complexity:
 * - Uses the result of the first program as a starting point
 * - Demonstrates more complex expression sequences
 * - Shows reuse of existing program components
 *
 * @return A statement representing the progressive program
 */
A::Stm *ProgProg() {
  // a = 5 + 3; b = (print(a, a-1), 10*a); print b;
  A::Stm *stm1 = Prog();

  // a = 5 + b;
  A::Stm *ass_stm1 = new A::AssignStm(
      "a", new A::OpExp(new A::NumExp(5), A::PLUS, new A::IdExp("b")));

  // print(a, a, a-1)
  A::Exp *exp1 = new A::OpExp(new A::IdExp("a"), A::MINUS, new A::NumExp(1));

  auto exp_list2 = new A::PairExpList(new A::IdExp("a"), 
                     new A::PairExpList(new A::IdExp("a"),
                       new A::LastExpList(exp1)));

  // 10 * a
  A::Exp *exp2 = new A::OpExp(new A::NumExp(10), A::TIMES, new A::IdExp("a"));

  // b = (print(a, a, a-1), 10*a);
  A::Stm *ass_stm2 =
      new A::AssignStm("b", new A::EseqExp(new A::PrintStm(exp_list2), exp2));

  // print b;
  A::Stm *pr_stm2 = new A::PrintStm(new A::LastExpList(new A::IdExp("b")));

  return new A::CompoundStm(
      stm1, new A::CompoundStm(ass_stm1, new A::CompoundStm(ass_stm2, pr_stm2)));
}

/**
 * @brief Creates the most complex example program
 *
 * This program demonstrates the most advanced SLP features:
 * - Builds upon the progressive program
 * - Uses nested expression sequences
 * - Shows assignment within expression sequences
 * - Demonstrates complex variable dependencies
 *
 * @return A statement representing the most complex program
 */
A::Stm *RightProg() {
  A::Stm *stm1 = ProgProg();
  return new A::CompoundStm(
      stm1,
      new A::AssignStm(
          "a", new A::EseqExp(new A::AssignStm(
                                  "a", new A::OpExp(new A::IdExp("a"), A::PLUS,
                                                    new A::IdExp("b"))),
                              new A::IdExp("a"))));
}
