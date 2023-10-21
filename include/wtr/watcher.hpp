#ifndef W973564ED9F278A21F3E12037288412FBAF175F889
#define W973564ED9F278A21F3E12037288412FBAF175F889

#include <array>
#include <charconv>
#include <chrono>
#include <filesystem>
#include <functional>
#include <ios>
#include <limits>
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
  /*  Ensure the user's callback can recieve events
      and will return nothing. */
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

  event const* const associated{nullptr};

  inline event(
    std::filesystem::path const& path_name,
    enum effect_type const& effect_type,
    enum path_type const& path_type) noexcept
      : path_name{path_name}
      , effect_type{effect_type}
      , path_type{path_type} {};

  inline event(event const& base, event&& associated) noexcept
      : path_name{base.path_name}
      , effect_type{base.effect_type}
      , path_type{base.path_type}
      , associated{&associated} {};

  inline ~event() noexcept = default;

  /*  An equality comparison for all the fields in this object.
      Includes the `effect_time`, which might not be wanted,
      because the `effect_time` is typically (not always) unique. */
  inline friend auto
  operator==(::wtr::event const& l, ::wtr::event const& r) noexcept -> bool
  {
    return l.path_name == r.path_name && l.effect_time == r.effect_time
        && l.path_type == r.path_type && l.effect_type == r.effect_type
        && (l.associated && r.associated ? *l.associated == *r.associated
                                         : ! l.associated && ! r.associated);
  }

  inline friend auto
  operator!=(::wtr::event const& l, ::wtr::event const& r) noexcept -> bool
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
              auto f = *from.associated;                                      \
              auto&& ttl = Cls{Lit",\"associated\""};                         \
              auto&& ety = Lit"\"" + to<Cls>(from.effect_type) + Lit"\"";     \
              auto&& pnm = Lit"\"" + to<Cls>(from.path_name)   + Lit"\"";     \
              auto&& pty = Lit"\"" + to<Cls>(from.path_type)   + Lit"\"";     \
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
  /*  `fs::path` has no hash function, so we use this. */
  using pathset = std::unordered_set<std::string>;
  ::wtr::watcher::event::callback const callback{};
  std::shared_ptr<pathset> seen_created_paths{new pathset{}};
  std::filesystem::path last_rename_from_path{};
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
    auto differs = ! lrf_path.empty() && lrf_path != path;
    auto missing = access(lrf_path.c_str(), F_OK) == -1;
    if (differs && missing) {
      callback({
        {lrf_path, effect_type::rename, pt},
        {    path, effect_type::rename, pt}
      });
      lrf_path.clear();
    }
    else {
      lrf_path = path;
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
      duration_cast<seconds>(16ms).count(),
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

inline auto block_while(std::atomic<bool>& b)
{
  using namespace std::chrono_literals;
  using std::this_thread::sleep_for;

  while (b) sleep_for(16ms);
}

} /*  namespace */

inline auto watch(
  std::filesystem::path const& path,
  ::wtr::watcher::event::callback const& callback,
  std::atomic<bool>& is_living) noexcept -> bool
{
  auto&& [ok, sysres] = open_event_stream(path, callback);
  return ok ? (block_while(is_living), close_event_stream(sysres)) : false;
}

} /*  namespace adapter */
} /*  namespace watcher */
} /*  namespace wtr   */
} /*  namespace detail */

#endif

#if (defined(__linux__) || defined(__ANDROID_API__)) \
  && ! defined(WATER_WATCHER_USE_WARTHOG)

#include <linux/version.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0)) \
  && ! defined(__ANDROID_API__)

#include <atomic>
#include <cassert>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <functional>
#include <optional>
#include <sys/epoll.h>
#include <sys/fanotify.h>
#include <system_error>
#include <tuple>
#include <unistd.h>
#include <unordered_map>
#include <unordered_set>

