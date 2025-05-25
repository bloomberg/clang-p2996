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
// ADDITIONAL_COMPILE_FLAGS: -freflection-new-syntax
// ADDITIONAL_COMPILE_FLAGS: -fattribute-reflection

// <experimental/reflection>
//
// [reflection]

#include <experimental/meta>
#include <utility>

// Helpers
struct Witness {
  int nodiscard;
  int maybe_unused;
  int deprecated;
};

enum class Result {
  Success,
  Ambiguous,
  Warning,
  Error,
};

struct [[nodiscard("yep"), deprecated("dont use me")]] Foo {};

consteval auto member_named(std::string_view name) {
  constexpr auto access = std::meta::access_context::unchecked();
  for (std::meta::info field : nonstatic_data_members_of(^^Witness, access)) {
    if (has_identifier(field) && identifier_of(field) == name)
      return field;
  }
  std::unreachable();
}

// Test fragments
consteval bool testHasIdentifier() {
  constexpr auto attr = ^^[[nodiscard]];
  return std::meta::has_identifier(attr);
}

consteval bool testIsAttribute() {
  constexpr auto attr = ^^[[nodiscard]];
  return std::meta::is_attribute(attr);
}

consteval bool testIdentifierOf() {
  constexpr auto attr = ^^[[deprecated]];
  Witness s{0, 0, 0};
  s.[: member_named(std::meta::identifier_of(attr)) :]++;
  return s.deprecated == 1;
}

consteval bool testAttributesOfAttr() {
  return std::meta::identifier_of(std::meta::attributes_of(^^[[nodiscard]])[0]) == "nodiscard"
    && std::meta::attributes_of(^^[[nodiscard]]).size() == 1;
}

consteval bool testAttributesOfType() {
  static_assert(std::meta::attributes_of(^^Foo).size() == 2);
  Witness s{0, 0, 0};
    s.[: member_named(std::meta::identifier_of(std::meta::attributes_of(^^Foo)[0])) :]++;
    s.[: member_named(std::meta::identifier_of(std::meta::attributes_of(^^Foo)[1])) :]++;

    return s.nodiscard == 1 && s.maybe_unused == 0 && s.deprecated == 1;
}

consteval bool testAppertain() {
  constexpr auto deprecated = ^^[[deprecated]];
  constexpr auto nodiscard  = ^^[[nodiscard]];

  consteval {
    std::meta::appertain(^^Witness, { deprecated, nodiscard });
  }
  static_assert(std::meta::attributes_of(^^Witness).size() == 2);

  Witness s{0, 0, 0};
  s.[: member_named(std::meta::identifier_of(std::meta::attributes_of(^^Witness)[0])) :]++;
  s.[: member_named(std::meta::identifier_of(std::meta::attributes_of(^^Witness)[1])) :]++;
  return s.nodiscard == 1 && s.maybe_unused == 0 && s.deprecated == 1;
}

int main() {
  static_assert(testIsAttribute(), "IsAttribute");
  static_assert(testHasIdentifier(), "HasIdentifier");
  static_assert(testIdentifierOf(), "IdentifierOf");
  static_assert(testAttributesOfAttr() ,"AttributesOfAttr");
  static_assert(testAttributesOfType() ,"AttributesOfType");
  static_assert(testAppertain() ,"Appertain");
}