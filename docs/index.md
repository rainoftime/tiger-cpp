# Tiger Compiler Documentation

A C++ implementation of a compiler for the Tiger programming language, based on Andrew W. Appel's "Modern Compiler Implementation in C".

## Quick Start

```bash
mkdir build && cd build && cmake .. && make
./tiger-compiler ../testdata/lab5or6/testcases/queens.tig
```

## Documentation

- [Overview](overview.md) - Project introduction and compilation pipeline
- [Tiger Language](tiger-language.md) - Language syntax and semantics
- [Architecture](architecture.md) - Implementation details
- [Testing](testing.md) - Running and debugging tests

## Project Structure

```
src/tiger/
├── lex/         # Lexical analysis
├── parse/       # Syntax analysis  
├── semant/      # Semantic analysis
├── translate/   # IR translation
├── codegen/     # Code generation
└── regalloc/    # Register allocation
```

## Lab Sequence

1. **Lab 1**: Straightline programs
2. **Lab 2**: Lexical analysis
3. **Lab 3**: Parsing
4. **Lab 4**: Semantic analysis
5. **Lab 5**: IR translation and code generation
6. **Lab 6**: Register allocation

## Key Features

- **Complete Tiger Language Support**: Full implementation of the Tiger language specification
- **Multi-Stage Compilation**: Lexical analysis, parsing, semantic analysis, IR generation, and code generation
- **X64 Target**: Generates assembly code for x64 architecture
- **Modern C++**: Uses C++17 features, CMake build system, and modern C++ practices
- **Comprehensive Testing**: Extensive test suite with reference outputs
- **Educational Focus**: Designed for teaching compiler construction concepts

## Learning Path

If you're new to compiler construction, follow this learning path:

1. Start with [Project Overview](overview.md) to understand the big picture
2. Read [Tiger Language Reference](tiger-language.md) to learn the language
3. Follow the [Lab Assignments](labs/README.md) in order:
   - Lab 1: Straightline Programs
   - Lab 2: Lexical Analysis
   - Lab 3: Parsing
   - Lab 4: Semantic Analysis
   - Lab 5: IR Translation and Code Generation
   - Lab 6: Register Allocation
4. Consult [Architecture Guide](architecture.md) for implementation details
- Use [API Reference](api/README.md) for specific class and function documentation

## Getting Help

- Check the [Testing Guide](testing.md) if you're having issues with test cases
- Review [Development Guide](development.md) for coding standards and best practices
- Look at existing test cases in `testdata/` for examples
- Consult the original textbook "Modern Compiler Implementation in C"

---

*This documentation is maintained as part of the Tiger Compiler educational project.* 