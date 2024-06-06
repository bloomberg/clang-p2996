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
// ADDITIONAL_COMPILE_FLAGS: -Wno-inconsistent-missing-override

// <experimental/reflection>
//
// [reflection]

#include <experimental/meta>


                         // ==========================
                         // class_or_namespace_members
                         // ==========================

namespace class_or_namespace_members {
int x;
void fn();
namespace inner {}
template <typename T> void TFn();

struct S {
  int mem;
  void memfn();
  template <typename T> void TMemFn();
  struct Inner {};
};

static_assert(is_class_member(^S::mem));
static_assert(!is_namespace_member(^S::mem));

static_assert(is_class_member(^S::memfn));
static_assert(!is_namespace_member(^S::memfn));

static_assert(is_class_member(^S::TMemFn));
static_assert(!is_namespace_member(^S::TMemFn));

static_assert(is_class_member(^S::TMemFn<int>));
static_assert(!is_namespace_member(^S::TMemFn<int>));

static_assert(is_class_member(^S::Inner));
static_assert(!is_namespace_member(^S::Inner));

static_assert(!is_class_member(^x));
static_assert(is_namespace_member(^x));

static_assert(!is_class_member(^fn));
static_assert(is_namespace_member(^fn));

static_assert(!is_class_member(^inner));
static_assert(is_namespace_member(^inner));

static_assert(!is_class_member(^TFn));
static_assert(is_namespace_member(^TFn));

static_assert(!is_class_member(^S));
static_assert(is_namespace_member(^S));

static_assert(!is_class_member(^class_or_namespace_members));
static_assert(is_namespace_member(^class_or_namespace_members));

static_assert(!is_class_member(^::));
static_assert(!is_namespace_member(^::));

static_assert(!is_class_member(^4));
static_assert(!is_namespace_member(^4));
}  // namespace class_or_namespace_members

                        // ============================
                        // static_and_nonstatic_members
                        // ============================

namespace static_and_nonstatic_members {
struct S {
  int nonstatic_mem;
  static int static_mem;

  int nonstatic_func();
  static int static_func();
};

static_assert(!is_static_member(^S::nonstatic_mem));
static_assert(is_nonstatic_data_member(^S::nonstatic_mem));

static_assert(!is_nonstatic_data_member(^S::static_mem));
static_assert(is_static_member(^S::static_mem));

static_assert(!is_nonstatic_data_member(^S::nonstatic_func));
static_assert(!is_static_member(^S::nonstatic_func));

static_assert(!is_nonstatic_data_member(^S::static_func));
static_assert(is_static_member(^S::static_func));

int x;
void fn();
namespace inner {}
template <typename T> void TFn();

static_assert(!is_nonstatic_data_member(^3));
static_assert(!is_nonstatic_data_member(^int));
static_assert(!is_nonstatic_data_member(^TFn));
static_assert(!is_nonstatic_data_member(^TFn<int>));
static_assert(!is_nonstatic_data_member(^::));
static_assert(!is_nonstatic_data_member(^inner));

static_assert(!is_static_member(^3));
static_assert(!is_static_member(^int));
static_assert(!is_static_member(^TFn));
static_assert(!is_static_member(^TFn<int>));
static_assert(!is_static_member(^::));
static_assert(!is_static_member(^inner));
}  // static_and_nonstatic_data_members

                       // ==============================
                       // virtual_and_overridden_members
                       // ==============================