namespace detail {
namespace wtr {
namespace watcher {
namespace adapter {
namespace fanotify {

/*  - delay
      The delay, in milliseconds, while `epoll_wait` will
      'sleep' for until we are woken up. We usually check
      if we're still alive at that point.
    - event_wait_queue_max
      Number of events allowed to be given to recv
      (returned by `epoll_wait`). Any number between 1
      and some large number should be fine. We don't
      lose events if we 'miss' them, the events are
      still waiting in the next call to `epoll_wait`.
    - event_buf_len:
      For our event buffer, 4096 is a typical page size
      and sufficiently large to hold a great many events.
      That's a good thumb-rule, and it might be the best
      value to use because there will be a possibly long
      character string (for the filename) in the event.
      We can infer some things about the size we need for
      the event buffer, but it's unlikely to be meaningful
      because of the variably sized character string being
      reported. We could use something like:
          event_buf_len
            = ((event_wait_queue_max + PATH_MAX)
            * (3 * sizeof(fanotify_event_metadata)));
      But that's a lot of flourish for 72 bytes that won't
      be meaningful.
    - fan_init_flags:
      Post-event reporting, non-blocking IO and unlimited
      marks. We need sudo mode for the unlimited marks.
      If we were making a filesystem auditor, we might use:
          FAN_CLASS_PRE_CONTENT
          | FAN_UNLIMITED_QUEUE
          | FAN_UNLIMITED_MARKS
    - fan_init_opt_flags:
      Read-only, non-blocking, and close-on-exec. */
inline constexpr auto delay_ms = 16;
inline constexpr auto event_wait_queue_max = 1;
inline constexpr auto event_buf_len = PATH_MAX;
inline constexpr auto fan_init_flags = FAN_CLASS_NOTIF | FAN_REPORT_DFID_NAME
                                     | FAN_UNLIMITED_QUEUE
                                     | FAN_UNLIMITED_MARKS;
inline constexpr auto fan_init_opt_flags = O_RDONLY | O_NONBLOCK | O_CLOEXEC;

/*  - mark_set_type
        A set of file descriptors for fanotify resources.
    - system_resources
        An object holding:
          - An fanotify file descriptor
          - An epoll file descriptor
          - An epoll configuration
          - A set of watch marks (as returned by fanotify_mark)
          - A map of (sub)path handles to filesystem paths (names)
          - A boolean: whether or not the resources are valid */
using mark_set_type = std::unordered_set<int>;

struct system_resources {
  bool valid;
  int watch_fd;
  int event_fd;
  epoll_event event_conf;
  mark_set_type mark_set;
};

inline auto mark(
  std::filesystem::path const& full_path,
  int watch_fd,
  mark_set_type& ms) noexcept -> bool
{
  int wd = fanotify_mark(
    watch_fd,
    FAN_MARK_ADD,
    FAN_ONDIR | FAN_CREATE | FAN_MODIFY | FAN_DELETE | FAN_MOVE
      | FAN_DELETE_SELF | FAN_MOVE_SELF,
    AT_FDCWD,
    full_path.c_str());
  if (wd >= 0) {
    ms.insert(wd);
    return true;
  }
  else
    return false;
};

inline auto
mark(std::filesystem::path const& full_path, system_resources& sr) noexcept
  -> bool
{
  return mark(full_path, sr.watch_fd, sr.mark_set);
};

inline auto unmark(
  std::filesystem::path const& full_path,
  int watch_fd,
  mark_set_type& mark_set) noexcept -> bool
{
  int wd = fanotify_mark(
    watch_fd,
    FAN_MARK_REMOVE,
    FAN_ONDIR | FAN_CREATE | FAN_MODIFY | FAN_DELETE | FAN_MOVE
      | FAN_DELETE_SELF | FAN_MOVE_SELF,
    AT_FDCWD,
    full_path.c_str());
  auto const& at = mark_set.find(wd);

  if (wd >= 0 && at != mark_set.end()) {
    mark_set.erase(at);
    return true;
  }
  else
    return false;
};

inline auto
unmark(std::filesystem::path const& full_path, system_resources& sr) noexcept
  -> bool
{
  return unmark(full_path, sr.watch_fd, sr.mark_set);
};

/*  Produces a `system_resources` with the file descriptors from
    `fanotify_init` and `epoll_create`. Invokes `callback` on errors. */
inline auto open_system_resources(
  std::filesystem::path const& path,
  ::wtr::watcher::event::callback const& callback) noexcept -> system_resources
{
  namespace fs = std::filesystem;
  using ev = ::wtr::watcher::event;

  auto do_error = [&path, &callback](
                    char const* const msg,
                    int watch_fd,
                    int event_fd = -1) noexcept -> system_resources
  {
    callback(
      {std::string{msg} + "(" + std::strerror(errno) + ")@" + path.string(),
       ev::effect_type::other,
       ev::path_type::watcher});

    return system_resources{
      .valid = false,
      .watch_fd = watch_fd,
      .event_fd = event_fd,
      .event_conf = {.events = 0, .data = {.fd = watch_fd}},
      .mark_set = {},
    };
  };

  auto do_path_map_container_create =
    [](
      int const watch_fd,
      fs::path const& base_path,
      ::wtr::watcher::event::callback const& callback) -> mark_set_type
  {
    using diter = fs::recursive_directory_iterator;

    static constexpr auto dopt =
      fs::directory_options::skip_permission_denied
      & fs::directory_options::follow_directory_symlink;

    static constexpr auto rsrv_count = 1024;

    auto pmc = mark_set_type{};
    pmc.reserve(rsrv_count);

    try {
      if (mark(base_path, watch_fd, pmc))
        if (fs::is_directory(base_path))
          for (auto& dir : diter(base_path, dopt))
            if (fs::is_directory(dir))
              if (! mark(dir.path(), watch_fd, pmc))
                callback(
                  {"w/sys/not_watched@" + base_path.string() + "@"
                     + dir.path().string(),
                   ev::effect_type::other,
                   ev::path_type::watcher});
    } catch (...) {}

    return pmc;
  };

  int watch_fd = fanotify_init(fan_init_flags, fan_init_opt_flags);
  if (watch_fd < 1) return do_error("e/sys/fanotify_init", watch_fd);

  auto pmc = do_path_map_container_create(watch_fd, path, callback);
  if (pmc.empty()) return do_error("e/sys/fanotify_mark", watch_fd);

  epoll_event event_conf{.events = EPOLLIN, .data{.fd = watch_fd}};
  int event_fd = epoll_create1(EPOLL_CLOEXEC);
  if (event_fd < 1) return do_error("e/sys/epoll_create", watch_fd, event_fd);

  /*  @note We could make the epoll and fanotify file
      descriptors non-blocking with `fcntl`. It's not
      clear if we can do this from their `*_init` calls
      or if we need them to be non-blocking with our
      current (threaded) architecture.

      fcntl(watch_fd, F_SETFL, O_NONBLOCK);
      fcntl(event_fd, F_SETFL, O_NONBLOCK); */
  int ctl_ec = epoll_ctl(event_fd, EPOLL_CTL_ADD, watch_fd, &event_conf);
  if (ctl_ec != 0) return do_error("e/sys/epoll_ctl", watch_fd, event_fd);

  return system_resources{
    .valid = true,
    .watch_fd = watch_fd,
    .event_fd = event_fd,
    .event_conf = event_conf,
    .mark_set = std::move(pmc),
  };
};

inline auto close_system_resources(system_resources&& sr) noexcept -> bool
{
  return close(sr.watch_fd) == 0 && close(sr.event_fd) == 0;
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

/*  note at the end of file re. clang format */
inline auto parse_event(fanotify_event_metadata const* mtd) noexcept
  -> ::wtr::watcher::event
{
  using e = ::wtr::watcher::event;
  using pt = enum e::path_type;
  using et = enum e::effect_type;

  auto parse_full_path_into =
    [](char* path_buf, fanotify_event_metadata const* const mtd) noexcept
  {
    auto dir_info_fid = ((fanotify_event_info_fid const*)(mtd + 1));
    auto dir_fh = (struct file_handle*)(dir_info_fid->handle);

    ssize_t file_name_offset = 0;

    /*  Directory name */
    {
      int fd =
        open_by_handle_at(AT_FDCWD, dir_fh, O_RDONLY | O_CLOEXEC | O_PATH);
      if (fd > 0) {
        /*  If we have a pid with more than 128 digits... Well... */
        char fs_proc_path[128];
        snprintf(fs_proc_path, sizeof(fs_proc_path), "/proc/self/fd/%d", fd);
        file_name_offset =
          readlink(fs_proc_path, path_buf, PATH_MAX - sizeof(0));
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
        auto file_name_is_not_dir = strcmp(file_name, ".") != 0;
        if (file_name && file_name_is_not_dir) {
          snprintf(
            path_buf + file_name_offset,
            PATH_MAX - file_name_offset,
            "/%s",
            file_name);
        }
      }
    }
  };

  auto effect_type = mtd->mask & FAN_CREATE ? et::create
                   : mtd->mask & FAN_DELETE ? et::destroy
                   : mtd->mask & FAN_MODIFY ? et::modify
                   : mtd->mask & FAN_MOVE   ? et::rename
                                            : et::other;

  auto path_type = mtd->mask & FAN_ONDIR ? pt::dir : pt::file;

  char path_name[PATH_MAX];
  parse_full_path_into(path_name, mtd);

  return {path_name, effect_type, path_type};
};

/*  Send events to the user.
    This is the important part.
    Most of the other code is
    a layer of translation
    between us and the kernel. */
inline auto send(
  ::wtr::watcher::event const& ev,
  system_resources& sr,
  ::wtr::watcher::event::callback const& callback) noexcept -> bool
{
  using e = ::wtr::watcher::event;
  using pt = enum e::path_type;
  using et = enum e::effect_type;

  auto ok = ev.path_type == pt::dir
            ? ev.effect_type == et::create  ? mark(ev.path_name, sr)
            : ev.effect_type == et::destroy ? unmark(ev.path_name, sr)
                                            : true
            : true;

  return ok ? (callback(ev), true) : false;
};

/*  Reads through available (fanotify) filesystem events.
    Discerns their path and type.
    Calls the callback.
    Returns false on eventful errors.
    @note
    The `metadata->fd` field contains either a file
    descriptor or the value `FAN_NOFD`. File descriptors
    are always greater than 0. `FAN_NOFD` represents an
    event queue overflow for `fanotify` listeners which
    are _not_ monitoring file handles, such as mount
    monitors. The file handle is in the metadata when an
    `fanotify` listener is monitoring events by their
    file handles.
    The `metadata->vers` field may differ between kernel
    versions, so we check it against the version we were
    compiled with. */
inline auto recv(
  system_resources& sr,
  std::filesystem::path const& base_path,
  ::wtr::watcher::event::callback const& callback) noexcept -> bool
{
  /*  Read some events into a buffer.
      Send valid events to the user's callback.
      Send a diagnostic to the user on warnings and errors.
      Return false on errors. True otherwise. */

  using event_metadata = fanotify_event_metadata;
  using event_info_fid = fanotify_event_info_fid;

  static constexpr auto event_count_upper_lim =
    event_buf_len / sizeof(fanotify_event_metadata);

  auto do_error = [&base_path,
                   &callback](char const* const msg) noexcept -> bool
  {
    return (
      callback(
        {std::string{msg} + "@" + base_path.string(),
         ::wtr::watcher::event::effect_type::other,
         ::wtr::watcher::event::path_type::watcher}),
      false);
  };

  unsigned event_count = 0;
  alignas(fanotify_event_metadata) char event_buf[event_buf_len];
  auto read_len = read(sr.watch_fd, event_buf, sizeof(event_buf));
  enum class read_state {
    eventful,
    eventless,
    e_read
  } read_state = read_len > 0    ? read_state::eventful
               : read_len == 0   ? read_state::eventless
               : errno == EAGAIN ? read_state::eventless
                                 : read_state::e_read;

  switch (read_state) {
    case read_state::eventful :
      for (auto* mtd = (event_metadata*)(event_buf);
           FAN_EVENT_OK(mtd, read_len);
           mtd = FAN_EVENT_NEXT(mtd, read_len)) {
        auto hifty = ((event_info_fid const*)(mtd + 1))->hdr.info_type;
        enum class event_state {
          eventful,
          e_version,
          e_count,
          w_fd,
          w_q_overflow,
          w_info_type
        } event_state =
          mtd->vers != FANOTIFY_METADATA_VERSION   ? event_state::e_version
          : event_count++ > event_count_upper_lim  ? event_state::e_count
          : mtd->fd != FAN_NOFD                    ? event_state::w_fd
          : mtd->mask & FAN_Q_OVERFLOW             ? event_state::w_q_overflow
          : hifty != FAN_EVENT_INFO_TYPE_DFID_NAME ? event_state::w_info_type
                                                   : event_state::eventful;
        switch (event_state) {
          case event_state::eventful :
            send(parse_event(mtd), sr, callback);
            break;
          case event_state::e_version : return do_error("e/sys/kernel_version");
          case event_state::e_count : return do_error("e/sys/bad_count");
          case event_state::w_fd : return ! do_error("w/sys/bad_fd");
          case event_state::w_q_overflow : return ! do_error("w/sys/overflow");
          case event_state::w_info_type : return ! do_error("w/sys/bad_info");
        }
      }
      return true;

    case read_state::eventless : return true;

    case read_state::e_read : return do_error("e/sys/read");
  }

  assert(! "Unreachable");
  return false;
};

inline auto watch(
  std::filesystem::path const& path,
  ::wtr::watcher::event::callback const& callback,
  std::atomic<bool>& is_living) noexcept -> bool
{
  using ev = ::wtr::watcher::event;

  auto do_error = [&path, &callback](auto& f, char const* const msg) -> bool
  {
    return (
      callback(
        {std::string{msg} + "@" + path.string(),
         ev::effect_type::other,
         ev::path_type::watcher}),

      f(),

      false);
  };

  /*  While:
      - System resources for fanotify and epoll
      - An event list for receiving epoll events
      - We're alive
      Do:
      - Await filesystem events
      - Invoke `callback` on errors and events */

  auto sr = open_system_resources(path, callback);

  auto close = [&sr]() -> bool
  { return close_system_resources(std::move(sr)); };

  epoll_event event_recv_list[event_wait_queue_max];

  if (sr.valid) [[likely]] {
    while (is_living) [[likely]]

    {
      int event_count = epoll_wait(
        sr.event_fd,
        event_recv_list,
        event_wait_queue_max,
        delay_ms);
      if (event_count < 0)
        return do_error(close, "e/sys/epoll_wait");

      else if (event_count > 0) [[likely]]
        for (int n = 0; n < event_count; n++)
          if (event_recv_list[n].data.fd == sr.watch_fd) [[likely]]
            if (is_living) [[likely]]
              if (! recv(sr, path, callback)) [[unlikely]]
                return do_error(close, "e/self/event_recv");
    }

    return close();
  }

  else
    return do_error(close, "e/self/sys_resource");
};

} /*  namespace fanotify */
} /*  namespace adapter */
} /*  namespace watcher */
} /*  namespace wtr */
} /*  namespace detail */

#endif
#endif

#if (defined(__linux__) || defined(__ANDROID_API__)) \
  && ! defined(WATER_WATCHER_USE_WARTHOG)

#include <linux/version.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 7, 0)) || defined(__ANDROID_API__)

#include <atomic>
#include <cassert>
#include <cstring>
#include <filesystem>
#include <functional>
#include <string>
#include <sys/epoll.h>
#include <sys/inotify.h>
#include <system_error>
#include <tuple>
#include <unistd.h>
#include <unordered_map>
#include <vector>

namespace detail::wtr::watcher::adapter::inotify {

/*  - pathmap_type
        An alias for a map of file descriptors to paths.
    - sysres_type
        An object representing an inotify file descriptor,
        an epoll file descriptor, an epoll configuration,
        and whether or not the resources are ok */
using pathmap_type = std::unordered_map<int, std::filesystem::path>;

struct sysres_type {
  /*  Number of events allowed to be
      given to do_event_recv. The same
      value is usually returned by a
      call to `epoll_wait`). Any number
      between 1 and INT_MAX should be
      fine... But 1 is fine for us.
      We don't lose events if we 'miss'
      them, the events are still waiting
      in the next call to `epoll_wait`.
  */
  static constexpr auto ep_q_sz = 1;
  /*  The delay, in milliseconds, while
      `epoll_wait` will 'pause' us for
      until we are woken up. We use the
      time after this to check if we're
      still alive, then re-enter epoll.
  */
  static constexpr auto ep_delay_ms = 16;
  /*  Practically, this buffer is large
      enough for most read calls we would
      ever need to make. It's eight times
      the size of the largest possible
      event, and those events are rare.
      Note that this isn't scaled with
      our `eq_q_sz` because events are
      batched together elsewhere as well.
  */
  static constexpr auto ev_buf_len = (sizeof(inotify_event) + NAME_MAX + 1) * 8;
  /*  The upper limit of how many events
      we could possibly read into a buffer
      which is `ev_buf_len` bytes large.
      Practically, if we come half-way
      close to this value, we should
      be skeptical of the `read`.
  */
  static constexpr auto ev_c_ulim = ev_buf_len / sizeof(inotify_event);
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
  static constexpr unsigned in_ev_mask = IN_CREATE | IN_DELETE | IN_DELETE_SELF
                                       | IN_MODIFY | IN_MOVE_SELF
                                       | IN_MOVED_FROM | IN_MOVED_TO;

