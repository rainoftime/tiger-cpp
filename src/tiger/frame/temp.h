/**
 * @file temp.h
 * @brief Temporaries (virtual registers) and labels for the Tiger compiler
 *
 * This file defines the fundamental abstractions used throughout the
 * back-end of the Tiger compiler:
 *
 *   temp::Temp   – a virtual register (unlimited supply before register alloc)
 *   temp::Label  – a symbolic assembly label (alias for sym::Symbol)
 *   temp::Map    – a mapping from Temp* to register-name strings
 *   temp::TempList – an ordered list of temporaries with set operations
 *
 * ─────────────────────────────────────────────────────────────────────────
 * Temporaries (Temp)
 * ─────────────────────────────────────────────────────────────────────────
 * A Temp represents an abstract (virtual) register.  Before register
 * allocation there is an unlimited supply; after allocation each Temp is
 * mapped to a physical machine register or spilled to memory.
 *
 * Temps are created by TempFactory::NewTemp() and identified by a unique
 * integer (starting at 100 to avoid confusion with small constants).
 *
 * ─────────────────────────────────────────────────────────────────────────
 * Labels (Label)
 * ─────────────────────────────────────────────────────────────────────────
 * A Label is simply a sym::Symbol used as an assembly label.  Labels are
 * interned (unique per name) so pointer equality can be used.
 *
 *   LabelFactory::NewLabel()        – create a fresh anonymous label (L0, L1, …)
 *   LabelFactory::NamedLabel(name)  – create/retrieve a named label
 *   LabelFactory::LabelString(l)    – get the string name of a label
 *
 * ─────────────────────────────────────────────────────────────────────────
 * Temp-to-register map (Map)
 * ─────────────────────────────────────────────────────────────────────────
 * A Map associates Temp* objects with register-name strings.  It supports
 * layering: Map::LayerMap(over, under) looks up in `over` first, then `under`.
 *
 *   Map::Empty()  – empty map (no bindings)
 *   Map::Name()   – map that returns the temp's numeric name (e.g., "t42")
 *
 * The register manager (RegManager) uses a Map to associate pre-colored
 * temps with physical register names (e.g., "%rax").
 *
 * ─────────────────────────────────────────────────────────────────────────
 * TempList
 * ─────────────────────────────────────────────────────────────────────────
 * An ordered list of Temp* with set-like operations (Union, Diff, Contain).
 * Used to represent:
 *   - Def/Use sets for instructions (liveness analysis)
 *   - Argument register lists (calling convention)
 *   - Callee-saved / caller-saved register sets
 */

#ifndef TIGER_FRAME_TEMP_H_
#define TIGER_FRAME_TEMP_H_

#include "tiger/symbol/symbol.h"

#include <list>

namespace temp {

/** @brief A symbolic assembly label (interned string via sym::Symbol) */
using Label = sym::Symbol;

/**
 * @brief Factory for creating assembly labels
 *
 * Provides two kinds of labels:
 *   - Anonymous labels: L0, L1, L2, … (for compiler-generated control flow)
 *   - Named labels: arbitrary strings (for function names, string literals)
 */
class LabelFactory {
public:
  /**
   * @brief Create a fresh anonymous label
   * @return A new label with a unique name (e.g., "L0", "L1", …)
   */
  static Label *NewLabel();

  /**
   * @brief Get or create a named label
   * @param name The desired label name
   * @return The unique Label object for this name
   */
  static Label *NamedLabel(std::string_view name);

