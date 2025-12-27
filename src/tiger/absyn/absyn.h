/**
 * @file absyn.h
 * @brief Abstract Syntax Tree (AST) definitions for Tiger language
 * 
 * This file defines the complete abstract syntax tree for the Tiger programming
 * language. The AST is constructed by the parser and serves as the intermediate
 * representation for all subsequent compiler phases:
 * 
 * - Semantic Analysis: Type checking and symbol resolution
 * - Escape Analysis: Determining variable storage requirements
 * - IR Translation: Converting AST to intermediate representation trees
 * 
 * The AST follows the visitor pattern with virtual methods for:
 * - Print(): Pretty-printing the AST structure
 * - SemAnalyze(): Type checking and semantic analysis
 * - Translate(): Translation to IR trees
 * - Traverse(): Escape analysis traversal
 * 
 * All AST nodes store position information (pos_) for error reporting.
 */

#ifndef TIGER_ABSYN_ABSYN_H_
#define TIGER_ABSYN_ABSYN_H_

#include <cstdio>
#include <list>
#include <string>

#include "tiger/env/env.h"
#include "tiger/errormsg/errormsg.h"
#include "tiger/escape/escape.h"
//#include "tiger/frame/frame.h"
#include "tiger/semant/types.h"
#include "tiger/symbol/symbol.h"

/**
 * @brief Forward declarations for translation module
 */
namespace tr {
class Exp;
class ExpAndTy;
} // namespace tr

namespace absyn {

class Var;
class Exp;
class Dec;
class Ty;

class ExpList;
class FieldList;
class FunDecList;
class NameAndTyList;
class DecList;
class EFieldList;

/**
 * @brief Binary and comparison operators
 * 
 * Arithmetic operators: PLUS_OP, MINUS_OP, TIMES_OP, DIVIDE_OP
 * Comparison operators: EQ_OP, NEQ_OP, LT_OP, LE_OP, GT_OP, GE_OP
 */
enum Oper {
  PLUS_OP,
  MINUS_OP,
  TIMES_OP,
  DIVIDE_OP,
  EQ_OP,
  NEQ_OP,
  LT_OP,
  LE_OP,
  GT_OP,
  GE_OP,
  ABSYN_OPER_COUNT,
};

/**
 * @brief Root of the abstract syntax tree
 * 
 * Wraps the top-level expression of a Tiger program.
 * Provides high-level operations for semantic analysis, translation,
 * and escape analysis.
 */
class AbsynTree {
public:
  AbsynTree() = delete;
  AbsynTree(nullptr_t) = delete;
  explicit AbsynTree(absyn::Exp *root);
  AbsynTree(const AbsynTree &absyn_tree) = delete;
  AbsynTree(AbsynTree &&absyn_tree) = delete;
  AbsynTree &operator=(const AbsynTree &absyn_tree) = delete;
  AbsynTree &operator=(AbsynTree &&absyn_tree) = delete;
  ~AbsynTree();

  void Print(FILE *out) const;
  void SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                  err::ErrorMsg *errormsg) const;
  tr::ExpAndTy *Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                          tr::Level *level, temp::Label *label,
                          err::ErrorMsg *errormsg) const;
  void Traverse(esc::EscEnvPtr env);

private:
  absyn::Exp *root_;
};

/**
 * @brief Variable access expressions
 * 
 * Variables in Tiger can be:
 * - Simple variables: identifier
 * - Field access: record.field
 * - Array subscript: array[index]
 */

/**
 * @brief Abstract base class for variable access
 * 
 * Variables represent l-values (locations that can be assigned to).
 * All variable nodes support semantic analysis, translation, and escape analysis.
 */
class Var {
public:
  int pos_;
  virtual ~Var() = default;
  virtual void Print(FILE *out, int d) const = 0;
  virtual type::Ty *SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount,
                               err::ErrorMsg *errormsg) const = 0;
  virtual tr::ExpAndTy *Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const = 0;
  virtual void Traverse(esc::EscEnvPtr env, int depth) = 0;

