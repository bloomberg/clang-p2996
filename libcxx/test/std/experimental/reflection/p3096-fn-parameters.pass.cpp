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

static_assert(parameters_of(^fn).size() == 0);
static_assert(parameters_of(^tfn<>).size() == 0);
}  // namespace with_no_arguments

                            // ====================
                            // with_fixed_arguments
                            // ====================

namespace with_fixed_arguments {
template <typename T>
struct S {};

void fn(int a, bool b, const S<int> &s);
static_assert(parameters_of(^fn).size() == 3);
static_assert(parameters_of(type_of(^fn)) ==
              std::vector {^int, ^bool, ^const S<int> &});
static_assert(type_of(parameters_of(^fn)[0]) == ^int);
static_assert(name_of(parameters_of(^fn)[0]) == u8"a");
static_assert(has_consistent_name(parameters_of(^fn)[0]));
static_assert(!has_default_argument(parameters_of(^fn)[0]));
static_assert(type_of(parameters_of(^fn)[1]) == ^bool);
static_assert(name_of(parameters_of(^fn)[1]) == u8"b");
static_assert(has_consistent_name(parameters_of(^fn)[1]));
static_assert(!has_default_argument(parameters_of(^fn)[1]));
static_assert(type_of(parameters_of(^fn)[2]) == ^const S<int> &);
static_assert(name_of(parameters_of(^fn)[2]) == u8"s");
static_assert(has_consistent_name(parameters_of(^fn)[2]));
static_assert(!has_default_argument(parameters_of(^fn)[2]));
}  // namespace with_fixed_arguments

                            // =====================
                            // with_member_functions
                            // =====================

namespace with_member_functions {
struct Cls {
  Cls(int a);
  ~Cls();

  void fn(int a, bool b = false);
  void fn2(this Cls &self, int);
  static void sfn(int a);
};

constexpr auto ctor = [] {
  return members_of(^Cls, std::meta::is_constructor)[0];
}();
static_assert(parameters_of(ctor).size() == 1);
static_assert(type_of(parameters_of(ctor)[0]) == ^int);
static_assert(name_of(parameters_of(ctor)[0]) == u8"a");
static_assert(has_consistent_name(parameters_of(ctor)[0]));
static_assert(!has_default_argument(parameters_of(ctor)[0]));

constexpr auto dtor = [] {
  return members_of(^Cls, std::meta::is_destructor)[0];
}();
static_assert(parameters_of(dtor).size() == 0);

static_assert(parameters_of(^Cls::fn).size() == 2);
static_assert(parameters_of(type_of(^Cls::fn)) == std::vector {^int, ^bool});
static_assert(type_of(parameters_of(^Cls::fn)[0]) == ^int);
static_assert(name_of(parameters_of(^Cls::fn)[0]) == u8"a");
static_assert(has_consistent_name(parameters_of(^Cls::fn)[0]));
static_assert(!has_default_argument(parameters_of(^Cls::fn)[0]));
static_assert(type_of(parameters_of(^Cls::fn)[1]) == ^bool);
static_assert(name_of(parameters_of(^Cls::fn)[1]) == u8"b");
static_assert(has_consistent_name(parameters_of(^Cls::fn)[1]));
static_assert(has_default_argument(parameters_of(^Cls::fn)[1]));

static_assert(parameters_of(^Cls::fn2).size() == 2);
static_assert(parameters_of(type_of(^Cls::fn2)) == std::vector {^Cls &, ^int});
static_assert(type_of(parameters_of(^Cls::fn2)[0]) == ^Cls &);
static_assert(name_of(parameters_of(^Cls::fn2)[0]) == u8"self");
static_assert(has_consistent_name(parameters_of(^Cls::fn2)[0]));
static_assert(!has_default_argument(parameters_of(^Cls::fn2)[0]));
static_assert(type_of(parameters_of(^Cls::fn2)[1]) == ^int);
static_assert(name_of(parameters_of(^Cls::fn2)[1]) == u8"");
static_assert(has_consistent_name(parameters_of(^Cls::fn2)[1]));
static_assert(!has_default_argument(parameters_of(^Cls::fn2)[1]));

static_assert(parameters_of(^Cls::sfn).size() == 1);
static_assert(parameters_of(type_of(^Cls::sfn)) == std::vector {^int});
static_assert(type_of(parameters_of(^Cls::sfn)[0]) == ^int);
static_assert(name_of(parameters_of(^Cls::sfn)[0]) == u8"a");
static_assert(has_consistent_name(parameters_of(^Cls::sfn)[0]));
static_assert(!has_default_argument(parameters_of(^Cls::sfn)[0]));
}  // namespace with_member_functions

                         // ===========================
                         // with_template_instantiation
                         // ===========================