  /**
   * @brief Get the string name of a label
   * @param s The label
   * @return The label's string name
   */
  static std::string LabelString(Label *s);

private:
  int label_id_ = 0;                    ///< Counter for anonymous label names
  static LabelFactory label_factory;    ///< Singleton factory instance
};

/**
 * @brief A virtual (abstract) register
 *
 * Temps are created by TempFactory::NewTemp() and identified by a unique
 * integer.  Before register allocation, the compiler uses an unlimited
 * number of temps.  After allocation, each temp is mapped to a physical
 * register or spilled to a stack slot.
 *
 * Temps numbered 0–99 are reserved; user temps start at 100.
 * Pre-colored temps (machine registers) are created by X64RegManager and
 * have fixed colors assigned before register allocation begins.
 */
class Temp {
  friend class TempFactory;

public:
  /**
   * @brief Get the unique integer identifier for this temp
   * @return The temp's numeric ID
   */
  [[nodiscard]] int Int() const;

private:
  int num_;                             ///< Unique numeric identifier
  explicit Temp(int num) : num_(num) {}
};

/**
 * @brief Factory for creating virtual registers (temps)
 *
 * All temps are created through this factory to ensure unique IDs.
 */
class TempFactory {
public:
  /**
   * @brief Create a fresh virtual register
   * @return A new Temp with a unique ID (starting at 100)
   */
  static Temp *NewTemp();

private:
  int temp_id_ = 100;                   ///< Counter for temp IDs
  static TempFactory temp_factory;      ///< Singleton factory instance
};

/**
 * @brief A mapping from Temp* to register-name strings
 *
 * Used to print assembly instructions with human-readable register names.
 * Supports layering: LayerMap(over, under) looks up in `over` first, then
 * falls through to `under` if not found.
 *
 * The register manager populates a Map with physical register names.
 * After register allocation, the coloring map is layered on top to
 * translate virtual temps to their assigned physical registers.
 */
class Map {
public:
  /**
   * @brief Bind a temp to a register-name string
   * @param t The temp to bind
   * @param s Pointer to the register name string (e.g., "%rax")
   */
  void Enter(Temp *t, std::string *s);

  /**
   * @brief Look up the register name for a temp
   * @param t The temp to look up
   * @return Pointer to the name string, or nullptr if not found
   */
  std::string *Look(Temp *t);

  /** @brief Print all bindings in this map (for debugging) */
  void DumpMap(FILE *out);

  /** @brief Create an empty map with no bindings */
  static Map *Empty();

  /**
   * @brief Create a map that returns each temp's numeric name
   *
   * Returns strings like "t100", "t101", etc.  Used as a fallback
   * when no physical register has been assigned yet.
   */
  static Map *Name();

  /**
   * @brief Create a layered map: look up in `over` first, then `under`
   * @param over  Primary map (checked first)
   * @param under Fallback map (checked if not found in over)
   * @return A new Map that layers over on top of under
   */
  static Map *LayerMap(Map *over, Map *under);

private:
  tab::Table<Temp, std::string> *tab_; ///< Hash table of bindings
  Map *under_;                          ///< Fallback map (may be nullptr)

  Map() : tab_(new tab::Table<Temp, std::string>()), under_(nullptr) {}
  Map(tab::Table<Temp, std::string> *tab, Map *under)
      : tab_(tab), under_(under) {}
};

/**
 * @brief An ordered list of virtual registers with set operations
 *
 * Used throughout the back-end to represent:
 *   - Def sets: temporaries written by an instruction
 *   - Use sets: temporaries read by an instruction
 *   - Argument register lists (calling convention)
 *   - Callee-saved / caller-saved register sets
 *   - Live-in / live-out sets (liveness analysis)
 *
 * Set operations (Union, Diff, Contain) treat the list as a set
 * (no duplicates in results).
 */
class TempList {
public:
  explicit TempList(Temp *t) : temp_list_({t}) {}
  TempList(std::initializer_list<Temp *> list) : temp_list_(list) {}
  TempList() = default;

  /** @brief Append a temp to the end of the list */
  void Append(Temp *t) { temp_list_.push_back(t); }

  /** @brief Get the n-th temp (0-indexed) */
  [[nodiscard]] Temp *NthTemp(int i) const;

  /** @brief Get the underlying list */
  [[nodiscard]] const std::list<Temp *> &GetList() const { return temp_list_; }

  /** @brief Test whether temp @p t is in this list */
  bool Contain(Temp *t) const;

  /** @brief Append all elements of @p tl to this list (may create duplicates) */
  void CatList(const TempList *tl);

  /**
   * @brief Compute the set union of this list and @p tl
   * @return New TempList containing all elements of both, without duplicates
   */
  TempList *Union(const TempList *tl) const;

  /**
   * @brief Compute the set difference: this \ tl
   * @return New TempList containing elements in this but not in @p tl
   */
  TempList *Diff(const TempList *tl) const;

  /**
   * @brief Test whether this list and @p tl contain the same elements
   * @return true if both lists have the same set of temps
   */
  bool IdentitalTo(const TempList *tl) const;

  /**
   * @brief Replace the temp at position @p pos with @p temp
   * @param pos Iterator pointing to the temp to replace
   * @param temp The replacement temp
   * @return Iterator pointing to the newly inserted temp
   */
  std::list<Temp *>::const_iterator Replace(
    std::list<Temp *>::const_iterator pos, Temp *temp);

private:
  std::list<Temp *> temp_list_;
};

} // namespace temp

#endif // TIGER_FRAME_TEMP_H_
