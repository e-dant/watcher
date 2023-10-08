# Notes

## Lifetime Management and Asio

```
/*  Keeping this here, away from the `while (is_living())
    ...` loop, because I'm thinking about moving all the
    lifetime management up a layer or two. Maybe the
    user-facing `watch` class can take over the sleep timer,
    threading, and closing the system's resources. Maybe we
    don't even need an adapter layer... Just a way to expose
    a concistent and non-blocking kernel API.

    The sleep timer and threading are probably unnecessary,
    anyway. Maybe there's some stop token or something more
    asio-like that we can use instead of the
    sleep/`is_living()` loop. Instead of threading, we should
    just become part of an `io_context` and let `asio` handle
    the runtime.

    I'm also thinking of ways use `asio` in this project.
    The `awaitable` coroutines look like they might fit.
    Might need to rip out the `callback` param. This is a
    relatively small project, so there isn't *too* much work
    to do. (Last words?) */
/*
inline auto open_watch(
  std::filesystem::path const& path,
  ::wtr::watcher::event::callback const& callback) noexcept
{
  return [sysres = open_event_stream(path, callback)]() noexcept -> bool
  { return close_event_stream(std::move(sysres)); };
}
*/
```

## Event Stream Context

To set up a context with some parameters, something like this, from the
`fswatch` project repo, could be used:

  ```cpp
  std::unique_ptr<FSEventStreamContext> context(
      new FSEventStreamContext());
  context->version         = 0;
  context->info            = nullptr;
  context->retain          = nullptr;
  context->release         = nullptr;
  context->copyDescription = nullptr;
  ```

## Inode and Time

To grab the inode and time information about an event, something like this, also
from `fswatch`, could be used:

  ```cpp
  time_t curr_time;
  time(&curr_time);
  auto cf_inode = static_cast<CFNumberRef>(CFDictionaryGetValue(
      _path_info_dict, kFSEventStreamEventExtendedFileIDKey));
  unsigned long inode;
  CFNumberGetValue(cf_inode, kCFNumberLongType, &inode);
  std::cout << "_path_cfstring "
            << std::string(CFStringGetCStringPtr(_path_cfstring,
            kCFStringEncodingUTF8))
            << " (time/inode " << curr_time << "/" << inode << ")"
            << std::endl;
  ```

## Parsing Flags as Pairs

```cpp
/* @brief
   Basic information about what happened to some path.
   this group is the important one.
   See note [Extra Event Flags] */

// clang-format off
inline constexpr std::array<flag_effect_pair_type, flag_effect_pair_count>
    flag_effect_pair{
      flag_effect_pair_type(kFSEventStreamEventFlagItemCreated,        event::effect_type::create),
      flag_effect_pair_type(kFSEventStreamEventFlagItemModified,       event::effect_type::modify),
      flag_effect_pair_type(kFSEventStreamEventFlagItemRemoved,        event::effect_type::destroy),
      flag_effect_pair_type(kFSEventStreamEventFlagItemRenamed,        event::effect_type::rename),
    };
inline constexpr std::array<flag_path_type_map_type, flag_to_path_type_count>
    flag_path_type_map{
      flag_path_type_map_type(kFSEventStreamEventFlagItemIsDir,          event::path_type::dir),
      flag_path_type_map_type(kFSEventStreamEventFlagItemIsFile,         event::path_type::file),
      flag_path_type_map_type(kFSEventStreamEventFlagItemIsSymlink,      event::path_type::sym_link),
      flag_path_type_map_type(kFSEventStreamEventFlagItemIsHardlink,     event::path_type::hard_link),
      flag_path_type_map_type(kFSEventStreamEventFlagItemIsLastHardlink, event::path_type::hard_link),
    };
// clang-format on
```

## Extra Event Flags

