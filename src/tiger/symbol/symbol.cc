/**
 * @file symbol.cc
 * @brief Implementation of symbol interning
 * 
 * Implements the symbol interning hash table. Uses a simple hash table
 * with chaining to store unique symbols.
 */

#include "tiger/symbol/symbol.h"

namespace {

constexpr unsigned int HASH_TABSIZE = 109;  ///< Hash table size (prime number)
sym::Symbol *hashtable[HASH_TABSIZE];        ///< Hash table for symbol interning

/**
 * @brief Hash function for symbol names
 * @param str String to hash
 * @return Hash value
 * 
 * Uses a simple multiplicative hash (65599 is a common choice).
 */
unsigned int Hash(std::string_view str) {
  unsigned int h = 0;
  for (const char *s = str.data(); *s; s++)
    h = h * 65599 + *s;
  return h;
}

} // namespace

namespace sym {

Symbol *Symbol::UniqueSymbol(std::string_view name) {
  // Compute hash bucket index
  unsigned int index = Hash(name) % HASH_TABSIZE;
  Symbol *syms = hashtable[index], *sym;
  
  // Search for existing symbol in hash bucket
  for (sym = syms; sym; sym = sym->next_)
    if (sym->name_ == name)
      return sym;
  
  // Not found: create new symbol and add to front of bucket
  sym = new Symbol(static_cast<std::string>(name), syms);
  hashtable[index] = sym;
  return sym;
}

} // namespace sym
