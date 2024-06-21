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
#include <tuple>

struct type {};
using alias = type;

int var;
void func();

template <typename T> struct IncompleteTCls;
template <typename T> struct TCls {};
template <typename T> void TFn();
template <typename T> concept TConcept = requires { true; };
template <typename T> int TVar;
template <typename T> using TClsAlias = TCls<T>;

namespace ns {}
namespace ns_alias = ns;

enum Enum { A };
enum class EnumCls { A };

constexpr std::meta::info null_reflection;
static_assert(!is_type(null_reflection));
static_assert(!is_incomplete_type(null_reflection));
static_assert(!is_alias(null_reflection));
static_assert(!is_function(null_reflection));
static_assert(!is_variable(null_reflection));
static_assert(!is_namespace(null_reflection));
static_assert(!is_template(null_reflection));
static_assert(!is_function_template(null_reflection));
static_assert(!is_variable_template(null_reflection));
static_assert(!is_class_template(null_reflection));
static_assert(!is_alias_template(null_reflection));
static_assert(!is_concept(null_reflection));
static_assert(!is_base(null_reflection));
static_assert(!is_value(null_reflection));
static_assert(!is_object(null_reflection));

static_assert(!is_type(std::meta::reflect_value(3)));
static_assert(!is_incomplete_type(std::meta::reflect_value(3)));
static_assert(!is_alias(std::meta::reflect_value(3)));
static_assert(!is_function(std::meta::reflect_value(3)));
static_assert(!is_variable(std::meta::reflect_value(3)));
static_assert(!is_namespace(std::meta::reflect_value(3)));
static_assert(!is_template(std::meta::reflect_value(3)));
static_assert(!is_function_template(std::meta::reflect_value(3)));
static_assert(!is_variable_template(std::meta::reflect_value(3)));
static_assert(!is_class_template(std::meta::reflect_value(3)));
static_assert(!is_alias_template(std::meta::reflect_value(3)));
static_assert(!is_concept(std::meta::reflect_value(3)));
static_assert(!is_base(std::meta::reflect_value(3)));
static_assert(is_value(std::meta::reflect_value(3)));
static_assert(!is_object(std::meta::reflect_value(3)));

static_assert(is_type(^type));
static_assert(!is_incomplete_type(^type));
static_assert(!is_alias(^type));
static_assert(!is_function(^type));
static_assert(!is_variable(^type));
static_assert(!is_template(^type));
static_assert(!is_namespace(^type));
static_assert(!is_function_template(^type));
static_assert(!is_variable_template(^type));
static_assert(!is_class_template(^type));
static_assert(!is_alias_template(^type));
static_assert(!is_concept(^type));
static_assert(!is_base(^type));
static_assert(!is_value(^type));
static_assert(!is_object(^type));

static_assert(!is_type(^func));
static_assert(!is_incomplete_type(^func));
static_assert(!is_alias(^func));
static_assert(is_function(^func));
static_assert(!is_variable(^func));
static_assert(!is_template(^func));
static_assert(!is_namespace(^func));
static_assert(!is_function_template(^func));
static_assert(!is_variable_template(^func));
static_assert(!is_class_template(^func));
static_assert(!is_alias_template(^func));
static_assert(!is_concept(^func));
static_assert(!is_base(^func));
static_assert(!is_value(^func));
static_assert(!is_object(^func));
static_assert(is_user_provided(^func));

static_assert(is_type(^alias));
static_assert(!is_incomplete_type(^alias));
static_assert(is_alias(^alias));
static_assert(!is_function(^alias));
static_assert(!is_variable(^alias));
static_assert(!is_template(^alias));
static_assert(!is_namespace(^alias));
static_assert(!is_function_template(^alias));
static_assert(!is_variable_template(^alias));
static_assert(!is_class_template(^alias));
static_assert(!is_alias_template(^alias));
static_assert(!is_concept(^alias));
static_assert(!is_base(^alias));
static_assert(!is_value(^alias));
static_assert(!is_object(^alias));

static_assert(!is_type(^var));
static_assert(!is_incomplete_type(^var));
static_assert(!is_alias(^var));
static_assert(!is_function(^var));
static_assert(is_variable(^var));
static_assert(!is_template(^var));
static_assert(!is_namespace(^var));
static_assert(!is_function_template(^var));
static_assert(!is_variable_template(^var));
static_assert(!is_class_template(^var));
static_assert(!is_alias_template(^var));
static_assert(!is_concept(^var));
static_assert(!is_base(^var));
static_assert(!is_value(^var));
static_assert(is_object(^var));

