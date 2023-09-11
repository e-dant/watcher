#include "wtr/watcher.hpp"
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <filesystem>
#include <functional>
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <tuple>

/*  @todo
    [ -filter-effect-type
      < all rename modify create destroy owner other > = all
    ]
    [ -filter-path-type
      < all dir file hard_link sym_link watcher other > = all
    ] */
inline constexpr auto usage =
  "wtr.watcher [ -h | --help ] [ PATH = . [ -UNIT < TIME > "
  "]\n"
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
  -> std::function<
    bool(std::filesystem::path const&, wtr::event::callback const&)>
{
  return [alive_for](
           std::filesystem::path const& path,
           wtr::event::callback const& callback)
  {
    using namespace std::chrono;

    auto const then = system_clock::now();
    std::cout << R"({"wtr":{"watcher":{"stream":{)" << std::endl;

    auto watcher = wtr::watch(path, callback);

    if (alive_for.has_value())
      std::this_thread::sleep_for(alive_for.value());

    else
      std::cin.get();

    std::cout << "}"
              << "\n,\"milliseconds\":"
              << duration_cast<milliseconds>(system_clock::now() - then).count()
              << "\n}}}" << std::endl;

    return true;
  };
};

auto from_cmdline(int const argc, char const** const argv)
{
  using namespace std::chrono;

  auto argis = [&](auto const i, char const* a)
  { return argc > i ? std::strcmp(a, argv[i]) == 0 : false; };

  auto given_or_current_path = [](int const argc, char const** const argv)
  {
    namespace fs = std::filesystem;

    return fs::canonical(
      [&]
      {
        auto p = fs::path(argc > 1 ? argv[1] : ".");
        return fs::exists(p) ? p : fs::current_path();
      }());
  };

  /*  Show what happens.
      Use the event's stream operator, formatting as json.
      Append a comma until the last event: Watcher dying.
      Flush stdout, via `endl`, for piping.
      For manual parsing, see the file watcher/event.hpp. */
  wtr::event::callback show_json = [](wtr::event const& ev) noexcept
  {
    auto comma_or_nothing =
      ev.path_type == wtr::event::path_type::watcher
          && ev.effect_type == wtr::event::effect_type::destroy
        ? ""
        : ",";

    std::cout << ev << comma_or_nothing << std::endl;
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

    return given_or_zero_time.count() > 0
           ? std::make_optional(given_or_zero_time)
           : std::nullopt;
  };

  auto maybe_help = [&]() -> std::optional<std::function<int()>>
  {
    auto help = [] { return (std::cout << usage, 0); };

    if (argis(1, "-h") || argis(1, "--help"))
      return help;

    else
      return std::nullopt;
  };

  return std::make_tuple(
    given_or_current_path(argc, argv),
    show_json,
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
