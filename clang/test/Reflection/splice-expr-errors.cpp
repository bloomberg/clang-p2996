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

                   // =======================================
                   // member_access_through_inaccessible_base
                   // =======================================

namespace member_access_through_inaccessible_base {
struct C {
  int pub = 1;
  int fn() const { return 2; }
};
struct E : private C {};  // expected-note 2 {{declared private here}}
struct F : protected C {};  // expected-note 2 {{declared protected here}}
struct G : C, E {};
  // expected-warning@-1 {{direct base 'C' is inaccessible due to ambiguity}}

// The member is accessible however it is designated, but the object
// expression must be convertible to a pointer to the designating class.
int a = E{}.[:^^C::pub:];
  // expected-error@-1 {{cannot cast 'E' to its private base class}}
int b = F{}.[:^^C::pub:];
  // expected-error@-1 {{cannot cast 'F' to its protected base class}}
int c = (new F)->[:^^C::fn:]();
  // expected-error@-1 {{cannot cast 'F' to its protected base class}}
int d = G{}.[:^^C::pub:];
  // expected-error@-1 {{ambiguous conversion from derived class 'G' to base class}}

template <typename T, info R>
int dependent(T t) {
  return t.[:R:];
    // expected-error@-1 {{to its private base class}}
}
int e = dependent<E, ^^C::pub>({});
  // expected-note@-1 {{in instantiation of function template specialization}}

}  // namespace member_access_through_inaccessible_base

                     // ==================================
                     // specialization_of_non_template
                     // ==================================

namespace specialization_of_non_template {
// The splice-specifier of a splice-specialization-specifier shall designate
// a template ([basic.splice]/2).
struct NT { static constexpr int v = 1; using type = int; };
template <typename> struct TB {};
int x;

using T1 = [:^^NT:]<int>;
  // expected-error@-1 {{cannot specialize a splice that does not designate a template}}
using T2 = typename [:^^int:]<char>;
  // expected-error@-1 {{cannot specialize a splice that does not designate a template}}
using T3 = [:^^TB<int>:]<char>;
  // expected-error@-1 {{cannot specialize a splice that does not designate a template}}
int v2 = template [:^^x:]<int>;
  // expected-error@-1 {{reflection not usable in a template splice}}
}  // namespace specialization_of_non_template
