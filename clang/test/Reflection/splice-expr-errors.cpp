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
// RUN: %clang_cc1 %s -std=c++23 -freflection -freflection-new-syntax -verify

using info = decltype(^^int);

                         // ===========================
                         // with_implicit_member_access
                         // ===========================

namespace with_implicit_member_access {

// Non-dependent case
struct S {
  int k;

  void fn2() { }

  void fn() {
    (void) [:^^k:];  // expected-error {{cannot implicitly reference}} \
                     // expected-note {{explicit 'this' pointer}}
    (void) [:^^S:]::k;  // expected-error {{cannot implicitly reference}} \
                        // expected-note {{explicit 'this' pointer}}
    [:^^fn:]();  // expected-error {{cannot implicitly reference}} \
                 // expected-note {{explicit 'this' pointer}}
    [:^^S:]::fn2();  // expected-error {{cannot implicitly reference}} \
                     // expected-note {{explicit 'this' pointer}}
  }
};

// Dependent case
struct D {
  int k;

  void fn2() { }

  template <typename T>
  void fn() {
    (void) [:^^T:]::k;  // expected-error {{cannot implicitly reference}} \
                        // expected-note {{explicit 'this' pointer}}
    [:^^T:]::fn2();  // expected-error {{cannot implicitly reference}} \
                     // expected-note {{explicit 'this' pointer}}
  }
};

void runner() {
    D f = {4};
    f.fn<D>();  // expected-note {{in instantiation of function template}}
}

}  // namespace with_implicit_member_access
