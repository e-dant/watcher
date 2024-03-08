#include <array>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#include <dispatch/dispatch.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <tuple>

using namespace std;

static std::mutex cout_mutex{};
static auto println = [](auto&&... args)
{
  auto _ = std::scoped_lock{cout_mutex};
  (cout << ... << args) << endl;
};

/*  A fat pointer to a fixed buffer sized for paths. */
struct Path {
  int len = 0;
  char buf[PATH_MAX] = {0};

  static auto from_cstr(char const* cstr, int cstr_len) -> Path
  {
    auto path = Path{};
    auto len_capped = std::min(cstr_len, PATH_MAX);
    memcpy(path.buf, cstr, len_capped);
    path.buf[len_capped] = '\0';
    path.len = len_capped;
    return path;
  }

  auto lenclamp(Path const& r) const { return std::min(this->len, r.len); }

  auto cmp(Path const& r) const
  {
    return strncmp(buf, r.buf, std::max(len, r.len));
  }

  bool operator<(Path const& other) const { return this->cmp(other) < 0; }

  bool operator>(Path const& other) const { return this->cmp(other) > 0; }

  bool operator==(Path const& other) const { return this->cmp(other) == 0; }
};

class LockedUniquePaths {
  set<Path> paths{};
  mutex mtx{};
public:
  auto get() -> set<Path>& {
    auto _ = std::scoped_lock{mtx};
    return this->paths;
  }
};

char const* cstr_from_fs_event(void* event_recv_paths, unsigned long i)
{
  if (! event_recv_paths) return NULL;
  void const* from_arr =
    CFArrayGetValueAtIndex((CFArrayRef)event_recv_paths, (CFIndex)i);
  if (! from_arr) return NULL;
  void const* from_dict = CFDictionaryGetValue(
    (CFDictionaryRef)from_arr,
    kFSEventStreamEventExtendedDataPathKey);
  if (! from_dict) return NULL;
  char const* as_cstr =
    CFStringGetCStringPtr((CFStringRef)from_dict, kCFStringEncodingUTF8);
  if (! as_cstr) return NULL;
  return as_cstr;
}

auto event_recv(
  ConstFSEventStreamRef,
  void* recv_ctx,
  unsigned long recv_path_count,
  void* recv_paths,
  unsigned const* recv_flags,
  FSEventStreamEventId const*) -> void
{
  println("event_recv on thread ", this_thread::get_id());
  if (recv_ctx && recv_paths && recv_flags) {
    auto paths = static_cast<LockedUniquePaths*>(recv_ctx)->get();
    for (unsigned long nth_path = 0; nth_path < recv_path_count; nth_path++) {
      auto recv_cstr = cstr_from_fs_event(recv_paths, nth_path);
      if (! recv_cstr) continue;
      auto path = Path::from_cstr(recv_cstr, strlen(recv_cstr));
      auto [at, is_new] = paths.insert(std::move(path));
      if (is_new) { println("recv ", at->buf); }
    }
  }
}

auto get_scheduler()
{
#ifdef WTR_WATCHER_USE_UNSAFE_BUT_UNDEPRECATED_DISPATCH
  static auto sched =
    dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
#else
  auto sched = CFRunLoopGetCurrent();
#endif
  return sched;
}

using Scheduler = decltype(get_scheduler());

auto associate_fsevent_stream_with_scheduler(FSEventStreamRef stream) -> Scheduler
{
  auto sched = get_scheduler();
#ifdef WTR_WATCHER_USE_UNSAFE_BUT_UNDEPRECATED_DISPATCH
  FSEventStreamSetDispatchQueue(stream, sched);
#else
  /*  Any warnings about using deprecated
      Core Foundation run-loop APIs are not
      relevant. We support dispatch. We just
      don't use it because it crashes under
      heavy load. Dispatch appears to call
      into garbage memory. Reallocations and
      frees on memory sometimes fails due to
      "not being allocated" or botched free
      list checks. These crashes are very
      rare and very real. */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
  FSEventStreamScheduleWithRunLoop(stream, sched, kCFRunLoopDefaultMode);
#pragma clang diagnostic pop
#endif
  return sched;
}

