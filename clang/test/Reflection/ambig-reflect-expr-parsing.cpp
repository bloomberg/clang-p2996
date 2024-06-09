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
// RUN: %clang_cc1 -std=c++26 -freflection -Wno-parentheses -verify %s


enum MyEnum { x, y, e = -1, f, z = 99 };

void func(MyEnum && x) { // ok
  MyEnum value{};
  MyEnum& ref = value;

  constexpr auto reflValue = ^value;

  constexpr bool test_comparison_0 = reflValue != (^MyEnum) && true; // ok
  constexpr bool test_comparison_1 = (reflValue != (^MyEnum) && true); // ok

  constexpr bool test_comparison_2 = ^MyEnum != (^MyEnum) && true; // ok
  constexpr bool test_comparison_3 = (^MyEnum != (^MyEnum) && true); // ok

  constexpr bool test_comparison_4 = (reflValue != (^MyEnum) & true); // ok

  constexpr bool test_comparison_5 = reflValue != ^MyEnum && true;
  // expected-warning@-1 {{token '&&' binds left to type 'T &&'; did you mean '(^T) &&'?}}
  // expected-error@-2 {{expected ';' at end of declaration}}
  constexpr bool test_comparison_6 = reflValue != ^MyEnum & true;
  // expected-warning@-1 {{token '&' binds left to type 'T &'; did you mean '(^T) &'?}}
  // expected-error@-2 {{expected ';' at end of declaration}}

  constexpr bool test_comparison_7 = ^MyEnum& == reflValue; // ok
  constexpr bool test_comparison_8 = ^MyEnum& != reflValue; // ok

  constexpr bool test_comparison_9 = ^MyEnum& < reflValue;
  // expected-error@-1 {{invalid operands to binary expression ('meta::info' and 'const meta::info')}}
  constexpr bool test_comparison_10 = ^MyEnum& > reflValue;
  // expected-error@-1 {{invalid operands to binary expression ('meta::info' and 'const meta::info')}}
  constexpr bool test_comparison_11 = ^MyEnum& <= reflValue;
  // expected-error@-1 {{invalid operands to binary expression ('meta::info' and 'const meta::info')}}
  constexpr bool test_comparison_12 = ^MyEnum& >= reflValue;
  // expected-error@-1 {{invalid operands to binary expression ('meta::info' and 'const meta::info')}}

  constexpr auto test_comparison_13 = reflValue != ^MyEnum&& || true; // ok
  constexpr auto test_comparison_14 = reflValue != ^MyEnum&  | true; // ok

  constexpr auto test_ref_type_0 = ^MyEnum &; // ok
  constexpr auto test_ref_type_1 = ^MyEnum &&; // ok
  constexpr auto test_ref_type_2 = (^MyEnum &); // ok
  constexpr auto test_ref_type_3 = (^MyEnum &&); // ok

  constexpr auto ternary_comparison_test_0 = reflValue == ^MyEnum& ? 1 : 0; // ok
  constexpr auto ternary_comparison_test_1 = (2 > 0) ? ^MyEnum& : ^MyEnum; // ok

  constexpr auto test_init_0 {^MyEnum&}; // ok
  constexpr auto test_type_id_0 = ^MyEnum&(*)(bool); // ok
}

