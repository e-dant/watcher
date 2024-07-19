{
  "targets": [
    {
      "target_name": "watcher",
      "sources": [
        "../watcher-c/src/watcher-c.cpp",
        "watcher.cpp",
      ],
      "include_dirs": [
        "../watcher-c/include",
      ],
      "link_settings": {
        "libraries": [
          "-Wl,-rpath,/usr/local/lib",
        ]
      }
    }
  ]
}

