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


static_assert(u8name_of(^::) == u8"");
static_assert(name_of(^::) == "");
static_assert(u8name_of(std::meta::reflect_value(3)) == u8"");
static_assert(name_of(std::meta::reflect_value(3)) == "");
static_assert(u8name_of(^int) == u8"int");
static_assert(name_of(^int) == "int");
static_assert(u8display_name_of(^::) == u8"");
static_assert(display_name_of(^::) == "");
static_assert(u8display_name_of(std::meta::reflect_value(3)) == u8"");
static_assert(display_name_of(std::meta::reflect_value(3)) == "");
static_assert(u8display_name_of(^int) == u8"int");
static_assert(display_name_of(^int) == "int");


using Alias = int;
int var;
void fn();
[[maybe_unused]] void sfn();
template <typename> struct TCls {};
template <typename P, auto P2, template<typename> class P3> void TFn();
template <typename> using TAlias = int;
template <typename> int TVar = 0;
template <typename> concept Concept = requires { true; };
template <typename...> struct WithTypePack {};
template <auto...> struct WithAutoPack {};
enum Enum { A };
enum class EnumCls { A };

static_assert(name_of(^Alias) == "Alias");
static_assert(name_of(^var) == "var");
static_assert(name_of(^fn) == "fn");
static_assert(name_of(^TCls) == "TCls");
static_assert(name_of(^TCls<int>) == "TCls");
static_assert(name_of(^TFn) == "TFn");
static_assert(name_of(^TFn<int, 0, TCls>) == "TFn");
static_assert(name_of(^TAlias) == "TAlias");
static_assert(name_of(^TAlias<int>) == "TAlias");
static_assert(name_of(^TVar) == "TVar");
static_assert(name_of(^TVar<int>) == "TVar");
static_assert(name_of(^Concept) == "Concept");
static_assert(name_of(^Enum) == "Enum");
static_assert(name_of(^Enum::A) == "A");
static_assert(name_of(^EnumCls) == "EnumCls");
static_assert(name_of(^EnumCls::A) == "A");
static_assert(name_of(template_arguments_of(^TFn<int, 0, TCls>)[0]) == "int");
static_assert(name_of(template_arguments_of(^TFn<int, 0, TCls>)[1]) == "");
static_assert(name_of(template_arguments_of(^TFn<int, 0, TCls>)[2]) == "TCls");
static_assert(name_of(template_arguments_of(^WithTypePack<int>)[0]) == "int");
static_assert(name_of(template_arguments_of(^WithAutoPack<3>)[0]) == "");
static_assert(display_name_of(^::) == "");
static_assert(display_name_of(^var) == "var");
static_assert(display_name_of(^fn) == "fn");
static_assert(display_name_of(^TCls) == "TCls");
static_assert(display_name_of(^TFn) == "TFn");
static_assert(display_name_of(^TAlias) == "TAlias");
static_assert(display_name_of(^TVar) == "TVar");
static_assert(display_name_of(^Concept) == "Concept");
static_assert(display_name_of(^Enum) == "Enum");
static_assert(display_name_of(^Enum::A) == "A");
static_assert(display_name_of(^EnumCls) == "EnumCls");
static_assert(display_name_of(^EnumCls::A) == "A");
static_assert(display_name_of(template_arguments_of(^TFn<int, 0, TCls>)[0]) ==
              "int");
static_assert(display_name_of(template_arguments_of(^TFn<int, 0, TCls>)[1]) ==
              "");
static_assert(display_name_of(template_arguments_of(^TFn<int, 0, TCls>)[2]) ==
              "TCls");
static_assert(display_name_of(template_arguments_of(^WithTypePack<int>)[0]) ==
              "int");
static_assert(display_name_of(template_arguments_of(^WithAutoPack<3>)[0]) ==
              "");


struct Base {};
struct Cls : Base {
  using Alias = int;

  int mem;
  void memfn() const;
  static void sfn();
  struct Inner { int inner_mem; };

  Cls() {}
  template <typename T> Cls();
  ~Cls();

  operator bool();

  template <typename> struct TInner {};
  template <typename> void TMemFn();
  template <typename> using TAlias = int;
  template <typename> static constexpr int TSVar = 0;
  template <typename> operator int();

  enum Enum { B };
  enum class EnumCls { B };
};
static_assert(name_of(^Cls) == "Cls");
static_assert(name_of(bases_of(^Cls)[0]) == "Base");
static_assert(name_of(^Cls::Alias) == "Alias");
static_assert(name_of(^Cls::mem) == "mem");
static_assert(name_of(^Cls::memfn) == "memfn");
static_assert(name_of(^Cls::sfn) == "sfn");
static_assert(name_of(^Cls::Inner) == "Inner");
static_assert(
    (members_of(^Cls) |
         std::views::filter(std::meta::is_constructor) |
         std::views::filter(std::meta::is_user_provided) |
         std::views::transform(std::meta::name_of) |
         std::ranges::to<std::vector>()) ==
    std::vector<std::string_view>{"Cls"});
static_assert(
    (members_of(^Cls) |
         std::views::filter(std::meta::is_constructor) |
         std::views::filter(std::meta::is_template) |
         std::views::transform(std::meta::name_of) |
         std::ranges::to<std::vector>()) ==
    std::vector<std::string_view>{"Cls"});
static_assert(
    (members_of(^Cls) |
         std::views::filter(std::meta::is_destructor) |
         std::views::transform(std::meta::name_of) |
         std::ranges::to<std::vector>()) ==
    std::vector<std::string_view>{"~Cls"});
