# Statically Typed Embeddable LAnguage

[![Build Status](https://travis-ci.org/Kerndog73/STELA.svg?branch=master)](https://travis-ci.org/Kerndog73/STELA) [![Coverage Status](https://coveralls.io/repos/github/Kerndog73/STELA/badge.svg?branch=master)](https://coveralls.io/github/Kerndog73/STELA?branch=master) [![Codacy Badge](https://api.codacy.com/project/badge/Grade/9a5be676e21c47c09c0ee3aed1e65bd5)](https://www.codacy.com/app/kerndog73/STELA?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=Kerndog73/STELA&amp;utm_campaign=Badge_Grade) [![Lines of Code](https://tokei.rs/b1/github/Kerndog73/STELA)](https://github.com/Aaronepower/tokei)

A scripting language built for speed in world where JavaScript runs on web servers.

## Examples

The semantic analyser is nearing completion as I write this. These are a few examples taken from the [semantic tests](test/src/semantics.cpp) that illistrate (with code) the semantics of the language. You can't actually run any of these examples but you can put them through the semantic analyser and not get any errors. Also, notice that they're being syntax highlighted as Go because the syntax of this language is pretty close to Go.

### Modules

Modules are pretty cool. So are 2D vectors!

```go
module glm;

type vec2 struct {
  x: real;
  y: real;
};

func add(a: vec2, b: vec2) -> vec2 {
  return make vec2 {a.x + b.x, a.y + b.y};
}

// We don't have a builtin sqrt function yet so this will have to do
func mag2(v: vec2) -> real {
  return v.x * v.x + v.y * v.y;
}
```

Other module...

```go
// Optional
// module main;

import glm;

func main() {
  // Wow, glm::vec2 is just like C++!
  let one_two = make glm::vec2 {1.0, 2.0};
  let three_four = make glm::vec2 {3.0, 4.0};
  let four_six = glm::add(one_two, three_four);
  let five = glm::mag2(one_two);
}
```

### Lambdas

Lambdas make partial application pretty easy.

```go
func makeAdder(left: sint) -> func(sint) -> sint {
  return func(right: sint) -> sint {
    return left + right;
  };
}

func test() {
  let add3 = makeAdder(3);
  let nine = add3(6);
  let eight = makeAdder(6)(2);
}
```

### Arrays

What can you do without arrays?

```go
func squares(count: uint) -> [uint] {
  var array: [uint] = [];
  if (count == 0u) {
    return array;
  }
  reserve(array, count);
  for (i := 1u; i <= count; i++) {
    push_back(array, i * i);
  }
  return array;
}

func test() {
  let empty = squares(0u);
  let one_four_nine = squares(3u);
}
```

### Get a pointer to function

Just like in C++, if you want a pointer to an overloaded function, you need to select which overload you want.

```go
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

### Tag dispatch

This example shows strong type aliases and function overloading.

```go
type first_t struct{};
type second_t struct{};
type third_t struct{};

let first: first_t = {};
let second: second_t = {};
let third: third_t = {};

// This is a little silly but you get the idea!
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

### Member functions

Member functions can be created for any type, including builtin types!

```go
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

### Enums

Enums aren't supported natively simply because I don't find the following example too inconvienient.

```go
// Strong enum
type Dir sint;
// Dir is a strong alias of sint
// You need to explicitly cast the sint literal to a Dir
let Dir_up    = make Dir 0;
let Dir_right = make Dir 1;
let Dir_down  = make Dir 2;
let Dir_left  = make Dir 3;

// Weak enums are a little bit less verbose
type Choice = sint;
let Choice_no  = 0;
let Choice_yes = 1;
```

## Progress
 * The lexical analyser is done.
 * The syntax analyser is done.
 * The semantic analyser is pretty much done. See [semantic tests](test/src/semantics.cpp). It can:
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
   * handle lambdas and function pointers
   * produce unused symbol warnings
   * lookup builtin types and builtin functions

## Plans

It might be possible to compile to C before the end of the year.

 * Compile to C. C isn't far from LLVM IR. To do this, I'll need to do name mangling and work out how to implement closures and slices.
 * Compile to LLVM IR. This is going to be really difficult. LLVM is a big, compilcated library.
 * Interop between C++ and Stela. I need to work out how to call Stela from C++ and call C++ from Stela. Maybe need to work out how to registry C++ classes with Stela. Should be easy to use with EnTT.
 * Work on the command-line interface. The CLI is for doing things like pretty-printing, syntax hightlighting and compiling to LLVM IR ahead-of-time.
 * Spread the word! I'll make a website and a logo. I'll make some posts on the programming subreddit and various other places. 
 * I might make a programming game revolved around this language. That should be a lot of fun to make!

## Building

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
