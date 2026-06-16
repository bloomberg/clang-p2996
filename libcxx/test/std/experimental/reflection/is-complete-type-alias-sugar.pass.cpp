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
// Regression test: is_complete_type must see through ALIAS sugar before
// triggering instantiation. It used to pass the sugared type to findTypeDecl,
// get back the TypedefDecl (for which EnsureInstantiated is a no-op), and
// report a never-yet-instantiated -- but perfectly instantiable --
// specialization as incomplete. Asking about the same specialization through
// its direct spelling first flipped the answer: order-dependent results.
// The members_of family always desugared; is_complete_type was the gap.

#include <experimental/meta>

template <class T> struct Box { T v; };

// Alias to a spec nothing else references: completion must be triggered
// THROUGH the alias.
using BoxInt = Box<int>;
static_assert(std::meta::is_complete_type(^^BoxInt));

// Member typedefs are the field shape (spdlog's sink chain reaches signatures
// through them).
template <class T> struct Wrap {
  using boxed = Box<T*>;
};
static_assert(std::meta::is_complete_type(^^Wrap<char>::boxed));

// A forward-declared-only template is incomplete through any spelling.
template <class T> struct Undefined;
using UndefinedInt = Undefined<int>;
static_assert(!std::meta::is_complete_type(^^UndefinedInt));
static_assert(!std::meta::is_complete_type(^^Undefined<long>));

// Direct spellings keep working, before and after alias queries of the same
// entity.
using BoxLong = Box<long>;
static_assert(std::meta::is_complete_type(^^Box<long>));
static_assert(std::meta::is_complete_type(^^BoxLong));

// Non-template shapes are unaffected.
struct Complete {};
struct Incomplete;
using CompleteAlias = Complete;
using IncompleteAlias = Incomplete;
static_assert(std::meta::is_complete_type(^^CompleteAlias));
static_assert(!std::meta::is_complete_type(^^IncompleteAlias));

int main() { return 0; }
