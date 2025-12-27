/**
 * @file env.h
 * @brief Environment entries for symbol tables
 * 
 * This module defines the entries stored in symbol tables:
 * - EnvEntry: Base class for all environment entries
 * - VarEntry: Variable bindings (type, access information)
 * - FunEntry: Function bindings (parameters, return type, level, label)
 * 
 * The environment is split into two symbol tables:
 * - VEnv: Variable and function environment (VarEntry, FunEntry)
 * - TEnv: Type environment (Ty objects)
 * 
 * Entries are used during semantic analysis and IR translation.
 */

#ifndef TIGER_ENV_ENV_H_
#define TIGER_ENV_ENV_H_

#include "tiger/frame/temp.h"
#include "tiger/semant/types.h"
#include "tiger/symbol/symbol.h"

/**
 * @brief Forward declarations
 */
namespace tr {
class Access;
class Level;
} // namespace tr

namespace env {

/**
 * @brief Base class for environment entries
 * 
 * All entries in the variable environment inherit from this.
 * Tracks whether the entry is readonly (e.g., function parameters, loop variables).
 */
class EnvEntry {
public:
  bool readonly_;  ///< Whether this entry can be modified

  explicit EnvEntry(bool readonly = true) : readonly_(readonly) {}
  virtual ~EnvEntry() = default;
};

/**
 * @brief Variable entry in the environment
 * 
 * Stores information about a variable:
 * - Type: The variable's type
 * - Access: Frame location (register or memory offset) - set during translation
 * - Readonly: Whether the variable can be assigned to
 */
class VarEntry : public EnvEntry {
public:
  tr::Access *access_;  ///< Frame access (set during IR translation)
  type::Ty *ty_;        ///< Variable type

  /**
   * @brief Constructor for semantic analysis phase (Lab 4)
   * @param ty Variable type
   * @param readonly Whether variable is readonly
   */
  explicit VarEntry(type::Ty *ty, bool readonly = false)
      : EnvEntry(readonly), ty_(ty), access_(nullptr){};

  /**
   * @brief Constructor for IR translation phase (Lab 5)
   * @param access Frame access location
   * @param ty Variable type
   * @param readonly Whether variable is readonly
   */
  VarEntry(tr::Access *access, type::Ty *ty, bool readonly = false)
      : EnvEntry(readonly), ty_(ty), access_(access){};
};

/**
 * @brief Function entry in the environment
 * 
 * Stores information about a function:
 * - Formals: List of parameter types
 * - Result: Return type
 * - Level: Static link level (for nested functions)
 * - Label: Function entry label (for code generation)
 */
class FunEntry : public EnvEntry {
public:
  tr::Level *level_;      ///< Function's static link level (set during translation)
  temp::Label *label_;    ///< Function entry label (set during translation)
  type::TyList *formals_; ///< List of parameter types
  type::Ty *result_;      ///< Return type

  /**
   * @brief Constructor for semantic analysis phase (Lab 4)
   * @param formals List of parameter types
   * @param result Return type
   */
  FunEntry(type::TyList *formals, type::Ty *result)
      : formals_(formals), result_(result), level_(nullptr), label_(nullptr) {}

  /**
   * @brief Constructor for IR translation phase (Lab 5)
   * @param level Function's static link level
   * @param label Function entry label
   * @param formals List of parameter types
   * @param result Return type
   */
  FunEntry(tr::Level *level, temp::Label *label, type::TyList *formals,
           type::Ty *result)
      : formals_(formals), result_(result), level_(level), label_(label) {}
};

// Type aliases for environment tables
using VEnv = sym::Table<env::EnvEntry>;      ///< Variable environment table
using TEnv = sym::Table<type::Ty>;            ///< Type environment table
using VEnvPtr = sym::Table<env::EnvEntry> *;  ///< Pointer to variable environment
using TEnvPtr = sym::Table<type::Ty> *;       ///< Pointer to type environment
} // namespace env

#endif // TIGER_ENV_ENV_H_
