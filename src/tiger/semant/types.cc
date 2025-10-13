/**
 * @file types.cc
 * @brief Implementation of the Tiger type system - Lab 4
 *
 * This file implements the type system used during semantic analysis.
 * It provides the core functionality for type equivalence checking and
 * actual type resolution for named types.
 */

#include "tiger/semant/types.h"

namespace type {

/**
 * @brief Static instances of built-in types
 *
 * These singleton instances are used throughout the compiler for the
 * built-in Tiger types: nil, int, string, and void.
 */
NilTy NilTy::nilty_;
IntTy IntTy::intty_;
StringTy StringTy::stringty_;
VoidTy VoidTy::voidty_;

/**
 * @brief Get the actual type, resolving any type aliases (default implementation)
 *
 * For simple types like IntTy, StringTy, etc., returns this. For NameTy,
 * recursively resolves until reaching a concrete type.
 *
 * @return The concrete type this type refers to
 */
Ty *Ty::ActualTy() { return this; }

/**
 * @brief Get the actual type for named types
 *
 * Named types refer to other types, so this method recursively resolves
 * the type chain until reaching a concrete type (RecordTy, ArrayTy, etc.).
 *
 * @return The concrete type this named type refers to
 */
Ty *NameTy::ActualTy() {
  assert(ty_ != this);
  return ty_->ActualTy();
}

/**
 * @brief Check if two types are equivalent
 *
 * Compares types for structural equivalence, taking into account:
 * - Actual types for named types (resolves type aliases)
 * - Special compatibility between nil and record types
 * - Pointer equality for concrete types
 *
 * @param expected Type to compare with
 * @return true if types are equivalent, false otherwise
 */
bool Ty::IsSameType(Ty *expected) {
  Ty *a = ActualTy();
  Ty *b = expected->ActualTy();

  if ((typeid(*a) == typeid(NilTy) && typeid(*b) == typeid(RecordTy)) ||
      (typeid(*a) == typeid(RecordTy) && typeid(*b) == typeid(NilTy)))
    return true;

  return a == b;
}

} // namespace type
