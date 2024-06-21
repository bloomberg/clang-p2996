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


static_assert(name_of(^::) == u8"");
static_assert(name_of<std::string_view>(^::) == "");
static_assert(name_of(std::meta::reflect_value(3)) == u8"");
static_assert(name_of<std::string_view>(std::meta::reflect_value(3)) == "");
static_assert(name_of(^int) == u8"int");
static_assert(name_of<std::string_view>(^int) == "int");
static_assert(display_name_of(^::) == u8"");
static_assert(display_name_of<std::string_view>(^::) == "");
static_assert(display_name_of(std::meta::reflect_value(3)) == u8"");
static_assert(display_name_of<std::string_view>(std::meta::reflect_value(3)) ==
              "");
static_assert(display_name_of<std::string_view>(^int) == "int");


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

static_assert(name_of(^Alias) == u8"Alias");
static_assert(name_of(^var) == u8"var");
static_assert(name_of(^fn) == u8"fn");
static_assert(name_of(^TCls) == u8"TCls");
static_assert(name_of(^TCls<int>) == u8"TCls");
static_assert(name_of(^TFn) == u8"TFn");
static_assert(name_of(^TFn<int, 0, TCls>) == u8"TFn");
static_assert(name_of(^TAlias) == u8"TAlias");
static_assert(name_of(^TAlias<int>) == u8"TAlias");
static_assert(name_of(^TVar) == u8"TVar");
static_assert(name_of(^TVar<int>) == u8"TVar");
static_assert(name_of(^Concept) == u8"Concept");
static_assert(name_of(^Enum) == u8"Enum");
static_assert(name_of(^Enum::A) == u8"A");
static_assert(name_of(^EnumCls) == u8"EnumCls");
static_assert(name_of(^EnumCls::A) == u8"A");
static_assert(name_of(template_arguments_of(^TFn<int, 0, TCls>)[0]) == u8"int");
static_assert(name_of(template_arguments_of(^TFn<int, 0, TCls>)[1]) == u8"");
static_assert(name_of(template_arguments_of(^TFn<int, 0, TCls>)[2]) ==
              u8"TCls");
static_assert(name_of(template_arguments_of(^WithTypePack<int>)[0]) == u8"int");
static_assert(name_of(template_arguments_of(^WithAutoPack<3>)[0]) == u8"");
static_assert(display_name_of(^::) == u8"");
static_assert(display_name_of(^var) == u8"var");
static_assert(display_name_of(^fn) == u8"fn");
static_assert(display_name_of(^TCls) == u8"TCls");
static_assert(display_name_of(^TFn) == u8"TFn");
static_assert(display_name_of(^TAlias) == u8"TAlias");
static_assert(display_name_of(^TVar) == u8"TVar");
static_assert(display_name_of(^Concept) == u8"Concept");
static_assert(display_name_of(^Enum) == u8"Enum");
static_assert(display_name_of(^Enum::A) == u8"A");
static_assert(display_name_of(^EnumCls) == u8"EnumCls");
static_assert(display_name_of(^EnumCls::A) == u8"A");
static_assert(display_name_of(template_arguments_of(^TFn<int, 0, TCls>)[0]) ==
              u8"int");
static_assert(display_name_of(template_arguments_of(^TFn<int, 0, TCls>)[1]) ==
              u8"");
static_assert(display_name_of(template_arguments_of(^TFn<int, 0, TCls>)[2]) ==
              u8"TCls");
static_assert(display_name_of(template_arguments_of(^WithTypePack<int>)[0]) ==
              u8"int");
static_assert(display_name_of(template_arguments_of(^WithAutoPack<3>)[0]) ==
              u8"");


struct Base {};
struct Cls : Base {
  using Alias = int;

  int mem;
  void memfn() const;
  static void sfn();
  struct Inner { int inner_mem; };

