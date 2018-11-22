# Statically Typed Embeddable LAnguage

[![Build Status](https://travis-ci.org/Kerndog73/STELA.svg?branch=master)](https://travis-ci.org/Kerndog73/STELA) [![Coverage Status](https://coveralls.io/repos/github/Kerndog73/STELA/badge.svg?branch=master)](https://coveralls.io/github/Kerndog73/STELA?branch=master) [![Codacy Badge](https://api.codacy.com/project/badge/Grade/9a5be676e21c47c09c0ee3aed1e65bd5)](https://www.codacy.com/app/kerndog73/STELA?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=Kerndog73/STELA&amp;utm_campaign=Badge_Grade) [![Lines of Code](https://tokei.rs/b1/github/Kerndog73/STELA)](https://github.com/Aaronepower/tokei)

A scripting language built for speed in world where JavaScript runs on web servers.

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

### Examples

The semantic analyser is nearing completion as I write this. These are a few examples taken from the [semantic tests](test/src/semantics.cpp) that illistrate (with code) the semantics of the language. You can't actually run any of these examples but you can put them through the semantic analyser and not get any errors.

#### Get a pointer to function

Just like in C++, if you want a pointer to an overloaded function, you need to select which overload you want.

```
func add(a: sint, b: sint) -> sint {
  return a + b;
}
func add(a: uint, b: uint) -> uint {
  return a + b;
}
func add(a: real, b: real) -> real {
  return a + b;
}

func sub(a: sint, b: sint) -> sint {
  return a - b;
}

// Need to provide signature to select overloaded function
let add_ptr0: func(real, real) -> real = add;
let add_ptr1 = make func(sint, sint) -> sint add;

// Signature is optional for non-overloaded functions
let sub_ptr = sub;
```

#### Tag dispatch

This is example shows strong type aliases and overloading again.

```
type first_t struct{};
type second_t struct{};
type third_t struct{};

let first: first_t = {};
let second: second_t = {};
let third: third_t = {};

// This is a little bit silly but you get the idea
func get(t: first_t, arr: [sint]) -> sint {
  return arr[0];
}
func get(t: second_t, arr: [sint]) -> sint {
  return arr[1];
}
func get(t: third_t, arr: [sint]) -> sint {
  return arr[2];
}

func test() {
  let arr = [5, 2, 6];
  let two = get(second, arr);
  let five = get(first, arr);
  let six = get(third, arr);
}
```

#### Member functions

Member functions can be created for any type, including builtin types!

```
// Just to demonstrate weak type aliases too
type Integer = sint;

func (self: Integer) half() -> Integer {
  return self / 2;
}

func test() {
  let fourteen = 14;
  let seven = fourteen.half();
}
```

#### Enums

Enums aren't supported natively simply because I don't find the following example too inconvienient.

```
type Dir sint;
// Dir is a strong alias of sint
// You need to explicitly cast the sint literal to a Dir
let Dir_up    = make Dir 0;
let Dir_right = make Dir 1;
let Dir_down  = make Dir 2;
let Dir_left  = make Dir 3;
```

#### Modules

Modules are pretty cool. So are 2D vectors.

```
module glm;

type vec2 struct {
  x: real;
  y: real;
};

func plus(a: vec2, b: vec2) -> vec2 {
  return make vec2 {a.x + b.x, a.y + b.y};
}

// We don't have a builtin sqrt function yet so this will have to do
func magnitude2(v: vec2) -> real {
  return v.x * v.x + v.y * v.y;
}
```

Other module...

```
// Optional
// module main;

import glm;

func main() {
  let one_two = make glm::vec2 {1.0, 2.0};
  let three_four = make glm::vec2 {3.0, 4.0};
  let four_six = add(one_two, three_four);
  let five = magnitude2(one_two);
}
```

### Goals (Wrote this months ago!)
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
   * I see something quite wrong with the standard library being part of the language. The standard library should be implemented in the language itself.
   * All of the necessary tools for implementing the standard library are provided without exposing dangerous things like pointers or manual memory management.

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
