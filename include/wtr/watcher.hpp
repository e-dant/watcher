#ifndef W973564ED9F278A21F3E12037288412FBAF175F889
#define W973564ED9F278A21F3E12037288412FBAF175F889

#include <array>
#include <charconv>
#include <chrono>
#include <filesystem>
#include <functional>
#include <ios>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>

namespace wtr {
inline namespace watcher {

/*  The `event` object is used to carry information about
    filesystem events to the user through the (user-supplied)
    callback given to `watch`.

    The `event` object will contain the:
      - `path_name`: The path to the event.
      - `path_type`: One of:
        - dir
        - file
        - hard_link
        - sym_link
        - watcher
        - other
      - `effect_type`: One of:
        - rename
        - modify
        - create
        - destroy
        - owner
        - other
      - `effect_time`:
        The time of the event in nanoseconds since epoch.

    The `watcher` type is special.
    Events with this type will include messages from
    the watcher. You may recieve error messages or
    important status updates.

    The first event always has a `create` value for the
    `effect_type`, a `watcher` value for the `path_type`,
    and a status message in the `path_name` field; Either
    "s/self/live@{some path}" or "e/self/live@{some path}".

    Similarly, the last event always carries `destroy` and
    `watcher` in the `effect_type` and `path_type` fields.
    The `path_name` field will have the same message as the
    first event, except for "die" instead of "live". */

struct event {

private:
  /*  I like these names. Very human. */
  using Nanos = std::chrono::nanoseconds;
  using Clock = std::chrono::system_clock;
  using TimePoint = std::chrono::time_point<Clock>;

public:
  /*  Ensure the user's callback can receive
      events and will return nothing. */
  using callback = std::function<void(event const&)>;

  /*  Represents "what happened" to a path. */
  enum class effect_type {
    rename,
    modify,
    create,
    destroy,
    owner,
    other,
  };

  /*  The essential types of paths. */
  enum class path_type {
    dir,
    file,
    hard_link,
    sym_link,
    watcher,
    other,
  };

  std::filesystem::path const path_name{};

  enum effect_type const effect_type {};

  enum path_type const path_type {};

  long long const effect_time{std::chrono::duration_cast<Nanos>(
                                TimePoint{Clock::now()}.time_since_epoch())
                                .count()};

  std::unique_ptr<event> const associated{nullptr};

  inline event(event const& from) noexcept
      : path_name{from.path_name}
      , effect_type{from.effect_type}
      , path_type{from.path_type}
      , effect_time{from.effect_time}
      , associated{
          from.associated ? std::make_unique<event>(*from.associated)
                          : nullptr} {};

  inline event(
    std::filesystem::path const& path_name,
    enum effect_type effect_type,
    enum path_type path_type) noexcept
      : path_name{path_name}
      , effect_type{effect_type}
      , path_type{path_type} {};

  inline event(event const& base, event&& associated) noexcept
      : path_name{base.path_name}
      , effect_type{base.effect_type}
      , path_type{base.path_type}
      , associated{std::make_unique<event>(std::forward<event>(associated))} {};

  inline ~event() noexcept = default;

  /*  An equality comparison for all the fields in this object.
      Includes the `effect_time`, which might not be wanted,
      because the `effect_time` is typically (not always) unique. */
  inline friend auto operator==(event const& l, event const& r) noexcept -> bool
  {
    return l.path_name == r.path_name && l.effect_time == r.effect_time
        && l.path_type == r.path_type && l.effect_type == r.effect_type
        && (l.associated && r.associated ? *l.associated == *r.associated
                                         : ! l.associated && ! r.associated);
  }

  inline friend auto operator!=(event const& l, event const& r) noexcept -> bool
  {
    return ! (l == r);
  }
};

} /*  namespace watcher */

// clang-format off

namespace {

#define wtr_effect_type_to_str_lit(Char, from, Lit) \
  switch (from) { \
    case ::wtr::event::effect_type::rename  : return Lit"rename"; \
    case ::wtr::event::effect_type::modify  : return Lit"modify"; \
    case ::wtr::event::effect_type::create  : return Lit"create"; \
    case ::wtr::event::effect_type::destroy : return Lit"destroy"; \
    case ::wtr::event::effect_type::owner   : return Lit"owner"; \
    case ::wtr::event::effect_type::other   : return Lit"other"; \
    default                                 : return Lit"other"; \
  }

#define wtr_path_type_to_str_lit(Char, from, Lit) \
  switch (from) { \
    case ::wtr::event::path_type::dir       : return Lit"dir"; \
    case ::wtr::event::path_type::file      : return Lit"file"; \
    case ::wtr::event::path_type::hard_link : return Lit"hard_link"; \
    case ::wtr::event::path_type::sym_link  : return Lit"sym_link"; \
    case ::wtr::event::path_type::watcher   : return Lit"watcher"; \
    case ::wtr::event::path_type::other     : return Lit"other"; \
    default                                 : return Lit"other"; \
  }

#define wtr_event_to_str_cls_as_json(Char, from, Lit)                         \
  using Cls = std::basic_string<Char>;                                        \
  auto&& etm = Lit"\"" + to<Cls>(from.effect_time) + Lit"\"";                 \
  auto&& ety = Lit"\"" + to<Cls>(from.effect_type) + Lit"\"";                 \
  auto&& pnm = Lit"\"" + to<Cls>(from.path_name)   + Lit"\"";                 \
  auto&& pty = Lit"\"" + to<Cls>(from.path_type)   + Lit"\"";                 \
  return {                         etm + Lit":{"                              \
         + Lit"\"effect_type\":" + ety + Lit","                               \
         + Lit"\"path_name\":"   + pnm + Lit","                               \
         + Lit"\"path_type\":"   + pty                                        \
         + [&]() -> Cls {                                                     \
              if (! from.associated) return Cls{};                            \
              auto asc = from.associated.get();                               \
              auto ttl = Cls{Lit",\"associated\""};                           \
              auto ety = Lit"\"" + to<Cls>(asc->effect_type) + Lit"\"";       \
              auto pnm = Lit"\"" + to<Cls>(asc->path_name)   + Lit"\"";       \
              auto pty = Lit"\"" + to<Cls>(asc->path_type)   + Lit"\"";       \
              return { ttl                                                    \
                     + Lit":{"                                                \
                     + Lit"\"effect_type\":" + ety + Lit","                   \
                     + Lit"\"path_name\":"   + pnm + Lit","                   \
                     + Lit"\"path_type\":"   + pty + Lit"}"                   \
                     };                                                       \
           }()                                                                \
         + Lit"}"                                                             \
         };

/*  For types larger than char and/or char8_t, we can just cast
    each element in our `char` buffer to an `unsigned char`, and
    then zero-extend the elements to any of `wchar_t`, `char16_t`
    and/or `char32_t`.
    We can use something like `format_to(chararray, "{}", from)`
    when library support is more common.
    If we support C++20 later on, we should parameterize `from`
    as `std::integral auto from` and add `char8_t` to the list
    of allowed narrow character types. */
template<class Char>
inline auto num_to_str(long long from) noexcept -> std::basic_string<Char> {
  static_assert(std::is_integral_v<decltype(from)>);
  static constexpr bool is_sys_sane_narrow_char_size =
      (sizeof(char) == sizeof(signed char))
      && (sizeof(char) == sizeof(unsigned char))
  ;
  static constexpr bool is_narrow_char = (
    is_sys_sane_narrow_char_size
    && (  std::is_same_v<Char, char>
       || std::is_same_v<Char, signed char>
       || std::is_same_v<Char, unsigned char>
    )
  );
  static constexpr bool is_wide_char = (
       std::is_same_v<Char, wchar_t>
    || std::is_same_v<Char, char16_t>
    || std::is_same_v<Char, char32_t>
  );
  static_assert(is_narrow_char || is_wide_char);

  static constexpr auto buflen = std::numeric_limits<decltype(from)>::digits10 + 1;
  auto buf = std::array<char, buflen>{0};
  auto [_, ec] = std::to_chars(buf.data(), buf.data() + buf.size(), from);
  if (ec != std::errc{}) {
    return {};
  }
  else if constexpr (is_narrow_char) {
    return {reinterpret_cast<Char const*>(buf.data()), buflen};
  }
  else if constexpr (is_wide_char) {
    auto xbuf = std::array<Char, buflen>{0};
    for (std::size_t i = 0; i < buflen; ++i)
      xbuf[i] = static_cast<Char>(static_cast<unsigned char>(buf[i]));
    return {xbuf.data(), buflen};
  }
};

} /*  namespace */

/*  We use function templates, as opposed to class templates with an
    overloaded () operator, so that we can avoid the extra () or {}
    when (needlessly) "constructing" the class and to make extending
    the templates once or more for the same type (a-la parameterize the
    return type *and* the arguments) possible.

    The `template<template<class...> class... Ts> auto to(auto) noexcept`
    is just for the user. It allows specialization outside of this file,
    and has no effect within this file (it does not guide deduction here).

    Some downsides:
    `std::hash` takes the class template approach. That's a (well?) known
    api. It's not ideal that we miss out on a user's existing familiarity
    with that pattern.
    We need to "declare" a function template before "defining" it:
    `template<class T> auto to(OurTypeOrConcept) -> T` must come before
    the `template<> auto to(...) -> T { ... }`. I'm not sure why class
    templates can get away without declaring a template like that, or
    why we need to do that at all.
    Another downside to the function templates is that we need to "declare"
    function templates before "defining" them. That's not exactly intuitive
    to me, I'm fuzzy on why that is, and users probably shouldn't need to
    remember that whenever they specialize this template.
    We need to declare a template before defining it:
    `template<class T> auto to(OurTypeOrConcept) -> T` must come before
    the `template<> auto to(...) -> T { ... }`. I'm not sure why class
    templates can get away without declaring a template like that, or
    why we need to do that at all.

    Beware of linker errors when using a concept as an argument type.
    AFAICT This is something that can't be done, or I haven't figured out
    how just yet.

    The upsides to function templates are:
    - We get to a concise api, `to<Type>(from_thing)`
    - The user can further specialize the function templates, i.e. We can
      have a specialization `to<string>(type_in_file_a)` and, somewhere else,
      `to<string>(type_in_file_b)` as well. Class templates cannot do that. */

template<class T, class... Ts> auto to(T) noexcept -> decltype(auto);

template<class T> auto to (long long                                from) noexcept -> T ;
template<class T> auto to (decltype(::wtr::event::effect_time)      from) noexcept -> T ;
template<class T> auto to (enum     ::wtr::event::effect_type       from) noexcept -> T ;
template<class T> auto to (decltype(::wtr::event::path_name) const& from) noexcept -> T ;
template<class T> auto to (enum     ::wtr::event::path_type         from) noexcept -> T ;
template<class T> auto to (         ::wtr::event             const& from) noexcept -> T ;

template<> inline constexpr auto to<std::basic_string_view<char>>   (enum     ::wtr::event::effect_type       from) noexcept -> std::basic_string_view<char>    { wtr_effect_type_to_str_lit(  char,    from,   ""          );  }
template<> inline constexpr auto to<std::basic_string_view<char>>   (enum     ::wtr::event::path_type         from) noexcept -> std::basic_string_view<char>    { wtr_path_type_to_str_lit(    char,    from,   ""          );  }
template<> inline           auto to<std::basic_string     <char>>   (decltype(::wtr::event::path_name) const& from) noexcept -> std::basic_string<char>         { return {                     from.string()                };  }
template<> inline           auto to<std::basic_string     <char>>   (enum     ::wtr::event::path_type         from) noexcept -> std::basic_string<char>         { wtr_path_type_to_str_lit(    char,    from,   ""          );  }
template<> inline           auto to<std::basic_string     <char>>   (decltype(::wtr::event::effect_time)      from) noexcept -> std::basic_string<char>         { return num_to_str           <char>(   from                );  }
template<> inline           auto to<std::basic_string     <char>>   (enum     ::wtr::event::effect_type       from) noexcept -> std::basic_string<char>         { wtr_effect_type_to_str_lit(  char,    from,   ""          );  }
template<> inline           auto to<std::basic_string     <char>>   (         ::wtr::event             const& from) noexcept -> std::basic_string<char>         { wtr_event_to_str_cls_as_json(char,    from,   ""          );  }
template<> inline constexpr auto to<std::basic_string_view<wchar_t>>(enum     ::wtr::event::effect_type       from) noexcept -> std::basic_string_view<wchar_t> { wtr_effect_type_to_str_lit(  wchar_t, from,  L""          );  }
template<> inline constexpr auto to<std::basic_string_view<wchar_t>>(enum     ::wtr::event::path_type         from) noexcept -> std::basic_string_view<wchar_t> { wtr_path_type_to_str_lit(    wchar_t, from,  L""          );  }
template<> inline           auto to<std::basic_string     <wchar_t>>(enum     ::wtr::event::path_type         from) noexcept -> std::basic_string<wchar_t>      { wtr_path_type_to_str_lit(    wchar_t, from,  L""          );  }
template<> inline           auto to<std::basic_string     <wchar_t>>(decltype(::wtr::event::path_name) const& from) noexcept -> std::basic_string<wchar_t>      { return {                     from.wstring()               };  }
template<> inline           auto to<std::basic_string     <wchar_t>>(decltype(::wtr::event::effect_time)      from) noexcept -> std::basic_string<wchar_t>      { return num_to_str           <wchar_t>(from                );  }
template<> inline           auto to<std::basic_string     <wchar_t>>(enum     ::wtr::event::effect_type       from) noexcept -> std::basic_string<wchar_t>      { wtr_effect_type_to_str_lit(  wchar_t, from,  L""          );  }
template<> inline           auto to<std::basic_string     <wchar_t>>(         ::wtr::event             const& from) noexcept -> std::basic_string<wchar_t>      { wtr_event_to_str_cls_as_json(wchar_t, from,  L""          );  }
/*
template<> inline constexpr auto to<std::basic_string_view<char8_t>>(enum     ::wtr::event::effect_type       from) noexcept -> std::basic_string_view<char8_t> { wtr_effect_type_to_str_lit(  char8_t, from, u8""          );  }
template<> inline constexpr auto to<std::basic_string_view<char8_t>>(enum     ::wtr::event::path_type         from) noexcept -> std::basic_string_view<char8_t> { wtr_path_type_to_str_lit(    char8_t, from, u8""          );  }
template<> inline           auto to<std::basic_string     <char8_t>>(enum     ::wtr::event::path_type         from) noexcept -> std::basic_string<char8_t>      { wtr_path_type_to_str_lit(    char8_t, from, u8""          );  }
template<> inline           auto to<std::basic_string     <char8_t>>(decltype(::wtr::event::path_name) const& from) noexcept -> std::basic_string<char8_t>      { return {                     from.u8string()              };  }
template<> inline           auto to<std::basic_string     <char8_t>>(decltype(::wtr::event::effect_time)      from) noexcept -> std::basic_string<char8_t>      { return num_to_str           <char8_t>(from                );  }
template<> inline           auto to<std::basic_string     <char8_t>>(enum     ::wtr::event::effect_type       from) noexcept -> std::basic_string<char8_t>      { wtr_effect_type_to_str_lit(  char8_t, from, u8""          );  }
template<> inline           auto to<std::basic_string     <char8_t>>(         ::wtr::event             const& from) noexcept -> std::basic_string<char8_t>      { wtr_event_to_str_cls_as_json(char8_t, from, u8""          );  }
*/

#undef wtr_effect_type_to_str_lit
#undef wtr_path_type_to_str_lit
#undef wtr_event_to_str_cls_as_json

template<class Char, class Traits>
inline auto operator<<(
  std::basic_ostream<Char, Traits>& into,
  enum ::wtr::event::effect_type from) noexcept
-> std::basic_ostream<Char, Traits>&
{ return into << to<std::basic_string<Char, Traits>>(from); };

template<class Char, class Traits>
inline auto operator<<(
  std::basic_ostream<Char, Traits>& into,
  enum ::wtr::event::path_type from) noexcept
-> std::basic_ostream<Char, Traits>&
{ return into << to<std::basic_string<Char, Traits>>(from); };

/*  Streams out `path_name`, `effect_type` and `path_type`.
    Formats the stream as a json object.
    Looks like this (without line breaks)
      "1678046920675963000":{
       "effect_type":"create",
       "path_name":"/some_file.txt",
       "path_type":"file"
      } */
template<class Char, class Traits>
inline auto operator<<(
  std::basic_ostream<Char, Traits>& into,
  ::wtr::event const& from) noexcept
-> std::basic_ostream<Char, Traits>&
{ return into << to<std::basic_string<Char, Traits>>(from); };

// clang-format on

} /*  namespace wtr   */

