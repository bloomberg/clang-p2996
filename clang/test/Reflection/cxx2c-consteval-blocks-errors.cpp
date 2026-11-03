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

// The expression of a consteval block must be a constant expression
// ([dcl.pre]); an evaluation that leaks an allocation or has undefined
// behavior is not one, even if it produces a value.
consteval { new int; }
  // expected-error@-1 {{evaluating expression of a consteval block must be a constant expression}} \
  // expected-note@-1 {{allocation performed here was not deallocated}}

consteval { int *p = new int; }
  // expected-error@-1 {{must be a constant expression}} \
  // expected-note@-1 {{allocation performed here was not deallocated}}

consteval { int *p = new int; delete p; }

consteval { int i = __INT_MAX__; ++i; }
  // expected-error@-1 {{must be a constant expression}} \
  // expected-note@-1 {{outside the range of representable values}} \
  // expected-note@-1 {{in call to}}

template <typename T>
struct S {
  consteval { new T; }
    // expected-error@-1 {{must be a constant expression}} \
    // expected-note@-1 {{allocation performed here was not deallocated}}
};
template struct S<int>;
  // expected-note@-1 {{in instantiation of template class}}
