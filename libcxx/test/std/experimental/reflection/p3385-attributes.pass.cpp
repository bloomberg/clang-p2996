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

// 'Foo' mix standard and vendor namespaced, supported and unsupported
struct [[nodiscard("Standard nodiscard"), deprecated("Standard deprecated")]]
[[clang::warn_unused_result("clang variant")]]
[[clang::availability(macos,introduced=10.4,deprecated=10.6,obsoleted=10.7)]] Foo {};

// This is vendor specific, does not exist in the standard
[[gnu::constructor(200)]] void gnuConstructor(void);

// NS and function
namespace [[deprecated("Standard deprecated")]] DeprecatedNamespace {}
[[deprecated("Standard deprecated")]] bool DeprecatedFunction() {}

// Test for variadic string
class [[clang::suppress("one", "two", "three")]] Bar {};

// Test for variadic enum
struct [[clang::consumable(unconsumed)]] FooBar {
    [[clang::callable_when(unconsumed)]] void f() {}
};

// Some samples of vendor and standard attributes
constexpr auto stdAttr = ^^[[nodiscard("Standard nodiscard")]];
constexpr auto clangAttr = ^^[[clang::warn_unused_result("clang variant")]];
constexpr auto msvcAttr = ^^[[msvc::no_unique_address]];
constexpr auto gnuAttr = ^^[[gnu::constructor(200)]];
constexpr auto variadicAttr = ^^[[clang::suppress("one", "two", "three")]];
constexpr auto enumAttr = ^^[[clang::callable_when(unconsumed)]];
constexpr auto stdDeprecated = ^^[[deprecated("Standard deprecated")]];

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

consteval bool testHasAttr() {
  static_assert(
       std::meta::has_attribute(^^Foo, stdAttr)
    && std::meta::has_attribute(^^Foo, stdDeprecated)
    && std::meta::has_attribute(^^Foo, clangAttr)
  );
  static_assert(std::meta::has_attribute(^^DeprecatedNamespace, stdDeprecated));
  static_assert(std::meta::has_attribute(^^DeprecatedFunction, stdDeprecated));
  // Ignore the argument only
  static_assert(std::meta::has_attribute(
    ^^DeprecatedFunction,
    ^^[[deprecated]],
    std::meta::AttributeComparison::IgnoreArgument
  ));
  // Ignore the namespace only
  static_assert(std::meta::has_attribute(
    ^^DeprecatedFunction,
    ^^[[gnu::deprecated("Standard deprecated")]],
    std::meta::AttributeComparison::IgnoreNamespace
  ));
  // Ignore both the namespace and the argument
  static_assert(std::meta::has_attribute(
    ^^DeprecatedFunction,
    ^^[[gnu::deprecated]],
    std::meta::AttributeComparison::IgnoreNamespace | std::meta::AttributeComparison::IgnoreArgument
  ));
  return true;
}

consteval bool testAttributesOfAttr() {
  static_assert(std::meta::attributes_of(stdAttr)[0] == stdAttr);
  static_assert(std::meta::attributes_of(clangAttr)[0] == clangAttr);
  static_assert(std::meta::attributes_of(msvcAttr)[0] == msvcAttr);
  static_assert(std::meta::attributes_of(gnuAttr)[0] == gnuAttr);
  return true;
}

consteval bool testAttributesofNs() {
  static_assert(std::meta::has_attribute(^^DeprecatedNamespace, stdDeprecated));
  return true;
}

consteval bool testAttributesofFunction() {
  static_assert(std::meta::has_attribute(^^DeprecatedFunction, stdDeprecated));
  return true;
}

consteval bool testAttributesOfType() {
  // This should not return the unsupported 'availability'
  static_assert(std::meta::attributes_of(^^Foo).size() == 3);
  
  static_assert(std::meta::has_attribute(^^Foo, stdAttr));
  static_assert(std::meta::has_attribute(^^Foo, stdDeprecated));
  static_assert(std::meta::has_attribute(^^Foo, clangAttr));

  static_assert(std::meta::attributes_of(^^Foo, "warn_unused_result", std::meta::AttributeNamespace::Clang).size() == 1);
  static_assert(std::meta::attributes_of(^^Foo, "warn_unused_result", std::meta::AttributeNamespace::Clang)[0] == clangAttr);

  static_assert(std::meta::attributes_of(^^Foo, "nodiscard").size() == 1);
  static_assert(std::meta::attributes_of(^^Foo, "nodiscard")[0] == stdAttr);

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
  static_assert(std::meta::attributes_of(r)[0] == gnuAttr);
  return true;
}

consteval bool testUnsupportedAttributes() {
  static_assert(!std::meta::is_attribute(^^[[my::stuff("anything")]])); // expected-error {{reflecting over the unsupported 'availability' attribute is ill-formed}}
  static_assert(!std::meta::is_attribute(^^[[clang::availability(macos,introduced=10.4,deprecated=10.6,obsoleted=10.7)]])); // expected-error {{reflecting over the unsupported 'stuff' attribute is ill-formed}}
  return true;
}

consteval bool testVariadicStringArgument() {
  static_assert(std::meta::attributes_of(^^Bar).size() == 1);
  static_assert(std::meta::attributes_of(^^Bar, "suppress", std::meta::AttributeNamespace::Clang)[0] == variadicAttr);
  static_assert(std::meta::attributes_of(^^Bar, "suppress", std::meta::AttributeNamespace::Clang)[0] != ^^[[clang::suppress("one", "too", "three")]]); // typo on 'two'
  return true;
}

consteval bool testVariadicEnumArgument() {
  constexpr auto r = ^^FooBar::f;
  static_assert(std::meta::attributes_of(r).size() == 1);
  static_assert(std::meta::attributes_of(r)[0] == enumAttr);
  return true;
}

struct TestDefineAggregate {
  struct Impl;
  consteval {
    constexpr auto r = std::meta::data_member_spec(
      ^^int,
      std::meta::data_member_options{
        .name= "idx",
        .attributes = { ^^[[maybe_unused]], ^^[[no_unique_address]] }
      }
    );

    std::meta::define_aggregate(
      ^^Impl, {
        r
      }
    );
  }
};
static_assert(std::meta::is_complete_type(^^TestDefineAggregate::Impl));
static_assert(std::meta::attributes_of(std::meta::nonstatic_data_members_of(
                  ^^TestDefineAggregate::Impl, std::meta::access_context::current())[0]).size() == 2);

consteval bool testAssumeAttribute() {
  static_assert(std::meta::is_attribute(^^[[assume(true)]]));
  static_assert(std::meta::is_attribute(^^[[clang::assume(true)]]));

  static_assert(^^[[assume(true)]] == ^^[[assume(true)]]);
  static_assert(^^[[assume(true)]] != ^^[[assume(false)]]);
  int i = 0;
  static_assert(^^[[assume(i > 0)]] != ^^[[assume(i == 0)]]);
  static_assert(^^[[assume(i != 0)]] == ^^[[assume(i != 0)]]);

  return true;
}

int main() {
  return 0;
}