```cpp
    // path information, i.e. whether the path is a file, directory, etc.
    // we can get this info much more easily later on in `wtr/watcher/event`.

    flag_effect_map(kFSEventStreamEventFlagItemIsDir,          event::effect_type::dir),
    flag_effect_map(kFSEventStreamEventFlagItemIsFile,         event::effect_type::file),
    flag_effect_map(kFSEventStreamEventFlagItemIsSymlink,      event::effect_type::sym_link),
    flag_effect_map(kFSEventStreamEventFlagItemIsHardlink,     event::effect_type::hard_link),
    flag_effect_map(kFSEventStreamEventFlagItemIsLastHardlink, event::effect_type::hard_link),

    // path attribute events, such as the owner and some xattr data.
    // will be worthwhile soon to implement these.
    // @todo this.
    flag_effect_map(kFSEventStreamEventFlagItemXattrMod,       event::effect_type::other),
    flag_effect_map(kFSEventStreamEventFlagOwnEvent,           event::effect_type::other),
    flag_effect_map(kFSEventStreamEventFlagItemFinderInfoMod,  event::effect_type::other),
    flag_effect_map(kFSEventStreamEventFlagItemInodeMetaMod,   event::effect_type::other),

    // some edge-cases which may be interesting later on.
    flag_effect_map(kFSEventStreamEventFlagNone,               event::effect_type::other),
    flag_effect_map(kFSEventStreamEventFlagMustScanSubDirs,    event::effect_type::other),
    flag_effect_map(kFSEventStreamEventFlagUserDropped,        event::effect_type::other),
    flag_effect_map(kFSEventStreamEventFlagKernelDropped,      event::effect_type::other),
    flag_effect_map(kFSEventStreamEventFlagEventIdsWrapped,    event::effect_type::other),
    flag_effect_map(kFSEventStreamEventFlagHistoryDone,        event::effect_type::other),
    flag_effect_map(kFSEventStreamEventFlagRootChanged,        event::effect_type::other),
    flag_effect_map(kFSEventStreamEventFlagMount,              event::effect_type::other),
    flag_effect_map(kFSEventStreamEventFlagUnmount,            event::effect_type::other),
    flag_effect_map(kFSEventStreamEventFlagItemFinderInfoMod,  event::effect_type::other),
    flag_effect_map(kFSEventStreamEventFlagItemIsLastHardlink, event::effect_type::other),
    flag_effect_map(kFSEventStreamEventFlagItemCloned,         event::effect_type::other),

    // for debugging, it may be useful to print everything.
    if (flag_recv & kFSEventStreamEventFlagMount)
      std::cout << "mount" << std::endl;
    if (flag_recv & kFSEventStreamEventFlagUnmount)
      std::cout << "unmount" << std::endl;
    if (flag_recv & kFSEventStreamEventFlagNone)
      std::cout << "none" << std::endl;
    if (flag_recv & kFSEventStreamEventFlagItemCreated)
      std::cout << "created" << std::endl;
    if (flag_recv & kFSEventStreamEventFlagItemModified)
      std::cout << "modified" << std::endl;
    if (flag_recv & kFSEventStreamEventFlagItemRemoved)
      std::cout << "removed" << std::endl;
    if (flag_recv & kFSEventStreamEventFlagItemRenamed)
      std::cout << "renamed" << std::endl;
    if (flag_recv & kFSEventStreamEventFlagItemCloned)
      std::cout << "cloned" << std::endl;
    if (flag_recv & kFSEventStreamEventFlagHistoryDone)
      std::cout << "history done" << std::endl;
    if (flag_recv & kFSEventStreamEventFlagKernelDropped)
      std::cout << "kernel dropped" << std::endl;
    if (flag_recv & kFSEventStreamEventFlagUserDropped)
      std::cout << "user dropped" << std::endl;
    if (flag_recv & kFSEventStreamEventFlagOwnEvent)
      std::cout << "own event" << std::endl;
    if (flag_recv & kFSEventStreamEventFlagRootChanged)
      std::cout << "root changed" << std::endl;
    if (flag_recv & kFSEventStreamEventFlagEventIdsWrapped)
      std::cout << "event ids wrapped" << std::endl;
    if (flag_recv & kFSEventStreamEventFlagItemChangeOwner)
      std::cout << "change owner" << std::endl;
    if (flag_recv & kFSEventStreamEventFlagItemXattrMod)
      std::cout << "xattr mod" << std::endl;
    if (flag_recv & kFSEventStreamEventFlagItemFinderInfoMod)
      std::cout << "finder info mod" << std::endl;
    if (flag_recv & kFSEventStreamEventFlagItemInodeMetaMod)
      std::cout << "inode meta mod" << std::endl;
    if (flag_recv & kFSEventStreamEventFlagMustScanSubDirs)
      std::cout << "must scan sub dirs" << std::endl;
```