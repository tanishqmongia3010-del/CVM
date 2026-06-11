# CVM++ — Stack-Based Virtual Machine & Custom Compiler

> A lightweight scripting language built from scratch in C++. Raw source text goes in; bytecode executes on a custom stack-based VM. No libraries, no magic — every stage is hand-written.

---

## Table of Contents

- [What is CVM++?](#what-is-cvm)
- [Project Structure](#project-structure)
- [Pipeline Overview](#pipeline-overview)
- [Component Deep Dives](#component-deep-dives)
  - [Lexer](#1-lexerlexerhpp)
  - [Parser](#2-parserparserhpp)
  - [AST & Compiler](#3-ast--compiler-asthpp)
  - [Chunk](#4-bytecode-store-chunkhpp)
  - [Virtual Machine](#5-virtual-machine-vmhpp)
  - [Main / CLI](#6-entry-point-maincpp)
- [Instruction Set Architecture (ISA)](#instruction-set-architecture-isa)
- [Language Reference](#language-reference)
- [How to Build & Run](#how-to-build--run)
- [CLI Flags & Debug Modes](#cli-flags--debug-modes)
- [Optimisations & Design Decisions](#optimisations--design-decisions)

---

## What is CVM++?

Most developers use Python, JavaScript, or Java without ever understanding how text becomes execution. CVM++ tears that curtain down. It is a complete language implementation written in C++ with zero external dependencies:

```
source code  →  Lexer  →  Tokens  →  Parser  →  AST  →  Compiler  →  Bytecode  →  VM  →  output
```

The scripting language it implements supports integers, booleans, variables, arithmetic, comparisons, `if/else`, `while` loops, `print`, and `input` — enough to write real programs.

---

## Project Structure

```
CVM++/
├── headers/
│   ├── lexer.hpp       — Tokeniser: source string → Token stream
│   ├── parser.hpp      — Recursive-descent parser: tokens → AST
│   ├── ast.hpp         — AST node definitions + per-node compile()
│   ├── chunk.hpp       — Bytecode container (code[], constants[], variablenames[])
│   └── vm.hpp          — Stack-based execution engine
└── main.cpp            — CLI entry point, REPL, debug helpers
```

All five compiler stages live in header files so a single `#include` chain wires everything together with no separate build steps needed beyond compiling `main.cpp`.

---

## Pipeline Overview

```
┌─────────────┐     characters      ┌─────────────┐     tokens
│  Source Code │ ─────────────────► │    Lexer     │ ────────────────►
└─────────────┘                     └─────────────┘
                                                          │
                                                          ▼ tokens
                                                   ┌─────────────┐
                                                   │   Parser    │
                                                   └─────────────┘
                                                          │
                                                          ▼ AST (ProgramNode)
                                                   ┌─────────────┐
                                                   │  Compiler   │  (inside each AST node)
                                                   └─────────────┘
                                                          │
                                                          ▼ Chunk (bytecode + constants)
                                                   ┌─────────────┐
                                                   │     VM      │
                                                   └─────────────┘
                                                          │
                                                          ▼ stdout
```

---

## Component Deep Dives

### 1. Lexer — `lexer.hpp`

**Job:** Turn a raw source string into a flat stream of `Token` objects, one at a time.

**How it works:**

The `Lexer` class holds a cursor into the source string and a `current` character. Every call to `nextToken()` skips whitespace/comments, inspects `current`, and returns the matching `Token`.

```
"let x = 10 + 3;"
  ↓
LET  IDENTIFIER("x")  EQUALS  NUMBER("10")  PLUS  NUMBER("3")  SEMICOLON
```

**Key internals:**

| Method | Purpose |
|---|---|
| `advance()` | Move cursor forward one character; track line numbers |
| `peak(offset)` | Look ahead without consuming (used for `==`, `<=`, `>=`) |
| `skipwhitespace()` | Skip spaces, tabs, newlines, and `//` single-line comments |
| `readnumber()` | Consume digits and return a `NUMBER` token |
| `readword()` | Consume alphanumeric chars; check against keyword table (`let`, `if`, `else`, `while`, `input`, `print`) |

**Token types defined (`TokenType` enum):**
`NUMBER`, `IDENTIFIER`, `PLUS`, `MINUS`, `STAR`, `SLASH`, `EQUALS`, `DOUBLE_EQUALS`, `LESS_THAN`, `GREATER_THAN`, `LESS_THAN_EQUALS`, `GREATER_THAN_EQUALS`, `LET`, `IF`, `ELSE`, `WHILE`, `INPUT`, `PRINT`, `SEMICOLON`, `LEFT_PAREN`, `RIGHT_PAREN`, `LEFT_BRACE`, `RIGHT_BRACE`, `END_OF_FILE`, `ILLEGAL`

**Design choice — lazy tokenisation:** The lexer does not pre-tokenise the whole file into a vector. It produces one token per `nextToken()` call. The parser drives it on demand, which avoids allocating a large token list.

---

### 2. Parser — `parser.hpp`

**Job:** Consume the token stream and build an Abstract Syntax Tree (AST) that represents the program's structure.

**Technique:** Recursive Descent Parsing — each grammar rule becomes a C++ function. The call stack itself encodes operator precedence.

**Precedence staircase (lowest → highest priority):**

```
parsecomparison()   ==, <, >, <=, >=
    └─ parseexpression()    +, -
           └─ parseTerm()          *, /
                  └─ parsePrimary()       numbers, identifiers, (grouped expr), input
```

When you write `5 + 10 * 2`, `parseexpression` calls `parseTerm` for the right side, which grabs `10 * 2` first — that is how precedence is enforced without any precedence table.

**Statement parsers:**

| Function | Parses |
|---|---|
| `parsestatement()` | Dispatcher — routes to the right parser below |
| `parseif()` | `if (cond) { block } else { block }` |
| `parseWhile()` | `while (cond) { block }` |
| `parseblock()` | `{ statement* }` — returns a `BlockNode` |
| `parseAssignment()` | `identifier = expression ;` |
| `parseprogram()` | Top-level loop — collects statements until EOF |

**Error handling:** Every parser function checks that the expected token is present before consuming it. If not, it prints a descriptive error with the line number and returns `nullptr`, which causes `parseprogram()` to halt cleanly.

---

### 3. AST & Compiler — `ast.hpp`

**Job:** Define the node types that make up the tree, and give each node a `compile(Chunk*)` method that emits bytecode directly.

**Node hierarchy:**

```
ASTnode
├── expression
│   ├── numbernode          — a literal integer
│   ├── identifierNode      — a variable name (emits OP_GET_GLOBAL)
│   ├── operatornode        — binary op: left op right
│   └── InputExpr           — the `input` keyword (emits OP_INPUT)
└── statement
    ├── Letnode             — let x = expr ;
    ├── PrintNode           — print expr ;
    ├── AssignmentNode      — x = expr ;
    ├── BlockNode           — { stmt* }
    ├── IfNode              — if/else
    ├── WhileNode           — while loop
    └── ProgramNode         — root of the whole tree
```

**Compiler pattern — the Visitor trick without Visitor:**

Instead of a separate compiler class that visits nodes, every node compiles *itself*. `ProgramNode::compile()` calls `compile()` on each child statement, which calls `compile()` on its expressions, and so on. The bytecode is emitted in exactly the right order through this recursive descent.

**Jump patching for control flow:**

For `if` and `while`, the compiler uses a two-step process because it cannot know the jump distance until after compiling the body:

1. Emit the jump opcode with placeholder bytes `0xff 0xff`
2. Compile the body
3. Calculate the actual offset and write it back into the placeholder bytes

```cpp
// emit jump with placeholder
chunk->writechunk((uint8_t)OpCode::OP_JUMP_IF_FALSE);
chunk->writechunk(0xff);
chunk->writechunk(0xff);
int jumpIndex = chunk->code.size() - 2;  // remember where

// ... compile body ...

// patch the placeholder
int offset = chunk->code.size() - jumpIndex - 2;
chunk->code[jumpIndex]     = (offset >> 8) & 0xff;
chunk->code[jumpIndex + 1] =  offset       & 0xff;
```

Offsets are 16-bit (two bytes), supporting up to 65 535 bytes of jump distance.

**`>=` and `<=` implemented without extra opcodes:**

Rather than adding `OP_GREATER_EQUAL` and `OP_LESS_EQUAL`, the compiler reuses existing opcodes:

- `a >= b`  →  `NOT (a < b)`  →  `OP_LESS` then `OP_NOT`
- `a <= b`  →  `NOT (a > b)`  →  `OP_GREATER` then `OP_NOT`

This keeps the ISA small without losing functionality.

---

### 4. Bytecode Store — `chunk.hpp`

**Job:** A plain data container that the compiler writes into and the VM reads from.

```cpp
class Chunk {
    vector<uint8_t> code;          // raw instruction bytes
    vector<int>     constants;     // integer literal pool
    vector<string>  variablenames; // global variable name table
};
```

**`addconstant(int)`** — pushes a value into the constant pool and returns its index. The instruction stream stores only the index, keeping instructions compact.

**`addvariablename(string)`** — checks if the name already exists before adding. If `x` appears in ten places in the source, there is only one entry for it in the table. Variable resolution at runtime is just an array lookup by index.

**`writechunk(uint8_t)`** — appends one byte to the code stream.

**Why `uint8_t`?** Each opcode and operand fits in one byte. Using 8-bit integers means the bytecode is as dense as possible and array indexing is trivially fast.

---

### 5. Virtual Machine — `vm.hpp`

**Job:** Execute the bytecode by running a fetch-decode-execute loop over `chunk->code`.

**Core state:**

```cpp
int    ip;               // instruction pointer — index into code[]
int    stack[256];       // value stack
int    stacktop;         // index of next empty slot
map<string,int> globals; // variable storage
```

**Execution loop (`run()`):**

```
fetch:   instruction = code[ip++]
decode:  switch((OpCode)instruction)
execute: each case manipulates the stack or globals
```

**Stack discipline — every operation is push/pop:**

```
5 + 3

OP_CONSTANT 0   → push(5)      stack: [5]
OP_CONSTANT 1   → push(3)      stack: [5, 3]
OP_ADD          → a=pop(), b=pop(), push(b+a)   stack: [8]
```

**Control flow at the VM level:**

| Opcode | Effect |
|---|---|
| `OP_JUMP_IF_FALSE offset` | `if pop()==0: ip += offset` |
| `OP_JUMP offset` | `ip += offset` unconditionally |
| `OP_LOOP offset` | `ip -= offset` (jumps backward for while) |

**Variable opcodes:**

| Opcode | Effect |
|---|---|
| `OP_DEFINE_GLOBAL i` | `globals[name[i]] = pop()` — creates variable |
| `OP_GET_GLOBAL i` | `push(globals[name[i]])` — reads variable |
| `OP_SET_GLOBAL i` | `globals[name[i]] = pop()` — updates variable |

Runtime errors (undefined variable) print a message and `return` cleanly without crashing.

---

### 6. Entry Point — `main.cpp`

`main.cpp` is the user-facing shell around all five compiler stages. It has four responsibilities:

**`read_file(path)`** — Opens a `.cvm` file, reads it character by character into a `stringstream`, and returns the full source as a `string`. Exits with an error message if the file cannot be opened.

**`print_tokens(source)`** — Creates a fresh `Lexer`, calls `nextToken()` in a loop, and prints each token with its line number. Used for the `--tokens` debug flag.

**`print_bytecode(chunk)`** — Walks `chunk->code` with a manual index (because instructions have variable byte widths) and pretty-prints every opcode along with its operands. For `OP_CONSTANT` it also shows the actual constant value. Used for the `--bytecode` debug flag.

**`run_source(source, show_tokens, show_AST, show_bytecode)`** — The full pipeline in one function: lex (optionally), parse, optionally print AST, compile, optionally print bytecode, then run on the VM.

**`run_REPL()`** — Starts an interactive prompt. One `VM` instance is created outside the loop so that **globals persist across lines** — you can `let x = 5;` on one line and `print x;` on the next. Each line is parsed and compiled into a fresh `Chunk` and then interpreted.

**`main(argc, argv)`** — Argument routing:
- No args → REPL
- `<file.cvm>` → run file, with optional `--tokens`, `--ast`, `--bytecode`, or `--all` flags

---

## Instruction Set Architecture (ISA)

| Opcode | Bytes | Description |
|---|---|---|
| `OP_CONSTANT i` | 2 | Push `constants[i]` onto stack |
| `OP_ADD` | 1 | Pop two, push sum |
| `OP_SUBTRACT` | 1 | Pop two, push difference |
| `OP_MULTIPLY` | 1 | Pop two, push product |
| `OP_DIVIDE` | 1 | Pop two, push quotient |
| `OP_EQUAL` | 1 | Pop two, push 1 if equal else 0 |
| `OP_GREATER` | 1 | Pop two, push 1 if left > right |
| `OP_LESS` | 1 | Pop two, push 1 if left < right |
| `OP_NOT` | 1 | Pop one, push its boolean inverse |
| `OP_DEFINE_GLOBAL i` | 2 | Pop value, store in `globals[names[i]]` |
| `OP_GET_GLOBAL i` | 2 | Push `globals[names[i]]` |
| `OP_SET_GLOBAL i` | 2 | Pop value, update `globals[names[i]]` |
| `OP_PRINT` | 1 | Pop and print to stdout |
| `OP_INPUT` | 1 | Read integer from stdin, push it |
| `OP_JUMP_IF_FALSE hi lo` | 3 | If top==0, `ip += (hi<<8)\|lo` |
| `OP_JUMP hi lo` | 3 | `ip += (hi<<8)\|lo` |
| `OP_LOOP hi lo` | 3 | `ip -= (hi<<8)\|lo` |
| `OP_RETURN` | 1 | Halt execution |

---

## Language Reference

```
// Variables
let x = 10;
let result = x * 2 + 5;

// Reassignment
x = x + 1;

// Print
print x;
print 3 + 4;

// Input
let n = input;

// Comparisons  (produce 0 or 1)
let flag = x == 10;
let big  = x > 5;

// If / Else
if (x > 5) {
    print x;
} else {
    print 0;
}

// While loop
while (x < 100) {
    x = x + 1;
}

// Comments
// this is a comment
```

**Types:** Integers only (all values on the stack are `int`). Booleans are represented as `0` (false) and `1` (true).

---

## How to Build & Run

```bash
# Compile
g++ -std=c++17 -o cvm main.cpp

# Run a script
./cvm script.cvm

# Interactive REPL
./cvm

# Debug modes
./cvm script.cvm --tokens    # show token stream
./cvm script.cvm --ast       # show AST
./cvm script.cvm --bytecode  # show compiled bytecode
./cvm script.cvm --all       # show everything
```

---

## CLI Flags & Debug Modes

The `--all` flag is invaluable for understanding the full pipeline. For a script `let x = 5; print x;` you will see:

```
--- TOKENS ---
  [line 0] let
  [line 0] x
  [line 0] =
  [line 0] 5
  ...

--- AST ---
--- PROGRAM START ---
(LET x = 5)
(PRINT x)
--- PROGRAM END ---

--- BYTECODE ---
  Constants:  [0]=5
  Variables:  [0]=x
  Instructions:
    0: OP_CONSTANT      index=0  (value=5)
    2: OP_DEFINE_GLOBAL 'x'
    4: OP_GET_GLOBAL    'x'
    6: OP_PRINT
    7: OP_RETURN

5
```

---


---

## Optimisations & Design Decisions

**1. Lazy tokenisation** — The lexer produces tokens on demand rather than pre-building a vector. This avoids an O(n) allocation pass before parsing even begins.

**2. Deduplicating variable name table** — `addvariablename()` scans the existing list before inserting. A variable used 50 times in a program takes exactly one entry, making every variable access an O(1) array index.

**3. `>=` / `<=` via opcode reuse** — Two operators for the price of zero extra opcodes. `a >= b` compiles to `OP_LESS` + `OP_NOT`. Keeps the ISA minimal and the VM switch-statement lean.

**4. 16-bit jump offsets** — Jump distances are stored as two bytes (`hi` and `lo`), patched after the body is compiled. This supports programs up to 65 535 bytes of bytecode — far beyond what this language needs — at essentially no cost.

**5. `unique_ptr` throughout the AST** — All child node ownership is managed by `unique_ptr`. When a `ProgramNode` is destroyed, the entire tree is freed automatically with no manual `delete` anywhere. The `move()` pattern is used at every ownership transfer to make this explicit.

**6. REPL with persistent globals** — A single `VM` instance lives outside the REPL loop. Each new line gets a fresh `Chunk` and `Parser`, but the `globals` map in the VM accumulates state across lines, so multi-line sessions work naturally.

**7. Self-compiling AST nodes (distributed compiler)** — Rather than a monolithic compiler class that pattern-matches on node types, each node knows how to compile itself via `virtual void compile(Chunk*)`. Adding a new node type only requires implementing its own `compile()` — no other file changes needed.

*Built from scratch — no interpreter libraries, no regex, no parser generators. Just C++, the stack, and arithmetic.*  
