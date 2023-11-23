
cc_library(
  name = "hdr",
  hdrs = ["include/wtr/watcher.hpp"],
  includes = ["include"],
  linkopts = select({
      "@platforms//os:macos": [
        "-framework", "CoreServices",
        "-framework", "CoreFoundation",
      ]
  }),
  visibility = ["//visibility:public"],
)

cc_binary(
  name = "cli",
  srcs = ["src/wtr/watcher/main.cpp"],
  copts = ["-std=c++17"],
  deps = ["//:hdr"],
)

