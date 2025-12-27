/**
 * @file escape.h
 * @brief Escape analysis for Tiger compiler
 * 
 * Escape analysis determines which variables "escape" their defining scope,
 * i.e., are accessed from nested functions. Variables that escape must be
 * stored in the frame (heap/stack) rather than registers.
 * 
 * This analysis is performed before IR translation to inform frame layout
 * decisions. Variables marked as escaping will be allocated in memory.
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
 * @brief Escape analysis entry for a variable
 * 
 * Tracks the nesting depth where a variable is defined and a pointer to
 * the escape flag that will be set if the variable escapes.
 */
class EscapeEntry {
public:
  int depth_;      ///< Nesting depth where variable is defined
  bool *escape_;   ///< Pointer to escape flag (set if variable escapes)

  EscapeEntry(int depth, bool *escape) : depth_(depth), escape_(escape) {}
};

using EscEnv = sym::Table<esc::EscapeEntry>;      ///< Escape analysis environment
using EscEnvPtr = sym::Table<esc::EscapeEntry> *;  ///< Pointer to escape environment

/**
 * @brief Escape analysis finder
 * 
 * Performs escape analysis on an AST to mark variables that escape their scope.
 * Variables accessed from nested functions are marked as escaping.
 */
class EscFinder {
public:
  EscFinder() = delete;
  explicit EscFinder(std::unique_ptr<absyn::AbsynTree> absyn_tree)
      : absyn_tree_(std::move(absyn_tree)), env_(std::make_unique<EscEnv>()) {}

  /**
   * @brief Perform escape analysis
   * 
   * Traverses the AST and marks variables that escape their defining scope.
   * Variables accessed from functions nested deeper than their definition
   * are marked as escaping.
   */
  void FindEscape();

  /**
   * Transfer the ownership of absyn tree to outer scope
   * @return unique pointer to the absyn tree
   */
  std::unique_ptr<absyn::AbsynTree> TransferAbsynTree() {
    return std::move(absyn_tree_);
  }

private:
  std::unique_ptr<absyn::AbsynTree> absyn_tree_;
  std::unique_ptr<esc::EscEnv> env_;
};
} // namespace esc

#endif