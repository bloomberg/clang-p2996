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
#include <ranges>

constexpr auto ctx = std::meta::access_context::unchecked();

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
static_assert(!has_template_arguments(^^::));
static_assert(!has_template_arguments(^^myns));
static_assert(!has_template_arguments(^^int));
static_assert(!has_template_arguments(^^var));
static_assert(!has_template_arguments(^^fn));
static_assert(!has_template_arguments(^^Alias));
static_assert(!has_template_arguments(^^Typedef));
static_assert(!has_template_arguments(^^NSAlias));
static_assert(
    !has_template_arguments(
        substitute(^^Concept,
                   {^^int, std::meta::reflect_constant(9), ^^std::vector})));
}  // namespace non_templates

                             // ==================
                             // all_template_kinds
                             // ==================

namespace all_template_kinds {
static_assert(!has_template_arguments(^^TCls));
static_assert(has_template_arguments(^^TCls<int, 9, std::vector>));
static_assert(template_arguments_of(^^TCls<int, 9, std::vector>).size() == 3);
static_assert(template_arguments_of(^^TCls<int, 9, std::vector>)[0] == ^^int);
static_assert([:template_arguments_of(^^TCls<int, 9, std::vector>)[1]:] == 9);
static_assert(template_arguments_of(^^TCls<int, 9, std::vector>)[2] ==
              ^^std::vector);

static_assert(!has_template_arguments(^^TFn));
static_assert(has_template_arguments(^^TFn<int, 9, std::vector>));
static_assert(template_arguments_of(^^TFn<int, 9, std::vector>).size() == 3);
static_assert(template_arguments_of(^^TFn<int, 9, std::vector>)[0] == ^^int);
static_assert([:template_arguments_of(^^TFn<int, 9, std::vector>)[1]:] == 9);
static_assert(template_arguments_of(^^TFn<int, 9, std::vector>)[2] ==
              ^^std::vector);

static_assert(!has_template_arguments(^^TVar));
static_assert(has_template_arguments(^^TVar<int, 9, std::vector>));
static_assert(template_arguments_of(^^TVar<int, 9, std::vector>).size() == 3);
static_assert(template_arguments_of(^^TVar<int, 9, std::vector>)[0] == ^^int);
static_assert([:template_arguments_of(^^TVar<int, 9, std::vector>)[1]:] == 9);
static_assert(template_arguments_of(^^TVar<int, 9, std::vector>)[2] ==
              ^^std::vector);

static_assert(!has_template_arguments(^^TAlias));
static_assert(has_template_arguments(^^TAlias<int, 9, std::vector>));
static_assert(template_arguments_of(^^TAlias<int, 9, std::vector>).size() == 3);
static_assert(template_arguments_of(^^TAlias<int, 9, std::vector>)[0] == ^^int);
static_assert([:template_arguments_of(^^TAlias<int, 9, std::vector>)[1]:] == 9);
static_assert(template_arguments_of(^^TAlias<int, 9, std::vector>)[2] ==
              ^^std::vector);

static_assert(
        type_of(template_arguments_of(^^TCls<int, false, std::vector>)[1]) ==
        ^^bool);
}  // namespace all_template_kinds

                                // =============
                                // special_cases
                                // =============

namespace special_cases {
template <typename T>
using DependentAlias = TCls<int, 9, std::vector>::template TInnerCls<T>;
static_assert(!has_template_arguments(^^DependentAlias));
static_assert(has_template_arguments(^^DependentAlias<int>));
static_assert(template_arguments_of(^^DependentAlias<int>).size() == 1);
static_assert(template_arguments_of(^^DependentAlias<int>)[0] == ^^int);

struct SubclassOfTemplate : TCls<int, 9, std::vector> {};
static_assert(!has_template_arguments(^^SubclassOfTemplate));

template <typename T, T Val> struct WithDependentArgument {};
static_assert(!has_template_arguments(^^WithDependentArgument));
static_assert(has_template_arguments(^^WithDependentArgument<int, 5>));
static_assert(
      template_arguments_of(^^WithDependentArgument<int, 5>).size() == 2);
static_assert(template_arguments_of(^^WithDependentArgument<int, 5>)[0] ==
              ^^int);
static_assert(template_arguments_of(^^WithDependentArgument<int, 5>)[1] ==
              std::meta::reflect_constant(5));
}  // namespace special_cases

                               // ===============
                               // parameter_packs
                               // ===============

