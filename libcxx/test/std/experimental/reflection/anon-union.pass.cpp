//===----------------------------------------------------------------------===//
//
// Copyright 2025 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20
// ADDITIONAL_COMPILE_FLAGS: -freflection

// <experimental/reflection>
//
// [reflection]

struct foo {
  int i;
  union {
    int a;
    long b;
  };
};

constexpr foo bar { .i = 11, .a = 1 };
static_assert(bar.[:^^foo::a:] == 1);
static_assert(bar.*&[:^^foo::a:] == 1);

int main() {
  return 0;
}
