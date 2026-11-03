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
// [meta.reflection.queries]/1: a function whose type contains an undeduced
// placeholder type has no type, and so no return type either.

#include <meta>

auto undeduced();
auto deduced() { return 1; }
template <typename T> auto tmpl(T t) { return t; }

constexpr auto t1 = std::meta::type_of(^^undeduced);
  // expected-error@-1 {{must be initialized by a constant expression}} \
  // expected-note@-1 {{cannot query the type of a function before its return type is deduced}}
constexpr auto r1 = std::meta::return_type_of(^^undeduced);
  // expected-error@-1 {{must be initialized by a constant expression}} \
  // expected-note@-1 {{cannot query the type of a function before its return type is deduced}}

static_assert(std::meta::type_of(^^deduced) == ^^int());
static_assert(std::meta::return_type_of(^^deduced) == ^^int);
static_assert(std::meta::return_type_of(^^tmpl<char>) == ^^char);