protected:
  explicit Var(int pos) : pos_(pos) {}
};

/**
 * @brief Simple variable: identifier reference
 * 
 * Represents a direct variable reference: x
 * Looks up the variable in the current scope.
 */
class SimpleVar : public Var {
public:
  sym::Symbol *sym_;  ///< Variable name symbol
  SimpleVar(int pos, sym::Symbol *sym) : Var(pos), sym_(sym) {}
  ~SimpleVar() override;

  void Print(FILE *out, int d) const override;
  type::Ty *SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                       err::ErrorMsg *errormsg) const override;
  tr::ExpAndTy *Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                          tr::Level *level, temp::Label *label,
                          err::ErrorMsg *errormsg) const override;
  void Traverse(esc::EscEnvPtr env, int depth) override;
};

/**
 * @brief Record field access: record.field
 * 
 * Represents accessing a field of a record: var.field
 * First evaluates var to get the record, then accesses the named field.
 */
class FieldVar : public Var {
public:
  Var *var_;        ///< Record variable
  sym::Symbol *sym_; ///< Field name

  FieldVar(int pos, Var *var, sym::Symbol *sym)
      : Var(pos), var_(var), sym_(sym) {}
  ~FieldVar() override;

  void Print(FILE *out, int d) const override;
  type::Ty *SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                       err::ErrorMsg *errormsg) const override;
  tr::ExpAndTy *Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                          tr::Level *level, temp::Label *label,
                          err::ErrorMsg *errormsg) const override;
  void Traverse(esc::EscEnvPtr env, int depth) override;
};

/**
 * @brief Array subscript access: array[index]
 * 
 * Represents array element access: var[exp]
 * First evaluates var to get the array, then evaluates exp to get the index.
 */
class SubscriptVar : public Var {
public:
  Var *var_;        ///< Array variable
  Exp *subscript_;  ///< Index expression

  SubscriptVar(int pos, Var *var, Exp *exp)
      : Var(pos), var_(var), subscript_(exp) {}
  ~SubscriptVar() override;

  void Print(FILE *out, int d) const override;
  type::Ty *SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                       err::ErrorMsg *errormsg) const override;
  tr::ExpAndTy *Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                          tr::Level *level, temp::Label *label,
                          err::ErrorMsg *errormsg) const override;
  void Traverse(esc::EscEnvPtr env, int depth) override;
};

/**
 * @brief Expression nodes
 * 
 * Expressions represent r-values (values that can be computed).
 * All expressions support semantic analysis (type checking), translation
 * to IR, and escape analysis.
 */

/**
 * @brief Abstract base class for expressions
 * 
 * Expressions compute values and may have side effects.
 * All expression nodes implement:
 * - SemAnalyze(): Type checking and symbol resolution
 * - Translate(): Conversion to IR tree representation
 * - Traverse(): Escape analysis for nested functions
 */
class Exp {
public:
  int pos_;
  virtual ~Exp() = default;
  virtual void Print(FILE *out, int d) const = 0;
  virtual type::Ty *SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount,
                               err::ErrorMsg *errormsg) const = 0;
  virtual tr::ExpAndTy *Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const = 0;
  virtual void Traverse(esc::EscEnvPtr env, int depth) = 0;

protected:
  explicit Exp(int pos) : pos_(pos) {}
};

/**
 * @brief Variable expression: evaluates a variable
 * 
 * Represents reading a variable's value: var
 * Evaluates the variable access to get its current value.
 */
class VarExp : public Exp {
public:
  Var *var_;  ///< Variable to evaluate

  VarExp(int pos, Var *var) : Exp(pos), var_(var) {}
  ~VarExp() override;

  void Print(FILE *out, int d) const override;
  type::Ty *SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                       err::ErrorMsg *errormsg) const override;
  tr::ExpAndTy *Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                          tr::Level *level, temp::Label *label,
                          err::ErrorMsg *errormsg) const override;
  void Traverse(esc::EscEnvPtr env, int depth) override;
};

