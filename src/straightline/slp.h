/**
 * @file slp.h
 * @brief Straightline Program (SLP) Abstract Syntax Tree definitions
 * 
 * This file defines the abstract syntax tree (AST) for a simple straightline
 * programming language used in Lab 1. The SLP language supports:
 * - Variable assignments
 * - Arithmetic expressions (addition, subtraction, multiplication, division)
 * - Print statements with multiple expressions
 * - Expression sequences (side effects within expressions)
 * - Compound statements
 * 
 * The AST nodes implement two key operations:
 * - MaxArgs(): Calculates the maximum number of arguments in any print statement
 * - Interp(): Interprets and executes the program, returning updated symbol table
 * 
 * This is a warm-up exercise before implementing the full Tiger compiler.
 */

#ifndef STRAIGHTLINE_SLP_H_
#define STRAIGHTLINE_SLP_H_

#include <algorithm>
#include <cassert>
#include <string>
#include <list>

namespace A {

class Stm;
class Exp;
class ExpList;

/**
 * @brief Binary arithmetic operators
 * 
 * Supported operations: addition, subtraction, multiplication, and division.
 * Used in OpExp nodes to represent binary arithmetic expressions.
 */
enum BinOp { PLUS = 0, MINUS, TIMES, DIV };

/**
 * @brief Forward declarations for interpreter data structures
 */
class Table;
class IntAndTable;

/**
 * @brief Abstract base class for statements
 * 
 * Statements represent executable actions in the SLP language.
 * All statements can:
 * - Calculate the maximum number of arguments in any nested print statement
 * - Be interpreted/executed, updating the symbol table
 */
class Stm {
 public:
  /**
   * @brief Calculate maximum number of arguments in any print statement
   * @return Maximum number of arguments found in nested print statements
   */
  virtual int MaxArgs() const = 0;
  
  /**
   * @brief Interpret and execute this statement
   * @param t Current symbol table (may be nullptr for empty table)
   * @return Updated symbol table after execution
   */
  virtual Table *Interp(Table *) const = 0;
};

/**
 * @brief Compound statement: executes two statements sequentially
 * 
 * Represents statement composition: stm1; stm2
 * Executes stm1 first, then stm2, using the updated symbol table from stm1.
 */
class CompoundStm : public Stm {
 public:
  /**
   * @param stm1 First statement to execute
   * @param stm2 Second statement to execute after stm1
   */
  CompoundStm(Stm *stm1, Stm *stm2) : stm1(stm1), stm2(stm2) {}
  int MaxArgs() const override;
  Table *Interp(Table *) const override;

 private:
  Stm *stm1, *stm2;  ///< The two statements to execute sequentially
};

/**
 * @brief Assignment statement: assigns expression value to variable
 * 
 * Represents variable assignment: id = exp
 * Evaluates the expression and updates the symbol table with the new binding.
 */
class AssignStm : public Stm {
 public:
  /**
   * @param id Variable name to assign to
   * @param exp Expression to evaluate and assign
   */
  AssignStm(std::string id, Exp *exp) : id(std::move(id)), exp(exp) {}
  int MaxArgs() const override;
  Table *Interp(Table *) const override;

 private:
  std::string id;  ///< Variable identifier
  Exp *exp;        ///< Expression to evaluate
};

/**
 * @brief Print statement: prints expression values
 * 
 * Represents print statement: print(exp1, exp2, ..., expN)
 * Evaluates all expressions in order and prints their integer values.
 * The number of arguments is the number of expressions in the list.
 */
class PrintStm : public Stm {
 public:
  /**
   * @param exps List of expressions to evaluate and print
   */
  explicit PrintStm(ExpList *exps) : exps(exps) {}
  int MaxArgs() const override;
  Table *Interp(Table *) const override;

 private:
  ExpList *exps;  ///< List of expressions to print
};

/**
 * @brief Abstract base class for expressions
 * 
 * Expressions represent values that can be evaluated.
 * All expressions can:
 * - Calculate the maximum number of arguments in any nested print statement
 * - Be evaluated, returning both the integer result and updated symbol table
 */
class Exp {
 public:
  /**
   * @brief Calculate maximum number of arguments in any nested print statement
   * @return Maximum number of arguments found in nested print statements
   */
  virtual int MaxArgs() const = 0;
  
  /**
   * @brief Evaluate this expression
   * @param t Current symbol table
   * @return Pair containing the integer result and updated symbol table
   */
  virtual IntAndTable *Interp(Table*) const = 0;
};

/**
 * @brief Identifier expression: looks up variable value
 * 
 * Represents variable reference: id
 * Looks up the variable in the symbol table and returns its value.
 */
class IdExp : public Exp {
public:
  /**
   * @param id Variable name to look up
   */
  explicit IdExp(std::string id) : id(std::move(id)) {}

  int MaxArgs() const override;
  IntAndTable *Interp(Table *) const override;

 private:
  std::string id;  ///< Variable identifier
};

/**
 * @brief Integer literal expression
 * 
 * Represents an integer constant: 42
 * Returns the literal value without modifying the symbol table.
 */
class NumExp : public Exp {
 public:
  /**
   * @param num Integer value
   */
  explicit NumExp(int num) : num(num) {}
  int MaxArgs() const override;
  IntAndTable *Interp(Table *) const override;

