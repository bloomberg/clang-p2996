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
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

// <experimental/reflection>
//
// [reflection]
//
// Regression test: reflections of same-named (overloaded) function templates
// used as non-type template arguments must produce DISTINCT specializations
// all the way through codegen. Previously the Itanium mangling for a
// ReflectionKind::Template encoded only the template's name, so a dispatcher
// like fn_probe<T, tmpl> below got one mangled name for its two
// instantiations; CodeGen silently folded the linkonce_odr definitions, and a
// single body (here: the SFINAE-false pack sibling's) served both call sites.
// The AST-level specializations were always correct, so only a RUNTIME
// observation catches this.

#include <meta>

#include <cassert>
#include <string>
#include <type_traits>
#include <vector>

template <bool C> using EnableIf = std::enable_if_t<C, int>;
template <class K, bool V> inline constexpr bool LTB = !V;

struct B {
  // Instantiable with zero explicit arguments (every parameter defaulted).
  template <class K = int, class P = double, int = EnableIf<LTB<K, false>>()>
  std::string& operator[](K&& k);
  // SFINAE-false pack sibling: same name, never instantiable.
  template <class K = int, class P = double, int&..., EnableIf<LTB<K, true>> = 0>
  std::string& operator[](K&& k);
};

consteval bool can_sub0(std::meta::info tmpl) {
  return std::meta::can_substitute(tmpl, std::vector<std::meta::info>{});
}
consteval std::meta::info sub0(std::meta::info tmpl) {
  return std::meta::substitute(tmpl, std::vector<std::meta::info>{});
}
consteval int member_index(std::meta::info cls, std::meta::info fn) {
  int i = 0;
  for (auto m : std::meta::members_of(cls, std::meta::access_context::unchecked())) {
    if (m == fn) return i;
    ++i;
  }
  return -1;
}

// The probe: substitute() two dependent levels deep (called from a `template
// for` body in fn_driver). Encodes which template it was instantiated for and
// whether it substituted into its return value.
template <typename T, std::meta::info tmpl>
int fn_probe() {
  if constexpr (can_sub0(tmpl)) {
    constexpr auto spec = sub0(tmpl);
    static_assert(std::meta::is_operator_function(spec));
    static_assert(!std::meta::is_volatile(spec));
    static_assert(!std::meta::is_rvalue_reference_qualified(spec));
    return member_index(^^T, tmpl) * 10 + 1;
  } else {
    return member_index(^^T, tmpl) * 10;
  }
}

template <typename T>
void fn_driver() {
  int results[2];
  int (*addrs[2])();
  int n = 0;
  template for (constexpr auto fn : std::define_static_array(
          std::meta::members_of(^^T, std::meta::access_context::unchecked()))) {
    if constexpr (std::meta::is_function_template(fn)) {
      results[n] = fn_probe<T, fn>();
      addrs[n] = &fn_probe<T, fn>;
      ++n;
    }
  }
  assert(n == 2);
  // Distinct specializations: under the mangling collision these were one
  // folded symbol, so the addresses compared equal and one body ran twice.
  assert(addrs[0] != addrs[1]);
  // member#0 substitutes (0 * 10 + 1), the pack sibling does not (1 * 10).
  assert(results[0] == 1);
  assert(results[1] == 10);
}

int main() {
  fn_driver<B>();
  return 0;
}
