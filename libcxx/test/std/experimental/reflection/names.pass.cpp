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


static_assert(u8display_string_of(^^::) == u8"(global-namespace)");
static_assert(display_string_of(^^::) == "(global-namespace)");
static_assert(!has_identifier(^^::));

static_assert(!has_identifier(^^int));

static_assert(u8display_string_of(std::meta::reflect_value(3)) ==
              u8"(value : int)");
static_assert(display_string_of(std::meta::reflect_value(3)) == "3");
static_assert(u8display_string_of(^^int) == u8"int");
static_assert(display_string_of(^^int) == "int");

static_assert(identifier_of(^^std::string) == "string");


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

static_assert(identifier_of(^^Alias) == "Alias");
static_assert(has_identifier(^^Alias));

static_assert(identifier_of(^^var) == "var");
static_assert(display_string_of(^^var) == "var");
static_assert(has_identifier(^^var));

static_assert(identifier_of(^^fn) == "fn");
static_assert(has_identifier(^^fn));

static_assert(identifier_of(^^TCls) == "TCls");
static_assert(has_identifier(^^TCls));

static_assert(!has_identifier(^^TCls<int>));

static_assert(identifier_of(^^TFn) == "TFn");
static_assert(display_string_of(^^TFn) == "TFn");
static_assert(has_identifier(^^TFn));

static_assert(identifier_of(^^TAlias) == "TAlias");
static_assert(has_identifier(^^TAlias));
static_assert(!has_identifier(^^TAlias<int>));

static_assert(identifier_of(^^TVar) == "TVar");
static_assert(has_identifier(^^TVar));
static_assert(!has_identifier(^^TVar<int>));

static_assert(identifier_of(^^Concept) == "Concept");
static_assert(has_identifier(^^Concept));

static_assert(identifier_of(^^Enum) == "Enum");
static_assert(has_identifier(^^Enum));

static_assert(identifier_of(^^Enum::A) == "A");
static_assert(has_identifier(^^Enum::A));

static_assert(identifier_of(^^EnumCls) == "EnumCls");
static_assert(has_identifier(^^EnumCls));

static_assert(identifier_of(^^EnumCls::A) == "A");
static_assert(has_identifier(^^EnumCls::A));

static_assert(!has_identifier(^^decltype([] {})));

static_assert(!has_identifier(template_arguments_of(^^TFn<int, 0, TCls>)[0]));
static_assert(!has_identifier(template_arguments_of(^^TFn<int, 0, TCls>)[1]));
static_assert(identifier_of(template_arguments_of(^^TFn<int, 0, TCls>)[2]) ==
              "TCls");
static_assert(!has_identifier(template_arguments_of(^^WithTypePack<int>)[0]));
static_assert(display_string_of(^^::) == "(global-namespace)");
static_assert(display_string_of(^^TCls) == "TCls");
static_assert(display_string_of(^^TAlias) == "TAlias");
static_assert(display_string_of(^^TVar) == "TVar");
static_assert(display_string_of(^^Concept) == "Concept");
static_assert(display_string_of(^^Enum) == "Enum");
static_assert(display_string_of(^^Enum::A) == "A");
static_assert(display_string_of(^^EnumCls) == "EnumCls");
static_assert(display_string_of(^^EnumCls::A) == "A");
static_assert(
        display_string_of(template_arguments_of(^^TFn<int, 0, TCls>)[0]) ==
        "int");
static_assert(
        display_string_of(template_arguments_of(^^TFn<int, 0, TCls>)[1]) ==
        "0");
static_assert(
        display_string_of(template_arguments_of(^^TFn<int, 0, TCls>)[2]) ==
        "TCls");
static_assert(
        display_string_of(template_arguments_of(^^WithTypePack<int>)[0]) ==
        "int");
static_assert(display_string_of(template_arguments_of(^^WithAutoPack<3>)[0]) ==
              "3");
