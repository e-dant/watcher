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
#include <watcher/watcher.hpp>

/* Watch a path for some time.
   Or watch a path forever.
   Show what happens. */
int main(int argc, char** argv)
{
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

    auto const path = [&]() {
      namespace fs = std::filesystem;
      auto const maybe_given_path = fs::path(argc > 1 ? argv[1] : ".");
      return fs::exists(maybe_given_path) ? fs::absolute(maybe_given_path)
                                          : fs::current_path();
    };

    auto const unit = [&](auto const& f) {
      auto const opt
          = [&](char const* a) { return std::strcmp(a, argv[2]) == 0; };
      return opt("-nanoseconds") || opt("-ns")     ? nanoseconds(f())
             : opt("-microseconds") || opt("-mcs") ? microseconds(f())
             : opt("-milliseconds") || opt("-ms")  ? milliseconds(f())
             : opt("-seconds") || opt("-s")        ? seconds(f())
             : opt("-minutes") || opt("-m")        ? minutes(f())
             : opt("-hours") || opt("-h")          ? hours(f())
             : opt("-days") || opt("-d")           ? days(f())
             : opt("-weeks") || opt("-w")          ? weeks(f())
             : opt("-months") || opt("-mts")       ? months(f())
             : opt("-years") || opt("-y")          ? years(f())
                                                   : milliseconds(f());
    };

    auto const time = [&]() { return argc > 3 ? std::stoull(argv[3]) : 0; };

    return std::make_tuple(path(), unit(time));
  };

  /* Show what happens.
     Use the event's stream operator, formatting as json.
     Append a comma until the last event: Watcher dying.
     Flush stdout, via `endl`, for piping.
     For manual parsing, see the file watcher/event.hpp. */
  auto const stream_json = [](auto const& this_event) {
    using event::kind::watcher, event::what::destroy;

    auto const maybe_comma
        = this_event.kind == watcher && this_event.what == destroy ? "" : ",";

    std::cout << this_event << maybe_comma << std::endl;
  };

  auto const watch_expire = [](auto const& path, auto const& callback,
                               auto const& alive_for) -> bool {
    using namespace std::chrono;
    auto const then = system_clock::now();
    std::cout << R"({"wtr":{"watcher":{"stream":{)" << std::endl;

    /* This is the heart of the function.
       Watching, concurrently. */
    auto life = std::async(std::launch::async,
                           [&]() { return watch(path, callback); });

    /* Until our time is up. */
    life.wait_for(alive_for);

    /* Then dying. */
    auto const died = die(path, callback);
    auto const lived = life.get();

    std::cout << "}"
              << "\n,\"milliseconds\":"
              << duration_cast<milliseconds>(system_clock::now() - then).count()
              << "\n,\"dead\":" << (died && lived ? "true" : "false") << "\n}}}"
              << std::endl;

    return died && lived;
  };

  auto const [path, alive_for] = lift_options(argc, argv);

  return alive_for > 0ns
             /* Either watch for some time */
             ? !watch_expire(path, stream_json, alive_for)
             /* Or run forever */
             : !watch(path, stream_json);
}
