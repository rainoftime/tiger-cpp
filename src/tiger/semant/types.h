/**
 * @file types.h
 * @brief Type system for the Tiger programming language - Lab 4
 *
 * This file defines the type system used during semantic analysis.
 * It provides classes for representing all Tiger types including:
 * - Basic types (int, string, void)
 * - Composite types (records, arrays)
 * - Named types (type aliases)
 * - Type lists and field lists for function signatures and record definitions
 *
 * The type system supports type equivalence checking and actual type resolution
 * for named types, enabling proper type checking during semantic analysis.
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
 * This class defines the interface that all type classes must implement.
 * It provides methods for type equivalence checking and actual type resolution
 * (for named types that may refer to other types).
 */
class Ty {
public:
  /**
   * @brief Get the actual type, resolving any type aliases
   *
   * For simple types like IntTy, returns this. For NameTy, recursively
   * resolves until reaching a concrete type (RecordTy, ArrayTy, etc.).
   *
   * @return The concrete type this type refers to
   */
  virtual Ty *ActualTy();

  /**
   * @brief Check if this type is the same as another type
   *
   * Compares types for structural equivalence, taking into account
   * actual types for named types and proper handling of nil types.
   *
   * @param other Type to compare with
   * @return true if types are equivalent, false otherwise
   */
  virtual bool IsSameType(Ty *other);

protected:
  /** @brief Protected constructor to prevent direct instantiation */
  Ty() = default;
};

class NilTy : public Ty {
public:
  static NilTy *Instance() { return &nilty_; }

private:
  static NilTy nilty_;
};

class IntTy : public Ty {
public:
  static IntTy *Instance() { return &intty_; }

private:
  static IntTy intty_;
};

class StringTy : public Ty {
public:
  static StringTy *Instance() { return &stringty_; }

private:
  static StringTy stringty_;
};

class VoidTy : public Ty {
public:
  static VoidTy *Instance() { return &voidty_; }

private:
  static VoidTy voidty_;
};

class RecordTy : public Ty {
public:
  FieldList *fields_;

  explicit RecordTy(FieldList *fields) : fields_(fields) {}
};

class ArrayTy : public Ty {
public:
  Ty *ty_;

  explicit ArrayTy(Ty *ty) : ty_(ty) {}
};

class NameTy : public Ty {
public:
  sym::Symbol *sym_;
  Ty *ty_;

  NameTy(sym::Symbol *sym, Ty *ty) : sym_(sym), ty_(ty) {}

  Ty *ActualTy() override;
};

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

class Field {
public:
  sym::Symbol *name_;
  Ty *ty_;

  Field(sym::Symbol *name, Ty *ty) : name_(name), ty_(ty) {}
};

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
