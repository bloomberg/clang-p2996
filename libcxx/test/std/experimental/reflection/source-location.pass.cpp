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
// REQUIRES: stdlib=libc++
// ADDITIONAL_COMPILE_FLAGS: -freflection
// ADDITIONAL_COMPILE_FLAGS: -freflection-new-syntax
// ADDITIONAL_COMPILE_FLAGS: -Wno-inconsistent-missing-override
// ADDITIONAL_COMPILE_FLAGS: -Wno-unneeded-internal-declaration

// <experimental/reflection>
//
// [reflection]

#include <experimental/meta>


constexpr std::source_location loc = std::source_location::current();
constexpr std::source_location reflected_loc = source_location_of(^^loc);
static_assert(std::string_view(reflected_loc.file_name()) == loc.file_name());
static_assert(reflected_loc.line() == loc.line());
static_assert(reflected_loc.column() == 32);

void foo(int param) {
    constexpr std::string_view FnName = __PRETTY_FUNCTION__;

    static_assert(source_location_of(^^param).function_name() == FnName);

    int var;
    static_assert(source_location_of(^^var).function_name() == FnName);

    struct S { std::source_location mem; };
    static_assert(source_location_of(^^S).function_name() == FnName);

    struct C : S {};
    static_assert(source_location_of(bases_of(^^C)[0]).line() ==
                  std::source_location::current().line() - 2);

    // Check that it works with aliases.
    using intAlias = int;
    static_assert(source_location_of(^^intAlias).function_name() == FnName);

    // Should members of S be considered in this function? Currently, no.
    static_assert(source_location_of(^^S::mem).function_name() ==
                  std::string_view());

    // Check that it works with reflections of templates.
    constexpr std::string_view meta_loc =
          source_location_of(^^std::meta::extract).file_name();
    static_assert(meta_loc.ends_with("/meta"));

    // Check that it works with built-ins.
    static_assert(source_location_of(^^int).file_name() == std::string_view());

    // Check that it doesn't fail for nonsense queries.
    constexpr std::source_location a =
          source_location_of(data_member_spec(^^int, {}));
    constexpr std::source_location b {};
    static_assert(a.line() == b.line() && a.column() == b.column());
}


int main() { }
