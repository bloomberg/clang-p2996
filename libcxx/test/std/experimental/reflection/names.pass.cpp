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

#include <meta>
#include <ranges>

constexpr auto ctx = std::meta::access_context::unchecked();

static_assert(u8display_string_of(^^::) == u8"(global-namespace)");
static_assert(display_string_of(^^::) == "(global-namespace)");
static_assert(!has_identifier(^^::));

static_assert(!has_identifier(^^int));

static_assert(u8display_string_of(std::meta::reflect_constant(3)) ==
              u8"(value : int)");
static_assert(display_string_of(std::meta::reflect_constant(3)) == "3");
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
static_assert(has_identifier(bases_of(^^Cls, ctx)[0]));
static_assert(identifier_of(bases_of(^^Cls, ctx)[0]) == "Base");
static_assert(identifier_of(^^Cls::Alias) == "Alias");
static_assert(has_identifier(^^Cls::Alias));
static_assert(identifier_of(^^Cls::mem) == "mem");
static_assert(identifier_of(^^Cls::memfn) == "memfn");
static_assert(identifier_of(^^Cls::sfn) == "sfn");
static_assert(identifier_of(^^Cls::Inner) == "Inner");

static_assert(
    !has_identifier((members_of(^^Cls, ctx) |
         std::views::filter(std::meta::is_constructor)).front()));
static_assert(
    !has_identifier((members_of(^^Cls, ctx) |
         std::views::filter(std::meta::is_constructor_template)).front()));
static_assert(
    !has_identifier((members_of(^^Cls, ctx) |
         std::views::filter(std::meta::is_destructor)).front()));