static_assert(!is_type(^TCls));
static_assert(!is_incomplete_type(^TCls));
static_assert(!is_alias(^TCls));
static_assert(!is_function(^TCls));
static_assert(!is_variable(^TCls));
static_assert(is_template(^TCls));
static_assert(!is_namespace(^TCls));
static_assert(!is_function_template(^TCls));
static_assert(!is_variable_template(^TCls));
static_assert(is_class_template(^TCls));
static_assert(!is_alias_template(^TCls));
static_assert(!is_concept(^TCls));
static_assert(!is_base(^TCls));
static_assert(!is_value(^TCls));
static_assert(!is_object(^TCls));

static_assert(is_type(^IncompleteTCls<int>));
static_assert(is_incomplete_type(^IncompleteTCls<int>));
static_assert(!is_alias(^IncompleteTCls<int>));
static_assert(!is_function(^IncompleteTCls<int>));
static_assert(!is_variable(^IncompleteTCls<int>));
static_assert(!is_template(^IncompleteTCls<int>));
static_assert(!is_namespace(^IncompleteTCls<int>));
static_assert(!is_function_template(^IncompleteTCls<int>));
static_assert(!is_variable_template(^IncompleteTCls<int>));
static_assert(!is_class_template(^IncompleteTCls<int>));
static_assert(!is_alias_template(^IncompleteTCls<int>));
static_assert(!is_concept(^IncompleteTCls<int>));
static_assert(!is_base(^IncompleteTCls<int>));
static_assert(!is_value(^IncompleteTCls<int>));
static_assert(!is_object(^IncompleteTCls<int>));

static_assert(is_type(^TCls<int>));
static_assert(!is_incomplete_type(^TCls<int>));
static_assert(!is_alias(^TCls<int>));
static_assert(!is_function(^TCls<int>));
static_assert(!is_variable(^TCls<int>));
static_assert(!is_template(^TCls<int>));
static_assert(!is_namespace(^TCls<int>));
static_assert(!is_function_template(^TCls<int>));
static_assert(!is_variable_template(^TCls<int>));
static_assert(!is_class_template(^TCls<int>));
static_assert(!is_alias_template(^TCls<int>));
static_assert(!is_concept(^TCls<int>));
static_assert(!is_base(^TCls<int>));
static_assert(!is_value(^TCls<int>));
static_assert(!is_object(^TCls<int>));

static_assert(!is_type(^TFn));
static_assert(!is_incomplete_type(^TFn));
static_assert(!is_alias(^TFn));
static_assert(!is_function(^TFn));
static_assert(!is_variable(^TFn));
static_assert(is_template(^TFn));
static_assert(!is_namespace(^TFn));
static_assert(is_function_template(^TFn));
static_assert(!is_variable_template(^TFn));
static_assert(!is_class_template(^TFn));
static_assert(!is_alias_template(^TFn));
static_assert(!is_concept(^TFn));
static_assert(!is_base(^TFn));
static_assert(!is_value(^TFn));
static_assert(!is_object(^TFn));

static_assert(!is_type(^TFn<int>));
static_assert(!is_incomplete_type(^TFn<int>));
static_assert(!is_alias(^TFn<int>));
static_assert(is_function(^TFn<int>));
static_assert(!is_variable(^TFn<int>));
static_assert(!is_template(^TFn<int>));
static_assert(!is_namespace(^TFn<int>));
static_assert(!is_function_template(^TFn<int>));
static_assert(!is_variable_template(^TFn<int>));
static_assert(!is_class_template(^TFn<int>));
static_assert(!is_alias_template(^TFn<int>));
static_assert(!is_concept(^TFn<int>));
static_assert(!is_base(^TFn<int>));
static_assert(!is_value(^TFn<int>));
static_assert(!is_object(^TFn<int>));
static_assert(is_user_provided(^TFn<int>));

