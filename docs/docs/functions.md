---
layout: default
title: Functions
nav_order: 4
---

# Functions
{: .no_toc }

## Table of contents
{: .no_toc .text-delta }

1. TOC
{:toc}

---

## Functions

Functions are declared with the 'fn' keyword.

```rs
fn someFunction() {
    return 12
}
```

Functions and function arguments do not need type information yet.

```rs
fn add(num1, num2) {
    return num1 + num2
}
```

Functions are first class members.

```rs
fn someFunc(arg) {
    println("You said:", arg)
}

funcVar := someFunc
funcVar("Hello!")
```

The above will print You said: Hello!

## Default Arguments (Not Yet Implemented)

Functions can have default arguments.

```rs
fn someFunc(a: string, b: number = 12) {
    println(a, b)
}

someFunc("Hi") // Prints "Hi 12"

fn someFunc(a: string, b: number = 12, c: bool) { // Won't compile! Can't have a required argument after a default argument.
    ...
}

```

## Optional Arguments (Not Yet Implemented)

Functions can have optional arguments as well.

```rs
fn someFunc(a: string, b?: number) {
    println(a, b)
}

someFunc("Hi") // Prints "Hi null"

fn func2(a: string, b?: number, c: bool) { // Won't compile! Can't have required arguments after optional ones.
    ...
}

fn func3(a: string, b?: number = 12) { // Won't compile! An argument can't be optional and have a default value.
    ...
}

```
