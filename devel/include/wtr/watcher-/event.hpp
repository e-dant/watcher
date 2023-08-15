#pragma once

/* std::basic_ostream */
#include <ios>
/* std::chrono::system_clock::now
   std::chrono::duration_cast
   std::chrono::system_clock
   std::chrono::nanoseconds
   std::chrono::time_point */
#include <chrono>
/* std::filesystem::path */
#include <filesystem>
/* std::function */
#include <functional>

namespace wtr {
inline namespace watcher {

/*  @brief watcher/event
    There are only three things the user needs:
      - The `die` function
      - The `watch` function
      - The `event` object

    The `event` object is used to pass information about
    filesystem events to the (user-supplied) callback
    given to `watch`.

    The `event` object will contain the:
      - Path, which is always absolute.
      - Type, one of:
        - dir
        - file
        - hard_link
        - sym_link
        - watcher
        - other
      - Event type, one of:
        - rename
        - modify
        - create
        - destroy
        - owner
        - other
      - Event time in nanoseconds since epoch

    The `watcher` type is special.
    Events with this type will include messages from
    the watcher. You may recieve error messages or
    important status updates.  */

struct event {

private:
  /* I like these names. Very human.
     'what happen'
     'event kind' */
  using path_type = std::filesystem::path;
  using ns = std::chrono::nanoseconds;
  using clock = std::chrono::system_clock;
  using time_point = std::chrono::time_point<clock>;

public:
  /*  @brief watcher/event/callback
      Ensure the adapters can recieve events
      and will return nothing. */
  using callback = std::function<void(event const&)>;

  /*  @brief wtr/watcher/event/what
      A structure intended to represent
      what has happened to some path
      at the moment of some affecting event. */
  enum class what {
    /* The essentials */
    rename,
    modify,
    create,
    destroy,
    /* The attributes */
    owner,
    /* Catch-all */
    other,
  };

  /*  @brief wtr/watcher/event/kind
      The essential kinds of paths. */
  enum class kind {
    /* The essentials */
    dir,
    file,
    hard_link,
    sym_link,
    /* The specials */
    watcher,
    /* Catch-all */
    other,
  };

  path_type const where{};

  enum what const what {};

  enum kind const kind {};

  long long const when{
    std::chrono::duration_cast<ns>(time_point{clock::now()}.time_since_epoch())
      .count()};

  event(std::filesystem::path const& where,
        enum what const& what,
        enum kind const& kind) noexcept
      : where{where},
        what{what},
        kind{kind} {};

  ~event() noexcept = default;
};

/*  @brief wtr/watcher/event/<<
    Streams out a `what` value. */
template<class Char, class CharTraits>
inline constexpr auto operator<<(std::basic_ostream<Char, CharTraits>& os,
                                 enum event::what const& w) noexcept
  -> std::basic_ostream<Char, CharTraits>&
{
  /* clang-format off */
  switch (w) {
    case event::what::rename  : return os << "\"rename\"";
    case event::what::modify  : return os << "\"modify\"";
    case event::what::create  : return os << "\"create\"";
    case event::what::destroy : return os << "\"destroy\"";
    case event::what::owner   : return os << "\"owner\"";
    case event::what::other   : return os << "\"other\"";
    default                   : return os << "\"other\"";
  }
  /* clang-format on */
};

/*  @brief wtr/watcher/event/<<
    Streams out a `kind` value. */
template<class Char, class CharTraits>
inline constexpr auto operator<<(std::basic_ostream<Char, CharTraits>& os,
                                 enum event::kind const& k) noexcept
  -> std::basic_ostream<Char, CharTraits>&
{
  /* clang-format off */
  switch (k) {
    case event::kind::dir       : return os << "\"dir\"";
    case event::kind::file      : return os << "\"file\"";
    case event::kind::hard_link : return os << "\"hard_link\"";
    case event::kind::sym_link  : return os << "\"sym_link\"";
    case event::kind::watcher   : return os << "\"watcher\"";
    case event::kind::other     : return os << "\"other\"";
    default                     : return os << "\"other\"";
  }
  /* clang-format on */
};

/*  @brief wtr/watcher/event/<<
    Streams out `where`, `what` and `kind`.
    Formats the stream as a json object.
    Looks like this (without line breaks)
      "1678046920675963000":{
       "where":"/some_file.txt",
       "what":"create",
       "kind":"file"
      } */
template<class Char, class CharTraits>
inline constexpr auto operator<<(std::basic_ostream<Char, CharTraits>& os,
                                 event const& ev) noexcept
  -> std::basic_ostream<Char, CharTraits>&
{
  /* clang-format off */
    return os << '"' << ev.when
              << "\":{\"where\":" << ev.where
              <<    ",\"what\":"  << ev.what
              <<    ",\"kind\":"  << ev.kind  << '}';
  /* clang-format on */
};

/*  @brief wtr/watcher/event/==
    A "strict" comparison of an event's `when`,
    `where`, `what` and `kind` values.
    Keep in mind that this compares `when`,
    which might not be desireable. */
inline auto operator==(event const& l, event const& r) noexcept -> bool
{
  /* clang-format off */
  return l.where == r.where
      && l.what  == r.what
      && l.kind  == r.kind
      && l.when  == r.when;
  /* clang-format on */
};

/*  @brief wtr/watcher/event/!=
    Not == */
inline auto operator!=(event const& l, event const& r) noexcept -> bool
{
  return ! (l == r);
};

} /* namespace watcher */
} /* namespace wtr   */
