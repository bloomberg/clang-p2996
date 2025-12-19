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

// <experimental/reflection>
//
// [reflection]

#include <meta>


struct C {};
struct Child : C {};
union U {
  U &operator=(const C&);
  U &operator=(C&&);
};
enum class E : short {};

class AC { virtual void fn() = 0; };
class FC final {};

struct WithCtor { WithCtor(int, bool) noexcept {} };

struct Triv { int i; bool b; };

class VDtor { virtual ~VDtor() {} };

int fn(bool, char) noexcept { return 0; }

static_assert(is_void_type(^^void));
static_assert(is_null_pointer_type(^^nullptr_t));
static_assert(is_integral_type(^^int));
static_assert(is_floating_point_type(^^float));
static_assert(is_array_type(^^int[]));
static_assert(is_pointer_type(^^int *));
static_assert(is_lvalue_reference_type(^^int&));
static_assert(is_rvalue_reference_type(^^int&&));
static_assert(is_member_object_pointer_type(^^int (C::*)));
static_assert(is_member_function_pointer_type(^^int (C::*)()));
static_assert(is_enum_type(^^E));
static_assert(is_class_type(^^C));
static_assert(is_function_type(^^void()));
static_assert(is_reflection_type(^^std::meta::info));

static_assert(is_reference_type(^^int&));
static_assert(is_arithmetic_type(^^int));
static_assert(is_fundamental_type(^^int));
static_assert(is_object_type(^^C));
static_assert(is_scalar_type(^^int));
static_assert(is_compound_type(^^C));
static_assert(is_member_pointer_type(^^int (C::*)));

static_assert(is_const_type(^^const int));
static_assert(is_volatile_type(^^volatile int));
static_assert(is_trivial_type(^^int));
static_assert(is_trivially_copyable_type(^^C));
static_assert(is_standard_layout_type(^^int));
static_assert(is_empty_type(^^C));
static_assert(is_polymorphic_type(^^AC));
static_assert(is_abstract_type(^^AC));
static_assert(is_final_type(^^FC));
static_assert(is_aggregate_type(^^C));
static_assert(is_consteval_only_type(^^std::meta::info));
static_assert(is_signed_type(^^int));
static_assert(is_unsigned_type(^^unsigned));;
static_assert(is_bounded_array_type(^^int[3]));
static_assert(is_unbounded_array_type(^^int[]));
static_assert(is_scoped_enum_type(^^E));

static_assert(is_constructible_type(^^WithCtor, {^^int, ^^bool}));
static_assert(is_default_constructible_type(^^C));
static_assert(is_copy_constructible_type(^^C));
static_assert(is_move_constructible_type(^^C));

static_assert(is_assignable_type(^^U, ^^C));
static_assert(is_copy_assignable_type(^^U));
static_assert(is_move_assignable_type(^^U));

static_assert(is_swappable_with_type(^^int&, ^^int&));
static_assert(is_swappable_type(^^int));

static_assert(is_destructible_type(^^C));

static_assert(is_trivially_constructible_type(^^Triv, {^^int, ^^bool}));
static_assert(is_trivially_default_constructible_type(^^C));
static_assert(is_trivially_copy_constructible_type(^^C));
static_assert(is_trivially_move_constructible_type(^^C));

static_assert(is_trivially_assignable_type(^^C, ^^const C&));
static_assert(is_trivially_copy_assignable_type(^^C));
static_assert(is_trivially_move_assignable_type(^^C));
static_assert(is_trivially_destructible_type(^^C));

static_assert(is_nothrow_constructible_type(^^WithCtor, {^^int, ^^bool}));
static_assert(is_nothrow_default_constructible_type(^^C));
static_assert(is_nothrow_copy_constructible_type(^^C));
static_assert(is_nothrow_move_constructible_type(^^C));

static_assert(is_nothrow_assignable_type(^^C, ^^const C&));
static_assert(is_nothrow_copy_assignable_type(^^C));
static_assert(is_nothrow_move_assignable_type(^^C));

static_assert(is_nothrow_swappable_with_type(^^int&, ^^int&));
static_assert(is_nothrow_swappable_type(^^int));

static_assert(is_nothrow_destructible_type(^^C));

static_assert(has_virtual_destructor(^^VDtor));

static_assert(has_unique_object_representations(^^int));

static_assert(rank(^^int[4][2]) == 2);
static_assert(extent(^^int[4][2], 1) == 2);

static_assert(is_same_type(^^int, ^^int));
static_assert(is_base_of_type(^^C, ^^Child));
static_assert(is_convertible_type(^^bool, ^^int));
static_assert(is_nothrow_convertible_type(^^bool, ^^int));

static_assert(is_invocable_type(type_of(^^fn), {^^bool, ^^char}));
static_assert(is_invocable_r_type(^^int, type_of(^^fn), {^^bool, ^^char}));

static_assert(is_nothrow_invocable_type(type_of(^^fn), {^^bool, ^^char}));
static_assert(is_nothrow_invocable_r_type(^^int, type_of(^^fn),
                                          {^^bool, ^^char}));

