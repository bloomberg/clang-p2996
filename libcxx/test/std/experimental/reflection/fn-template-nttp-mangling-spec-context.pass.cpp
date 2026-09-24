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
// Regression test: reflections of same-named member function templates of a
// CLASS TEMPLATE SPECIALIZATION used as non-type template arguments must
// mangle distinctly even when the siblings share an identical template head.
// The NTTP discriminator introduced for overloaded function templates hashed
// the template head plus the declaration via ODRHash::AddFunctionDecl -- but
// AddFunctionDecl silently NO-OPS for any declaration in "specialization
// context" (a member of a ClassTemplateSpecializationDecl, the common
// members_of shape), so an overload set like tl::expected<T,E>'s four value()
// member templates (const& / & / const&& / &&, identical heads) hashed
// identically: CodeGen silently folded the linkonce_odr specializations of a
// dispatcher taking the reflection as an NTTP, and one body served all four
// call sites. The AST-level specializations are correct, so only a RUNTIME
// observation catches this (same silent-fold mode as the overloaded
// function-template discriminator's original motivation).

#include <meta>

#include <cassert>
#include <string_view>

namespace meta = std::meta;
constexpr auto ctx = meta::access_context::unchecked();

template <class T> struct trait { static constexpr bool value = true; };

// The tl::expected<T,E>::value() field shape: four same-named member function
// templates with IDENTICAL template heads, differing only in cv/ref qualifiers
// and (correspondingly) return type.
template <class T> struct Exp {
  template <class U = T, bool = trait<U>::value>
  const U &value() const & { return v; }
  template <class U = T, bool = trait<U>::value>
  U &value() & { return v; }
  template <class U = T, bool = trait<U>::value>
  const U &&value() const && { return static_cast<const U &&>(v); }
  template <class U = T, bool = trait<U>::value>
  U &&value() && { return static_cast<U &&>(v); }
  T v;
};

// Two same-headed siblings differing ONLY in ref-qualifier (the return types
// coincide): the ref-qualifier is not part of what VisitFunctionProtoType
// hashes, so it needs its own discrimination.
template <class T> struct RQ {
  template <class U = T> int get() & { return 1; }
  template <class U = T> int get() && { return 2; }
};

template <meta::info R> int probe() { return 1; }

int main() {
  // All four value() siblings of a specialization must instantiate probe<m>
  // DISTINCTLY (pre-fix: one mangled name, "definition with same mangled
  // name" at best, a silent linkonce_odr fold at worst).
  int (*addrs[8])() = {};
  int n = 0;
  template for (constexpr auto m : std::define_static_array(
      meta::members_of(^^Exp<int>, ctx))) {
    if constexpr (meta::is_function_template(m) && meta::has_identifier(m)) {
      if constexpr (meta::identifier_of(m) == std::string_view("value")) {
        addrs[n++] = &probe<m>;
      }
    }
  }
  assert(n == 4);
  for (int i = 0; i < n; ++i)
    for (int j = i + 1; j < n; ++j)
      assert(addrs[i] != addrs[j]);

  // Ref-qualifier-only siblings stay distinct too.
  int (*raddrs[8])() = {};
  int rn = 0;
  template for (constexpr auto m : std::define_static_array(
      meta::members_of(^^RQ<int>, ctx))) {
    if constexpr (meta::is_function_template(m) && meta::has_identifier(m)) {
      if constexpr (meta::identifier_of(m) == std::string_view("get")) {
        raddrs[rn++] = &probe<m>;
      }
    }
  }
  assert(rn == 2);
  assert(raddrs[0] != raddrs[1]);
  return 0;
}
