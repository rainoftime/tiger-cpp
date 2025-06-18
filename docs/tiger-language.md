# Tiger Language Reference

## Lexical Elements

### Comments
```tiger
/* Single line comment */
/* Multi-line
   comment */
/* Nested /* comments */ allowed */
```

### Identifiers
Letters, digits, underscores. Case-sensitive. Start with letter.

### Literals
```tiger
42          /* integers */
"hello"     /* strings with escapes: \n \t \" \\ \^C \ddd */
```

### Keywords
```
array break do else end for function if in let nil of then to type var while
```

### Operators
```
:= + - * / = <> < <= > >= & | ( ) [ ] { } , : ; .
```

## Types

### Basic Types
- `int` - signed integers
- `string` - immutable character sequences

### Arrays
```tiger
type intArray = array of int
var arr := intArray[10] of 0    /* 10 elements, initialized to 0 */
arr[0] := 42                    /* array access */
```

### Records
```tiger
type point = {x: int, y: int}
var p := point{x=10, y=20}      /* record creation */
p.x := 100                      /* field access */
```

### Type Aliases
```tiger
type distance = int
type intlist = {head: int, tail: intlist}  /* recursive types */
```

## Variables

```tiger
var x := 42                     /* type inference */
var name: string := "Alice"     /* explicit type */
```

## Expressions

### Arithmetic
```tiger
x + y  x - y  x * y  x / y  -x
```

### Comparison  
```tiger
x = y  x <> y  x < y  x <= y  x > y  x >= y
```

### Logical (short-circuiting)
```tiger
cond1 & cond2    /* AND */
cond1 | cond2    /* OR */
```

## Control Flow

### Conditionals
```tiger
if x > 0 then print("positive")
if x > 0 then print("pos") else print("non-pos")
```

### Loops
```tiger
while x > 0 do (print_int(x); x := x - 1)
for i := 1 to 10 do print_int(i)
```

### Break
```tiger
while 1 do (if done then break; process())
```

## Functions

```tiger
function add(x: int, y: int): int = x + y
function greet(name: string) = print(name)     /* procedure (no return) */

/* Mutually recursive functions */
function even(n: int): int = if n = 0 then 1 else odd(n-1)
function odd(n: int): int = if n = 0 then 0 else even(n-1)
```

## Let Expressions

```tiger
let 
    type point = {x: int, y: int}
    var origin := point{x=0, y=0}
    function distance(p: point): int = 
        sqrt(p.x * p.x + p.y * p.y)
in
    distance(origin)
end
```

## Arrays and Records

### Array Operations
```tiger
let type intArray = array of int
    var arr := intArray[10] of 0
in
    arr[5] := 42;
    arr[5]
end
```

### Record Operations
```tiger
let type person = {name: string, age: int}
    var p := person{name="John", age=25}
in
    p.age := p.age + 1;
    p.name
end
```

## Standard Library

```tiger
print(s: string)           /* print string */
print_int(i: int)          /* print integer */
flush()                    /* flush output */
getchar(): string          /* read character */
ord(s: string): int        /* ASCII value */
chr(i: int): string        /* character from ASCII */
size(s: string): int       /* string length */
substring(s: string, start: int, len: int): string
concat(s1: string, s2: string): string
not(i: int): int           /* logical not */
exit(i: int)               /* exit program */
```

## Semantics

### Type Checking
- Static type checking with type inference
- `nil` belongs to any record type
- Array/record types are structural (name equivalence)

### Scoping
- Let expressions create new scopes
- Functions can access variables from enclosing scopes
- Mutually recursive functions/types allowed in same declaration group

### Evaluation Order
- Function arguments evaluated left to right
- Short-circuit evaluation for `&` and `|`
- Assignment returns no value (`void`)

## Grammar Summary

Here's a simplified grammar for Tiger:

```
program     → exp

exp         → lvalue
           | nil
           | int
           | string
           | - exp
           | exp binop exp
           | lvalue := exp
           | id ( explist )
           | ( expseq )
           | type-id { fieldlist }
           | type-id [ exp ] of exp
           | if exp then exp
           | if exp then exp else exp
           | while exp do exp
           | for id := exp to exp do exp
           | break
           | let declist in expseq end

lvalue      → id
           | lvalue . id
           | lvalue [ exp ]

binop       → + | - | * | / | = | <> | < | <= | > | >= | & | |

explist     → exp { , exp }
expseq      → exp { ; exp }

declist     → { dec }

dec         → type id = type
           | var id := exp
           | var id : type-id := exp
           | function id ( fieldlist ) = exp
           | function id ( fieldlist ) : type-id = exp

type        → type-id
           | { fieldlist }
           | array of type-id

fieldlist   → id : type-id { , id : type-id }
```

## Language Limitations

- No floating-point numbers
- No pointers (except implicit record/array references)
- No separate compilation/modules
- No exception handling
- Limited standard library
- No garbage collection (implementation-dependent)

---

*This reference covers the complete Tiger language as implemented in the Tiger Compiler project.* 