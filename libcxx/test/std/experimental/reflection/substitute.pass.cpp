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
// ADDITIONAL_COMPILE_FLAGS: -Wno-unneeded-internal-declaration

// <experimental/reflection>
//
// [reflection]

#include <experimental/meta>

#include <queue>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

template <int K>
constexpr std::meta::info RVal = std::meta::reflect_value(K);

                               // ===============
                               // class_templates
                               // ===============

namespace class_templates {
// Handles all template argument kinds.
template <typename T, auto V, template <typename, size_t> class C>
struct Cls;
static_assert(can_substitute(^^Cls, {^^int, RVal<1>, ^^std::array}));
[[maybe_unused]] constexpr auto obj1 = substitute(^^Cls,
                                                  {^^int, RVal<1>,
                                                   ^^std::array});
static_assert(!is_complete_type(^^Cls<int, 1, std::array>));

template <typename T, auto V, template <typename, size_t> class C>
struct Cls {};
static_assert(is_complete_type(^^Cls<int, 1, std::array>));

// Template arguments are dependent.
template <typename T, auto V, template <typename, size_t> class C>
consteval auto makeCls() {
  static_assert(can_substitute(^^Cls, {^^T, RVal<V>, ^^C}));
  return typename [:substitute(^^Cls, {^^T, RVal<V>, ^^C}):]{};
}
[[maybe_unused]] constexpr auto obj2 = makeCls<int, 2, std::array>();

// With a dependent type parameter pack.
template <typename... Ts>
consteval auto makeTuple(Ts... vs) {
  static_assert(can_substitute(^^std::tuple, {^^Ts...}));
  typename [:substitute(^^std::tuple, {^^Ts...}):] tup { vs... };
  return tup;
}
[[maybe_unused]] constexpr auto tup1 = makeTuple<int, bool, char>(4, true, 'f');
static_assert(get<2>(tup1) == 'f');

// With a dependent non-type parameter pack.
template <auto... Vs>
consteval auto returnNumElems() {
  static_assert(can_substitute(^^std::integer_sequence, {^^int, RVal<Vs>...}));
  typename [:substitute(^^std::integer_sequence, {^^int, RVal<Vs>...}):] seq;
  return seq.size();
}
static_assert(returnNumElems<1, 3, 5>() == 3);

// With a dependent template template parameter pack.
template <template <typename...> class... Cs>
struct SizeOfContainersOfInts {
  static constexpr size_t Sz = (... + sizeof(Cs<int>));
};
template <template <typename...> class... Cs>
consteval auto FindSize() {
  static_assert(can_substitute(^^SizeOfContainersOfInts, {^^Cs...}));
  return [:substitute(^^SizeOfContainersOfInts, {^^Cs...}):]::Sz;
}
static_assert(FindSize<std::vector, std::queue>() ==
              sizeof(std::vector<int>) + sizeof(std::queue<int>));
}  // namespace class_templates

                             // ==================
                             // function_templates
                             // ==================

namespace function_templates {
//  Handles all template argument kinds.
template <template <typename, size_t> class C, typename T, size_t V>
consteval size_t getContainerSize() {
  return sizeof(C<T, V>);
}
static_assert(can_substitute(^^getContainerSize,
                             {^^std::array, ^^int, RVal<4>}));
static_assert([:substitute(^^getContainerSize,
                           {^^std::array, ^^int, RVal<4>}):]() ==
              sizeof(std::array<int, 4>));

// Template arguments are dependent.
template <template <typename, size_t> class C, typename T, size_t V>
consteval auto dependentContainerSize() {
  static_assert(can_substitute(^^getContainerSize, {^^C, ^^T, RVal<V>}));
  return [:substitute(^^getContainerSize, {^^C, ^^T, RVal<V>}):]();
}
static_assert(dependentContainerSize<std::array, int, 4>() ==
              sizeof(std::array<int, 4>));

// With a dependent type parameter pack.
template <typename... Ts>
consteval size_t getTotalSize() { return (... + sizeof(Ts)); }
template <typename... Ts>
consteval size_t dependentTotalSize() {
  static_assert(can_substitute(^^getTotalSize, {^^Ts...}));
  return [:substitute(^^getTotalSize, {^^Ts...}):]();
}
static_assert(dependentTotalSize<int, bool, char>() ==
              sizeof(int) + sizeof(bool) + sizeof(char));

// With a dependent non-type parameter pack.
template <int... Vs>
consteval int sum() { return (... + Vs); }
template <int... Vs>
consteval int dependentSum() {
  static_assert(can_substitute(^^sum, {RVal<Vs>...}));
  return [:substitute(^^sum, {RVal<Vs>...}):]();
}
static_assert(dependentSum<1, 3, 5, 7>() == 16);

// With a dependent template template parameter pack.
template <template <typename...> class... Cs>
consteval size_t countArgs() { return sizeof...(Cs); }
template <template <typename...> class... Cs>
consteval size_t dependentCountArgs() {
  static_assert(can_substitute(^^countArgs, {^^Cs...}));
  return [:substitute(^^countArgs, {^^Cs...}):]();
}
static_assert(dependentCountArgs<std::vector, std::queue, std::vector>() == 3);
}  // namespace function_templates

                          // =========================
                          // member_function_templates
                          // =========================

