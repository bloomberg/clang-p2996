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
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest -fattribute-reflection

// <experimental/reflection>
//
// [reflection]
//
// Attribute reflection for attributes with type arguments (TypeArgument in
// Attr.td). Verifies that type-arg attributes are reflectable, that
// different type arguments compare unequal, and that has_attribute properly
// distinguishes them.

#include <meta>

struct [[gsl::Owner(int)]] OwnerInt {};
struct [[gsl::Owner(float)]] OwnerFloat {};
struct [[gsl::Pointer(int)]] PointerInt {};

// Basic reflectability
constexpr auto ownerIntAttr = ^^[[gsl::Owner(int)]];
constexpr auto ownerFloatAttr = ^^[[gsl::Owner(float)]];
constexpr auto pointerIntAttr = ^^[[gsl::Pointer(int)]];

static_assert(std::meta::is_attribute(ownerIntAttr));
static_assert(std::meta::is_attribute(ownerFloatAttr));
static_assert(std::meta::is_attribute(pointerIntAttr));

// Identity: same attr + same type arg are equal
static_assert(^^[[gsl::Owner(int)]] == ^^[[gsl::Owner(int)]]);
static_assert(^^[[gsl::Pointer(int)]] == ^^[[gsl::Pointer(int)]]);

// Different type arg => not equal
static_assert(^^[[gsl::Owner(int)]] != ^^[[gsl::Owner(float)]]);

// Different attr name => not equal (even with same type arg)
static_assert(^^[[gsl::Owner(int)]] != ^^[[gsl::Pointer(int)]]);

// has_attribute: matches when type arg agrees
static_assert(std::meta::has_attribute(^^OwnerInt, ownerIntAttr));
static_assert(std::meta::has_attribute(^^OwnerFloat, ownerFloatAttr));
static_assert(std::meta::has_attribute(^^PointerInt, pointerIntAttr));

// has_attribute: does NOT match when type arg differs
static_assert(!std::meta::has_attribute(^^OwnerInt, ownerFloatAttr));
static_assert(!std::meta::has_attribute(^^OwnerFloat, ownerIntAttr));

// ignore_argument: matches regardless of type arg
static_assert(std::meta::has_attribute(
  ^^OwnerInt,
  ownerFloatAttr,
  std::meta::attribute_comparison::ignore_argument
));

// ignore_namespace: gsl::Owner vs Owner
static_assert(std::meta::has_attribute(
  ^^OwnerInt,
  ^^[[gsl::Owner(int)]],
  std::meta::attribute_comparison::ignore_namespace
));

int main() { return 0; }