#include <atomic>

#ifdef __linux__
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <sys/eventfd.h>
#include <unistd.h>
#endif

namespace detail::wtr::watcher {

/*  A semaphore which is pollable on Linux.
    On all platforms, this behaves like an
    atomic boolean flag. (On non-Linux, this
    literally is an atomic boolean flag.)
    On Linux, this is an eventfd in semaphore
    mode, which means that it can be waited on
    with poll() and friends. */

class semabin {
public:
  enum state { pending, released, error };

private:
  mutable std::atomic<state> is = pending;

public:
#ifdef __linux__

  int const fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK | EFD_SEMAPHORE);

  inline auto release() noexcept -> state
  {
    auto write_ev = [this]()
    {
      if (eventfd_write(this->fd, 1) == 0)
        return released;
      else
        return error;
    };

    if (this->is == released)
      return released;
    else
      return this->is = write_ev();
  }

  inline auto state() const noexcept -> state
  {
    auto read_ev = [this]()
    {
      uint64_t _ = 0;
      if (eventfd_read(this->fd, &_) == 0)
        return released;
      else if (errno == EAGAIN)
        return pending;
      else
        return error;
    };

    if (this->is == pending)
      return pending;
    else
      return this->is = read_ev();
  }

  inline ~semabin() noexcept { close(this->fd); }

#else

  inline auto release() noexcept -> enum state { return this->is = released; }

  inline auto state() const noexcept -> enum state { return this->is; }

#endif
};

} /*  namespace detail::wtr::watcher */

#if defined(__APPLE__)

#include <atomic>
#include <chrono>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#include <cstdio>
#include <filesystem>
#include <limits>
#include <random>
#include <string>
#include <thread>
#include <tuple>
#include <unistd.h>
#include <unordered_set>
#include <vector>