/**
 * @brief Nil literal: null value for records
 * 
 * Represents the nil value: nil
 * Used for initializing or comparing record types.
 * Type must be a record type (or compatible).
 */
class NilExp : public Exp {
public:
  explicit NilExp(int pos) : Exp(pos) {}
  ~NilExp() override;

  void Print(FILE *out, int d) const override;
  type::Ty *SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                       err::ErrorMsg *errormsg) const override;
  tr::ExpAndTy *Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                          tr::Level *level, temp::Label *label,
                          err::ErrorMsg *errormsg) const override;
  void Traverse(esc::EscEnvPtr env, int depth) override;
};

/**
 * @brief Integer literal expression
 * 
 * Represents an integer constant: 42
 * Type is always int.
 */
class IntExp : public Exp {
public:
  int val_;  ///< Integer value

  IntExp(int pos, int val) : Exp(pos), val_(val) {}
  ~IntExp() override;

  void Print(FILE *out, int d) const override;
  type::Ty *SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                       err::ErrorMsg *errormsg) const override;
  tr::ExpAndTy *Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                          tr::Level *level, temp::Label *label,
                          err::ErrorMsg *errormsg) const override;
  void Traverse(esc::EscEnvPtr env, int depth) override;
};

/**
 * @brief String literal expression
 * 
 * Represents a string constant: "hello"
 * Type is always string.
 */
class StringExp : public Exp {
public:
  std::string str_;  ///< String value

  StringExp(int pos, std::string *str) : Exp(pos), str_(*str) {}
  ~StringExp() override;

  void Print(FILE *out, int d) const override;
  type::Ty *SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                       err::ErrorMsg *errormsg) const override;
  tr::ExpAndTy *Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                          tr::Level *level, temp::Label *label,
                          err::ErrorMsg *errormsg) const override;
  void Traverse(esc::EscEnvPtr env, int depth) override;
};

/**
 * @brief Function call expression
 * 
 * Represents a function call: func(arg1, arg2, ...)
 * Looks up the function, checks argument count and types,
 * and evaluates all arguments before calling.
 */
class CallExp : public Exp {
public:
  sym::Symbol *func_;  ///< Function name
  ExpList *args_;      ///< Argument expressions

  CallExp(int pos, sym::Symbol *func, ExpList *args)
      : Exp(pos), func_(func), args_(args) {
    assert(args);
  }
  ~CallExp() override;

  void Print(FILE *out, int d) const override;
  type::Ty *SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                       err::ErrorMsg *errormsg) const override;
  tr::ExpAndTy *Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                          tr::Level *level, temp::Label *label,
                          err::ErrorMsg *errormsg) const override;
  void Traverse(esc::EscEnvPtr env, int depth) override;
};

/**
 * @brief Binary operator expression
 * 
 * Represents binary operations: left op right
 * Supports arithmetic (+, -, *, /) and comparison (==, !=, <, <=, >, >=).
 * Arithmetic operators return int; comparisons return int (0 or 1).
 */
class OpExp : public Exp {
public:
  Oper oper_;      ///< Binary operator
  Exp *left_, *right_;  ///< Left and right operands

  OpExp(int pos, Oper oper, Exp *left, Exp *right)
      : Exp(pos), oper_(oper), left_(left), right_(right) {}
  ~OpExp() override;

  void Print(FILE *out, int d) const override;
  type::Ty *SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                       err::ErrorMsg *errormsg) const override;
  tr::ExpAndTy *Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                          tr::Level *level, temp::Label *label,
                          err::ErrorMsg *errormsg) const override;
  void Traverse(esc::EscEnvPtr env, int depth) override;
};

/**
 * @brief Record creation expression
 * 
 * Represents record construction: TypeName{field1=val1, field2=val2, ...}
 * Creates a new record instance with the specified field values.
 * All fields must be provided and match the record type definition.
 */
