/*  std::chrono::* */
#include <chrono>
/*  std::strcmp */
#include <cstring>
/*  std::filesystem::
      current_path
      absolute
      exists
      path */
#include <filesystem>
/*  std::cout
    std::endl */
#include <iostream>
/*  std::optional */
#include <optional>
/*  std::stoull */
#include <string>
/*  std::make_tuple */
#include <tuple>
/*  std::this_thread::sleep_for */
#include <thread>
/*  wtr::watcher::
      watch
      event::
        event
        what
        kind */
#include <wtr/watcher.hpp>

using namespace wtr::watcher;
using namespace std::chrono;
using std::function;
namespace fs = std::filesystem;

/*  @todo
    [ -what < all rename modify create destroy owner other > = all ]
    [ -kind < all dir file hard_link sym_link watcher other > = all ] */
inline constexpr auto usage =
  "wtr.watcher [ -h | --help ] [ PATH = . [ -UNIT < TIME > ]\n"
  "\n"
  "  PATH\n"
  "    Any path. Relative or absolute.\n"
  "    Defaults to the current directory.\n"
  "    If the given path doesn't exist,\n"
  "    the current directory is used.\n"
  "\n"
  "  UNIT\n"
  "    One of:\n"
  "    -nanoseconds,  -ns,\n"
  "    -microseconds, -us,\n"
  "    -milliseconds, -ms,\n"
  "    -seconds,      -s,\n"
  "    -minutes,      -m,\n"
  "    -hours,        -h,\n"
  "    -days,         -d,\n"
  "    -weeks,        -w,\n"
  "    -months,       -mts,\n"
  "    -years,        -y\n"
  "\n"
  "  TIME\n"
  "    Any positive integer, as long as it's\n"
  "    less than ULONG_MAX. Which is large.\n";

auto watch_forever_or_expire(
  std::optional<std::chrono::nanoseconds const> const& alive_for)
{

  /*  Watch for some time, `alive_for`. */
  auto expire = std::function<bool(std::filesystem::path const&,
                                   wtr::event::callback const&)>{
    [alive_for](std::filesystem::path const& path,
                wtr::event::callback const& callback)
    {
      auto const then = system_clock::now();
      std::cout << R"({"wtr":{"watcher":{"stream":{)" << std::endl;

      auto const die = wtr::watch(path, callback);

      std::this_thread::sleep_for(alive_for.value());

      auto const dead = die();

      std::cout
        << "}"
        << "\n,\"milliseconds\":"
        << duration_cast<milliseconds>(system_clock::now() - then).count()
        << "\n,\"dead\":" << (dead ? "true" : "false") << "\n}}}" << std::endl;

      return dead;
    }};

  /*  Watch forever. */
  auto forever = std::function<bool(std::filesystem::path const&,
                                    wtr::event::callback const&)>{
    [](std::filesystem::path const& path, wtr::event::callback const& callback)
    {
      auto _{watch(path, callback)};
      return true;
    }};

  if (alive_for.has_value())
    return expire;

  else
    return forever;
};

auto from_cmdline(int const argc, char const** const argv)
{
  auto argis = [&](auto const i, char const* a)
  { return argc > i ? std::strcmp(a, argv[i]) == 0 : false; };

  auto given_or_current_path = [](int const argc, char const** const argv)
  {
    return fs::absolute(
      [&]
      {
        auto p = fs::path(argc > 1 ? argv[1] : ".");
        return fs::exists(p) ? p : fs::current_path();
      }());
  };

  auto maybe_filter_callback = []
  {
    /*  Show what happens.
        Use the event's stream operator, formatting as json.
        Append a comma until the last event: Watcher dying.
        Flush stdout, via `endl`, for piping.
        For manual parsing, see the file watcher/event.hpp. */
    wtr::event::callback show_json = [](wtr::event::event const& ev)
    {
      using wtr::watcher::event::kind, wtr::watcher::event::what;

      auto comma_or_nothing =
        ev.kind == kind::watcher && ev.what == what::destroy ? "" : ",";

      std::cout << ev << comma_or_nothing << std::endl;
    };

    return show_json;
  };

  auto maybe_time = [&](int const argc, char const** const argv)
  {
    auto given_or_zero_time = [&]()
    {
      auto t = [&] { return argc > 3 ? std::stoull(argv[3]) : 0; }();

      auto targis = [&](char const* a) { return argis(2, a); };

      // clang-format off
      return targis("-nanoseconds")  || targis("-ns")  ? nanoseconds(t)
           : targis("-microseconds") || targis("-us")  ? microseconds(t)
           : targis("-milliseconds") || targis("-ms")  ? milliseconds(t)
           : targis("-seconds")      || targis("-s")   ? seconds(t)
           : targis("-minutes")      || targis("-m")   ? minutes(t)
           : targis("-hours")        || targis("-h")   ? hours(t)
           : targis("-days")         || targis("-d")   ? 24   * hours(t)
           : targis("-weeks")        || targis("-w")   ? 168  * hours(t)
           : targis("-months")       || targis("-mts") ? 730  * hours(t)
           : targis("-years")        || targis("-y")   ? 8760 * hours(t)
                                                     : milliseconds(t);
      // clang-format on
    }();
    return given_or_zero_time > 0ns ? std::make_optional(given_or_zero_time)
                                    : std::nullopt;
  };

  auto maybe_help = [&]() -> std::optional<std::function<int()>>
  {
    auto help = []
    {
      std::cout << usage;
      return 0;
    };

    if (argis(1, "-h") || argis(1, "--help"))
      return help;
    else
      return std::nullopt;
  };

  return std::make_tuple(given_or_current_path(argc, argv),
                         maybe_filter_callback(),
                         maybe_time(argc, argv),
                         maybe_help());
};

/*  Watch a path for some time.
    Or watch a path forever.
    Show what happens, or show help. */
int main(int const argc, char const** const argv)
{
  auto [path, callback, time, help] = from_cmdline(argc, argv);

  if (help.has_value())
    return help.value()();

  else
    return watch_forever_or_expire(time)(path, callback);

  return 0;
};