namespace detail {
namespace wtr {
namespace watcher {
namespace adapter {
namespace {

struct argptr_type {
  using fspath = std::filesystem::path;
  /*  `fs::path` has no hash function, so we use this. */
  using pathset = std::unordered_set<std::string>;
  ::wtr::watcher::event::callback const callback{};
  std::shared_ptr<pathset> seen_created_paths{new pathset{}};
  std::shared_ptr<fspath> last_rename_from_path{new fspath{}};
};

struct sysres_type {
  FSEventStreamRef stream{};
  argptr_type argptr{};
};

inline auto path_from_event_at(void* event_recv_paths, unsigned long i) noexcept
  -> std::filesystem::path
{
  /*  We make a path from a C string...
      In an array, in a dictionary...
      Without type safety...
      Because most of darwin's apis are `void*`-typed.

      We should be guarenteed that nothing in here is
      or can be null, but I'm skeptical. We ask Darwin
      for utf8 strings from a dictionary of utf8 strings
      which it gave us. Nothing should be able to be null.
      We'll check anyway, just in case Darwin lies.

      The dictionary contains looks like this:
        { "path": String
        , "fileID": Number
        }
      We can only call `CFStringGetCStringPtr()`
      on the `path` field. Not sure what function
      the `fileID` requires, or if it's different
      from what we'd get from `stat()`. (Is it an
      inode number?) Anyway, we seem to get this:
        -[__NSCFNumber length]: unrecognized ...
      Whenever we try to inspect it with Int or
      CStringPtr functions for CFStringGet...().
      The docs don't say much about these fields.
      I don't think they mention fileID at all.
  */
  namespace fs = std::filesystem;

  if (event_recv_paths)
    if (
      void const* from_arr = CFArrayGetValueAtIndex(
        static_cast<CFArrayRef>(event_recv_paths),
        static_cast<CFIndex>(i)))
      if (
        void const* from_dict = CFDictionaryGetValue(
          static_cast<CFDictionaryRef>(from_arr),
          kFSEventStreamEventExtendedDataPathKey))
        if (
          char const* as_cstr = CFStringGetCStringPtr(
            static_cast<CFStringRef>(from_dict),
            kCFStringEncodingUTF8))
          return fs::path{as_cstr};
  return fs::path{};
}

inline auto event_recv_one(
  argptr_type& ctx,
  std::filesystem::path const& path,
  unsigned flags)
{
  namespace fs = std::filesystem;

  using path_type = enum ::wtr::watcher::event::path_type;
  using effect_type = enum ::wtr::watcher::event::effect_type;

  static constexpr unsigned fsev_flag_path_file =
    kFSEventStreamEventFlagItemIsFile;
  static constexpr unsigned fsev_flag_path_dir =
    kFSEventStreamEventFlagItemIsDir;
  static constexpr unsigned fsev_flag_path_sym_link =
    kFSEventStreamEventFlagItemIsSymlink;
  static constexpr unsigned fsev_flag_path_hard_link =
    kFSEventStreamEventFlagItemIsHardlink
    | kFSEventStreamEventFlagItemIsLastHardlink;

  static constexpr unsigned fsev_flag_effect_create =
    kFSEventStreamEventFlagItemCreated;
  static constexpr unsigned fsev_flag_effect_remove =
    kFSEventStreamEventFlagItemRemoved;
  static constexpr unsigned fsev_flag_effect_modify =
    kFSEventStreamEventFlagItemModified
    | kFSEventStreamEventFlagItemInodeMetaMod
    | kFSEventStreamEventFlagItemFinderInfoMod
    | kFSEventStreamEventFlagItemChangeOwner
    | kFSEventStreamEventFlagItemXattrMod;
  static constexpr unsigned fsev_flag_effect_rename =
    kFSEventStreamEventFlagItemRenamed;

  static constexpr unsigned fsev_flag_effect_any =
    fsev_flag_effect_create | fsev_flag_effect_remove | fsev_flag_effect_modify
    | fsev_flag_effect_rename;

  auto [callback, sc_paths, lrf_path] = ctx;

  auto path_str = path.string();

  /*  A single path won't have different "types". */
  auto pt = flags & fsev_flag_path_file      ? path_type::file
          : flags & fsev_flag_path_dir       ? path_type::dir
          : flags & fsev_flag_path_sym_link  ? path_type::sym_link
          : flags & fsev_flag_path_hard_link ? path_type::hard_link
                                             : path_type::other;

  /*  We want to report odd events (even with an empty path)
      but we can bail early if we don't recognize the effect
      because everything else we do depends on that. */
  if (! (flags & fsev_flag_effect_any)) {
    callback({path, effect_type::other, pt});
    return;
  }

  /*  More than one effect might have happened to the
      same path. (Which is why we use non-exclusive `if`s.) */
  if (flags & fsev_flag_effect_create) {
    auto at = sc_paths->find(path_str);
    if (at == sc_paths->end()) {
      sc_paths->emplace(path_str);
      callback({path, effect_type::create, pt});
    }
  }
  if (flags & fsev_flag_effect_remove) {
    auto at = sc_paths->find(path_str);
    if (at != sc_paths->end()) {
      sc_paths->erase(at);
      callback({path, effect_type::destroy, pt});
    }
  }
  if (flags & fsev_flag_effect_modify) {
    callback({path, effect_type::modify, pt});
  }
  if (flags & fsev_flag_effect_rename) {
    /*  Assumes that the last "renamed-from" path
        if "honestly" correlated to the current
        "rename-to" path.
        For non-destructive rename events, we
        usually receive events in this order:
          1. A rename event on the "from-path"
          2. A rename event on the "to-path"
        As long as that pattern holds, we can
        store the first path in a set, look it
        up, test it against the current path
        for inequality, and check that it no
        longer exists -- In which case, we can
        say that we were renamed from that path
        to the current path.
        We want to store the last rename-from
        path in a set on the heap because the
        rename events might not be batched, and
        we don't want to trample on some other
        watcher with a static.
        This pattern breaks down if there are
        intervening rename events.
        For thoughts on recognizing destructive
        rename events, see this directory's
        notes (in the `notes.md` file).
    */
    auto differs = ! lrf_path->empty() && *lrf_path != path;
    auto missing = access(lrf_path->c_str(), F_OK) == -1;
    if (differs && missing) {
      callback({
        {*lrf_path, effect_type::rename, pt},
        {     path, effect_type::rename, pt}
      });
      lrf_path->clear();
    }
    else {
      *lrf_path = path;
    }
  }
}

/*  Sometimes events are batched together and re-sent
    (despite having already been sent).
    Example:
      [first batch of events from the os]
      file 'a' created
      -> create event for 'a' is sent
      [some tiny delay, 1 ms or so]
      [second batch of events from the os]
      file 'a' destroyed
      -> create event for 'a' is sent
      -> destroy event for 'a' is sent
    So, we filter out duplicate events when they're sent
    in a batch. We do this by storing and pruning the
    set of paths which we've seen created. */
inline auto event_recv(
  ConstFSEventStreamRef,      /*  `ConstFS..` is important */
  void* ctx,                  /*  Arguments passed to us */
  unsigned long count,        /*  Event count */
  void* paths,                /*  Paths with events */
  unsigned const* flags,      /*  Event flags */
  FSEventStreamEventId const* /*  event stream id */
  ) noexcept -> void
{
  if (! ctx || ! paths || ! flags) return;
  auto& ap = *static_cast<argptr_type*>(ctx);
  for (unsigned long i = 0; i < count; i++)
    event_recv_one(ap, path_from_event_at(paths, i), flags[i]);
}

/*  Make sure that event_recv has the same type as, or is
    convertible to, an FSEventStreamCallback. We don't use
    `is_same_v()` here because `event_recv` is `noexcept`.
    Side note: Is an exception qualifier *really* part of
    the type? Or, is it a "path_type"? Something else?
    We want this assertion for nice compiler errors. */
static_assert(FSEventStreamCallback{event_recv} == event_recv);

inline auto open_event_stream(
  std::filesystem::path const& path,
  ::wtr::watcher::event::callback const& callback) noexcept
  -> std::tuple<bool, std::shared_ptr<sysres_type>>
{
  using namespace std::chrono_literals;
  using std::chrono::duration_cast, std::chrono::seconds;

  auto sysres =
    std::make_shared<sysres_type>(sysres_type{nullptr, argptr_type{callback}});

  auto context = FSEventStreamContext{
    /*  FSEvents.h:
        "Currently the only valid value is zero." */
    .version = 0,
    /*  The "actual" context; our "argument pointer".
        This must be alive until we clear the event stream. */
    .info = static_cast<void*>(&sysres->argptr),
    /*  An optional "retention" callback. We don't need this
        because we manage `argptr`'s lifetime ourselves. */
    .retain = nullptr,
    /*  An optional "release" callback, not needed for the same
        reasons as the retention callback. */
    .release = nullptr,
    /*  An optional string for debugging purposes. */
    .copyDescription = nullptr,
  };

  void const* path_cfstring =
    CFStringCreateWithCString(nullptr, path.c_str(), kCFStringEncodingUTF8);
  CFArrayRef path_array = CFArrayCreate(
    /*  A custom allocator is optional */
    nullptr,
    /*  Data: A pointer-to-pointer of (in our case) strings */
    &path_cfstring,
    /*  We're just storing one path here */
    1,
    /*  A predefined structure which is "appropriate for use
        when the values in a CFArray are all CFTypes" (CFArray.h) */
    &kCFTypeArrayCallBacks);

  /*  If we want less "sleepy" time after a period of time
      without receiving filesystem events, we could OR like:
      `fsev_flag_listen | kFSEventStreamCreateFlagNoDefer`.
      We're talking about saving a maximum latency of `delay_s`
      after some period of inactivity, which is not likely to
      be noticeable. I'm not sure what Darwin sets the "period
      of inactivity" to, and I'm not sure it matters. */
  static constexpr unsigned fsev_flag_listen =
    kFSEventStreamCreateFlagFileEvents | kFSEventStreamCreateFlagUseExtendedData
    | kFSEventStreamCreateFlagUseCFTypes;

  /*  Request a filesystem event stream for `path` from the
      kernel. The event stream will call `event_recv` with
      `context` and some details about each filesystem event
      the kernel sees for the paths in `path_array`. */
  if (
    FSEventStreamRef stream = FSEventStreamCreate(
      /*  A custom allocator is optional */
      nullptr,
      /*  A callable to invoke on changes */
      &event_recv,
      /*  The callable's arguments (context) */
      &context,
      /*  The path(s) we were asked to watch */
      path_array,
      /*  The time "since when" we receive events */
      kFSEventStreamEventIdSinceNow,
      /*  The time between scans *after inactivity* */
      (0.016s).count(),
      /*  The event stream flags */
      fsev_flag_listen)) {
    FSEventStreamSetDispatchQueue(
      stream,
      /*  We don't need to retain, maintain or release this
          dispatch queue. It's a global system queue, and it
          outlives us. */
      dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0));

    FSEventStreamStart(stream);

    sysres->stream = stream;

    /*  todo: Do we need to release these?
        CFRelease(path_cfstring);
        CFRelease(path_array);
    */

    return {true, sysres};
  }
  else {
    return {false, {}};
  }
}

inline auto
close_event_stream(std::shared_ptr<sysres_type> const& sysres) noexcept -> bool
{
  if (sysres->stream) {
    /*  We want to handle any outstanding events before closing. */
    FSEventStreamFlushSync(sysres->stream);
    /*  `FSEventStreamInvalidate()` only needs to be called
        if we scheduled via `FSEventStreamScheduleWithRunLoop()`.
        That scheduling function is deprecated (as of macOS 13).
        Calling `FSEventStreamInvalidate()` fails an assertion
        and produces a warning in the console. However, calling
        `FSEventStreamRelease()` without first invalidating via
        `FSEventStreamInvalidate()` *also* fails an assertion,
        and produces a warning. I'm not sure what the right call
        to make here is. */
    FSEventStreamStop(sysres->stream);
    FSEventStreamInvalidate(sysres->stream);
    FSEventStreamRelease(sysres->stream);
    sysres->stream = nullptr;
    return true;
  }
  else
    return false;
}

inline auto block_while(semabin const& is_living)
{
  using namespace std::chrono_literals;
  using std::this_thread::sleep_for;

  while (is_living.state() == semabin::state::pending) sleep_for(16ms);
}

} /*  namespace */

inline auto watch(
  std::filesystem::path const& path,
  ::wtr::watcher::event::callback const& callback,
  semabin const& is_living) noexcept -> bool
{
  auto&& [ok, sysres] = open_event_stream(path, callback);
  return ok ? (block_while(is_living), close_event_stream(sysres)) : false;
}

} /*  namespace adapter */
} /*  namespace watcher */
} /*  namespace wtr   */
} /*  namespace detail */