namespace with_template_instantiation {
template <typename... Ts>
void fn(Ts &&... ts);

template <std::meta::info TFn, typename... Ts>  // check with dependent names.
consteval bool check() {
  constexpr auto Fn = substitute(TFn, {^Ts...});
  static_assert(parameters_of(Fn).size() == 3);
  static_assert(parameters_of(type_of(Fn)) ==
                std::vector {^int &&, ^char &&, ^bool &&});
  static_assert(type_of(parameters_of(Fn)[0]) == ^int &&);
  static_assert(name_of(parameters_of(Fn)[0]) == u8"ts");
  static_assert(has_consistent_name(parameters_of(Fn)[0]));
  static_assert(!has_default_argument(parameters_of(Fn)[0]));
  static_assert(type_of(parameters_of(Fn)[1]) == ^char &&);
  static_assert(name_of(parameters_of(Fn)[1]) == u8"ts");
  static_assert(has_consistent_name(parameters_of(Fn)[1]));
  static_assert(!has_default_argument(parameters_of(Fn)[1]));
  static_assert(type_of(parameters_of(Fn)[2]) == ^bool &&);
  static_assert(name_of(parameters_of(Fn)[2]) == u8"ts");
  static_assert(has_consistent_name(parameters_of(Fn)[2]));
  static_assert(!has_default_argument(parameters_of(Fn)[2]));

  return true;
}
static_assert(check<^fn, int, char, bool>());
}  // namespace with_template_instantiation

                           // ======================
                           // with_default_arguments
                           // ======================

namespace with_default_arguments {
// Ensure 'has_default_argument' "sees through" redeclarations that omit them.
void fn(int a, bool b);
void fn(int a, bool b = false);
void fn(int a, bool b);

static_assert(parameters_of(^fn).size() == 2);
static_assert(parameters_of(type_of(^fn)) == std::vector {^int, ^bool});
static_assert(type_of(parameters_of(^fn)[0]) == ^int);
static_assert(name_of(parameters_of(^fn)[0]) == u8"a");
static_assert(has_consistent_name(parameters_of(^fn)[0]));
static_assert(!has_default_argument(parameters_of(^fn)[0]));
static_assert(type_of(parameters_of(^fn)[1]) == ^bool);
static_assert(name_of(parameters_of(^fn)[1]) == u8"b");
static_assert(has_consistent_name(parameters_of(^fn)[1]));
static_assert(has_default_argument(parameters_of(^fn)[1]));
}  // namespace with_default_arguments

                            // ====================
                            // with_ambiguous_names
                            // ====================

namespace with_ambiguous_names {
void fn(int a1, bool b, char c1);
void fn(int a2, bool b, char c2);
void fn(int a3, bool b, char c1);

static_assert(parameters_of(^fn).size() == 3);
static_assert(parameters_of(type_of(^fn)) == std::vector {^int, ^bool, ^char});
static_assert(name_of(parameters_of(^fn)[0]) == u8"a1" ||
              name_of(parameters_of(^fn)[0]) == u8"a2" ||
              name_of(parameters_of(^fn)[0]) == u8"a3");
static_assert(!has_consistent_name(parameters_of(^fn)[0]));
static_assert(name_of(parameters_of(^fn)[1]) == u8"b");
static_assert(has_consistent_name(parameters_of(^fn)[1]));
static_assert(name_of(parameters_of(^fn)[2]) == u8"c1" ||
              name_of(parameters_of(^fn)[2]) == u8"c2");
static_assert(!has_consistent_name(parameters_of(^fn)[2]));
}  // namespace with_ambiguous_names


int main() { }
