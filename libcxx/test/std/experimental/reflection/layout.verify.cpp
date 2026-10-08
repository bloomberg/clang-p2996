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
// [reflection]

#include <meta>

                             // ====================
                             // disallowed_operands
                             // ====================

// [meta.reflection.layout]/6, /8, /10: the queries are not defined for a
// variable of reference type, a bit-field (size and alignment), a value
// (alignment), or entities that are none of the listed kinds.
namespace disallowed_operands {
int g;
int &gref = g;
const int &cref = g;
struct S { int bf : 3; void fn(); };
enum E { e };

constexpr auto r1 = std::meta::size_of(^^gref);
  // expected-error@-1 {{must be initialized by a constant expression}} \
  // expected-note@-1 {{cannot query the size of a variable of reference type}}
constexpr auto r2 = std::meta::alignment_of(^^gref);
  // expected-error@-1 {{must be initialized by a constant expression}} \
  // expected-note@-1 {{cannot query the alignment of a variable of reference type}}
constexpr auto r3 = std::meta::bit_size_of(^^cref);
  // expected-error@-1 {{must be initialized by a constant expression}} \
  // expected-note@-1 {{cannot query the size of a variable of reference type}}
constexpr auto r4 = std::meta::size_of(^^S::bf);
  // expected-error@-1 {{must be initialized by a constant expression}} \
  // expected-note@-1 {{cannot query the size of a bit-field}}
constexpr auto r5 = std::meta::alignment_of(^^S::bf);
  // expected-error@-1 {{must be initialized by a constant expression}} \
  // expected-note@-1 {{cannot query the alignment of a bit-field}}
constexpr auto r6 = std::meta::alignment_of(std::meta::reflect_constant(3));
  // expected-error@-1 {{must be initialized by a constant expression}} \
  // expected-note@-1 {{cannot query the alignment of a value}}
constexpr auto r7 = std::meta::size_of(^^S::fn);
  // expected-error@-1 {{must be initialized by a constant expression}} \
  // expected-note@-1 {{cannot query the size of a function}}
constexpr auto r8 = std::meta::size_of(^^e);
  // expected-error@-1 {{must be initialized by a constant expression}} \
  // expected-note@-1 {{cannot query the size of an enumerator}}
constexpr auto r9 = std::meta::size_of(std::meta::data_member_spec(^^int, {.bit_width = 3}));
  // expected-error@-1 {{must be initialized by a constant expression}} \
  // expected-note@-1 {{cannot query the size of a description of a bit-field}}
}  // namespace disallowed_operands
