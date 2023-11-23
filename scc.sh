#! /bin/zsh
cloc . --by-file --exclude-dir=lib,inc,cmake-build-debug,cmake-build-release,.idea,.git,docs,benchmark,json,.vscode --not-match-f="CMakeLists.txt|scc|glad.c" --quiet --report-file=scc.txt