# Statically Typed Embeddable LAnguage

[![Build Status](https://travis-ci.org/Kerndog73/STELA.svg?branch=master)](https://travis-ci.org/Kerndog73/STELA) [![Coverage Status](https://img.shields.io/coveralls/github/Kerndog73/STELA.svg)](https://coveralls.io/github/Kerndog73/STELA)

A scripting language with all of the benefits of static typing, without the verbosity of type names.

## Progress
The lexical analyser is done. The syntax analyser is about halfway done. I'm hoping to start semantic analysis some time in July.

## About

### Goals
 * Statically typed
   * Naturally makes the language a little faster. 
   * Usually makes code easier to understand. 
   * More errors can be caught at compile time, like `"a string" + 5`.
   * Function overloading, real classes that aren't just maps, type safety, awesome stuff.
   * Type inference is used as much as possible. If the type can be infered unambiguously, then the type will be infered. That's the type inference promise! 
 * Embeddable
   * A C++ API is exposed for embedding the language into a C++ application. I think that there is something wrong with exposing a C interface and then having write a C++ wrapper. Sol2 for Lua, Pybind for Python, etc.
   * A C API that wraps the C++ API is also exposed so you get the best of both words!
   * All steps of the compilation are exposed (lexer tokens, AST, bytecode) but if you just want to `compile(source)` and `run(bytecode)` then that's all you have to do.
   * Reflection facilities are built-in so binding your C++ classes to STELA couldn't be easier.
 * Minimal
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
cmake ..
make
```

This will build a static library, a command-line tool and a test suite. Optionally, you may install these.

```bash
# might want to run the tests before installing
test/suite
make install
```

This will install `lib/libSTELA.a`, `include/STELA` and `bin/stela`.