namespace parameter_packs {
template <typename... Ts> struct WithTypeParamPack {};
static_assert(!has_template_arguments(^^WithTypeParamPack));
static_assert(has_template_arguments(^^WithTypeParamPack<int, bool>));
static_assert(template_arguments_of(^^WithTypeParamPack<int, bool>).size() ==
              2);
static_assert(template_arguments_of(^^WithTypeParamPack<int, bool>)[0] ==
              ^^int);
static_assert(template_arguments_of(^^WithTypeParamPack<int, bool>)[1] ==
              ^^bool);

struct S {
  int mem;
  bool operator==(const S&) const = default;
};

template <auto... Vs> struct WithAutoParamPack {};
static_assert(!has_template_arguments(^^WithAutoParamPack));
static_assert(has_template_arguments(^^WithAutoParamPack<4, S{3}>));
static_assert(template_arguments_of(^^WithAutoParamPack<4, S{3}>).size() == 2);
static_assert([:template_arguments_of(^^WithAutoParamPack<4, S{3}>)[0]:] == 4);
static_assert([:template_arguments_of(^^WithAutoParamPack<4, S{3}>)[1]:] ==
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
static_assert(!has_template_arguments(^^WithFloat));
static_assert(has_template_arguments(^^WithFloat<F>));
static_assert(template_arguments_of(^^WithFloat<F>).size() == 1);
static_assert([:template_arguments_of(^^WithFloat<F>)[0]:] == F);

constexpr float Fs[] = {4.5, 5.5, 6.5};
static_assert(!has_template_arguments(^^WithPtr));
static_assert(has_template_arguments(^^WithPtr<Fs>));
static_assert(template_arguments_of(^^WithPtr<Fs>).size() == 1);
static_assert([:template_arguments_of(^^WithPtr<Fs>)[0]:] == +Fs);

static_assert(has_template_arguments(^^WithPtr<nullptr>));
static_assert(template_arguments_of(^^WithPtr<nullptr>).size() == 1);
static_assert([:template_arguments_of(^^WithPtr<nullptr>)[0]:] == nullptr);

const int I = 5;
using T = WithRef<I>;
static_assert(!has_template_arguments(^^WithRef));
static_assert(has_template_arguments(dealias(^^T)));
static_assert(template_arguments_of(dealias(^^T)).size() == 1);
static_assert([:template_arguments_of(^^WithRef<I>)[0]:] == I);

static_assert(!has_template_arguments(^^WithReflection));
static_assert(has_template_arguments(^^WithReflection<^^int>));
static_assert(template_arguments_of(^^WithReflection<^^int>).size() == 1);
static_assert(template_arguments_of(^^WithReflection<^^int>)[0] ==
              std::meta::reflect_constant(^^int));

void instantiations() {
  [[maybe_unused]] WithReflection<std::meta::reflect_constant(nullptr)> wr;
  FnWithReflection<std::meta::reflect_constant(nullptr)>();
}
}  // namespace non_auto_non_types

