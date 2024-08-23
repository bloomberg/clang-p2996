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

#include <print>


class TU_Ticket {
  template<int N> struct Helper;
public:
  static consteval int next() {
    int k = 0;

    // Search for the next incomplete 'Helper<k>'.
    std::meta::info r;
    while (!is_incomplete_type(r = substitute(^^Helper,
                                             { std::meta::reflect_value(k) })))
      ++k;

    // Define 'Helper<k>' and return its index.
    define_class(r, {});
    return k;
  }
};

int main() {
  // RUN: grep "0, 1, 2" %t.stdout
  std::println("{}, {}, {}",
               TU_Ticket::next(), TU_Ticket::next(), TU_Ticket::next());
}
