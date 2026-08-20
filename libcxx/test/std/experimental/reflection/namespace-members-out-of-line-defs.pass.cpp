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
// Regression test: an out-of-line definition of a class member at namespace
// scope is a redeclaration of the CLASS member -- it must never be enumerated
// as a member of the namespace, and its presence must not derail the
// namespace walk. Two defects compounded when a RE-OPENED namespace block
// began with such a definition (Eigen's header layout: EulerAngles.h opens
// `namespace Eigen` with the MatrixBase<Derived>::canonicalEulerAngles
// definition):
//   1. the definition (for a class template, the dependent definition
//      PATTERN) was yielded as a namespace member -- and reflections of such
//      patterns break downstream metafunctions;
//   2. stepping from it walked the CLASS's chain instead of the namespace's,
//      silently dropping every remaining namespace member (members_of
//      returned ONE wrong entry for the whole namespace).

#include <experimental/meta>

#include <string_view>

namespace single {
struct Plain { int f() const; };
int Plain::f() const { return 1; }            // out-of-line, same block
template <class T> struct Tmpl { int g() const; };
template <class T> int Tmpl<T>::g() const { return 2; }
inline int h() { return 3; }
}  // namespace single

namespace re {
template <class T> struct Tmpl { int g() const; };
inline int h() { return 3; }
}  // namespace re

namespace re {                                 // re-opened; FIRST decl is the
template <class T> int Tmpl<T>::g() const {    // out-of-line definition PATTERN
  return 2;
}
inline int k() { return 4; }
struct Plain { int f() const; };
}  // namespace re

namespace re {                                 // re-opened again; starts with
int Plain::f() const { return 1; }             // a NON-template out-of-line def
inline int m() { return 5; }
}  // namespace re

consteval bool ns_has_member(std::meta::info ns, std::string_view name) {
  for (auto m :
       std::meta::members_of(ns, std::meta::access_context::unchecked()))
    if (std::meta::has_identifier(m) && std::meta::identifier_of(m) == name)
      return true;
  return false;
}

consteval std::size_t ns_member_count(std::meta::info ns) {
  std::size_t n = 0;
  for (auto m :
       std::meta::members_of(ns, std::meta::access_context::unchecked())) {
    (void)m;
    ++n;
  }
  return n;
}

int main() {
  // Single-block namespace: out-of-line defs interleaved with members.
  static_assert(ns_has_member(^^single, "Plain"));
  static_assert(ns_has_member(^^single, "Tmpl"));
  static_assert(ns_has_member(^^single, "h"));
  static_assert(!ns_has_member(^^single, "f"));
  static_assert(!ns_has_member(^^single, "g"));
  static_assert(ns_member_count(^^single) == 3);

  // Re-opened blocks BEGINNING with out-of-line definitions: every real
  // member across all blocks must survive, no definition may leak in.
  static_assert(ns_has_member(^^re, "Tmpl"));
  static_assert(ns_has_member(^^re, "Plain"));
  static_assert(ns_has_member(^^re, "h"));
  static_assert(ns_has_member(^^re, "k"));
  static_assert(ns_has_member(^^re, "m"));
  static_assert(!ns_has_member(^^re, "g"));
  static_assert(!ns_has_member(^^re, "f"));
  static_assert(ns_member_count(^^re) == 5);

  // The class member itself is still enumerable through its class, and the
  // namespace-scope definition did not duplicate it.
  static_assert(ns_member_count(^^re) == 5);
  consteval {
    std::size_t fs = 0;
    for (auto m : std::meta::members_of(
             ^^re::Plain, std::meta::access_context::unchecked()))
      if (std::meta::has_identifier(m) && std::meta::identifier_of(m) == "f")
        ++fs;
    if (fs != 1)
      __builtin_abort();
  }

  return 0;
}