                           // =======================
                           // properties_of_non_types
                           // =======================

namespace properties_of_non_types {
template <int P> void fn_int_value() {
  static_assert(is_value(std::meta::reflect_constant(P)));
  static_assert(type_of(std::meta::reflect_constant(P)) == ^^int);
  static_assert([:std::meta::reflect_constant(P):] == 1);
}

template <const int &P> void fn_int_ref() {
  static constexpr auto R = std::meta::reflect_object(P);

  static_assert(is_object(R));
  static_assert(!is_variable(R));
  if constexpr (is_const(R)) {
    static_assert(type_of(R) == ^^const int);
    static_assert([:R:] == 2);
  } else {
    static_assert(type_of(R) == ^^int);
  }
}

template <const int &P> void fn_int_subobject_ref() {
  static constexpr auto R = std::meta::reflect_object(P);

  static_assert(is_object(R));
  static_assert(!is_variable(R));
  static_assert(type_of(R) == ^^const int);
  static_assert([:R:] == 3);
}

struct S { int m; };

template <S P> void fn_cls_value() {
  static constexpr auto R = std::meta::reflect_object(P);

  static_assert(is_object(R));
  static_assert(!is_variable(R));  // template-parameter-object
  static_assert(type_of(R) == ^^const S);
  static_assert([:R:].m == 5);
}

template <S &P> void fn_cls_ref() {
  static constexpr auto R = std::meta::reflect_object(P);

  static_assert(is_object(R));
  static_assert(!is_variable(R));
  static_assert(type_of(R) == ^^S);
}

template <void(&P)()> void fn_fn_ref_param() {
  static constexpr auto R = std::meta::reflect_function(P);

  static_assert(is_function(R));
  static_assert(type_of(R) == ^^void());
  static_assert(identifier_of(R) == "instantiations");
}

template <void(*P)()> void fn_fn_ptr_param() {
  static constexpr auto R = std::meta::reflect_constant(P);

  static_assert(is_value(R));
  static_assert(!is_function(R));
  static_assert(type_of(R) == ^^void(*)());
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
static_assert(template_arguments_of(^^fn<p[1]>)[0] ==
              std::meta::reflect_object(p[1]));

// An array index in an lvalue path is a raw integer, not a tagged pointer:
// indices large enough to survive masking off the low bits of a 'Decl *' must
// not be decoded as declarations.
int big[64];
struct Base { int b; };
struct Derived : Base { int arr[64]; };
Derived d;

template <const int *> struct PtrCls {};
template <int &> struct RefCls {};

static_assert(template_arguments_of(^^fn<big[40]>)[0] ==
              std::meta::reflect_object(big[40]));
static_assert(type_of(template_arguments_of(^^fn<big[40]>)[0]) == ^^int);

static_assert(type_of(template_arguments_of(^^PtrCls<&big[40]>)[0]) ==
              ^^const int *);

// The same, reached through a base-class and member path before the index.
static_assert(template_arguments_of(^^fn<d.arr[33]>)[0] ==
              std::meta::reflect_object(d.arr[33]));
static_assert(type_of(template_arguments_of(^^RefCls<d.arr[33]>)[0]) == ^^int);
static_assert(type_of(template_arguments_of(^^RefCls<d.b>)[0]) == ^^int);

}  // namespace subobject_reflection_arguments

