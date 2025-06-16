//===----------------------------------------------------------------------===//
//
// Copyright 2025 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// RUN: %clang_cc1 %s -std=c++26 -freflection -verify

using info = decltype(^^int);

                                // ============
                                // basic_splice
                                // ============

namespace basic_splice {

constexpr int v = 1;
template <int V> struct TCls {
  static constexpr int s = V + 1;
};

using alias = [:^^TCls:]<([:^^v:])>;

static_assert(alias::s == 2);

auto o1 = [:^^TCls:]<([:^^v:])>();
  // expected-error@-1 {{not usable in a splice expression}} \
  // expected-error@-1 {{expected expression}}
auto o2 = typename [:^^TCls:]<([:^^v:])>();

consteval int bad_splice(info v) {  // expected-note {{declared here}}
    return [:v:]; // expected-error {{operand must be a constant expression}} \
                  // expected-note {{parameter 'v' with unknown value}}
}
}  // namespace basic_splice

                            // ====================
                            // expr_prim_id_general
                            // ====================

namespace expr_prim_id_general {
struct S {
  int m;
};
int S::*k = &[:^^S::m:];        // OK
}  // namespace expr_prim_id_general

                              // =================
                              // expr_prim_id_qual
                              // =================

namespace expr_prim_id_qual {
template <int V>
struct TCls {
  static constexpr int s = V;
  using type = int;
};

constexpr int v1 = [:^^TCls<1>:]::s;
constexpr int v2 = template [:^^TCls:]<2>::s;

constexpr typename [:^^TCls:]<3>::type v3 = 3;

template [:^^TCls:]<3>::type v4 = 4;

void fn() {
  [:^^TCls:]<3>::type v5 = 5;
    // expected-error@-1 {{reflection not usable in a splice expression}} \
    // expected-error@-1 {{no member named 'type' in the global namespace}}
}
}  // namespace expr_prim_id_qual

                             // ==================
                             // expr_prim_req_type
                             // ==================

namespace expr_prim_req_type {
template<typename T> concept C = requires {
  typename [:T::r1:];
  typename [:T::r2:]<int>;
};
}  // namespace expr_prim_req_type

                              // ================
                              // expr_prim_splice
                              // ================

namespace expr_prim_splice {
struct S { static constexpr int a = 1; };
template <typename> struct TCls { static constexpr int b = 2; };

constexpr int c = [:^^S:]::a;

constexpr int d = template [:^^TCls:]<int>::b;
template <auto V> constexpr int e = [:V:];
constexpr int f = template [:^^e:]<^^S::a>;

auto g = typename [:^^int:](42);
}  // namespace expr_prim_splice

                               // ===============
                               // dcl_type_splice
                               // ===============

namespace dcl_type_splice {
struct S { using type = int; };
template <auto R> struct TCls {
  typename [:R:]::type member;
};

int fn() {
  [:^^S::type:] *var;
    // expected-error@-1 {{reflection not usable in a splice expression}} \
    // expected-error@-1 {{undeclared identifier 'var'}}
  typename [:^^S::type:] *var;
}

using alias = [:^^S::type:];
}  // namespace dcl_type_splice

                                 // ==========
                                 // temp_names
                                 // ==========

namespace temp_names {
struct X {
  template<unsigned> static X* adjust();
};
template<class T> void f(T* p) {
  static constexpr auto r = ^^T::adjust;
  T* p3 = [:r:]<200>();
    // expected-error@-1 {{expected expression}}
  T* p4 = template [:r:]<200>();
}
}  // namespace temp_names

                              // ================
                              // temp_res_general
                              // ================

namespace temp_res_general {
enum class Enum { A, B, C };

template<class T> struct S {
  using Alias = [:^^int:];
  auto h() -> [:^^S:]<T*>;
  using enum [:^^Enum:];
};
}  // namespace temp_res_general

                               // ===============
                               // temp_dep_splice
                               // ===============

namespace temp_dep_splice {
template <auto T, auto NS>
void fn() {
  using a = [:T:]<1>;

  static_assert([:NS:]::template TCls<1>::v == a::v);
}

namespace NS {
template <auto V> struct TCls { static constexpr int v = V; };
}

int fn() {
  fn<^^NS::TCls, ^^NS>();
}
}  // namespace temp_dep_splice

                             // ==================
                             // temp_dep_namespace
                             // ==================

namespace temp_dep_namespace {
template <info R> int fn() {
  namespace Alias = [:R:];  // [:R:] is dependent
  return Alias::v;  // Alias is dependent
}

namespace NS {
  int v = 1;
}

int a = fn<^^NS>();
}  // namespace temp_dep_namespace