namespace virtual_and_overridden_members {
int x;
void fn();
namespace inner {}
template <typename T> void TFn();

struct Base {
  void nonvirt();
  virtual void virt_no_override();
  virtual void virt_implicit_override();
  virtual void virt_explicit_override();
  virtual void pure_virt() = 0;
};

struct Derived : Base {
  void nonvirt();
  virtual void virt_implicit_override();
  virtual void virt_explicit_override() override;
  virtual void pure_virt();
};

static_assert(!is_virtual(^Base::nonvirt));
static_assert(is_virtual(^Base::virt_no_override));
static_assert(is_virtual(^Base::virt_implicit_override));
static_assert(is_virtual(^Base::virt_explicit_override));
static_assert(is_virtual(^Base::pure_virt));
static_assert(!is_virtual(^Derived::nonvirt));
static_assert(is_virtual(^Derived::virt_no_override));
static_assert(is_virtual(^Derived::virt_implicit_override));
static_assert(is_virtual(^Derived::virt_explicit_override));
static_assert(is_virtual(^Derived::pure_virt));
static_assert(!is_virtual(^3));
static_assert(!is_virtual(^int));
static_assert(!is_virtual(^TFn));
static_assert(!is_virtual(^TFn<int>));
static_assert(!is_virtual(^::));
static_assert(!is_virtual(^inner));

static_assert(!is_pure_virtual(^Base::nonvirt));
static_assert(!is_pure_virtual(^Base::virt_no_override));
static_assert(!is_pure_virtual(^Base::virt_implicit_override));
static_assert(!is_pure_virtual(^Base::virt_explicit_override));
static_assert(is_pure_virtual(^Base::pure_virt));
static_assert(!is_override(^Derived::nonvirt));
static_assert(!is_override(^Derived::virt_no_override));
static_assert(is_override(^Derived::virt_implicit_override));
static_assert(is_override(^Derived::virt_explicit_override));
static_assert(is_override(^Derived::pure_virt));
static_assert(!is_pure_virtual(^3));
static_assert(!is_pure_virtual(^int));
static_assert(!is_pure_virtual(^TFn));
static_assert(!is_pure_virtual(^TFn<int>));
static_assert(!is_pure_virtual(^::));
static_assert(!is_pure_virtual(^inner));

static_assert(!is_override(^Base::nonvirt));
static_assert(!is_override(^Base::virt_no_override));
static_assert(!is_override(^Base::virt_implicit_override));
static_assert(!is_override(^Base::virt_explicit_override));
static_assert(!is_override(^Base::pure_virt));
static_assert(!is_override(^Derived::nonvirt));
static_assert(!is_override(^Derived::virt_no_override));
static_assert(is_override(^Derived::virt_implicit_override));
static_assert(is_override(^Derived::virt_explicit_override));
static_assert(is_override(^Derived::pure_virt));
static_assert(!is_override(^3));
static_assert(!is_override(^int));
static_assert(!is_override(^TFn));
static_assert(!is_override(^TFn<int>));
static_assert(!is_override(^::));
static_assert(!is_override(^inner));
}  // namespace virtual_and_overridden_members

                                // =============
                                // virtual_bases
                                // =============

namespace virtual_bases {
int x;
void fn();
namespace inner {}
template <typename T> void TFn();

struct B1 {};
struct B2 {};
struct D : B1, virtual B2 { };

static_assert(!is_virtual(bases_of(^D)[0]));
static_assert(is_virtual(bases_of(^D)[1]));

static_assert(!is_virtual(^B1));
static_assert(!is_virtual(^B2));
static_assert(!is_virtual(^D));
}  // namespace virtual_bases

                               // ===============
                               // special_members
                               // ===============
namespace special_members {
struct S {
  S();
  ~S();
  S(const S&);
  S(S&&);
  S& operator=(const S&);
  S&& operator=(S&&);

  S(int);

  int mem;
  void memfn();
  template <typename T> void TMemFn();
  struct Inner {};
};
static_assert(members_of(^S, std::meta::is_constructor).size() == 4);
static_assert(members_of(^S, std::meta::is_destructor).size() == 1);
static_assert(members_of(^S, std::meta::is_special_member).size() == 6);
static_assert(!is_special_member(^S::mem));
static_assert(!is_special_member(^S::memfn));
static_assert(!is_special_member(^S::TMemFn));
static_assert(!is_special_member(^S::Inner));

int x;
void fn();
namespace inner {}
template <typename T> void TFn();
static_assert(!is_special_member(^3));
static_assert(!is_special_member(^int));
static_assert(!is_special_member(^TFn));
static_assert(!is_special_member(^TFn<int>));
static_assert(!is_special_member(^::));
static_assert(!is_special_member(^inner));
}  // special_members

                        // =============================
                        // defaulted_and_deleted_members
                        // =============================

