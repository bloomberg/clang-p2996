//===----------------------------------------------------------------------===//
//
// Copyright 2025 Bloomberg Finance L.P.
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

struct A {
  virtual void foo();

  virtual void bar() final;

  virtual ~A() = default;
};

struct B final : A {
  void foo() override;

  ~B() final = default;
};


struct C : A {
  void foo() final;
};

struct D final : A {
  static_assert (std::meta::is_final (^^D));
  // Easy to confuse is_final with is_final_type, which is really asking for completeness
  static_assert (!std::meta::is_final_type(^^D));
};

static_assert (!std::meta::is_final (^^A));
static_assert (std::meta::is_final (^^B));
static_assert (!std::meta::is_final (^^C));

static_assert (!std::meta::is_final (^^A::foo));
static_assert (!std::meta::is_final (^^B::foo));
static_assert (std::meta::is_final (^^C::foo));

static_assert (std::meta::is_final (^^A::bar));
static_assert (std::meta::is_final (^^B::bar));
static_assert (std::meta::is_final (^^C::bar));

static_assert (!std::meta::is_final (^^A::~A));
static_assert (std::meta::is_final (^^B::~B));
static_assert (!std::meta::is_final (^^C::~C));

A a;
B b;
C c;

static_assert (!std::meta::is_final (^^::));
static_assert (!std::meta::is_final (^^a));
static_assert (!std::meta::is_final (^^b));
static_assert (!std::meta::is_final (^^c));

static_assert (!std::meta::is_final (std::meta::type_of (^^a)));
static_assert (std::meta::is_final (std::meta::type_of (^^b)));
static_assert (!std::meta::is_final (std::meta::type_of (^^c)));
