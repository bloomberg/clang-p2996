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

#include <experimental/meta>


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

static_assert(type_is_void(^^void));
static_assert(type_is_null_pointer(^^nullptr_t));
static_assert(type_is_integral(^^int));
static_assert(type_is_floating_point(^^float));
static_assert(type_is_array(^^int[]));
static_assert(type_is_pointer(^^int *));
static_assert(type_is_lvalue_reference(^^int&));
static_assert(type_is_rvalue_reference(^^int&&));
static_assert(type_is_member_object_pointer(^^int (C::*)));
static_assert(type_is_member_function_pointer(^^int (C::*)()));
static_assert(type_is_enum(^^E));
static_assert(type_is_class(^^C));
static_assert(type_is_function(^^void()));

static_assert(type_is_reference(^^int&));
static_assert(type_is_arithmetic(^^int));
static_assert(type_is_fundamental(^^int));
static_assert(type_is_object(^^C));
static_assert(type_is_scalar(^^int));
static_assert(type_is_compound(^^C));
static_assert(type_is_member_pointer(^^int (C::*)));

static_assert(type_is_const(^^const int));
static_assert(type_is_volatile(^^volatile int));
static_assert(type_is_trivial(^^int));
static_assert(type_is_trivially_copyable(^^C));
static_assert(type_is_standard_layout(^^int));
static_assert(type_is_empty(^^C));
static_assert(type_is_polymorphic(^^AC));
static_assert(type_is_abstract(^^AC));
static_assert(type_is_final(^^FC));
static_assert(type_is_aggregate(^^C));
static_assert(type_is_signed(^^int));
static_assert(type_is_unsigned(^^unsigned));;
static_assert(type_is_bounded_array(^^int[3]));
static_assert(type_is_unbounded_array(^^int[]));
static_assert(type_is_scoped_enum(^^E));

static_assert(type_is_constructible(^^WithCtor, {^^int, ^^bool}));
static_assert(type_is_default_constructible(^^C));
static_assert(type_is_copy_constructible(^^C));
static_assert(type_is_move_constructible(^^C));

static_assert(type_is_assignable(^^U, ^^C));
static_assert(type_is_copy_assignable(^^U));
static_assert(type_is_move_assignable(^^U));

static_assert(type_is_swappable_with(^^int&, ^^int&));
static_assert(type_is_swappable(^^int));

static_assert(type_is_destructible(^^C));

static_assert(type_is_trivially_constructible(^^Triv, {^^int, ^^bool}));
static_assert(type_is_trivially_default_constructible(^^C));
static_assert(type_is_trivially_copy_constructible(^^C));
static_assert(type_is_trivially_move_constructible(^^C));

static_assert(type_is_trivially_assignable(^^C, ^^const C&));
static_assert(type_is_trivially_copy_assignable(^^C));
static_assert(type_is_trivially_move_assignable(^^C));
static_assert(type_is_trivially_destructible(^^C));

static_assert(type_is_nothrow_constructible(^^WithCtor, {^^int, ^^bool}));
static_assert(type_is_nothrow_default_constructible(^^C));
static_assert(type_is_nothrow_copy_constructible(^^C));
static_assert(type_is_nothrow_move_constructible(^^C));

static_assert(type_is_nothrow_assignable(^^C, ^^const C&));
static_assert(type_is_nothrow_copy_assignable(^^C));
static_assert(type_is_nothrow_move_assignable(^^C));

static_assert(type_is_nothrow_swappable_with(^^int&, ^^int&));
static_assert(type_is_nothrow_swappable(^^int));

static_assert(type_is_nothrow_destructible(^^C));

static_assert(type_has_virtual_destructor(^^VDtor));

static_assert(type_has_unique_object_representations(^^int));

static_assert(type_alignment_of(^^int) == sizeof(int));
static_assert(type_rank(^^int[4][2]) == 2);
static_assert(type_extent(^^int[4][2], 1) == 2);

static_assert(type_is_same(^^int, ^^int));
static_assert(type_is_base_of(^^C, ^^Child));
static_assert(type_is_convertible(^^bool, ^^int));
static_assert(type_is_nothrow_convertible(^^bool, ^^int));

static_assert(type_is_invocable(type_of(^^fn), {^^bool, ^^char}));
static_assert(type_is_invocable_r(^^int, type_of(^^fn), {^^bool, ^^char}));

static_assert(type_is_nothrow_invocable(type_of(^^fn), {^^bool, ^^char}));
static_assert(type_is_nothrow_invocable_r(^^int, type_of(^^fn),
                                          {^^bool, ^^char}));

