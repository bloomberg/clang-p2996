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

#include <meta>
#include <ranges>

using std::meta::access_context;

struct PublicBase { int mem; };
struct ProtectedBase { int mem; };
struct PrivateBase { int mem; };

struct Access
    : PublicBase, protected ProtectedBase, private PrivateBase {
    static consteval access_context token() {
      return access_context::current();
    }

    int pub;
public:
    struct PublicCls {};
    template <typename T> void PublicTFn();
protected:
    int prot;
    struct ProtectedCls {};
    template <typename T> void ProtectedTFn();
private:
    int priv;
    struct PrivateCls {};
    template <typename T> void PrivateTFn();

    void complete_class_context() {
        // Public members.
        static_assert(is_public(^^pub));
        static_assert(!is_access_specified(^^pub));
        static_assert(is_public(^^PublicCls));
        static_assert(is_access_specified(^^PublicCls));
        static_assert(is_public(^^PublicTFn));
        static_assert(is_access_specified(^^PublicTFn));
        static_assert(is_public(bases_of(^^Access,
                                         access_context::unchecked())[0]));
        static_assert(
                !is_access_specified(bases_of(^^Access,
                                              access_context::unchecked())[0]));

        // Not public members.
        static_assert(!is_public(^^prot));
        static_assert(!is_public(^^priv));
        static_assert(!is_public(^^ProtectedCls));
        static_assert(!is_public(^^PrivateCls));
        static_assert(!is_public(^^ProtectedTFn));
        static_assert(!is_public(^^PrivateTFn));
        static_assert(!is_public(bases_of(^^Access,
                                          access_context::unchecked())[1]));
        static_assert(!is_public(bases_of(^^Access,
                                          access_context::unchecked())[2]));

        // Protected members.
        static_assert(is_protected(^^prot));
        static_assert(is_protected(^^ProtectedCls));
        static_assert(is_protected(^^ProtectedTFn));
        static_assert(is_protected(bases_of(^^Access,
                                            access_context::unchecked())[1]));

        // Not protected members.
        static_assert(!is_protected(^^pub));
        static_assert(!is_protected(^^priv));
        static_assert(!is_protected(^^PublicCls));
        static_assert(!is_protected(^^PrivateCls));
        static_assert(!is_protected(^^PublicTFn));
        static_assert(!is_protected(^^PrivateTFn));
        static_assert(!is_protected(bases_of(^^Access,
                                             access_context::unchecked())[0]));
        static_assert(!is_protected(bases_of(^^Access,
                                             access_context::unchecked())[2]));

        // Private members.
        static_assert(is_private(^^priv));
        static_assert(is_private(^^PrivateCls));
        static_assert(is_private(^^PrivateTFn));
        static_assert(is_private(bases_of(^^Access,
                                          access_context::unchecked())[2]));

        // Not private members.
        static_assert(!is_private(^^pub));
        static_assert(!is_private(^^prot));
        static_assert(!is_private(^^PublicCls));
        static_assert(!is_private(^^ProtectedCls));
        static_assert(!is_private(^^PublicTFn));
        static_assert(!is_private(^^ProtectedTFn));
        static_assert(!is_private(bases_of(^^Access,
                                           access_context::unchecked())[0]));
        static_assert(!is_private(bases_of(^^Access,
                                           access_context::unchecked())[1]));

        // Everything in this class is accessible.
        constexpr auto ctx = access_context::current();
        static_assert(is_accessible(^^pub, ctx));
        static_assert(is_accessible(^^prot, ctx));
        static_assert(is_accessible(^^priv, ctx));
        static_assert(is_accessible(^^PublicCls, ctx));
        static_assert(is_accessible(^^ProtectedCls, ctx));
        static_assert(is_accessible(^^PublicTFn, ctx));
        static_assert(is_accessible(^^ProtectedTFn, ctx));
        static_assert(is_accessible(^^PrivateCls, ctx));
        static_assert(is_accessible(bases_of(^^Access,
                                             access_context::unchecked())[0],
                                    ctx));
        static_assert(is_accessible(bases_of(^^Access,
                                             access_context::unchecked())[1],
                                    ctx));
        static_assert(is_accessible(bases_of(^^Access,
                                             access_context::unchecked())[2],
                                    ctx));
    }

    friend struct FriendClsOfAccess;
    friend consteval access_context FriendFnOfAccess();

public:
    static constexpr auto r_prot = ^^prot;
};

struct Derived : Access {
  static constexpr auto ctx = access_context::current();
  static constexpr auto obj_ctx = access_context::current().via(^^Derived);

  static_assert(is_accessible(^^Access::PublicBase::mem, ctx));
  static_assert(is_accessible(^^Access::ProtectedBase::mem, ctx));
  static_assert(is_accessible(^^::PrivateBase::mem, ctx));
  static_assert(!is_accessible(^^::PrivateBase::mem, obj_ctx));
  static_assert(!is_accessible(Access::r_prot, ctx));
  static_assert(is_accessible(Access::r_prot, obj_ctx));
};

