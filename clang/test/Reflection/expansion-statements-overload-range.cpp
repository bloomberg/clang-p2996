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
// RUN: %clang_cc1 -std=c++2c -freflection-latest -fexpansion-statements -fsyntax-only -verify %s

void f();          // expected-note {{possible target for call}}
void f(int) {      // expected-note {{possible target for call}}
  template for (auto x : f); // expected-error {{reference to overloaded function could not be resolved; did you mean to call it?}}
}

int g();
void h() {
  template for (auto x : g); // expected-error {{cannot expand over a function 'int ()'; did you mean to call it with no arguments?}}
}

// With pointer
void h2() {
  int (*fp)() = g;
  template for (auto x : fp); // expected-error {{cannot expand over a function 'int (*)()'}}
}

template <typename T> int dep();
template <typename T>
void inst() {
  template for (auto x : dep<T>); // expected-error {{cannot expand over a function 'int ()'; did you mean to call it with no arguments?}}
}
template void inst<int>(); // expected-note {{in instantiation of function template specialization 'inst<int>' requested here}}