auto open_event_stream(
  filesystem::path const& path,
  FSEventStreamContext* fsev_ctx) -> FSEventStreamRef
{
  void const* path_cfstring =
    CFStringCreateWithCString(nullptr, path.c_str(), kCFStringEncodingUTF8);
  auto const path_array =
    CFArrayCreate(nullptr, &path_cfstring, 1, &kCFTypeArrayCallBacks);

  auto fsev_listen_since = kFSEventStreamEventIdSinceNow;
  unsigned fsev_listen_for = kFSEventStreamCreateFlagFileEvents
                           | kFSEventStreamCreateFlagUseExtendedData
                           | kFSEventStreamCreateFlagUseCFTypes;
  FSEventStreamRef stream = FSEventStreamCreate(
    nullptr,
    &event_recv,
    fsev_ctx,
    path_array,
    fsev_listen_since,
    0.016,
    fsev_listen_for);

#ifdef WTR_WATCHER_USE_UNSAFE_BUT_UNDEPRECATED_DISPATCH
  static auto on_scheduler =
    dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
#else
  auto on_scheduler = CFRunLoopGetCurrent();
#endif

  if (stream && on_scheduler) {
#ifdef WTR_WATCHER_USE_UNSAFE_BUT_UNDEPRECATED_DISPATCH
    FseventStreamSetDispatchQueue(stream, on_scheduler);
#else
    /*  Any warnings about using deprecated
        Core Foundation run-loop APIs are not
        relevant. We support dispatch. We just
        don't use it because it crashes under
        heavy load. Dispatch appears to call
        into garbage memory. Reallocations and
        frees on memory sometimes fails due to
        "not being allocated" or botched free
        list checks. These crashes are very
        rare and very real. */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    FSEventStreamScheduleWithRunLoop(
      stream,
      on_scheduler,
      kCFRunLoopDefaultMode);
#pragma clang diagnostic pop
#endif

    FSEventStreamStart(stream);
    return stream;
  }
  else
    return nullptr;
}

auto close_event_stream(FSEventStreamRef s) -> bool
{
  if (s) {
    FSEventStreamFlushSync(s);
    FSEventStreamStop(s);
    FSEventStreamInvalidate(s);
    FSEventStreamRelease(s);
    s = nullptr;
    return true;
  }
  else
    return false;
}

class StreamTask {
  enum class Error {
    AlreadyOpen,
    CreationFailed,
  };

private:
  enum class State {
    Fresh,
    Opening,
    Opened,
    Closing,
    Closed,
  };

  static constexpr uint64_t fsev_listen_since = kFSEventStreamEventIdSinceNow;
  static constexpr uint32_t fsev_listen_for =
    kFSEventStreamCreateFlagFileEvents | kFSEventStreamCreateFlagUseExtendedData
    | kFSEventStreamCreateFlagUseCFTypes;
  static constexpr double fsev_inactive_latency = 0.016;

  State state = State::Fresh;
  filesystem::path path = {};
  shared_ptr<LockedUniquePaths> user_ctx = nullptr;
  unique_ptr<FSEventStreamContext> fsev_ctx = nullptr;
  void const* path_cfstring = nullptr;
  CFArrayRef path_array = nullptr;
  FSEventStreamRef stream = nullptr;
  unique_ptr<thread> stream_thread{};
  Scheduler sched = nullptr;

public:
  auto close() -> void
  {
    if (this->state == State::Opened) {
      println("closing");
      this->state = State::Closing;
      if (this->stream) {
        FSEventStreamFlushSync(this->stream);
        FSEventStreamStop(this->stream);
        FSEventStreamInvalidate(this->stream);
        FSEventStreamRelease(this->stream);
        this->stream = nullptr;
        println("stream closed");
      }
      if (this->sched) {
        println("stopping sched");
        CFRunLoopStop(this->sched);
        this->sched = nullptr;
      }
      //if (this->path_cfstring) {
      //  CFRelease(this->path_cfstring);
      //  this->path_cfstring = nullptr;
      //}
      //if (this->path_array) {
      //  CFRelease(this->path_array);
      //  this->path_array = nullptr;
      //}
      if (this->stream_thread->joinable()) {
        println("joining");
        this->stream_thread->join();
      }
      this->state = State::Closed;
      // println("closed");
    }
  }