#endif

#if (defined(__linux__) || __ANDROID_API__) \
  && ! defined(WATER_WATCHER_USE_WARTHOG)

#include <cstring>
#include <dirent.h>
#include <functional>
#include <stdio.h>
#include <string>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace detail::wtr::watcher::adapter {

struct ep {
  /*  Number of events allowed to be
      given to do_event_recv. The same
      value is usually returned by a
      call to `epoll_wait`). Any number
      between 1 and INT_MAX should be
      fine... But lower is fine for us.
      We don't lose events if we 'miss'
      them, the events are still waiting
      in the next call to `epoll_wait`.
  */
  static constexpr auto q_ulim = 64;
  /*  The delay, in milliseconds, while
      `epoll_wait` will 'pause' us for
      until we are woken up. We check if
      we are still alive through the fd
      from our semaphore-like eventfd.
  */
  static constexpr auto wake_ms = -1;

  int fd = -1;
  epoll_event interests[q_ulim]{};
};

inline auto do_error =
  [](std::string&& msg, char const* const path, auto const& cb) -> bool
{
  using et = enum ::wtr::watcher::event::effect_type;
  using pt = enum ::wtr::watcher::event::path_type;
  cb({msg + path, et::other, pt::watcher});
  return false;
};

inline auto do_warn =
  [](std::string&& msg, auto const& path, auto const& cb) -> bool
{ return (do_error(std::move(msg), path, cb), true); };

inline auto make_ep = [](
                        char const* const base_path,
                        auto const& cb,
                        int ev_il_fd,
                        int ev_fs_fd) -> ep
{
  auto do_error = [&](auto&& msg)
  { return (adapter::do_error(msg, base_path, cb), ep{}); };
#if __ANDROID_API__
  int fd = epoll_create(1);
#else
  int fd = epoll_create1(EPOLL_CLOEXEC);
#endif
  auto want_ev_fs = epoll_event{.events = EPOLLIN, .data{.fd = ev_fs_fd}};
  auto want_ev_il = epoll_event{.events = EPOLLIN, .data{.fd = ev_il_fd}};
  bool ctl_ok = epoll_ctl(fd, EPOLL_CTL_ADD, ev_fs_fd, &want_ev_fs) >= 0
             && epoll_ctl(fd, EPOLL_CTL_ADD, ev_il_fd, &want_ev_il) >= 0;
  return fd < 0   ? do_error("e/sys/epoll_create@")
       : ! ctl_ok ? (close(fd), do_error("e/sys/epoll_ctl@"))
                  : ep{.fd = fd};
};

inline auto is_dir(char const* const path) -> bool
{
  struct stat s;
  return stat(path, &s) == 0 && S_ISDIR(s.st_mode);
}

inline auto strany = [](char const* const s, auto... cmp) -> bool
{ return ((strcmp(s, cmp) == 0) || ...); };

/*  $ echo time wtr.watcher / -ms 1
      | sudo bash -E
      ...
      real 0m25.094s
      user 0m4.091s
      sys  0m20.856s

    $ sudo find / -type d
      | wc -l
      ...
      784418

    We could parallelize this, but
    it's never going to be instant.

    It might be worth it to start
    watching before we're done this
    hot find-and-mark path, despite
    not having a full picture.
*/
template<class Fn>
inline auto walkdir_do(char const* const path, Fn const& f) -> void
{
  auto pappend = [&](char* head, char* tail)
  { return snprintf(head, PATH_MAX, "%s/%s", path, tail); };
  if (DIR* d = opendir(path)) {
    f(path);
    while (dirent* de = readdir(d)) {
      char next[PATH_MAX];
      char real[PATH_MAX];
      if (de->d_type != DT_DIR) continue;
      if (strany(de->d_name, ".", "..")) continue;
      if (pappend(next, de->d_name) <= 0) continue;
      if (! realpath(next, real)) continue;
      walkdir_do(real, f);
    }
    (void)closedir(d);
  }
}

/*  We aren't worried about losing data after
    a failed call to `close()` (whereas closing
    a file descriptor in use for, say, writing
    would be a problem). Linux will eventually
    close the file descriptor regardless of the
    return value of `close()`, so we always set
    the file descriptors to -1 during cleanup
    to avoid double-closing or checking these
    descriptors for events.
    We are only interested in failures about
    bad file descriptors. We would probably
    hit that if we failed to create a valid
    file descriptor, on, say, an out-of-fds
    device or a machine running some odd OS.
    Everything else is fine to pass on.
*/
inline auto close_sysres = [](auto& sr) -> bool
{
  sr.ok = false;
  close(sr.ke.fd);
  close(sr.ep.fd);
  sr.ke.fd = sr.ep.fd = -1;
  return errno != EBADF;
};

} /*  namespace detail::wtr::watcher::adapter */

#endif

#if (defined(__linux__) || __ANDROID_API__) \
  && ! defined(WATER_WATCHER_USE_WARTHOG)

#include <linux/version.h>

#if (KERNEL_VERSION(5, 9, 0) <= LINUX_VERSION_CODE) && ! __ANDROID_API__

#include <cassert>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <functional>
#include <optional>
#include <sys/epoll.h>
#include <sys/fanotify.h>
#include <system_error>
#include <tuple>
#include <unistd.h>
#include <unordered_map>
#include <unordered_set>

namespace detail::wtr::watcher::adapter::fanotify {

/*  We request post-event reporting, non-blocking
    IO and unlimited marks for fanotify. We need
    sudo mode for the unlimited marks. If we were
    making a filesystem auditor, we might use:
      FAN_CLASS_PRE_CONTENT
      | FAN_UNLIMITED_QUEUE
      | FAN_UNLIMITED_MARKS
    and some writable fields within the callback.
*/

// clang-format off
struct ke_fa_ev {
  static constexpr auto buf_len = 4096;
  static constexpr auto c_ulim = buf_len / sizeof(fanotify_event_metadata);
  static constexpr auto init_io_flags
    = O_RDONLY
    | O_CLOEXEC
    | O_NONBLOCK;
  static constexpr auto init_flags
    = FAN_CLASS_NOTIF
    | FAN_REPORT_FID
    | FAN_REPORT_DIR_FID
    | FAN_REPORT_NAME
    | FAN_UNLIMITED_QUEUE
    | FAN_UNLIMITED_MARKS
    | FAN_NONBLOCK;
  static constexpr auto recv_flags
    = FAN_ONDIR
    | FAN_CREATE
    | FAN_MODIFY
    // | FAN_ATTRIB todo: Support change of ownership
    | FAN_MOVE
    | FAN_DELETE
    | FAN_EVENT_ON_CHILD;

