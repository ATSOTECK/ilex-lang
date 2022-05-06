---
layout: default
title: Operators
nav_order: 5
---

# Operators
{: .no_toc }

## Table of contents
{: .no_toc .text-delta }

1. TOC
{:toc}

---
## Operators

| Operator     | Description                                                    | Example                    |
|:-------------|:---------------------------------------------------------------|:---------------------------|
| and, &&      | Returns true if both operands are truthy values otherwise false | true && true |
| or, \|\|     | Returns the first truthy operand unless all are false and the last operand is returned | true \|\| false |
| not, !       | Returns the inverse boolean value for a given operand | !true |
| +            | Adds the values on either side of the operator together, aslo concatinates strings        | 10 + 10, "hello" + " world" |
| -            | Subtracts the values on either side of the operator together   | 10 - 10                    |
| *            | Multiplies the values on either side of the operator together  | 10 * 2                     |
| /            | Divides the values on either side of the operator together.    | 10 / 3                     |
| %            | Modulo of values on either side of the operator                | 10 % 2                     |
| **           | Exponent (power) of the values                                 | 2 ** 2                     |
| &            | Bitwise AND of the values                                      | 10 & 2                     |
| ^            | Bitwise XOR of the values                                      | 10 ^ 2                     |
| \|           | Bitwise OR of the values                                       | 10 \| 2                    |
| +=           | Same as +, however its shorthand to assign too                 | x += 10 Same as x = x + 10 |
| ++           | Same as +, however its shorthand to assign too                 | x++ Same as x = x + 1 (postfix only for now)|
| -=           | Same as -, however its shorthand to assign too                 | x -= 10 Same as x = x - 10 |
| --           | Same as -, however its shorthand to assign too                 | x-- Same as x = x - 1 (postfix only for now)|
| *=           | Same as *, however its shorthand to assign too                 | x *= 10 Same as x = x * 10 |
| /=           | Same as /, however its shorthand to assign too                 | x /= 10 Same as x = x / 10 |
| &=           | Same as &, however its shorthand to assign too                 | x &= 10 Same as x = x & 10 |
| ^=           | Same as ^, however its shorthand to assign too                 | x ^= 10 Same as x = x ^ 10 |
| \|=          | Same as \|, however its shorthand to assign too                | x \|= 10 Same as x = x     |
| ?            | Ternary operator - See below                                   | true ? 'value' : 'other'   |
| ??           | Null coalescing operator - See below                           | x = func() ?? 12           |
| ??=          | Null coalescing operator - See below                           | x ??= 12                   |
| ?.           | Optional chaining - Same as checking object != null first. | object?.someMethod()           |
| \|\|         | Nullish coalescing operator - See below                        | x = func() \|\| 12         |
| \|\|=        | Nullish coalescing operator - See below                        | x \|\|= 12                 |

## Precedence

Precedence table from highest to lowest, with all operators having a left-to-right associativity. 

| Operators |
| . () []   |
| ?.        |
| ! -       |
| \*\*      |
| * /       |
| \+ \-     |
| &         |
| ^         |
| \|        |
| < > <= >= |
| == !=     |
| &&        |
| \|\|      |
| \=        |

### Ternary Operator

The ternary operator is an operator which takes 3 operands and returns either the second or third depending on whether the first operand is truthy. 

```go
value := true ? "true!" : "false!"
println(value) // "true!"

otherValue := 0 ? "true!" : "false!"
println(otherValue) // "false!"
```

### Null Coalescing Operator

The null coalescing operator is an operator which returns the value of its left-hand operand if it isn't null; otherwise, it evaluates the right-hand operand and returns its result.

```ts
value := null ?? "not null"
println(value) // "not null"

otherValue := "not null" ?? "null"
println(otherValue) // "not null"

xVar := null
xVar ??= 12
println(xVar) // "12"

// xVar ??= 12 is the same as
if (xVar == null) {
    xVar = 12
}
```

### Nullish Coalescing Operator

The nullish coalescing operator is an operator which returns the value of its left-hand operand if it isn't falsy; otherwise, it evaluates the right-hand operand and returns its result.

```ts
value := 0 || "not falsy"
println(value) // "not falsy"

otherValue := true || "false"
println(otherValue) // "true"

xVar := 0
xVar ||= 12
println(xVar) // "12"

// xVar ||= 12 is the same as
if (!xVar) {
    xVar = 12
}
```

### Falsy

The following are considered falsy: 

| null                   | 
| false                  |
| 0                      |
| "", '' (empty strings) |
| []     (empty arrays)  |

 Where empty is defined as having a length of 0.
 Everything else is considered truthy.
