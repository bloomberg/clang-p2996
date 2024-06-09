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
  // expected-warning@-1 {{given expression may be misinterpreted due to 'T &&' type parsing logic. did you mean '... (^T) && ...'?}}
  // expected-error@-2 {{expected ';' at end of declaration}}
  constexpr bool test_comparison_6 = reflValue != ^MyEnum & true;
  // expected-warning@-1 {{given expression may be misinterpreted due to 'T &' type parsing logic. did you mean '... (^T) & ...'?}}
  // expected-error@-2 {{expected ';' at end of declaration}}

  constexpr auto test_ref_type_0 = ^int &; // ok
  constexpr auto test_ref_type_1 = ^int &&; // ok
  constexpr auto test_ref_type_2 = (^int &); // ok
  constexpr auto test_ref_type_3 = (^int &&); // ok
}