                       // ===============================
                       // pointer_and_reference_parameters
                       // ===============================

// [meta.reflection.queries]/59.3: for a constant template argument whose
// parameter has reference type, the reflection represents the object or
// function referred to (59.3.1); otherwise, for a pointer or pointer to member,
// it represents the value of the argument (59.3.3).
namespace pointer_and_reference_parameters {
void fn(int);
void fn_noexcept(int) noexcept;
int obj;
int arr[3];
struct S { int m; static int sm; void mf(); static void sf(); };
template <void (*P)(int)> struct FnPtr {};
template <void (&R)(int)> struct FnRef {};
template <int *P> struct ObjPtr {};
template <int &R> struct ObjRef {};
template <const int *P> struct ConstObjPtr {};
template <int S::*P> struct MemPtr {};
template <void (S::*P)()> struct MemFnPtr {};
template <auto V> struct Auto {};
template <typename T> using FnPtrAlias = FnPtr<fn>;
template <auto V> using AutoAlias = Auto<V>;

static_assert(template_arguments_of(^^FnPtr<&fn>)[0] ==
              std::meta::reflect_constant(&fn));
static_assert(template_arguments_of(^^FnPtr<fn>)[0] ==
              std::meta::reflect_constant(&fn));
static_assert(is_value(template_arguments_of(^^FnPtr<&fn>)[0]));
static_assert(type_of(template_arguments_of(^^FnPtr<&fn>)[0]) ==
              ^^void (*)(int));
static_assert(template_arguments_of(^^FnPtr<fn_noexcept>)[0] ==
              std::meta::reflect_constant<void (*)(int)>(&fn_noexcept));

static_assert(template_arguments_of(^^FnRef<fn>)[0] == ^^fn);
static_assert(is_function(template_arguments_of(^^FnRef<fn>)[0]));
static_assert(template_arguments_of(^^FnRef<fn>)[0] ==
              std::meta::reflect_function(fn));

static_assert(template_arguments_of(^^ObjPtr<&obj>)[0] ==
              std::meta::reflect_constant(&obj));
static_assert(is_value(template_arguments_of(^^ObjPtr<&obj>)[0]));
static_assert(type_of(template_arguments_of(^^ObjPtr<&obj>)[0]) == ^^int *);
static_assert(template_arguments_of(^^ObjPtr<arr>)[0] ==
              std::meta::reflect_constant(+arr));
static_assert(template_arguments_of(^^ObjPtr<&arr[1]>)[0] ==
              std::meta::reflect_constant(&arr[1]));
static_assert(template_arguments_of(^^ConstObjPtr<&obj>)[0] ==
              std::meta::reflect_constant<const int *>(&obj));
static_assert(template_arguments_of(^^ObjPtr<nullptr>)[0] ==
              std::meta::reflect_constant<int *>(nullptr));
static_assert(is_value(template_arguments_of(^^ObjPtr<nullptr>)[0]));

static_assert(template_arguments_of(^^ObjRef<obj>)[0] ==
              std::meta::reflect_object(obj));
static_assert(is_object(template_arguments_of(^^ObjRef<obj>)[0]));
static_assert(template_arguments_of(^^ObjRef<obj>)[0] == object_of(^^obj));
static_assert(template_arguments_of(^^ObjRef<S::sm>)[0] ==
              std::meta::reflect_object(S::sm));

static_assert(template_arguments_of(^^MemPtr<&S::m>)[0] ==
              std::meta::reflect_constant(&S::m));
static_assert(is_value(template_arguments_of(^^MemPtr<&S::m>)[0]));
static_assert(type_of(template_arguments_of(^^MemPtr<&S::m>)[0]) ==
              ^^int S::*);
static_assert(template_arguments_of(^^MemFnPtr<&S::mf>)[0] ==
              std::meta::reflect_constant(&S::mf));
static_assert(template_arguments_of(^^MemPtr<nullptr>)[0] ==
              std::meta::reflect_constant<int S::*>(nullptr));

static_assert(template_arguments_of(^^Auto<&fn>)[0] ==
              std::meta::reflect_constant(&fn));
static_assert(is_value(template_arguments_of(^^Auto<&fn>)[0]));
static_assert(template_arguments_of(^^Auto<&obj>)[0] ==
              std::meta::reflect_constant(&obj));
static_assert(template_arguments_of(^^Auto<&S::sm>)[0] ==
              std::meta::reflect_constant(&S::sm));
static_assert(template_arguments_of(^^Auto<&S::m>)[0] ==
              std::meta::reflect_constant(&S::m));
static_assert(template_arguments_of(^^Auto<&S::sf>)[0] ==
              std::meta::reflect_constant(&S::sf));

// The same through alias template specializations.
static_assert(template_arguments_of(^^AutoAlias<&fn>)[0] ==
              std::meta::reflect_constant(&fn));
static_assert(template_arguments_of(^^AutoAlias<&obj>)[0] ==
              std::meta::reflect_constant(&obj));
static_assert(template_arguments_of(dealias(^^FnPtrAlias<int>))[0] ==
              std::meta::reflect_constant(&fn));

// Splicing the value or entity back.
static_assert([:template_arguments_of(^^FnPtr<&fn>)[0]:] == &fn);
static_assert(&[:template_arguments_of(^^FnRef<fn>)[0]:] == &fn);
static_assert([:template_arguments_of(^^ObjPtr<&obj>)[0]:] == &obj);
static_assert(&[:template_arguments_of(^^ObjRef<obj>)[0]:] == &obj);
static_assert([:template_arguments_of(^^MemPtr<&S::m>)[0]:] == &S::m);
}  // namespace pointer_and_reference_parameters

                   // =======================================
                   // bb_clang_p2996_issue_41_regression_test
                   // =======================================

namespace bb_clang_p2996_issue_41_regression_test {
template<class T> struct TCls {};

TCls<int> obj1;
TCls<decltype(obj1)> obj2;

static_assert(
        has_template_arguments(template_arguments_of(^^decltype(obj2))[0]) ==
        has_template_arguments(^^TCls<int>));
}  // namespace bb_clang_p2996_issue_41_regression_test

                   // =======================================
                   // bb_clang_p2996_issue_54_regression_test
                   // =======================================

namespace bb_clang_p2996_issue_54_regression_test {
template <auto R> void fn() { }

void fn() {
    class S { S(); ~S(); };
    fn<(members_of(^^S, ctx) |
            std::views::filter(std::meta::is_constructor)).front()>();
    fn<(members_of(^^S, ctx) |
            std::views::filter(std::meta::is_destructor)).front()>();
}
}  // namespace bb_clang_p2996_issue_54_regression_test

int main() { }