  auto open(filesystem::path const& p) -> optional<StreamTask::Error>
  {
    if (this->state != State::Fresh) { return StreamTask::Error::AlreadyOpen; }
    this->state = State::Opening;
    this->path = p;
    this->user_ctx = make_shared<LockedUniquePaths>();
    this->fsev_ctx = unique_ptr<FSEventStreamContext>(new FSEventStreamContext{
      .version = 0,
      .info = this->user_ctx.get(),
      .retain = nullptr,
      .release = nullptr,
      .copyDescription = nullptr,
    });
    this->path_cfstring = CFStringCreateWithCString(
      nullptr,
      this->path.c_str(),
      kCFStringEncodingUTF8);
    this->path_array =
      CFArrayCreate(nullptr, &this->path_cfstring, 1, &kCFTypeArrayCallBacks);
    this->stream = FSEventStreamCreate(
      nullptr,
      &event_recv,
      this->fsev_ctx.get(),
      this->path_array,
      StreamTask::fsev_listen_since,
      StreamTask::fsev_inactive_latency,
      StreamTask::fsev_listen_for);
    if (! this->stream) {
      this->close();
      return StreamTask::Error::CreationFailed;
    }
    this->stream_thread = make_unique<thread>(
      [this]()
      {
        this->sched = associate_fsevent_stream_with_scheduler(this->stream);
        FSEventStreamStart(this->stream);
        this->state = State::Opened;
        println("started with thread id ", this_thread::get_id());
        CFRunLoopRun();
      });
    this->stream_thread->detach();
    // while (this->state != State::Opened) { this_thread::yield(); }
    return nullopt;
  }

  ~StreamTask() { this->close(); }
};

struct Watcher {
  shared_ptr<LockedUniquePaths> user_ctx = make_shared<LockedUniquePaths>();
  unique_ptr<FSEventStreamContext> fsev_ctx =
    unique_ptr<FSEventStreamContext>(new FSEventStreamContext{
      .version = 0,
      .info = this->user_ctx.get(),
      .retain = nullptr,
      .release = nullptr,
      .copyDescription = nullptr,
    });
  FSEventStreamRef fsev_stream = nullptr;
};

static constexpr auto n_watchers = 5;
static constexpr auto event_count = 300;

auto work() -> void
{
  // Some events in the background
  for (int i = 0; i < event_count; i++) {
    char path_buf[256];
    memset(path_buf, 0, sizeof(path_buf));
    snprintf(path_buf, sizeof(path_buf), "/tmp/hi%d", i);
    FILE* f = fopen(path_buf, "w");
    if (! f) {
      println("error: ", errno);
      exit(1);
    }
    fclose(f);
    unlink(path_buf);
  }
}

auto tasked_main() -> int
{
  // Some number of watchers
  auto t_start = std::chrono::system_clock::now();
  auto watchers = array<StreamTask, n_watchers>{};
  for (auto& w : watchers) {
    auto err = w.open("/tmp");
    if (err) {
      println("error: ", (int)err.value());
      return 1;
    }
  }
  auto t_init = std::chrono::system_clock::now();
  work();
  auto t_work = std::chrono::system_clock::now();
  // Clean up (we never get here during a crash)
  println("ok");
  for (auto& w : watchers) { w.close(); }
  auto t_end = std::chrono::system_clock::now();
  auto t_init_dur =
    std::chrono::duration_cast<std::chrono::milliseconds>(t_init - t_start);
  auto t_work_dur =
    std::chrono::duration_cast<std::chrono::milliseconds>(t_work - t_init);
  auto t_end_dur =
    std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_work);
  println("(tasked_main) t init: ", t_init_dur.count(), "ms");
  println("(tasked_main) t work: ", t_work_dur.count(), "ms");
  println("(tasked_main) t end: ", t_end_dur.count(), "ms");
  return 0;
}

auto main() -> int try {
  return tasked_main();
} catch (std::exception const& e) {
  println("error: ", e.what());
  return 1;
} catch (...) {
  println("unknown error");
  return 1;
}
