@echo off
cloc . --by-file --exclude-dir=lib,cmake-build-debug,cmake-build-release,.idea,.git --not-match-f="CMakeLists.txt|scc" --quiet --report-file=scc.txt