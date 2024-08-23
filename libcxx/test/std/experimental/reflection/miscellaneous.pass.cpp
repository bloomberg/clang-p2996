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
// ADDITIONAL_COMPILE_FLAGS: -fblocks
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
#include <array>
#include <print>
#include <ranges>
#include <tuple>


                        // ============================
                        // alexandrescu_lambda_to_tuple
                        // ============================

namespace alexandrescu_lambda_to_tuple {
consteval auto struct_to_tuple_type(std::meta::info type) -> std::meta::info {
  constexpr auto remove_cvref = [](std::meta::info r) consteval {
    return substitute(^^std::remove_cvref_t, {r});
  };

  return substitute(^^std::tuple,
                    nonstatic_data_members_of(type)
                    | std::views::transform(std::meta::type_of)
                    | std::views::transform(remove_cvref)
                    | std::ranges::to<std::vector>());
}

template <typename To, typename From, std::meta::info... members>
constexpr auto struct_to_tuple_helper(From const& from) -> To {
  return To(from.[:members:]...);
}

template<typename From>
consteval auto get_struct_to_tuple_helper() {
  using To = [: struct_to_tuple_type(^^From) :];

  std::vector args = {^^To, ^^From};
  for (auto mem : nonstatic_data_members_of(^^From)) {
    args.push_back(reflect_value(mem));
  }

  return extract<To(*)(From const&)>(substitute(^^struct_to_tuple_helper,
                                                args));
}

template <typename From>
constexpr auto struct_to_tuple(From const& from) {
  return get_struct_to_tuple_helper<From>()(from);
}

void run_test() {
  struct S { bool i; int j; int k; };

  int a = 1;
  bool b = true;
  [[maybe_unused]] int c = 32;
  std::string d = "hello";
  auto fn = [=]() {
    (void) a; (void) b; (void) d;
  };

  // RUN: grep "Lambda state: <1, true, \"hello\">" %t.stdout
  auto t = struct_to_tuple(fn);
  std::println(R"(Lambda state: <{}, {}, "{}">)",
               get<0>(t), get<1>(t), get<2>(t));
}
}  // namespace alexandrescu_lambda_to_tuple

                           // =======================
                           // pdimov_sorted_type_list
                           // =======================

namespace pdimov_sorted_type_list {
template<class...> struct type_list { };

consteval auto sorted_impl(std::vector<std::meta::info> v)
{
    std::ranges::sort(v, {}, std::meta::alignment_of);
    return v;
}

template<class... Ts> consteval auto sorted()
{
    using R = typename [:substitute(^^type_list, sorted_impl({^^Ts...})):];
    return R{};
}

void run_test() {
  static_assert(typeid(sorted<int, double, char>()) ==
                typeid(type_list<char, int, double>));
}
}  // namespace pdimov_sorted_type_list

                           // =======================
                           // alisdair_universal_swap
                           // =======================

namespace alisdair_universal_swap {
namespace __impl {
template<auto... vals>
struct replicator_type {
  template<typename F>
  constexpr void operator>>(F body) const {
    (body.template operator()<vals>(), ...);
  }
};

template<auto... vals>
replicator_type<vals...> replicator = {};

} // namespace __impl

template<typename R>
consteval auto expand(R range) {
  std::vector<std::meta::info> args;
  for (auto r : range) {
    args.push_back(std::meta::reflect_value(r));
  }
  return substitute(^^__impl::replicator, args);
}

template <typename T>
void do_swap_representations(T& lhs, T& rhs) {
  // This implementation cannot rebind references, and does not handle const
  // data members --- still need to decide whether we support the latter
  if constexpr (std::is_class_v<T>) {
    // This implementation ensures that empty types do nothing
    [: expand(bases_of(^^T)) :] >> [&]<auto base> {
      using Base = [:type_of(base):];
      do_swap_representations<Base>((Base &)lhs, (Base &)rhs);
    };
    [: expand(nonstatic_data_members_of(^^T)) :] >> [&]<auto mem>{
      do_swap_representations<[:type_of(mem):]>(lhs.[:mem:], rhs.[:mem:]);
    };
  } else if constexpr (std::is_array_v<T>) {
    static_assert(0 < std::rank_v<T>, "cannot swap arrays of unknown bound");
    using MemT = std::decay_t<decltype(lhs[0])>;
    [:expand([] {
      std::vector<size_t> result;
      for (size_t idx = 0; idx < std::size(result); ++idx)
        result.push_back(idx);
      return result;
    }()):] >> [&]<size_t Idx> {
      do_swap_representations<MemT>(lhs[Idx], rhs[Idx]);
    };
  } else if constexpr (std::is_scalar_v<T> or std::is_union_v<T>) {
    // correct for unions without tail padding, including swapping active
    // element will need compiler magic to eliminate tail padding though
    // language ensures to not overwrite tail padding for scalars
    // May be broken if union overloads `operator=`
    T intrm = lhs;
    lhs = rhs;
    rhs = intrm;
  } else if constexpr (std::is_reference_v<T>) {
    static_assert(false, "Does not yet rebind references");
  } else {
    static_assert(false, "Unexpected type category");
  }
}

void run_test() {
    std::vector<int> a = {1, 2, 3};
    std::vector<int> b = {3, 2, 1};
    do_swap_representations(a, b);

    // RUN: grep "universal-swap: a = \[3, 2, 1]" %t.stdout
    // RUN: grep "universal-swap: b = \[1, 2, 3]" %t.stdout
    std::println("universal-swap: a = [{}, {}, {}]", a[0], a[1], a[2]);
    std::println("universal-swap: b = [{}, {}, {}]", b[0], b[1], b[2]);
}
}  // namespace alisdair_universal_swap

                       // ==============================
                       // std_apply_with_function_splice
                       // ==============================

namespace std_apply_with_function_splice {
void run_test() {
  struct S {
    static void print(std::string_view sv) {
      std::println("std::apply: {}", sv);
    }
  };

  // RUN: grep "std::apply: Hello, world!" %t.stdout
  std::apply([:^^S::print:], std::tuple {"Hello, world!"});
}
}  // namespace std_apply_with_function_splice

                 // ==========================================
                 // array_with_default_initialized_reflections
                 // ==========================================

namespace array_with_default_initialized_reflections {
consteval auto fn() {
    std::array<std::meta::info, 3> arr = {};
    arr[0] = ^^int;

    return arr;
}

[[maybe_unused]] constexpr auto rs = fn();
}  // namespace array_with_default_initialized_reflections

                           // ======================
                           // compatible_with_blocks
                           // ======================

namespace compatible_with_blocks {
constexpr auto block = std::meta::reflect_value(^int() { return 4; });
static_assert(type_of(block) == ^^int(^)());

void run_test() {
  // RUN: grep "block result: 4" %t.stdout
  std::println("block result: {}", [:block:]());
}
}  // namespace compatible_with_blocks


template <std::meta::info... Tests>
void run_tests() {
  (..., [:Tests:]::run_test());
}

int main() {
  run_tests<
      ^^alexandrescu_lambda_to_tuple,
      ^^pdimov_sorted_type_list,
      ^^alisdair_universal_swap,
      ^^std_apply_with_function_splice,
      ^^compatible_with_blocks
  >();
}
