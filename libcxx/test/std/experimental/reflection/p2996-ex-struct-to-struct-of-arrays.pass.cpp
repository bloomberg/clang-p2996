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

#include <array>
#include <print>


template <typename T, std::size_t N>
struct struct_of_arrays_impl;

consteval auto make_struct_of_arrays(std::meta::info type,
                                     std::meta::info N) -> std::meta::info {
  std::vector<std::meta::info> old_members = nonstatic_data_members_of(type);
  std::vector<std::meta::info> new_members = {};
  for (std::meta::info member : old_members) {
    auto array_type = substitute(^^std::array, {type_of(member), N });
    auto mem_descr = data_member_spec(array_type,
                                      {.name=identifier_of(member)});
    new_members.push_back(mem_descr);
  }
  return std::meta::define_class(substitute(^^struct_of_arrays_impl, {type, N}),
                                 new_members);
}

template <typename T, size_t N>
using struct_of_arrays = 
    [: make_struct_of_arrays(^^T, std::meta::reflect_value(N)) :];


struct point {
  float x;
  float y;
  float z;
};

int main() {
  using points = struct_of_arrays<point, 3>;

  points pts = {{1, 1, 1}, {2, 2, 2}, {1, 2, 3}};

  // RUN: grep "Pts\[z\]: 1, 2, 3" %t.stdout
  std::println("Pts[z]: {}, {}, {}", pts.z[0], pts.z[1], pts.z[2]);
}
