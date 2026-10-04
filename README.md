# IFJ26 Compiler

Compiler for IFJ26 to IFJ26Code developed as a part of the IFJ and IAL courses
at FIT VUT during the winter semester of 2026. The symbol table is implemented
as a self balancing binary search tree.

## Authors

- David Alois Bureš (xburesd00): Lexer
- David Daniel Šlapal (xslapad00): Binary search tree, codegen
- Jan Kostka (xkostkj00): Optimizer, CI/CD
- Lukáš Pollák (xpollal00): Parser

## Usage

Prerequisites: clang, make\
Platforms: Windows, Linux, MacOS

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
