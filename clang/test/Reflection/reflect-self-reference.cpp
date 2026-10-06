//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// A reflection is a reference to an entity: AST visitors must not descend
// into the reflected declaration, which may contain the reflection itself, or
// constructs that are only diagnosed in the context of the visitor.
//
// RUN: %clang_cc1 %s -std=c++26 -freflection -fexpansion-statements -verify
// expected-no-diagnostics

using info = decltype(^^int);

                            // ====================
                            // reflecting_the_enclosing_entity
                            // ====================

int f() {
  constexpr info r = ^^f;
  static_assert(r == ^^f);
  return 0;
}

struct S {
  static constexpr info self = ^^S;
  void m() { constexpr info r = ^^S::m; }
  static_assert(self == ^^S);
};

namespace ns {
constexpr info self = ^^ns;
}  // namespace ns

template <typename... Ts>
struct Pack {
  static constexpr int n = sizeof...(Ts);
  void m() { constexpr info r = ^^Pack; }
};
static_assert(Pack<int, long>::n == 2);

                          // ==========================
                          // labels_in_reflected_functions
                          // ==========================

// A label in the reflected function is not a label of the expansion body.
int g() {
  label:
  return 0;
}

consteval int h() {
  int n = 0;
  template for (constexpr int i : {1, 2}) {
    constexpr info r = ^^g;
    n += i;
  }
  return n;
}
static_assert(h() == 3);
