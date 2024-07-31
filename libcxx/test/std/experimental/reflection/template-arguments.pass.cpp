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

#include <experimental/meta>


template <typename P1, auto P2, template <typename...> class P3>
struct TCls {
  template <typename> struct TInnerCls {};
};

template <typename P1, auto P2, template <typename...> class P3>
void TFn();

template <typename P1, auto P2, template <typename...> class P3>
int TVar = 0;

template <typename P1, auto P2, template <typename...> class P3>
using TAlias = int;

template <typename P1, auto P2, template <typename...> class P3>
concept Concept = requires { true; };

                                // =============
                                // non_templates
                                // =============

namespace non_templates {
namespace myns {}
TCls<int, 9, std::vector> var;
void fn();
using Alias = TCls<int, 9, std::vector>;
typedef TCls<int, 9, std::vector> Typedef;
namespace NSAlias = myns;
static_assert(!has_template_arguments(^::));
static_assert(!has_template_arguments(^myns));
static_assert(!has_template_arguments(^int));
static_assert(!has_template_arguments(^var));
static_assert(!has_template_arguments(^fn));
static_assert(!has_template_arguments(^Alias));
static_assert(!has_template_arguments(^Typedef));
static_assert(!has_template_arguments(^NSAlias));
static_assert(
    !has_template_arguments(
        substitute(^Concept,
                   {^int, std::meta::reflect_value(9), ^std::vector})));
}  // namespace non_templates

                             // ==================
                             // all_template_kinds
                             // ==================

namespace all_template_kinds {
static_assert(!has_template_arguments(^TCls));
static_assert(has_template_arguments(^TCls<int, 9, std::vector>));
static_assert(template_arguments_of(^TCls<int, 9, std::vector>).size() == 3);
static_assert(template_arguments_of(^TCls<int, 9, std::vector>)[0] == ^int);
static_assert([:template_arguments_of(^TCls<int, 9, std::vector>)[1]:] == 9);
static_assert(template_arguments_of(^TCls<int, 9, std::vector>)[2] ==
              ^std::vector);

static_assert(!has_template_arguments(^TFn));
static_assert(has_template_arguments(^TFn<int, 9, std::vector>));
static_assert(template_arguments_of(^TFn<int, 9, std::vector>).size() == 3);
static_assert(template_arguments_of(^TFn<int, 9, std::vector>)[0] == ^int);
static_assert([:template_arguments_of(^TFn<int, 9, std::vector>)[1]:] == 9);
static_assert(template_arguments_of(^TFn<int, 9, std::vector>)[2] ==
              ^std::vector);

static_assert(!has_template_arguments(^TVar));
static_assert(has_template_arguments(^TVar<int, 9, std::vector>));
static_assert(template_arguments_of(^TVar<int, 9, std::vector>).size() == 3);
static_assert(template_arguments_of(^TVar<int, 9, std::vector>)[0] == ^int);
static_assert([:template_arguments_of(^TVar<int, 9, std::vector>)[1]:] == 9);
static_assert(template_arguments_of(^TVar<int, 9, std::vector>)[2] ==
              ^std::vector);

static_assert(!has_template_arguments(^TAlias));
static_assert(has_template_arguments(^TAlias<int, 9, std::vector>));
static_assert(template_arguments_of(^TAlias<int, 9, std::vector>).size() == 3);
static_assert(template_arguments_of(^TAlias<int, 9, std::vector>)[0] == ^int);
static_assert([:template_arguments_of(^TAlias<int, 9, std::vector>)[1]:] == 9);
static_assert(template_arguments_of(^TAlias<int, 9, std::vector>)[2] ==
              ^std::vector);

static_assert(
        type_of(template_arguments_of(^TCls<int, false, std::vector>)[1]) ==
        ^bool);
}  // namespace all_template_kinds

                                // =============
                                // special_cases
                                // =============

