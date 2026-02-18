/**
 * @file translate.h
 * @brief IR translation layer for the Tiger compiler
 *
 * This module bridges the front-end (AST / semantic analysis) and the
 * back-end (IR tree / code generation).  It is responsible for:
 *
 *   1. Managing activation records (frames) and nesting levels
 *   2. Translating AST nodes into IR tree fragments (tr::Exp / tr::ExpAndTy)
 *   3. Handling static links for nested function access
 *   4. Collecting procedure and string fragments (frame::Frags)
 *
 * ─────────────────────────────────────────────────────────────────────────
 * Key abstractions
 * ─────────────────────────────────────────────────────────────────────────
 *
 * tr::Level
 *   Represents one nesting level (one function activation).  Each Level
 *   owns a frame::Frame and a pointer to its parent Level.  The outermost
 *   level has no parent (parent_ == nullptr).
 *
 * tr::Access
 *   Represents a variable's storage location relative to a Level.
 *   Wraps a frame::Access (in-register or in-frame) together with the
 *   Level at which the variable was declared.  When a variable is accessed
 *   from a deeper level, the translator follows static links to reach the
 *   declaring frame.
 *
 * tr::Exp  (ExExp / NxExp / CxExp)
 *   Three-way representation of translated expressions, following Appel
 *   Chapter 7:
 *     ExExp – an expression that produces a value (tree::Exp)
 *     NxExp – a statement with no value (tree::Stm)
 *     CxExp – a conditional (a CJUMP with dangling true/false labels)
 *   Each variant can be coerced to any of the three forms via UnEx/UnNx/UnCx.
 *
 * tr::ExpAndTy
 *   Pairs a translated expression (tr::Exp*) with its Tiger type (type::Ty*).
 *   Returned by every Translate() method on AST nodes.
 *
 * ─────────────────────────────────────────────────────────────────────────
 * Static links
 * ─────────────────────────────────────────────────────────────────────────
 * Tiger supports nested functions.  A function at depth d that accesses a
 * variable declared at depth d' < d must follow (d - d') static links.
 * Each frame stores the static link as its first formal parameter.
 * The translator generates the appropriate chain of MEM(MEM(...)) accesses.
 *
 * ─────────────────────────────────────────────────────────────────────────
 * Fragment collection
 * ─────────────────────────────────────────────────────────────────────────
 * The global `frags` list accumulates:
 *   - frame::ProcFrag  – one per function body (IR tree + frame)
 *   - frame::StringFrag – one per string literal (label + content)
 * These are later consumed by output::AssemGen to produce assembly.
 */

#ifndef TIGER_TRANSLATE_TRANSLATE_H_
#define TIGER_TRANSLATE_TRANSLATE_H_

#include <list>
#include <memory>

#include "tiger/absyn/absyn.h"
#include "tiger/env/env.h"
#include "tiger/errormsg/errormsg.h"
#include "tiger/frame/frame.h"
#include "tiger/semant/types.h"

namespace tr {

class Exp;
class ExpAndTy;
class Level;

/**
 * @brief A variable's storage location relative to its declaring Level
 *
 * Combines a frame::Access (which describes the physical location: register
 * or frame slot) with the Level at which the variable was declared.
 *
 * When generating code to read/write the variable, the translator compares
 * the current Level with level_ and follows static links as needed.
 */
class Access {
public:
  Level *level_;          ///< The nesting level where this variable is declared
  frame::Access *access_; ///< Physical location within that level's frame

  Access(Level *level, frame::Access *access)
      : level_(level), access_(access) {}

