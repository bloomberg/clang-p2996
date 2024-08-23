//===----------------------------------------------------------------------===//
//
// Copyright 2024 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20
// ADDITIONAL_COMPILE_FLAGS: -freflection
// ADDITIONAL_COMPILE_FLAGS: -freflection-new-syntax
// ADDITIONAL_COMPILE_FLAGS: -Wno-inconsistent-missing-override

// <experimental/reflection>
//
// [reflection]

#include <experimental/meta>

#include <print>
#include <type_traits>


// start 'expand' definition
namespace __impl {
  template<auto... vals>
  struct replicator_type {
    template<typename F>
      constexpr void operator>>(F body) const {
        (body.template operator()<vals>(), ...);
      }
  };

  template<auto... vals>
  replicator_type<vals...> replicator = {};
}

template<typename R>
consteval auto expand(R range) {
  std::vector<std::meta::info> args;
  for (auto r : range) {
    args.push_back(std::meta::reflect_value(r));
  }
  return substitute(^^__impl::replicator, args);
}
// end 'expand' definition


template <typename E>
  requires std::is_enum_v<E>
constexpr std::string enum_to_string(E value) {
    std::string result = "<unnamed>";
    [:expand(enumerators_of(^^E)):] >> [&] <auto e> {
        if (value == [:e:]) {
            result = std::string(identifier_of(e));
        }
    };
    return result;
}

template <typename E>
  requires std::is_enum_v<E>
constexpr std::optional<E> string_to_enum(std::string_view name) {
  std::optional<E> result = std::nullopt;
  [:expand(enumerators_of(^^E)):] >> [&] <auto e> {
    if (name == identifier_of(e)) {
      result = [:e:];
    }
  };
  return result;
}

int main() {
  enum Color { red, green, blue };

  static_assert(enum_to_string(Color::red) == "red");
  static_assert(enum_to_string(Color(42)) == "<unnamed>");

  static_assert(string_to_enum<Color>("red") == Color::red);
  static_assert(string_to_enum<Color>("blue") == Color::blue);
  static_assert(string_to_enum<Color>("yellow") == std::nullopt);

  std::println("{} (red)", enum_to_string(Color::red));
  std::println("{} (blue)", enum_to_string(Color::blue));
}
