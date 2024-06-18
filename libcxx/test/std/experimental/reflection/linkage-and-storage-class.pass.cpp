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
// ADDITIONAL_COMPILE_FLAGS: -Wno-unneeded-internal-declaration

// <experimental/reflection>
//
// [reflection]

#include <experimental/meta>


                         // ==========================
                         // storage_class_and_duration
                         // ==========================

namespace storage_class_and_duration {
struct S {
    int nsdm;
    static_assert(!has_static_storage_duration(^nsdm));
    static_assert(is_nonstatic_data_member(^nsdm));
    static_assert(!is_static_member(^nsdm));

    static inline int sdm;
    static_assert(has_static_storage_duration(^sdm));
    static_assert(!is_nonstatic_data_member(^sdm));
    static_assert(is_static_member(^sdm));
};

int i1;
static_assert(has_static_storage_duration(^i1));

static int i2;
static_assert(has_static_storage_duration(^i2));

thread_local int i3;
static_assert(!has_static_storage_duration(^i3));

static thread_local int i4;
static_assert(!has_static_storage_duration(^i4));

void foo() {
    int nonstatic_var;
    static_assert(!has_static_storage_duration(^nonstatic_var));

    static int static_var;
    static_assert(has_static_storage_duration(^static_var));
}

template <auto V> struct TCls {};
static_assert(!has_static_storage_duration(template_arguments_of(^TCls<5>)[0]));

template <auto K> constexpr auto R = ^K;
static_assert(has_static_storage_duration(R<S{}>));

static std::pair<int, int> p;

constexpr auto first = std::meta::reflect_object(p.first);
static_assert(has_static_storage_duration(first));

static_assert(!has_static_storage_duration(std::meta::reflect_value(4)));
}  // namespace storage_class_and_duration

                                   // =======
                                   // linkage
                                   // =======

namespace linkage {
int global;
static int s_global;

std::pair<int, int> p1;
static std::pair<int, int> p2;

void fn();

static_assert(has_linkage(^global));
static_assert(!has_internal_linkage(^global));
static_assert(has_external_linkage(^global));

static_assert(has_linkage(^s_global));
static_assert(has_internal_linkage(^s_global));
static_assert(!has_external_linkage(^s_global));

static_assert(has_linkage(^fn));
static_assert(!has_internal_linkage(^fn));
static_assert(has_external_linkage(^fn));

static_assert(has_linkage(std::meta::reflect_object(p1.first)));
static_assert(!has_internal_linkage(std::meta::reflect_object(p1.first)));
static_assert(has_external_linkage(std::meta::reflect_object(p1.first)));

static_assert(has_linkage(std::meta::reflect_object(p1.first)));
static_assert(has_internal_linkage(std::meta::reflect_object(p2.first)));
static_assert(!has_external_linkage(std::meta::reflect_object(p2.first)));

void fn() {
  struct S { static void fn(); };
  static_assert(!has_linkage(^S::fn));
  static_assert(!has_internal_linkage(^S::fn));
  static_assert(!has_external_linkage(^S::fn));
}

template <typename T> struct TCls;
template <typename T> void TFn();
template <typename T> int TVar;

static_assert(!has_linkage(^::));
static_assert(!has_linkage(^::linkage));
static_assert(!has_linkage(std::meta::reflect_value(3)));
static_assert(!has_linkage(^int));
static_assert(!has_linkage(^TCls));
static_assert(!has_linkage(^TFn));
static_assert(!has_linkage(^TVar));
static_assert(!has_internal_linkage(^::));
static_assert(!has_internal_linkage(^::linkage));
static_assert(!has_internal_linkage(std::meta::reflect_value(3)));
static_assert(!has_internal_linkage(^int));
static_assert(!has_internal_linkage(^TCls));
static_assert(!has_internal_linkage(^TFn));
static_assert(!has_internal_linkage(^TVar));
static_assert(!has_external_linkage(^::));
static_assert(!has_external_linkage(^::linkage));
static_assert(!has_external_linkage(std::meta::reflect_value(3)));
static_assert(!has_external_linkage(^int));
static_assert(!has_external_linkage(^TCls));
static_assert(!has_external_linkage(^TFn));
static_assert(!has_external_linkage(^TVar));
}  // namespace linkage


int main() {}
