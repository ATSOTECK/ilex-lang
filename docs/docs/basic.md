---
layout: default
title: Basics
nav_order: 2
---

# Basics
{: .no_toc }

## Table of contents
{: .no_toc .text-delta }

1. TOC
{:toc}

---
## Hello World!

Here you will learn how to do the obligitory 'Hello World!' first program. Make a file ending in .ilex and type:

```cs
println("Hello, World!")
```

Now run the file and you should see Hello, World!

You may have noticed there is no semicolon. Semicolons in Ilex are optional. So

```cs
println("Hello, World!");
```

Is perfectly valid as well. At the moment error messages are more acurate if you use semicolons but this is a know issue that will be addressed. So if you're not using semicolons and are getting some unrelated error try using semicolons on the lines you think might have the issue and that should help the compiler out and give you a better error message.


## Comments

Ilex has three different ways to comment:

```cs
// This is a single line comment

/-
This is a
multiline comment
-/

/*
This is also a
multiline comment
*/
```

Comments can be nested as well:

```cs
/*
This is also a
multiline comment
    /*
    This is a comment inside a comment
    */
    
    /-
    This is also a comment inside a comment
    -/
    
    //Yay comments!
    
    /-
        Multiline comments can mix the start and stop
    */
    
    /*
        Like so
    -/
*/
```

{: .no_toc }