  bool ok = false;
  int in_fd = -1;
  int ep_fd = -1;
  epoll_event ep_interests[ep_q_sz]{};
  pathmap_type dm{};
  alignas(inotify_event) char ev_buf[ev_buf_len]{0};
};

inline auto do_error =
  [](auto&& msg, auto const& path, auto const& callback) noexcept -> bool
{
  using et = enum ::wtr::watcher::event::effect_type;
  using pt = enum ::wtr::watcher::event::path_type;
  callback({msg + path.string(), et::other, pt::watcher});
  return false;
};

inline auto do_warn =
  [](auto&& msg, auto const& path, auto const& callback) noexcept -> bool
{ return (do_error(std::move(msg), path, callback), true); };

inline auto do_mark =
  [](auto& dm, int dirfd, auto const& dirname) noexcept -> bool
{
  if (! std::filesystem::is_directory(dirname)) return false;
  int wd = inotify_add_watch(dirfd, dirname.c_str(), sysres_type::in_ev_mask);
  return wd > 0 ? dm.emplace(wd, dirname).first != dm.end() : false;
};

/*  If the path given is a directory
      - find all directories above the base path given.
      - ignore nonexistent directories.
      - return a map of watch descriptors -> directories.
    If `path` is a file
      - return it as the only value in a map.
      - the watch descriptor key should always be 1. */
inline auto make_pathmap = [](
                             auto const& base_path,
                             auto const& callback,
                             int inotify_watch_fd) noexcept -> pathmap_type
{
  namespace fs = std::filesystem;
  using dopt = fs::directory_options;
  using diter = fs::recursive_directory_iterator;
  using fs::is_directory;
  constexpr auto fs_dir_opt =
    dopt::skip_permission_denied & dopt::follow_directory_symlink;
  auto dm = pathmap_type{};
  dm.reserve(256);
  auto do_mark = [&](auto d) noexcept
  { return inotify::do_mark(dm, inotify_watch_fd, d); };
  try {
    if (is_directory(base_path) && do_mark(base_path))
      for (auto dir : diter(base_path, fs_dir_opt))
        if (is_directory(dir) && ! do_mark(dir.path()))
          do_warn("w/sys/not_watched@", base_path, callback);
  } catch (...) {}
  return dm;
};

inline auto make_sysres =
  [](auto const& base_path, auto const& callback) noexcept -> sysres_type
{
  auto do_error = [&](auto&& msg, int in_fd, int ep_fd = -1)
  {
    return (
      inotify::do_error(std::move(msg), base_path, callback),
      sysres_type{
        .ok = false,
        .in_fd = in_fd,
        .ep_fd = ep_fd,
      });
  };
#if defined(__ANDROID_API__)
  int in_fd = inotify_init();
#else
  /*  todo: Do we need to be non-blocking? */
  int in_fd = inotify_init1(IN_NONBLOCK);
#endif
  if (in_fd < 0) return do_error("e/sys/inotify_init@", in_fd);
#if defined(__ANDROID_API__)
  int ep_fd = epoll_create(1);
#else
  int ep_fd = epoll_create1(EPOLL_CLOEXEC);
#endif
  if (ep_fd < 0) return do_error("e/sys/epoll_create@", in_fd, ep_fd);
  auto dm = make_pathmap(base_path, callback, in_fd);
  if (dm.empty()) return do_error("e/self/pathmap@", in_fd, ep_fd);
  auto want_ev = epoll_event{.events = EPOLLIN, .data{.fd = in_fd}};
  int ctlec = epoll_ctl(ep_fd, EPOLL_CTL_ADD, in_fd, &want_ev);
  if (ctlec < 0) return do_error("e/sys/epoll_ctl@", in_fd, ep_fd);
  return sysres_type{
    .ok = true,
    .in_fd = in_fd,
    .ep_fd = ep_fd,
    .dm = std::move(dm),
  };
};

inline auto close_sysres = [](auto& sr) noexcept -> bool
{
  sr.ok |= close(sr.in_fd) == 0;
  sr.ok |= close(sr.ep_fd) == 0;
  return sr.ok;
};

/*  Event notes:
    Phantom Events --
    An event, probably from some
    other recent instance of
    inotify, somehow got to us.
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
    We need to postpone removing
    this watch and, possibly, its
    watch descriptor from our path
    map until we're dont with this
    even loop. Self-destroy events
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
inline auto do_event_recv_one = [](
                                  auto const& base_path,
                                  auto const& callback,
                                  sysres_type& sr,
                                  size_t read_len) noexcept -> bool
{
  auto do_warn = [&](auto&& msg) noexcept -> bool
  { return inotify::do_warn(std::move(msg), base_path, callback); };
  auto do_error = [&](auto&& msg) noexcept -> bool
  { return inotify::do_error(std::move(msg), base_path, callback); };
  std::vector<int> defer_close{};
  auto do_deferred = [&]() noexcept
  {
    auto ok = true;
    for (auto wd : defer_close) {
      /*  No need to check rm_watch for errors
          because there is a very good chance
          that inotify closed the fd by itself.
          It's just here in case it didn't. */
      inotify_rm_watch(sr.in_fd, wd);
      if (sr.dm.find(wd) == sr.dm.end())
        do_error("w/self/impossible_event@");
      else
        ok |= sr.dm.erase(wd);
    }
    return ok;
  };
  auto const* event = (inotify_event*)(sr.ev_buf);
  auto const* const tail = (inotify_event*)(sr.ev_buf + read_len);
  while (event < tail) {
    unsigned ev_c = 0;
    unsigned msk = event->mask;
    auto d = sr.dm.find(event->wd);
    enum {
      e_lim,
      w_lim,
      phantom,
      impossible,
      ignore,
      self_del,
      self_delmov,
      eventful,
    } event_state = ev_c++ > sysres_type::ev_c_ulim                ? e_lim
                  : msk & IN_Q_OVERFLOW                            ? w_lim
                  : d == sr.dm.end()                               ? phantom
                  : ! (msk & sysres_type::in_ev_mask)              ? impossible
                  : msk & IN_IGNORED                               ? ignore
                  : msk & IN_DELETE_SELF && ! (msk & IN_MOVE_SELF) ? self_del
                  : msk & IN_DELETE_SELF || msk & IN_MOVE_SELF     ? self_delmov
                                                                   : eventful;
    switch (event_state) {
      case e_lim : return (do_deferred(), do_error("e/sys/event_lim@"));
      case w_lim : do_warn("w/sys/event_lim@"); break;
      case phantom : do_warn("w/sys/phantom_event@"); break;
      case impossible : break;
      case ignore : break;
      case self_del : defer_close.push_back(event->wd); break;
      case self_delmov : break;
      case eventful : {
        namespace fs = std::filesystem;
        using pt = enum ::wtr::watcher::event::path_type;
        using et = enum ::wtr::watcher::event::effect_type;
        auto path_name = d->second / fs::path{event->name};
        auto path_type = msk & IN_ISDIR ? pt::dir : pt::file;
        auto effect_type = msk & IN_CREATE     ? et::create
                         : msk & IN_DELETE     ? et::destroy
                         : msk & IN_MOVED_FROM ? et::rename
                         : msk & IN_MOVED_TO   ? et::rename
                         : msk & IN_MODIFY     ? et::modify
                                               : et::other;
        if (msk & IN_ISDIR && msk & IN_CREATE) {
          if (! do_mark(sr.dm, sr.in_fd, path_name))
            do_warn("w/sys/add_watch@");
        }
        callback({path_name, effect_type, path_type});  // <- Magic happens
      } break;
    }
    auto full_len = sizeof(inotify_event) + event->len;
    event = (inotify_event*)((char*)event + full_len);
  }
  return do_deferred();
};