namespace member_function_templates {
// Handles all template argument kinds.
struct S {
  size_t Sz;

  template <template <typename, size_t> class C, typename T, size_t V>
  consteval bool IsContainerLargerThanSz() const {
    return sizeof(C<T, V>) > Sz;
  }
};
constexpr S s(2);
static_assert(can_substitute(^^S::IsContainerLargerThanSz,
                             {^^std::array, ^^char, RVal<3>}));
static_assert(s.[:substitute(^^S::IsContainerLargerThanSz,
                             {^^std::array, ^^char, RVal<3>}):]());
static_assert(can_substitute(^^S::IsContainerLargerThanSz,
                             {^^std::array, ^^char, RVal<1>}));
static_assert(!s.[:substitute(^^S::IsContainerLargerThanSz,
                              {^^std::array, ^^char, RVal<1>}):]());
}  // namespace member_function_templates

                             // ==================
                             // variable_templates
                             // ==================

namespace variable_templates {
// Handles all template kinds.
template <template <typename, size_t> class C, typename T, size_t V>
constexpr size_t ArrSz = sizeof(C<T, V>);
static_assert(can_substitute(^^ArrSz, {^^std::array, ^^int, RVal<3>}));
static_assert([:substitute(^^ArrSz, {^^std::array, ^^int, RVal<3>}):] ==
              3 * sizeof(int));

// Template arguments are dependent.
template <template <typename, size_t> class C, typename T, size_t V>
consteval size_t dependentArrSz() {
  static_assert(can_substitute(^^ArrSz, {^^C, ^^T, RVal<V>}));
  return [:substitute(^^ArrSz, {^^C, ^^T, RVal<V>}):];
};
static_assert(dependentArrSz<std::array, bool, 5>() ==
              sizeof(std::array<bool, 5>));

// With a dependent type parameter pack.
template <typename...Ts>
constexpr size_t CountTypes = sizeof...(Ts);
static_assert(can_substitute(^^CountTypes, {^^int, ^^int, ^^bool}));
static_assert([:substitute(^^CountTypes, {^^int, ^^int, ^^bool}):] == 3);

// With a dependent non-type parameter pack.
template <int... Vs>
constexpr int Sum = (... + Vs);
template <int... Vs>
consteval int dependentSum() { return Sum<Vs...>; }
static_assert(can_substitute(^^dependentSum,
                             {RVal<1>, RVal<3>, RVal<5>, RVal<7>}));
static_assert([:substitute(^^dependentSum,
                           {RVal<1>, RVal<3>, RVal<5>, RVal<7>}):]() == 16);

// With a dependent template template parameter pack.
template <template <typename...> class... Cs>
constexpr int CountContainers = sizeof...(Cs);
template <template <typename...> class... Cs>
consteval int dependentCountContainers() { return CountContainers<Cs...>; }
static_assert(can_substitute(^^dependentCountContainers,
                             {^^std::vector, ^^std::queue}));
static_assert([:substitute(^^dependentCountContainers,
                           {^^std::vector, ^^std::queue}):]() == 2);
}  // namespace variable_templates

                               // ===============
                               // alias_templates
                               // ===============