static_assert(!is_type(^TConcept));
static_assert(!is_incomplete_type(^TConcept));
static_assert(!is_alias(^TConcept));
static_assert(!is_function(^TConcept));
static_assert(!is_variable(^TConcept));
static_assert(is_template(^TConcept));
static_assert(!is_namespace(^TConcept));
static_assert(!is_function_template(^TConcept));
static_assert(!is_variable_template(^TConcept));
static_assert(!is_class_template(^TConcept));
static_assert(!is_alias_template(^TConcept));
static_assert(is_concept(^TConcept));
static_assert(!is_base(^TConcept));
static_assert(!is_value(^TConcept));
static_assert(!is_object(^TConcept));

static_assert(!is_type(substitute(^TConcept,{^int})));
static_assert(!is_incomplete_type(substitute(^TConcept, {^int})));
static_assert(!is_alias(substitute(^TConcept, {^int})));
static_assert(!is_function(substitute(^TConcept, {^int})));
static_assert(!is_variable(substitute(^TConcept, {^int})));
static_assert(!is_template(substitute(^TConcept, {^int})));
static_assert(!is_namespace(substitute(^TConcept, {^int})));
static_assert(!is_function_template(substitute(^TConcept, {^int})));
static_assert(!is_variable_template(substitute(^TConcept, {^int})));
static_assert(!is_class_template(substitute(^TConcept, {^int})));
static_assert(!is_alias_template(substitute(^TConcept, {^int})));
static_assert(!is_concept(substitute(^TConcept, {^int})));
static_assert(!is_base(substitute(^TConcept, {^int})));
static_assert(is_value(substitute(^TConcept, {^int})));
static_assert(!is_object(substitute(^TConcept, {^int})));

static_assert(!is_type(^TVar));
static_assert(!is_incomplete_type(^TVar));
static_assert(!is_alias(^TVar));
static_assert(!is_function(^TVar));
static_assert(!is_variable(^TVar));
static_assert(is_template(^TVar));
static_assert(!is_namespace(^TVar));
static_assert(!is_function_template(^TVar));
static_assert(is_variable_template(^TVar));
static_assert(!is_class_template(^TVar));
static_assert(!is_alias_template(^TVar));
static_assert(!is_concept(^TVar));
static_assert(!is_base(^TVar));
static_assert(!is_value(^TVar));
static_assert(!is_object(^TVar));

static_assert(!is_type(^TVar<int>));
static_assert(!is_incomplete_type(^TVar<int>));
static_assert(!is_alias(^TVar<int>));
static_assert(!is_function(^TVar<int>));
static_assert(is_variable(^TVar<int>));
static_assert(!is_template(^TVar<int>));
static_assert(!is_namespace(^TVar<int>));
static_assert(!is_function_template(^TVar<int>));
static_assert(!is_variable_template(^TVar<int>));
static_assert(!is_class_template(^TVar<int>));
static_assert(!is_alias_template(^TVar<int>));
static_assert(!is_concept(^TVar<int>));
static_assert(!is_base(^TVar<int>));
static_assert(!is_value(^TVar<int>));
static_assert(is_object(^TVar<int>));

static_assert(!is_type(^TClsAlias));
static_assert(!is_incomplete_type(^TClsAlias));
static_assert(is_alias(^TClsAlias));
static_assert(!is_function(^TClsAlias));
static_assert(!is_variable(^TClsAlias));
static_assert(is_template(^TClsAlias));
static_assert(!is_namespace(^TClsAlias));
static_assert(!is_function_template(^TClsAlias));
static_assert(!is_variable_template(^TClsAlias));
static_assert(!is_class_template(^TClsAlias));
static_assert(is_alias_template(^TClsAlias));
static_assert(!is_concept(^TClsAlias));
static_assert(!is_base(^TClsAlias));
static_assert(!is_value(^TClsAlias));
static_assert(!is_object(^TClsAlias));

static_assert(is_type(^TClsAlias<int>));
static_assert(!is_incomplete_type(^TClsAlias<int>));
static_assert(is_alias(^TClsAlias<int>));
static_assert(!is_function(^TClsAlias<int>));
static_assert(!is_variable(^TClsAlias<int>));
static_assert(!is_template(^TClsAlias<int>));
static_assert(!is_namespace(^TClsAlias<int>));
static_assert(!is_function_template(^TClsAlias<int>));
static_assert(!is_variable_template(^TClsAlias<int>));
static_assert(!is_class_template(^TClsAlias<int>));
static_assert(!is_alias_template(^TClsAlias<int>));
static_assert(!is_concept(^TClsAlias<int>));
static_assert(!is_base(^TClsAlias<int>));
static_assert(!is_value(^TClsAlias<int>));
static_assert(!is_object(^TClsAlias<int>));