  Cls();
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
static_assert(name_of(^Cls) == u8"Cls");
static_assert(name_of(bases_of(^Cls)[0]) == u8"Base");
static_assert(name_of(^Cls::Alias) == u8"Alias");
static_assert(name_of(^Cls::mem) == u8"mem");
static_assert(name_of(^Cls::memfn) == u8"memfn");
static_assert(name_of(^Cls::sfn) == u8"sfn");
static_assert(name_of(^Cls::Inner) == u8"Inner");
static_assert(name_of(members_of(^Cls, std::meta::is_constructor)[0]) ==
              u8"Cls");
static_assert(name_of(members_of(^Cls, std::meta::is_constructor)[1]) ==
              u8"Cls");
static_assert(name_of(members_of(^Cls, std::meta::is_destructor)[0]) ==
              u8"~Cls");
static_assert(name_of(^Cls::operator bool) == u8"operator bool");
static_assert(name_of(members_of(^Cls, std::meta::is_template)[5]) ==
              u8"operator int");
static_assert(name_of(^Cls::TInner) == u8"TInner");
static_assert(name_of(^Cls::TMemFn) == u8"TMemFn");
static_assert(name_of(^Cls::TAlias) == u8"TAlias");
static_assert(name_of(^Cls::TSVar) == u8"TSVar");
static_assert(name_of(^Cls::Enum) == u8"Enum");
static_assert(name_of(^Cls::Enum::B) == u8"B");
static_assert(name_of(^Cls::EnumCls) == u8"EnumCls");
static_assert(name_of(^Cls::EnumCls::B) == u8"B");
static_assert(display_name_of(^Cls) == u8"Cls");
static_assert(display_name_of(bases_of(^Cls)[0]) == u8"Base");
static_assert(display_name_of(^Cls::Alias) == u8"Alias");
static_assert(display_name_of(^Cls::mem) == u8"mem");
static_assert(display_name_of(^Cls::memfn) == u8"memfn");
static_assert(display_name_of(^Cls::sfn) == u8"sfn");
static_assert(display_name_of(^Cls::Inner) == u8"Inner");
static_assert(display_name_of(members_of(^Cls,
                              std::meta::is_constructor)[0]) == u8"Cls");
static_assert(display_name_of(members_of(^Cls,
                              std::meta::is_constructor)[1]) == u8"Cls");
static_assert(display_name_of(members_of(^Cls,
                              std::meta::is_destructor)[0]) == u8"~Cls");
static_assert(display_name_of(^Cls::operator bool) == u8"operator bool");
static_assert(display_name_of(members_of(^Cls, std::meta::is_template)[5]) ==
              u8"operator int");
static_assert(display_name_of(^Cls::TInner) == u8"TInner");
static_assert(display_name_of(^Cls::TMemFn) == u8"TMemFn");
static_assert(display_name_of(^Cls::TAlias) == u8"TAlias");
static_assert(display_name_of(^Cls::TSVar) == u8"TSVar");
static_assert(display_name_of(^Cls::Enum) == u8"Enum");
static_assert(display_name_of(^Cls::Enum::B) == u8"B");
static_assert(display_name_of(^Cls::EnumCls) == u8"EnumCls");
static_assert(display_name_of(^Cls::EnumCls::B) == u8"B");



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
static_assert(name_of(^myns) == u8"myns");
static_assert(name_of(^myns::mem) == u8"mem");
static_assert(name_of(^myns::memfn) == u8"memfn");
static_assert(name_of(^myns::Cls) == u8"Cls");
static_assert(name_of(^myns::TInner) == u8"TInner");
static_assert(name_of(^myns::TFn) == u8"TFn");
static_assert(name_of(^myns::TAlias) == u8"TAlias");
static_assert(name_of(^myns::TVar) == u8"TVar");
static_assert(name_of(^myns::Concept) == u8"Concept");
static_assert(name_of(^myns::Enum) == u8"Enum");
static_assert(name_of(^myns::Enum::C) == u8"C");
static_assert(name_of(^myns::EnumCls) == u8"EnumCls");
static_assert(name_of(^myns::EnumCls::C) == u8"C");
static_assert(display_name_of(^myns) == u8"myns");
static_assert(display_name_of(^myns::mem) == u8"mem");
static_assert(display_name_of(^myns::memfn) == u8"memfn");
static_assert(display_name_of(^myns::Cls) == u8"Cls");
static_assert(display_name_of(^myns::TInner) == u8"TInner");
static_assert(display_name_of(^myns::TFn) == u8"TFn");
static_assert(display_name_of(^myns::TAlias) == u8"TAlias");
static_assert(display_name_of(^myns::TVar) == u8"TVar");
static_assert(display_name_of(^myns::Concept) == u8"Concept");
static_assert(display_name_of(^myns::Enum) == u8"Enum");
static_assert(display_name_of(^myns::Enum::C) == u8"C");
static_assert(display_name_of(^myns::EnumCls) == u8"EnumCls");
static_assert(display_name_of(^myns::EnumCls::C) == u8"C");

class K\u{00FC}hl1 {};

static_assert(name_of<std::string_view>(^Kühl1) == "K\\u{00FC}hl1");
static_assert(display_name_of<std::string_view>(^Kühl1) == "Kühl1");
static_assert(name_of(^Kühl1) == u8"Kühl1");
static_assert(display_name_of(^Kühl1) == u8"Kühl1");

class Kühl2 {};
static_assert(name_of<std::string_view>(^Kühl2) == "K\\u{00FC}hl2");
static_assert(display_name_of<std::string_view>(^Kühl2) == "Kühl2");
static_assert(name_of(^Kühl2) == u8"Kühl2");
static_assert(display_name_of(^Kühl2) == u8"Kühl2");


int main() { }
