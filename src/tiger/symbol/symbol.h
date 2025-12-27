/**
 * @file symbol.h
 * @brief Symbol table implementation for Tiger compiler
 * 
 * This module provides symbol management for the Tiger compiler:
 * - Symbol: Interned string identifiers (unique per name)
 * - Table: Scoped symbol table with begin/end scope support
 * 
 * Symbols are interned (unique per name) to enable fast comparison by pointer.
 * The Table class provides scoped symbol lookup with BeginScope()/EndScope()
 * for managing nested scopes (e.g., function parameters, let expressions).
 */

#ifndef TIGER_SYMBOL_SYMBOL_H_
#define TIGER_SYMBOL_SYMBOL_H_

#include <string>

#include "tiger/util/table.h"

/**
 * @brief Forward declarations
 */
namespace env {
class EnvEntry;
} // namespace env
namespace type {
class Ty;
} // namespace type

namespace sym {

/**
 * @brief Interned symbol (unique identifier)
 * 
 * Symbols are interned strings: each unique name maps to a single Symbol object.
 * This enables fast comparison by pointer equality instead of string comparison.
 * Symbols are created via UniqueSymbol() which maintains a hash table.
 */
class Symbol {
  template <typename ValueType> friend class Table;

public:
  /**
   * @brief Get or create a unique symbol for the given name
   * @param name String name to intern
   * @return Unique Symbol object for this name
   * 
   * If a symbol with this name already exists, returns it.
   * Otherwise, creates a new symbol and adds it to the hash table.
   */
  static Symbol *UniqueSymbol(std::string_view);
  
  /**
   * @brief Get the string name of this symbol
   * @return The original string name
   */
  [[nodiscard]] std::string Name() const { return name_; }

private:
  Symbol(std::string name, Symbol *next)
      : name_(std::move(name)), next_(next) {}

  std::string name_;  ///< The interned string name
  Symbol *next_;      ///< Next symbol in hash bucket (for collision handling)
};

/**
 * @brief Scoped symbol table
 * 
 * A symbol table that supports nested scopes. Symbols can be looked up
 * in the current scope and all enclosing scopes. BeginScope() starts a new
 * scope; EndScope() removes all bindings added since the last BeginScope().
 * 
 * @tparam ValueType Type of values stored in the table (e.g., EnvEntry, Ty)
 */
template <typename ValueType>
class Table : public tab::Table<Symbol, ValueType> {
public:
  Table() : tab::Table<Symbol, ValueType>() {}
  
  /**
   * @brief Begin a new scope
   * 
   * Pushes a scope marker onto the table. All subsequent Enter() calls
   * will be in this new scope until EndScope() is called.
   */
  void BeginScope();
  
  /**
   * @brief End the current scope
   * 
   * Removes all bindings added since the last BeginScope(), restoring
   * the previous scope. The scope marker is also removed.
   */
  void EndScope();

private:
  Symbol marksym_ = {"<mark>", nullptr};  ///< Scope marker symbol
};

template <typename ValueType> void Table<ValueType>::BeginScope() {
  this->Enter(&marksym_, nullptr);
}

template <typename ValueType> void Table<ValueType>::EndScope() {
  Symbol *s;
  do
    s = this->Pop();
  while (s != &marksym_);
}

} // namespace sym

#endif // TIGER_SYMBOL_SYMBOL_H_
