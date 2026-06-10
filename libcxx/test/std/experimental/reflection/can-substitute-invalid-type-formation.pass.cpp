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
// Regression test: can_substitute must report substitution failure -- not
// crash -- when substituting the arguments into the DECLARATION forms an
// invalid type inside a template-id (e.g. a reference to void, the canonical
// enable_if-era immediate-context SFINAE failure). The arguments themselves
// validate against the template-parameter list (void is a fine argument for
// `class OT`), so the failure surfaces only while instantiating the
// declaration. Previously:
//   * function templates: clang SIGSEGV'd (null Spec dereferenced in
//     MetaActionsImpl::Substitute);
//   * alias templates: a hard error leaked out of CheckTemplateIdType and the
//     evaluation was non-constant;
//   * variable templates: an assertion failure ("substitution failed after
//     validating arguments?").
// The field shape is tl::expected<void, E>: its swap member template's
// DEFAULTED parameter picks up the enclosing specialization's void argument,
// so merely probing members with can_substitute({}) crashed the compiler.

#include <meta>

#include <vector>

namespace meta = std::meta;
constexpr auto ctx = meta::access_context::unchecked();

template <class T> struct trait { using type = void; };

// Substituting OT=void must form trait<void&>: an invalid type in the
// immediate context => substitution failure, not a crash.
template <class OT> typename trait<OT&>::type f() {}

// The tl::expected<void, E>::swap field shape: a member template whose
// DEFAULTED parameter picks up the enclosing specialization's argument.
template <class T> struct Exp {
  template <class OT = T>
  typename trait<OT&>::type swap(Exp&) {}
};

// Same-family siblings hardened in the same change.
template <class T> using Ref = typename trait<T&>::type;    // alias template
template <class T> typename trait<T&>::type* vt = nullptr;  // variable template

// Probes every function template among cls's members with a zero-argument
// substitution (the defaulted parameter supplies the enclosing argument);
// returns substitutable-count * 100 + probed-count.
consteval int probe_member_templates(meta::info cls) {
  int substitutable = 0, probed = 0;
  for (auto mem : meta::members_of(cls, ctx))
    if (meta::is_function_template(mem)) {
      ++probed;
      if (meta::can_substitute(mem, std::vector<meta::info>{}))
        ++substitutable;
    }
  return substitutable * 100 + probed;
}

// Controls: non-void substitution succeeds, and substitute() agrees.
static_assert(meta::can_substitute(^^f, {^^int}));
static_assert(meta::substitute(^^f, {^^int}) != meta::info{});

// Crash shape #1: explicit void argument on a free function template.
static_assert(!meta::can_substitute(^^f, {^^void}));

// Crash shape #2: the defaulted-parameter member template found via the
// members_of walk -- Exp<void>::swap must report not-substitutable (0 of 1)...
static_assert(probe_member_templates(^^Exp<void>) == 1);
// ...while the Exp<int> control substitutes (1 of 1).
static_assert(probe_member_templates(^^Exp<int>) == 101);

// Same-family: alias template (previously a leaked hard error).
static_assert(!meta::can_substitute(^^Ref, {^^void}));
static_assert(meta::can_substitute(^^Ref, {^^int}));

// Same-family: variable template (previously an assertion failure).
static_assert(!meta::can_substitute(^^vt, {^^void}));
static_assert(meta::can_substitute(^^vt, {^^int}));

int main() { return 0; }