/*  Reads through available (inotify) filesystem events.
    There might be several events from a single read.
    Three possible states:
     - eventful: there are events to read
     - eventless: there are no events to read
     - error: there was an error reading events
    The EAGAIN "error" means there is nothing
    to read. We count that as 'eventless'.
    Discerns each event's full path and type.
    Looks for the full path in `dm`, our map of
    watch descriptors to directory paths.
    Updates the path map, adding the directories
    with `create` events and removing the ones
    with `destroy` events.
    Forward events and errors to the user.
    Return when eventless.
    @todo
    Consider running and returning `find_dirs` from here.
    Remove destroyed watches. */
inline auto do_event_recv = [](
                              auto const& base_path,
                              auto const& callback,
                              sysres_type& sr) noexcept -> bool
{
  memset(sr.ev_buf, 0, sizeof(sr.ev_buf));
  auto read_len = read(sr.in_fd, sr.ev_buf, sizeof(sr.ev_buf));
  enum {
    eventful,
    eventless,
    error
  } read_state = read_len > 0 && *sr.ev_buf ? eventful
               : read_len == 0              ? eventless
               : errno == EAGAIN            ? eventless
                                            : error;
  switch (read_state) {
    case eventless : return true;
    case error : return do_error("e/sys/read@", base_path, callback);
    case eventful : {
      return do_event_recv_one(base_path, callback, sr, read_len);
    }
  }

  assert(! "Unreachable");
  return false;
};

