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
// RUN: %clang_cc1 %s -std=c++23 -freflection -freflection-new-syntax

using info = decltype(^^int);


template <typename T1, typename T2>
constexpr bool is_same_v = false;

template <typename T1>
constexpr bool is_same_v<T1, T1> = true;

                                 // ===========
                                 // idempotency
                                 // ===========

// Check idempotency of the splice operator composed with reflection.
namespace idempotency {
template <typename T> struct TCls {};
template <typename T> using TAlias = TCls<T>;
template <typename T> void TFn() {}
template <typename T> static constexpr int TVar = 0;
template <typename T> concept Concept = requires { requires true; };
struct S {
  template <typename T> struct TInner {};
  template <typename T> void TFn();
  template <typename T> static constexpr int TMemVar = 0;
};

static_assert(is_same_v<typename [:^^TCls:]<int>, TCls<int>>);
static_assert(is_same_v<typename [:^^TAlias:]<int>, TCls<int>>);
static_assert(&template [:^^TFn:]<int> == &TFn<int>);
static_assert(&template [:^^TVar:]<int> == &TVar<int>);
static_assert(is_same_v<typename [:^^S::TInner:]<int>, S::TInner<int>>);
static_assert(&template [:^^S::TFn:]<int> == &S::TFn<int>);
static_assert(&template [:^^S::TMemVar:]<int> == &S::TMemVar<int>);
}  // namespace idempotency

                                // =============
                                // non_dependent
                                // =============

namespace non_dependent {
template <typename T> struct TCls {
  T value;
  static constexpr T zero = 0;
  using type = T;
  template <typename U> constexpr U tmemfn(U v) const { return v; }
  template <typename U> static constexpr int tsmem = 13;
};
template <typename T> TCls(T) -> TCls<T>;

template <typename T> using TAlias = TCls<T>;
template <typename T> consteval int TFn(T value) { return value; }
template <int Value> constexpr int TVar = Value;
template <int Value> concept IsOdd = requires { requires Value % 2 == 1; };

constexpr [:^^TCls:]<int> obj1 = {1};
static_assert(obj1.value == 1);
constexpr [:^^TAlias:]<int> obj2 = {2};
static_assert(obj2.value == 2);

constexpr [:^^TCls:] obj3 = {3};
static_assert(obj3.value == 3);

constexpr [:^^TAlias:] obj4 = {4};
static_assert(obj4.value == 4);

static_assert([:^^TCls:]<int>::zero == 0);
static_assert(is_same_v<[:^^TCls:]<int>::type, int>);
static_assert([:^^TAlias:]<int>::zero == 0);
static_assert(is_same_v<[:^^TAlias:]<int>::type, int>);

static_assert(template [:^^TFn:]<char>('a') == 'a');
static_assert(template [:^^TFn:]<>('b') == 'b');
static_assert(template [:^^TFn:]('c') == 'c');
static_assert(template [:^^TVar:]<41> == 41);

static_assert(template [:^^IsOdd:]<13>);
static_assert(!template [:^^IsOdd:]<10>);

static_assert(obj1.template [:^^TCls<int>::tmemfn:]<int>(4) == 4);
static_assert(obj1.template [:^^TCls<int>::tmemfn:]<>(3) == 3);
static_assert(obj1.template [:^^TCls<int>::tmemfn:](2) == 2);
static_assert(((&obj1)->*(&template [:^^TCls<int>::tmemfn:]<int>))(1) == 1);

static_assert(template [:^^TCls<int>::tsmem:]<int> == 13);
static_assert(obj1.template [:^^TCls<int>::tsmem:]<int> == 13);
static_assert((&obj1)->template [:^^TCls<int>::tsmem:]<int> == 13);

// TODO(P2996): Test splicing concepts as type constraints.
}  // namespace non_dependent

namespace dependent {
template <info R> consteval auto DepTClsFn(int value) {
  typename [:R:]<int> obj = {value};
  return obj;
}
template <info R> consteval auto DepTClsStaticMember() {
  typename [:R:]<int>::type result = [:R:]<int>::zero;
  return result;
}
template <info R> consteval auto DepTClsCTAD(int value) {
  typename [:R:] obj = {value};
  return obj;
}
template <info R> consteval int DepTFnFn(int value) {
  return template [:R:]<int>(value) + template [:R:]<>(value) +
         template [:R:](value);
}
template <info R, int Value> consteval int DepTVarFn() {
  return template [:R:]<Value>;
}
template <info R> consteval bool DepConceptFn() {
  return template [:R:]<13>;
}
template <info R>
consteval int DepTMemFn(const non_dependent::TCls<int> &self, int v) {
  return self.template [:R:]<int>(v) + self.template [:R:]<>(v) +
         self.template [:R:](v);
}
template <info R>
consteval info TMemFnWithDepScope() { return ^^[:R:]::template tmemfn; }

static_assert(DepTClsFn<^^non_dependent::TCls>(11).value == 11);
static_assert(DepTClsFn<^^non_dependent::TAlias>(12).value == 12);
static_assert(DepTClsCTAD<^^non_dependent::TCls>(13).value == 13);
static_assert(DepTClsCTAD<^^non_dependent::TAlias>(15).value == 15);
static_assert(DepTClsStaticMember<^^non_dependent::TCls>() == 0);
static_assert(DepTClsStaticMember<^^non_dependent::TAlias>() == 0);
static_assert(DepTFnFn<^^non_dependent::TFn>(3) == 9);
static_assert(DepTVarFn<^^non_dependent::TVar, 15>() == 15);
static_assert(DepConceptFn<^^non_dependent::IsOdd>());

constexpr non_dependent::TCls<int> obj1 = {1};
static_assert(DepTMemFn<^^non_dependent::TCls<int>::tmemfn>(obj1, 4) == 12);

static_assert(obj1.template [:TMemFnWithDepScope<^^decltype(obj1)>():](3) == 3);

// TODO(P2996): Test splicing concepts as type constraints.
}  // namespace dependent

                        // ============================
                        // less_than_operator_ambiguity
                        // ============================

namespace less_than_operator_ambiguity {
constexpr int x = 3;
static_assert([:^^x:] < 5);  // make sure this parses.
}  // namespace less_than_operator_ambiguity
