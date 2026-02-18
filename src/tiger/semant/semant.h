/**
 * @file semant.h
 * @brief Semantic analysis driver for the Tiger compiler - Lab 4
 *
 * This module drives the semantic analysis phase of the Tiger compiler.
 * Semantic analysis verifies that a syntactically correct program also
 * obeys the type and scoping rules of the Tiger language.
 *
 * Responsibilities:
 * - Type checking: every expression has a well-defined type
 * - Scope checking: every identifier is declared before use
 * - Mutually recursive declarations: type and function groups are handled
 * - Read-only loop variables: for-loop variables cannot be assigned
 * - Break statement validity: break only inside loops
 *
 * The analysis is performed by the SemAnalyze() methods on each AST node
 * (defined in absyn.cc / semant.cc). This class sets up the initial
 * environments and invokes the traversal.
 *
 * Environments:
 * - VEnv (variable environment): maps names to VarEntry / FunEntry
 * - TEnv (type environment): maps names to type::Ty objects
 *
 * Pre-populated entries (Tiger built-ins):
 *   Types:  int, string
 *   Functions: print, flush, getchar, ord, chr, size, substring,
 *              concat, not, exit
 */

#ifndef TIGER_SEMANT_SEMANT_H_
#define TIGER_SEMANT_SEMANT_H_

#include <list>
#include <memory>

#include "tiger/absyn/absyn.h"
#include "tiger/env/env.h"
#include "tiger/errormsg/errormsg.h"
#include "tiger/semant/types.h"

namespace sem {

/**
 * @brief Semantic analysis driver (Lab 4 only)
 *
 * Owns the AST and error-message object for the duration of semantic
 * analysis. After SemAnalyze() returns, the caller retrieves the
 * (possibly annotated) AST and error state via the Transfer* methods.
 *
 * Typical usage:
 * @code
 *   sem::ProgSem prog_sem(std::move(absyn_tree), std::move(errormsg));
 *   prog_sem.SemAnalyze();
 *   absyn_tree = prog_sem.TransferAbsynTree();
 *   errormsg   = prog_sem.TransferErrormsg();
 * @endcode
 */
class ProgSem {
public:
  /**
   * @brief Construct a semantic analysis driver
   * @param absyn_tree Parsed AST (ownership transferred in)
   * @param errormsg   Error reporter (ownership transferred in)
   */
  ProgSem(std::unique_ptr<absyn::AbsynTree> absyn_tree,
         std::unique_ptr<err::ErrorMsg> errormsg)
      : absyn_tree_(std::move(absyn_tree)), errormsg_(std::move(errormsg)),
        tenv_(std::make_unique<env::TEnv>()),
        venv_(std::make_unique<env::VEnv>()) {}

  /**
   * @brief Run semantic analysis on the AST
   *
   * Populates the base environments with Tiger built-in types and
   * functions, then traverses the AST calling SemAnalyze() on each node.
   * Errors are accumulated in errormsg_; check errormsg_->AnyErrors()
   * after this call.
   */
  void SemAnalyze();

  /**
   * @brief Transfer ownership of the error-message object to the caller
   * @return Unique pointer to the error-message object
   */
  std::unique_ptr<err::ErrorMsg> TransferErrormsg() {
    return std::move(errormsg_);
  }

  /**
   * @brief Transfer ownership of the AST to the caller
   * @return Unique pointer to the abstract syntax tree
   */
  std::unique_ptr<absyn::AbsynTree> TransferAbsynTree() {
    return std::move(absyn_tree_);
  }

private:
  std::unique_ptr<absyn::AbsynTree> absyn_tree_; ///< The program AST
  std::unique_ptr<err::ErrorMsg> errormsg_;       ///< Error reporter

  /// Type environment: maps type names (sym::Symbol*) to type::Ty*
  std::unique_ptr<env::TEnv> tenv_;
  /// Variable/function environment: maps names to env::EnvEntry*
  std::unique_ptr<env::VEnv> venv_;

  /**
   * @brief Populate venv_ with Tiger built-in functions
   *
   * Adds entries for: print, flush, getchar, ord, chr, size,
   * substring, concat, not, exit.
   */
  void FillBaseVEnv();

  /**
   * @brief Populate tenv_ with Tiger built-in types
   *
   * Adds entries for: int, string.
   */
  void FillBaseTEnv();
};

} // namespace sem

#endif // TIGER_SEMANT_SEMANT_H_
