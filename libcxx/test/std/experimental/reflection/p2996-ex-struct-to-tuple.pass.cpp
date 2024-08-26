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
//
// RUN: %{build}
// RUN: %{exec} %t.exe > %t.stdout

#include <experimental/meta>

#include <array>
#include <print>
#include <ranges>
#include <tuple>


consteval auto struct_to_tuple_type(std::meta::info type) -> std::meta::info {
  constexpr auto remove_cvref = [](std::meta::info r) consteval {
    return substitute(^^std::remove_cvref_t, {r});
  };

  return substitute(^^std::tuple,
                    nonstatic_data_members_of(type)
                    | std::views::transform(std::meta::type_of)
                    | std::views::transform(remove_cvref)
                    | std::ranges::to<std::vector>());
}

template <typename To, typename From, std::meta::info... members>
constexpr auto struct_to_tuple_helper(From const& from) -> To {
  return To(from.[:members:]...);
}

template<typename From>
consteval auto get_struct_to_tuple_helper() {
  using To = [: struct_to_tuple_type(^^From) :];

  std::vector args = {^^To, ^^From};
  for (auto mem : nonstatic_data_members_of(^^From)) {
    args.push_back(reflect_value(mem));
  }

  return extract<To(*)(From const&)>(substitute(^^struct_to_tuple_helper,
                                                args));
}

template <typename From>
constexpr auto struct_to_tuple(From const& from) {
  return get_struct_to_tuple_helper<From>()(from);
}

struct S { bool i; int j; int k; };

int main() {
  {
    constexpr S s = {true, 55, 66};
    constexpr auto t = struct_to_tuple(s);
    std::println("({}, {}, {})", get<0>(t), get<1>(t), get<2>(t));
  }

  {
    auto t = struct_to_tuple<S>({false, 11, 22});
    std::println("({}, {}, {})", get<0>(t), get<1>(t), get<2>(t));
  }
}