class RecordExp : public Exp {
public:
  sym::Symbol *typ_;     ///< Record type name
  EFieldList *fields_;   ///< Field initializers

  RecordExp(int pos, sym::Symbol *typ, EFieldList *fields)
      : Exp(pos), typ_(typ), fields_(fields) {}
  ~RecordExp() override;

  void Print(FILE *out, int d) const override;
  type::Ty *SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                       err::ErrorMsg *errormsg) const override;
  tr::ExpAndTy *Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                          tr::Level *level, temp::Label *label,
                          err::ErrorMsg *errormsg) const override;
  void Traverse(esc::EscEnvPtr env, int depth) override;
};

/**
 * @brief Sequence expression: evaluates expressions in order
 * 
 * Represents expression sequence: (exp1; exp2; ...; expN)
 * Evaluates all expressions in order, returning the value of the last one.
 * Used for sequencing side effects.
 */
class SeqExp : public Exp {
public:
  ExpList *seq_;  ///< List of expressions to evaluate

  SeqExp(int pos, ExpList *seq) : Exp(pos), seq_(seq) {}
  ~SeqExp() override;

  void Print(FILE *out, int d) const override;
  type::Ty *SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                       err::ErrorMsg *errormsg) const override;
  tr::ExpAndTy *Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                          tr::Level *level, temp::Label *label,
                          err::ErrorMsg *errormsg) const override;
  void Traverse(esc::EscEnvPtr env, int depth) override;
};

/**
 * @brief Assignment expression: assigns value to variable
 * 
 * Represents assignment: var := exp
 * Evaluates exp and assigns the result to var.
 * Returns the assigned value (for use in expressions).
 */
class AssignExp : public Exp {
public:
  Var *var_;  ///< Variable to assign to
  Exp *exp_;  ///< Expression to evaluate

  AssignExp(int pos, Var *var, Exp *exp) : Exp(pos), var_(var), exp_(exp) {}
  ~AssignExp() override;

  void Print(FILE *out, int d) const override;
  type::Ty *SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                       err::ErrorMsg *errormsg) const override;
  tr::ExpAndTy *Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                          tr::Level *level, temp::Label *label,
                          err::ErrorMsg *errormsg) const override;
  void Traverse(esc::EscEnvPtr env, int depth) override;
};

/**
 * @brief Conditional expression: if-then-else
 * 
 * Represents conditional: if test then then_exp else else_exp
 * Evaluates test; if true, evaluates then_exp, otherwise else_exp.
 * The else clause is optional (elsee_ may be nullptr).
 */
class IfExp : public Exp {
public:
  Exp *test_;   ///< Condition expression
  Exp *then_;   ///< Expression when condition is true
  Exp *elsee_;  ///< Expression when condition is false (may be nullptr)

  IfExp(int pos, Exp *test, Exp *then, Exp *elsee)
      : Exp(pos), test_(test), then_(then), elsee_(elsee) {}
  ~IfExp() override;

  void Print(FILE *out, int d) const override;
  type::Ty *SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                       err::ErrorMsg *errormsg) const override;
  tr::ExpAndTy *Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                          tr::Level *level, temp::Label *label,
                          err::ErrorMsg *errormsg) const override;
  void Traverse(esc::EscEnvPtr env, int depth) override;
};

/**
 * @brief While loop expression
 * 
 * Represents while loop: while test do body
 * Repeatedly evaluates test; if true, evaluates body and loops.
 * Returns nil (void value).
 */
class WhileExp : public Exp {
public:
  Exp *test_;  ///< Loop condition
  Exp *body_;  ///< Loop body

  WhileExp(int pos, Exp *test, Exp *body)
      : Exp(pos), test_(test), body_(body) {}
  ~WhileExp() override;

  void Print(FILE *out, int d) const override;
  type::Ty *SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                       err::ErrorMsg *errormsg) const override;
  tr::ExpAndTy *Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                          tr::Level *level, temp::Label *label,
                          err::ErrorMsg *errormsg) const override;
  void Traverse(esc::EscEnvPtr env, int depth) override;
};

