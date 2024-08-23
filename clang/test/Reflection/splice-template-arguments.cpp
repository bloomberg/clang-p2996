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

template <typename T, unsigned Sz>
struct ArrayCls { T elems[Sz]; };

template <typename T, unsigned Sz>
using Array = T[Sz];

template <template <typename T, unsigned Sz> class C, typename T, unsigned Sz>
struct TCls { static constexpr unsigned value = sizeof(C<T, Sz>); };

template <template <typename T, unsigned Sz> class C, typename T, unsigned Sz>
using TAlias = TCls<C, T, Sz>;

template <template <typename T, unsigned Sz> class C, typename T, unsigned Sz>
consteval unsigned TFn() { return sizeof(C<T, Sz>); };

template <template <typename T, unsigned Sz> class C, typename T, unsigned Sz>
constexpr unsigned TVar = sizeof(C<T, Sz>);

template <template <typename T, unsigned Sz> class C, typename T, unsigned Sz>
concept Concept = requires { requires sizeof(C<T, Sz>) == sizeof(T) * Sz; };

                           // =======================
                           // non_dependent_arguments
                           // =======================

namespace non_dependent_arguments {
constexpr unsigned x = 4;
constexpr info R1a = ^^ArrayCls, R1b = ^^Array, R2 = ^^int, R3 = ^^x;
static_assert(TCls<[:R1a:], [:R2:], [:R3:]>::value == sizeof(int) * 4);
static_assert(TCls<template [:R1a:], typename [:R2:], ([:R3:])>::value ==
              sizeof(int) * 4);
static_assert(TCls<[:R1b:], [:R2:], [:R3:]>::value == sizeof(int) * 4);
static_assert(TAlias<[:R1a:], [:R2:], [:R3:]>::value == sizeof(int) * 4);
static_assert(TAlias<[:R1b:], [:R2:], [:R3:]>::value == sizeof(int) * 4);
static_assert(TAlias<template [:R1b:], typename [:R2:], ([:R3:])>::value ==
              sizeof(int) * 4);
static_assert(TFn<[:R1a:], [:R2:], [:R3:]>() == sizeof(int) * 4);
static_assert(TFn<[:R1b:], [:R2:], [:R3:]>() == sizeof(int) * 4);
static_assert(TFn<template [:R1b:], typename [:R2:], ([:R3:])>() ==
              sizeof(int) * 4);
static_assert(TVar<[:R1a:], [:R2:], [:R3:]> == sizeof(int) * 4);
static_assert(TVar<[:R1b:], [:R2:], [:R3:]> == sizeof(int) * 4);
static_assert(TVar<template [:R1b:], typename [:R2:], ([:R3:])> ==
              sizeof(int) * 4);
static_assert(Concept<[:R1a:], [:R2:], [:R3:]>);
static_assert(Concept<[:R1b:], [:R2:], [:R3:]>);
static_assert(Concept<template [:R1b:], typename [:R2:], ([:R3:])>);
}  // namespace non_dependent_arguments

                             // ===================
                             // dependent_arguments
                             // ===================

namespace dependent_arguments {
template <info TN, info T, info Sz>
consteval unsigned szTCls() { return TCls<[:TN:], [:T:], [:Sz:]>::value; }

template <info TN, info T, info Sz>
consteval unsigned constrainedSzTCls() {
  return TCls<template [:TN:], typename [:T:], ([:Sz:])>::value;
}

template <info TN, info T, info Sz>
consteval unsigned szTAlias() { return TAlias<[:TN:], [:T:], [:Sz:]>::value; }

template <info TN, info T, info Sz>
consteval unsigned szTFn() { return TFn<[:TN:], [:T:], [:Sz:]>(); }

template <info TN, info T, info Sz>
constexpr unsigned szTVar = TVar<[:TN:], [:T:], [:Sz:]>;

template <info TN, info T, info Sz>
concept DependentConcept = Concept<[:TN:], [:T:], [:Sz:]>;

constexpr unsigned x = 4;
constexpr info R1a = ^^ArrayCls, R1b = ^^Array, R2 = ^^int, R3 = ^^x;
static_assert(szTCls<R1a, R2, R3>() == sizeof(int) * 4);
static_assert(szTCls<R1b, R2, R3>() == sizeof(int) * 4);
static_assert(constrainedSzTCls<R1a, R2, R3>() == sizeof(int) * 4);
static_assert(szTAlias<R1a, R2, R3>() == sizeof(int) * 4);
static_assert(szTAlias<R1a, R2, R3>() == sizeof(int) * 4);
static_assert(szTFn<R1a, R2, R3>() == sizeof(int) * 4);
static_assert(szTFn<R1b, R2, R3>() == sizeof(int) * 4);
static_assert(szTVar<R1b, R2, R3> == sizeof(int) * 4);
static_assert(szTVar<R1b, R2, R3> == sizeof(int) * 4);
static_assert(DependentConcept<R1a, R2, R3>);
static_assert(DependentConcept<R1b, R2, R3>);
}  // namespace dependent_arguments

