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
// ADDITIONAL_COMPILE_FLAGS: -fparameter-reflection

// <experimental/reflection>
//
// [reflection]

#include <experimental/meta>


                              // =================
                              // with_no_arguments
                              // =================

namespace with_no_arguments {
void fn();

template <typename... Ts>
void tfn(Ts...);

static_assert(parameters_of(^^fn).size() == 0);
static_assert(parameters_of(^^tfn<>).size() == 0);
static_assert(!has_ellipsis_parameter(^^tfn<>));
static_assert(!has_ellipsis_parameter(type_of(^^tfn<>)));
static_assert(return_type_of(^^fn) == ^^void);
static_assert(return_type_of(^^tfn<>) == ^^void);
}  // namespace with_no_arguments

                            // ====================
                            // with_fixed_arguments
                            // ====================

namespace with_fixed_arguments {
template <typename T>
struct S {};

void fn(int a, bool b, const S<int> &s);
static_assert(parameters_of(^^fn).size() == 3);
static_assert(parameters_of(type_of(^^fn)) ==
              std::vector {^^int, ^^bool, ^^const S<int> &});
static_assert(type_of(parameters_of(^^fn)[0]) == ^^int);
static_assert(identifier_of(parameters_of(^^fn)[0]) == "a");
static_assert(has_consistent_identifier(parameters_of(^^fn)[0]));
static_assert(!has_default_argument(parameters_of(^^fn)[0]));
static_assert(!is_explicit_object_parameter(parameters_of(^^fn)[0]));
static_assert(type_of(parameters_of(^^fn)[1]) == ^^bool);
static_assert(identifier_of(parameters_of(^^fn)[1]) == "b");
static_assert(has_consistent_identifier(parameters_of(^^fn)[1]));
static_assert(!has_default_argument(parameters_of(^^fn)[1]));
static_assert(!is_explicit_object_parameter(parameters_of(^^fn)[1]));
static_assert(type_of(parameters_of(^^fn)[2]) == ^^const S<int> &);
static_assert(identifier_of(parameters_of(^^fn)[2]) == "s");
static_assert(has_consistent_identifier(parameters_of(^^fn)[2]));
static_assert(!has_default_argument(parameters_of(^^fn)[2]));
static_assert(!is_explicit_object_parameter(parameters_of(^^fn)[2]));
static_assert(!has_ellipsis_parameter(^^fn));
static_assert(!has_ellipsis_parameter(type_of(^^fn)));
static_assert(return_type_of(^^fn) == ^^void);
}  // namespace with_fixed_arguments

                            // =====================
                            // with_member_functions
                            // =====================

namespace with_member_functions {
struct Cls {
  Cls(int a);
  ~Cls();