static_assert(!is_type(^::));
static_assert(!is_incomplete_type(^::));
static_assert(!is_alias(^::));
static_assert(!is_function(^::));
static_assert(!is_variable(^::));
static_assert(!is_template(^::));
static_assert(is_namespace(^::));
static_assert(!is_function_template(^::));
static_assert(!is_variable_template(^::));
static_assert(!is_class_template(^::));
static_assert(!is_alias_template(^::));
static_assert(!is_concept(^::));
static_assert(!is_base(^::));
static_assert(!is_value(^::));
static_assert(!is_object(^::));

static_assert(!is_type(^ns));
static_assert(!is_incomplete_type(^ns));
static_assert(!is_alias(^ns));
static_assert(!is_function(^ns));
static_assert(!is_variable(^ns));
static_assert(!is_template(^ns));
static_assert(is_namespace(^ns));
static_assert(!is_function_template(^ns));
static_assert(!is_variable_template(^ns));
static_assert(!is_class_template(^ns));
static_assert(!is_alias_template(^ns));
static_assert(!is_concept(^ns));
static_assert(!is_base(^ns));
static_assert(!is_value(^ns));
static_assert(!is_object(^ns));

static_assert(!is_type(^ns_alias));
static_assert(!is_incomplete_type(^ns_alias));
static_assert(is_alias(^ns_alias));
static_assert(!is_function(^ns_alias));
static_assert(!is_variable(^ns_alias));
static_assert(!is_template(^ns_alias));
static_assert(is_namespace(^ns_alias));
static_assert(!is_function_template(^ns_alias));
static_assert(!is_variable_template(^ns_alias));
static_assert(!is_class_template(^ns_alias));
static_assert(!is_alias_template(^ns_alias));
static_assert(!is_concept(^ns_alias));
static_assert(!is_base(^ns_alias));
static_assert(!is_value(^ns_alias));
static_assert(!is_object(^ns_alias));

static_assert(is_type(^Enum));
static_assert(!is_incomplete_type(^Enum));
static_assert(!is_alias(^Enum));
static_assert(!is_function(^Enum));
static_assert(!is_variable(^Enum));
static_assert(!is_template(^Enum));
static_assert(!is_namespace(^Enum));
static_assert(!is_function_template(^Enum));
static_assert(!is_variable_template(^Enum));
static_assert(!is_class_template(^Enum));
static_assert(!is_alias_template(^Enum));
static_assert(!is_concept(^Enum));
static_assert(!is_base(^Enum));
static_assert(!is_value(^Enum));
static_assert(!is_object(^Enum));

static_assert(!is_type(^Enum::A));
static_assert(!is_incomplete_type(^Enum::A));
static_assert(!is_alias(^Enum::A));
static_assert(!is_function(^Enum::A));
static_assert(!is_variable(^Enum::A));
static_assert(!is_template(^Enum::A));
static_assert(!is_namespace(^Enum::A));
static_assert(!is_function_template(^Enum::A));
static_assert(!is_variable_template(^Enum::A));
static_assert(!is_class_template(^Enum::A));
static_assert(!is_alias_template(^Enum::A));
static_assert(!is_concept(^Enum::A));
static_assert(!is_base(^Enum::A));
static_assert(!is_value(^Enum::A));
static_assert(!is_object(^Enum::A));

static_assert(is_type(^EnumCls));
static_assert(!is_incomplete_type(^EnumCls));
static_assert(!is_alias(^EnumCls));
static_assert(!is_function(^EnumCls));
static_assert(!is_variable(^EnumCls));
static_assert(!is_template(^EnumCls));
static_assert(!is_namespace(^EnumCls));
static_assert(!is_function_template(^EnumCls));
static_assert(!is_variable_template(^EnumCls));
static_assert(!is_class_template(^EnumCls));
static_assert(!is_alias_template(^EnumCls));
static_assert(!is_concept(^EnumCls));
static_assert(!is_base(^EnumCls));
static_assert(!is_value(^EnumCls));
static_assert(!is_object(^EnumCls));

