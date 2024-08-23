//===----------------------------------------------------------------------===//
//
// Copyright 2024 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// RUN: %clang_cc1 %s -std=c++23 -freflection -freflection-new-syntax

// Reflecting Types
using info = decltype(^^void);

constexpr info info_void = ^^void;
constexpr info info_decltype = ^^decltype(42);

using alias = int;
constexpr info info_alias = ^^alias;

constexpr info info_infoint = ^^decltype(^^int);

constexpr info abominable = ^^void() const & noexcept;

// Reflecting variables
constexpr int i = 42;
constexpr info info_i = ^^i;

// Reflecting templates
template <typename T>
void TemplateFunc();
constexpr info info_template_func = ^^TemplateFunc;

template <typename T>
int TemplateVar;
constexpr info info_template_var = ^^TemplateVar;

template <typename T>
struct TemplateStruct {};
constexpr info info_template_struct = ^^TemplateStruct;

// Reflecting members of a class
struct Members {
    struct Type {};
    int member_var;
    static int static_member_var;

    int member_func();
    static int static_member_func();
};
constexpr info info_nested_type = ^^Members::Type;
constexpr info info_mem_var = ^^Members::member_var;
constexpr info info_static_mem_var = ^^Members::static_member_var;
constexpr info info_mem_func = ^^Members::member_func;
constexpr info info_static_mem_func = ^^Members::static_member_func;

// Reflecting member templates
struct MemberTemplates {
    template <typename T>
    struct NestedTemplateStruct {};

    template <typename T>
    void template_func();

    template <typename T>
    MemberTemplates &operator+(const T&);

    template <typename T>
    static void template_static_func();

    template <typename T>
    static int template_var;
};
constexpr info info_nested_template_struct =
      ^^MemberTemplates::NestedTemplateStruct;
constexpr info info_nested_template_struct2 =
      ^^MemberTemplates::template NestedTemplateStruct;
constexpr info info_nested_template_func = ^^MemberTemplates::template_func;
constexpr info info_nested_template_func2 =
      ^^MemberTemplates::template template_func;
constexpr info info_nested_template_static_func =
      ^^MemberTemplates::template_static_func;
constexpr info info_nested_template_static_func2 =
      ^^MemberTemplates::template template_static_func;
constexpr info info_nested_template_var = ^^MemberTemplates::template_var;
constexpr info info_nested_template_var2 =
      ^^MemberTemplates::template template_var;
constexpr info info_nested_template_operator = ^^MemberTemplates::operator+;
constexpr info info_nested_template_operator2 =
      ^^MemberTemplates::template operator+;

template <typename T>
void DepScope() {
  constexpr info nested_struct = ^^T::template NestedTemplateStruct;
  constexpr info nested_func = ^^T::template template_func;
  constexpr info nested_static_func = ^^T::template template_static_func;
  constexpr info nested_var = ^^T::template template_var;
  constexpr info nested_operator = ^^T::template operator+;
}
void InstantiateDepScope() {
  DepScope<MemberTemplates>();
}

// Reflecting function scope variables
void reflect_func_scope(int param) {
    constexpr info info_param = ^^param;

    int local_var;
    constexpr info info_local = ^^local_var;

    static int static_var;
    constexpr info info_static = ^^static_var;

    thread_local int thread_var;
    constexpr info info_thread = ^^thread_var;

    struct local_type {};
    constexpr info info_type = ^^local_type;
}

// Reflecting a template parameter
template <typename T>
consteval info foo() {
    return ^^T;
}
constexpr info info_tmplparam = foo<int>();

namespace ns {}
constexpr info info_ns = ^^ns;

// Reflection as a default initializer for a class member
class WithDefaultInitializer {
    [[maybe_unused]] info k = ^^::;
};
constexpr WithDefaultInitializer with_default_init;

                   // =======================================
                   // bb_clang_p2996_issue_35_regression_test
                   // =======================================

namespace bb_clang_p2996_issue_35_regression_test {
template <auto R = ^^::> class S {};
S s;
}  // namespace bb_clang_p2996_issue_35_regression_test

                   // =======================================
                   // bb_clang_p2996_issue_73_regression_test
                   // =======================================

namespace bb_clang_p2996_issue_73_regression_test {
namespace foo {
struct foo {
  static_assert(^^foo == ^^::bb_clang_p2996_issue_73_regression_test::foo::foo);
};
static_assert(^^foo == ^^::bb_clang_p2996_issue_73_regression_test::foo::foo);
}  // namespace foo

namespace tfoo {
template <typename T> struct tfoo {
  static_assert(^^tfoo == ^^tfoo<T>);
};
static_assert(^^tfoo ==
              ^^::bb_clang_p2996_issue_73_regression_test::tfoo::tfoo);

tfoo<int> instantiation;
}  // namespace tfoo

static_assert(^^foo == ^^::bb_clang_p2996_issue_73_regression_test::foo);
static_assert(^^tfoo == ^^::bb_clang_p2996_issue_73_regression_test::tfoo);
}  // namespace bb_clang_p2996_issue_73_regression_test
