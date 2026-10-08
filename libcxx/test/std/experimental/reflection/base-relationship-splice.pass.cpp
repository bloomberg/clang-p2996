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
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

// <experimental/reflection>
//
// [expr.ref]/8.6: a class member access whose right operand is a splice
// designating a direct base class relationship (D, B).

#include <meta>
#include <type_traits>

#pragma clang diagnostic ignored "-Winaccessible-base"
#pragma clang diagnostic ignored "-Wmissing-braces"

using namespace std::meta;

                             // ================
                             // standard_example
                             // ================

namespace standard_example {
struct B {
  int b;
};
struct C : B {
  constexpr int get() const { return b; }
};
struct D : B, C {};
constexpr int f() {
  D d = {1, {}};
  // b unambiguously refers to the direct base class of type B, not the
  // indirect base class of type B.
  B &b = d.[: bases_of(^^D, access_context::current())[0] :];
  b.b += 10;
  return 10 * b.b + d.get();
}
static_assert(f() == 110);
}  // namespace standard_example

                              // =============
                              // object_kinds
                              // =============

namespace object_kinds {
struct B { int b = 1; };
struct V { int v = 2; };
struct D : private B, V {};
struct E : D { int e = 3; };
constexpr auto rB = bases_of(^^D, access_context::unchecked())[0];
constexpr auto rV = bases_of(^^D, access_context::unchecked())[1];

// The access of the base does not matter; the object expression may be of the
// derived class D or of a class derived from it, an lvalue, a pointer, or a
// prvalue.
constexpr int g() {
  D d;
  E e;
  const D *p = &d;
  d.[:rB:].b = 5;
  return d.[:rB:].b + p->[:rB:].b + e.[:rB:].b + D{}.[:rV:].v + e.[:rV:].v;
}
static_assert(g() == 15);

// Value categories and types ([expr.ref]/8.6).
[[maybe_unused]] D d;
[[maybe_unused]] const D cd;
static_assert(std::is_same_v<decltype((d.[:rB:])), B &>);
static_assert(std::is_same_v<decltype((cd.[:rB:])), const B &>);
static_assert(std::is_same_v<decltype((D{}.[:rB:])), B &&>);
static_assert(std::is_same_v<decltype((&d)->[:rV:]), V &>);

// Dependent object expressions and reflections.
template <info R, typename T>
constexpr int h(T &&t) { return static_cast<T &&>(t).[:R:].b; }
static_assert(h<rB>(E{}) == 1);
template <typename T>
constexpr int k(const T &t) {
  return t.[: bases_of(^^T, access_context::unchecked())[0] :].b;
}
static_assert(k(D{}) == 1);

// A virtual base.
struct W { int w = 4; };
struct VD : virtual W {};
struct VE : VD {};
constexpr auto rW = bases_of(^^VD, access_context::unchecked())[0];
int virtual_base() {
  VE ve;
  VD *p = &ve;
  return ve.[:rW:].w + p->[:rW:].w;
}
}  // namespace object_kinds

int main() {
  if (object_kinds::virtual_base() != 8)
    return 1;
}
