# Notes

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

/* clang-format off */
inline constexpr std::array<flag_what_pair_type, flag_what_pair_count>
    flag_what_pair{
      flag_what_pair_type(kFSEventStreamEventFlagItemCreated,        event::what::create),
      flag_what_pair_type(kFSEventStreamEventFlagItemModified,       event::what::modify),
      flag_what_pair_type(kFSEventStreamEventFlagItemRemoved,        event::what::destroy),
      flag_what_pair_type(kFSEventStreamEventFlagItemRenamed,        event::what::rename),
    };
inline constexpr std::array<flag_kind_pair_type, flag_kind_pair_count>
    flag_kind_pair{
      flag_kind_pair_type(kFSEventStreamEventFlagItemIsDir,          event::kind::dir),
      flag_kind_pair_type(kFSEventStreamEventFlagItemIsFile,         event::kind::file),
      flag_kind_pair_type(kFSEventStreamEventFlagItemIsSymlink,      event::kind::sym_link),
      flag_kind_pair_type(kFSEventStreamEventFlagItemIsHardlink,     event::kind::hard_link),
      flag_kind_pair_type(kFSEventStreamEventFlagItemIsLastHardlink, event::kind::hard_link),
    };
/* clang-format on */
```

## Extra Event Flags

```cpp
    // path information, i.e. whether the path is a file, directory, etc.
    // we can get this info much more easily later on in `wtr/watcher/event`.

    flag_pair(kFSEventStreamEventFlagItemIsDir,          event::what::dir),
    flag_pair(kFSEventStreamEventFlagItemIsFile,         event::what::file),
    flag_pair(kFSEventStreamEventFlagItemIsSymlink,      event::what::sym_link),
    flag_pair(kFSEventStreamEventFlagItemIsHardlink,     event::what::hard_link),
    flag_pair(kFSEventStreamEventFlagItemIsLastHardlink, event::what::hard_link),

    // path attribute events, such as the owner and some xattr data.
    // will be worthwhile soon to implement these.
    // @todo this.
    flag_pair(kFSEventStreamEventFlagItemXattrMod,       event::what::other),
    flag_pair(kFSEventStreamEventFlagOwnEvent,           event::what::other),
    flag_pair(kFSEventStreamEventFlagItemFinderInfoMod,  event::what::other),
    flag_pair(kFSEventStreamEventFlagItemInodeMetaMod,   event::what::other),

    // some edge-cases which may be interesting later on.
    flag_pair(kFSEventStreamEventFlagNone,               event::what::other),
    flag_pair(kFSEventStreamEventFlagMustScanSubDirs,    event::what::other),
    flag_pair(kFSEventStreamEventFlagUserDropped,        event::what::other),
    flag_pair(kFSEventStreamEventFlagKernelDropped,      event::what::other),
    flag_pair(kFSEventStreamEventFlagEventIdsWrapped,    event::what::other),
    flag_pair(kFSEventStreamEventFlagHistoryDone,        event::what::other),
    flag_pair(kFSEventStreamEventFlagRootChanged,        event::what::other),
    flag_pair(kFSEventStreamEventFlagMount,              event::what::other),
    flag_pair(kFSEventStreamEventFlagUnmount,            event::what::other),
    flag_pair(kFSEventStreamEventFlagItemFinderInfoMod,  event::what::other),
    flag_pair(kFSEventStreamEventFlagItemIsLastHardlink, event::what::other),
    flag_pair(kFSEventStreamEventFlagItemCloned,         event::what::other),

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