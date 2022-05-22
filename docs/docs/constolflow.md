---
layout: default
title: Control flow
nav_order: 6
---

# Control Flow
{: .no_toc }

## Table of contents
{: .no_toc .text-delta }

1. TOC
{:toc}

---
## If statement

```ts
if (false || true) {
    variable := "I will print"
    println(variable)
}
    
if (false) {
    println("I wont print")
} else {
    println("I will print")
}
    
    
x := 5;
if (x == 6) {
    println("Not going to print")
} else if (x == 5) {
    println("Will print")
} elif (x == 4) {
    println("Not gonna print")
} else {
    println("Willn't print")
}
```

## Switch Statement

Switch statements can be thought of like special if-else statements. Switch checks a value and executes the corresponing
case statement. If the expression matches then that code is called.

```cs
switch (10) {
    case 1: println("This is one")
    case 10: println("This is not one")
}
```

### Blocks

Cases can have blocks of code as well as expressions.

```go
x := 2
switch (x) {
    case 1: {
        println("Not printing")
        println("Also not printing")
    }
    case 2: {
        println("This will print")
        println("And so will this)
    }
}
```

### Default

Sometimes none of the case statements match the provided value but you still need to run some code in that case. In that situation you would use the default case as shown below.

```c++
switch ("string") {
    case "nope": println("This shouldn't print")
    case "nope again": println("No printing here")
    default: println("This will print!)
}
```

### Multi-Value Case

Often you will want multiple sets of values to call the same code. To do that you can have multiple values in
one case statement.

```cs
switch (10) {
    case 1, 3, 5, 7, 9: println("Not 10")
    case 0, 2, 4, 6, 8, 10: println("Finally 10!")
}
```

### Fall through Case

Sometimes you want one value to trigger multiple cases. You can do that by using '->' instead of ':'
to fall through.

```js
x := 2
switch (x) {
    case 1 -> println("One")
    case 2 -> println("Two")
    case 3: println("Three") // Don't fallthrough.
    default: println("Something else!")
}

// The above will print:
// Two
// Three
```

## Loops
### While loop

```cs
// While loop
i := 0;
while (i < 10) {
    println(i)
    i++
}
```

### Do while loop

Do while loops are very similar to while loops except that the block of code will always be executed at least once.
Useful for when something needs to happen but the condition may not be true yet or true at all.

```cs
// While loop
i := 10;
do {
    println(i)
    i++
} while (i < 10)
```

### For loop

For loops are used to iterate over a sequence. In this case it loops from 0 to 9 incrementing by 1.

```cs
// For loop
for (i := 0; i < 10; i++) {
    println("Iteration:", i)
}
```

### Continue statement

Continue allows execution of a loop to restart prematurely.

```go
for (i := 0; i < 10; i += 1) {
    if (i % 2 == 0) {
        continue // Skip all even numbers
    }

    println(i) // Only odd numbers will be printed
}
```

### Break statement

Break allows execution of a loop to stop prematurely.

```go
for (i := 0; i < 10; i += 1) {
    if (i > 5) {
        break // Exit the loop here
    }

    println(i) // Only numbers less than or equal to 5 will be printed
}
```

