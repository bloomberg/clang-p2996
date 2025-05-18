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
// ADDITIONAL_COMPILE_FLAGS: -Wno-unneeded-internal-declaration

// <experimental/reflection>
//
// [reflection]

#include <experimental/meta>


constexpr auto ctx = std::meta::access_context::unchecked();

                                // =============
                                // static_arrays
                                // =============

namespace static_arrays {
constexpr auto empty = std::define_static_array(std::vector<int>{});
static_assert(empty.size() == 0);

constexpr auto ints = std::define_static_array(std::vector{1, 3, 5});
static_assert(ints.size() == 3);
static_assert(ints[0] == 1 && ints[1] == 3 && ints[2] == 5);

struct Cls {
  int k;

  consteval Cls(int v) : k(v) {}
  consteval Cls(const Cls &rhs) : k(rhs.k + 1) {}
};
constexpr auto objs = std::define_static_array(std::vector<Cls>{1, 3, 5});
static_assert(objs.size() == 3);
static_assert(objs[0].k == 4 && objs[1].k == 6 && objs[2].k == 8);

constexpr auto infos = std::define_static_array(
                                              nonstatic_data_members_of(^^Cls,
                                                                        ctx));
static_assert(infos.size() == 1);
static_assert(infos[0] == ^^Cls::k);
}  // namespace static_arrays

                               // ==============
                               // static_strings
                               // ==============

namespace static_strings {
// Ensure 'define_static_string("literal")' can be used as a template argument.
template <auto S> consteval auto fn() { return S[0]; }
static_assert(fn<std::define_static_string("literal")>() == 'l');

}  // namespace static_strings

int main() { }