  int fd = -1;
  alignas(fanotify_event_metadata) char buf[buf_len]{0};
};

// clang-format on

struct sysres {
  bool ok = false;
  ke_fa_ev ke{};
  semabin const& il{};
  adapter::ep ep{};
};

inline auto do_mark =
  [](char const* const dirpath, int fa_fd, auto const& cb) -> bool
{
  auto do_error = [&]() -> bool
  { return adapter::do_error("w/sys/not_watched@", dirpath, cb); };
  char real[PATH_MAX];
  if (! realpath(dirpath, real)) return do_error();
  int anonymous_wd =
    fanotify_mark(fa_fd, FAN_MARK_ADD, ke_fa_ev::recv_flags, AT_FDCWD, real);
  return anonymous_wd == 0 ? true : is_dir(real) ? do_error() : false;
};

/*  Grabs the resources we need from `fanotify`
    and `epoll`. Marks itself invalid on errors,
    sends diagnostics on warnings and errors.
    Walks the given base path, recursively,
    marking each directory along the way. */
inline auto make_sysres = [](
                            char const* const base_path,
                            auto const& cb,
                            semabin const& is_living) -> sysres
{
  auto do_error = [&](auto&& msg)
  { return (adapter::do_error(std::move(msg), base_path, cb), sysres{}); };
  int fa_fd = fanotify_init(ke_fa_ev::init_flags, ke_fa_ev::init_io_flags);
  if (fa_fd < 1) return do_error("e/sys/fanotify_init@");
  walkdir_do(base_path, [&](auto dir) { do_mark(dir, fa_fd, cb); });
  auto ep = make_ep(base_path, cb, is_living.fd, fa_fd);
  if (ep.fd < 1) return (close(fa_fd), do_error("e/self/resource@"));
  return {
    .ok = true,
    .ke{.fd = fa_fd},
    .il = is_living,
    .ep = ep,
  };
};

/*  Parses a full path from an event's metadata.
    The shenanigans we do here depend on this event being
    `FAN_EVENT_INFO_TYPE_DFID_NAME`. The kernel passes us
    some info about the directory and the directory entry
    (the filename) of this event that doesn't exist when
    other event types are reported. In particular, we need
    a file descriptor to the directory (which we use
    `readlink` on) and a character string representing the
    name of the directory entry.
    TLDR: We need information for the full path of the
    event, information which is only reported inside this
    `if`.
    From the kernel:
    Variable size struct for
    dir file handle + child file handle + name
    [ Omitting definition of `fanotify_info` here ]
    (struct fanotify_fh)
      dir_fh starts at
        buf[0] (optional)
      dir2_fh starts at
        buf[dir_fh_totlen] (optional)
      file_fh starts at
        buf[dir_fh_totlen+dir2_fh_totlen]
      name starts at
        buf[dir_fh_totlen+dir2_fh_totlen+file_fh_totlen]
      ...
    The kernel guarentees that there is a null-terminated
    character string to the event's directory entry
    after the file handle to the directory.
    Confusing, right? */
inline auto pathof(fanotify_event_metadata const* const mtd) -> std::string
{
  constexpr size_t path_ulim = PATH_MAX - sizeof('\0');
  auto dir_info = (fanotify_event_info_fid*)(mtd + 1);
  auto dir_fh = (file_handle*)(dir_info->handle);

  char path_buf[PATH_MAX];
  ssize_t file_name_offset = 0;

  /*  Directory name */
  {
    constexpr int ofl = O_RDONLY | O_CLOEXEC | O_PATH;
    int fd = open_by_handle_at(AT_FDCWD, dir_fh, ofl);
    if (fd > 0) {
      /*  If we have a pid with more than 128 digits... Well... */
      char fs_ev_pidpath[128];
      snprintf(fs_ev_pidpath, sizeof(fs_ev_pidpath), "/proc/self/fd/%d", fd);
      file_name_offset = readlink(fs_ev_pidpath, path_buf, path_ulim);
      close(fd);
    }
    path_buf[file_name_offset] = 0;
    /*  todo: Is is an error if we can't get the directory name? */
  }

  /*  File name ("Directory entry")
      If we wrote the directory name before here, we
      can start writing the file name after its offset. */
  {
    if (file_name_offset > 0) {
      char* file_name = ((char*)dir_fh->f_handle + dir_fh->handle_bytes);
      auto not_selfdir = strcmp(file_name, ".") != 0;
      auto beg = path_buf + file_name_offset;
      auto end = PATH_MAX - file_name_offset;
      if (file_name && not_selfdir) snprintf(beg, end, "/%s", file_name);
    }
  }

  return {path_buf};
}

inline auto peek(fanotify_event_metadata const* const m, size_t read_len)
  -> fanotify_event_metadata const*
{
  if (m) {
    auto ev_len = m->event_len;
    auto next = (fanotify_event_metadata*)((char*)m + ev_len);
    return FAN_EVENT_OK(next, read_len - ev_len) ? next : nullptr;
  }
  else
    return nullptr;
}

struct Parsed {
  ::wtr::watcher::event ev;
  fanotify_event_metadata const* next = nullptr;
  unsigned this_len = 0;
};

inline auto parse_ev(fanotify_event_metadata const* const m, size_t read_len)
  -> Parsed
{
  using ev = ::wtr::watcher::event;
  using ev_pt = enum ev::path_type;
  using ev_et = enum ev::effect_type;
  auto n = peek(m, read_len);
  auto pt = m->mask & FAN_ONDIR ? ev_pt::dir : ev_pt::file;
  auto et = m->mask & FAN_CREATE ? ev_et::create
          : m->mask & FAN_DELETE ? ev_et::destroy
          : m->mask & FAN_MODIFY ? ev_et::modify
          : m->mask & FAN_MOVE   ? ev_et::rename
                                 : ev_et::other;
  auto isfromto = [et](unsigned a, unsigned b) -> bool
  { return et == ev_et::rename && a & FAN_MOVED_FROM && b & FAN_MOVED_TO; };
  auto e = [et, pt](auto*... m) -> ev { return ev(ev(pathof(m), et, pt)...); };
  auto one = [&](auto* m) -> Parsed { return {e(m), n, m->event_len}; };
  auto assoc = [&](auto* m, auto* n) -> Parsed
  {
    auto nn = peek(n, read_len);
    auto here_to_nnn = m->event_len + n->event_len;
    return {e(m, n), nn, here_to_nnn};
  };
  return ! n                        ? one(m)
       : isfromto(m->mask, n->mask) ? assoc(m, n)
       : isfromto(n->mask, m->mask) ? assoc(n, m)
                                    : one(m);
}

inline auto do_mark_if_newdir =
  [](::wtr::watcher::event const& ev, int fa_fd, auto const& cb) -> bool
{
  auto is_newdir = ev.effect_type == ::wtr::watcher::event::effect_type::create
                && ev.path_type == ::wtr::watcher::event::path_type::dir;
  if (is_newdir)
    return do_mark(ev.path_name.c_str(), fa_fd, cb);
  else
    return true;
};

/*  Read some events from what fanotify gives
    us. Sends (the valid) events to the user's
    callback. Send a diagnostic to the user on
    warnings and errors. Returns false on errors.
    True otherwise.
    Notes:
    The `metadata->fd` field contains either a
    file descriptor or the value `FAN_NOFD`. File
    descriptors are always greater than 0 (but we
    will get the FAN_NOFD value, and we can grab
    other information from the /proc/self/<fd>
    filesystem, which we do in `pathof()`).
    `FAN_NOFD` represents an event queue overflow
    for `fanotify` listeners which are *not*
    monitoring file handles, such as mount/volume
    watchers. The file handle is in the metadata
    when an `fanotify` listener is monitoring
    events by their file handles.
    The `metadata->vers` field may differ between
    kernel versions, so we check it against the
    version we were compiled with. */
inline auto do_ev_recv =
  [](char const* const base_path, auto const& cb, sysres& sr) -> bool
{
  auto do_error = [&](auto&& msg) -> bool
  { return adapter::do_error(msg, base_path, cb); };
  auto do_warn = [&](auto&& msg) -> bool { return ! do_error(msg); };
  auto ev_info = [](fanotify_event_metadata const* const m)
  { return (fanotify_event_info_fid*)(m + 1); };
  auto ev_has_dirname = [&](fanotify_event_metadata const* const m) -> bool
  { return ev_info(m)->hdr.info_type == FAN_EVENT_INFO_TYPE_DFID_NAME; };

  unsigned ev_c = 0;
  memset(sr.ke.buf, 0, sr.ke.buf_len);
  int read_len = read(sr.ke.fd, sr.ke.buf, sr.ke.buf_len);
  auto const* mtd = (fanotify_event_metadata*)(sr.ke.buf);
  if (read_len <= 0 && errno != EAGAIN)
    return true;
  else if (read_len < 0)
    return do_error("e/sys/read@");
  else
    while (mtd && FAN_EVENT_OK(mtd, read_len))
      if (ev_c++ > sr.ke.c_ulim)
        return do_error("e/sys/ev_lim@");
      else if (mtd->vers != FANOTIFY_METADATA_VERSION)
        return do_error("e/sys/kernel_version@");
      else if (mtd->fd != FAN_NOFD)
        return do_warn("w/sys/bad_fd@");
      else if (mtd->mask & FAN_Q_OVERFLOW)
        return do_warn("w/sys/ev_lim@");
      else if (! ev_has_dirname(mtd))
        return do_warn("w/sys/bad_info@");
      else {
        auto [ev, n, l] = parse_ev(mtd, read_len);
        do_mark_if_newdir(ev, sr.ke.fd, cb);
        cb(ev);
        mtd = n;
        read_len -= l;
      }
  return true;
};

inline auto watch =
  [](char const* const path, auto const& cb, semabin const& is_living) -> bool
{
  auto sr = make_sysres(path, cb, is_living);
  auto do_error = [&](auto&& msg) -> bool
  { return (close_sysres(sr), adapter::do_error(msg, path, cb)); };
  auto is_ev_of = [&](int nth, int fd) -> bool
  { return sr.ep.interests[nth].data.fd == fd; };

  if (! sr.ok) return do_error("e/self/resource@");
  while (true) {
    int ep_c =
      epoll_wait(sr.ep.fd, sr.ep.interests, sr.ep.q_ulim, sr.ep.wake_ms);
    if (ep_c < 0)
      return do_error("e/sys/epoll_wait@");
    else if (ep_c == 0)
      continue;
    else
      for (int n = 0; n < ep_c; ++n)
        if (is_ev_of(n, sr.il.fd))
          return sr.il.state() == semabin::state::released
                 ? close_sysres(sr)
                 : do_error("e/self/semabin@");
        else if (is_ev_of(n, sr.ke.fd) && ! do_ev_recv(path, cb, sr))
          return do_error("e/self/ev_recv@");
  }
};

} /* namespace detail::wtr::watcher::adapter::fanotify */

