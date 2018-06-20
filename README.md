# Statically Typed Embeddable LAnguage

This project is just getting started so there isn't really much here yet. Just a few vague plans.

## File Extension
`.stela` or `.ste` (if the filesystem limits extensions to three characters).

## Goals
This language was designed to be fast, minimal and easy to embed in a C++ application.

## Influence
Much of the syntax was influenced by Swift but **this is not a subset of Swift**.

## Planning
Trying to plan this perfectly from the start is just not going to work. I'm going to just start implementing the most basic features and see how it goes. I think I can simplify Swift into something that is still really expressive while being as simple as C. Some parts of Swift are useful for a systems language but not really for scripting. Generics, protocols, extensions, exceptions, fancy enums, etc. I'm trimming the fat basically.

To summarise, I'll implement the very basics like variables, functions, conditionals. Then I'll carefully add things that I think I need.

This is a scripting language, not embedded Swift.
