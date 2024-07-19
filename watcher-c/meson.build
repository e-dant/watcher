# I know it's not the convential way to version a library...
# But I'm not sure how to do it the "right" way in Meson.
libwatcher_c_name = 'watcher-c-' + meson.project_version()

if target_machine.system() == 'darwin'
  libwatcher_c_deps = [dependency('CoreServices'), dependency('CoreFoundation')]
else
  libwatcher_c_deps = [dependency('threads')]
endif

libwatcher_c = library(
  libwatcher_c_name,
  ['src/watcher-c.cpp'],
  include_directories : ['include'],
  dependencies : libwatcher_c_deps,
  build_rpath : '/usr/local/lib',
  install_rpath : '/usr/local/lib',
  install : true,
)

test_libwatcher_c = executable(
  'test-watcher-c',
  ['src/test-watcher-c.c'],
  include_directories : ['include'],
  link_with : [libwatcher_c],
  build_rpath : '/usr/local/lib',
  install_rpath : '/usr/local/lib',
  install : false,
)
