---
layout: default
title: Standard Libraries
nav_order: 1
parent: Standard Library
---

# Standard Libraries
{: .no_toc }

## Table of contents
{: .no_toc .text-delta }

1. TOC
{:toc}

---

## Standard Libraries

Ilex comes with several standard libraries for you to use. To use a built in library you use the use keyword followed by '<' and then the name of the library followed by '>' The use of <> tells the compiler that you are using a built in library instead of a user made library.

```ts
use <math> // You can use math hooray!
```

To use functions and variables from a library you use the scope resolution operator.

```ts
use <math>

println("Pi is:", math::pi)

// The dot operator works as well but :: is preferred.
println("E is:" math.e)
```

It is also possible to use individual functions and variables from a library.

```ts
use { sqrt2, sqrt} from <math>

println(sqrt2 == sqrt(2)) // true
```

Or you can use all of them.

```ts
use * from <io>

num := getNumber("Enter a number: ")
println("You entered", num)
```

It is also possible to rename libraries and functions and variables.

```ts
use <math as M>

println("Pi is:" M::pi)
```

```ts
use { number as randomNumber } from <random>

println("RNGesus chose:", randomNumber(10)) // I'm thinking of a number between 1 and 10...
```

You cannot rename the library if you are using something specific from that library.

```ts
use { pi } from <math as M> // Will not compile.
```

```ts
use { pi } from <math> // Only uses pi, math is not used.

println(math::e) // Will not compile.
```
