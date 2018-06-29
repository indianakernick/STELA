# Statically Typed Embeddable LAnguage

A scripting language with all of the benefits of static typing, without the verbosity of type names.

## Progress
The lexical analyser is done. The syntax analyser is about halfway done.

## About

### Goals
This language was designed to be fast, minimal and easy to embed in a C++ application.

### File Extension
`.stela` or `.ste` (if the filesystem limits extensions to three characters).

### Influence
Much of the syntax was influenced by Swift but **this is not embedded Swift**.

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
test/Test
make install
```

This will install `lib/libSTELA.a`, `include/STELA` and `bin/stela`.