#endif
#endif

#if (defined(__linux__) || __ANDROID_API__) \
  && ! defined(WATER_WATCHER_USE_WARTHOG)

#include <linux/version.h>

#if (KERNEL_VERSION(2, 7, 0) <= LINUX_VERSION_CODE) || __ANDROID_API__

#include <cstring>
#include <filesystem>
#include <functional>
#include <limits.h>
#include <sys/epoll.h>
#include <sys/inotify.h>
#include <system_error>
#include <unistd.h>
#include <unordered_map>

namespace detail::wtr::watcher::adapter::inotify {

// clang-format off
struct ke_in_ev {
  /*  The maximum length of an inotify
      event. Inotify will add at least
      one null byte to the end of the
      event to terminate the path name,
      but *also* to align the next event
      on an 8-byte boundary. We add one
      here to account for that byte, in
      this case for alignment.
  */
  static constexpr unsigned one_ulim = sizeof(inotify_event) + NAME_MAX + 1;
  static_assert(one_ulim % 8 == 0, "alignment");
  /*  Practically, this buffer is large
      enough for most read calls we would
      ever need to make. It's many times
      the size of the largest possible
      event, and those events are rare.
      Note that this isn't scaled with
      our `ep_q_sz` because this buffer
      length has nothing to do with an
      `epoll` event loop and everything
      to do with `read()` calls which
      fill a buffer with batched events.
  */
  static constexpr unsigned buf_len = 4096;
  static_assert(buf_len > one_ulim * 8, "capacity");
  /*  The upper limit of how many events
      we could possibly read into a buffer
      which is `ev_buf_len` bytes large.
      Practically, if we come half-way
      close to this value, we should
      be skeptical of the `read`.
  */
  static constexpr unsigned c_ulim = buf_len / sizeof(inotify_event);
  static_assert(c_ulim == 256);
  /*  These are the kinds of events which
      we're intersted in. Inotify *should*
      only send us these events, but that's
      not always the case. What's more, we
      sometimes receive events not in the
      IN_ALL_EVENTS mask. We'll ask inotify
      for these events and filter more if
      needed later on.
      These are all the supported events:
        IN_ACCESS
        IN_ATTRIB
        IN_CLOSE_WRITE
        IN_CLOSE_NOWRITE
        IN_CREATE
        IN_DELETE
        IN_DELETE_SELF
        IN_MODIFY
        IN_MOVE_SELF
        IN_MOVED_FROM
        IN_MOVED_TO
        IN_OPEN
  */
  static constexpr unsigned recv_mask
    = IN_CREATE
    | IN_DELETE
    | IN_DELETE_SELF
    | IN_MODIFY
    | IN_MOVE_SELF
    | IN_MOVED_FROM
    | IN_MOVED_TO;

  int fd = -1;
  using paths = std::unordered_map<int, std::filesystem::path>;
  paths dm{};
  alignas(inotify_event) char buf[buf_len]{0};
};

// clang-format on

struct sysres {
  bool ok = false;
  ke_in_ev ke{};
  semabin const& il{};
  adapter::ep ep{};
};

inline auto do_mark =
  [](char const* const dirpath, int dirfd, auto& dm, auto const& cb) -> bool
{
  char real[PATH_MAX];
  int wd = -1;
  if (realpath(dirpath, real) && is_dir(real))
    wd = inotify_add_watch(dirfd, real, ke_in_ev::recv_mask);
  if (wd > 0)
    return (dm.emplace(wd, real), true);
  else
    return do_error("w/sys/not_watched@", dirpath, cb);
};

inline auto make_sysres = [](
                            char const* const base_path,
                            auto const& cb,
                            semabin const& is_living) -> sysres
{
  auto do_error = [&](auto&& msg)
  { return (adapter::do_error(std::move(msg), base_path, cb), sysres{}); };
  /*  todo: Do we really need to be non-blocking? */
  int in_fd = inotify_init();
  if (in_fd < 0) return do_error("e/sys/inotify_init@");
  auto dm = ke_in_ev::paths{};
  walkdir_do(
    base_path,
    [&](char const* const dir) { do_mark(dir, in_fd, dm, cb); });
  auto ep = make_ep(base_path, cb, is_living.fd, in_fd);
  if (dm.empty() || ep.fd < 0)
    return (close(in_fd), do_error("e/self/resource@"));
  return sysres{
    .ok = true,
    .ke{
        .fd = in_fd,
        .dm = std::move(dm),
        },
    .il = is_living,
    .ep = ep,
  };
};

inline auto
peek(inotify_event const* const in_ev, inotify_event const* const ev_tail)
  -> inotify_event*
{
  auto len_to_next = sizeof(inotify_event) + (in_ev ? in_ev->len : 0);
  auto next = (inotify_event*)((char*)in_ev + len_to_next);
  return next < ev_tail ? next : nullptr;
};

struct parsed {
  ::wtr::watcher::event ev;
  inotify_event* next = nullptr;
};

inline auto parse_ev(
  std::filesystem::path const& dirname,
  inotify_event const* const in,
  inotify_event const* const tail) -> parsed
{
  using ev = ::wtr::watcher::event;
  using ev_pt = enum ev::path_type;
  using ev_et = enum ev::effect_type;
  auto pathof = [&](inotify_event const* const m)
  { return dirname / std::filesystem::path{m->name}; };
  auto pt = in->mask & IN_ISDIR ? ev_pt::dir : ev_pt::file;
  auto et = in->mask & IN_CREATE ? ev_et::create
          : in->mask & IN_DELETE ? ev_et::destroy
          : in->mask & IN_MOVE   ? ev_et::rename
          : in->mask & IN_MODIFY ? ev_et::modify
                                 : ev_et::other;
  auto isassoc = [&](auto* a, auto* b) -> bool
  { return b && b->cookie && b->cookie == a->cookie && et == ev_et::rename; };
  auto isfromto = [](auto* a, auto* b) -> bool
  { return (a->mask & IN_MOVED_FROM) && (b->mask & IN_MOVED_TO); };
  auto one = [&](auto* a, auto* next) -> parsed {
    return {ev(pathof(a), et, pt), next};
  };
  auto assoc = [&](auto* a, auto* b) -> parsed {
    return {ev(ev(pathof(a), et, pt), ev(pathof(b), et, pt)), peek(b, tail)};
  };
  auto next = peek(in, tail);

  return ! isassoc(in, next) ? one(in, next)
       : isfromto(in, next)  ? assoc(in, next)
       : isfromto(next, in)  ? assoc(next, in)
                             : one(in, next);
}

struct defer_dm_rm_wd {
  unsigned back_idx = 0;
  int buf[ke_in_ev::c_ulim];
  ke_in_ev::paths& dm;

  inline auto push(int wd) -> void
  {
    if (back_idx < sizeof(buf)) buf[back_idx++] = wd;
  };

  inline defer_dm_rm_wd(ke_in_ev::paths& dm)
      : dm{dm} {};

  inline ~defer_dm_rm_wd()
  {
    for (unsigned i = 0; i < back_idx; i++) {
      auto at = dm.find(buf[i]);
      if (at != dm.end()) dm.erase(at);
    }
  };
};

/*  Parses each event's path name,
    path type and effect.
    Looks for the directory path
    in a map of watch descriptors
    to directory paths.
    Updates the path map, adding
    new directories as they are
    created, and removing them
    as they are destroyed.

    Forward events and errors to
    the user. Returns on errors
    and when eventless.

    Event notes:
    Phantom Events --
    An event from an unmarked path,
    or a path which we didn't mark,
    was somehow reported to us.
    These events are rare, but they
    do happen. We won't be able to
    parse this event's real path.
    The ->name field seems to be
    null on these events, and we
    won't have a directory path to
    prepend it with. I'm not sure
    if we should try to parse the
    other fields, or if they would
    be interesting to the user.
    This may change.

    Impossible Events --
    These events are relatively
    rare, but they happen more
    than I think they should. We
    usually see these during
    high-throughput flurries of
    events. Maybe there is an
    error in our implementation?

    Deferred Events --
    Inotify closes the removed watch
    descriptors itself. We want to
    keep parity with inotify in our
    path map. That way, we can be
    in agreement about which watch
    descriptors map to which paths.
    We need to postpone removing
    this watch and, possibly, its
    watch descriptor from our path
    map until we're done with this
    event batch. Self-destroy events
    might come before we read other
    events that would map the watch
    descriptor to a path. Because
    we need to look the directory
    path up in the path map, we
    will defer its removal.

    Other notes:
    Sometimes we can fail to mark
    a new directory on its create
    event. This can happen if the
    directory is removed quickly
    after being created.
    In that case, we are unlikely
    to lose any path names on
    future events because events
    won't happen in that directory.
    If this happens for some other
    reason, we're in trouble.
*/
inline auto do_ev_recv =
  [](char const* const base_path, auto const& cb, sysres& sr) -> bool
{
  auto is_physical_ev = [](unsigned msk) -> bool
  {
    bool is_any = msk & ke_in_ev::recv_mask;
    bool is_self = msk & IN_DELETE_SELF || msk & IN_MOVE_SELF;
    bool is_ignored = msk & IN_IGNORED;
    return is_any && ! is_self && ! is_ignored;
  };

  memset(sr.ke.buf, 0, sizeof(sr.ke.buf));
  auto dmrm = defer_dm_rm_wd{sr.ke.dm};
  auto read_len = read(sr.ke.fd, sr.ke.buf, sizeof(sr.ke.buf));
  auto const* in_ev = (inotify_event*)(sr.ke.buf);
  auto const* const in_ev_tail = (inotify_event*)(sr.ke.buf + read_len);
  if (read_len < 0 && errno != EAGAIN)
    return do_error("e/sys/read@", base_path, cb);
  while (in_ev && in_ev < in_ev_tail) {
    auto in_ev_next = peek(in_ev, in_ev_tail);
    unsigned in_ev_c = 0;
    unsigned msk = in_ev->mask;
    auto dmhit = sr.ke.dm.find(in_ev->wd);
    if (in_ev_c++ > ke_in_ev::c_ulim)
      return do_error("e/sys/ev_lim@", base_path, cb);
    else if (msk & IN_Q_OVERFLOW)
      do_warn("w/sys/ev_lim@", base_path, cb);
    else if (dmhit == sr.ke.dm.end())
      do_warn("w/sys/phantom_event@", base_path, cb);
    else if (msk & IN_DELETE_SELF && ! (msk & IN_MOVE_SELF))
      dmrm.push(in_ev->wd);
    else if (is_physical_ev(msk)) {
      auto [ev, next] = parse_ev(dmhit->second, in_ev, in_ev_tail);
      if (msk & IN_ISDIR && msk & IN_CREATE)
        do_mark(ev.path_name.c_str(), sr.ke.fd, sr.ke.dm, cb);
      cb(ev);
      in_ev_next = next;
    }
    in_ev = in_ev_next;
  }
  return true;
};

inline auto watch =
  [](char const* const path, auto const& cb, semabin const& is_living) -> bool
{
  auto sr = make_sysres(path, cb, is_living);
  auto do_error = [&](auto&& msg) -> bool
  { return (close_sysres(sr), adapter::do_error(msg, path, cb)); };
  auto is_ev_of = [&](int nth, int fd) -> bool
  { return sr.ep.interests[nth].data.fd == fd; };

  if (! sr.ok) return do_error("e/self/resource@");
  while (true) {
    int ep_c =
      epoll_wait(sr.ep.fd, sr.ep.interests, sr.ep.q_ulim, sr.ep.wake_ms);
    if (ep_c < 0)
      return do_error("e/sys/epoll_wait@");
    else if (ep_c == 0)
      continue;
    else
      for (int n = 0; n < ep_c; ++n)
        if (is_ev_of(n, sr.il.fd))
          return sr.il.state() == semabin::state::released
                 ? close_sysres(sr)
                 : do_error("e/self/semabin@");
        else if (is_ev_of(n, sr.ke.fd) && ! do_ev_recv(path, cb, sr))
          return do_error("e/self/ev_recv@");
  }
};

} /* namespace detail::wtr::watcher::adapter::inotify */

