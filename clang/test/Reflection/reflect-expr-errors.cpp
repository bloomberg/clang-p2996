//===----------------------------------------------------------------------===//
//
// Copyright 2024 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// RUN: %clang_cc1 %s -std=c++26 -freflection -verify

using info = decltype(^^int);

                            // ===================
                            // constructor_operand
                            // ===================

namespace constructor_operand {
// After a nested-name-specifier nominating class C, the injected-class-name
// of C names the constructors of C ([class.qual]/2), an overload set for
// which '&C::C' is ill-formed ([expr.reflect]/7.2).
struct S { S(); S(int); };
template <typename T> struct TS { TS(); };

constexpr info r1 = ^^S::S;
  // expected-error@-1 {{cannot take the reflection of the constructors of 'constructor_operand::S'}}
constexpr info r2 = ^^TS<int>::TS;
  // expected-error@-1 {{cannot take the reflection of the constructors of 'TS<int>'}}
struct Inner {
  struct Nested { Nested(); };
  static constexpr info r3 = ^^Inner::Inner;
    // expected-error@-1 {{cannot take the reflection of the constructors of 'constructor_operand::Inner'}}
  static constexpr info r4 = ^^Inner;
  static constexpr info r5 = ^^Nested::Nested;
    // expected-error@-1 {{cannot take the reflection of the constructors of 'constructor_operand::Inner::Nested'}}
};

// The injected-class-name found through a different class names the type.
struct Base {};
struct Derived : Base {};
static_assert(^^Derived::Base == ^^Base);
static_assert(^^Inner::Nested == ^^Inner::Nested);
}  // namespace constructor_operand

                            // ===================
                            // invalid_declaration
                            // ===================

namespace invalid_declaration {
// A reflection of an invalid declaration is not formed (this crashed).
struct P { int x, y; };
void fn() {
  auto &[u, v] = P{};
    // expected-error@-1 {{non-const lvalue reference to type 'P' cannot bind to a temporary}}
  constexpr auto r = ^^u;
}
}  // namespace invalid_declaration
