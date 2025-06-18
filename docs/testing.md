# Testing Guide

## Test Structure

```
testdata/
├── lab2/           # Lexical analysis
├── lab3/           # Parsing  
├── lab4/           # Semantic analysis
└── lab5or6/        # Code generation
    ├── testcases/  # Input .tig files
    ├── refs/       # Expected execution output
    └── refs-part1/ # Expected IR translation output
```

## Running Tests

### Unit Tests
```bash
# Build test executables
make test_lex test_parse test_semant test_translate test_codegen

# Run specific phase tests
./test_lex ../testdata/lab2/testcases/test1.tig
./test_parse ../testdata/lab3/testcases/queens.tig  
./test_semant ../testdata/lab4/testcases/merge.tig
```

### Automated Testing
```bash
# Run all tests for a lab
./scripts/grade.sh lab2
./scripts/grade.sh lab3
./scripts/grade.sh lab4
./scripts/grade.sh lab5

# Run regression tests
./regression.sh
```

## Test Categories

- **lab2**: Token recognition, string/comment handling
- **lab3**: Grammar parsing, AST construction
- **lab4**: Type checking, semantic errors
- **lab5or6**: IR translation, code generation, execution

## Debugging

### Common Issues
- **Lexer**: Check token boundaries, string escapes
- **Parser**: Verify grammar rules, precedence
- **Semantic**: Type mismatches, undefined symbols
- **Codegen**: Register usage, calling conventions

### Debug Output
```bash
# Enable debug output
export TIGER_DEBUG=1
./test_parse input.tig

# Compare with reference
diff output.txt expected.txt
```

## Test File Format

### Input Files (`.tig`)
Standard Tiger programs testing specific language features.

### Reference Files (`.out`)
- **Labs 2-4**: Compiler output (tokens, AST, errors)
- **Lab 5-6**: Program execution results