namespace defaulted_and_deleted_members {
void deleted() = delete;
void not_deleted();
struct S {
    ~S() = delete;
};
static_assert(is_deleted(^deleted));
static_assert(is_deleted(^S::~S));
static_assert(!is_deleted(^not_deleted));

struct not_dflt {
    ~not_dflt();
};
static_assert(!is_defaulted(^not_dflt::~not_dflt));

struct explicit_dflt {
    ~explicit_dflt() = default;
};
static_assert(is_defaulted(^explicit_dflt::~explicit_dflt));

struct implicit_dflt {
};
static_assert(is_defaulted(^implicit_dflt::~implicit_dflt));

struct impl_dflt {
    ~impl_dflt();
};
static_assert(!is_defaulted(^impl_dflt::~impl_dflt));
impl_dflt::~impl_dflt() = default;
static_assert(is_defaulted(^impl_dflt::~impl_dflt));

int x;
void fn();
namespace inner {}
template <typename T> void TFn();
static_assert(!is_deleted(^3));
static_assert(!is_deleted(^int));
static_assert(!is_deleted(^TFn));
static_assert(!is_deleted(^TFn<int>));
static_assert(!is_deleted(^::));
static_assert(!is_deleted(^inner));
static_assert(!is_defaulted(^3));
static_assert(!is_defaulted(^int));
static_assert(!is_defaulted(^TFn));
static_assert(!is_defaulted(^TFn<int>));
static_assert(!is_defaulted(^::));
static_assert(!is_defaulted(^inner));
}  // namespace defaulted_and_deleted_members

                             // ==================
                             // explicit_functions
                             // ==================

namespace explicit_functions {
struct S {
  int mem;
  void memfn();
  template <typename T> void TMemFn();

  S();
  template <typename T> S(bool);

  explicit S(int);
  template <typename T> explicit S(T);

  operator int();
  template <typename T> operator short();
  explicit operator bool();
  template <typename T> explicit operator char();
};
static_assert(!is_explicit(^S::mem));
static_assert(!is_explicit(^S::memfn));
static_assert(!is_explicit(^S::TMemFn));
static_assert(!is_explicit(members_of(^S, std::meta::is_constructor)[0]));
static_assert(!is_explicit(members_of(^S, std::meta::is_constructor)[1]));
static_assert(is_explicit(members_of(^S, std::meta::is_constructor)[2]));
static_assert(!is_explicit(^S::operator int));
static_assert(!is_explicit(members_of(^S, std::meta::is_template)[3]));
static_assert(is_explicit(^S::operator bool));

// P2996R3 removes support for checking 'explicit' on templates.
static_assert(!is_explicit(members_of(^S, std::meta::is_constructor)[3]));
static_assert(!is_explicit(members_of(^S, std::meta::is_template)[4]));

int x;
void fn();
namespace inner {}
template <typename T> void TFn();
static_assert(!is_explicit(^3));
static_assert(!is_explicit(^int));
static_assert(!is_explicit(^TFn));
static_assert(!is_explicit(^TFn<int>));
static_assert(!is_explicit(^::));
static_assert(!is_explicit(^inner));
}  // namespace explicit_functions

                             // ==================
                             // noexcept_functions
                             // ==================

namespace noexcept_functions {
struct S {
  // methods
  void noexcept_method() noexcept;
  void not_noexcept_method();
  
  // virtual methods
  virtual void noexcept_virtual_method() noexcept;
  virtual void not_noexcept_virtual_method();

