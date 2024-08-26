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

// <experimental/reflection>
//
// [reflection]
//
// RUN: %{build}
// RUN: %{exec} %t.exe > %t.stdout

#include <experimental/meta>

#include <algorithm>
#include <array>
#include <print>
#include <ranges>
#include <string_view>


// Represents a 'std::meta::info' constrained by a predicate.
template <std::meta::info Pred>
  requires (^^decltype([:Pred:](^^int)) == ^^bool)
struct metatype {
  std::meta::info value;

  // Construction is ill-formed unless predicate is satisfied.
  consteval metatype(std::meta::info r) : value(r) {
    if (![:Pred:](r))
      throw "Reflection is not a member of this metatype";
  }

  // Cast to 'std::meta::info' allows values of this type to be spliced.
  consteval operator std::meta::info() const { return value; }

  static consteval bool check(std::meta::info r) { return [:Pred:](r); }
};

// Type representing a "failure to match" any known metatypes.
struct unmatched {
  consteval unmatched(std::meta::info) {}
  static consteval bool check(std::meta::info) { return true; }
};

// Returns the given reflection "enriched" with a more descriptive type.
template <typename... Choices>
consteval std::meta::info enrich(std::meta::info r) {
  // Because we control the type, we know that the constructor taking info is
  // the first constructor. The copy/move constructors are added at the }, so
  // will be the last ones in the list.
  std::array ctors = {
      (members_of(^^Choices) |
           std::views::filter(std::meta::is_constructor) |
           std::views::filter(std::meta::is_user_provided)).front()...,
      (members_of(^^unmatched) |
           std::views::filter(std::meta::is_constructor) |
           std::views::filter(std::meta::is_user_provided)).front()
  };
  std::array checks = {^^Choices::check..., ^^unmatched::check};

  for (auto [check, ctor] : std::views::zip(checks, ctors))
    if (extract<bool>(reflect_invoke(check, {reflect_value(r)})))
      return reflect_invoke(ctor, {reflect_value(r)});

  std::unreachable();
}

using type_t = metatype<^^std::meta::is_type>;
using template_t = metatype<^^std::meta::is_template>;

// Example of a function overloaded for different "types" of reflections.
void PrintKind(type_t) { std::println("type"); }
void PrintKind(template_t) { std::println("template"); }
void PrintKind(unmatched) { std::println("unknown kind"); }

// RUN: cat %t.stdout | head -n1 | grep template
// RUN: cat %t.stdout | head -n2 | tail -n1 | grep type
// RUN: cat %t.stdout | head -n3 | tail -n1 | grep "unknown kind"
int main() {
  // Classifies any reflection as one of: Type, Template, or Unmatched.
  auto enrich = [](std::meta::info r) {
    return ::enrich<type_t, template_t>(r);
  };

  // Demonstration of using 'enrich' to select an overload.
  PrintKind([:enrich(^^metatype):]);                    // "template"
  PrintKind([:enrich(^^type_t):]);                      // "type"
  PrintKind([:enrich(std::meta::reflect_value(3)):]);  // "unknown kind"
}