static_assert(display_string_of(^^decltype([] {})) == "(anonymous type)");


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
static_assert(identifier_of(^^Cls) == "Cls");
static_assert(!has_identifier(bases_of(^^Cls)[0]));
static_assert(identifier_of(^^Cls::Alias) == "Alias");
static_assert(has_identifier(^^Cls::Alias));
static_assert(identifier_of(^^Cls::mem) == "mem");
static_assert(identifier_of(^^Cls::memfn) == "memfn");
static_assert(identifier_of(^^Cls::sfn) == "sfn");
static_assert(identifier_of(^^Cls::Inner) == "Inner");

static_assert(
    !has_identifier((members_of(^^Cls) |
         std::views::filter(std::meta::is_constructor)).front()));
static_assert(
    !has_identifier((members_of(^^Cls) |
         std::views::filter(std::meta::is_constructor_template)).front()));
static_assert(
    !has_identifier((members_of(^^Cls) |
         std::views::filter(std::meta::is_destructor)).front()));
static_assert(!has_identifier(^^Cls::operator bool));
static_assert(
    !has_identifier(
        (members_of(^^Cls) |
             std::views::filter(std::meta::is_template) |
             std::ranges::to<std::vector>())[5]));
static_assert(identifier_of(^^Cls::TInner) == "TInner");
static_assert(identifier_of(^^Cls::TMemFn) == "TMemFn");
static_assert(identifier_of(^^Cls::TAlias) == "TAlias");
static_assert(identifier_of(^^Cls::TSVar) == "TSVar");
static_assert(identifier_of(^^Cls::Enum) == "Enum");
static_assert(identifier_of(^^Cls::Enum::B) == "B");
static_assert(identifier_of(^^Cls::EnumCls) == "EnumCls");
static_assert(identifier_of(^^Cls::EnumCls::B) == "B");
static_assert(display_string_of(^^Cls) == "Cls");
static_assert(display_string_of(bases_of(^^Cls)[0]) == "Base");
static_assert(display_string_of(^^Cls::Alias) == "Alias");
static_assert(display_string_of(^^Cls::mem) == "mem");
static_assert(display_string_of(^^Cls::memfn) == "memfn");
static_assert(display_string_of(^^Cls::sfn) == "sfn");
static_assert(display_string_of(^^Cls::Inner) == "Inner");
static_assert(
    (members_of(^^Cls) |
         std::views::filter(std::meta::is_constructor) |
         std::views::filter(std::meta::is_user_provided) |
         std::views::transform(std::meta::display_string_of) |
         std::ranges::to<std::vector>()) ==
    std::vector<std::string_view>{"Cls"});
static_assert(
    (members_of(^^Cls) |
         std::views::filter(std::meta::is_constructor_template) |
         std::views::transform(std::meta::display_string_of) |
         std::ranges::to<std::vector>()) ==
    std::vector<std::string_view>{"Cls"});
static_assert(
    (members_of(^^Cls) |
         std::views::filter(std::meta::is_destructor) |
         std::views::transform(std::meta::display_string_of) |
         std::ranges::to<std::vector>()) ==
    std::vector<std::string_view>{"~Cls"});
static_assert(display_string_of(^^Cls::operator bool) == "operator bool");
static_assert(
    (members_of(^^Cls) |
         std::views::filter(std::meta::is_template) |
         std::views::transform(std::meta::display_string_of) |
         std::ranges::to<std::vector>())[5] ==
    "(conversion-function-template)");
static_assert(display_string_of(^^Cls::TInner) == "TInner");
static_assert(display_string_of(^^Cls::TMemFn) == "TMemFn");
static_assert(display_string_of(^^Cls::TAlias) == "TAlias");
static_assert(display_string_of(^^Cls::TSVar) == "TSVar");
static_assert(display_string_of(^^Cls::Enum) == "Enum");
static_assert(display_string_of(^^Cls::Enum::B) == "B");
static_assert(display_string_of(^^Cls::EnumCls) == "EnumCls");
static_assert(display_string_of(^^Cls::EnumCls::B) == "B");



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