namespace alias_templates {
// Handles all template kinds.
template <template <typename, size_t> class C, typename T, size_t V>
using Alias1 = C<T, V>;
static_assert(^^Alias1<std::array, int, 5> != ^^std::array<int, 5>);
static_assert(can_substitute(^^Alias1, {^^std::array, ^^int, RVal<5>}));
static_assert(substitute(^^Alias1, {^^std::array, ^^int, RVal<5>}) !=
              ^^std::array<int, 5>);
static_assert(dealias(substitute(^^Alias1, {^^std::array, ^^int, RVal<5>})) ==
              ^^std::array<int, 5>);

// Template arguments are dependent.
template <std::meta::info A, template <typename, size_t> class C, typename T,
          size_t V>
consteval size_t dependentSz() {
  static_assert(can_substitute(A, {^^C, ^^T, RVal<V>}));
  return sizeof(typename [:substitute(A, {^^C, ^^T, RVal<V>}):]);
}
static_assert(dependentSz<^^Alias1, std::array, int, 3>() ==
              sizeof(std::array<int, 3>));

// With a dependent type parameter pack.
template <typename... Ts>
using Alias2 = std::tuple<Ts...>;
template <typename... Ts>
consteval size_t dependentTupleSz() {
  static_assert(can_substitute(^^Alias2, {^^Ts...}));
  return sizeof(typename [:substitute(^^Alias2, {^^Ts...}):]);
}
static_assert(dependentTupleSz<int, bool, char>() ==
              sizeof(std::tuple<int, bool, char>));

// With a dependent non-type parameter pack.
template <int... Vs>
using Alias3 = std::tuple<decltype(Vs)...>;
template <int... Vs>
consteval size_t getLastValue() {
  static_assert(can_substitute(^^Alias3, {RVal<Vs>...}));
  typename [:substitute(^^Alias3, {RVal<Vs>...}):] values = {Vs...};
  return get<sizeof...(Vs) - 1>(values);
}
static_assert(getLastValue<1, 2, 3, 4>() == 4);

// With a dependent template template parameter pack.
template <template <typename...> class... Cs>
using Alias4 = std::tuple<Cs<int>...>;
template <template <typename...> class... Cs>
consteval size_t getContainerTupleSz() {
  static_assert(can_substitute(^^Alias4, {^^Cs...}));
  return sizeof(typename [:substitute(^^Alias4, {^^Cs...}):]);
}
static_assert(getContainerTupleSz<std::vector, std::queue, std::queue>() ==
              sizeof(std::tuple<std::vector<int>, std::queue<int>,
                                std::queue<int>>));
}  // namespace alias_templates

                                  // ========
                                  // concepts
                                  // ========

namespace concepts {
// Handles all template kinds.
template <template <typename, size_t> class C, typename T, size_t V>
concept LargerThanInt = requires { requires sizeof(C<T, V>) > sizeof(int); };
static_assert(can_substitute(^^LargerThanInt, {^^std::array, ^^int, RVal<2>}));
static_assert([:substitute(^^LargerThanInt, {^^std::array, ^^int, RVal<2>}):]);
static_assert(can_substitute(^^LargerThanInt, {^^std::array, ^^int, RVal<1>}));
static_assert(![:substitute(^^LargerThanInt,
                            {^^std::array, ^^char, RVal<1>}):]);

// Template arguments are dependent.
template <template <typename, size_t> class C, typename T, size_t V>
consteval bool isContainerLargerThanInt() {
  static_assert(can_substitute(^^LargerThanInt, {^^C, ^^T, RVal<V>}));
  return [:substitute(^^LargerThanInt, {^^C, ^^T, RVal<V>}):];
}
static_assert(isContainerLargerThanInt<std::array, int, 2>());
static_assert(!isContainerLargerThanInt<std::array, int, 1>());

// With a dependent type parameter pack.
template <typename... Ts>
concept AreAnyLargerThanInt = requires {
  requires (... || (sizeof(Ts) > sizeof(int)));
};
template <typename... Ts>
consteval bool dependentAnyLargerThanInt() {
  static_assert(can_substitute(^^AreAnyLargerThanInt, {^^Ts...}));
  return [:substitute(^^AreAnyLargerThanInt, {^^Ts...}):];
}
static_assert(dependentAnyLargerThanInt<bool, std::array<int, 2>, char>());
static_assert(!dependentAnyLargerThanInt<bool, short, char>());

// With a dependent non-type parameter pack.
template <int... Vs>
concept SumGreaterThan10 = requires { requires (... + Vs) > 10; };
template <int... Vs>
consteval int dependentSumGreaterThan10() {
  static_assert(can_substitute(^^SumGreaterThan10, {RVal<Vs>...}));
  return [:substitute(^^SumGreaterThan10, {RVal<Vs>...}):];
}
static_assert(!dependentSumGreaterThan10<1, 2, 3, 4>());
static_assert(dependentSumGreaterThan10<1, 2, 3, 4, 5>());

// With a dependent template template parameter pack.
template <template <typename...> class... Cs>
concept AtLeastThree = requires { requires sizeof...(Cs) >= 3; };
template <template <typename...> class... Cs>
consteval bool dependentAtLeastThree() {
  static_assert(can_substitute(^^AtLeastThree, {^^Cs...}));
  return [:substitute(^^AtLeastThree, {^^Cs...}):];
}
static_assert(!dependentAtLeastThree<std::vector, std::queue>());
static_assert(dependentAtLeastThree<std::vector, std::queue, std::vector>());
} // namespace concepts

                // ============================================
                // equality_respects_default_template_arguments
                // ============================================