/**
 * @brief For loop expression
 * 
 * Represents for loop: for var := lo to hi do body
 * Iterates var from lo to hi (inclusive), executing body each iteration.
 * The loop variable var is read-only within the body.
 * Returns nil (void value).
 */
class ForExp : public Exp {
public:
  sym::Symbol *var_;  ///< Loop variable name
  Exp *lo_, *hi_;     ///< Lower and upper bounds (inclusive)
  Exp *body_;         ///< Loop body
  bool escape_;       ///< Escape flag (set by escape analysis)

  ForExp(int pos, sym::Symbol *var, Exp *lo, Exp *hi, Exp *body)
      : Exp(pos), var_(var), lo_(lo), hi_(hi), body_(body), escape_(true) {}
  ~ForExp() override;

  void Print(FILE *out, int d) const override;
  type::Ty *SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                       err::ErrorMsg *errormsg) const override;
  tr::ExpAndTy *Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                          tr::Level *level, temp::Label *label,
                          err::ErrorMsg *errormsg) const override;
  void Traverse(esc::EscEnvPtr env, int depth) override;
};

/**
 * @brief Break expression: exits enclosing loop
 * 
 * Represents break statement: break
 * Exits the innermost enclosing while or for loop.
 * Must be inside a loop context.
 */
class BreakExp : public Exp {
public:
  explicit BreakExp(int pos) : Exp(pos) {}
  ~BreakExp() override;

  void Print(FILE *out, int d) const override;
  type::Ty *SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                       err::ErrorMsg *errormsg) const override;
  tr::ExpAndTy *Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                          tr::Level *level, temp::Label *label,
                          err::ErrorMsg *errormsg) const override;
  void Traverse(esc::EscEnvPtr env, int depth) override;
};

/**
 * @brief Let expression: introduces local declarations
 * 
 * Represents let expression: let decs in body end
 * Processes declarations (types, variables, functions) in order,
 * then evaluates body in the extended scope.
 * Returns the value of body.
 */
class LetExp : public Exp {
public:
  DecList *decs_;  ///< List of declarations
  Exp *body_;      ///< Body expression

  LetExp(int pos, DecList *decs, Exp *body)
      : Exp(pos), decs_(decs), body_(body) {}
  ~LetExp() override;

  void Print(FILE *out, int d) const override;
  type::Ty *SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                       err::ErrorMsg *errormsg) const override;
  tr::ExpAndTy *Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                          tr::Level *level, temp::Label *label,
                          err::ErrorMsg *errormsg) const override;
  void Traverse(esc::EscEnvPtr env, int depth) override;
};

/**
 * @brief Array creation expression
 * 
 * Represents array creation: TypeName[size] of init
 * Creates a new array of the specified type and size,
 * initializing all elements with the init expression.
 */
class ArrayExp : public Exp {
public:
  sym::Symbol *typ_;  ///< Array type name
  Exp *size_;         ///< Array size expression
  Exp *init_;         ///< Initial value expression

  ArrayExp(int pos, sym::Symbol *typ, Exp *size, Exp *init)
      : Exp(pos), typ_(typ), size_(size), init_(init) {}
  ~ArrayExp() override;

  void Print(FILE *out, int d) const override;
  type::Ty *SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                       err::ErrorMsg *errormsg) const override;
  tr::ExpAndTy *Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                          tr::Level *level, temp::Label *label,
                          err::ErrorMsg *errormsg) const override;
  void Traverse(esc::EscEnvPtr env, int depth) override;
};

/**
 * @brief Void expression: no value
 * 
 * Represents an expression that produces no value.
 * Used in contexts where a value is required but none is meaningful.
 */
class VoidExp : public Exp {
public:
  explicit VoidExp(int pos) : Exp(pos) {}
  ~VoidExp() override;

