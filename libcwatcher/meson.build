# I know it's not the convential way to version a library...
# But I'm not sure how to do it the "right" way in Meson.
libcwatcher_name = 'cwatcher-' + meson.project_version()

if target_machine.system() == 'darwin'
  libcwatcher_deps = [dependency('CoreServices'), dependency('CoreFoundation')]
else
  libcwatcher_deps = [dependency('threads')]
endif

libcwatcher = library(
  libcwatcher_name,
  ['src/watcher-cabi.cpp'],
  include_directories : ['include'],
  dependencies : libcwatcher_deps,
  build_rpath : '/usr/local/lib',
  install_rpath : '/usr/local/lib',
  install : true,
)

test_libcwatcher = executable(
  'test-watcher-cabi',
  ['src/test-watcher-cabi.c'],
  include_directories : ['include'],
  link_with : [libcwatcher],
  build_rpath : '/usr/local/lib',
  install_rpath : '/usr/local/lib',
  install : false,
)
