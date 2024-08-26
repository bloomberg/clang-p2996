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

// <experimental/reflection>
//
// [reflection]

#include <experimental/meta>


                                // ============
                                // find_type_of
                                // ============

namespace find_type_of {
using int_alias = int;
using int_alias_alias = int_alias;

extern int i;
extern int_alias ia;
extern decltype(int_alias{}) dtia;

static_assert(type_of(^^i) == ^^int);
static_assert(type_of(^^ia) == ^^int_alias);
static_assert(type_of(^^ia) != ^^int);
static_assert(type_of(^^dtia) == ^^int);
static_assert(type_of(^^dtia) != ^^int_alias);
static_assert(^^int_alias != ^^int);
static_assert(^^int_alias != ^^int_alias_alias);

constexpr auto a = data_member_spec(^^int, {});
constexpr auto b = data_member_spec(^^int_alias, {});

static_assert(type_of(a) == ^^int);
static_assert(type_of(b) == ^^int_alias);
}  // namespace expression_type

                              // ================
                              // find_template_of
                              // ================

namespace find_template_of {
template <typename> struct TCls {
  template <typename T> struct TInnerCls {};
};
template <typename> void TFn() {};
template <typename> int TVar = 0;
template <typename> using TAlias = int;
template <typename U> using DependentAlias = TCls<int>::template TInnerCls<U>;

static_assert(template_of(^^TCls<int>) == ^^TCls);
static_assert(template_of(^^TFn<int>) == ^^TFn);
static_assert(template_of(^^TVar<int>) == ^^TVar);
static_assert(template_of(^^TAlias<int>) == ^^TAlias);
static_assert(template_of(^^DependentAlias<bool>) == ^^DependentAlias);
}  // namespace find_template_of

                               // ==============
                               // find_parent_of
                               // ==============

namespace find_parent_of {
struct Cls {
  struct InnerCls {};
  int mem;
  void memfn();
  static void sfn();
  using Alias = int;

  template <typename> static constexpr int TSMem = 0;
  template <typename> void TMemFn();
  template <typename> static void TSMemFn();
  template <typename> struct TInnerCls { int mem; };
  template <typename> using TAlias = int;
};

int var;
void fn();
namespace NestedNS { int var; };

template <typename> constexpr int TVar = 0;
template <typename> void TFn();
template <typename> struct TCls { int var; };
template <typename> using TAlias = int;
template <typename> concept Concept = requires { true; };

static_assert(parent_of(^^find_parent_of) == ^^::);

static_assert(parent_of(^^Cls::InnerCls) == ^^Cls);
static_assert(parent_of(^^Cls::mem) == ^^Cls);
static_assert(parent_of(^^Cls::memfn) == ^^Cls);
static_assert(parent_of(^^Cls::sfn) == ^^Cls);
static_assert(parent_of(^^Cls::Alias) == ^^Cls);

static_assert(parent_of(^^Cls::TSMem) == ^^Cls);
static_assert(parent_of(^^Cls::TSMem<int>) == ^^Cls);
static_assert(parent_of(^^Cls::TMemFn) == ^^Cls);
static_assert(parent_of(^^Cls::TMemFn<int>) == ^^Cls);
static_assert(parent_of(^^Cls::TSMemFn) == ^^Cls);
static_assert(parent_of(^^Cls::TSMemFn<int>) == ^^Cls);
static_assert(parent_of(^^Cls::TInnerCls) == ^^Cls);
static_assert(parent_of(^^Cls::TInnerCls<int>) == ^^Cls);
static_assert(parent_of(^^Cls::TInnerCls<int>::mem) == ^^Cls::TInnerCls<int>);
static_assert(parent_of(^^Cls::TAlias) == ^^Cls);
static_assert(parent_of(^^Cls::TAlias<int>) == ^^Cls);

static_assert(parent_of(^^Cls) == ^^find_parent_of);
static_assert(parent_of(^^var) == ^^find_parent_of);
static_assert(parent_of(^^fn) == ^^find_parent_of);
static_assert(parent_of(^^NestedNS) == ^^find_parent_of);
static_assert(parent_of(^^NestedNS::var) == ^^NestedNS);

static_assert(parent_of(^^TVar) == ^^find_parent_of);
static_assert(parent_of(^^TVar<int>) == ^^find_parent_of);
static_assert(parent_of(^^TFn) == ^^find_parent_of);
static_assert(parent_of(^^TFn<int>) == ^^find_parent_of);
static_assert(parent_of(^^TCls) == ^^find_parent_of);
static_assert(parent_of(^^TCls<int>) == ^^find_parent_of);
static_assert(parent_of(^^TAlias) == ^^find_parent_of);
static_assert(parent_of(^^TAlias<int>) == ^^find_parent_of);
static_assert(parent_of(^^Concept) == ^^find_parent_of);
}  // namespace find_parent_of

                                 // ==========
                                 // dealiasing
                                 // ==========

namespace dealiasing {
using int_alias = int;
using int_alias_alias = int_alias;
template <typename T> using TAlias = T;

namespace NSAlias = dealiasing;
namespace NSAliasAlias = NSAlias;

static_assert(dealias(^^int) == ^^int);
static_assert(dealias(^^int_alias) == ^^int);
static_assert(dealias(^^int_alias_alias) == ^^int);
static_assert(dealias(^^TAlias<int_alias_alias>) == ^^int);

static_assert(dealias(^^::) == ^^::);
static_assert(dealias(^^dealiasing) == ^^dealiasing);
static_assert(dealias(^^NSAlias) == ^^dealiasing);
static_assert(dealias(^^NSAliasAlias) == ^^dealiasing);

static_assert(dealias(std::meta::info{}) == std::meta::info{});
}  // namespace dealiasing


int main() { }