  /**
   * @brief Allocate a new local variable in the given level
   * @param level  The level (function) in which to allocate
   * @param escape true if the variable escapes (must be in the frame);
   *               false if it can live in a register
   * @return A new Access describing the allocated location
   */
  static Access *AllocLocal(Level *level, bool escape);
};

/**
 * @brief One nesting level (one function activation record)
 *
 * Each function in the Tiger program corresponds to one Level.  Levels
 * form a tree rooted at the outermost level (the main program).
 *
 * The frame_ member holds the machine-specific activation record layout
 * (formal parameters, local variables, frame size, etc.).
 *
 * The parent_ pointer is used by the translator to follow static links
 * when accessing variables declared in enclosing functions.
 */
class Level {
public:
  frame::Frame *frame_; ///< Activation record for this function
  Level *parent_;       ///< Enclosing function's level (nullptr for outermost)

  /** @brief Construct the outermost (dummy) level */
  Level() : frame_(nullptr), parent_(nullptr) {}

  /** @brief Construct a level with a frame but no parent (top-level) */
  explicit Level(frame::Frame *frame) : frame_(frame), parent_(nullptr) {}

  /** @brief Construct a nested level with a parent */
  Level(frame::Frame *frame, Level *parent) : frame_(frame), parent_(parent) {}
};

/**
 * @brief IR translation driver
 *
 * Owns the AST, error reporter, type/variable environments, and the
 * outermost level.  Translate() walks the AST and produces IR fragments
 * that are accumulated in the global `frags` list.
 *
 * Typical usage:
 * @code
 *   tr::ProgTr prog_tr(std::move(absyn_tree), std::move(errormsg));
 *   prog_tr.Translate();
 *   errormsg = prog_tr.TransferErrormsg();
 *   // frags now contains ProcFrag and StringFrag objects
 * @endcode
 */
class ProgTr {
public:
  /**
   * @brief Construct the translation driver
   * @param absyn_tree Parsed and semantically-checked AST (ownership in)
   * @param errormsg   Error reporter (ownership in)
   *
   * Initialises the outermost level, populates the base environments with
   * Tiger built-in types and functions, and prepares for translation.
   */
  ProgTr(std::unique_ptr<absyn::AbsynTree> absyn_tree,
    std::unique_ptr<err::ErrorMsg> errormsg) {
      absyn_tree_ = std::move(absyn_tree);
      errormsg_ = std::move(errormsg);
      tenv_ = std::make_unique<env::TEnv>();
      venv_ = std::make_unique<env::VEnv>();
      outermost_level_ = std::make_unique<Level>();
      FillBaseVEnv();
      FillBaseTEnv();
    }

  /**
   * @brief Translate the entire program to IR
   *
   * Creates the main function frame (label "tigermain"), translates the
   * top-level expression, wraps it with ProcEntryExit1 (view shift +
   * callee-save save/restore), and pushes a ProcFrag onto `frags`.
   */
  void Translate();

  /**
   * @brief Transfer ownership of the error-message object to the caller
   * @return Unique pointer to the error-message object
   */
  std::unique_ptr<err::ErrorMsg> TransferErrormsg() {
    return std::move(errormsg_);
  }

private:
  std::unique_ptr<absyn::AbsynTree> absyn_tree_; ///< The program AST
  std::unique_ptr<err::ErrorMsg> errormsg_;       ///< Error reporter
  std::unique_ptr<Level> outermost_level_;        ///< Dummy outermost level
  std::unique_ptr<env::TEnv> tenv_;               ///< Type environment
  std::unique_ptr<env::VEnv> venv_;               ///< Variable/function environment

  /** @brief Populate venv_ with Tiger built-in functions */
  void FillBaseVEnv();
  /** @brief Populate tenv_ with Tiger built-in types (int, string) */
  void FillBaseTEnv();
};

/**
 * @brief Record a completed function body as a fragment
 *
 * Wraps the body IR tree in a frame::ProcFrag and appends it to the
 * global `frags` list.  Called after translating each function body.
 *
 * @param level The level (frame) of the completed function
 * @param body  The translated body expression (as NxExp)
 */
void ProcEntryExit(Level *level, Exp *body);

} // namespace tr

#endif // TIGER_TRANSLATE_TRANSLATE_H_
