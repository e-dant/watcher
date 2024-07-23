# Libwatcher-c depends on ../include/wtr/watcher.hpp, but meson doesn't allow
# that kind of structure. We can just copy the header file around ourselves.
portable_cp = join_paths(meson.source_root(), 'tool', 'portable-cp')
watcher_hpp_src = join_paths(meson.source_root(), 'include', 'wtr', 'watcher.hpp')
watcher_hpp_dst = join_paths(meson.source_root(), 'watcher-c', 'include', 'wtr', 'watcher.hpp')
watcher_hpp = custom_target(
  'watcher_hpp',
  build_by_default : true,
  command : [portable_cp, watcher_hpp_src, watcher_hpp_dst],
  output : 'watcher.hpp',
  install : false,
)
watcher_hpp_dep = declare_dependency(sources : watcher_hpp)

# I know it's not the convential way to version a library...
# But I'm not sure how to do it the "right" way in Meson.
libwatcher_c_name = 'watcher-c-' + meson.project_version()

if target_machine.system() == 'darwin'
  libwatcher_c_deps = [dependency('CoreServices'), dependency('CoreFoundation'), watcher_hpp_dep]
else
  libwatcher_c_deps = [dependency('threads'), watcher_hpp_dep]
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