static_assert(!has_identifier(^^Cls::operator bool));
static_assert(
    !has_identifier(
        (members_of(^^Cls, ctx) |
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
static_assert(display_string_of(bases_of(^^Cls, ctx)[0]) == "Base");
static_assert(display_string_of(^^Cls::Alias) == "Alias");
static_assert(display_string_of(^^Cls::mem) == "mem");
static_assert(display_string_of(^^Cls::memfn) == "memfn");
static_assert(display_string_of(^^Cls::sfn) == "sfn");
static_assert(display_string_of(^^Cls::Inner) == "Inner");
static_assert(
    (members_of(^^Cls, ctx) |
         std::views::filter(std::meta::is_constructor) |
         std::views::filter(std::meta::is_user_provided) |
         std::views::transform(std::meta::display_string_of) |
         std::ranges::to<std::vector>()) ==
    std::vector<std::string_view>{"Cls"});
static_assert(
    (members_of(^^Cls, ctx) |
         std::views::filter(std::meta::is_constructor_template) |
         std::views::transform(std::meta::display_string_of) |
         std::ranges::to<std::vector>()) ==
    std::vector<std::string_view>{"Cls"});
static_assert(
    (members_of(^^Cls, ctx) |
         std::views::filter(std::meta::is_destructor) |
         std::views::transform(std::meta::display_string_of) |
         std::ranges::to<std::vector>()) ==
    std::vector<std::string_view>{"~Cls"});
static_assert(display_string_of(^^Cls::operator bool) == "operator bool");
static_assert(
    (members_of(^^Cls, ctx) |
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

template <typename... Ts>
consteval bool param_pack_has_ident([[maybe_unused]] Ts...ts) {
  return (has_identifier(^^ts) || ...);
}
static_assert(!param_pack_has_ident<int, bool, char>(3, false, 'c'));


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
static_assert(identifier_of(template_of(^^myns::operator""_b<'a'>)) == "_b");
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
static_assert(display_string_of(template_of(^^myns::operator""_b<'a'>)) == R"(operator""_b)");

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

  S &operator^=(const S &);
  int operator co_await();
};
int operator+(const S&, const S&);
int operator,(const S&, const S&);

static_assert(display_string_of(^^operator+) == "operator+");
static_assert(display_string_of(^^operator,) == "operator,");
static_assert(display_string_of(template_of(^^S::operator-<int>)) == "operator-");
static_assert(display_string_of(^^S::operator new) == "operator new");
static_assert(display_string_of(^^S::operator^=) == "operator^=");
static_assert(display_string_of(^^S::operator co_await) == "operator co_await");

// [meta.reflection.operators], Table "Operator representations".
using std::meta::operators;
static_assert(symbol_of(operators::op_co_await) == "co_await");
static_assert(u8symbol_of(operators::op_co_await) == u8"co_await");
static_assert(symbol_of(operators::op_caret) == "^");
static_assert(u8symbol_of(operators::op_caret) == u8"^");
static_assert(symbol_of(operators::op_caret_equals) == "^=");
static_assert(u8symbol_of(operators::op_caret_equals) == u8"^=");
static_assert(symbol_of(operators::op_ampersand_equals) == "&=");
static_assert(symbol_of(operators::op_comma) == ",");
static_assert(operator_of(^^S::operator co_await) == operators::op_co_await);
static_assert(operator_of(^^S::operator^=) == operators::op_caret_equals);
}  // namespace Ops

namespace DataMemberSpecs {
// A description without a name must describe a bit-field
// ([meta.reflection.define.aggregate]/5.3).
constexpr auto a = data_member_spec(^^int, {.bit_width=3});
constexpr auto c = data_member_spec(^^int, {.name="ident"});
constexpr auto d = data_member_spec(^^int, {.name=u8"ident"});

static_assert(!has_identifier(a));
static_assert(has_identifier(c));
static_assert(has_identifier(d));

static_assert(identifier_of(c) == "ident");
static_assert(identifier_of(d) == "ident");

static_assert(u8identifier_of(c) == u8"ident");
static_assert(u8identifier_of(d) == u8"ident");

}  // namespace DataMemberSpecs


                          // ==========================
                          // [meta.reflection.names]/1
                          // ==========================

namespace names_p1 {
struct S {};
enum E { e };
using CI = const int;
using CS = const S;
using CE = const E;
template <typename T> using TAlias = T;

// /1.1: an unnamed class or enumeration with a typedef name for linkage
// purposes has that name as its identifier.
typedef struct { int z; } TL;
typedef enum { tle } TLE;
typedef struct { int z; } *PTL;  // no typedef name for linkage purposes
static_assert(^^TL != dealias(^^TL));
static_assert(has_identifier(dealias(^^TL)));
static_assert(identifier_of(dealias(^^TL)) == "TL");
static_assert(u8identifier_of(dealias(^^TL)) == u8"TL");
static_assert(has_identifier(dealias(^^TLE)));
static_assert(identifier_of(dealias(^^TLE)) == "TLE");
static_assert(has_identifier(type_of(^^tle)));
static_assert(identifier_of(type_of(^^tle)) == "TLE");
static_assert(!has_identifier(dealias(^^PTL)));
static_assert(!has_identifier(remove_pointer(dealias(^^PTL))));
static_assert(!has_identifier(^^const TL));

// /1.3: a type alias has an identifier, even when the aliased type is
// cv-qualified.
static_assert(has_identifier(^^CI));
static_assert(identifier_of(^^CI) == "CI");
static_assert(has_identifier(^^CS));
static_assert(identifier_of(^^CS) == "CS");
static_assert(has_identifier(^^CE));
static_assert(!has_identifier(^^TAlias<const S>));

// /1.4: a cv-qualified class or enumeration type has no identifier.
static_assert(has_identifier(^^S));
static_assert(has_identifier(^^E));
static_assert(!has_identifier(^^const S));
static_assert(!has_identifier(^^volatile S));
static_assert(!has_identifier(^^const volatile S));
static_assert(!has_identifier(^^const E));
static_assert(!has_identifier(^^const CS));
static_assert(!has_identifier(^^volatile CS));
static_assert(!has_identifier(^^const CI));
static_assert(!has_identifier(^^const TAlias<S>));
static_assert(has_identifier(dealias(^^CS)) == has_identifier(^^const S));

// /1.5, /1.6, /3.2: a literal operator (template) is not an operator
// function (template); its identifier is its ud-suffix.
int operator""_a(const char *);
template <char...> int operator""_b();
static_assert(!is_operator_function(^^operator""_a));
static_assert(is_literal_operator(^^operator""_a));
static_assert(has_identifier(^^operator""_a));
static_assert(identifier_of(^^operator""_a) == "_a");
static_assert(u8identifier_of(^^operator""_a) == u8"_a");
static_assert(!is_operator_function_template(template_of(^^operator""_b<'a'>)));
static_assert(is_literal_operator_template(template_of(^^operator""_b<'a'>)));
static_assert(has_identifier(template_of(^^operator""_b<'a'>)));
static_assert(identifier_of(template_of(^^operator""_b<'a'>)) == "_b");
static_assert(!has_identifier(^^operator""_b<'a'>));

// /1.12, /3.5: a direct base class relationship has the identifier of the
// type of its base class.
struct Base {};
using BaseAlias = Base;
template <typename> struct TBase {};
typedef struct { } TLBase;
struct D : BaseAlias, TBase<int>, TLBase {};
static_assert(has_identifier(bases_of(^^D, ctx)[0]));
static_assert(identifier_of(bases_of(^^D, ctx)[0]) == "Base");
static_assert(u8identifier_of(bases_of(^^D, ctx)[0]) == u8"Base");
static_assert(!has_identifier(bases_of(^^D, ctx)[1]));
static_assert(has_identifier(bases_of(^^D, ctx)[2]));
static_assert(identifier_of(bases_of(^^D, ctx)[2]) == "TLBase");
}  // namespace names_p1

int main() { }