  int fn(int a, bool b = false);
  bool fn2(this Cls &self, int, ...);
  static Cls &sfn(int a, ...);
};

constexpr auto ctor =
    (members_of(^^Cls) | std::views::filter(std::meta::is_constructor)).front();
static_assert(parameters_of(ctor).size() == 1);
static_assert(type_of(parameters_of(ctor)[0]) == ^^int);
static_assert(identifier_of(parameters_of(ctor)[0]) == "a");
static_assert(has_consistent_identifier(parameters_of(ctor)[0]));
static_assert(!has_default_argument(parameters_of(ctor)[0]));
static_assert(!is_explicit_object_parameter(parameters_of(ctor)[0]));
static_assert(!has_ellipsis_parameter(ctor));

constexpr auto dtor =
    (members_of(^^Cls) | std::views::filter(std::meta::is_destructor)).front();
static_assert(parameters_of(dtor).size() == 0);
static_assert(!has_ellipsis_parameter(dtor));

static_assert(parameters_of(^^Cls::fn).size() == 2);
static_assert(parameters_of(type_of(^^Cls::fn)) == std::vector {^^int, ^^bool});
static_assert(type_of(parameters_of(^^Cls::fn)[0]) == ^^int);
static_assert(identifier_of(parameters_of(^^Cls::fn)[0]) == "a");
static_assert(has_consistent_identifier(parameters_of(^^Cls::fn)[0]));
static_assert(!has_default_argument(parameters_of(^^Cls::fn)[0]));
static_assert(!is_explicit_object_parameter(parameters_of(^^Cls::fn)[0]));
static_assert(type_of(parameters_of(^^Cls::fn)[1]) == ^^bool);
static_assert(identifier_of(parameters_of(^^Cls::fn)[1]) == "b");
static_assert(has_consistent_identifier(parameters_of(^^Cls::fn)[1]));
static_assert(has_default_argument(parameters_of(^^Cls::fn)[1]));
static_assert(!is_explicit_object_parameter(parameters_of(^^Cls::fn)[1]));
static_assert(!has_ellipsis_parameter(^^Cls::fn));
static_assert(!has_ellipsis_parameter(type_of(^^Cls::fn)));
static_assert(return_type_of(^^Cls::fn) == ^^int);

static_assert(parameters_of(^^Cls::fn2).size() == 2);
static_assert(parameters_of(type_of(^^Cls::fn2)) ==
              std::vector {^^Cls &, ^^int});
static_assert(type_of(parameters_of(^^Cls::fn2)[0]) == ^^Cls &);
static_assert(identifier_of(parameters_of(^^Cls::fn2)[0]) == "self");
static_assert(has_consistent_identifier(parameters_of(^^Cls::fn2)[0]));
static_assert(!has_default_argument(parameters_of(^^Cls::fn2)[0]));
static_assert(is_explicit_object_parameter(parameters_of(^^Cls::fn2)[0]));
static_assert(type_of(parameters_of(^^Cls::fn2)[1]) == ^^int);
static_assert(!has_identifier(parameters_of(^^Cls::fn2)[1]));
static_assert(has_consistent_identifier(parameters_of(^^Cls::fn2)[1]));
static_assert(!has_default_argument(parameters_of(^^Cls::fn2)[1]));
static_assert(has_ellipsis_parameter(^^Cls::fn2));
static_assert(has_ellipsis_parameter(type_of(^^Cls::fn2)));
static_assert(!is_explicit_object_parameter(parameters_of(^^Cls::fn2)[1]));
static_assert(return_type_of(^^Cls::fn2) == ^^bool);

static_assert(parameters_of(^^Cls::sfn).size() == 1);
static_assert(parameters_of(type_of(^^Cls::sfn)) == std::vector {^^int});
static_assert(type_of(parameters_of(^^Cls::sfn)[0]) == ^^int);
static_assert(identifier_of(parameters_of(^^Cls::sfn)[0]) == "a");
static_assert(has_consistent_identifier(parameters_of(^^Cls::sfn)[0]));
static_assert(!has_default_argument(parameters_of(^^Cls::sfn)[0]));
static_assert(!is_explicit_object_parameter(parameters_of(^^Cls::sfn)[0]));
static_assert(has_ellipsis_parameter(^^Cls::sfn));
static_assert(has_ellipsis_parameter(type_of(^^Cls::sfn)));
static_assert(return_type_of(^^Cls::sfn) == ^^Cls&);
}  // namespace with_member_functions

                         // ===========================
                         // with_template_instantiation
                         // ===========================

namespace with_template_instantiation {
template <typename... Ts>
std::tuple<Ts *...> fn(Ts &&... ts);

template <std::meta::info TFn, typename... Ts>  // check with dependent names.
consteval bool check() {
  constexpr auto Fn = substitute(TFn, {^^Ts...});
  static_assert(parameters_of(Fn).size() == 3);
  static_assert(parameters_of(type_of(Fn)) ==
                std::vector {^^int &&, ^^char &&, ^^bool &&});
  static_assert(type_of(parameters_of(Fn)[0]) == ^^int &&);
  static_assert(identifier_of(parameters_of(Fn)[0]) == "ts");
  static_assert(has_consistent_identifier(parameters_of(Fn)[0]));
  static_assert(!has_default_argument(parameters_of(Fn)[0]));
  static_assert(!is_explicit_object_parameter(parameters_of(Fn)[0]));
  static_assert(type_of(parameters_of(Fn)[1]) == ^^char &&);
  static_assert(identifier_of(parameters_of(Fn)[1]) == "ts");
  static_assert(has_consistent_identifier(parameters_of(Fn)[1]));
  static_assert(!has_default_argument(parameters_of(Fn)[1]));
  static_assert(!is_explicit_object_parameter(parameters_of(Fn)[1]));
  static_assert(type_of(parameters_of(Fn)[2]) == ^^bool &&);
  static_assert(identifier_of(parameters_of(Fn)[2]) == "ts");
  static_assert(has_consistent_identifier(parameters_of(Fn)[2]));
  static_assert(!has_default_argument(parameters_of(Fn)[2]));
  static_assert(!is_explicit_object_parameter(parameters_of(Fn)[2]));
  static_assert(!has_ellipsis_parameter(Fn));
  static_assert(!has_ellipsis_parameter(type_of(Fn)));
  static_assert(return_type_of(Fn) == ^^std::tuple<int *, char *, bool *>);

  return true;
}
static_assert(check<^^fn, int, char, bool>());
}  // namespace with_template_instantiation

                           // ======================
                           // with_default_arguments
                           // ======================

