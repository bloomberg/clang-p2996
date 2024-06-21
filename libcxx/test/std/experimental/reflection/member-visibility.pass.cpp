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

#include <experimental/meta>


struct PublicBase { int mem; };
struct ProtectedBase { int mem; };
struct PrivateBase { int mem; };

struct Access
    : public PublicBase, protected ProtectedBase, private PrivateBase {
public:
    int pub;
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

    // Public members.
    static_assert(is_public(^pub));
    static_assert(is_public(^PublicCls));
    static_assert(is_public(^PublicTFn));
    static_assert(is_public(bases_of(^Access)[0]));
    
    // Not public members.
    static_assert(!is_public(^prot));
    static_assert(!is_public(^priv));
    static_assert(!is_public(^ProtectedCls));
    static_assert(!is_public(^PrivateCls));
    static_assert(!is_public(^ProtectedTFn));
    static_assert(!is_public(^PrivateTFn));
    static_assert(!is_public(bases_of(^Access)[1]));
    static_assert(!is_public(bases_of(^Access)[2]));

    // Protected members.
    static_assert(is_protected(^prot));
    static_assert(is_protected(^ProtectedCls));
    static_assert(is_protected(^ProtectedTFn));
    static_assert(is_protected(bases_of(^Access)[1]));

    // Not protected members.
    static_assert(!is_protected(^pub));
    static_assert(!is_protected(^priv));
    static_assert(!is_protected(^PublicCls));
    static_assert(!is_protected(^PrivateCls));
    static_assert(!is_protected(^PublicTFn));
    static_assert(!is_protected(^PrivateTFn));
    static_assert(!is_protected(bases_of(^Access)[0]));
    static_assert(!is_protected(bases_of(^Access)[2]));

    // Private members.
    static_assert(is_private(^priv));
    static_assert(is_private(^PrivateCls));
    static_assert(is_private(^PrivateTFn));
    static_assert(is_private(bases_of(^Access)[2]));

    // Not private members.
    static_assert(!is_private(^pub));
    static_assert(!is_private(^prot));
    static_assert(!is_private(^PublicCls));
    static_assert(!is_private(^ProtectedCls));
    static_assert(!is_private(^PublicTFn));
    static_assert(!is_private(^ProtectedTFn));
    static_assert(!is_private(bases_of(^Access)[0]));
    static_assert(!is_private(bases_of(^Access)[1]));

    // Everything in this class is accessible.
    static_assert(is_accessible(^pub));
    static_assert(is_accessible(^prot));
    static_assert(is_accessible(^priv));
    static_assert(is_accessible(^PublicCls));
    static_assert(is_accessible(^ProtectedCls));
    static_assert(is_accessible(^PublicTFn));
    static_assert(is_accessible(^ProtectedTFn));
    static_assert(is_accessible(^PrivateCls));
    static_assert(is_accessible(bases_of(^Access)[0]));
    static_assert(is_accessible(bases_of(^Access)[1]));
    static_assert(is_accessible(bases_of(^Access)[2]));

    friend struct FriendClsOfAccess;
    friend void FriendFnOfAccess();
};

struct Derived : Access {
  static_assert(is_accessible(^Access::PublicBase::mem));
  static_assert(is_accessible(^Access::ProtectedBase::mem));
  static_assert(  // PrivateBase::mem
      !is_accessible(
            nonstatic_data_members_of(type_of(bases_of(^Access)[2]))[0]));
};

static_assert(is_accessible(^Access::pub));
static_assert(is_accessible(^Access::PublicCls));
static_assert(is_accessible(^Access::PublicTFn));
static_assert(is_accessible(^Access::PublicBase::mem));

static_assert(  // Access::prot
        !is_accessible(members_of(^Access, std::meta::is_nonstatic_data_member,
                                           std::meta::is_protected)[0]));
static_assert(  // Access:priv
        !is_accessible(members_of(^Access, std::meta::is_nonstatic_data_member,
                                           std::meta::is_private)[0]));
static_assert(  // Access::ProtectedCls
        !is_accessible(members_of(^Access, std::meta::is_type,
                                           std::meta::is_protected)[0]));
static_assert(  // Access::PrivateCls
        !is_accessible(members_of(^Access, std::meta::is_type,
                                           std::meta::is_private)[0]));
static_assert(  // Access::ProtectedTFn
        !is_accessible(members_of(^Access, std::meta::is_template,
                                           std::meta::is_protected)[0]));
static_assert(  // Access::PrivateTFn
        !is_accessible(members_of(^Access, std::meta::is_template,
                                           std::meta::is_private)[0]));
static_assert(is_accessible(bases_of(^Access)[0]));   // PublicBase
static_assert(!is_accessible(bases_of(^Access)[1]));  // ProtectedBase
static_assert(!is_accessible(bases_of(^Access)[2]));  // PrivateBase

struct FriendClsOfAccess {
  static_assert(is_accessible(^Access::pub));
  static_assert(is_accessible(^Access::prot));
  static_assert(is_accessible(^Access::priv));
  static_assert(is_accessible(^Access::PublicCls));
  static_assert(is_accessible(^Access::ProtectedCls));
  static_assert(is_accessible(^Access::PublicTFn));
  static_assert(is_accessible(^Access::ProtectedTFn));
  static_assert(is_accessible(^Access::PrivateCls));
  static_assert(is_accessible(^Access::PublicBase::mem));
  static_assert(is_accessible(^Access::ProtectedBase::mem));
  static_assert(is_accessible(^Access::PrivateBase::mem));
  static_assert(is_accessible(bases_of(^Access)[0]));  // PublicBase
  static_assert(is_accessible(bases_of(^Access)[1]));  // ProtectedBase
  static_assert(is_accessible(bases_of(^Access)[2]));  // PrivateBase
};

void FriendFnOfAccess() {
  static_assert(is_accessible(^Access::pub));
  static_assert(is_accessible(^Access::prot));
  static_assert(is_accessible(^Access::priv));
  static_assert(is_accessible(^Access::PublicCls));
  static_assert(is_accessible(^Access::ProtectedCls));
  static_assert(is_accessible(^Access::PublicTFn));
  static_assert(is_accessible(^Access::ProtectedTFn));
  static_assert(is_accessible(^Access::PrivateCls));
  static_assert(is_accessible(^Access::PublicBase::mem));
  static_assert(is_accessible(^Access::ProtectedBase::mem));
  static_assert(is_accessible(^Access::PrivateBase::mem));
  static_assert(is_accessible(bases_of(^Access)[0]));  // PublicBase
  static_assert(is_accessible(bases_of(^Access)[1]));  // ProtectedBase
  static_assert(is_accessible(bases_of(^Access)[2]));  // PrivateBase
}

                            // =====================
                            // alt_accessibility_api
                            // =====================

static_assert(std::meta::access_context() == ^::);
namespace alt_accessibility_api {
static_assert(std::meta::access_context() == ^::alt_accessibility_api);

void fn() {
  static_assert(std::meta::access_context() == ^fn);
  [] {
    constexpr auto l = std::meta::access_context();
    static_assert(is_function(l));
    static_assert(l != ^fn);
  }();
}

static_assert(!is_accessible(members_of(^Access, std::meta::is_private)[0]));
static_assert(is_accessible(members_of(^Access, std::meta::is_private)[0],
                            ^FriendFnOfAccess));
static_assert(is_accessible(members_of(^Access, std::meta::is_private)[0],
                            ^Access));

}  // namespace alt_accessibility_api

int main() { }
