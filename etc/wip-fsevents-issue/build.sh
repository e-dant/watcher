#! /usr/bin/env bash
(
  cd "$(dirname "$0")"
  c++ -framework CoreFoundation -framework CoreServices -std=c++17 main.cpp -o fsevents-issue-cpp
  cc  -framework CoreFoundation -framework CoreServices -std=c17   main.c   -o fsevents-issue-c
  install_name_tool -add_rpath /usr/local/lib fsevents-issue-cpp
  install_name_tool -add_rpath /usr/local/lib fsevents-issue-c
)