inline auto watch(
  std::filesystem::path const& path,
  ::wtr::watcher::event::callback const& callback,
  std::atomic<bool>& is_living) noexcept -> bool
{
  /*  While:
      - A lifetime the user hasn't ended
      - A historical map of watch descriptors
        to long paths (for event reporting)
      - System resources for inotify and epoll
      - An event buffer for events from epoll
      - We're alive
      Do:
      - Await filesystem events
      - Invoke `callback` on errors and events */

  auto sr = make_sysres(path, callback);

  auto do_error = [&](auto&& msg)
  {
    return (
      close_sysres(sr),
      inotify::do_error(std::move(msg), path, callback));
  };

  if (sr.ok) {
    while (is_living) {
      int ev_count = epoll_wait(
        sr.ep_fd,
        sr.ep_interests,
        sysres_type::ep_q_sz,
        sysres_type::ep_delay_ms);
      if (ev_count < 0)
        return do_error("e/sys/epoll_wait@");
      else if (ev_count > 0)
        for (int n = 0; n < ev_count; n++)
          if (sr.ep_interests[n].data.fd == sr.in_fd)
            if (! do_event_recv(path, callback, sr))
              return do_error("e/self/event_recv@");
    }
  }
  return close_sysres(sr);
}

} /* namespace detail::wtr::watcher::adapter::inotify */

