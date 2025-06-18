# Tiger Compiler Architecture

## Module Structure

```
src/tiger/
├── absyn/      # Abstract syntax tree
├── canon/      # IR canonicalization  
├── codegen/    # Code generation
├── env/        # Symbol environments
├── frame/      # Runtime frames
├── lex/        # Lexical analysis
├── parse/      # Syntax analysis
├── regalloc/   # Register allocation
├── semant/     # Semantic analysis
├── symbol/     # Symbol tables
└── translate/  # IR translation
```

## Key Data Structures

### Abstract Syntax Tree
```cpp
namespace absyn {
    class Exp : public Absyn {};
    class IntExp : public Exp { int val_; };
    class VarExp : public Exp { Var* var_; };
    class CallExp : public Exp { Symbol* func_; ExpList* args_; };
    
    class Dec : public Absyn {};
    class VarDec : public Dec { Symbol* var_; Ty* typ_; Exp* init_; };
    class FunDec : public Dec { Symbol* name_; FieldList* params_; };
}
```

### Symbol Management
```cpp
namespace env {
    class TEnv { sym::Table<sem::Ty>* tenv_; };      // Types
    class VEnv { sym::Table<EnvEntry>* venv_; };     // Variables/functions
}
```

### Intermediate Representation
```cpp
namespace tree {
    class Stm {};
    class MoveStm : public Stm { Exp* dst_; Exp* src_; };
    class ExpStm : public Stm { Exp* exp_; };
    
    class Exp {};
    class TempExp : public Exp { temp::Temp* temp_; };
    class BinopExp : public Exp { BinOp op_; Exp* left_; Exp* right_; };
}
```

## Compilation Phases

### 1. Lexical Analysis (`lex/`)
- **Input**: Tiger source code
- **Output**: Token stream
- **Implementation**: Flex specification in `tiger.lex`
- **Features**: Nested comments, string escapes, position tracking

### 2. Syntax Analysis (`parse/`)
- **Input**: Token stream  
- **Output**: Abstract syntax tree
- **Implementation**: Bison grammar in `tiger.y`
- **Features**: Error recovery, precedence handling

### 3. Semantic Analysis (`semant/`)
- **Input**: AST
- **Output**: Type-checked AST
- **Key Classes**: `ProgSem`, type hierarchy in `types.h`
- **Features**: Type checking, symbol resolution, mutually recursive definitions

### 4. Escape Analysis (`escape/`)
- **Purpose**: Determine which variables need frame storage
- **Algorithm**: Mark variables accessed from nested functions

### 5. IR Translation (`translate/`)
- **Input**: Type-checked AST
- **Output**: IR tree
- **Key Concepts**: Expression vs statement translation, frame management

### 6. Canonicalization (`canon/`)
- **Purpose**: Simplify IR trees for code generation
- **Transformations**: Remove ESEQ nodes, linearize statements

### 7. Code Generation (`codegen/`)
- **Input**: Canonical IR
- **Output**: Assembly instructions
- **Algorithm**: Maximal munch instruction selection

### 8. Register Allocation (`regalloc/`)
- **Algorithm**: Graph coloring with spilling
- **Components**: Liveness analysis, interference graph, coloring

## Key Algorithms

### Type Checking
1. Initialize built-in types (`int`, `string`)
2. Process type declarations (handle cycles)
3. Check expressions recursively
4. Unify types and report errors

### Frame Management
- **X64Frame**: Manages stack frames, calling conventions
- **Access**: Describes variable locations (register vs memory)
- **Fragments**: Code and data fragments for linking

### Register Allocation
1. **Liveness Analysis**: Determine variable lifetimes
2. **Interference Graph**: Variables that can't share registers  
3. **Graph Coloring**: Assign registers, spill if needed
4. **Code Rewriting**: Insert spill/reload instructions

## Error Handling

- **Position Tracking**: Line/column information throughout pipeline
- **Error Recovery**: Continue compilation after errors when possible
- **Reporting**: Centralized error message system in `errormsg/` 