static_assert(name_of(^Cls::operator bool) == "operator bool");
static_assert(
    (members_of(^Cls) |
         std::views::filter(std::meta::is_template) |
         std::views::transform(std::meta::name_of) |
         std::ranges::to<std::vector>())[5] == "operator int");
static_assert(name_of(^Cls::TInner) == "TInner");
static_assert(name_of(^Cls::TMemFn) == "TMemFn");
static_assert(name_of(^Cls::TAlias) == "TAlias");
static_assert(name_of(^Cls::TSVar) == "TSVar");
static_assert(name_of(^Cls::Enum) == "Enum");
static_assert(name_of(^Cls::Enum::B) == "B");
static_assert(name_of(^Cls::EnumCls) == "EnumCls");
static_assert(name_of(^Cls::EnumCls::B) == "B");
static_assert(display_name_of(^Cls) == "Cls");
static_assert(display_name_of(bases_of(^Cls)[0]) == "Base");
static_assert(display_name_of(^Cls::Alias) == "Alias");
static_assert(display_name_of(^Cls::mem) == "mem");
static_assert(display_name_of(^Cls::memfn) == "memfn");
static_assert(display_name_of(^Cls::sfn) == "sfn");
static_assert(display_name_of(^Cls::Inner) == "Inner");
static_assert(
    (members_of(^Cls) |
         std::views::filter(std::meta::is_constructor) |
         std::views::filter(std::meta::is_user_provided) |
         std::views::transform(std::meta::display_name_of) |
         std::ranges::to<std::vector>()) ==
    std::vector<std::string_view>{"Cls"});
static_assert(
    (members_of(^Cls) |
         std::views::filter(std::meta::is_constructor) |
         std::views::filter(std::meta::is_template) |
         std::views::transform(std::meta::display_name_of) |
         std::ranges::to<std::vector>()) ==
    std::vector<std::string_view>{"Cls"});
static_assert(
    (members_of(^Cls) |
         std::views::filter(std::meta::is_destructor) |
         std::views::transform(std::meta::display_name_of) |
         std::ranges::to<std::vector>()) ==
    std::vector<std::string_view>{"~Cls"});
static_assert(display_name_of(^Cls::operator bool) == "operator bool");
static_assert(
    (members_of(^Cls) |
         std::views::filter(std::meta::is_template) |
         std::views::transform(std::meta::display_name_of) |
         std::ranges::to<std::vector>())[5] == "operator int");
static_assert(display_name_of(^Cls::TInner) == "TInner");
static_assert(display_name_of(^Cls::TMemFn) == "TMemFn");
static_assert(display_name_of(^Cls::TAlias) == "TAlias");
static_assert(display_name_of(^Cls::TSVar) == "TSVar");
static_assert(display_name_of(^Cls::Enum) == "Enum");
static_assert(display_name_of(^Cls::Enum::B) == "B");
static_assert(display_name_of(^Cls::EnumCls) == "EnumCls");
static_assert(display_name_of(^Cls::EnumCls::B) == "B");



namespace myns {
int mem;
void memfn();
struct Cls { int mem; };

template <typename> struct TInner {};
template <typename> void TFn();
template <typename> using TAlias = int;
template <typename> int TVar = 0;
template <typename> concept Concept = requires { true; };

enum Enum { C };
enum class EnumCls { C };
}  // namespace myns
static_assert(name_of(^myns) == "myns");
static_assert(name_of(^myns::mem) == "mem");
static_assert(name_of(^myns::memfn) == "memfn");
static_assert(name_of(^myns::Cls) == "Cls");
static_assert(name_of(^myns::TInner) == "TInner");
static_assert(name_of(^myns::TFn) == "TFn");
static_assert(name_of(^myns::TAlias) == "TAlias");
static_assert(name_of(^myns::TVar) == "TVar");
static_assert(name_of(^myns::Concept) == "Concept");
static_assert(name_of(^myns::Enum) == "Enum");
static_assert(name_of(^myns::Enum::C) == "C");
static_assert(name_of(^myns::EnumCls) == "EnumCls");
static_assert(name_of(^myns::EnumCls::C) == "C");
static_assert(display_name_of(^myns) == "myns");
static_assert(display_name_of(^myns::mem) == "mem");
static_assert(display_name_of(^myns::memfn) == "memfn");
static_assert(display_name_of(^myns::Cls) == "Cls");
static_assert(display_name_of(^myns::TInner) == "TInner");
static_assert(display_name_of(^myns::TFn) == "TFn");
static_assert(display_name_of(^myns::TAlias) == "TAlias");
static_assert(display_name_of(^myns::TVar) == "TVar");
static_assert(display_name_of(^myns::Concept) == "Concept");
static_assert(display_name_of(^myns::Enum) == "Enum");
static_assert(display_name_of(^myns::Enum::C) == "C");
static_assert(display_name_of(^myns::EnumCls) == "EnumCls");
static_assert(display_name_of(^myns::EnumCls::C) == "C");

class K\u{00FC}hl1 {};

static_assert(name_of(^Kühl1) == "K\\u{00FC}hl1");
static_assert(display_name_of(^Kühl1) == "Kühl1");
static_assert(u8name_of(^Kühl1) == u8"Kühl1");
static_assert(u8display_name_of(^Kühl1) == u8"Kühl1");

class Kühl2 {};
static_assert(name_of(^Kühl2) == "K\\u{00FC}hl2");
static_assert(display_name_of(^Kühl2) == "Kühl2");
static_assert(u8name_of(^Kühl2) == u8"Kühl2");
static_assert(u8display_name_of(^Kühl2) == u8"Kühl2");


int main() { }