#endif
#endif

#if (defined(__linux__) || defined(__ANDROID_API__)) \
  && ! defined(WATER_WATCHER_USE_WARTHOG)

#include <atomic>
#include <functional>
#include <linux/version.h>
#include <unistd.h>

namespace detail {
namespace wtr {
namespace watcher {
namespace adapter {

/*  If we have a kernel that can use either `fanotify` or
    `inotify`, then we will use `fanotify` if the user is
    (effectively) root.
    We can use `fanotify` (and `inotify`, too) on a kernel
    version 5.9.0 or greater.
    If we can only use `inotify`, then we'll just use that.
    We only use `inotify` on Android and on kernel versions
    less than 5.9.0 and greater than/equal to 2.7.0.
    Below 2.7.0, you can use the `warthog` adapter by
    defining `WATER_WATCHER_USE_WARTHOG` at some point during
    the build or before including 'wtr/watcher.hpp'. */

inline auto watch(
  std::filesystem::path const& path,
  ::wtr::watcher::event::callback const& callback,
  std::atomic<bool>& is_living) noexcept -> bool
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0)) \
  && ! defined(__ANDROID_API__)
  return geteuid() == 0 ? fanotify::watch(path, callback, is_living)
                        : inotify::watch(path, callback, is_living);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 7, 0)) \
  || defined(__ANDROID_API__)
  return inotify::watch(path, callback, is_living);
