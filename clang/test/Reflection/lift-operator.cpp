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
// RUN: %clang_cc1 %s -std=c++23 -freflection

// Reflecting Types
using info = decltype(^void);

constexpr info info_void = ^void;
constexpr info info_decltype = ^decltype(42);

using alias = int;
constexpr info info_alias = ^alias;

constexpr info info_infoint = ^decltype(^int);

constexpr info abominable = ^void() const & noexcept;

// Reflecting variables
constexpr int i = 42;
constexpr info info_i = ^i;

// Reflecting templates
template <typename T>
void TemplateFunc();
constexpr info info_template_func = ^TemplateFunc;

template <typename T>
int TemplateVar;
constexpr info info_template_var = ^TemplateVar;

template <typename T>
struct TemplateStruct {};
constexpr info info_template_struct = ^TemplateStruct;

// Reflecting members of a class
struct Members {
    struct Type {};
    int member_var;
    static int static_member_var;

    int member_func();
    static int static_member_func();
};
constexpr info info_nested_type = ^Members::Type;
constexpr info info_mem_var = ^Members::member_var;
constexpr info info_static_mem_var = ^Members::static_member_var;
constexpr info info_mem_func = ^Members::member_func;
constexpr info info_static_mem_func = ^Members::static_member_func;

// Reflecting member templates
struct MemberTemplates {
    template <typename T>
    struct NestedTemplateStruct {};

    template <typename T>
    void template_func();

    template <typename T>
    static void template_static_func();

    template <typename T>
    static int template_var;
};
constexpr info info_nested_template_struct = ^MemberTemplates::NestedTemplateStruct;
constexpr info info_nested_template_func = ^MemberTemplates::template_func;
constexpr info info_nested_template_static_func = ^MemberTemplates::template_static_func;
constexpr info info_nested_template_var = ^MemberTemplates::template_var;

// Reflecting function scope variables
void reflect_func_scope(int param) {
    constexpr info info_param = ^param;

    int local_var;
    constexpr info info_local = ^local_var;

    static int static_var;
    constexpr info info_static = ^static_var;

    thread_local int thread_var;
    constexpr info info_thread = ^thread_var;

    struct local_type {};
    constexpr info info_type = ^local_type;
}

// Reflecting a template parameter
template <typename T>
consteval info foo() {
    return ^T;
}
constexpr info info_tmplparam = foo<int>();

namespace ns {}
constexpr info info_ns = ^ns;

                   // =======================================
                   // bb_clang_p2996_issue_35_regression_test
                   // =======================================

namespace bb_clang_p2996_issue_35_regression_test {
template <auto R = ^::> class S {};
S s;
}  // namespace bb_clang_p2996_issue_35_regression_test