static_assert(!is_type(^EnumCls::A));
static_assert(!is_incomplete_type(^EnumCls::A));
static_assert(!is_alias(^EnumCls::A));
static_assert(!is_function(^EnumCls::A));
static_assert(!is_variable(^EnumCls::A));
static_assert(!is_template(^EnumCls::A));
static_assert(!is_namespace(^EnumCls::A));
static_assert(!is_function_template(^EnumCls::A));
static_assert(!is_variable_template(^EnumCls::A));
static_assert(!is_class_template(^EnumCls::A));
static_assert(!is_alias_template(^EnumCls::A));
static_assert(!is_concept(^EnumCls::A));
static_assert(!is_base(^EnumCls::A));
static_assert(!is_value(^EnumCls::A));
static_assert(!is_object(^EnumCls::A));

struct incomplete_type;
using incomplete_alias = incomplete_type;
static_assert(is_incomplete_type(^incomplete_type));
static_assert(is_incomplete_type(^incomplete_alias));
struct incomplete_type {};
static_assert(!is_incomplete_type(^incomplete_type));
static_assert(!is_incomplete_type(^incomplete_alias));

template <typename T> using IncompleteTClsAlias = IncompleteTCls<T>;
static_assert(is_incomplete_type(^IncompleteTCls<int>));
static_assert(is_incomplete_type(^IncompleteTClsAlias<int>));
template <typename T> struct IncompleteTCls {};
static_assert(!is_incomplete_type(^IncompleteTCls<int>));
static_assert(!is_incomplete_type(^IncompleteTClsAlias<int>));

struct Base {};
struct Derived : Base {};
static_assert(!is_base(^Base));
static_assert(!is_type(bases_of(^Derived)[0]));
static_assert(is_base(bases_of(^Derived)[0]));

                               // =====================
                               // test_is_structured_binding_and_related_edge_cases
                               // =====================
namespace test_is_structured_binding_and_related_edge_cases {
static int struct_binding_case1[] = {1, 2, 3};
auto [x1, y1, z1]                 = struct_binding_case1;
static_assert(is_structured_binding(^x1));
static_assert(is_structured_binding(^y1));
static_assert(is_structured_binding(^z1));
static_assert(!is_variable(^x1));
static_assert(!is_variable(^y1));
static_assert(!is_variable(^z1));
static_assert(type_of(^x1) == ^int);
static_assert(type_of(^y1) == ^int);
static_assert(type_of(^z1) == ^int);

auto struct_binding_case2() { return std::make_tuple(1, 2, 3); }
auto [x2, y2, z2] = struct_binding_case2();
static_assert(is_structured_binding(^x2));
static_assert(is_structured_binding(^y2));
static_assert(is_structured_binding(^z2));
static_assert(!is_variable(^x2));
static_assert(!is_variable(^y2));
static_assert(!is_variable(^z2));
// "wrapped" type of each element is 'std::tuple_element<I, std::tuple<int,int,int>>::type',
// where I is index of tuple field
static_assert(type_of(^x2) == ^int);
static_assert(type_of(^y2) == ^int);
static_assert(type_of(^z2) == ^int);

struct StructBinding {
  int a, b, c;
};
auto struct_binding_case3() { return StructBinding{1, 2, 3}; }
auto [x3, y3, z3] = struct_binding_case3();
static_assert(is_structured_binding(^x3));
static_assert(is_structured_binding(^y3));
static_assert(is_structured_binding(^z3));
static_assert(!is_variable(^x3));
static_assert(!is_variable(^y3));
static_assert(!is_variable(^z3));
static_assert(type_of(^x3) == ^int);
static_assert(type_of(^y3) == ^int);
static_assert(type_of(^z3) == ^int);

static_assert(!is_structured_binding(^var));
static_assert(!is_structured_binding(std::meta::reflect_value(3)));
} // namespace test_is_structured_binding_and_related_edge_cases

                            // =====================
                            // test_is_user_provided
                            // =====================

namespace test_is_user_provided {
struct S1 {};
struct S2 { S2() = default; };
struct S3 { S3(); };
S3::S3() {}

static_assert(!is_user_provided(members_of(^S1, std::meta::is_constructor)[0]));
static_assert(!is_user_provided(members_of(^S2, std::meta::is_constructor)[0]));
static_assert(is_user_provided(members_of(^S3, std::meta::is_constructor)[0]));

}  // namespace test_is_user_provided


int main() { }
