/**
 * @file env.cc
 * @brief Implementation of base environment initialization
 * 
 * This file initializes the base type and variable environments with
 * built-in types and standard library functions. The base environment
 * is populated separately for semantic analysis (sem namespace) and
 * IR translation (tr namespace).
 */

#include "tiger/env/env.h"
#include "tiger/translate/translate.h"
#include "tiger/semant/semant.h"

namespace sem {

/**
 * @brief Initialize base type environment for semantic analysis
 * 
 * Adds built-in types: int and string
 */
void ProgSem::FillBaseTEnv() {
  tenv_->Enter(sym::Symbol::UniqueSymbol("int"), type::IntTy::Instance());
  tenv_->Enter(sym::Symbol::UniqueSymbol("string"), type::StringTy::Instance());
}

/**
 * @brief Initialize base variable environment for semantic analysis
 * 
 * Adds standard library functions:
 * - flush(): Flush output buffer
 * - exit(n): Exit with code n
 * - chr(n): Convert integer to character
 * - getchar(): Read character from input
 * - print(s): Print string
 * - printi(n): Print integer
 * - ord(s): Get first character of string as integer
 * - size(s): Get string length
 * - concat(s1, s2): Concatenate two strings
 * - substring(s, start, len): Extract substring
 */
void ProgSem::FillBaseVEnv() {
  type::Ty *result;
  type::TyList *formals;

  venv_->Enter(sym::Symbol::UniqueSymbol("flush"),
               new env::FunEntry(new type::TyList(), type::VoidTy::Instance()));

  formals = new type::TyList(type::IntTy::Instance());

  venv_->Enter(
      sym::Symbol::UniqueSymbol("exit"),
      new env::FunEntry(formals, type::VoidTy::Instance()));

  result = type::StringTy::Instance();

  venv_->Enter(sym::Symbol::UniqueSymbol("chr"),
               new env::FunEntry(formals, result));

  venv_->Enter(sym::Symbol::UniqueSymbol("getchar"),
               new env::FunEntry(new type::TyList(), result));

  formals = new type::TyList(type::StringTy::Instance());

  venv_->Enter(
      sym::Symbol::UniqueSymbol("print"),
      new env::FunEntry(formals, type::VoidTy::Instance()));
  venv_->Enter(sym::Symbol::UniqueSymbol("printi"),
               new env::FunEntry(new type::TyList(type::IntTy::Instance()),
                                 type::VoidTy::Instance()));

  result = type::IntTy::Instance();
  venv_->Enter(sym::Symbol::UniqueSymbol("ord"),
               new env::FunEntry(formals, result));

  venv_->Enter(sym::Symbol::UniqueSymbol("size"),
               new env::FunEntry(formals, result));

  result = type::StringTy::Instance();
  formals = new type::TyList(
      {type::StringTy::Instance(), type::StringTy::Instance()});
  venv_->Enter(sym::Symbol::UniqueSymbol("concat"),
               new env::FunEntry(formals, result));

  formals =
      new type::TyList({type::StringTy::Instance(), type::IntTy::Instance(),
                        type::IntTy::Instance()});
  venv_->Enter(sym::Symbol::UniqueSymbol("substring"),
               new env::FunEntry(formals, result));

}

} // namespace sem

namespace tr {

/**
 * @brief Initialize base type environment for IR translation
 * 
 * Adds built-in types: int and string
 * Same as semantic analysis version.
 */
void ProgTr::FillBaseTEnv() {
  tenv_->Enter(sym::Symbol::UniqueSymbol("int"), type::IntTy::Instance());
  tenv_->Enter(sym::Symbol::UniqueSymbol("string"), type::StringTy::Instance());
}

/**
 * @brief Initialize base variable environment for IR translation
 * 
 * Adds standard library functions with frame information (level, label).
 * Same functions as semantic analysis, but includes translation-time
 * information needed for code generation.
 */
void ProgTr::FillBaseVEnv() {
  type::Ty *result;
  type::TyList *formals;

  temp::Label *label = nullptr;
  tr::Level *level = outermost_level_.get();

  venv_->Enter(sym::Symbol::UniqueSymbol("flush"),
               new env::FunEntry(level, label, new type::TyList(),
                                 type::VoidTy::Instance()));

  formals = new type::TyList(type::IntTy::Instance());

  venv_->Enter(
      sym::Symbol::UniqueSymbol("exit"),
      new env::FunEntry(level, label, formals, type::VoidTy::Instance()));

  result = type::StringTy::Instance();

  venv_->Enter(sym::Symbol::UniqueSymbol("chr"),
               new env::FunEntry(level, label, formals, result));

  venv_->Enter(sym::Symbol::UniqueSymbol("getchar"),
               new env::FunEntry(level, label, new type::TyList(), result));

  formals = new type::TyList(type::StringTy::Instance());

  venv_->Enter(
      sym::Symbol::UniqueSymbol("print"),
      new env::FunEntry(level, label, formals, type::VoidTy::Instance()));
  venv_->Enter(sym::Symbol::UniqueSymbol("printi"),
               new env::FunEntry(level, label,
                                 new type::TyList(type::IntTy::Instance()),
                                 type::VoidTy::Instance()));

  result = type::IntTy::Instance();
  venv_->Enter(sym::Symbol::UniqueSymbol("ord"),
               new env::FunEntry(level, label, formals, result));

  venv_->Enter(sym::Symbol::UniqueSymbol("size"),
               new env::FunEntry(level, label, formals, result));

  result = type::StringTy::Instance();
  formals = new type::TyList(
      {type::StringTy::Instance(), type::StringTy::Instance()});
  venv_->Enter(sym::Symbol::UniqueSymbol("concat"),
               new env::FunEntry(level, label, formals, result));

  formals =
      new type::TyList({type::StringTy::Instance(), type::IntTy::Instance(),
                        type::IntTy::Instance()});
  venv_->Enter(sym::Symbol::UniqueSymbol("substring"),
               new env::FunEntry(level, label, formals, result));

}

} // namespace tr