static_assert(remove_const(^^const int) == ^^int);
static_assert(display_string_of(remove_const(^^const int)) ==
              display_string_of(^^int));
static_assert(remove_volatile(^^volatile int) == ^^int);
static_assert(display_string_of(remove_volatile(^^volatile int)) ==
              display_string_of(^^int));
static_assert(remove_cv(^^const volatile int) == ^^int);
static_assert(display_string_of(remove_cv(^^const volatile int)) ==
              display_string_of(^^int));
static_assert(add_const(^^int) == ^^const int);
static_assert(display_string_of(add_const(^^int)) ==
              display_string_of(^^const int));
static_assert(add_volatile(^^int) == ^^volatile int);
static_assert(display_string_of(add_volatile(^^int)) ==
              display_string_of(^^volatile int));
static_assert(add_cv(^^int) == ^^const volatile int);
static_assert(display_string_of(add_cv(^^int)) ==
              display_string_of(^^const volatile int));

static_assert(remove_reference(^^int&&) == ^^int);
static_assert(display_string_of(remove_reference(^^int&&)) ==
              display_string_of(^^int));
static_assert(add_lvalue_reference(^^int) == ^^int&);
static_assert(display_string_of(add_lvalue_reference(^^int)) ==
              display_string_of(^^int&));
static_assert(add_rvalue_reference(^^int) == ^^int&&);
static_assert(display_string_of(add_rvalue_reference(^^int)) ==
              display_string_of(^^int&&));

static_assert(make_signed(^^unsigned) == ^^int);
static_assert(display_string_of(make_signed(^^unsigned)) ==
              display_string_of(^^int));
static_assert(make_unsigned(^^int) == ^^unsigned);
static_assert(display_string_of(make_unsigned(^^int)) ==
              display_string_of(^^unsigned));

static_assert(remove_extent(^^int[2][3]) == ^^int[3]);
static_assert(display_string_of(remove_extent(^^int[2][3])) ==
              display_string_of(^^int[3]));
static_assert(remove_all_extents(^^int[2][3]) == ^^int);
static_assert(display_string_of(remove_all_extents(^^int[2][3])) ==
              display_string_of(^^int));

static_assert(remove_pointer(^^int **) == ^^int *);
static_assert(display_string_of(remove_pointer(^^int **)) ==
              display_string_of(^^int *));
static_assert(add_pointer(^^int *) == ^^int **);
static_assert(display_string_of(add_pointer(^^int *)) ==
              display_string_of(^^int **));

static_assert(remove_cvref(^^const volatile int &&) == ^^int);
static_assert(display_string_of(remove_cvref(^^const volatile int &&)) ==
              display_string_of(^^int));
static_assert(decay(^^int[]) == ^^int *);
static_assert(display_string_of(decay(^^int[])) ==
              display_string_of(^^int *));
static_assert(std::meta::common_type({^^int, ^^short, ^^bool}) == ^^int);
static_assert(display_string_of(std::meta::common_type({^^int,
                                                        ^^short,
                                                        ^^bool})) ==
              display_string_of(^^int));
static_assert(std::meta::common_reference({^^int &, ^^const int &}) ==
              ^^const int &);
static_assert(display_string_of(std::meta::common_reference({^^int &,
                                                        ^^const int &})) ==
              display_string_of(^^const int &));
static_assert(underlying_type(^^E) == ^^short);
static_assert(display_string_of(underlying_type(^^E)) ==
              display_string_of(^^short));
static_assert(invoke_result(type_of(^^fn), {^^bool, ^^char}) == ^^int);
static_assert(display_string_of(invoke_result(type_of(^^fn),
                                              {^^bool, ^^char})) ==
              display_string_of(^^int));
static_assert(unwrap_reference(^^std::reference_wrapper<int>) == ^^int &);
static_assert(display_string_of(
                    unwrap_reference(^^std::reference_wrapper<int>)) ==
              display_string_of(^^int &));
static_assert(unwrap_ref_decay(^^std::reference_wrapper<const int>) ==
                                    ^^const int &);
static_assert(
    display_string_of(unwrap_ref_decay(^^std::reference_wrapper<const int>)) ==
    display_string_of(^^const int &));

using Tup = std::tuple<int, bool, char>;
static_assert(tuple_size(^^Tup) == 3);
static_assert(tuple_element(1, ^^Tup) == ^^bool);

using Var = std::variant<int, bool, char, int *>;
static_assert(variant_size(^^Var) == 4);
static_assert(variant_alternative(3, ^^Var) == ^^int *);

 struct X { int x; };
 struct Y { private: [[maybe_unused]] int x; };
 template <class T> struct TCls { T t; };
 static_assert(is_structural_type(^^X));
 static_assert(!is_structural_type(^^Y));
 static_assert(is_structural_type(^^TCls<X>));
 static_assert(!is_structural_type(^^TCls<Y>));

int main() { }
