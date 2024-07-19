{
  "targets": [
    {
      "target_name": "watcher",
      "sources": [
        "../libcwatcher/src/watcher-cabi.cpp",
        "cwatcher_wrapper.cpp",
      ],
      "include_dirs": [
        "../libcwatcher/include",
      ],
      "link_settings": {
        "libraries": [
          "-Wl,-rpath,/usr/local/lib",
        ]
      }
    }
  ]
}