namespace special_cases {
template <typename T>
using DependentAlias = TCls<int, 9, std::vector>::template TInnerCls<T>;
static_assert(!has_template_arguments(^DependentAlias));
static_assert(has_template_arguments(^DependentAlias<int>));
static_assert(template_arguments_of(^DependentAlias<int>).size() == 1);
static_assert(template_arguments_of(^DependentAlias<int>)[0] == ^int);

struct SubclassOfTemplate : TCls<int, 9, std::vector> {};
static_assert(!has_template_arguments(^SubclassOfTemplate));

template <typename T, T Val> struct WithDependentArgument {};
static_assert(!has_template_arguments(^WithDependentArgument));
static_assert(has_template_arguments(^WithDependentArgument<int, 5>));
static_assert(
      template_arguments_of(^WithDependentArgument<int, 5>).size() == 2);
static_assert(template_arguments_of(^WithDependentArgument<int, 5>)[0] == ^int);
static_assert(template_arguments_of(^WithDependentArgument<int, 5>)[1] ==
              std::meta::reflect_value(5));
}  // namespace special_cases

                               // ===============
                               // parameter_packs
                               // ===============

namespace parameter_packs {
template <typename... Ts> struct WithTypeParamPack {};
static_assert(!has_template_arguments(^WithTypeParamPack));
static_assert(has_template_arguments(^WithTypeParamPack<int, bool>));
static_assert(template_arguments_of(^WithTypeParamPack<int, bool>).size() == 2);
static_assert(template_arguments_of(^WithTypeParamPack<int, bool>)[0] == ^int);
static_assert(template_arguments_of(^WithTypeParamPack<int, bool>)[1] == ^bool);

struct S {
  int mem;
  bool operator==(const S&) const = default;
};

template <auto... Vs> struct WithAutoParamPack {};
static_assert(!has_template_arguments(^WithAutoParamPack));
static_assert(has_template_arguments(^WithAutoParamPack<4, S{3}>));
static_assert(template_arguments_of(^WithAutoParamPack<4, S{3}>).size() == 2);
static_assert([:template_arguments_of(^WithAutoParamPack<4, S{3}>)[0]:] == 4);
static_assert([:template_arguments_of(^WithAutoParamPack<4, S{3}>)[1]:] ==
              S{3});
}  // namespace parameter_packs

                             // ==================
                             // non_auto_non_types
                             // ==================

namespace non_auto_non_types {
template <float> struct WithFloat {};
template <const float *> struct WithPtr {};
template <const int &> struct WithRef {};
template <std::meta::info> struct WithReflection {};

template <std::meta::info> void FnWithReflection() {}

constexpr float F = 4.5f;
static_assert(!has_template_arguments(^WithFloat));
static_assert(has_template_arguments(^WithFloat<F>));
static_assert(template_arguments_of(^WithFloat<F>).size() == 1);
static_assert([:template_arguments_of(^WithFloat<F>)[0]:] == F);

constexpr float Fs[] = {4.5, 5.5, 6.5};
static_assert(!has_template_arguments(^WithPtr));
static_assert(has_template_arguments(^WithPtr<Fs>));
static_assert(template_arguments_of(^WithPtr<Fs>).size() == 1);
static_assert([:template_arguments_of(^WithPtr<Fs>)[0]:] == +Fs);

static_assert(has_template_arguments(^WithPtr<nullptr>));
static_assert(template_arguments_of(^WithPtr<nullptr>).size() == 1);
static_assert([:template_arguments_of(^WithPtr<nullptr>)[0]:] == nullptr);

const int I = 5;
using T = WithRef<I>;
static_assert(!has_template_arguments(^WithRef));
static_assert(has_template_arguments(dealias(^T)));
static_assert(template_arguments_of(dealias(^T)).size() == 1);
static_assert([:template_arguments_of(^WithRef<I>)[0]:] == I);

static_assert(!has_template_arguments(^WithReflection));
static_assert(has_template_arguments(^WithReflection<^int>));
static_assert(template_arguments_of(^WithReflection<^int>).size() == 1);
static_assert(template_arguments_of(^WithReflection<^int>)[0] ==
              std::meta::reflect_value(^int));

void instantiations() {
  [[maybe_unused]] WithReflection<std::meta::reflect_value(nullptr)> wr;
  FnWithReflection<std::meta::reflect_value(nullptr)>();
}
}  // namespace non_auto_non_types

                           // =======================
                           // properties_of_non_types
                           // =======================

