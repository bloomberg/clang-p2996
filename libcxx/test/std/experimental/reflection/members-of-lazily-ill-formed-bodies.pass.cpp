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
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

// <experimental/reflection>
//
// [reflection]
//
// Regression test: a type-completing metafunction (members_of & co.), when it
// is the FIRST thing to instantiate a class template specialization, must
// complete it with ordinary implicit-instantiation semantics -- member
// DECLARATIONS instantiated, member DEFINITIONS not. EnsureInstantiated used
// TSK_ExplicitInstantiationDefinition plus
// InstantiateClassTemplateSpecializationMembers, eagerly instantiating every
// member body; a specialization whose never-odr-used member bodies are
// ill-formed (the "specialized storage base" idiom, e.g. tl::expected<void, E>
// -- valid C++ as long as those members are never odr-used) was
// wrong-rejected, but ONLY when reflection got to the type before ordinary
// use did (order-dependent enumeration).

#include <meta>

#include <cassert>
#include <string_view>

namespace meta = std::meta;
constexpr auto ctx = meta::access_context::unchecked();

template <class T> struct storage { T m_val; };
template <> struct storage<void> { char m_dummy; };

// Each probe below must be the FIRST instantiation trigger for its
// specialization, so each metafunction gets a distinct template.
template <class T> struct Exp1 : private storage<T> {
  T *valptr() { return &this->m_val; }  // body ill-formed for T = void
  bool has_value() const { return true; }
};
template <class T> struct Exp2 : private storage<T> {
  T *valptr() { return &this->m_val; }
  bool has_value() const { return true; }
};
template <class T> struct Exp3 : private storage<T> {
  T *valptr() { return &this->m_val; }
  bool has_value() const { return true; }
};
template <class T> struct Exp4 : private storage<T> {
  T *valptr() { return &this->m_val; }
  bool has_value() const { return true; }
};
template <class T> struct Exp5 : private storage<T> {
  T *valptr() { return &this->m_val; }
  bool has_value() const { return true; }
};

consteval int count_named(meta::info cls, std::string_view name) {
  int n = 0;
  for (auto m : meta::members_of(cls, ctx))
    if (meta::has_identifier(m) && meta::identifier_of(m) == name)
      ++n;
  return n;
}

// members_of as the first instantiation trigger: must compile and enumerate
// the member declarations. Control: the void specialization enumerates the
// same surface as the well-formed-body int specialization.
static_assert(meta::members_of(^^Exp1<void>, ctx).size() ==
              meta::members_of(^^Exp1<int>, ctx).size());
static_assert(count_named(^^Exp1<void>, "valptr") == 1);
static_assert(count_named(^^Exp1<void>, "has_value") == 1);

// Order-independence: ordinary use first, then the identical enumeration.
Exp2<void> preinstantiated;
static_assert(meta::members_of(^^Exp2<void>, ctx).size() ==
              meta::members_of(^^Exp1<void>, ctx).size());

// Other completing metafunctions as first triggers (same completion funnel).
static_assert(meta::nonstatic_data_members_of(^^Exp3<void>, ctx).size() == 0);
static_assert(meta::bases_of(^^Exp3<void>, ctx).size() == 1);
static_assert(meta::is_complete_type(^^Exp4<void>));
static_assert(meta::size_of(^^Exp5<void>) == 1);  // just storage<void>::m_dummy

int main() {
  // Lazy semantics still instantiate bodies that ARE odr-used.
  Exp1<int> e;
  assert(e.valptr() != nullptr);
  assert(e.has_value());
  return 0;
}
