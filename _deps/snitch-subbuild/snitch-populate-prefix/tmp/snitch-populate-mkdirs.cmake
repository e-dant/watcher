# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/Users/edant/dev/watcher/_deps/snitch-src"
  "/Users/edant/dev/watcher/_deps/snitch-build"
  "/Users/edant/dev/watcher/_deps/snitch-subbuild/snitch-populate-prefix"
  "/Users/edant/dev/watcher/_deps/snitch-subbuild/snitch-populate-prefix/tmp"
  "/Users/edant/dev/watcher/_deps/snitch-subbuild/snitch-populate-prefix/src/snitch-populate-stamp"
  "/Users/edant/dev/watcher/_deps/snitch-subbuild/snitch-populate-prefix/src"
  "/Users/edant/dev/watcher/_deps/snitch-subbuild/snitch-populate-prefix/src/snitch-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/edant/dev/watcher/_deps/snitch-subbuild/snitch-populate-prefix/src/snitch-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/edant/dev/watcher/_deps/snitch-subbuild/snitch-populate-prefix/src/snitch-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