template <typename T, unsigned Sz>
struct NCopies { T mem[Sz]; };

                            // ====================
                            // type_parameter_packs
                            // ====================

namespace type_parameter_packs {
template <typename... Ts>
struct TotalSzCls { static constexpr unsigned sz = (... + sizeof(Ts)); };

template <typename... Ts>
using TotalSzAlias = TotalSzCls<Ts...>;

template <typename... Ts>
consteval unsigned TotalSzFn() { return (... + sizeof(Ts)); }

template <typename... Ts>
constexpr unsigned TotalSzVar = (... + sizeof(Ts));

template <typename... Ts>
concept HasThreeTypes = requires { requires sizeof...(Ts) == 3; };

template <template <typename...> class TN, info... Rs>
consteval unsigned depTotalSzCls() { return TN<[:Rs:]...>::sz; }

template <template <typename...> class TN, info... Rs>
consteval unsigned constrainedDepTotalSzCls() {
  return TN<typename [:Rs:]...>::sz;
}

template <info... Rs>
consteval unsigned depTotalSzFn() { return TotalSzFn<[:Rs:]...>(); }

template <info... Rs>
constexpr unsigned depTotalSzVar = TotalSzVar<[:Rs:]...>;

template <info... Rs>
concept depHasThreeTypes = HasThreeTypes<[:Rs:]...>;

constexpr info R1 = ^^int, R2 = ^^bool, R3 = ^^char;
constexpr unsigned Expected = sizeof(int) + sizeof(bool) + sizeof(char);
static_assert(TotalSzCls<[:R1:], [:R2:], [:R3:]>::sz == Expected);
static_assert(TotalSzAlias<[:R1:], [:R2:], [:R3:]>::sz == Expected);
static_assert(TotalSzFn<[:R1:], [:R2:], [:R3:]>() == Expected);
static_assert(TotalSzVar<[:R1:], [:R2:], [:R3:]> == Expected);
static_assert(HasThreeTypes<[:R1:], [:R2:], [:R3:]>);
static_assert(depTotalSzCls<TotalSzCls, R1, R2, R3>() == Expected);
static_assert(depTotalSzCls<TotalSzAlias, R1, R2, R3>() == Expected);
static_assert(constrainedDepTotalSzCls<TotalSzCls, R1, R2, R3>() == Expected);
static_assert(depTotalSzFn<R1, R2, R3>() == Expected);
static_assert(depTotalSzVar<R1, R2, R3> == Expected);
static_assert(depHasThreeTypes<R1, R2, R3>);
}  // namespace type_parameter_packs

                          // ========================
                          // non_type_parameter_packs
                          // ========================

namespace non_type_parameter_packs {
template <unsigned... Vs>
struct SumCls { static constexpr unsigned sz = (... + Vs); };

template <unsigned... Vs>
using SumAlias = SumCls<Vs...>;

template <unsigned... Vs>
consteval unsigned SumFn() { return (... + Vs); }

template <unsigned... Vs>
constexpr unsigned SumVar = (... + Vs);

template <unsigned... Vs>
concept SumMoreThan5 = requires { requires (... + Vs) > 5; };

template <template <unsigned...> class TN, info... Rs>
consteval unsigned depSumCls() { return TN<[:Rs:]...>::sz; }

template <template <unsigned...> class TN, info... Rs>
consteval unsigned constrainedDepSumCls() { return TN<([:Rs:])...>::sz; }

template <info... Rs>
consteval unsigned depSumFn() { return SumFn<[:Rs:]...>(); }

template <info... Rs>
constexpr unsigned depSumVar = SumVar<[:Rs:]...>;

template <info... Rs>
concept depSumMoreThan5 = SumMoreThan5<[:Rs:]...>;

constexpr unsigned x = 1, y = 2, z = 3;
constexpr info R1 = ^^x, R2 = ^^y, R3 = ^^z;
constexpr unsigned Expected = [:R1:] + [:R2:] + [:R3:];
static_assert(SumCls<[:R1:], [:R2:], [:R3:]>::sz == Expected);
static_assert(SumAlias<[:R1:], [:R2:], [:R3:]>::sz == Expected);
static_assert(SumFn<[:R1:], [:R2:], [:R3:]>() == Expected);
static_assert(SumVar<[:R1:], [:R2:], [:R3:]> == Expected);
static_assert(SumMoreThan5<[:R1:], [:R2:], [:R3:]>);
static_assert(depSumCls<SumCls, R1, R2, R3>() == Expected);
static_assert(depSumCls<SumAlias, R1, R2, R3>() == Expected);
static_assert(constrainedDepSumCls<SumCls, R1, R2, R3>() == Expected);
static_assert(depSumFn<R1, R2, R3>() == Expected);
static_assert(depSumVar<R1, R2, R3> == Expected);
static_assert(depSumMoreThan5<R1, R2, R3>);
}  // namespace non_type_parameter_packs

                          // ========================
                          // template_parameter_packs
                          // ========================

