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
// RUN: %clang_cc1 %s -std=c++23 -freflection -freflection-new-syntax

using info = decltype(^^::);

template <info> void fn() { }
template <info> void fn2() { }

// Test when various function template instantiations do and do not resolve to
// the same function.

namespace myns {
struct Test {
  using type = int;
};

constexpr info rTest = ^^Test;
}  // namespace myns

struct Test {
  using type = int;
};

constexpr info null1;
constexpr info null2;

// Test equality of instantiations parameterized by reflection types.
static_assert(fn<null1> == fn<null2>);
static_assert(fn<^^int> == fn<^^int>);
static_assert(fn<^^int> != fn<^^void>);
static_assert(fn<^^int> != fn2<^^int>);
static_assert(fn<^^int> != fn<^^const int>);
static_assert(fn<^^int> != fn<^^::Test::type>);
static_assert(fn<^^int> != fn<^^myns::Test::type>);
static_assert(fn<^^int> != fn<myns::rTest>);
static_assert(fn<^^::Test::type> != fn<^^myns::Test::type>);
static_assert(fn<^^::Test::type> == fn<^^::Test::type>);

// Test instantiations in the presence of a variable holding the reflection.
constexpr info refl = ^^int;
static_assert(fn<refl> == fn<refl>);
static_assert(fn<refl> == fn<^^int>);
static_assert(fn<refl> != fn<null1>);
static_assert(fn<refl> != fn<^^myns::Test>);
static_assert(fn<refl> != fn<^^myns::Test::type>);
static_assert(fn<refl> != fn<^^::Test>);
static_assert(fn<refl> != fn<^^::Test::type>);

// Ensure that nested name specifiers are correctly handled.
static_assert(fn<^^myns::Test> == fn<myns::rTest>);
static_assert(fn<^^myns::Test> != fn<^^const myns::Test>);

// Ensure that decltypes are correctly handled.
static_assert(fn<^^decltype(3)> == fn<^^int>);

// Ensure that reflections of namespaces are handled correctly.
namespace Alias = ::myns;
static_assert(fn<^^::> == fn<^^::>);
static_assert(fn<^^myns> == fn<^^myns>);
static_assert(fn<^^Alias> == fn<^^Alias>);
static_assert(fn<^^::> != fn<^^myns>);
static_assert(fn<^^::> != fn<^^Alias>);
static_assert(fn<^^myns> != fn<^^Alias>);

// Ensure that reflections of templates are handled correctly.
static_assert(fn<^^fn> == fn<^^fn>);
static_assert(fn<^^fn2> == fn<^^fn2>);
static_assert(fn<^^fn> != fn<^^fn2>);