#else
#error "Define 'WATER_WATCHER_USE_WARTHOG' on kernel versions < 2.7.0"
#endif
}

} /*  namespace adapter */
} /*  namespace watcher */
} /*  namespace wtr */
} /*  namespace detail */

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
}  // namespace

}  // namespace

/*  while living
    watch for events
    return when dead
    true if no errors */
inline auto watch(
  std::filesystem::path const& path,
  ::wtr::watcher::event::callback const& callback,
  std::atomic<bool>& is_living) noexcept -> bool
{
  using namespace ::wtr::watcher;

  auto w = watch_event_proxy{path};

  if (is_valid(w)) {
    do_event_recv(w, callback);

    while (is_valid(w) && has_event(w)) { do_event_send(w, callback); }

    while (is_living) {
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
  std::atomic<bool>& is_living) noexcept -> bool
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

  while (is_living) {
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

#include <atomic>
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
  std::atomic<bool> is_living{true};
  std::future<bool> watching{};

public:
  inline watch(
    std::filesystem::path const& path,
    event::callback const& callback) noexcept
      : watching{std::async(
        std::launch::async,
        [this, path, callback]
        {
          auto abs_path_ec = std::error_code{};
          auto abs_path = std::filesystem::absolute(path, abs_path_ec);
          callback(
            {(abs_path_ec ? "e/self/live@" : "s/self/live@")
               + abs_path.string(),
             ::wtr::watcher::event::effect_type::create,
             ::wtr::watcher::event::path_type::watcher});
          auto watched_and_died_ok = abs_path_ec
                                     ? false
                                     : ::detail::wtr::watcher::adapter::watch(
                                       abs_path,
                                       callback,
                                       this->is_living);
          callback(
            {(watched_and_died_ok ? "s/self/die@" : "e/self/die@")
               + abs_path.string(),
             ::wtr::watcher::event::effect_type::destroy,
             ::wtr::watcher::event::path_type::watcher});
          return watched_and_died_ok;
        })}
  {}

  inline auto close() noexcept -> bool
  {
    this->is_living = false;
    return this->watching.valid() && this->watching.get();
  };

  inline ~watch() noexcept { this->close(); }
};

} /*  namespace watcher */
} /*  namespace wtr   */
#endif /* W973564ED9F278A21F3E12037288412FBAF175F889 */
