---
layout: default
title: Variables
nav_order: 3
---

# Variables
{: .no_toc }

## Table of contents
{: .no_toc .text-delta }

1. TOC
{:toc}

---

## Variables

Variables are declared with the 'var' keyword.

```ts
var someNumber = 12
```

Multiple variables can be declared at once.

```ts
var a = 1, b = 2, c = 3
```

Alternatively variables can be declared with the variable declaration operator. This is the preferred method and is what I will use on this site, but you do you.
```go
x := 12
```

Constants are declared with the 'const' keyword.

```ts
const someNumber = 12
const a = 1, b = 2, c = 3
```

Alternatively constants can be declared with the constant declaration operator. (Not yet implemented)
```go
x ::= 12
```

## Data types

| Type | Example | Note |
| --- | --- | --- |
| String | `'foo'`, `"bar"` | You can use single or double quotes to represent strings in Ilex. |
| Number | `12`, `12.5` | Numbers with or without decimals. |
| Boolean | `true`, `false` | Can be true or false. Shocking. |
| Array | `[1, 2, 'yo', true, null]` | Arrays can contain any data type or combination of data types. |
| Null | `null` | No value. |

Ilex will be statically typed at some point and this page will be updated to reflect that.