  void Print(FILE *out, int d) const override;
  type::Ty *SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                       err::ErrorMsg *errormsg) const override;
  tr::ExpAndTy *Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                          tr::Level *level, temp::Label *label,
                          err::ErrorMsg *errormsg) const override;
  void Traverse(esc::EscEnvPtr env, int depth) override;
};

/**
 * @brief Declaration nodes
 * 
 * Declarations introduce new names into the current scope:
 * - Type declarations: type name = ...
 * - Variable declarations: var name := exp
 * - Function declarations: function name(params) = exp
 */

/**
 * @brief Abstract base class for declarations
 * 
 * All declarations support semantic analysis and translation.
 * Declarations update the symbol tables (venv and tenv).
 */
class Dec {
public:
  int pos_;
  virtual ~Dec() = default;
  virtual void Print(FILE *out, int d) const = 0;
  virtual void SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                          err::ErrorMsg *errormsg) const = 0;
  virtual tr::Exp *Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                             tr::Level *level, temp::Label *label,
                             err::ErrorMsg *errormsg) const = 0;
  virtual void Traverse(esc::EscEnvPtr env, int depth) = 0;

protected:
  explicit Dec(int pos) : pos_(pos) {}
};

/**
 * @brief Function declaration: defines one or more functions
 * 
 * Represents function declaration(s): function name(params) = body
 * Supports mutually recursive functions (multiple functions in one declaration).
 * Functions are first-class values and can be nested.
 */
class FunctionDec : public Dec {
public:
  FunDecList *functions_;  ///< List of function definitions

  FunctionDec(int pos, FunDecList *functions)
      : Dec(pos), functions_(functions) {}
  ~FunctionDec() override;

  void Print(FILE *out, int d) const override;
  void SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                  err::ErrorMsg *errormsg) const override;
  tr::Exp *Translate(env::VEnvPtr venv, env::TEnvPtr tenv, tr::Level *level,
                     temp::Label *label, 
                     err::ErrorMsg *errormsg) const override;
  void Traverse(esc::EscEnvPtr env, int depth) override;
};

/**
 * @brief Variable declaration: introduces a new variable
 * 
 * Represents variable declaration: var name : type := exp
 * The type annotation is optional. If omitted, type is inferred from init.
 * The escape_ flag indicates if the variable escapes its scope (set by escape analysis).
 */
class VarDec : public Dec {
public:
  sym::Symbol *var_;  ///< Variable name
  sym::Symbol *typ_;  ///< Type name (may be nullptr if type omitted)
  Exp *init_;         ///< Initial value expression
  bool escape_;       ///< Escape flag (set by escape analysis)

  VarDec(int pos, sym::Symbol *var, sym::Symbol *typ, Exp *init)
      : Dec(pos), var_(var), typ_(typ), init_(init), escape_(true) {}
  ~VarDec() override;

  void Print(FILE *out, int d) const override;
  void SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                  err::ErrorMsg *errormsg) const override;
  tr::Exp *Translate(env::VEnvPtr venv, env::TEnvPtr tenv, tr::Level *level,
                     temp::Label *label, 
                     err::ErrorMsg *errormsg) const override;
  void Traverse(esc::EscEnvPtr env, int depth) override;
};

/**
 * @brief Type declaration: defines new types
 * 
 * Represents type declaration: type name = type_def
 * Supports mutually recursive types (multiple types in one declaration).
 * Types can be records, arrays, or type aliases.
 */
class TypeDec : public Dec {
public:
  NameAndTyList *types_;  ///< List of type definitions

  TypeDec(int pos, NameAndTyList *types) : Dec(pos), types_(types) {}
  ~TypeDec() override;

  void Print(FILE *out, int d) const override;
  void SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                  err::ErrorMsg *errormsg) const override;
  tr::Exp *Translate(env::VEnvPtr venv, env::TEnvPtr tenv, tr::Level *level,
                     temp::Label *label, 
                     err::ErrorMsg *errormsg) const override;
  void Traverse(esc::EscEnvPtr env, int depth) override;
};

