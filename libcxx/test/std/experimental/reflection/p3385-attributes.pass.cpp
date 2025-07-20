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

// Note that 'Foo' mix standard and vendor namespaced, supported and unsupported
struct [[nodiscard("std variant"), deprecated("dont use me")]]
[[clang::warn_unused_result("clang variant")]]
[[clang::availability(macos,introduced=10.4,deprecated=10.6,obsoleted=10.7)]] Foo {};

// This is vendor specific & does not exist in the standard
[[gnu::constructor(200)]] void gnuConstructor(void);

// Some samples of vendor and standard attributes
constexpr auto stdAttr = ^^[[nodiscard("std variant")]];
constexpr auto clangAttr = ^^[[clang::warn_unused_result("clang variant")]];
constexpr auto msvcAttr = ^^[[msvc::no_unique_address]];
constexpr auto gnuAttr = ^^[[gnu::constructor(200)]];


// Test fragments
consteval bool testHasIdentifier() {
  static_assert(std::meta::has_identifier(stdAttr));
  static_assert(std::meta::has_identifier(clangAttr));
  static_assert(std::meta::has_identifier(msvcAttr));
  static_assert(std::meta::has_identifier(gnuAttr));
  return true;
}

consteval bool testIsAttribute() {
  static_assert(std::meta::is_attribute(stdAttr));
  static_assert(std::meta::is_attribute(clangAttr));
  static_assert(std::meta::is_attribute(msvcAttr));
  static_assert(std::meta::is_attribute(gnuAttr));
  return true;
}

consteval bool testIdentifierOf() {
  static_assert(std::meta::identifier_of(stdAttr) == "nodiscard");
  static_assert(std::meta::identifier_of(clangAttr) == "clang::warn_unused_result");
  static_assert(std::meta::identifier_of(msvcAttr) == "msvc::no_unique_address");
  static_assert(std::meta::identifier_of(gnuAttr) == "gnu::constructor");
  return true;
}

consteval bool testAttributesOfAttr() {
  static_assert(std::meta::attributes_of(stdAttr)[0] == stdAttr);
  static_assert(std::meta::attributes_of(clangAttr)[0] == clangAttr);
  static_assert(std::meta::attributes_of(msvcAttr)[0] == msvcAttr);
  static_assert(std::meta::attributes_of(gnuAttr)[0] == gnuAttr);
  return true;
}

consteval bool testAttributesOfType() {
  constexpr auto h = [](std::meta::info i) {
    return i == ^^[[nodiscard("std variant")]]
      || i == ^^[[deprecated("dont use me")]]
      || i == ^^[[clang::warn_unused_result("clang variant")]];
  };

  // This should not return the unsupported 'availability'
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

consteval bool testComparison() {
  // Vendor variant of identical semantic attribute
  static_assert(stdAttr != clangAttr);

   // Same spelling but different vendor
   static_assert(^^[[no_unique_address]] != ^^[[msvc::no_unique_address]]); // same spelling, diff namespace

   // Different arguments
   static_assert(^^[[gnu::constructor(200)]] != ^^[[gnu::constructor(100)]]);
   return true;
}

consteval bool testVendorSpecific() {
  constexpr auto r = ^^gnuConstructor;
  static_assert(std::meta::attributes_of(r).size() == 1);
  static_assert(std::meta::attributes_of(r)[0] == ^^[[gnu::constructor(200)]]);
  return true;
}

consteval bool testUnsupported() {
  static_assert(!std::meta::is_attribute(^^[[assume(true)]]));
  static_assert(!std::meta::is_attribute(^^[[clang::assume(true)]]));
  static_assert(!std::meta::is_attribute(^^[[my::stuff("anything")]]));
  static_assert(^^[[assume(true)]] == std::meta::info());
  return true;
}

int main() {
  static_assert(testIsAttribute(), "IsAttribute");
  static_assert(testHasIdentifier(), "HasIdentifier");
  static_assert(testIdentifierOf(), "IdentifierOf");
  static_assert(testAttributesOfAttr() ,"AttributesOfAttr");
  static_assert(testAttributesOfType() ,"AttributesOfType");
  static_assert(testComparison() ,"Comparison");
  static_assert(testVendorSpecific() ,"VendorSpecific");
  static_assert(testUnsupported() ,"Unsupported");
}

