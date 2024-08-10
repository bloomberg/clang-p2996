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

using info = decltype(^int);


template <typename T1, typename T2>
constexpr bool is_same_v = false;

template <typename T1>
constexpr bool is_same_v<T1, T1> = true;

template <typename T1, typename T2>
concept same_as = is_same_v<T1, T2>;

template <bool B, class T = void>
struct enable_if {};

template <class T>
struct enable_if<true, T> { typedef T type; };

template <bool B, class T = void>
using enable_if_t = typename enable_if<B,T>::type;

                                 // ===========
                                 // idempotency
                                 // ===========

// Check idempotency of the splice operator composed with reflection.
namespace idempotency {
static_assert(is_same_v<typename [:^int:], int>);
static_assert(is_same_v<typename [:^const int:], const int>);
static_assert(!is_same_v<typename [:^const int:], int>);
}  // namespace idempotency

                                   // =======
                                   // aliases
                                   // =======

// Check use of splices in alias definitions.
namespace aliases {
constexpr info r_int_alias = ^int, r_const_int_alias = ^const int;
using int_alias = [:r_int_alias:];
using const_int_alias = [:r_const_int_alias:];
static_assert(is_same_v<typename [:r_int_alias:], int>);
static_assert(is_same_v<typename [:r_int_alias:], int_alias>);
static_assert(is_same_v<typename [:r_const_int_alias:], const int>);
static_assert(is_same_v<typename [:r_const_int_alias:], const_int_alias>);
static_assert(!is_same_v<typename [:r_int_alias:],
                         typename [:r_const_int_alias:]>);
}  // namespace aliases

                                  // ========
                                  // in_decls
                                  // ========

// Check use of splices in declarations.
namespace in_decls {
constexpr info r_const_int = ^const int;
constexpr [:r_const_int:] x = 42;
static_assert(is_same_v<decltype(x), const int>);
static_assert(x == 42);
}  // namespace in_decls

                                 // ==========
                                 // in_fn_defs
                                 // ==========

// Check use of splices in function return and parameter types.
namespace in_fn_defs {
constexpr info r_int = ^int;
constexpr info r_const_int = ^const int;
consteval typename [:r_const_int:] incr(typename [:r_int:] p) {
  return p + 1;
}
static_assert(incr(14) == 15);

consteval auto decr(typename [:r_int:] p) -> [:r_const_int:] {
  return p - 1;
}
static_assert(decr(13) == 12);

// 'typename' should be optional in parameter declarations.
void fn([:r_int:]);
void fn(typename [:r_int:]);
class S {
  S([:r_int:]);
  void fn([:r_int:]);
};
}  // in_fn_defs

                                   // ======
                                   // in_nns
                                   // ======

// Check use of splices as leading components of nested name specifiers.
namespace in_nns {
struct S {
  struct Inner {
    int k;

    using type = int;
    static constexpr int s_value = 1;
  } I;
  using type = bool;
  static constexpr int s_value = 2;
};
constexpr auto r_S = ^S;
constexpr auto r_S_Inner = ^S::Inner;
static_assert(is_same_v<[:r_S:]::type, bool>);
static_assert(is_same_v<[:r_S_Inner:]::type, int>);
static_assert(is_same_v<int [:r_S:]::*, int S::*>);
static_assert(is_same_v<int ([:r_S:]::*)(bool), int (S::*)(bool)>);
static_assert([:r_S:]::s_value == 2);
static_assert([:r_S_Inner:]::s_value == 1);

// Check splice as leading component of nested name specifier appearing in the
// RHS of a member access expression.
consteval int fn(int p, int q) {
  S s;
  s.I.[:r_S:]::Inner::k = p;
  s.I.[:r_S_Inner:]::k += q;
  return s.I.k;
}
static_assert(fn(3, 4) == 7);

// Check splice as leading component of nested name specifier appearing in the
// lookup of an inner class type or static data member.
template <info T, typename ExpectedMemberTypedef, int ExpectedMemberValue>
consteval bool fn_with_dependent() {
  return (is_same_v<typename [:T:]::type, ExpectedMemberTypedef>) &&
         ([:T:]::s_value == ExpectedMemberValue);
}
static_assert(fn_with_dependent<^S, bool, 2>());
static_assert(fn_with_dependent<^S::Inner, int, 1>());

// Check splice as leading component of nested name specifier appearing in the
// RHS of a member access expression when the reflection is value-dependent on
// a template parameter name.
template <info T>
consteval int fn_with_dependent() {
  S s;
  s.I.[:T:]::k = 13;

  return s.I.k;
}
static_assert(fn_with_dependent<^S::Inner>() == 13);

// Check composition of splice and reflect operators to obtain a reflection to
// an inner class belonging to a reflected class, in which the reflection is
// value-dependent on a template parameter name.
template <info T>
consteval info getReflectionOfInnerCls() {
  return ^typename [:T:]::Inner;
}
static_assert(getReflectionOfInnerCls<^S>() == ^S::Inner);
}  // namespace in_nns

                               // ===============
                               // with_enum_types
                               // ===============

// Check splicing of enum types.
namespace with_enum_types {
enum Enum { A, B, C };
enum class EnumCls { A, B, C };

constexpr info rEnum = ^Enum, rEnumCls = ^EnumCls;
static_assert([:rEnum:]::B == B);
static_assert([:rEnumCls:]::B == EnumCls::B);
static_assert(static_cast<Enum>([:rEnumCls:]::B) == [:rEnum:]::B);

// Splicing of a dependent reflection of an enum class.
template <info R> consteval int bval() { return int([:R:]::B); }
static_assert(bval<^Enum>() == bval<^EnumCls>());

// using-enum-declaration with a splice.
namespace {
using enum [:rEnumCls:];
static_assert(int(C) == 2);
}  // namespace

// using-enum-declaration with a qualified-id having a splice as the leading
// nested-name-specifier.
namespace {
constexpr auto rns = ^with_enum_types;
using enum [:rns:]::EnumCls;
static_assert(int(B) == 1);
}  // namespace
}  // namespace with_enum_types

                             // ===================
                             // friend_declarations
                             // ===================

