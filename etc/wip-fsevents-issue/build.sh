#! /usr/bin/env bash
(
  cd "$(dirname "$0")"
  c++ -framework CoreFoundation -framework CoreServices -std=c++17 main.cpp -g -o fsevents-issue-cpp
  c++ -fsanitize=thread -framework CoreFoundation -framework CoreServices -std=c++17 main.cpp -g -o fsevents-issue-cpp-tsan
  c++ -fsanitize=address -framework CoreFoundation -framework CoreServices -std=c++17 main.cpp -g -o fsevents-issue-cpp-asan
  cc  -framework CoreFoundation -framework CoreServices -std=c17   main.c   -g -fPIC -o fsevents-issue-c
  install_name_tool -add_rpath /usr/local/lib fsevents-issue-cpp
  install_name_tool -add_rpath /usr/local/lib fsevents-issue-cpp-tsan
  install_name_tool -add_rpath /usr/local/lib fsevents-issue-cpp-asan
  install_name_tool -add_rpath /usr/local/lib fsevents-issue-c
)

