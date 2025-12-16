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
// ADDITIONAL_COMPILE_FLAGS: -fparameter-reflection

// <experimental/reflection>
//
// [reflection]

#include <meta>
#include <ranges>
#include <string_view>

constexpr auto ctx = std::meta::access_context::unchecked();

namespace test_with_base_specifier {

struct A {};
struct B : A {};

template <typename T>
consteval auto list_bases() {
  std::string result{};
  result += identifier_of(^^T);
  result += ":";
  for (auto base : bases_of(^^T, ctx)) {
    result += identifier_of(base);
  }
  return std::define_static_string(result);
}

static_assert(std::string_view(list_bases<B>()) == "B:A");

} // namespace test_with_base_specifier

int main() {}