static constexpr auto gctx = access_context::current();
static_assert(is_accessible(^^Access::pub, gctx));
static_assert(is_accessible(^^Access::PublicCls, gctx));
static_assert(is_accessible(^^Access::PublicTFn, gctx));
static_assert(is_accessible(^^Access::PublicBase::mem, gctx));

static_assert(  // Access::prot
    !is_accessible((members_of(^^Access, access_context::unchecked()) |
                      std::views::filter(std::meta::is_nonstatic_data_member) |
                      std::views::filter(std::meta::is_protected)).front(),
                   gctx));
static_assert(  // Access::priv
    !is_accessible((members_of(^^Access, access_context::unchecked()) |
                      std::views::filter(std::meta::is_nonstatic_data_member) |
                      std::views::filter(std::meta::is_private)).front(),
                   gctx));
static_assert(  // Access::ProtectedCls
    !is_accessible((members_of(^^Access, access_context::unchecked()) |
                      std::views::filter(std::meta::is_type) |
                      std::views::filter(std::meta::is_protected)).front(),
                   gctx));
static_assert(  // Access::PrivateCls
    !is_accessible((members_of(^^Access, access_context::unchecked()) |
                      std::views::filter(std::meta::is_type) |
                      std::views::filter(std::meta::is_private)).front(),
                   gctx));
static_assert(  // Access::ProtectedTFn
    !is_accessible((members_of(^^Access, access_context::unchecked()) |
                      std::views::filter(std::meta::is_template) |
                      std::views::filter(std::meta::is_protected)).front(),
                   gctx));
static_assert(  // Access::ProtectedTFn
    !is_accessible((members_of(^^Access, access_context::unchecked()) |
                      std::views::filter(std::meta::is_template) |
                      std::views::filter(std::meta::is_private)).front(),
                   gctx));
static_assert(is_accessible(bases_of(^^Access,   // PublicBase
                                     access_context::unchecked())[0], gctx));
static_assert(!is_accessible(bases_of(^^Access,  // ProtectedBase
                                      access_context::unchecked())[1], gctx));
static_assert(!is_accessible(bases_of(^^Access,  // PrivateBase
                                      access_context::unchecked())[2], gctx));

struct FriendClsOfAccess {
  static constexpr auto ctx = access_context::current();

  static_assert(is_accessible(^^Access::pub, ctx));
  static_assert(is_accessible(^^Access::prot, ctx));
  static_assert(is_accessible(^^Access::priv, ctx));
  static_assert(is_accessible(^^Access::PublicCls, ctx));
  static_assert(is_accessible(^^Access::ProtectedCls, ctx));
  static_assert(is_accessible(^^Access::PublicTFn, ctx));
  static_assert(is_accessible(^^Access::ProtectedTFn, ctx));
  static_assert(is_accessible(^^Access::PrivateCls, ctx));
  static_assert(is_accessible(^^Access::PublicBase::mem, ctx));
  static_assert(is_accessible(^^Access::ProtectedBase::mem, ctx));
  static_assert(is_accessible(^^Access::PrivateBase::mem, ctx));
  static_assert(is_accessible(bases_of(^^Access,  // PublicBase
                                       access_context::unchecked())[0],
                ctx));
  static_assert(is_accessible(bases_of(^^Access,  // ProtectedBase
                                       access_context::unchecked())[1],
                ctx));
  static_assert(is_accessible(bases_of(^^Access,  // PrivateBase
                                       access_context::unchecked())[2],
                ctx));
};

consteval access_context FriendFnOfAccess() {
  static constexpr auto ctx = access_context::current();

  static_assert(is_accessible(^^Access::pub, ctx));
  static_assert(is_accessible(^^Access::prot, ctx));
  static_assert(is_accessible(^^Access::priv, ctx));
  static_assert(is_accessible(^^Access::PublicCls, ctx));
  static_assert(is_accessible(^^Access::ProtectedCls, ctx));
  static_assert(is_accessible(^^Access::PublicTFn, ctx));
  static_assert(is_accessible(^^Access::ProtectedTFn, ctx));
  static_assert(is_accessible(^^Access::PrivateCls, ctx));
  static_assert(is_accessible(^^Access::PublicBase::mem, ctx));
  static_assert(is_accessible(^^Access::ProtectedBase::mem, ctx));
  static_assert(is_accessible(^^Access::PrivateBase::mem, ctx));
  static_assert(is_accessible(bases_of(^^Access,  // PublicBase
                                       access_context::unchecked())[0],
                ctx));
  static_assert(is_accessible(bases_of(^^Access,  // ProtctedBase
                                       access_context::unchecked())[1],
                ctx));
  static_assert(is_accessible(bases_of(^^Access,  // PrivateBase
                                       access_context::unchecked())[2],
                ctx));

  return ctx;
}

                            // =====================
                            // new_accessibility_api
                            // =====================


