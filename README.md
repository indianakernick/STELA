# Statically Typed Embeddable LAnguage

[![Build Status](https://travis-ci.org/Kerndog73/STELA.svg?branch=master)](https://travis-ci.org/Kerndog73/STELA)
[![Coverage Status](https://coveralls.io/repos/github/Kerndog73/STELA/badge.svg?branch=master)](https://coveralls.io/github/Kerndog73/STELA?branch=master)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/9a5be676e21c47c09c0ee3aed1e65bd5)](https://www.codacy.com/app/kerndog73/STELA?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=Kerndog73/STELA&amp;utm_campaign=Badge_Grade)
[![Lines of Code](https://tokei.rs/b1/github/Kerndog73/STELA)](https://github.com/Aaronepower/tokei)

A scripting language built for speed in world where JavaScript runs on web servers.

**Disclamer:**
I'm calling this a "scripting language built for speed" but does that really
make sense? If you want speed, you need control. You need to be able to do
unsafe things. `std::string_view` is fast but `std::string_view` is also unsafe. It
could hold a dangling pointer if you're not careful. `operator[]` is fast but it's
unsafe. `operator[]` doesn't do bounds checking. I've never used `.at()` and I never
will! I know what I'm doing. I know not to access memory outside the bounds of
an array.

Being safe costs performance. Scripting languages are meant to be safe. I'll
try to get as much safety and speed as I can. Even if I don't make this as fast
as I would like, it will still be much faster than Lua simply due to LLVM and
static typing.

## Examples

The LLVM backend is underway. Lambdas and arrays are not supported yet. It's still very experimental.
Part of the C++ backend is still in the code but commented out.

The CLI is not implemented yet so here is an example of compiling a Stela program to LLVM IR and executing it. See the **Building** section.

```C++
#include <iostream>
#include <STELA/llvm.hpp>
#include <STELA/binding.hpp>
#include <STELA/code generation.hpp>
#include <STELA/syntax analysis.hpp>
#include <STELA/semantic analysis.hpp>
#include <llvm/ExecutionEngine/ExecutionEngine.h>

int main() {
  const std::string_view source = R"(
    func plus(left: real, right: real) {
      return left + right;
    }
  )";

  // Create a log sink
  // ColorSink implements color logging using ANSI escape codes
  // See STELA/log.hpp for the other sinks
  stela::ColorSink sink;
  
  // Create an Abstract Syntax Tree from the source code
  stela::AST ast = stela::createAST(source, sink);
  // Initialize the builtin types and functions
  stela::Symbols syms = stela::initModules(sink);
  // Perform semantic analysis (type checking and all that) on the AST
  stela::compileModule(syms, ast, sink);
  
  // Make sure LLVM is initialized before you do any code generation
  stela::initLLVM();
  
  // Generate LLVM IR
  // The whole program (with all of the modules) is compiled to one llvm::Module
  std::unique_ptr<llvm::Module> module = stela::generateIR(syms, sink);
  // Create an executable from the LLVM IR
  llvm::ExecutionEngine *engine = stela::generateCode(std::move(module), sink);
  
  // stela::Real is just an alias for float so you can do this if you want
  // using Signature = float(float, float);
  using Signature = stela::Real(stela::Real, stela::Real);
  // There's no type checking yet
  // We're just blindly reinterpret_casting a pointer
  stela::Function<Signature> plus{engine->getFunctionAddress("plus")};
  std::cout << "7 + 9 = " << plus(7.0f, 9.0f) << '\n';
  
  stela::quitLLVM();
  
  return 0;
}
```

~~Here's some programs you can try out!~~ Currently, only the **Member functions** test can be compiled to LLVM IR.
The other tests use arrays, lambdas and global variables which are not supported yet.

### Lambdas

Lambdas and return type deduction make partial application pretty easy.

```go
func makeAdder(left: sint) {
  return func(right: sint) {
    return left + right;
  };
}

func test() {
  let add3 = makeAdder(3);
  let nine = add3(6);
  let eight = makeAdder(6)(2);
}
```

Function pointers are initialized to `panic` functions so if you call a function pointer that hasn't been assigned
a function, you'll get an error message that lets you know and then the program crashes.
Compile `var lam: func();` to see what I mean.
This also makes calling function pointers a little bit faster because there's no need to check for null.

Lambdas are stateful. They carry around copies of all of the captured variables.

```go
func makeIDgen(first: sint) {
  return func() {
    let id = first;
    first++;
    return id;
  };
}

func test() {
  let gen = makeIDgen(4);
  let four = gen();
  let five = gen();
  let six = gen();
  // gen and otherGen share state
  let otherGen = gen;
  let seven = otherGen();
  let eight = gen();
  let nine = otherGen();
  // Closures in Stela behave like this std::shared_ptr<std::function>
  // except that they're pretty damn close to being as fast as a
  // plain old function pointer
  // See the generated C++ for this program to learn more:
  //   let lam = func() { return 2; };
  //   let two = lam();
}
```

I spent about a day getting this monstrosity to produce the right code.

If you look at the `return` statement for the deepest lambda, you'll see a reference to `a`.
To access `a`, this lambda has to capture `a`, the parent has to capture `a` and the parent of the parent has to capture `a`.
While doing this, we have to make sure we don't accidentially get `a` mixed up with `c` or something.
To test this, I made all of the parameters different types and compiled with `-Wconversion`.
It seems like a simple problem but then you start writing code that writes code and you realise it's quite tricky indeed.

```go
func makeAdd(a: sint) {
  var other0 = 0;
  return func(b: uint) {
    other0++;
    var other1 = 1;
    var other2 = 2;
    a *= 2;
    return func(c: byte) {
      other0++;
      other1++;
      c = make byte (make sint c * 2);
      other2++;
      other1++;
      return func(d: char) {
        d *= 2c;
        other1++;
        b *= 2u;
        other0++;
        return make real a + make real b + make real c + make real d;
      };
    };
  };
}

func test() {
  let add_1 = makeAdd(1);
  let add_1_2 = add_1(2u);
  let add_1_2_3 = add_1_2(3b);
  let twenty = add_1_2_3(4c);
}
```

### Modules

Modules are essentially just a way of concatenating ASTs in the right order. 
There's a function called `findModuleOrder` which analyses the graph of imports in multiple ASTs and determines the order.
You can pass the `ModuleOrder` to `compileModules` to concatenate the ASTs in the right order and then compile them.

To compile a program with modules, you'll have to do something like this:

```C++
stela::ASTs asts;
asts.push_back(stela::createAST(glm_source, log));
asts.push_back(stela::createAST(main_source, log));
stela::ModuleOrder order = stela::findModuleOrder(asts, log);
stela::compileModules(syms, order, asts, log);
````

If you change one line of code in any of your modules, the whole program will have to be recompiled.
Programs you write in Stela probably aren't going to be large enough for this to really matter at all.
Compiling the whole program produces faster code. It's a bit like passing `-flto` to GCC or Clang.

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
  let one_two = make vec2 {1.0, 2.0};
  let three_four = make vec2 {3.0, 4.0};
  let four_six = add(one_two, three_four);
  let five = mag2(one_two);
}
```

### Arrays

Arrays in Stela behave like `std::vector`.
If you're worried about passing a big array to a function, you can pass by reference.
I haven't implemented `const &` yet but I plan to. I'm not aware of any way to leak memory or access a nullptr.
There's no way of creating a circular reference because there's no way for a lambda to point to itself.

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

Calling a function pointer is Stela is almost as fast as calling a function pointer in C++.
It's faster than `std::function` because it avoids a lot of indirection and virtual function calls.
A function pointer in Stela is a `struct` with a C function pointer and a pointer to the captured
data. The pointer to the captured data is passed as the first argument.

All generated non-member functions have a `void *` as their first parameter so that they
can be passed a pointer to nothing! I do plan on removing the `void *` parameter for functions
that are never stored in function pointers.

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

This example is the reason why I can't rely on C++'s function overloading method.
In the generated code, `first_t`, `second_t` and `third_t` are not distinct types.
The three `get` functions have the same signature in the generated C++ so they have to be named `f_0`, `f_1` and `f_2`.

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

There's nothing special about member functions. They just allow you to write `v.f()` instead of `f(v)`.
That's really all they are. Just a little bit of sugar!

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
 * The semantic analyser is done. It can:
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
 * LLVM code generation is underway.
 
## Building

This project depends on [Simpleton](https://github.com/Kerndog73/Simpleton-Engine) as a build-time dependency. 
CMake will automatically download Simpleton so you don't have to worry about it.
Another dependency (that you *do* have to worry about) is LLVM. See the **Install LLVM** section for details.

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

## Install LLVM

If LLVM is not available with your favorite package manager (`brew`, `apt-get`, `vcpkg`),
visit the [LLVM Download Page](https://releases.llvm.org/download.html)
to download the sources or pre-built binaries.

If CMake is unable to find LLVM, set the prefix path to the directory containing `LLVMConfig.cmake`. 
For example, on MacOS, you might need pass this flag to CMake if you're installing with Homebrew: 

```
-DCMAKE_PREFIX_PATH=/usr/local/opt/llvm/lib/cmake/llvm
```
