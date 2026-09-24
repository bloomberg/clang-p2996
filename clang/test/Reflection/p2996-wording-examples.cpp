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

                  // ========================================
                  // bb_clang_p2996_issue_342_regression_test
                  // ========================================

namespace bb_clang_p2996_issue_342_regression_test {
struct Base {
  int operator()(int x) const { return x; }
  operator int() const { return 0; }
  template <typename T> T tfn(T v) const { return v; }
  int fn() const { return 1; }
  static void *operator new(decltype(sizeof(0)) n);
};

struct Derived : Base {
  using Base::operator();
  using Base::operator int;
  using Base::tfn;
  using Base::fn;
  using Base::operator new;
};

// The operand is an id-expression, so it names the function that the
// using-declaration introduced ([expr.reflect]/7).
static_assert(^^Derived::operator() == ^^Base::operator());
static_assert(^^Derived::operator int == ^^Base::operator int);
static_assert(^^Derived::tfn<int> == ^^Base::tfn<int>);
static_assert(^^Derived::operator new == ^^Base::operator new);
static_assert(&[:^^Derived::operator():] == &Base::operator());

// The operand is a reflection-name, so lookup finding a declaration that
// replaced a using-declarator is ill-formed ([expr.reflect]/5.1).
constexpr info r = ^^Derived::fn;
  // expected-error@-1 {{cannot take the reflection of a using-declarator}}

// The same distinction applies to dependent operands.
template <typename T>
struct S : T {
  using T::operator();
  using T::tfn;
  using T::operator new;
  static constexpr info a = ^^S::operator();
  static constexpr info b = ^^T::operator();
  static constexpr info c = ^^S::template tfn<int>;
  static constexpr info d = ^^operator new;
};
static_assert(S<Base>::a == ^^Base::operator());
static_assert(S<Base>::b == ^^Base::operator());
static_assert(S<Base>::c == ^^Base::tfn<int>);
static_assert(S<Base>::d == ^^Base::operator new);

template <typename T>
struct U : T {
  using T::fn;
  static constexpr info a = ^^U::fn;
    // expected-error@-1 {{cannot take the reflection of a using-declarator}}
  static constexpr info b = ^^fn;
    // expected-error@-1 {{cannot take the reflection of a using-declarator}}
};
constexpr info ua = U<Base>::a;
  // expected-note@-1 {{in instantiation of static data member}}
constexpr info ub = U<Base>::b;
  // expected-note@-1 {{in instantiation of static data member}}

// An id-expression that names an overload set must be the operand of a
// well-formed '&' ([expr.reflect]/7.2). Without a nested-name-specifier, '&'
// cannot form a pointer to a non-static member function.
struct Self {
  int operator()(int) const;
  template <typename T> T tfn(T v) const { return v; }
  static constexpr info a = ^^operator();
    // expected-error@-1 {{cannot take the reflection of non-static member function 'operator()' named by an unqualified name}}
  static constexpr info b = ^^tfn<int>;
    // expected-error@-1 {{cannot take the reflection of non-static member function 'tfn<int>' named by an unqualified name}}
};

struct DerivedSelf : Base {
  using Base::operator();
  static constexpr info a = ^^operator();
    // expected-error@-1 {{cannot take the reflection of non-static member function 'operator()' named by an unqualified name}}
};

template <typename T>
struct V : T {
  using T::operator();
  static constexpr info a = ^^operator();
    // expected-error@-1 {{cannot take the reflection of non-static member function 'operator()' named by an unqualified name}}
};
constexpr info va = V<Base>::a;
  // expected-note@-1 {{in instantiation of static data member}}

// Here '&' cannot select a specialization of the function template because T
// has neither a deduction source nor a default. A reflection-name naming the
// template is not an id-expression, so it still represents the template
// ([expr.reflect]/5.5.2).
struct TemplateBase {
  template <typename T> int operator()(T) const;
  template <typename T> int tfn(T) const;
};

struct TemplateDerived : TemplateBase {
  using TemplateBase::operator();
};

constexpr info tb = ^^TemplateBase::operator();
  // expected-error@-1 {{cannot take the reflection of an overload set}}
constexpr info td = ^^TemplateDerived::operator();
  // expected-error@-1 {{cannot take the reflection of an overload set}}
constexpr info tfn = ^^TemplateBase::tfn;
static_assert(^^TemplateBase::operator()<int> ==
              ^^TemplateDerived::operator()<int>);

template <typename T>
struct W : T {
  using T::operator();
  static constexpr info a = ^^T::operator();
    // expected-error@-1 {{cannot take the reflection of an overload set}}
  static constexpr info b = ^^operator();
    // expected-error@-1 {{cannot take the reflection of an overload set}}
};
constexpr info wa = W<TemplateBase>::a;
  // expected-note@-1 {{in instantiation of static data member}}
constexpr info wb = W<TemplateBase>::b;
  // expected-note@-1 {{in instantiation of static data member}}

// A reflection-name that finds neither a template nor a type is interpreted as
// an id-expression ([expr.reflect]/5.9), so the same holds for an identifier.
// A non-static data member is not an overload set ([expr.reflect]/7.3).
struct SelfIdentifier {
  int k;
  int fn() const;
  static int sfn();
  static constexpr info a = ^^fn;
    // expected-error@-1 {{cannot take the reflection of non-static member function 'fn' named by an unqualified name}}
  static constexpr info b = ^^sfn;
  static constexpr info c = ^^k;
  void mem() const {
    constexpr info d = ^^fn;
      // expected-error@-1 {{cannot take the reflection of non-static member function 'fn' named by an unqualified name}}
    constexpr info e = ^^SelfIdentifier::fn;
  }
};
}  // namespace bb_clang_p2996_issue_342_regression_test

                  // ========================================
                  // bb_clang_p2996_issue_353_regression_test
                  // ========================================

