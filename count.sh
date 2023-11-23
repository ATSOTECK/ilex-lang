#! /bin/zsh
cloc --exclude-dir=inc,cmake-build-debug,cmake-build-release,.idea,.vscode,lib,docs,tst --exclude-list-file=glad.c,CMakeLists.txt --exclude-ext=json,ts,d c/