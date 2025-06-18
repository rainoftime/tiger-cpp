# Project Overview

Tiger Compiler is a complete C++ implementation of a compiler for the Tiger programming language, designed for educational purposes based on Andrew W. Appel's textbook.

## Design Goals

- **Educational**: Step-by-step labs building compiler knowledge
- **Modern C++**: Type safety, RAII, smart pointers, C++17 features
- **Real Output**: Generates actual x64 assembly code

## Compilation Pipeline

```
Tiger Source (.tig) → Lexer → Parser → Semantic Analysis → 
Escape Analysis → IR Translation → Canonicalization → 
Code Generation → Register Allocation → x64 Assembly (.s)
```

## Core Phases

1. **Lexical Analysis** - Tokenization using Flex
2. **Syntax Analysis** - AST construction using Bison  
3. **Semantic Analysis** - Type checking and symbol tables
4. **Escape Analysis** - Variable storage optimization
5. **IR Translation** - Convert AST to intermediate representation
6. **Canonicalization** - Simplify IR trees
7. **Code Generation** - IR to assembly instructions
8. **Register Allocation** - Graph coloring algorithm

## Dependencies

- **Build**: CMake 3.2+, C++17 compiler, Flex, Bison
- **Runtime**: C standard library, GNU assembler/linker

## Testing

- **Unit Tests**: `test_lex`, `test_parse`, `test_semant`, `test_translate`, `test_codegen`
- **Integration**: End-to-end compilation with reference outputs
- **Test Data**: Organized by lab in `testdata/` directory

## Getting Started

1. Read [Tiger Language Reference](tiger-language.md)
2. Follow labs sequentially starting with Lab 1
3. Use [Architecture Guide](architecture.md) for implementation details
4. Consult [Testing Guide](testing.md) for debugging

---

*This overview provides the foundation for understanding the Tiger Compiler project. Each subsequent document builds upon these concepts.* 