/**
 * @file types.h
 * @brief Type system for the Tiger programming language
 *
 * This file defines the type system used during semantic analysis and IR
 * translation. It provides classes for representing all Tiger types:
 *
 *   - NilTy    : the type of the `nil` literal (compatible with any record)
 *   - IntTy    : the built-in integer type
 *   - StringTy : the built-in string type
 *   - VoidTy   : the "no value" type (returned by procedures, loops, etc.)
 *   - RecordTy : a struct-like type with named, typed fields
 *   - ArrayTy  : a homogeneous array type parameterised by element type
 *   - NameTy   : a named type alias (resolved lazily via ActualTy())
 *
 * Singleton instances are used for the four primitive types (NilTy, IntTy,
 * StringTy, VoidTy) so that pointer equality can be used for fast checks.
 *
 * Type equivalence in Tiger:
 *   - Two record types are equal only if they are the *same* object (nominal
 *     typing, not structural).
 *   - Two array types are equal only if they are the *same* object.
 *   - NilTy is compatible with any RecordTy.
 *   - NameTy is transparent: ActualTy() resolves the chain of aliases.
 *
 * Cycle detection for mutually recursive type declarations is performed in
 * the semantic analysis phase (TypeDec::SemAnalyze).
 */

#ifndef TIGER_SEMANT_TYPES_H_
#define TIGER_SEMANT_TYPES_H_

#include "tiger/symbol/symbol.h"
#include <list>

namespace type {

class TyList;
class Field;
class FieldList;

/**
 * @brief Abstract base class for all Tiger types
 *
 * Every concrete type class inherits from Ty and may override:
 *   - ActualTy()    – resolve type aliases (default: return this)
 *   - IsSameType()  – structural/nominal equality check
 */
class Ty {
public:
  /**
   * @brief Resolve type aliases, returning the underlying concrete type
   *
   * For most types this simply returns `this`. For NameTy it follows the
   * chain of aliases until a non-NameTy is reached.
   *
   * @return The concrete (non-alias) type this type ultimately refers to
   */
  virtual Ty *ActualTy();

  /**
   * @brief Check whether this type is compatible with @p other
   *
   * Handles the special cases:
   *   - NilTy is compatible with any RecordTy
   *   - RecordTy / ArrayTy use object identity (nominal typing)
   *   - Primitive types use class identity
   *
   * Both sides are resolved via ActualTy() before comparison.
   *
   * @param other The type to compare against
   * @return true if the types are compatible, false otherwise
   */
  virtual bool IsSameType(Ty *other);

protected:
  Ty() = default;
};

/**
 * @brief The type of the `nil` literal
 *
 * NilTy is a singleton. It is compatible with any record type, allowing
 * `nil` to be used as an initial value or in comparisons for any record.
 */
class NilTy : public Ty {
public:
  /** @return The unique NilTy instance */
  static NilTy *Instance() { return &nilty_; }

private:
  static NilTy nilty_;
};

/**
 * @brief The built-in integer type
 *
 * IntTy is a singleton. All integer literals and arithmetic expressions
 * have this type.
 */
class IntTy : public Ty {
public:
  /** @return The unique IntTy instance */
  static IntTy *Instance() { return &intty_; }

private:
  static IntTy intty_;
};

/**
 * @brief The built-in string type
 *
 * StringTy is a singleton. String literals and the result of string
 * operations (e.g., concat, substring) have this type.
 */
class StringTy : public Ty {
public:
  /** @return The unique StringTy instance */
  static StringTy *Instance() { return &stringty_; }

private:
  static StringTy stringty_;
};

/**
 * @brief The "no value" type
 *
 * VoidTy is a singleton used for expressions that produce no value:
 *   - Procedure calls (functions without a return type)
 *   - while / for loop bodies
 *   - Assignment expressions
 *   - if-then (without else) expressions
 */
class VoidTy : public Ty {
public:
  /** @return The unique VoidTy instance */
  static VoidTy *Instance() { return &voidty_; }

private:
  static VoidTy voidty_;
};

/**
 * @brief A record type: a struct with named, typed fields
 *
 * Record types use nominal (object-identity) equality: two separately
 * declared record types with identical field lists are *not* the same type.
 * The `nil` literal is compatible with any record type.
 */
class RecordTy : public Ty {
public:
  FieldList *fields_; ///< Ordered list of (name, type) field descriptors

  explicit RecordTy(FieldList *fields) : fields_(fields) {}
};

/**
 * @brief An array type: a homogeneous sequence of elements
 *
 * Array types use nominal (object-identity) equality: two separately
 * declared array types with the same element type are *not* the same type.
 */
class ArrayTy : public Ty {
public:
  Ty *ty_; ///< Element type of the array

  explicit ArrayTy(Ty *ty) : ty_(ty) {}
};

/**
 * @brief A named type alias
 *
 * Represents a `type name = ...` declaration. The alias is resolved lazily
 * by ActualTy(), which follows the chain of NameTy pointers until a
 * concrete type is reached.
 *
 * During the first pass of TypeDec::SemAnalyze, NameTy objects are created
 * with ty_ == nullptr; the second pass fills in ty_. A cycle-detection pass
 * then checks for illegal recursive type aliases (e.g., `type a = a`).
 */
class NameTy : public Ty {
public:
  sym::Symbol *sym_; ///< The alias name (for cycle detection and error messages)
  Ty *ty_;           ///< The type this name resolves to (nullptr until filled in)

  NameTy(sym::Symbol *sym, Ty *ty) : sym_(sym), ty_(ty) {}

  /**
   * @brief Resolve the alias chain to the underlying concrete type
   *
   * Follows ty_ pointers through any number of NameTy layers.
   * Returns the first non-NameTy type encountered.
   *
   * @return The concrete type at the end of the alias chain
   */
  Ty *ActualTy() override;
};

/**
 * @brief An ordered list of types
 *
 * Used to represent the formal parameter types of a function.
 */
class TyList {
public:
  TyList() = default;
  explicit TyList(Ty *ty) : ty_list_({ty}) {}
  TyList(std::initializer_list<Ty *> list) : ty_list_(list) {}

  const std::list<Ty *> &GetList() { return ty_list_; }
  void Append(Ty *ty) { ty_list_.push_back(ty); }

private:
  std::list<Ty *> ty_list_;
};

/**
 * @brief A single record field descriptor: (name, type)
 *
 * Used inside FieldList to describe the fields of a RecordTy.
 */
class Field {
public:
  sym::Symbol *name_; ///< Field name
  Ty *ty_;            ///< Field type

  Field(sym::Symbol *name, Ty *ty) : name_(name), ty_(ty) {}
};

/**
 * @brief An ordered list of record field descriptors
 *
 * Represents the complete set of fields for a RecordTy, in declaration order.
 */
class FieldList {
public:
  FieldList() = default;
  explicit FieldList(Field *field) : field_list_({field}) {}

  std::list<Field *> &GetList() { return field_list_; }
  void Append(Field *field) { field_list_.push_back(field); }

private:
  std::list<Field *> field_list_;
};

} // namespace type

#endif // TIGER_SEMANT_TYPES_H_