#endif
#endif

#if (defined(__linux__) || __ANDROID_API__) \
  && ! defined(WATER_WATCHER_USE_WARTHOG)

#include <linux/version.h>
#include <unistd.h>

namespace detail::wtr::watcher::adapter {

inline auto watch =
  [](auto const& path, auto const& cb, auto const& is_living) -> bool
{
  auto p = path.c_str();
#if (KERNEL_VERSION(5, 9, 0) <= LINUX_VERSION_CODE) && ! __ANDROID_API__
  return geteuid() == 0 ? fanotify::watch(p, cb, is_living)
                        : inotify::watch(p, cb, is_living);
#elif (KERNEL_VERSION(2, 7, 0) <= LINUX_VERSION_CODE) || __ANDROID_API__
  return inotify::watch(p, cb, is_living);
#else
#error "Define 'WATER_WATCHER_USE_WARTHOG' on kernel versions < 2.7.0"
#endif
};

} /*  namespace detail::wtr::watcher::adapter */

#endif

#if defined(_WIN32)

#include <atomic>
#include <chrono>
#include <filesystem>
#include <string>
#include <thread>
#include <windows.h>

namespace detail {
namespace wtr {
namespace watcher {
namespace adapter {
namespace {

inline constexpr auto delay_ms = std::chrono::milliseconds(16);
inline constexpr auto delay_ms_dw = static_cast<DWORD>(delay_ms.count());
inline constexpr auto has_delay = delay_ms > std::chrono::milliseconds(0);
/*  I think the default page size in Windows is 64kb,
    so 65536 might also work well. */
inline constexpr auto event_buf_len_max = 8192;

/*  Hold resources necessary to recieve and send filesystem events. */
class watch_event_proxy {
public:
  bool is_valid{true};

  std::filesystem::path path{};

  wchar_t path_name[256]{L""};

  HANDLE path_handle{nullptr};

  HANDLE event_completion_token{nullptr};

  HANDLE event_token{CreateEventW(nullptr, true, false, nullptr)};

  OVERLAPPED event_overlap{};

  FILE_NOTIFY_INFORMATION event_buf[event_buf_len_max];

  DWORD event_buf_len_ready{0};

  watch_event_proxy(std::filesystem::path const& path) noexcept
      : path{path}
  {
    memcpy(path_name, path.c_str(), path.string().size());

    path_handle = CreateFileW(
      path.c_str(),
      FILE_LIST_DIRECTORY,
      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
      nullptr,
      OPEN_EXISTING,
      FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
      nullptr);

    if (path_handle)
      event_completion_token =
        CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);

    if (event_completion_token)
      is_valid = CreateIoCompletionPort(
                   path_handle,
                   event_completion_token,
                   (ULONG_PTR)path_handle,
                   1)
              && ResetEvent(event_token);
  }

  ~watch_event_proxy() noexcept
  {
    if (event_token) CloseHandle(event_token);
    if (event_completion_token) CloseHandle(event_completion_token);
  }
};

inline auto is_valid(watch_event_proxy& w) noexcept -> bool
{
  return w.is_valid;
}

inline auto has_event(watch_event_proxy& w) noexcept -> bool
{
  return w.event_buf_len_ready != 0;
}

inline auto do_event_recv(
  watch_event_proxy& w,
  ::wtr::watcher::event::callback const& callback) noexcept -> bool
{
  using namespace ::wtr::watcher;

  w.event_buf_len_ready = 0;
  DWORD bytes_returned = 0;
  memset(&w.event_overlap, 0, sizeof(OVERLAPPED));

  auto read_ok = ReadDirectoryChangesW(
    w.path_handle,
    w.event_buf,
    event_buf_len_max,
    true,
    FILE_NOTIFY_CHANGE_SECURITY | FILE_NOTIFY_CHANGE_CREATION
      | FILE_NOTIFY_CHANGE_LAST_ACCESS | FILE_NOTIFY_CHANGE_LAST_WRITE
      | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_ATTRIBUTES
      | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_FILE_NAME,
    &bytes_returned,
    &w.event_overlap,
    nullptr);

  if (read_ok) {
    w.event_buf_len_ready = bytes_returned > 0 ? bytes_returned : 0;
    return true;
  }
  else {
    switch (GetLastError()) {
      case ERROR_IO_PENDING :
        w.event_buf_len_ready = 0;
        w.is_valid = false;
        callback(
          {"e/sys/read/pending",
           ::wtr::event::effect_type::other,
           ::wtr::event::path_type::watcher});
        break;
      default :
        callback(
          {"e/sys/read",
           ::wtr::event::effect_type::other,
           ::wtr::event::path_type::watcher});
        break;
    }
    return false;
  }
}

inline auto do_event_send(
  watch_event_proxy& w,
  ::wtr::watcher::event::callback const& callback) noexcept -> bool
{
  using namespace ::wtr::watcher;

  FILE_NOTIFY_INFORMATION* buf = w.event_buf;

  if (is_valid(w)) {
    while (buf + sizeof(FILE_NOTIFY_INFORMATION)
           <= buf + w.event_buf_len_ready) {
      if (buf->FileNameLength % 2 == 0) {
        auto path_name =
          w.path / std::wstring{buf->FileName, buf->FileNameLength / 2};

        auto effect_type = [&buf]() noexcept
        {
          switch (buf->Action) {
            case FILE_ACTION_MODIFIED : return event::effect_type::modify;
            case FILE_ACTION_ADDED : return event::effect_type::create;
            case FILE_ACTION_REMOVED : return event::effect_type::destroy;
            case FILE_ACTION_RENAMED_OLD_NAME :
              return event::effect_type::rename;
            case FILE_ACTION_RENAMED_NEW_NAME :
              return event::effect_type::rename;
            default : return event::effect_type::other;
          }
        }();

        auto path_type = [&path_name]()
        {
          try {
            return std::filesystem::is_directory(path_name)
                   ? event::path_type::dir
                   : event::path_type::file;
          } catch (...) {
            return event::path_type::other;
          }
        }();

        callback({path_name, effect_type, path_type});

        if (buf->NextEntryOffset == 0)
          break;
        else
          buf =
            (FILE_NOTIFY_INFORMATION*)((uint8_t*)buf + buf->NextEntryOffset);
      }
    }
    return true;
  }

  else {
    return false;
  }
}

}  // namespace

/*  while living
    watch for events
    return when dead
    true if no errors */
inline auto watch(
  std::filesystem::path const& path,
  ::wtr::watcher::event::callback const& callback,
  semabin const& is_living) noexcept -> bool
{
  using namespace ::wtr::watcher;

  auto w = watch_event_proxy{path};

  if (is_valid(w)) {
    do_event_recv(w, callback);

    while (is_valid(w) && has_event(w)) { do_event_send(w, callback); }

    while (is_living.state() == semabin::state::pending) {
      ULONG_PTR completion_key{0};
      LPOVERLAPPED overlap{nullptr};

      bool complete = GetQueuedCompletionStatus(
        w.event_completion_token,
        &w.event_buf_len_ready,
        &completion_key,
        &overlap,
        delay_ms_dw);

      if (complete && overlap) {
        while (is_valid(w) && has_event(w)) {
          do_event_send(w, callback);
          do_event_recv(w, callback);
        }
      }
    }

    return true;
  }
  else {
    return false;
  }
}

} /*  namespace adapter */
} /*  namespace watcher */
} /*  namespace wtr */
} /*  namespace detail */

