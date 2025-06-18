# Tiger Compiler

A complete C++ implementation of a compiler for the Tiger programming language, built for educational purposes following Andrew W. Appel's compiler textbook.

## What is Tiger?

Tiger is a simple, statically-typed programming language with:
- Basic types: integers and strings
- Composite types: arrays and records  
- Functions with local variable scoping
- Control flow: if/then/else, while, for loops
- Let expressions for local declarations

## Quick Start

### Prerequisites
- CMake 3.2+
- C++17 compatible compiler (GCC/Clang)
- Flex (lexical analyzer generator)
- Bison (parser generator)

### Build
```bash
mkdir build && cd build
cmake ..
make
```

### Usage
```bash
# Compile a Tiger program to x64 assembly
./tiger-compiler input.tig

# Run individual compiler phases for testing
./test_lex input.tig      # Lexical analysis
./test_parse input.tig    # Syntax analysis  
./test_semant input.tig   # Semantic analysis
./test_translate input.tig # IR translation
./test_codegen input.tig  # Code generation
```

## Example Tiger Program

```tiger
let 
    function factorial(n: int): int =
        if n <= 1 then 1 
        else n * factorial(n-1)
in
    print_int(factorial(10))
end
```

## Project Structure

- `src/tiger/` - Main compiler implementation
- `src/straightline/` - Lab 1 warm-up exercises
- `testdata/` - Test cases organized by lab
- `docs/` - Detailed documentation and language reference

## Testing

Run the regression test suite:
```bash
./regression.sh
```

## Documentation

- [Tiger Language Reference](docs/tiger-language.md) - Complete language specification
- [Architecture Overview](docs/architecture.md) - Compiler design details
- [Testing Guide](docs/testing.md) - How to run and debug tests

---

*This project implements a complete compilation pipeline from Tiger source code to executable x64 assembly, demonstrating all major phases of compiler construction.*