namespace bb_clang_p2996_issue_353_regression_test {
namespace template_identifier_direct {
struct Base {
  template <class> struct X;
};
struct Derived : Base {
  using Base::X;
};
constexpr info r = ^^Derived::template X;
  // expected-error@-1 {{cannot take the reflection of a using-declarator}}
} // namespace template_identifier_direct

namespace template_identifier_dependent {
struct Base {
  template <class> struct X;
};
struct Derived : Base {
  using Base::X;
};
template <class T> struct Probe {
  static constexpr info value = ^^T::template X;
    // expected-error@-1 {{cannot take the reflection of a using-declarator}}
};
constexpr info r = Probe<Derived>::value;
  // expected-note@-1 {{in instantiation of static data member}}
} // namespace template_identifier_dependent

namespace using_pack {
struct Base {
  static void f();
  static void *operator new(decltype(sizeof(0)));
};
template <class... Bases> struct Derived : Bases... {
  using Bases::f...;
  using Bases::operator new...;
  static constexpr info name = ^^f;
    // expected-error@-1 {{cannot take the reflection of a using-declarator}}
  static constexpr info id = ^^operator new;
};
constexpr info r = Derived<Base>::name;
  // expected-note@-1 {{in instantiation of static data member}}
static_assert(Derived<Base>::id == ^^Base::operator new);
} // namespace using_pack

namespace defaulted_template_argument {
struct Base {
  template <class T = int> int operator()() const;
};
struct Derived : Base {
  using Base::operator();
};
static_assert(^^Base::operator() == ^^Base::operator()<int>);
static_assert(^^Derived::operator() == ^^Base::operator()<int>);
} // namespace defaulted_template_argument

namespace empty_template_parameter_pack {
struct S {
  template <class... Ts> int operator()() const;
};
static_assert(^^S::operator() == ^^S::operator()<>);
} // namespace empty_template_parameter_pack

namespace mixed_overload_set {
struct S {
  int operator()() const;
  template <class T> int operator()(T) const;
};
static_assert(&[:^^S::operator():] ==
              static_cast<int (S::*)() const>(&S::operator()));
} // namespace mixed_overload_set

namespace constrained_template_set {
struct S {
  template <class T = int>
    requires (sizeof(T) != 0)
  int operator()(int) const;
  template <class T = int>
    requires (sizeof(T) == 0)
  int operator()(long) const;
};
static_assert(&[:^^S::operator():] ==
              static_cast<int (S::*)(int) const>(&S::operator()<int>));
} // namespace constrained_template_set

namespace partially_ordered_template_set {
struct S {
  template <class T = int> int operator()(T) const;
  template <class T = int> int operator()(T *) const;
};
static_assert(&[:^^S::operator():] ==
              static_cast<int (S::*)(int *) const>(&S::operator()<int>));
} // namespace partially_ordered_template_set

namespace deleted_function {
struct S {
  int operator()() const = delete;
  template <class T> int operator()(T) const;
};
constexpr info r = ^^S::operator();
} // namespace deleted_function

namespace ambiguous_non_template_set {
struct S {
  int operator()(int) const;
  int operator()(double) const;
};
constexpr info r = ^^S::operator();
  // expected-error@-1 {{cannot take the reflection of an overload set}}
} // namespace ambiguous_non_template_set

namespace unqualified_defaulted_template {
struct S {
  template <class T = int> int operator()() const;
  static constexpr info r = ^^operator();
    // expected-error@-1 {{cannot take the reflection of non-static member function 'operator()<int>' named by an unqualified name}}
};
} // namespace unqualified_defaulted_template

namespace destructor_address {
struct S {
  ~S();
};
constexpr info r = ^^S::~S;
  // expected-error@-1 {{taking the address of a destructor}}
} // namespace destructor_address
} // namespace bb_clang_p2996_issue_353_regression_test