int operator""_a(const char *);
template <char...> int operator""_b();
}  // namespace myns
static_assert(identifier_of(^^myns) == "myns");
static_assert(identifier_of(^^myns::mem) == "mem");
static_assert(identifier_of(^^myns::memfn) == "memfn");
static_assert(identifier_of(^^myns::Cls) == "Cls");
static_assert(identifier_of(^^myns::TInner) == "TInner");
static_assert(identifier_of(^^myns::TFn) == "TFn");
static_assert(identifier_of(^^myns::TAlias) == "TAlias");
static_assert(identifier_of(^^myns::TVar) == "TVar");
static_assert(identifier_of(^^myns::Concept) == "Concept");
static_assert(identifier_of(^^myns::Enum) == "Enum");
static_assert(identifier_of(^^myns::Enum::C) == "C");
static_assert(identifier_of(^^myns::EnumCls) == "EnumCls");
static_assert(identifier_of(^^myns::EnumCls::C) == "C");
static_assert(identifier_of(^^myns::operator""_a) == "_a");
static_assert(identifier_of(^^myns::operator""_b) == "_b");
static_assert(display_string_of(^^myns) == "myns");
static_assert(display_string_of(^^myns::mem) == "mem");
static_assert(display_string_of(^^myns::memfn) == "memfn");
static_assert(display_string_of(^^myns::Cls) == "Cls");
static_assert(display_string_of(^^myns::TInner) == "TInner");
static_assert(display_string_of(^^myns::TFn) == "TFn");
static_assert(display_string_of(^^myns::TAlias) == "TAlias");
static_assert(display_string_of(^^myns::TVar) == "TVar");
static_assert(display_string_of(^^myns::Concept) == "Concept");
static_assert(display_string_of(^^myns::Enum) == "Enum");
static_assert(display_string_of(^^myns::Enum::C) == "C");
static_assert(display_string_of(^^myns::EnumCls) == "EnumCls");
static_assert(display_string_of(^^myns::EnumCls::C) == "C");
static_assert(display_string_of(^^myns::operator""_a) == R"(operator""_a)");
static_assert(display_string_of(^^myns::operator""_b) == R"(operator""_b)");

class K\u{00FC}hl1 {};

static_assert(identifier_of(^^Kühl1) == "Kühl1");
static_assert(display_string_of(^^Kühl1) == "Kühl1");
static_assert(u8identifier_of(^^Kühl1) == u8"Kühl1");
static_assert(u8display_string_of(^^Kühl1) == u8"Kühl1");

class Kühl2 {};
static_assert(identifier_of(^^Kühl2) == "Kühl2");
static_assert(display_string_of(^^Kühl2) == "Kühl2");
static_assert(u8identifier_of(^^Kühl2) == u8"Kühl2");
static_assert(u8display_string_of(^^Kühl2) == u8"Kühl2");

namespace Ops {
struct S {
  void *operator new(size_t);

  operator bool();

  template <typename T> S& operator-(T &);
};
int operator+(const S&, const S&);
int operator,(const S&, const S&);

static_assert(display_string_of(^^operator+) == "operator+");
static_assert(display_string_of(^^operator,) == "operator,");
static_assert(display_string_of(^^S::operator-) == "operator-");
static_assert(display_string_of(^^S::operator new) == "operator new");
}  // namespace Ops

namespace DataMemberSpecs {
constexpr auto a = data_member_spec(^^int, {});
constexpr auto b = data_member_spec(^^int, {.name=""});
constexpr auto c = data_member_spec(^^int, {.name="ident"});
constexpr auto d = data_member_spec(^^int, {.name=u8"ident"});

static_assert(!has_identifier(a));
static_assert(!has_identifier(b));
static_assert(has_identifier(c));
static_assert(has_identifier(d));

static_assert(identifier_of(c) == "ident");
static_assert(identifier_of(d) == "ident");

static_assert(u8identifier_of(c) == u8"ident");
static_assert(u8identifier_of(d) == u8"ident");

}  // namespace DataMemberSpecs


int main() { }