#endif

/*  A sturdy, universal adapter.

    This is the fallback adapter on platforms that either
      - Only support `kqueue` (`warthog` beats `kqueue`)
      - Only support the C++ standard library */

#if ! defined(__linux__) && ! defined(__ANDROID_API__) && ! defined(__APPLE__) \
  && ! defined(_WIN32)

#include <chrono>
#include <filesystem>
#include <functional>
#include <string>
#include <system_error>
#include <thread>
#include <unordered_map>

namespace detail {
namespace wtr {
namespace watcher {
namespace adapter {
namespace {

inline constexpr std::filesystem::directory_options scan_dir_options =
  std::filesystem::directory_options::skip_permission_denied
  & std::filesystem::directory_options::follow_directory_symlink;

using bucket_type =
  std::unordered_map<std::string, std::filesystem::file_time_type>;

/*  - Scans `path` for changes.
    - Updates our bucket to match the changes.
    - Calls `send_event` when changes happen.
    - Returns false if the file tree cannot be scanned. */
inline bool scan(
  std::filesystem::path const& path,
  auto const& send_event,
  bucket_type& bucket) noexcept
{
  /*  - Scans a (single) file for changes.
      - Updates our bucket to match the changes.
      - Calls `send_event` when changes happen.
      - Returns false if the file cannot be scanned. */
  auto const& scan_file =
    [&](std::filesystem::path const& file, auto const& send_event) -> bool
  {
    using namespace ::wtr::watcher;
    using std::filesystem::exists, std::filesystem::is_regular_file,
      std::filesystem::last_write_time;

    if (exists(file) && is_regular_file(file)) {
      auto ec = std::error_code{};
      /*  grabbing the file's last write time */
      auto const& timestamp = last_write_time(file, ec);
      if (ec) {
        /*  the file changed while we were looking at it.
            so, we call the closure, indicating destruction,
            and remove it from the bucket. */
        send_event(
          event{file, event::effect_type::destroy, event::path_type::file});
        if (bucket.contains(file)) bucket.erase(file);
      }
      /*  if it's not in our bucket, */
      else if (! bucket.contains(file)) {
        /*  we put it in there and call the closure,
            indicating creation. */
        bucket[file] = timestamp;
        send_event(
          event{file, event::effect_type::create, event::path_type::file});
      }
      /*  otherwise, it is already in our bucket. */
      else {
        /*  we update the file's last write time, */
        if (bucket[file] != timestamp) {
          bucket[file] = timestamp;
          /*  and call the closure on them,
              indicating modification */
          send_event(
            event{file, event::effect_type::modify, event::path_type::file});
        }
      }
      return true;
    } /*  if the path doesn't exist, we nudge the callee
          with `false` */
    else
      return false;
  };

  /*  - Scans a (single) directory for changes.
      - Updates our bucket to match the changes.
      - Calls `send_event` when changes happen.
      - Returns false if the directory cannot be scanned. */
  auto const& scan_directory =
    [&](std::filesystem::path const& dir, auto const& send_event) -> bool
  {
    using std::filesystem::recursive_directory_iterator,
      std::filesystem::is_directory;
    /*  if this thing is a directory */
    if (is_directory(dir)) {
      /*  try to iterate through its contents */
      auto dir_it_ec = std::error_code{};
      for (auto const& file :
           recursive_directory_iterator(dir, scan_dir_options, dir_it_ec))
        /*  while handling errors */
        if (dir_it_ec)
          return false;
        else
          scan_file(file.path(), send_event);
      return true;
    }
    else
      return false;
  };

  return scan_directory(path, send_event) ? true
       : scan_file(path, send_event)      ? true
                                          : false;
};

/*  If the bucket is empty, try to populate it.
    otherwise, prune it. */
inline bool tend_bucket(
  std::filesystem::path const& path,
  auto const& send_event,
  bucket_type& bucket) noexcept
{
  /*  Creates a file map, the "bucket", from `path`. */
  auto const& populate = [&](std::filesystem::path const& path) -> bool
  {
    using std::filesystem::exists, std::filesystem::is_directory,
      std::filesystem::recursive_directory_iterator,
      std::filesystem::last_write_time;
    /*  this happens when a path was changed while we were reading it.
        there is nothing to do here; we prune later. */
    auto dir_it_ec = std::error_code{};
    auto lwt_ec = std::error_code{};
    if (exists(path)) {
      /*  this is a directory */
      if (is_directory(path)) {
        for (auto const& file :
             recursive_directory_iterator(path, scan_dir_options, dir_it_ec)) {
          if (! dir_it_ec) {
            auto const& lwt = last_write_time(file, lwt_ec);
            if (! lwt_ec)
              bucket[file.path()] = lwt;
            else
              bucket[file.path()] = last_write_time(path);
          }
        }
      }
      /*  this is a file */
      else {
        bucket[path] = last_write_time(path);
      }
    }
    else {
      return false;
    }
    return true;
  };

  /*  Removes files which no longer exist from our bucket. */
  auto const& prune =
    [&](std::filesystem::path const& path, auto const& send_event) -> bool
  {
    using namespace ::wtr::watcher;
    using std::filesystem::exists, std::filesystem::is_regular_file,
      std::filesystem::is_directory, std::filesystem::is_symlink;

    auto bucket_it = bucket.begin();
    /*  while looking through the bucket's contents, */
    while (bucket_it != bucket.end()) {
      /*  check if the stuff in our bucket exists anymore. */
      exists(bucket_it->first)
        /*  if so, move on. */
        ? std::advance(bucket_it, 1)
        /*  if not, call the closure,
            indicating destruction,
            and remove it from our bucket. */
        : [&]()
      {
        send_event(event{
          bucket_it->first,
          event::effect_type::destroy,
          is_regular_file(path) ? event::path_type::file
          : is_directory(path)  ? event::path_type::dir
          : is_symlink(path)    ? event::path_type::sym_link
                                : event::path_type::other});
        /*  bucket, erase it! */
        bucket_it = bucket.erase(bucket_it);
      }();
    }
    return true;
  };

  return bucket.empty() ? populate(path)          ? true
                        : prune(path, send_event) ? true
                                                  : false
                        : true;
};

} /* namespace */

inline auto watch(
  std::filesystem::path const& path,
  ::wtr::watcher::event::callback const& callback,
  semabin const& is_living) noexcept -> bool
{
  using std::this_thread::sleep_for, std::chrono::milliseconds;

  /*  Sleep for `delay_ms`.

      Then, keep running if
        - We are alive
        - The bucket is doing well
        - No errors occured while scanning

      Otherwise, stop and return false. */

  bucket_type bucket;

  static constexpr auto delay_ms = 16;

  while (is_living.state() == semabin::state::pending) {
    if (
      ! tend_bucket(path, callback, bucket) || ! scan(path, callback, bucket)) {
      return false;
    }
    else {
      if constexpr (delay_ms > 0) sleep_for(milliseconds(delay_ms));
    }
  }

  return true;
}

} /*  namespace adapter */
} /*  namespace watcher */
} /*  namespace wtr */
} /*  namespace detail */

#endif

#include <filesystem>
#include <future>

namespace wtr {
inline namespace watcher {

/*  An asynchronous filesystem watcher.

    Begins watching when constructed.

    Stops watching when the object's lifetime ends
    or when `.close()` is called.

    Closing the watcher is the only blocking operation.

    @param path:
      The root path to watch for filesystem events.

    @param living_cb (optional):
      Something (such as a closure) to be called when events
      occur in the path being watched.

    This is an adaptor "switch" that chooses the ideal
   adaptor for the host platform.

    Every adapter monitors `path` for changes and invokes
   the `callback` with an `event` object when they occur.

    There are two things the user needs: `watch` and
   `event`.

    Typical use looks something like this:

    auto w = watch(".", [](event const& e) {
      std::cout
        << "path_name:   " << e.path_name   << "\n"
        << "path_type:   " << e.path_type   << "\n"
        << "effect_type: " << e.effect_type << "\n"
        << "effect_time: " << e.effect_time << "\n"
        << std::endl;
    };

    That's it.

    Happy hacking. */
class watch {
private:
  using sb = ::detail::wtr::watcher::semabin;
  sb is_living{};
  std::future<bool> watching{};

public:
  inline watch(
    std::filesystem::path const& path,
    event::callback const& callback) noexcept
      : watching{std::async(
        std::launch::async,
        [this, path, callback]
        {
          auto ec = std::error_code{};
          auto abs_path = std::filesystem::absolute(path, ec);
          auto pre_ok = ! ec && std::filesystem::is_directory(abs_path, ec)
                     && ! ec && this->is_living.state() == sb::state::pending;
          callback(
            {(pre_ok ? "s/self/live@" : "e/self/live@") + abs_path.string(),
             event::effect_type::create,
             event::path_type::watcher});
          auto post_ok = ! pre_ok ? true
                                  : ::detail::wtr::watcher::adapter::watch(
                                    abs_path,
                                    callback,
                                    this->is_living);
          callback(
            {(post_ok ? "s/self/die@" : "e/self/die@") + abs_path.string(),
             event::effect_type::destroy,
             event::path_type::watcher});
          return pre_ok && post_ok;
        })}
  {}

  inline auto close() noexcept -> bool
  {
    return this->is_living.release() == sb::state::released
        && this->watching.valid() && this->watching.get();
  };

  inline ~watch() noexcept { this->close(); }
};

} /*  namespace watcher */
} /*  namespace wtr   */
#endif /* W973564ED9F278A21F3E12037288412FBAF175F889 */
