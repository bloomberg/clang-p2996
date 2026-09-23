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
// RUN: %clang_cc1 %s -std=c++23 -freflection -verify

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
    [:^^S::fn:]();  // expected-error {{cannot implicitly reference}} \
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

                       // ===============================
                       // parameter_declaration_ambiguity
                       // ===============================

namespace parameter_declaration_ambiguity {
void fn([:^^int:]);
  // expected-error@-1 {{variable has incomplete type}} \
  // expected-error@-1 {{not usable in a splice expression}}

}  // namespace parameter_declaration_ambiguity

                              // =================
                              // enclosing_lambdas
                              // =================

namespace enclosing_lambdas {
void fn() {
  int x = 1;  // expected-note {{'x' declared here}}
  constexpr auto r = ^^x;

  (void) [] -> decltype([:r:]) {
    return [:r:];
      // expected-error@-1 {{'x' for which there is an intervening lambda}}
  };
}
}  // namespace enclosing_lambdas
