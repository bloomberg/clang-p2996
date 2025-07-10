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

// Note that 'Foo' mix standard and vendor namespaced
struct [[nodiscard("yep"), deprecated("dont use me")]]
[[clang::warn_unused_result("Look I'm clang...")]] Foo {};

// Test fragments
consteval bool testHasIdentifier() {
  constexpr auto attr = ^^[[nodiscard("yop")]];
  static_assert(std::meta::has_identifier(attr));
  return true;
}

consteval bool testIsAttribute() {
  constexpr auto attr = ^^[[nodiscard]];
  static_assert(std::meta::is_attribute(attr));
  return true;
}

consteval bool testIdentifierOf() {
  static_assert(std::meta::identifier_of(^^[[deprecated]]) == "deprecated");
  return true;
}

consteval bool testAttributesOfAttr() {
  static_assert(std::meta::attributes_of(^^[[nodiscard]]).size() == 1);
  static_assert(std::meta::identifier_of(std::meta::attributes_of(^^[[nodiscard]])[0]) == "nodiscard");
  return true;
}

consteval bool testAttributesOfType() {
  constexpr auto h = [](std::meta::info i) {
    return i == ^^[[nodiscard("yep")]]
      || i == ^^[[deprecated("dont use me")]]
      || i == ^^[[clang::warn_unused_result("Look I'm clang...")]];
  };

  static_assert(std::meta::attributes_of(^^Foo).size() == 3);
  
  constexpr auto att0 = std::meta::attributes_of(^^Foo)[0];
  constexpr auto att1 = std::meta::attributes_of(^^Foo)[1];
  constexpr auto att2 = std::meta::attributes_of(^^Foo)[2];
  
  static_assert(att0 != att1 && att1 != att2 && att0 != att2);
  static_assert(h(att0));
  static_assert(h(att1));
  static_assert(h(att2));

  return true;
}

consteval bool testVendorVariant() {
  // Same semantic attribute but different spelling
  constexpr auto wur = ^^[[clang::warn_unused_result("I'm the real nodiscard")]];
  constexpr auto nd = ^^[[nodiscard("I'm the real nodiscard")]];
  static_assert(std::meta::identifier_of(std::meta::attributes_of(wur)[0]) == "warn_unused_result");
  static_assert(wur != nd); // diff spelling and namespace

   // Same spelling but different vendor
   constexpr auto nua = ^^[[no_unique_address]];
   constexpr auto msvc_nua = ^^[[msvc::no_unique_address]];
   static_assert(nua != msvc_nua); // same spelling, diff namespace

   return true;
}

// This is vendor specific, does not exist in the standard
[[gnu::constructor(200)]] void gnuConstructor(void);

consteval bool testVendorSpecific() {
  constexpr auto r = ^^gnuConstructor;
  static_assert(std::meta::attributes_of(r).size() == 1);
  static_assert(std::meta::identifier_of(std::meta::attributes_of(r)[0]) == "constructor");
  static_assert(^^[[gnu::constructor(200)]] != ^^[[gnu::constructor(100)]]); // args participate
  static_assert(std::meta::attributes_of(r)[0] == ^^[[gnu::constructor(200)]]); // recover with arg
  return true;
}

int main() {
  static_assert(testIsAttribute(), "IsAttribute");
  static_assert(testHasIdentifier(), "HasIdentifier");
  static_assert(testIdentifierOf(), "IdentifierOf");
  static_assert(testAttributesOfAttr() ,"AttributesOfAttr");
  static_assert(testAttributesOfType() ,"AttributesOfType");
  static_assert(testVendorVariant() ,"VendorVariant");
  static_assert(testVendorSpecific() ,"VendorSpecific");
}
