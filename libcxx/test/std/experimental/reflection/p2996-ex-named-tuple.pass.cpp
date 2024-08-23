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
// ADDITIONAL_COMPILE_FLAGS: -Wno-inconsistent-missing-override

// <experimental/reflection>
//
// [reflection]
//
// RUN: %{build}
// RUN: %{exec} %t.exe > %t.stdout

#include <experimental/meta>

#include <algorithm>
#include <string_view>


template <size_t N>
struct fixed_string {
  char data[N];

  constexpr fixed_string(char const (&s)[N]) {
    std::copy(s, s+N, data);
  }

  constexpr auto view() const -> std::string_view { return data; }
};

template <size_t N>
fixed_string(const char (&s)[N]) -> fixed_string<N>;

template <class T, fixed_string Name>
struct pair {
  static constexpr auto name() -> std::string_view { return Name.view(); }
  using type = T;
};

template <class... Tags>
consteval auto make_named_tuple(std::meta::info type, Tags... tags) {
  std::vector<std::meta::info> nsdms;
  auto f = [&]<class Tag>(Tag){
    nsdms.push_back(data_member_spec(dealias(^^typename Tag::type),
                                     {.name=Tag::name()}));
  };
  (f(tags), ...);
  return define_class(type, nsdms);
}

struct R;
static_assert(is_type(make_named_tuple(^^R, pair<int, "x">{},
                                           pair<double, "y">{})));

static_assert(type_of(nonstatic_data_members_of(^^R)[0]) == ^^int);
static_assert(type_of(nonstatic_data_members_of(^^R)[1]) == ^^double);

int main() {
  [[maybe_unused]] auto r = R{.x=1, .y=2.0};
}
