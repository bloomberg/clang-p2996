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

// <experimental/reflection>
//
// [reflection]
//
// RUN: %{build}

#include <experimental/meta>
#include <algorithm>


consteval auto make_named_tuple(
      std::meta::info type,
      std::initializer_list<std::pair<std::meta::info,
                                      std::string_view>> members) {
    std::vector<std::meta::info> nsdms;
    for (auto [ty, name] : members) {
        nsdms.push_back(data_member_spec(ty, {.name=name}));
    }    
    return define_class(type, nsdms);
}

struct R;
static_assert(is_type(make_named_tuple(^^R, {{^^int, "x"}, {^^double, "y"}})));

static_assert(type_of(nonstatic_data_members_of(^^R)[0]) == ^^int);
static_assert(type_of(nonstatic_data_members_of(^^R)[1]) == ^^double);

int main() {
    [[maybe_unused]] auto r = R{.x=1, .y=2.0};
}