namespace with_default_arguments {
// Ensure 'has_default_argument' "sees through" redeclarations that omit them.
void fn(int a, bool b);
void fn(int a, bool b = false);
void fn(int a, bool b);

static_assert(parameters_of(^^fn).size() == 2);
static_assert(parameters_of(type_of(^^fn)) == std::vector {^^int, ^^bool});
static_assert(type_of(parameters_of(^^fn)[0]) == ^^int);
static_assert(identifier_of(parameters_of(^^fn)[0]) == "a");
static_assert(has_consistent_identifier(parameters_of(^^fn)[0]));
static_assert(!has_default_argument(parameters_of(^^fn)[0]));
static_assert(!is_explicit_object_parameter(parameters_of(^^fn)[0]));
static_assert(type_of(parameters_of(^^fn)[1]) == ^^bool);
static_assert(identifier_of(parameters_of(^^fn)[1]) == "b");
static_assert(has_consistent_identifier(parameters_of(^^fn)[1]));
static_assert(has_default_argument(parameters_of(^^fn)[1]));
static_assert(!is_explicit_object_parameter(parameters_of(^^fn)[1]));
static_assert(!has_ellipsis_parameter(^^fn));
static_assert(!has_ellipsis_parameter(type_of(^^fn)));
}  // namespace with_default_arguments

                            // ====================
                            // with_ambiguous_names
                            // ====================

namespace with_ambiguous_names {
void fn(int a1, bool b, char c1);
static_assert(has_consistent_identifier(parameters_of(^^fn)[2]));

void fn(int a2, bool,   char c2);
constexpr auto r_a2 = parameters_of(^^fn)[0];

static_assert(identifier_of(parameters_of(^^fn)[1]) == "b");

void fn(int a3, bool b, char c1);
constexpr auto r_a3 = parameters_of(^^fn)[0];

static_assert(parameters_of(^^fn).size() == 3);
static_assert(parameters_of(type_of(^^fn)) ==
              std::vector {^^int, ^^bool, ^^char});
static_assert(any_identifier_of(parameters_of(^^fn)[0]) == "a1" ||
              any_identifier_of(parameters_of(^^fn)[0]) == "a2" ||
              any_identifier_of(parameters_of(^^fn)[0]) == "a3");
static_assert(!has_consistent_identifier(parameters_of(^^fn)[0]));
static_assert(any_identifier_of(parameters_of(^^fn)[1]) == "b");
static_assert(identifier_of(parameters_of(^^fn)[1]) == "b");
static_assert(has_consistent_identifier(parameters_of(^^fn)[1]));
static_assert(any_identifier_of(parameters_of(^^fn)[2]) == "c1" ||
              any_identifier_of(parameters_of(^^fn)[2]) == "c2");
static_assert(!has_consistent_identifier(parameters_of(^^fn)[2]));

static_assert(r_a2 == r_a3);
}  // namespace with_ambiguous_names

                              // =================
                              // type_and_cv_decay
                              // =================

namespace with_ambiguous_types {
using Alias = int;
void fn(Alias, const bool, char [], char &);

static_assert(type_of(parameters_of(^^fn)[0]) == ^^int);
static_assert(type_of(parameters_of(^^fn)[1]) == ^^bool);
static_assert(type_of(parameters_of(^^fn)[2]) == ^^char *);
static_assert(type_of(parameters_of(^^fn)[3]) == ^^char &);
static_assert(!is_const(parameters_of(^^fn)[1]));
}  // namespace with_ambiguous_types

                        // ============================
                        // identify_function_parameters
                        // ============================

namespace identify_function_parameters {
void fn(int a, bool &b, std::string *, ...);

static_assert(is_function_parameter(parameters_of(^^fn)[0]));
static_assert(is_function_parameter(parameters_of(^^fn)[1]));
static_assert(is_function_parameter(parameters_of(^^fn)[2]));

static_assert(!is_function_parameter(^^::));
static_assert(!is_function_parameter(^^int));
static_assert(!is_function_parameter(^^fn));
static_assert(!is_function_parameter(std::meta::reflect_value(3)));
static_assert(has_ellipsis_parameter(type_of(^^fn)));
}  // namespace identify_function_parameters

                   // =======================================
                   // mangle_reflection_of_function_parameter
                   // =======================================

namespace mangle_reflection_of_function_parameter {
template <auto R> void fn() {}

void user(int) {
  fn<parameters_of(^^user)[0]>();
}
}  // namespace mangle_reflection_of_function_parameter

int main() { }