  // template methods
  template <typename T>
  void noexcept_template_method() noexcept;
  template <typename T>
  void not_noexcept_template_method();
};

// non generic lambdas
constexpr auto noexcept_lambda = []() noexcept {};
constexpr auto not_noexcept_lambda = []{};

// generic lambdas
constexpr auto noexcept_generic_lambda = []<typename T>() noexcept {};
constexpr auto not_noexcept_generic_lambda = []<typename T>() {};

// functions
void noexcept_function() noexcept;
void not_noexcept_function() noexcept;

// template functions
template <typename T>
void noexcept_template_function() noexcept;
template <typename T>
void not_noexcept_template_function() noexcept;

// Everything mentioned in the proposal
// (no-)noexcept member functions
static_assert(is_noexcept(^S::noexcept_method));
static_assert(!is_noexcept(^S::not_noexcept_method));

// (no-)noexcept member function types
static_assert(is_noexcept(type_of(^S::noexcept_method)));
static_assert(!is_noexcept(type_of(^S::not_noexcept_method)));

// (no-)noexcept virtual methods
static_assert(is_noexcept(^S::noexcept_virtual_method));
static_assert(!is_noexcept(^S::not_noexcept_virtual_method));

// (no-)noexcept virtual method types
static_assert(is_noexcept(type_of(^S::noexcept_virtual_method)));
static_assert(!is_noexcept(type_of(^S::not_noexcept_virtual_method)));

// (no-)noexcept lambdas
static_assert(is_noexcept(^noexcept_lambda));
static_assert(!is_noexcept(^not_noexcept_lambda));

// (no-)noexcept closure types
static_assert(is_noexcept(type_of(^noexcept_lambda)));
static_assert(!is_noexcept(type_of(^not_noexcept_lambda)));

// (no-)noexcept function
static_assert(is_noexcept(^noexcept_function));
static_assert(!is_noexcept(^not_noexcept_function));

// (no-)noexcept function type
static_assert(is_noexcept(type_of(^noexcept_function)));
static_assert(!is_noexcept(type_of(^not_noexcept_function)));

// The rest (should all be false regardless of noexcept specifier)
// (no-)noexcept template methods
static_assert(!is_noexcept(^S::noexcept_template_method));
static_assert(!is_noexcept(^S::not_noexcept_template_method));

// (no-)noexcept instantiated template methods
static_assert(!is_noexcept(^S::noexcept_template_method<int>));
static_assert(!is_noexcept(^S::not_noexcept_template_method<int>));
static_assert(!is_noexcept(type_of(^S::noexcept_template_method<int>)));
static_assert(!is_noexcept(type_of(^S::not_noexcept_template_method<int>)));

// (no-)noexcept generic closure types
static_assert(!is_noexcept(^noexcept_generic_lambda));
static_assert(!is_noexcept(^not_noexcept_generic_lambda));
static_assert(!is_noexcept(type_of(^noexcept_generic_lambda)));
static_assert(!is_noexcept(type_of(^not_noexcept_generic_lambda)));

// (no-)noexcept template functions
static_assert(!is_noexcept(^noexcept_template_function));
static_assert(!is_noexcept(^not_noexcept_template_function));

// (no-)noexcept instantiated template functions
static_assert(!is_noexcept(^noexcept_template_function<int>));
static_assert(!is_noexcept(^not_noexcept_template_function<int>));
static_assert(!is_noexcept(type_of(^noexcept_template_function<int>)));
static_assert(!is_noexcept(type_of(^not_noexcept_template_function<int>)));

// Random non callable types
auto x = 1;
auto s = S();

static_assert(!is_noexcept(^x));
static_assert(!is_noexcept(type_of(^x)));
static_assert(!is_noexcept(^s));
static_assert(!is_noexcept(type_of(^s)));
} // namespace noexcept_functions

                              // ================
                              // bitfield members
                              // ================

namespace bitfield_members {
struct S {
  int i : 2;
  int j;
  static inline int k;
};

static_assert(is_bit_field(^S::i));
static_assert(!is_bit_field(^S::j));
static_assert(!is_bit_field(^S::k));

static_assert(!is_bit_field(^S));
static_assert(!is_bit_field(^int));
static_assert(!is_bit_field(^4));
static_assert(!is_bit_field(^std::meta::extract));
}  // namespace bitfield_members

int main() { }
