#! /usr/bin/env bash
(
  cd "$(dirname "$0")"
  c++ -framework CoreFoundation -framework CoreServices -std=c++17 main.cpp -o fsevents-issue
  install_name_tool -add_rpath /usr/local/lib fsevents-issue
)

