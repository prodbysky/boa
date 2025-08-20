# **Boa**
Yeah so I'm making another language :DD

> [!CAUTION]
> This language is not at all finished and is subject to change really often

## Build
```bash
  gcc nob.c -o nob
  ./nob
  ./build/boa -help
```

## Supported targets
- Linux (via nasm)

## Code guidelines (be 'more formal')
 - Be as assertive as possible (via the `ASSERT` and `UNREACHABLE` macros in src/util.h)
 - Cover as MUCH code as possible via tests in the tests/ directory

## Features to be implemented until self-hosting
 - [X] Variables
 - [X] Control flow
 - [X] Functions
 - [X] Inline asm
 - [ ] Types
 - [ ] Arrays
 - [ ] Structs
