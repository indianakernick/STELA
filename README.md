# Statically Typed Embeddable LAnguage

[![Build Status](https://travis-ci.org/Kerndog73/STELA.svg?branch=master)](https://travis-ci.org/Kerndog73/STELA) [![Coverage Status](https://coveralls.io/repos/github/Kerndog73/STELA/badge.svg?branch=master)](https://coveralls.io/github/Kerndog73/STELA?branch=master) [![Codacy Badge](https://api.codacy.com/project/badge/Grade/9a5be676e21c47c09c0ee3aed1e65bd5)](https://www.codacy.com/app/kerndog73/STELA?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=Kerndog73/STELA&amp;utm_campaign=Badge_Grade) [![Lines of Code](https://tokei.rs/b1/github/Kerndog73/STELA)](https://github.com/Aaronepower/tokei)

A scripting language built for speed in world where JavaScript runs on web servers.

__If a compilers expert is reading this, I would love some advice!__

## Progress
 * The lexical analyser is done.
 * The syntax analyser is done.
 * The semantic analyser can:
   * check the type of variables
   * check types of assignments and expressions
   * lookup overloaded functions
   * lookup struct members and functions
   * deal with statements (if, while, for, ...)
   * handle arrays (literals, subscripting)
   * handle modules
   * initialize structs
   * cast between compatible types
   * check that the return expression matches return type
    
   Check out the [semantic tests](test/src/semantics.cpp). There's still a lot more to do:
    
   * `expected bool expression but got byte expression`
   * handle lambdas and function pointers to overloaded functions
   * raise warnings about unused symbols (`unused variable "myVar"`)
   * provide array operations (`size`, `push_back`, `pop_back`)
   * provide standard math functions (`sin`, `log`, `sqrt`)
   * do we need array slices? String literal should be a slice
   * read notes.txt about interfaces and generics
   * do we need interfaces? A lambda is an interface with a single function.
   * do we need generics? If we do decide to implement generics then interfaces should probably act like Swift prototypes
   * there's probably a lot of stuff that I can't think of right now!

## About

### Goals
 * __Statically typed__
   * Type inference is used as much as possible. If the type can be infered unambiguously, then the type will be infered. That's the type inference promise!
   * Naturally makes the language a little faster. 
   * Usually makes code easier to understand. 
   * More errors can be caught at compile time, like `"a string" + 5`.
   * Function overloading, real classes that aren't just maps, type safety, awesome stuff!
 * __Embeddable__
   * A C++ API is exposed for embedding the language into a C++ application. I think that there is something wrong with exposing a C interface and then having to write a C++ wrapper. Sol2 for Lua, Pybind for Python, etc.
   * A C API that wraps the C++ API is also exposed so you get the best of both words!
   * All steps of the compilation are exposed (lexer tokens, AST, symbol table, bytecode) but if you just want to `compile(source)` and `run(bytecode)` then that's all you have to do.
   * Reflection facilities are built-in so binding your C++ classes to STELA couldn't be easier.
 * __Minimal__
   * I see something quite wrong with the standard library being part of the language. The standard library should be implemented in the language itself with a few compiler hooks where necessary. For example, `std::initilizer_list` can't quite be implemented in pure C++. It needs a little help from the compiler.
   * All of the necessary tools for implementing the standard library are provided without exposing dangerous things like pointers or manual memory management.
   * A simple language that isn't interwoven with a standard library is a bit easier to parse.

This language is sounding quite ambitious! There probably won't be a working prototype until the end of the year.

### File Extension
`.stela` or `.ste` (if the filesystem limits extensions to three characters).

### Building

This project depends on [Simpleton](https://github.com/Kerndog73/Simpleton-Engine) as a build-time dependency. CMake will automatically download Simpleton so you don't have to worry about it.

```bash
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

This will build a static library, a command-line tool and a test suite. Optionally, you may install these.

```bash
# might want to run the tests before installing
test/suite
make install
```

This will install `lib/libSTELA.a`, `include/STELA` and `bin/stela`.
