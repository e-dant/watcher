project(
  'libcwatcher',
  ['cpp', 'c'],
  version : '0.12.0', # hook: tool/release
  default_options : ['c_std=c99', 'cpp_std=c++20'],
)

libname = 'cwatcher-' + meson.project_version()

if target_machine.system() == 'darwin'
  deps = dependency('appleframeworks', ['CoreServices', 'CoreFoundation'])
else
  deps = dependency('threads')
endif

cwatcher_shared_lib = shared_library(
  libname,
  ['src/watcher-cabi.cpp'],
  include_directories : ['include'],
  dependencies : deps,
  build_rpath : '/usr/local/lib',
  install_rpath : '/usr/local/lib',
  install : true,
)

cwatcher_static_lib = static_library(
  libname,
  ['src/watcher-cabi.cpp'],
  include_directories : ['include'],
  dependencies : deps,
  build_rpath : '/usr/local/lib',
  install_rpath : '/usr/local/lib',
  install : true,
)

executable(
  'test-watcher-cabi',
  ['src/test-watcher-cabi.c'],
  include_directories : ['include'],
  link_with : [cwatcher_shared_lib],
  build_rpath : '/usr/local/lib',
  install_rpath : '/usr/local/lib',
  install : false,
)
