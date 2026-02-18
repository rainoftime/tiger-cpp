/**
 * @file escape.h
 * @brief Escape analysis for the Tiger compiler
 *
 * This module performs escape analysis on the Tiger AST.  A variable
 * "escapes" if it is accessed from a nested function (i.e., a function
 * defined inside the function that declares the variable).
 *
 * ─────────────────────────────────────────────────────────────────────────
 * Why escape analysis matters
 * ─────────────────────────────────────────────────────────────────────────
 * In Tiger, functions can be nested.  A nested function can access variables
 * declared in an enclosing function via static links.  Such variables must
 * be stored in the stack frame (not in a register), because:
 *   - The nested function accesses them via a frame-pointer chain
 *   - Registers are not addressable from other activation records
 *
 * A variable that is NOT accessed from any nested function can be stored
 * in a virtual register (faster, no memory traffic).
 *
 * ─────────────────────────────────────────────────────────────────────────
 * Algorithm
 * ─────────────────────────────────────────────────────────────────────────
 * The analysis traverses the AST and maintains a depth counter (the current
 * nesting depth).  For each variable declaration, it records the depth at
 * which the variable was declared.  When a variable is used, if the current
 * depth is greater than the declaration depth, the variable escapes.
 *
 * The escape flag is stored directly in the AST nodes:
 *   - absyn::VarDec::escape_
 *   - absyn::Field::escape_  (for record field parameters)
 *
 * These flags are later read by the IR translator (translate.cc) to decide
 * whether to allocate the variable in a register or in the frame.
 *
 * ─────────────────────────────────────────────────────────────────────────
 * Environment
 * ─────────────────────────────────────────────────────────────────────────
 * The escape analysis uses EscEnv (a symbol table mapping variable names to
 * EscapeEntry objects).  Each EscapeEntry records:
 *   - depth_:   the nesting depth at which the variable was declared
 *   - escape_:  a pointer to the escape flag in the AST node
 *
 * When a variable is used at a depth greater than its declaration depth,
 * the analysis sets *escape_ = true via the stored pointer.
 */

#ifndef TIGER_ESCAPE_ESCAPE_H_
#define TIGER_ESCAPE_ESCAPE_H_

#include <memory>

#include "tiger/symbol/symbol.h"

/**
 * @brief Forward declarations
 */
namespace absyn {
class AbsynTree;
} // namespace absyn

namespace esc {

/**
 * @brief Escape analysis entry for a single variable
 *
 * Stored in the escape environment (EscEnv) for each declared variable.
 * Tracks the nesting depth of the declaration and a pointer to the
 * escape flag in the AST node so the analysis can set it when needed.
 */
class EscapeEntry {
public:
  int depth_;    ///< Nesting depth where the variable is declared (0 = outermost)
  bool *escape_; ///< Pointer to the escape flag in the AST node (VarDec or Field)

  EscapeEntry(int depth, bool *escape) : depth_(depth), escape_(escape) {}
};

/** @brief Escape analysis environment: maps variable names to EscapeEntry */
using EscEnv = sym::Table<esc::EscapeEntry>;
/** @brief Pointer to an escape analysis environment */
using EscEnvPtr = sym::Table<esc::EscapeEntry> *;

/**
 * @brief Escape analysis driver
 *
 * Traverses the AST and marks variables that escape (are accessed from
 * nested functions).  The escape flags are stored in the AST nodes and
 * are later used by the IR translator to decide storage allocation.
 *
 * Typical usage:
 * @code
 *   esc::EscFinder esc_finder(std::move(absyn_tree));
 *   esc_finder.FindEscape();
 *   absyn_tree = esc_finder.TransferAbsynTree();
 * @endcode
 */
class EscFinder {
public:
  EscFinder() = delete;

  /**
   * @brief Construct an escape analysis driver
   * @param absyn_tree The AST to analyse (ownership transferred in)
   */
  explicit EscFinder(std::unique_ptr<absyn::AbsynTree> absyn_tree)
      : absyn_tree_(std::move(absyn_tree)), env_(std::make_unique<EscEnv>()) {}

  /**
   * @brief Perform escape analysis on the AST
   *
   * Traverses the entire AST, setting the escape_ flag on every variable
   * declaration that is accessed from a nested function.
   *
   * After this call, all VarDec::escape_ and Field::escape_ flags in the
   * AST are correctly set.
   */
  void FindEscape();

  /**
   * @brief Transfer ownership of the annotated AST to the caller
   * @return Unique pointer to the AST (with escape flags set)
   */
  std::unique_ptr<absyn::AbsynTree> TransferAbsynTree() {
    return std::move(absyn_tree_);
  }

private:
  std::unique_ptr<absyn::AbsynTree> absyn_tree_; ///< The AST being analysed
  std::unique_ptr<esc::EscEnv> env_;             ///< The escape environment
};

} // namespace esc

#endif // TIGER_ESCAPE_ESCAPE_H_