namespace equality_respects_default_template_arguments {
// With a dependent template template parameter pack.
template <template <typename...> class... Cs>
using Alias = [:substitute(^^std::tuple, {substitute(^^Cs, {^^int})...}):];
static_assert(dealias(^^Alias<std::queue, std::vector>) ==
              ^^std::tuple<std::queue<int>, std::vector<int>>);
}  // namespace equality_respects_default_template_arguments 

                         // ==========================
                         // with_template_arguments_of
                         // ==========================

namespace with_template_arguments_of {
template <typename T, int = 3> struct Cls {};
template <typename T, int = 3> using Alias = int;

static_assert(template_arguments_of(substitute(^^Cls, {^^void})) ==
              std::vector{^^void, RVal<3>});
static_assert(template_arguments_of(substitute(^^Alias, {^^void})) ==
              std::vector{^^void, RVal<3>});
}  // namespace with_template_arguments_of

                       // ==============================
                       // with_reflection_of_declaration
                       // ==============================

namespace with_reflection_of_declaration {
template <int V> constexpr int Plus1 = V + 1;
struct S { static constexpr int val = 4; };

static_assert([:substitute(^^Plus1, {static_data_members_of(^^S)[0]}):] == 5);
}  // namespace with_reflection_of_declaration

                         // ==========================
                         // with_non_contiguous_ranges
                         // ==========================

namespace with_non_contiguous_ranges {
template <char... Is> consteval std::string join() {
  return std::string{Is...};
}
static_assert(
    [:substitute(^^join, "Hello, world!" |
                        std::views::filter([](char c) {
                          return (c >= 'A' && c <= 'Z') ||
                                 (c >= 'a' && c <= 'z') || c == ' ';
                        }) |
                        std::views::transform(std::meta::reflect_value<char>))
    :]() == "Hello world");

static_assert(!can_substitute(^^join,
                              std::vector {^^int, ^^std::array, ^^bool} |
                                   std::views::filter(std::meta::is_type)));
}  // namespace with_non_contiguous_ranges

                             // ===================
                             // with_reflect_object
                             // ===================

namespace with_reflect_object {
template <int &> void fn();

int p[2];
static_assert(&[:substitute(^^fn, {std::meta::reflect_object(p[1])}):] ==
              &fn<p[1]>);

}  // namespace with_reflect_object

                            // ====================
                            // invalid_template_ids
                            // ====================

namespace invalid_template_ids {
template <typename T, auto V, template <typename, size_t> class C>
struct Cls {};

static_assert(!can_substitute(^^Cls, {}));
static_assert(!can_substitute(^^Cls, {RVal<3>, RVal<2>, ^^std::array}));
static_assert(!can_substitute(^^Cls, {^^int, ^^bool, ^^std::array}));
static_assert(!can_substitute(^^Cls, {^^int, ^^bool, ^^bool}));
static_assert(!can_substitute(^^Cls,
                              {^^int, ^^bool, ^^std::array, ^^std::array}));
}  // namespace invalid_template_ids

int main() { }
