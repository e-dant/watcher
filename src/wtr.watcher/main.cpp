/* std::chrono::<units> */
#include <chrono>
/* std::strcmp */
#include <cstring>
/* std::filesystem::current_path
   std::filesystem::absolute
   std::filesystem::exists
   std::filesystem::path */
#include <filesystem>
/* std::launch::async
   std::async */
#include <future>
/* std::cout
   std::endl */
#include <iostream>
/* std::stoull */
#include <string>
/* std::make_tuple */
#include <tuple>
/* wtr::watcher::event::event
   wtr::watcher::event::what
   wtr::watcher::event::kind
   wtr::watcher::watch
   wtr::watcher::die */
#include <wtr/watcher.hpp>

/* Watch a path for some time.
   Or watch a path forever.
   Show what happens. */
int main(int argc, char** argv) {
  using namespace wtr::watcher;
  using namespace std::chrono_literals;

  /* Lift the user's choices from the command line.
     The options may be:
     1. Path to watch (optional)
     2. Time until death (optional)
     3. Time unit (optional, defaults to milliseconds)

     If the path to watch is unspecified,
     we use the user's current directory.

     If we aren't told when to die,
     we never do. */
  auto const lift_options = [](auto const& argc, auto const& argv) {
    using namespace std::chrono;

    auto const path = [&] {
      namespace fs = std::filesystem;
      auto const maybe_given_path = fs::path(argc > 1 ? argv[1] : ".");
      return fs::exists(maybe_given_path) ? fs::absolute(maybe_given_path)
                                          : fs::current_path();
    };

    auto const unit = [&](auto const& f) {
      auto const argis = [&](char const* a) {
        return argc > 2 ? std::strcmp(a, argv[2]) == 0 : false;
      };
      return argis("-nanoseconds") || argis("-ns")   ? nanoseconds(f())
           : argis("-microseconds") || argis("-mcs") ? microseconds(f())
           : argis("-milliseconds") || argis("-ms")  ? milliseconds(f())
           : argis("-seconds") || argis("-s")        ? seconds(f())
           : argis("-minutes") || argis("-m")        ? minutes(f())
           : argis("-hours") || argis("-h")          ? hours(f())
           : argis("-days") || argis("-d")           ? 24 * hours(f())
           : argis("-weeks") || argis("-w")          ? 168 * hours(f())
           : argis("-months") || argis("-mts")       ? 730 * hours(f())
           : argis("-years") || argis("-y")          ? 8760 * hours(f())
                                                     : milliseconds(f());
    };

    auto const help = [&]() -> std::function<bool()> {
      auto const argcmp = [&](auto const i, char const* a) {
        return argc > i ? std::strcmp(a, argv[i]) == 0 : false;
      };
      for (auto i = 0; i < argc; i++)
        if (argcmp(i, "-h") || argcmp(i, "--help"))
          return [] {
            std::cout << "wtr.watcher [ path = . [ time = 0 [ unit = ms ] ] ]"
                      << std::endl;
            return true;
          };
      return [] { return false; };
    };

    auto const time = [&] { return argc > 3 ? std::stoull(argv[3]) : 0; };

    return std::make_tuple(path(), unit(time), help());
  };

  /* Show what happens.
     Use the event's stream operator, formatting as json.
     Append a comma until the last event: Watcher dying.
     Flush stdout, via `endl`, for piping.
     For manual parsing, see the file watcher/event.hpp. */
  auto const stream_json = [](auto const& ev) {
    using event::kind, event::what;

    auto const maybe_comma
    = ev.kind == kind::watcher && ev.what == what::destroy ? "" : ",";

    std::cout << ev << maybe_comma << std::endl;
  };

  auto const watch_expire
  = [](auto const& path, auto const& callback, auto const& alive_for) -> bool {
    using namespace std::chrono;
    auto const then = system_clock::now();
    std::cout << R"({"wtr":{"watcher":{"stream":{)" << std::endl;

    /* This is the heart of the function.
       Watching, concurrently. */
    auto lifetime = watch(path, callback);

    /* Until our time is up. */
    std::this_thread::sleep_for(alive_for);

    /* Then dying. */
    auto const dead = lifetime();

    std::cout << "}"
              << "\n,\"milliseconds\":"
              << duration_cast<milliseconds>(system_clock::now() - then).count()
              << "\n,\"dead\":" << (dead ? "true" : "false") << "\n}}}"
              << std::endl;

    return dead;
  };

  auto const [path, alive_for, help] = lift_options(argc, argv);

  /* Might show help,
     Watch for some time,
     Or run forever. */
  return help()

       ? 0

       : alive_for > 0ns

         ? ! watch_expire(path, stream_json, alive_for)

         : ! watch(path, stream_json)();
}
