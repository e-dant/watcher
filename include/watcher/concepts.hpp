#pragma once

#include <string>

namespace water {

namespace concepts {
template <typename From>
concept String = requires(From str) {
  std::string{str};
};

template <typename From>
concept Path = requires {
  String<From>;
};

template <typename Type, typename... Types>
concept Same = std::is_same<Type, Types...>::value;

template <typename... Types>
concept Invocable = requires(Types... values) {
  std::is_invocable<Types...>(values...)
      or std::is_invocable_v<Types...>(values...)
      or std::is_invocable_r_v<Types...>(values...);
};

template <typename Returns, typename... Accepts>
concept Callback = requires(Returns fn, Accepts... args) {
  // A callback doesn't return a value,
  Same<void, Returns>
      // takes one or more arguments,
      and not Same<void, Accepts...>
      // and is callable.
      and Invocable<Returns, Accepts...>;
};

}  // namespace concepts
}  // namespace water