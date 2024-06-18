//===----------------------------------------------------------------------===//
//
// Copyright 2024 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20
// ADDITIONAL_COMPILE_FLAGS: -freflection
// ADDITIONAL_COMPILE_FLAGS: -Wno-unneeded-internal-declaration

// <experimental/reflection>
//
// [reflection]
//
// RUN: %{exec} %t.exe > %t.stdout

#include <experimental/meta>

#include <print>
#include <utility>


enum Enum { A = 42 };
enum class EnumCls { A = 42 };
const int ConstVar = 42;
constexpr int ConstexprVar = 42;
void fn() {}
struct Cls {
  int k;
  void fn();
};
template <int K> struct TCls {
  static constexpr int value = K;
};

                            // =====================
                            // reflect_value_results
                            // =====================

namespace reflect_value_results {
static_assert([:std::meta::reflect_value(42):] == 42);
static_assert(type_of(std::meta::reflect_value(42)) == ^int);

static_assert([:std::meta::reflect_value(EnumCls::A):] == EnumCls::A);
static_assert(type_of(std::meta::reflect_value(EnumCls::A)) == ^EnumCls);

static_assert([:std::meta::reflect_value(ConstVar):] == ConstVar);
static_assert(type_of(std::meta::reflect_value(ConstVar)) == ^int);

static_assert([:std::meta::reflect_value(ConstexprVar):] == ConstexprVar);
static_assert(type_of(std::meta::reflect_value(ConstexprVar)) == ^int);

static_assert([:std::meta::reflect_value(fn):] == &fn);
static_assert(type_of(std::meta::reflect_value(fn)) == ^void(*)());

static_assert([:std::meta::reflect_value(&Cls::k):] == &Cls::k);
static_assert(type_of(std::meta::reflect_value(&Cls::k)) == ^int (Cls::*));

static_assert([:std::meta::reflect_value(&Cls::fn):] == &Cls::fn);
static_assert(type_of(std::meta::reflect_value(&Cls::fn)) == ^void (Cls::*)());
}  // namespace reflect_value_results

                           // ======================
                           // reflect_object_results
                           // ======================

namespace reflect_object_results {
int NonConstVar;
static_assert(std::meta::reflect_object(NonConstVar) == ^NonConstVar);
static_assert(type_of(std::meta::reflect_object(NonConstVar)) == ^int);

static_assert(std::meta::reflect_object(ConstVar) == ^ConstVar);
static_assert(type_of(std::meta::reflect_object(ConstVar)) == ^const int);

static_assert(std::meta::reflect_object(ConstexprVar) == ^ConstexprVar);
static_assert(type_of(std::meta::reflect_object(ConstexprVar)) == ^const int);

static constexpr std::pair<int, short> p = {1, 2};
static_assert(std::meta::reflect_object(p) == ^p);
static_assert(std::meta::reflect_object(p.first) != ^p);
static_assert(type_of(std::meta::reflect_object(p)) ==
              ^const std::pair<int, short>);

static_assert(&[:std::meta::reflect_object(p.first):] == &p.first);
static_assert([:std::meta::reflect_object(p.first):] == 1);
static_assert(type_of(std::meta::reflect_object(p.first)) == ^const int);

static_assert(&[:std::meta::reflect_object(p.second):] == &p.second);
static_assert([:std::meta::reflect_object(p.second):] == 2);
static_assert(type_of(std::meta::reflect_object(p.second)) == ^const short);

const int &Ref = NonConstVar;
static_assert(std::meta::reflect_object(Ref) == ^NonConstVar);
static_assert(type_of(std::meta::reflect_object(Ref)) == ^int);
}  // namespace reflect_object_results

                          // ========================
                          // reflect_function_results
                          // ========================

namespace reflect_function_results {
static_assert(std::meta::reflect_function(::fn) == ^::fn);
static_assert(type_of(std::meta::reflect_function(::fn)) == ^void());

const auto &Ref = ::fn;
static_assert(std::meta::reflect_function(Ref) == ^::fn);
static_assert(type_of(std::meta::reflect_function(Ref)) == ^void());
}

                               // ===============
                               // extract_results
                               // ===============