static_assert(type_remove_const(^^const int) == ^^int);
static_assert(display_string_of(type_remove_const(^^const int)) ==
              display_string_of(^^int));
static_assert(type_remove_volatile(^^volatile int) == ^^int);
static_assert(display_string_of(type_remove_volatile(^^volatile int)) ==
              display_string_of(^^int));
static_assert(type_remove_cv(^^const volatile int) == ^^int);
static_assert(display_string_of(type_remove_cv(^^const volatile int)) ==
              display_string_of(^^int));
static_assert(type_add_const(^^int) == ^^const int);
static_assert(display_string_of(type_add_const(^^int)) ==
              display_string_of(^^const int));
static_assert(type_add_volatile(^^int) == ^^volatile int);
static_assert(display_string_of(type_add_volatile(^^int)) ==
              display_string_of(^^volatile int));
static_assert(type_add_cv(^^int) == ^^const volatile int);
static_assert(display_string_of(type_add_cv(^^int)) ==
              display_string_of(^^const volatile int));

static_assert(type_remove_reference(^^int&&) == ^^int);
static_assert(display_string_of(type_remove_reference(^^int&&)) ==
              display_string_of(^^int));
static_assert(type_add_lvalue_reference(^^int) == ^^int&);
static_assert(display_string_of(type_add_lvalue_reference(^^int)) ==
              display_string_of(^^int&));
static_assert(type_add_rvalue_reference(^^int) == ^^int&&);
static_assert(display_string_of(type_add_rvalue_reference(^^int)) ==
              display_string_of(^^int&&));

static_assert(type_make_signed(^^unsigned) == ^^int);
static_assert(display_string_of(type_make_signed(^^unsigned)) ==
              display_string_of(^^int));
static_assert(type_make_unsigned(^^int) == ^^unsigned);
static_assert(display_string_of(type_make_unsigned(^^int)) ==
              display_string_of(^^unsigned));

static_assert(type_remove_extent(^^int[2][3]) == ^^int[3]);
static_assert(display_string_of(type_remove_extent(^^int[2][3])) ==
              display_string_of(^^int[3]));
static_assert(type_remove_all_extents(^^int[2][3]) == ^^int);
static_assert(display_string_of(type_remove_all_extents(^^int[2][3])) ==
              display_string_of(^^int));

static_assert(type_remove_pointer(^^int **) == ^^int *);
static_assert(display_string_of(type_remove_pointer(^^int **)) ==
              display_string_of(^^int *));
static_assert(type_add_pointer(^^int *) == ^^int **);
static_assert(display_string_of(type_add_pointer(^^int *)) ==
              display_string_of(^^int **));

static_assert(type_remove_cvref(^^const volatile int &&) == ^^int);
static_assert(display_string_of(type_remove_cvref(^^const volatile int &&)) ==
              display_string_of(^^int));
static_assert(type_decay(^^int[]) == ^^int *);
static_assert(display_string_of(type_decay(^^int[])) ==
              display_string_of(^^int *));
static_assert(std::meta::type_common_type({^^int, ^^short, ^^bool}) == ^^int);
static_assert(display_string_of(std::meta::type_common_type({^^int,
                                                             ^^short,
                                                             ^^bool})) ==
              display_string_of(^^int));
static_assert(std::meta::type_common_reference({^^int &, ^^const int &}) ==
              ^^const int &);
static_assert(display_string_of(std::meta::type_common_reference({^^int &,
                                                        ^^const int &})) ==
              display_string_of(^^const int &));
static_assert(type_underlying_type(^^E) == ^^short);
static_assert(display_string_of(type_underlying_type(^^E)) ==
              display_string_of(^^short));
static_assert(type_invoke_result(type_of(^^fn), {^^bool, ^^char}) == ^^int);
static_assert(display_string_of(type_invoke_result(type_of(^^fn),
                                                   {^^bool, ^^char})) ==
              display_string_of(^^int));
static_assert(type_unwrap_reference(^^std::reference_wrapper<int>) == ^^int &);
static_assert(display_string_of(
                    type_unwrap_reference(^^std::reference_wrapper<int>)) ==
              display_string_of(^^int &));
static_assert(type_unwrap_ref_decay(^^std::reference_wrapper<const int>) ==
                                    ^^const int &);
static_assert(
    display_string_of(
          type_unwrap_ref_decay(^^std::reference_wrapper<const int>)) ==
    display_string_of(^^const int &));


int main() { }
