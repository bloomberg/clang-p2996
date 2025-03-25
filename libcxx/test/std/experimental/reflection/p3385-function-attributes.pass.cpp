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
// ADDITIONAL_COMPILE_FLAGS: -fattribute-reflection

// <experimental/reflection>
//
// [reflection]

#include <experimental/meta>

[[nodiscard]] int func();

consteval bool testAttributesOfFunc() {
  static_assert(attributes_of(^^func).size() == 1);
  static_assert(identifier_of(attributes_of(^^func)[0]) == "nodiscard");
  static_assert(attributes_of(^^func)[0] == ^^[[nodiscard]]);
  return true;
}

int main() 
{
    static_assert(testAttributesOfFunc());
}