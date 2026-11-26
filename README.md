# IFJ26 Compiler

Compiler for IFJ26 to IFJ26Code developed as a part of the IFJ and IAL courses
at FIT VUT during the winter semester of 2026. The symbol table is implemented
as a self balancing binary search tree.

## Authors

- David Alois Bureš (xburesd00): FSM, Lexer
- David Daniel Šlapal (xslapad00): Binary search tree, codegen
- Jan Kostka (xkostkj00): Optimizer, Lowering, CI/CD
- Lukáš Pollák (xpollal00): Parser, Syntactic and semantic analysis

## Implemented extensions

FUNEXP, BOOLTHEN, CONTROL, VALMEM, INTERPOLATEDS, LISTS

## Usage

Prerequisites: clang, make\
Platforms: Windows, Linux, MacOS

> [!NOTE]
> In the submitted archive, the scripts folder is moved to the tests folder.
> Some scripts correctly adapt to this (most testing scripts), but some do not
> (utility scripts). For best results, move the scripts folder back to the root.

```sh
# 1. Open the project root directory

# 2. Build the compiler
make

# 3. Compile ./source (IFJ26) to ./output (IFJ26Code)

# Windows
Get-Content source | ./ifj26 > output

# Linux/MacOS
./ifj26 < source > output
```
