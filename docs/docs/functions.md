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

### Anonymous functions

Anonymous functions are function definitions that are not bound to an identifier.
Anonymous functions are either a single expression or can include a function body using {}.

```
// If the anonymous function's body is not using {} then a single expression
// is allowed and the value is returned implicitly without needing "return".

func := fn -> 10;
func() // 10

// If a function body is enclosed by {} then it acts like a regular function.

func := fn -> {
    return 10 
}
func() // 10
```

Anonymous functions can take arguments as well. Anonymous function arguments are listed
inbetween two vertical bars like |x|, instead of using () as this makes them more distinct.

```
func := fn |x| x * 2 // In this case the '->' is not needed because the function
func(2) // 4         // has arguments and is a single expression.

                
// Anonymous functions can also have arguments and use {}.
// In this case '->' must be used because the function has a body.
func := fn |x, y| -> {
    return x * y
}
func(3, 4) // 12
```

Anonymous functions don't have to return anything.

```
sayHi := fn -> println('Hi!')
sayHi() // Hi!
```

Though the function will return null implicitly.

Anonymous functions can be passed to other functions as callbacks.

```
arr := [1, 2, 3, 4, 5, 6]
evens := arr.filter(fn |n| n % 2 == 0)
println(evens) // [2, 4, 6]
```


## Default Arguments (Not Yet Implemented)

Functions can have default arguments.

```rs
fn someFunc(a: string, b: number = 12) {
    println(a, b)
}

someFunc("Hi") // Prints "Hi 12"

// Won't compile! Can't have a required argument after a default argument.
fn someFunc(a: string, b: number = 12, c: bool) {
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

// Won't compile! Can't have required arguments after optional ones.
fn func2(a: string, b?: number, c: bool) {
    ...
}

// Won't compile! An argument can't be optional and have a default value.
fn func3(a: string, b?: number = 12) {
    ...
}
```
