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

#include <experimental/meta>

#include <array>
#include <cstdlib>
#include <print>


struct member_descriptor
{
  std::size_t offset;
  std::size_t size;
};

// returns std::array<member_descriptor, N>
template <typename S>
consteval auto get_layout() {
  auto members = nonstatic_data_members_of(^^S);
  constexpr size_t sz = nonstatic_data_members_of(^^S).size();
  std::array<member_descriptor, sz> layout;
  for (int i = 0; i < members.size(); ++i) {
      layout[i] = {
          .offset=offset_of(members[i]).bytes,
          .size=size_of(members[i])
      };
  }
  return layout;
}

struct X
{
    char a;
    int b;
    double c;
};


int main() {
  for (const auto &md : get_layout<X>())
    std::println("({}, {})", md.offset, md.size);
}
