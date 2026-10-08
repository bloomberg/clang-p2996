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
// ADDITIONAL_COMPILE_FLAGS: -freflection
// ADDITIONAL_COMPILE_FLAGS: -fannotation-attributes
// ADDITIONAL_COMPILE_FLAGS: -fparameter-reflection
// ADDITIONAL_COMPILE_FLAGS: -Wno-unused-parameter

// <experimental/reflection>
//
// [reflection]

#include <meta>

using std::meta::annotations_of;
using std::meta::extract;

                           // =====================
                           // declaration_order
                           // =====================

// [meta.reflection.annotation]/2: an annotation that precedes another appears
// before it; the annotations of every declaration of an entity are collected
// ([meta.reflection.annotation]/4, Example 1).
namespace declaration_order {
[[=1]] void f();
[[=2, =3]] void g();
void g [[=4]] ();
void g [[=5]] ();

static_assert(annotations_of(^^f).size() == 1);
static_assert(annotations_of(^^g).size() == 4);
static_assert([: constant_of(annotations_of(^^g)[0]) :] == 2);
static_assert(extract<int>(annotations_of(^^g)[1]) == 3);
static_assert(extract<int>(annotations_of(^^g)[2]) == 4);
static_assert(extract<int>(annotations_of(^^g)[3]) == 5);

[[=1]] int v;
extern int v [[=2]];
static_assert(extract<int>(annotations_of(^^v)[0]) == 1);
static_assert(extract<int>(annotations_of(^^v)[1]) == 2);

struct [[=1]] S;
struct [[=2]] S {};
static_assert(extract<int>(annotations_of(^^S)[0]) == 1);
static_assert(extract<int>(annotations_of(^^S)[1]) == 2);

namespace [[=1]] NS {}
namespace [[=2]] NS {}
static_assert(extract<int>(annotations_of(^^NS)[0]) == 1);
static_assert(extract<int>(annotations_of(^^NS)[1]) == 2);

// [dcl.attr.annotation]/3: each annotation produces a unique annotation, even
// with equal constants, and one attribute-specifier applies to each declarator.
[[=2, =3, =2]] void h();
void h [[=4, =2]] ();
static_assert(annotations_of(^^h).size() == 5);
static_assert(annotations_of(^^h)[0] != annotations_of(^^h)[2]);
[[=7]] int x, y;
static_assert(annotations_of(^^x).size() == 1 && annotations_of(^^y).size() == 1);
static_assert(annotations_of(^^x)[0] != annotations_of(^^y)[0]);
}  // namespace declaration_order

                         // ===========================
                         // function_template_specializations
                         // ===========================

// [meta.reflection.annotation]/1: for a specialization F of a function
// template, S(F) holds the declarations of the template as well, in order.
namespace function_template_specializations {
template <class T> [[=1]] void th(T);
template <class T> void th [[=2]] (T) {}
template <class T> void th(T);
static_assert(annotations_of(^^th<int>).size() == 2);
static_assert(extract<int>(annotations_of(^^th<int>)[0]) == 1);
static_assert(extract<int>(annotations_of(^^th<int>)[1]) == 2);
// Each instantiation has its own annotations ([dcl.attr.annotation]/3).
static_assert(annotations_of(^^th<int>) != annotations_of(^^th<long>));

template <class T> [[=1]] void only_declared(T);
static_assert(annotations_of(^^only_declared<int>).size() == 1);

template <class T> void explicit_spec(T);
template <> [[=3]] void explicit_spec<int>(int);
template <> void explicit_spec<int> [[=4]] (int) {}
static_assert(annotations_of(^^explicit_spec<int>).size() == 2);
static_assert(extract<int>(annotations_of(^^explicit_spec<int>)[0]) == 3);
static_assert(extract<int>(annotations_of(^^explicit_spec<int>)[1]) == 4);
static_assert(annotations_of(^^explicit_spec<long>).empty());

struct C {
  template <class T> [[=5]] void mf(T);
};
template <class T> void C::mf(T) {}
static_assert(annotations_of(^^C::mf<int>).size() == 1);
}  // namespace function_template_specializations

                              // ==================
                              // function_parameters
                              // ==================

// [meta.reflection.annotation]/2.1: for a parameter, the annotations of its
// declaration in each declaration of the function; [dcl.attr.annotation]/1
// Note 1: an annotation on a parameter of a definition applies to both the
// parameter and the variable.
namespace function_parameters {
void fp([[=5]] int p, [[=6]] int q);
void fp([[=7]] int p, int q) {
  constexpr auto rp = std::meta::parameters_of(^^fp)[0];
  constexpr auto rv = std::meta::variable_of(rp);
  static_assert(annotations_of(rp).size() == 2);
  static_assert(annotations_of(rv).size() == 1);
  static_assert(annotations_of(rp)[1] == annotations_of(rv)[0]);
}
static_assert(annotations_of(std::meta::parameters_of(^^fp)[0]).size() == 2);
static_assert(extract<int>(annotations_of(std::meta::parameters_of(^^fp)[0])[0]) == 5);
static_assert(extract<int>(annotations_of(std::meta::parameters_of(^^fp)[0])[1]) == 7);
static_assert(annotations_of(std::meta::parameters_of(^^fp)[1]).size() == 1);
static_assert(extract<int>(annotations_of(std::meta::parameters_of(^^fp)[1])[0]) == 6);

template <class T> void tp([[=8]] T t);
template <class T> void tp([[=9]] T t) {}
static_assert(annotations_of(std::meta::parameters_of(^^tp<int>)[0]).size() == 2);
static_assert(extract<int>(annotations_of(std::meta::parameters_of(^^tp<int>)[0])[0]) == 8);
static_assert(extract<int>(annotations_of(std::meta::parameters_of(^^tp<int>)[0])[1]) == 9);

void np(int a, int b);
static_assert(annotations_of(std::meta::parameters_of(^^np)[0]).empty());
}  // namespace function_parameters

int main() { }
