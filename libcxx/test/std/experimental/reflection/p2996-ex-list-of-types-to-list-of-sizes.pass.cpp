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

#include <array>
#include <ranges>


int main() {
  constexpr std::array types = {^^int, ^^float, ^^double};

  // https://bugs.llvm.org/show_bug.cgi?id=25627
  constexpr std::array sizes = [=]{
    std::array<std::size_t, types.size()> r;
    std::ranges::transform(types, r.begin(), std::meta::size_of);
    return r;
  }();

  static_assert(sizes[0] == sizeof(int));
  static_assert(sizes[1] == sizeof(float));
  static_assert(sizes[2] == sizeof(double));
}
