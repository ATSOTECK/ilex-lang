---
layout: default
title: Welcome
nav_order: 1
permalink: /
---


# Ilex v0.0.28
Ilex is a simple easy to use, statically (not yet) typed programming language.

Ilex is a high-level-statically(TODO) typed programming language. It has a familiar c/go/typescript-like syntax with inspiration from other languages such as Python.

The main goal of Ilex is to be easy to use. Ilex strives for adequacy and will settle for nothing less (eventually).

## Installing Ilex
If I haven't been too lazy there should be links to download the executables for windows and mac in the releases section of the github.

### Building With CMake

```bash
$ git clone https://github.com/ATSOTECK/ilex-lang.git
$ cd ilex-lang
$ cmake -DCMAKE_BUILD_TYPE=Release -B ./bin 
$ cmake --build ./bin
```

I haven't actually tested this yet but it worked with the GUI. Just make sure you do a release build or it will be slow.

## Contributing
The language isn't really ready for contributing at the moment. However if you have ideas for the language or suggestions for included libraries I'd like to hear them.

## License
Ilex is under the [MIT license](https://github.com/dictu-lang/Dictu/blob/master/LICENSE).