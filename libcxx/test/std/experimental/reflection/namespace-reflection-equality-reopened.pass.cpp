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
// Regression test: two reflections of the same NAMESPACE must compare equal
// regardless of which redeclaration (re-opened block) each was derived from.
// ^^ns wraps the namespace's first declaration, while parent_of(^^member)
// wraps the block that declared the member; profileReflection used to profile
// the raw declaration pointer for ReflectionKind::Namespace (unlike Template,
// which canonicalizes), so the two compared UNEQUAL whenever the namespace had
// been re-opened. Field shape: Eigen re-opens Eigen::internal in nearly every
// header, so an `^^Eigen::internal`-based exclusion test never matched the
// parent chain of internal entities.
//
// A namespace ALIAS must remain distinct from the aliased namespace (the
// dealias distinction): canonicalization is to the alias's own first
// declaration, not to its target.

#include <experimental/meta>

// A namespace declared once.
namespace single {
template <class T> struct S {};
inline void f() {}
}  // namespace single

// A namespace declared, then re-opened (the member comes from the SECOND
// block).
namespace re {
namespace opened {}
}  // namespace re
namespace re {
namespace opened {
template <class T> struct R {};
inline void g() {}
struct C {};
}  // namespace opened
}  // namespace re

// Nested re-opening: both levels re-opened.
namespace re {
namespace opened {
namespace deeper {
struct D {};
}  // namespace deeper
}  // namespace opened
}  // namespace re

namespace alias = re::opened;

int main() {
  // Single-block namespaces always worked.
  static_assert(std::meta::parent_of(^^single::S) == ^^single);
  static_assert(std::meta::parent_of(^^single::f) == ^^single);

  // Members declared in a RE-OPENED block: parent_of must still equal ^^ns.
  static_assert(std::meta::parent_of(^^re::opened::R) == ^^re::opened);
  static_assert(std::meta::parent_of(^^re::opened::g) == ^^re::opened);
  static_assert(std::meta::parent_of(^^re::opened::C) == ^^re::opened);

  // The parent of a class-template SPECIALIZATION routes through the
  // template's declaration context too.
  static_assert(std::meta::parent_of(^^re::opened::R<int>) == ^^re::opened);

  // Walking a parent CHAIN out of a doubly re-opened nesting.
  static_assert(std::meta::parent_of(^^re::opened::deeper::D) ==
                ^^re::opened::deeper);
  static_assert(std::meta::parent_of(std::meta::parent_of(
                    ^^re::opened::deeper::D)) == ^^re::opened);
  static_assert(std::meta::parent_of(^^re::opened) == ^^re);

  // The same namespace named two ways is one entity.
  static_assert(^^re::opened == ^^re::opened);

  // A namespace alias is NOT the namespace it names (dealias distinction)...
  static_assert(^^alias != ^^re::opened);
  // ...but dealiasing it gets the namespace, equal to any other reflection
  // of it.
  static_assert(std::meta::underlying_entity_of(^^alias) == ^^re::opened);
  static_assert(std::meta::underlying_entity_of(^^alias) ==
                std::meta::parent_of(^^re::opened::R));

  return 0;
}