/**
 * @brief Type definition nodes
 * 
 * Type definitions specify the structure of types:
 * - Name types: type alias
 * - Record types: struct with named fields
 * - Array types: homogeneous array of elements
 */

/**
 * @brief Abstract base class for type definitions
 * 
 * Type definitions are analyzed during semantic analysis to create
 * type objects in the type environment.
 */
class Ty {
public:
  int pos_;
  virtual ~Ty() = default;
  virtual void Print(FILE *out, int d) const = 0;
  virtual type::Ty *SemAnalyze(env::TEnvPtr tenv,
                               err::ErrorMsg *errormsg) const = 0;
  virtual type::Ty *Translate(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const = 0;

protected:
  explicit Ty(int pos) : pos_(pos) {}
};

/**
 * @brief Named type: type alias
 * 
 * Represents type alias: type name = existing_type
 * Creates a new name for an existing type.
 */
class NameTy : public Ty {
public:
  sym::Symbol *name_;  ///< Type name to alias

  NameTy(int pos, sym::Symbol *name) : Ty(pos), name_(name) {}
  ~NameTy() override;

  void Print(FILE *out, int d) const override;
  type::Ty *SemAnalyze(env::TEnvPtr tenv,
                       err::ErrorMsg *errormsg) const override;
  type::Ty *Translate(env::TEnvPtr tenv,
                      err::ErrorMsg *errormsg) const override;
};

/**
 * @brief Record type: struct with named fields
 * 
 * Represents record type: type name = {field1: type1, field2: type2, ...}
 * Defines a record type with named, typed fields.
 */
class RecordTy : public Ty {
public:
  FieldList *record_;  ///< List of field definitions

  RecordTy(int pos, FieldList *record) : Ty(pos), record_(record) {}
  ~RecordTy() override;

  void Print(FILE *out, int d) const override;
  type::Ty *SemAnalyze(env::TEnvPtr tenv,
                       err::ErrorMsg *errormsg) const override;
  type::Ty *Translate(env::TEnvPtr tenv,
                      err::ErrorMsg *errormsg) const override;
};

/**
 * @brief Array type: homogeneous array
 * 
 * Represents array type: type name = array of element_type
 * Defines an array type with elements of the specified type.
 */
class ArrayTy : public Ty {
public:
  sym::Symbol *array_;  ///< Element type name

  ArrayTy(int pos, sym::Symbol *array) : Ty(pos), array_(array) {}
  ~ArrayTy() override;

  void Print(FILE *out, int d) const override;
  type::Ty *SemAnalyze(env::TEnvPtr tenv,
                       err::ErrorMsg *errormsg) const override;
  type::Ty *Translate(env::TEnvPtr tenv,
                      err::ErrorMsg *errormsg) const override;
};

/**
 * @brief List structures for AST nodes
 * 
 * The AST uses linked lists (via std::list) to represent sequences:
 * - FieldList: record field definitions
 * - ExpList: expression sequences
 * - FunDecList: function declarations
 * - DecList: declaration sequences
 * - NameAndTyList: type declarations
 * - EFieldList: record field initializers
 */

/**
 * @brief Record field definition
 * 
 * Represents a single field in a record type: name: type
 */
class Field {
public:
  int pos_;              ///< Source position
  sym::Symbol *name_, *typ_;  ///< Field name and type
  bool escape_;          ///< Escape flag (set by escape analysis)

  Field(int pos, sym::Symbol *name, sym::Symbol *typ)
      : pos_(pos), name_(name), typ_(typ), escape_(true) {}

  void Print(FILE *out, int d) const;
};

class FieldList {
public:
  FieldList() = default;
  explicit FieldList(Field *field) : field_list_({field}) { assert(field); }