namespace template_parameter_packs {
template <typename> struct S1 {};
template <typename> struct S2 {};
template <typename> struct S3 {};

template <template <typename> class... TNs>
struct CountCls { static constexpr unsigned sz = sizeof...(TNs); };

template <template <typename> class... TNs>
using CountAlias = CountCls<TNs...>;

template <template <typename> class... TNs>
consteval unsigned CountFn() { return sizeof...(TNs); }

template <template <typename> class... TNs>
constexpr unsigned CountVar = sizeof...(TNs);

template <template <typename> class... TNs>
concept CountIs3 = requires { requires sizeof...(TNs) == 3; };

template <template <template <typename> class...> class TN, info... Rs>
consteval unsigned depCountCls() { return TN<[:Rs:]...>::sz; }

template <template <template <typename> class...> class TN, info... Rs>
consteval unsigned constrainedDepCountCls() {
  return TN<template [:Rs:]...>::sz;
}

template <info... Rs>
consteval unsigned depCountFn() { return CountFn<[:Rs:]...>(); }

template <info... Rs>
constexpr unsigned depCountVar = CountVar<[:Rs:]...>;

template <info... Rs>
concept depCountIs3 = CountIs3<[:Rs:]...>;

constexpr info R1 = ^^S1, R2 = ^^S2, R3 = ^^S3;
constexpr unsigned Expected = 3;
static_assert(CountCls<[:R1:], [:R2:], [:R3:]>::sz == Expected);
static_assert(CountAlias<[:R1:], [:R2:], [:R3:]>::sz == Expected);
static_assert(CountFn<[:R1:], [:R2:], [:R3:]>() == Expected);
static_assert(CountVar<[:R1:], [:R2:], [:R3:]> == Expected);
static_assert(CountIs3<[:R1:], [:R2:], [:R3:]>);
static_assert(depCountCls<CountCls, R1, R2, R3>() == Expected);
static_assert(depCountCls<CountAlias, R1, R2, R3>() == Expected);
static_assert(constrainedDepCountCls<CountCls, R1, R2, R3>() == Expected);
static_assert(depCountFn<R1, R2, R3>() == Expected);
static_assert(depCountVar<R1, R2, R3> == Expected);
static_assert(depCountIs3<R1, R2, R3>);

}  // namespace template_parameter_packs

                     // ==================================
                     // heterogeneous_packs_of_reflections
                     // ==================================

namespace heterogeneous_packs_of_reflections {
template <info... Rs>
consteval unsigned szTCls() { return TCls<[:Rs:]...>::value; }

template <info... Rs>
consteval unsigned szTFn() { return TFn<[:Rs:]...>(); }

template <info... Rs>
constexpr unsigned szTVar = TVar<[:Rs:]...>;

constexpr int x = 4;
constexpr info R1a = ^^ArrayCls, R1b = ^^Array, R2 = ^^int, R3 = ^^x;
static_assert(szTCls<R1a, R2, R3>() == sizeof(int) * 4);
static_assert(szTCls<R1b, R2, R3>() == sizeof(int) * 4);
static_assert(szTFn<R1a, R2, R3>() == sizeof(int) * 4);
static_assert(szTFn<R1b, R2, R3>() == sizeof(int) * 4);
static_assert(szTVar<R1b, R2, R3> == sizeof(int) * 4);
static_assert(szTVar<R1b, R2, R3> == sizeof(int) * 4);
}  // namespace heterogeneous_packs_of_reflections