namespace friend_declarations {
struct S;

template <decltype(^::) FriendCls>
class T {
  static constexpr int value = 13;

  friend [:FriendCls:];
};

struct S {
  static constexpr auto value = T<^S>::value;
};

static_assert(S::value == 13);
}  // namespace friend_declarations

                            // =====================
                            // base_class_specifiers
                            // =====================

namespace base_class_specifiers {
struct B1 { static constexpr int value1 = 1; };
struct B2 { static constexpr int value2 = 2; };
struct B3 { static constexpr int value3 = 3; };

struct S : public [:^B1:] {};
static_assert(S::value1 == 1);

template <decltype(^::)... Rs>
struct T : [:Rs:]... {};

using A = T<^B1, ^B2, ^B3>;
static_assert(A::value1 + A::value2 + A::value3 == 6);
}  // namespace base_class_specifiers

                               // ===============
                               // requires_clause
                               // ===============

namespace requires_clause {
struct Addable {
  friend Addable operator+(const Addable l, const Addable r) {
    return {};
  }
};

struct NonAddable {};

namespace Namespace {
  using requires_clause::Addable;
  using requires_clause::NonAddable;
}

struct HasNested {
  struct Nested {};
};

struct NoNested {};

template<typename T, typename = enable_if_t<is_same_v<float, T>>>
struct RequiresFloat {};

template<typename T, typename = enable_if_t<is_same_v<float, T>>>
using AliasFloat = T;


// Simple requirement
template <typename T>
constexpr auto simple_addable = requires(T a, T b) { [:^a:] + b; };
template <typename T>
constexpr auto simple_addable2 = requires(T a, T b) { a + [:^b:]; };
template <typename T>
constexpr auto simple_addable3 = requires(T a, T b) { [:^a:] + [:^b:]; };
template <typename T>
constexpr auto simple_addable_nns = 
  requires(T b) { [:^Namespace:]::Addable() + b; };
constexpr auto simple_addable_nns2 = requires { [:^Namespace:]::Addable(); };
template <typename T>
constexpr auto simple_dep_nns = requires { typename [:^T:]::Nested(); };

static_assert(simple_addable<Addable>);
static_assert(!simple_addable<NonAddable>);

static_assert(simple_addable2<Addable>);
static_assert(!simple_addable2<NonAddable>);

static_assert(simple_addable3<Addable>);
static_assert(!simple_addable3<NonAddable>);

static_assert(simple_addable_nns<Addable>);
static_assert(!simple_addable_nns<NonAddable>);

static_assert(simple_addable_nns2);

static_assert(simple_dep_nns<HasNested>);
static_assert(!simple_dep_nns<NoNested>);

// Type requirement
template <typename T>
constexpr auto type = requires { typename [:^T:]; };

template <typename T>
constexpr auto type_nested = requires { typename [:^T:]::Nested; };

template <typename T>
constexpr auto type_class_is_float = 
  requires { typename RequiresFloat<[:^T:]>; };

template <typename T>
constexpr auto type_class_is_float2 = 
  requires { typename [:^RequiresFloat<T>:]; };

template <typename T>
constexpr auto type_alias_is_float = 
  requires { typename AliasFloat<[:^T:]>; };

template <typename T>
constexpr auto type_alias_is_float2 = 
  requires { typename [:^AliasFloat<T>:]; };

static_assert(type<int>);

static_assert(type_nested<HasNested>);
static_assert(!type_nested<NoNested>);

static_assert(type_class_is_float<float>);
static_assert(!type_class_is_float<int>);
static_assert(type_class_is_float2<float>);
static_assert(!type_class_is_float2<int>);

static_assert(type_alias_is_float<float>);
static_assert(!type_alias_is_float<int>);
static_assert(type_alias_is_float2<float>);
static_assert(!type_alias_is_float2<int>);

// Compound requirements
template <typename T>
constexpr auto compound_returns_addable = 
  requires { {typename [:^T:]()} -> same_as<Addable>; };

template <typename T>
constexpr auto compound_returns_addable2 =
  requires { {T()} -> same_as<[:^Addable:]>; };

template <typename T>
constexpr auto compound_returns_addable3 = 
  requires { {[:^Namespace:]::Addable()} -> same_as<[:^T:]>; };

template <typename T>
constexpr auto compound_returns_addable4 = 
  requires { {T()} -> same_as<[:^Namespace:]::Addable>; };

static_assert(compound_returns_addable<Addable>);
static_assert(!compound_returns_addable<NonAddable>);

static_assert(compound_returns_addable2<Addable>);
static_assert(!compound_returns_addable2<NonAddable>);

static_assert(compound_returns_addable3<Addable>);
static_assert(!compound_returns_addable3<NonAddable>);

static_assert(compound_returns_addable4<Addable>);
static_assert(!compound_returns_addable4<NonAddable>);

// Nested requirements
template <typename T>
constexpr auto nested_addable = requires { 
  requires same_as<T, [:^Addable:]>;
};
template <typename T>
constexpr auto nested_addable2 = requires { 
  requires same_as<T, [:^Namespace:]::Addable>;
};

static_assert(nested_addable<Addable>);
static_assert(!nested_addable<NonAddable>);

static_assert(nested_addable2<Addable>);
static_assert(!nested_addable2<NonAddable>);

} // namespace requires_clause
