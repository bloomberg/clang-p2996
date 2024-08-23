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
#include <tuple>


template<typename... Ts> struct Tuple {
  struct storage;

  static_assert(is_type(define_class(^^storage, {data_member_spec(^^Ts)...})));
  storage data;

  Tuple(): data{} {}
  Tuple(Ts const& ...vs): data{ vs... } {}
};

consteval std::meta::info get_nth_field(std::meta::info r, std::size_t n) {
  return nonstatic_data_members_of(r)[n];
}

template<std::size_t I, typename... Ts>
constexpr auto get(Tuple<Ts...> &t) noexcept ->
    std::tuple_element_t<I, Tuple<Ts...>>& {
  return t.data.[:get_nth_field(^^decltype(t.data), I):];
}

template<typename... Ts>
struct std::tuple_size<Tuple<Ts...>>
    : public integral_constant<size_t, sizeof...(Ts)> {};

template<std::size_t I, typename... Ts>
struct std::tuple_element<I, Tuple<Ts...>> {
  static constexpr std::array types = {^^Ts...};
  using type = [: types[I] :];
};


int main() {
  Tuple<int, bool, char> t = {3, false, 'c'};

  // RUN: grep "t = <3, false, c>" %t.stdout
  std::println("t = <{}, {}, {}>", get<0>(t), get<1>(t), get<2>(t));
}
