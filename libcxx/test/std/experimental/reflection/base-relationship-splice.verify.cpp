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
// [expr.ref]/6, /8.6: a splice designating a direct base class relationship
// (D, B) is only usable as the right operand of a class member access whose
// object expression is of class D or of a class derived from D.

#include <meta>

using namespace std::meta;

struct B { int b; };
struct D : B {};
struct U {};
struct P : private D {};  // expected-note 2 {{declared private here}}
constexpr auto rB = bases_of(^^D, access_context::unchecked())[0];

// Not usable outside of a class member access.
auto *p = &[:rB:];  // expected-error {{reflection not usable in a splice expression}}
auto &r = [:rB:];  // expected-error {{reflection not usable in a splice expression}}

// The object expression must be of class D or derived from D.
int u = U{}.[:rB:].b;  // expected-error {{class 'U' not derived from 'D'}}
int b = B{}.[:rB:].b;  // expected-error {{class 'B' not derived from 'D'}}

// The conversion of the object expression to D must be accessible.
int pb = P{}.[:rB:].b;  // expected-error {{cannot cast 'P' to its private base class 'D'}}
int pp = static_cast<P *>(nullptr)->[:rB:].b;
  // expected-error@-1 {{cannot cast 'P' to its private base class 'D'}}

// A splice of an unrelated reflection kind is not usable either.
int t = D{}.[:^^int:];  // expected-error {{reflection not usable in a splice expression}}