namespace properties_of_non_types {
template <int P> void fn_int_value() {
  static_assert(is_value(^P));
  static_assert(type_of(^P) == ^int);
  static_assert([:^P:] == 1);
}

template <const int &P> void fn_int_ref() {
  static_assert(is_object(^P));
  static_assert(!is_variable(^P));
  if constexpr (is_const(^P)) {
    static_assert(type_of(^P) == ^const int);
    static_assert([:^P:] == 2);
  } else {
    static_assert(type_of(^P) == ^int);
  }
}

template <const int &P> void fn_int_subobject_ref() {
  static_assert(is_object(^P));
  static_assert(!is_variable(^P));
  static_assert(type_of(^P) == ^const int);
  static_assert([:^P:] == 3);
}

struct S { int m; };

template <S P> void fn_cls_value() {
  static_assert(is_object(^P));
  static_assert(!is_variable(^P));  // template-parameter-object
  static_assert(type_of(^P) == ^const S);
  static_assert([:^P:].m == 5);
}

template <S &P> void fn_cls_ref() {
  static_assert(is_object(^P));
  static_assert(!is_variable(^P));
  static_assert(type_of(^P) == ^S);
}

template <void(&P)()> void fn_fn_ref_param() {
  static_assert(is_function(^P));
  static_assert(type_of(^P) == ^void());
  static_assert(identifier_of(^P) == "instantiations");
}

template <void(*P)()> void fn_fn_ptr_param() {
  static_assert(is_value(^P));
  static_assert(!is_function(^P));
  static_assert(type_of(^P) == ^void(*)());
}

void instantiations() {
  fn_int_value<1>();

  static constexpr int k = 2;
  fn_int_ref<k>();

  static int k2;
  fn_int_ref<k2>();

  static constexpr std::pair<int, int> p {3, 4};
  fn_int_subobject_ref<p.first>();

  fn_cls_value<{5}>();

  static S s;
  fn_cls_ref<s>();

  fn_fn_ref_param<instantiations>();
  fn_fn_ptr_param<instantiations>();
}
}  // namespace properties_of_non_types

                       // ==============================
                       // subobject_reflection_arguments
                       // ==============================

namespace subobject_reflection_arguments {
template <int &> void fn();

int p[2];
static_assert(template_arguments_of(^fn<p[1]>)[0] ==
              std::meta::reflect_object(p[1]));

}  // namespace subobject_reflection_arguments

                   // =======================================
                   // bb_clang_p2996_issue_41_regression_test
                   // =======================================

namespace bb_clang_p2996_issue_41_regression_test {
template<class T> struct TCls {};

TCls<int> obj1;
TCls<decltype(obj1)> obj2;

static_assert(
        has_template_arguments(template_arguments_of(^decltype(obj2))[0]) ==
        has_template_arguments(^TCls<int>));
}  // namespace bb_clang_p2996_issue_41_regression_test

                   // =======================================
                   // bb_clang_p2996_issue_54_regression_test
                   // =======================================

namespace bb_clang_p2996_issue_54_regression_test {
template <auto R> void fn() { }

void fn() {
    class S { S(); ~S(); };
    fn<(members_of(^S) |
            std::views::filter(std::meta::is_constructor)).front()>();
    fn<(members_of(^S) |
            std::views::filter(std::meta::is_destructor)).front()>();
}
}  // namespace bb_clang_p2996_issue_54_regression_test

int main() { }