  FieldList *Prepend(Field *field) {
    field_list_.push_front(field);
    return this;
  }
  [[nodiscard]] const std::list<Field *> &GetList() const {
    return field_list_;
  }
  void Print(FILE *out, int d) const;
  type::TyList *MakeFormalTyList(env::TEnvPtr tenv,
                                 err::ErrorMsg *errormsg) const;
  type::FieldList *MakeFieldList(env::TEnvPtr tenv,
                                 err::ErrorMsg *errormsg) const;

private:
  std::list<Field *> field_list_;
};

class ExpList {
public:
  ExpList() = default;
  explicit ExpList(Exp *exp) : exp_list_({exp}) { assert(exp); }

  ExpList *Prepend(Exp *exp) {
    exp_list_.push_front(exp);
    return this;
  }
  [[nodiscard]] const std::list<Exp *> &GetList() const { return exp_list_; }
  void Print(FILE *out, int d) const;

private:
  std::list<Exp *> exp_list_;
};

class FunDec {
public:
  int pos_;
  sym::Symbol *name_;
  FieldList *params_;
  sym::Symbol *result_;
  Exp *body_;

  FunDec(int pos, sym::Symbol *name, FieldList *params, sym::Symbol *result,
         Exp *body)
      : pos_(pos), name_(name), params_(params), result_(result), body_(body) {
    assert(params);
  }

  void Print(FILE *out, int d) const;
};

class FunDecList {
public:
  explicit FunDecList(FunDec *fun_dec) : fun_dec_list_({fun_dec}) {
    assert(fun_dec);
  }

  FunDecList *Prepend(FunDec *fun_dec) {
    fun_dec_list_.push_front(fun_dec);
    return this;
  }
  [[nodiscard]] const std::list<FunDec *> &GetList() const {
    return fun_dec_list_;
  }
  void Print(FILE *out, int d) const;

private:
  std::list<FunDec *> fun_dec_list_;
};

class DecList {
public:
  DecList() = default;
  explicit DecList(Dec *dec) : dec_list_({dec}) { assert(dec); }

  DecList *Prepend(Dec *dec) {
    dec_list_.push_front(dec);
    return this;
  }
  [[nodiscard]] const std::list<Dec *> &GetList() const { return dec_list_; }
  void Print(FILE *out, int d) const;

private:
  std::list<Dec *> dec_list_;
};

class NameAndTy {
public:
  sym::Symbol *name_;
  Ty *ty_;

  NameAndTy(sym::Symbol *name, Ty *ty) : name_(name), ty_(ty) {}

  void Print(FILE *out, int d) const;
};

class NameAndTyList {
public:
  explicit NameAndTyList(NameAndTy *name_and_ty)
      : name_and_ty_list_({name_and_ty}) {}

  NameAndTyList *Prepend(NameAndTy *name_and_ty) {
    name_and_ty_list_.push_front(name_and_ty);
    return this;
  }
  [[nodiscard]] const std::list<NameAndTy *> &GetList() const {
    return name_and_ty_list_;
  }
  void Print(FILE *out, int d) const;

private:
  std::list<NameAndTy *> name_and_ty_list_;
};

class EField {
public:
  sym::Symbol *name_;
  Exp *exp_;

  EField(sym::Symbol *name, Exp *exp) : name_(name), exp_(exp) {}
  EField(const EField &efield) = delete;
  EField(EField &&efield) = delete;
  EField &operator=(const EField &efield) = delete;
  EField &operator=(EField &&efield) = delete;
  ~EField();

  void Print(FILE *out, int d) const;
};

class EFieldList {
public:
  EFieldList() = default;
  explicit EFieldList(EField *efield) : efield_list_({efield}) {}

  EFieldList *Prepend(EField *efield) {
    efield_list_.push_front(efield);
    return this;
  }
  [[nodiscard]] const std::list<EField *> &GetList() const {
    return efield_list_;
  }
  void Print(FILE *out, int d) const;

private:
  std::list<EField *> efield_list_;
};

}; // namespace absyn

#endif // TIGER_ABSYN_ABSYN_H_