namespace extract_results {
template <auto Expected>
consteval bool CheckValueIs(std::meta::info R) {
  return extract<decltype(Expected)>(R) == Expected;
}
static_assert(CheckValueIs<42>(std::meta::reflect_value(42)));
static_assert(CheckValueIs<EnumCls::A>(^EnumCls::A));
static_assert(CheckValueIs<Enum::A>(^Enum::A));
static_assert(CheckValueIs<42>(^ConstVar));
static_assert(CheckValueIs<42>(^ConstexprVar));
static_assert(CheckValueIs<&fn>(^fn));
static_assert(CheckValueIs<&Cls::k>(^Cls::k));
static_assert(CheckValueIs<&Cls::fn>(^Cls::fn));
static_assert(CheckValueIs<42>([]() {
  constexpr int x = 42;
  return ^x;
}()));

[[maybe_unused]] TCls<3> ignored;
static_assert(
    CheckValueIs<3>(
        static_data_members_of(substitute(^TCls,
                                          {std::meta::reflect_value(3)}))[0]));
}  // namespace extract_results

                                  // =========
                                  // roundtrip
                                  // =========

namespace roundtrip {
template <typename T>
consteval bool Roundtrip(T value) {
  return extract<T>(std::meta::reflect_value(value)) == value;
}
static_assert(Roundtrip(42));
static_assert(Roundtrip(EnumCls::A));
static_assert(Roundtrip(Enum::A));
static_assert(Roundtrip(ConstVar));
static_assert(Roundtrip(ConstexprVar));
static_assert(Roundtrip(fn));
static_assert(Roundtrip(&Cls::k));
static_assert(Roundtrip(&Cls::fn));
static_assert(Roundtrip([] { }));
}  // namespace roundtrip

                            // =====================
                            // extract_ref_semantics
                            // =====================

namespace extract_ref_semantics {
  int nonConstGlobal = 1;
  const int constGlobal = 2;

  static_assert(&extract<int &>(^nonConstGlobal) == &nonConstGlobal);
  static_assert(extract<int &>(^constGlobal) == 2);

  const int &constGlobalRef = constGlobal;
  static_assert(&extract<const int &>(^constGlobalRef) == &constGlobal);

  // TODO(P2996): Need to decide whether 'extract' should be legal on locals
  // within an immediate function context. It isn't legal as spec'd by P2996R3,
  // but we may want to make it work. Not going to sink more time into making
  // the commented line below work, until we have a decision.
  consteval int myfn(int arg) {
    int val = 3;
    int &ref = extract<int &>(^val);

    //int &ref2 = extract<int &>(^ref);
    //static_assert(&ref2 == &val);

    ref = 4;
    return val + extract<int>(^arg);
  }
  static_assert(myfn(5) == 9);

  consteval const int &returnsRef() { return constGlobal; }

  void nonConstFn() {
    constexpr auto r = std::meta::reflect_object(returnsRef());
    static_assert(extract<const int &>(r) == 2);
  }
}  // namespace extract_ref_semantics

                               // ==============
                               // value_of_types
                               // ==============

namespace value_of_types {
struct S{};

using Alias1 = int;
constexpr Alias1 a1 = 3;
static_assert(type_of(^a1) == ^const Alias1);
static_assert(type_of(value_of(^a1)) == ^int);

using Alias2 = S;
constexpr Alias2 a2 {};
static_assert(type_of(^a2) == ^const Alias2);
static_assert(type_of(value_of(^a2)) == ^const S);

constexpr const int &ref = a1;
static_assert(type_of(value_of(^ref)) == ^int);

constexpr std::pair<std::pair<int, bool>, int> p = {{1, true}, 2};
static_assert(type_of(value_of(std::meta::reflect_object(p.first))) ==
              ^const std::pair<int, bool>);
}  // namespace value_of_types

                             // ===================
                             // values_from_objects
                             // ===================

