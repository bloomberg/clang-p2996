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
// ADDITIONAL_COMPILE_FLAGS: -fentity-proxy-reflection

// <experimental/reflection>
//
// [reflection]
//
// Regression test: with -fentity-proxy-reflection, members_of enumerates
// using-shadow declarations as EntityProxy reflections, so ordinary member
// queries reach them directly. Several consumers used to treat that kind as
// llvm_unreachable("proxies should already have been unwrapped"):
//
//   * is_constructor / is_destructor / is_special_member_function /
//     is_static_member / is_enumerable_type / has_complete_definition ICEd
//     when asked of a proxy. They now answer false, consistent with how the
//     other kind predicates already classify proxies (a shadow declaration is
//     never itself one of these; query the underlying entity for its
//     properties via underlying_entity_of).
//   * The Itanium mangler encoded a proxy reflection as a template argument
//     by mangling the shadow declaration's NAME (mangleNameWithAbiTags).
//     Two defects followed: (1) an operator-named shadow (using B::operator*)
//     crashed outright -- mangleUnqualifiedName casts the decl to
//     FunctionDecl to disambiguate the operator name, and a UsingShadowDecl
//     is not one (cast<> assertion); (2) one using-declarator over an
//     overload set introduces several same-named shadows whose proxies all
//     mangled identically ("definition with same mangled name" at best, a
//     silent CodeGen fold at worst). The mangler now encodes the proxy's
//     TARGET declaration, keeping the proxy tag so proxy-of-X and
//     declaration-of-X reflections remain distinct template arguments
//     through codegen.

#include <meta>

#include <cassert>

constexpr auto unchecked = std::meta::access_context::unchecked();

// ===== Part 1: member-kind queries on proxies (used to be unreachable) =====

struct B {
  int f() const { return 1; }
  static int g() { return 2; }
  static int s;
  int field;
  using T = int;
  struct N {};
};

struct D : private B {
  using B::f;
  using B::g;
  using B::s;
  using B::field;
  using B::T;
  using B::N;
};

consteval bool check_member_queries() {
  int proxies = 0;
  for (auto m : members_of(^^D, unchecked)) {
    if (!is_entity_proxy(m))
      continue;
    ++proxies;

    // A using-shadow declaration is never itself any of these.
    if (is_constructor(m) || is_destructor(m) ||
        is_special_member_function(m) || is_static_member(m))
      return false;
    if (is_enumerable_type(m))
      return false;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    if (has_complete_definition(m))
      return false;
#pragma clang diagnostic pop
  }
  // One proxy per using-declarator above.
  return proxies == 6;
}
static_assert(check_member_queries());

// ===== Part 2: proxy reflections as template arguments through codegen =====

template <class T> struct OB {
  // One using-declarator over this overload set introduces FOUR same-named
  // shadows: their proxies must not collide as template arguments.
  int value() & { return 1; }
  int value() const & { return 2; }
  int value() && { return 3; }
  int value() const && { return 4; }
  // An operator-named shadow crashed the mangler outright (this is the shape
  // absl::StatusOr's `using StatusOr::OperatorBase::operator*;` hits).
  int operator*() const { return 5; }
};

template <class T> struct S : private OB<T> {
  using OB<T>::value;
  using OB<T>::operator*;
};

template <std::meta::info M> int probe() { return 7; }

void check_proxy_as_template_arg() {
  int n = 0;
  int total = 0;
  const void *proxy_addrs[5] = {};
  template for (constexpr auto m : std::define_static_array(
          members_of(^^S<int>, unchecked))) {
    if constexpr (is_entity_proxy(m)) {
      total += probe<m>();
      proxy_addrs[n] = reinterpret_cast<const void *>(&probe<m>);
      // A proxy-of-X must not collide with the declaration-of-X reflection
      // (the proxy tag in the mangling): otherwise CodeGen silently folds.
      assert(proxy_addrs[n] !=
             reinterpret_cast<const void *>(&probe<underlying_entity_of(m)>));
      ++n;
    }
  }
  assert(n == 5);
  assert(total == 35);
  // Every proxy must remain its own specialization: the four same-named
  // value() shadows and the operator* shadow are five distinct entities.
  for (int i = 0; i < n; ++i)
    for (int j = i + 1; j < n; ++j)
      assert(proxy_addrs[i] != proxy_addrs[j]);
}

int main() {
  check_proxy_as_template_arg();
  return 0;
}