 private:
  int num;  ///< Integer literal value
};

/**
 * @brief Binary operator expression
 * 
 * Represents binary arithmetic: left op right
 * Supports addition, subtraction, multiplication, and division.
 * Both operands are evaluated, then the operation is applied.
 */
class OpExp : public Exp {
 public:
  /**
   * @param left Left operand expression
   * @param oper Binary operator (PLUS, MINUS, TIMES, DIV)
   * @param right Right operand expression
   */
  OpExp(Exp *left, BinOp oper, Exp *right)
      : left(left), oper(oper), right(right) {}
  int MaxArgs() const override;
  IntAndTable *Interp(Table *) const override;

 private:
  Exp *left;   ///< Left operand
  BinOp oper;  ///< Binary operator
  Exp *right;  ///< Right operand
};

/**
 * @brief Expression sequence: statement followed by expression
 * 
 * Represents expression with side effects: (stm, exp)
 * Executes the statement first (for side effects), then evaluates the expression.
 * This allows side effects (like assignments or prints) within expressions.
 */
class EseqExp : public Exp {
 public:
  /**
   * @param stm Statement to execute for side effects
   * @param exp Expression to evaluate after statement
   */
  EseqExp(Stm *stm, Exp *exp) : stm(stm), exp(exp) {}
  int MaxArgs() const override;
  IntAndTable *Interp(Table *) const override;

 private:
  Stm *stm;  ///< Statement to execute first
  Exp *exp;  ///< Expression to evaluate after statement
};

/**
 * @brief Abstract base class for expression lists
 * 
 * Expression lists represent sequences of expressions, used in print statements.
 * All expression lists can:
 * - Calculate maximum arguments in nested prints
 * - Count the number of expressions
 * - Evaluate the first expression (for print statements)
 */
class ExpList {
 public:
  /**
   * @brief Calculate maximum number of arguments in nested print statements
   * @return Maximum arguments found
   */
  virtual int MaxArgs() const = 0;
  
  /**
   * @brief Count the number of expressions in this list
   * @return Number of expressions
   */
  virtual int NumExps() const = 0;
  
  /**
   * @brief Evaluate the first expression in the list
   * @param t Current symbol table
   * @return Result of first expression and updated table
   */
  virtual IntAndTable *Interp(Table*) const = 0;
};

/**
 * @brief Non-empty expression list: head and tail
 * 
 * Represents a list with at least one element: exp, tail
 * Used to build lists of expressions for print statements.
 */
class PairExpList : public ExpList {
 public:
  /**
   * @param exp First expression in the list
   * @param tail Rest of the expression list
   */
  PairExpList(Exp *exp, ExpList *tail) : exp(exp), tail(tail) {}
  int MaxArgs() const override;
  int NumExps() const override;
  IntAndTable * Interp(Table *) const override;

// private:
  Exp *exp;        ///< First expression
  ExpList *tail;   ///< Rest of the list
};

/**
 * @brief Singleton expression list: single element
 * 
 * Represents a list with exactly one element: exp
 * Used as the base case for building expression lists.
 */
class LastExpList : public ExpList {
 public:
  /**
   * @param exp The single expression in the list
   */
  LastExpList(Exp *exp) : exp(exp) {}
  int MaxArgs() const override;
  int NumExps() const override;
  IntAndTable * Interp(Table *) const override;

 private:
  Exp *exp;  ///< The single expression
};

/**
 * @brief Functional symbol table (immutable linked list)
 * 
 * Represents a symbol table as an immutable linked list of bindings.
 * Updates create new tables rather than modifying existing ones.
 * This functional style simplifies the interpreter implementation.
 */
class Table {
 public:
  /**
   * @param id Variable name
   * @param value Variable value
   * @param tail Previous table (for chaining bindings)
   */
  Table(std::string id, int value, const Table *tail)
      : id(std::move(id)), value(value), tail(tail) {}
  
  /**
   * @brief Look up a variable in the table
   * @param key Variable name to find
   * @return Variable value
   * @throws Assertion failure if variable not found
   */
  int Lookup(const std::string &key) const;
  
  /**
   * @brief Create new table with updated binding
   * @param key Variable name
   * @param val New value
   * @return New table with the binding (old table unchanged)
   */
  Table *Update(const std::string &key, int val) const;

 private:
  std::string id;      ///< Variable name for this binding
  int value;          ///< Variable value
  const Table *tail;  ///< Previous bindings (linked list)
};

/**
 * @brief Result of expression evaluation
 * 
 * Pairs an integer result with an updated symbol table.
 * Used by expression evaluation to return both value and side effects.
 */
struct IntAndTable {
  int i;        ///< Integer result value
  Table *t;     ///< Updated symbol table after evaluation

  /**
   * @param i Integer result
   * @param t Updated symbol table
   */
  IntAndTable(int i, Table *t) : i(i), t(t) {}
};

}  // namespace A

#endif  // STRAIGHTLINE_SLP_H_
