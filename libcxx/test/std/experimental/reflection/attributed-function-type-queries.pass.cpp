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
// Regression test: an attribute on a function declarator (e.g.
// [[clang::lifetimebound]] on the implicit object parameter -- Abseil applies
// it to every accessor via ABSL_ATTRIBUTE_LIFETIME_BOUND) wraps the
// declaration's FunctionProtoType in AttributedType sugar. Metafunctions that
// reached the FunctionProtoType with a sugar-blind dyn_cast misbehaved on such
// functions: is_const / is_volatile / is_lvalue_reference_qualified /
// is_rvalue_reference_qualified silently answered false, and return_type_of /
// parameters_of / has_ellipsis_parameter on the function's TYPE rejected it as
// not-a-function-type. They now desugar via getAs<FunctionProtoType>, like
// is_noexcept always did.

#include <meta>

#include <string_view>

constexpr auto unchecked = std::meta::access_context::unchecked();

consteval std::meta::info member_named(std::meta::info cls,
                                       std::string_view name) {
  for (auto m : members_of(cls, unchecked))
    if (has_identifier(m) && identifier_of(m) == name)
      return m;
  return {};
}

struct W {
  const int& cl() const & [[clang::lifetimebound]] { return v; }
  int& l() & [[clang::lifetimebound]] { return v; }
  const int&& cr() const && [[clang::lifetimebound]] {
    return static_cast<const int&&>(v);
  }
  int&& r() && [[clang::lifetimebound]] { return static_cast<int&&>(v); }
  const volatile int& cv() const volatile [[clang::lifetimebound]] {
    return v;
  }
  int v;
};

// Qualifier predicates on the declarations themselves.
static_assert(is_const(member_named(^^W, "cl")));
static_assert(is_lvalue_reference_qualified(member_named(^^W, "cl")));
static_assert(!is_rvalue_reference_qualified(member_named(^^W, "cl")));
static_assert(!is_const(member_named(^^W, "l")));
static_assert(is_lvalue_reference_qualified(member_named(^^W, "l")));
static_assert(is_const(member_named(^^W, "cr")));
static_assert(is_rvalue_reference_qualified(member_named(^^W, "cr")));
static_assert(!is_const(member_named(^^W, "r")));
static_assert(is_rvalue_reference_qualified(member_named(^^W, "r")));
static_assert(!is_lvalue_reference_qualified(member_named(^^W, "r")));
static_assert(is_const(member_named(^^W, "cv")));
static_assert(is_volatile(member_named(^^W, "cv")));

// The same queries through the function's TYPE (which carries the sugar),
// plus the type-introspection metafunctions that rejected it outright.
static_assert(is_const(type_of(member_named(^^W, "cl"))));
static_assert(is_lvalue_reference_qualified(type_of(member_named(^^W, "cl"))));
static_assert(is_rvalue_reference_qualified(type_of(member_named(^^W, "r"))));
static_assert(is_volatile(type_of(member_named(^^W, "cv"))));
static_assert(return_type_of(type_of(member_named(^^W, "l"))) == ^^int&);
static_assert(parameters_of(type_of(member_named(^^W, "l"))).size() == 0);
static_assert(!has_ellipsis_parameter(type_of(member_named(^^W, "l"))));

// And through an entity proxy's underlying entity (the shape that exposed
// this in the field: absl::StatusOr<int>'s re-exported accessors).
struct D : private W {
  using W::cl;
  using W::r;
};

static_assert(is_const(underlying_entity_of(member_named(^^D, "cl"))));
static_assert(
    is_lvalue_reference_qualified(underlying_entity_of(member_named(^^D, "cl"))));
static_assert(
    is_rvalue_reference_qualified(underlying_entity_of(member_named(^^D, "r"))));
static_assert(!is_const(underlying_entity_of(member_named(^^D, "r"))));

int main() { return 0; }
