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
// [meta.reflection.access.context]

#include <meta>
#include <type_traits>

using std::meta::access_context;
using std::meta::info;

                                 // ==========
                                 // class_shape
                                 // ==========

namespace class_shape {
// A non-aggregate class type that is copyable and assignable, but not
// default constructible.
static_assert(std::is_class_v<access_context>);
static_assert(!std::is_aggregate_v<access_context>);
static_assert(!std::is_default_constructible_v<access_context>);
static_assert(std::is_copy_constructible_v<access_context>);
static_assert(std::is_move_constructible_v<access_context>);
static_assert(std::is_copy_assignable_v<access_context>);
static_assert(std::is_move_assignable_v<access_context>);

consteval bool assignment_works() {
  access_context ctx = access_context::unchecked();
  ctx = access_context::unprivileged();
  if (ctx.scope() != ^^:: || ctx.designating_class() != info{})
    return false;

  access_context other = access_context::current();
  ctx = std::move(other);
  return is_function(ctx.scope()) &&
         identifier_of(ctx.scope()) == "assignment_works" &&
         ctx.designating_class() == info{};
}
static_assert(assignment_works());

// A structural type: usable as a template argument, with template-argument
// equivalence determined by scope and designating class.
template <access_context Ctx> struct Holder {
  static constexpr info scope = Ctx.scope();
  static constexpr info cls = Ctx.designating_class();
};
static_assert(Holder<access_context::unchecked()>::scope == info{});
static_assert(Holder<access_context::unprivileged()>::scope == ^^::);
static_assert(std::is_same_v<Holder<access_context::unchecked()>,
                             Holder<access_context::unchecked()>>);
static_assert(!std::is_same_v<Holder<access_context::unchecked()>,
                              Holder<access_context::unprivileged()>>);
}  // namespace class_shape

                               // ===============
                               // named_contexts
                               // ===============

namespace named_contexts {
static_assert(access_context::unchecked().scope() == info{});
static_assert(access_context::unchecked().designating_class() == info{});

static_assert(access_context::unprivileged().scope() == ^^::);
static_assert(access_context::unprivileged().designating_class() == info{});

static_assert(access_context::current().scope() == ^^named_contexts);
static_assert(access_context::current().designating_class() == info{});

struct S {
  static constexpr auto ctx = access_context::current();
  consteval static info fn_scope() { return access_context::current().scope(); }
};
static_assert(S::ctx.scope() == ^^S);
static_assert(S::fn_scope() == ^^S::fn_scope);
}  // namespace named_contexts

                                    // ===
                                    // via
                                    // ===

namespace via {
struct Complete { };
using CompleteAlias = Complete;
class Priv { [[maybe_unused]] int m; public: static constexpr auto r = ^^m; };
struct PubDerived : Priv { };

// via(cls) keeps the scope and replaces the designating class.
static_assert(access_context::unchecked().via(^^Complete).scope() == info{});
static_assert(access_context::unchecked().via(^^Complete).designating_class()
              == ^^Complete);
static_assert(access_context::unprivileged().via(^^Complete).scope() == ^^::);
static_assert(access_context::current().via(^^Complete).scope() == ^^via);
static_assert(access_context::current().via(^^Complete).designating_class()
              == ^^Complete);

// The null reflection is accepted and designates no class.
static_assert(access_context::unchecked().via(info{}).scope() == info{});
static_assert(access_context::unchecked().via(info{}).designating_class()
              == info{});
static_assert(access_context::current().via(^^Complete).via(info{})
                  .designating_class() == info{});

// An alias of a complete class type is accepted.
static_assert(access_context::unchecked().via(^^CompleteAlias)
                  .designating_class() == ^^CompleteAlias);

// via() is chainable and does not modify the source.
constexpr auto base = access_context::unprivileged();
constexpr auto derived = base.via(^^Complete);
static_assert(base.designating_class() == info{});
static_assert(derived.designating_class() == ^^Complete);
static_assert(derived.via(^^Priv).designating_class() == ^^Priv);

// A designating class participates in accessibility checks.
static_assert(!is_accessible(Priv::r, access_context::unprivileged()));
static_assert(!is_accessible(Priv::r,
                             access_context::unprivileged().via(^^PubDerived)));
static_assert(is_accessible(Priv::r, access_context::unchecked()));
static_assert(is_accessible(Priv::r, access_context::unchecked().via(info{})));
}  // namespace via

int main() { }
