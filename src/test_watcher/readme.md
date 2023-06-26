# Test Watcher

On macOS, when testing more than several thousand or so filesystem events, "ghost" events from prior tests appear.

For example, a few minutes after the directory test, we will still see events like this in our queue:

```
"1667826825936947000":{"where":"/Users/test/dev/watcher/build/tmp_test_watcher/dir_store/42980","what":"create","kind":"other"},
```

```
"1667826980514171000":{"where":"/Users/test/dev/watcher/build/tmp_test_watcher/dir_store/20654","what":"create","kind":"other"},
```

Still, it might not be a good idea to ignore these events or clear the queue. The events did happen, they're just being reported late.