static_assert(access_context::current().scope() == ^^::);
namespace new_accessibility_api {
static_assert(access_context::current().scope() == ^^::new_accessibility_api);

void fn() {
  static_assert(access_context::current().scope() == ^^fn);
  [] {
    constexpr auto repr = access_context::current().scope();
    static_assert(is_function(repr));
    static_assert(repr != ^^fn);
  }();
}

static_assert(
    !is_accessible((members_of(^^Access, access_context::unchecked()) |
                       std::views::filter(std::meta::is_private)).front(),
                   access_context::current()));
static_assert(
    is_accessible((members_of(^^Access, access_context::unchecked()) |
                      std::views::filter(std::meta::is_private)).front(),
                  FriendFnOfAccess()));
static_assert(
    is_accessible((members_of(^^Access, access_context::unchecked()) |
                      std::views::filter(std::meta::is_private)).front(),
                  Access::token()));
}  // namespace new_accessibility_api

                              // ================
                              // nonsense_queries
                              // ================

namespace nonsense_queries {
static_assert(!is_public(std::meta::info{}));
static_assert(!is_public(^^int));
static_assert(!is_protected(std::meta::info{}));
static_assert(!is_protected(^^int));
static_assert(!is_private(std::meta::info{}));
static_assert(!is_private(^^int));
}  // namespace queries

                               // ==============
                               // unscoped_enums
                               // ==============

namespace unscoped_enums {
class C {
  enum { E };
public:
  enum { F };
  static constexpr auto r = ^^E;
};

static_assert(!is_accessible(C::r,
              std::meta::access_context::unprivileged()));
static_assert(!is_accessible(C::r,
              std::meta::access_context::unprivileged().via(^^C)));

static_assert(is_accessible(^^C::F,
              std::meta::access_context::unprivileged()));
static_assert(is_accessible(^^C::F,
              std::meta::access_context::unprivileged().via(^^C)));
}  // namespace unscoped_enums

                          // ========================
                          // anonymous_structs_unions
                          // ========================

namespace anonymous_structs_unions {
class C {
  struct { struct { struct { int a; }; }; };
public:
  union { union { union { int b; }; }; };
  static constexpr auto r_a = ^^a;
};

static_assert(is_accessible(C::r_a,
              std::meta::access_context::unprivileged()));
static_assert(!is_accessible(C::r_a,
              std::meta::access_context::unprivileged().via(^^C)));

static_assert(is_accessible(^^C::b,
              std::meta::access_context::unprivileged()));
static_assert(is_accessible(^^C::b,
              std::meta::access_context::unprivileged().via(^^C)));

static union { int a; };

static_assert(is_accessible(^^a,
              std::meta::access_context::unprivileged()));
}  // namespace anonymous_structs_unions

                          // ========================
                          // default_mem_initializers
                          // ========================

namespace default_mem_initializers {
struct A {
  int a;
  consteval A(int p) : a(p) {}
};
struct B : A {
  using A::A;
  consteval B(int p, int q) : A(p * q) {}
  std::meta::info s = std::meta::access_context::current().scope();
};
static_assert(B(1).s == ^^B);
static_assert(is_constructor(B{1, 2}.s) && parent_of(B{1, 2}.s) == ^^B);
}  // namespace default_mem_initializers

                    // =====================================
                    // access_context_always_value_dependent
                    // =====================================

namespace access_context_always_value_dependent {
template <bool B> consteval auto baz() -> int {
  static constexpr int v = (std::meta::access_context::current().scope() ==
                            ^^baz<true>) ?
                           42 : INT_MAX + 1;
  return v;
}

static_assert(baz<true>() == 42);
}  // namespace access_context_always_value_dependent

                  // ========================================
                  // bb_clang_p2996_issue_148_regression_test
                  // ========================================

namespace bb_clang_p2996_issue_148_regression_test {
struct A {};
struct B {};
struct C {};

struct D : A, protected B, private C {
    static consteval auto foo()
    {
        constexpr auto ctx = std::meta::access_context::current();
        constexpr auto n = bases_of(^^D, ctx).size();
        static_assert(n == 3);
        static_assert(bases_of(^^D, ctx).size() == 3);
    }
};

}  // namespace bb_clang_p2996_issue_148_regression_test

                  // ========================================
                  // bb_clang_p2996_issue_193_regression_test
                  // ========================================

namespace bb_clang_p2996_issue_193_regression_test {
struct S {
    enum E {
        A
    };

    enum class SE {
        B
    };
};

static_assert(std::meta::is_class_member(^^S::E));
static_assert(std::meta::is_class_member(^^S::SE));

static_assert(std::meta::is_public(^^S::E));
static_assert(std::meta::is_public(^^S::SE));

static_assert(std::meta::is_class_member(^^S::E::A));
static_assert(!std::meta::is_class_member(^^S::SE::B));

static_assert(std::meta::is_public(^^S::E::A));
static_assert(!std::meta::is_public(^^S::SE::B));

}  // namespace bb_clang_p2996_issue_193_regression_test

int main() { }