namespace values_from_objects {

const int constGlobal = 11;
constexpr auto rref = std::meta::reflect_object(constGlobal);

static_assert(value_of(^constGlobal) != ^constGlobal);
static_assert([:value_of(^constGlobal):] == 11);
static_assert([:value_of(rref):] == 11);
static_assert(value_of(^constGlobal) == value_of(rref));
static_assert(value_of(^constGlobal) == std::meta::reflect_value(11));

enum Enum { A };
static constexpr Enum e = A;

enum EnumCls { CA };
static constexpr EnumCls ce = CA;

static_assert(value_of(^A) != value_of(^CA));

static_assert(value_of(^A) != std::meta::reflect_value(0));
static_assert(value_of(^A) == std::meta::reflect_value(Enum(0)));
static_assert(value_of(^A) != std::meta::reflect_value(EnumCls(0)));
static_assert(value_of(^e) != std::meta::reflect_value(0));
static_assert(value_of(^e) == std::meta::reflect_value(Enum(0)));
static_assert(value_of(^e) != std::meta::reflect_value(EnumCls(0)));
static_assert(value_of(^ce) != std::meta::reflect_value(0));
static_assert(value_of(^ce) != std::meta::reflect_value(Enum(0)));
static_assert(value_of(^ce) == std::meta::reflect_value(EnumCls(0)));

constexpr std::pair<std::pair<int, bool>, int> p = {{1, true}, 2};
constexpr std::meta::info rfirst = std::meta::reflect_object(p.first);
static_assert(is_object(rfirst) && !is_value(rfirst));
static_assert(type_of(rfirst) == ^const std::pair<int, bool>);
static_assert(rfirst != std::meta::reflect_value(std::make_pair(1, true)));

constexpr std::meta::info rvfirst = value_of(rfirst);
static_assert(!is_object(rvfirst) && is_value(rvfirst));
static_assert(type_of(rvfirst) == ^const std::pair<int, bool>);
static_assert(rvfirst == std::meta::reflect_value(std::make_pair(1, true)));
static_assert([:rvfirst:].first == 1);
}  // namespace values_from_objects

int main() {
  // RUN: grep "call-lambda-value: 1" %t.stdout
  extract<void(*)(int)>(
        std::meta::reflect_value(
            [](int id) {
              std::println("call-lambda-value: {}", id);
            }))(1);

  // RUN: grep "call-generic-lambda-value: 2 (int)" %t.stdout
  extract<void(*)(int)>(std::meta::reflect_value(
        [](auto id) {
          std::println("call-generic-lambda-value: {} ({})", id,
                       name_of<std::string_view>(type_of(^id)));
        }))(2);

  constexpr auto l = [](int id) {
    std::println("call-lambda-var: {}", id);
  };

  constexpr auto g = [](auto id) {
    std::println("call-generic-lambda-var: {} ({})", id,
                 name_of<std::string_view>(type_of(^id)));
  };

  // RUN: grep "call-lambda-var: 1" %t.stdout
  extract<void(*)(int)>(^l)(1);

  // RUN: grep "call-lambda-var: 2" %t.stdout
  extract<decltype(l)>(^l)(2);

  // RUN: grep "call-generic-lambda-var: 3 (int)" %t.stdout
  extract<decltype(g)>(^g)(3);

  // RUN: grep "call-generic-lambda-var: true (bool)" %t.stdout
  extract<void(*)(bool)>(^g)(true);

  // RUN: grep "updated-extract-global: 42" %t.stdout
  int &ref = extract<int &>(^extract_ref_semantics::nonConstGlobal);
  ref = 42;
  std::println("updated-extract-global: {}",
               extract_ref_semantics::nonConstGlobal);

  // RUN: grep "updated-reflect-result-global: 13" %t.stdout
  constexpr auto r = std::meta::reflect_object(
        extract_ref_semantics::nonConstGlobal);
  static_assert(type_of(r) == ^int);
  [:r:] = 13;
  std::println("updated-reflect-result-global: {}",
               extract_ref_semantics::nonConstGlobal);

  // RUN: grep "splice-value-reflection: 1" %t.stdout
  static constexpr std::pair<std::pair<int, bool>, int> p = {{1, true}, 2};
  constexpr auto rvfirst = value_of(std::meta::reflect_object(p.first));
  int v = [:rvfirst:].first;
  std::println("splice-value-reflection: {}", v);
}
