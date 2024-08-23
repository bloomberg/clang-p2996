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

static_assert(is_class_member(^^S::mem));
static_assert(!is_namespace_member(^^S::mem));

static_assert(is_class_member(^^S::memfn));
static_assert(!is_namespace_member(^^S::memfn));

static_assert(is_class_member(^^S::TMemFn));
static_assert(!is_namespace_member(^^S::TMemFn));

static_assert(is_class_member(^^S::TMemFn<int>));
static_assert(!is_namespace_member(^^S::TMemFn<int>));

static_assert(is_class_member(^^S::Inner));
static_assert(!is_namespace_member(^^S::Inner));

static_assert(!is_class_member(^^x));
static_assert(is_namespace_member(^^x));

static_assert(!is_class_member(^^fn));
static_assert(is_namespace_member(^^fn));

static_assert(!is_class_member(^^inner));
static_assert(is_namespace_member(^^inner));

static_assert(!is_class_member(^^TFn));
static_assert(is_namespace_member(^^TFn));

static_assert(!is_class_member(^^S));
static_assert(is_namespace_member(^^S));

static_assert(!is_class_member(^^class_or_namespace_members));
static_assert(is_namespace_member(^^class_or_namespace_members));

static_assert(!is_class_member(^^::));
static_assert(!is_namespace_member(^^::));

static_assert(!is_class_member(std::meta::reflect_value(4)));
static_assert(!is_namespace_member(std::meta::reflect_value(4)));
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

static_assert(!is_static_member(^^S::nonstatic_mem));
static_assert(is_nonstatic_data_member(^^S::nonstatic_mem));

static_assert(!is_nonstatic_data_member(^^S::static_mem));
static_assert(is_static_member(^^S::static_mem));

static_assert(!is_nonstatic_data_member(^^S::nonstatic_func));
static_assert(!is_static_member(^^S::nonstatic_func));

static_assert(!is_nonstatic_data_member(^^S::static_func));
static_assert(is_static_member(^^S::static_func));

int x;
void fn();
namespace inner {}
template <typename T> void TFn();

static_assert(!is_nonstatic_data_member(std::meta::reflect_value(3)));
static_assert(!is_nonstatic_data_member(^^int));
static_assert(!is_nonstatic_data_member(^^TFn));
static_assert(!is_nonstatic_data_member(^^TFn<int>));
static_assert(!is_nonstatic_data_member(^^::));
static_assert(!is_nonstatic_data_member(^^inner));

static_assert(!is_static_member(std::meta::reflect_value(3)));
static_assert(!is_static_member(^^int));
static_assert(!is_static_member(^^TFn));
static_assert(!is_static_member(^^TFn<int>));
static_assert(!is_static_member(^^::));
static_assert(!is_static_member(^^inner));
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

static_assert(!is_virtual(^^Base::nonvirt));
static_assert(is_virtual(^^Base::virt_no_override));
static_assert(is_virtual(^^Base::virt_implicit_override));
static_assert(is_virtual(^^Base::virt_explicit_override));
static_assert(is_virtual(^^Base::pure_virt));
static_assert(!is_virtual(^^Derived::nonvirt));
static_assert(is_virtual(^^Derived::virt_no_override));
static_assert(is_virtual(^^Derived::virt_implicit_override));
static_assert(is_virtual(^^Derived::virt_explicit_override));
static_assert(is_virtual(^^Derived::pure_virt));
static_assert(!is_virtual(std::meta::reflect_value(3)));
static_assert(!is_virtual(^^int));
static_assert(!is_virtual(^^TFn));
static_assert(!is_virtual(^^TFn<int>));
static_assert(!is_virtual(^^::));
static_assert(!is_virtual(^^inner));

static_assert(!is_pure_virtual(^^Base::nonvirt));
static_assert(!is_pure_virtual(^^Base::virt_no_override));
static_assert(!is_pure_virtual(^^Base::virt_implicit_override));
static_assert(!is_pure_virtual(^^Base::virt_explicit_override));
static_assert(is_pure_virtual(^^Base::pure_virt));
static_assert(!is_override(^^Derived::nonvirt));
static_assert(!is_override(^^Derived::virt_no_override));
static_assert(is_override(^^Derived::virt_implicit_override));
static_assert(is_override(^^Derived::virt_explicit_override));
static_assert(is_override(^^Derived::pure_virt));
static_assert(!is_pure_virtual(std::meta::reflect_value(3)));
static_assert(!is_pure_virtual(^^int));
static_assert(!is_pure_virtual(^^TFn));
static_assert(!is_pure_virtual(^^TFn<int>));
static_assert(!is_pure_virtual(^^::));
static_assert(!is_pure_virtual(^^inner));
static_assert(!is_pure_virtual(std::meta::info{}));
static_assert(!is_pure_virtual(^^int));

static_assert(!is_override(^^Base::nonvirt));
static_assert(!is_override(^^Base::virt_no_override));
static_assert(!is_override(^^Base::virt_implicit_override));
static_assert(!is_override(^^Base::virt_explicit_override));
static_assert(!is_override(^^Base::pure_virt));
static_assert(!is_override(^^Derived::nonvirt));
static_assert(!is_override(^^Derived::virt_no_override));
static_assert(is_override(^^Derived::virt_implicit_override));
static_assert(is_override(^^Derived::virt_explicit_override));
static_assert(is_override(^^Derived::pure_virt));
static_assert(!is_override(std::meta::reflect_value(3)));
static_assert(!is_override(^^int));
static_assert(!is_override(^^TFn));
static_assert(!is_override(^^TFn<int>));
static_assert(!is_override(^^::));
static_assert(!is_override(^^inner));
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

static_assert(!is_virtual(bases_of(^^D)[0]));
static_assert(is_virtual(bases_of(^^D)[1]));

static_assert(!is_virtual(^^B1));
static_assert(!is_virtual(^^B2));
static_assert(!is_virtual(^^D));
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
static_assert((members_of(^^S) | std::views::filter(std::meta::is_constructor) |
                                std::ranges::to<std::vector>()).size() == 4);
static_assert((members_of(^^S) | std::views::filter(std::meta::is_destructor) |
                                std::ranges::to<std::vector>()).size() == 1);
static_assert(
    (members_of(^^S) |
         std::views::filter(std::meta::is_special_member_function) |
         std::ranges::to<std::vector>()).size() == 6);

static_assert(!is_special_member_function(^^S::mem));
static_assert(!is_special_member_function(^^S::memfn));
static_assert(!is_special_member_function(^^S::TMemFn));
static_assert(!is_special_member_function(^^S::Inner));

int x;
void fn();
namespace inner {}
template <typename T> void TFn();
static_assert(!is_special_member_function(std::meta::reflect_value(3)));
static_assert(!is_special_member_function(^^int));
static_assert(!is_special_member_function(^^TFn));
static_assert(!is_special_member_function(^^TFn<int>));
static_assert(!is_special_member_function(^^::));
static_assert(!is_special_member_function(^^inner));
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
static_assert(is_deleted(^^deleted));
static_assert(is_deleted(^^S::~S));
static_assert(!is_deleted(^^not_deleted));

struct not_dflt {
    ~not_dflt();
};
static_assert(!is_defaulted(^^not_dflt::~not_dflt));

struct explicit_dflt {
    ~explicit_dflt() = default;
};
static_assert(is_defaulted(^^explicit_dflt::~explicit_dflt));

struct implicit_dflt {
};
static_assert(is_defaulted(^^implicit_dflt::~implicit_dflt));

struct impl_dflt {
    ~impl_dflt();
};
static_assert(!is_defaulted(^^impl_dflt::~impl_dflt));
impl_dflt::~impl_dflt() = default;
static_assert(is_defaulted(^^impl_dflt::~impl_dflt));

int x;
void fn();
namespace inner {}
template <typename T> void TFn();
static_assert(!is_deleted(std::meta::reflect_value(3)));
static_assert(!is_deleted(^^int));
static_assert(!is_deleted(^^TFn));
static_assert(!is_deleted(^^TFn<int>));
static_assert(!is_deleted(^^::));
static_assert(!is_deleted(^^inner));
static_assert(!is_defaulted(std::meta::reflect_value(3)));
static_assert(!is_defaulted(^^int));
static_assert(!is_defaulted(^^TFn));
static_assert(!is_defaulted(^^TFn<int>));
static_assert(!is_defaulted(^^::));
static_assert(!is_defaulted(^^inner));

static_assert(!is_deleted(std::meta::info{}));
static_assert(!is_deleted(^^int));
static_assert(!is_defaulted(std::meta::info{}));
static_assert(!is_defaulted(^^int));
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
static_assert(!is_explicit(^^S::mem));
static_assert(!is_explicit(^^S::memfn));
static_assert(!is_explicit(^^S::TMemFn));
static_assert(
    !is_explicit((members_of(^^S) |
                      std::views::filter(std::meta::is_constructor) |
                      std::ranges::to<std::vector>())[0]));
static_assert(
    !is_explicit((members_of(^^S) |
                      std::views::filter(std::meta::is_constructor_template) |
                      std::ranges::to<std::vector>())[0]));
static_assert(
    is_explicit((members_of(^^S) |
                     std::views::filter(std::meta::is_constructor) |
                     std::ranges::to<std::vector>())[1]));

static_assert(!is_explicit(^^S::operator int));
static_assert(
    !is_explicit((members_of(^^S) |
                      std::views::filter(std::meta::is_template) |
                      std::ranges::to<std::vector>())[3]));
static_assert(is_explicit(^^S::operator bool));

// P2996R3 removes support for checking 'explicit' on templates.
static_assert(
    !is_explicit((members_of(^^S) |
                      std::views::filter(std::meta::is_constructor) |
                      std::ranges::to<std::vector>())[3]));
static_assert(
    !is_explicit((members_of(^^S) |
                      std::views::filter(std::meta::is_template) |
                      std::ranges::to<std::vector>())[4]));

int x;
void fn();
namespace inner {}
template <typename T> void TFn();
static_assert(!is_explicit(std::meta::reflect_value(3)));
static_assert(!is_explicit(^^int));
static_assert(!is_explicit(^^TFn));
static_assert(!is_explicit(^^TFn<int>));
static_assert(!is_explicit(^^::));
static_assert(!is_explicit(^^inner));
}  // namespace explicit_functions

                             // ==================
                             // noexcept_functions
                             // ==================

namespace noexcept_functions {
struct S {
  // methods
  void noexcept_method() noexcept;
  void noexcept_true_method() noexcept(true);
  void noexcept_false_method() noexcept(false);
  void not_noexcept_method();
  
  // virtual methods
  // w/o defining it complains about vtable
  virtual void noexcept_virtual_method() noexcept {}
  virtual void noexcept_true_virtual_method() noexcept(true) {}
  virtual void noexcept_false_virtual_method() noexcept(false) {}
  virtual void not_noexcept_virtual_method() {}

  // template methods
  template <typename T>
  void noexcept_template_method() noexcept;
  template <typename T>
  void noexcept_true_template_method() noexcept(true);
  template <typename T>
  void noexcept_false_template_method() noexcept(false);
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
void noexcept_true_function() noexcept(true);
void noexcept_false_function() noexcept(false);
void not_noexcept_function();

// template functions
template <typename T>
void noexcept_template_function() noexcept;
template <typename T>
void noexcept_true_template_function() noexcept(true);
template <typename T>
void noexcept_false_template_function() noexcept(false);
template <typename T>
void not_noexcept_template_function();

// Everything mentioned in the proposal
// (no-)noexcept member functions
static_assert(is_noexcept(^^S::noexcept_method));
static_assert(is_noexcept(^^S::noexcept_true_method));
static_assert(!is_noexcept(^^S::noexcept_false_method));
static_assert(!is_noexcept(^^S::not_noexcept_method));

// (no-)noexcept member function types
static_assert(is_noexcept(type_of(^^S::noexcept_method)));
static_assert(is_noexcept(type_of(^^S::noexcept_true_method)));
static_assert(!is_noexcept(type_of(^^S::noexcept_false_method)));
static_assert(!is_noexcept(type_of(^^S::not_noexcept_method)));

// (no-)noexcept virtual method
static_assert(is_noexcept(^^S::noexcept_virtual_method));
static_assert(is_noexcept(^^S::noexcept_true_virtual_method));
static_assert(!is_noexcept(^^S::noexcept_false_virtual_method));
static_assert(!is_noexcept(^^S::not_noexcept_virtual_method));

// (no-)noexcept virtual method types
static_assert(is_noexcept(type_of(^^S::noexcept_virtual_method)));
static_assert(is_noexcept(type_of(^^S::noexcept_true_virtual_method)));
static_assert(!is_noexcept(type_of(^^S::noexcept_false_virtual_method)));
static_assert(!is_noexcept(type_of(^^S::not_noexcept_virtual_method)));

// (no-)noexcept instantiated template methods
static_assert(is_noexcept(^^S::noexcept_template_method<int>));
static_assert(is_noexcept(^^S::noexcept_true_template_method<int>));
static_assert(!is_noexcept(^^S::noexcept_false_template_method<int>));
static_assert(!is_noexcept(^^S::not_noexcept_template_method<int>));

// (no-)noexcept instantiated template method types
static_assert(is_noexcept(type_of(^^S::noexcept_template_method<int>)));
static_assert(is_noexcept(type_of(^^S::noexcept_true_template_method<int>)));
static_assert(!is_noexcept(type_of(^^S::noexcept_false_template_method<int>)));
static_assert(!is_noexcept(type_of(^^S::not_noexcept_template_method<int>)));

// (no-)noexcept function
static_assert(is_noexcept(^^noexcept_function));
static_assert(is_noexcept(^^noexcept_true_function));
static_assert(!is_noexcept(^^noexcept_false_function));
static_assert(!is_noexcept(^^not_noexcept_function));

// (no-)noexcept function type
static_assert(is_noexcept(type_of(^^noexcept_function)));
static_assert(is_noexcept(type_of(^^noexcept_true_function)));
static_assert(!is_noexcept(type_of(^^noexcept_false_function)));
static_assert(!is_noexcept(type_of(^^not_noexcept_function)));

// (no-)noexcept instantiated template functions
static_assert(is_noexcept(^^noexcept_template_function<int>));
static_assert(is_noexcept(^^noexcept_true_template_function<int>));
static_assert(!is_noexcept(^^noexcept_false_template_function<int>));
static_assert(!is_noexcept(^^not_noexcept_template_function<int>));

// (no-)noexcept instantiated template function types
static_assert(is_noexcept(type_of(^^noexcept_template_function<int>)));
static_assert(is_noexcept(type_of(^^noexcept_true_template_function<int>)));
static_assert(!is_noexcept(type_of(^^noexcept_false_template_function<int>)));
static_assert(!is_noexcept(type_of(^^not_noexcept_template_function<int>)));

// The rest (should all be false regardless of noexcept specifier)
// (no-)noexcept member function pointers
static_assert(!is_noexcept(std::meta::reflect_value(&S::noexcept_method)));
static_assert(!is_noexcept(std::meta::reflect_value(&S::noexcept_true_method)));
static_assert(
  !is_noexcept(std::meta::reflect_value(&S::noexcept_false_method)));
static_assert(!is_noexcept(std::meta::reflect_value(&S::not_noexcept_method)));

// (no-)noexcept member function pointer types
static_assert(
  !is_noexcept(type_of(std::meta::reflect_value(&S::noexcept_method))));
static_assert(
  !is_noexcept(type_of(std::meta::reflect_value(&S::noexcept_true_method))));
static_assert(
  !is_noexcept(type_of(std::meta::reflect_value(&S::noexcept_false_method))));
static_assert(
  !is_noexcept(type_of(std::meta::reflect_value(&S::not_noexcept_method))));

// (no-)noexcept virtual method pointers
static_assert(
  !is_noexcept(std::meta::reflect_value(&S::noexcept_virtual_method)));
static_assert(
  !is_noexcept(std::meta::reflect_value(&S::noexcept_true_virtual_method)));
static_assert(
  !is_noexcept(std::meta::reflect_value(&S::noexcept_false_virtual_method)));
static_assert(
  !is_noexcept(std::meta::reflect_value(&S::not_noexcept_virtual_method)));

// (no-)noexcept virtual method pointer types
static_assert(!is_noexcept(
  type_of(std::meta::reflect_value(&S::noexcept_virtual_method))));
static_assert(!is_noexcept(
  type_of(std::meta::reflect_value(&S::noexcept_true_virtual_method))));
static_assert(!is_noexcept(
  type_of(std::meta::reflect_value(&S::noexcept_false_virtual_method))));
static_assert(!is_noexcept(
  type_of(std::meta::reflect_value(&S::not_noexcept_virtual_method))));

// (no-)noexcept instantiated template method pointers
static_assert(!is_noexcept(
  std::meta::reflect_value(&S::noexcept_template_method<int>)));
static_assert(!is_noexcept(
  std::meta::reflect_value(&S::noexcept_true_template_method<int>)));
static_assert(!is_noexcept(
  std::meta::reflect_value(&S::noexcept_false_template_method<int>)));
static_assert(!is_noexcept(
  std::meta::reflect_value(&S::not_noexcept_template_method<int>)));

// (no-)noexcept instantiated template method pointer types
static_assert(!is_noexcept(
  type_of(std::meta::reflect_value(&S::noexcept_template_method<int>))));
static_assert(!is_noexcept(
  type_of(std::meta::reflect_value(&S::noexcept_true_template_method<int>))));
static_assert(!is_noexcept(
  type_of(std::meta::reflect_value(&S::noexcept_false_template_method<int>))));
static_assert(!is_noexcept(
  type_of(std::meta::reflect_value(&S::not_noexcept_template_method<int>))));

// (no-)noexcept lambdas
static_assert(!is_noexcept(^^noexcept_lambda));
static_assert(!is_noexcept(^^not_noexcept_lambda));

// (no-)noexcept closure types
static_assert(!is_noexcept(type_of(^^noexcept_lambda)));
static_assert(!is_noexcept(type_of(^^not_noexcept_lambda)));

// (no-)noexcept function pointer
static_assert(!is_noexcept(std::meta::reflect_value(&noexcept_function)));
static_assert(!is_noexcept(std::meta::reflect_value(&noexcept_true_function)));
static_assert(!is_noexcept(std::meta::reflect_value(&noexcept_false_function)));
static_assert(!is_noexcept(std::meta::reflect_value(&not_noexcept_function)));

// (no-)noexcept function pointer type
static_assert(
  !is_noexcept(type_of(std::meta::reflect_value(&noexcept_function))));
static_assert(
  !is_noexcept(type_of(std::meta::reflect_value(&noexcept_true_function))));
static_assert(
  !is_noexcept(type_of(std::meta::reflect_value(&noexcept_false_function))));
static_assert(
  !is_noexcept(type_of(std::meta::reflect_value(&not_noexcept_function))));

// (no-)noexcept instantiated template function pointers
static_assert(
  !is_noexcept(std::meta::reflect_value(&noexcept_template_function<int>)));
static_assert(!is_noexcept(
  std::meta::reflect_value(&noexcept_true_template_function<int>)));
static_assert(!is_noexcept(
  std::meta::reflect_value(&noexcept_false_template_function<int>)));
static_assert(
  !is_noexcept(std::meta::reflect_value(&not_noexcept_template_function<int>)));

// (no-)noexcept instantiated template function pointer types
static_assert(!is_noexcept(
  type_of(std::meta::reflect_value(&noexcept_template_function<int>))));
static_assert(!is_noexcept(
  type_of(std::meta::reflect_value(&noexcept_true_template_function<int>))));
static_assert(!is_noexcept(
  type_of(std::meta::reflect_value(&noexcept_false_template_function<int>))));
static_assert(!is_noexcept(
  type_of(std::meta::reflect_value(&not_noexcept_template_function<int>))));

// (no-)noexcept non-instantiated template methods
static_assert(!is_noexcept(^^S::noexcept_template_method));
static_assert(!is_noexcept(^^S::not_noexcept_template_method));

// (no-)noexcept generic closure types
static_assert(!is_noexcept(^^noexcept_generic_lambda));
static_assert(!is_noexcept(^^not_noexcept_generic_lambda));
static_assert(!is_noexcept(type_of(^^noexcept_generic_lambda)));
static_assert(!is_noexcept(type_of(^^not_noexcept_generic_lambda)));

// (no-)noexcept non-instantiated template functions
static_assert(!is_noexcept(^^noexcept_template_function));
static_assert(!is_noexcept(^^not_noexcept_template_function));

// Expressions that can't be noexcept
// Namespaces
static_assert(!is_noexcept(^^::));
static_assert(!is_noexcept(^^noexcept_functions));

// Non callable id-expressions & their types
struct T {
  static const int static_mem = 1;
  int non_static_mem = 2;
};

template <typename>
struct TT {};

enum class EC {
  Something
};

enum E {
  E_Something
};

static auto static_x = 1;
auto non_static_x = static_x;
auto t = T();
auto template_t = TT<int>();
int c_array[] = {1, 2};
auto [structured_binding1, structured_binding2] = c_array;

// a variable
static_assert(!is_noexcept(^^static_x));
static_assert(!is_noexcept(type_of(^^static_x)));
static_assert(!is_noexcept(^^non_static_x));
static_assert(!is_noexcept(type_of(^^non_static_x)));

// a static data member
static_assert(!is_noexcept(^^T::static_mem));
static_assert(!is_noexcept(type_of(^^T::static_mem)));

// a structured binding
static_assert(!is_noexcept(^^structured_binding1));

// a non static data member
static_assert(!is_noexcept(^^T::non_static_mem));
static_assert(!is_noexcept(type_of(^^T::non_static_mem)));

// a template
static_assert(!is_noexcept(^^TT));
static_assert(!is_noexcept(^^template_t));
static_assert(!is_noexcept(type_of(^^template_t)));

// an enum
static_assert(!is_noexcept(^^EC));
static_assert(!is_noexcept(^^EC::Something));
static_assert(!is_noexcept(type_of(^^EC::Something)));

static_assert(!is_noexcept(^^E));
static_assert(!is_noexcept(^^E_Something));
static_assert(!is_noexcept(type_of(^^E_Something)));
} // namespace noexcept_functions

                              // ================
                              // bitfield_members
                              // ================

namespace bitfield_members {
struct S {
  int i : 2;
  int j;
  static inline int k;
};

static_assert(is_bit_field(^^S::i));
static_assert(!is_bit_field(^^S::j));
static_assert(!is_bit_field(^^S::k));

static_assert(!is_bit_field(^^S));
static_assert(!is_bit_field(^^int));
static_assert(!is_bit_field(std::meta::reflect_value(4)));
static_assert(!is_bit_field(^^std::meta::extract));

static_assert(!is_bit_field(data_member_spec(^^int, {})));
static_assert(is_bit_field(data_member_spec(^^int, {.width=0})));
static_assert(is_bit_field(data_member_spec(^^int, {.width=5})));
}  // namespace bitfield_members

                           // =======================
                           // ref_qualified_functions
                           // =======================

namespace ref_qualified_functions {
struct S {
  void fn1();
  void fn2() &;
  void fn3() &&;
};

static_assert(!is_lvalue_reference_qualified(^^S::fn1));
static_assert(!is_rvalue_reference_qualified(^^S::fn1));

static_assert(is_lvalue_reference_qualified(^^S::fn2));
static_assert(!is_rvalue_reference_qualified(^^S::fn2));

static_assert(!is_lvalue_reference_qualified(^^S::fn3));
static_assert(is_rvalue_reference_qualified(^^S::fn3));

static_assert(!is_lvalue_reference_qualified(^^void(int)));
static_assert(!is_rvalue_reference_qualified(^^void(int)));

static_assert(is_lvalue_reference_qualified(^^void(int) &));
static_assert(!is_rvalue_reference_qualified(^^void(int) &));

static_assert(!is_lvalue_reference_qualified(^^void(int) &&));
static_assert(is_rvalue_reference_qualified(^^void(int) &&));

static_assert(!is_lvalue_reference_qualified(std::meta::info{}));
static_assert(!is_lvalue_reference_qualified(^^int));
static_assert(!is_lvalue_reference_qualified(^^::));
}  // namespace ref_qualified_functions

                            // ====================
                            // special_constructors
                            // ====================

namespace special_constructors {

struct S {
  S() {}
  S(int = 0) {}

  S(S&) {}
  S(const S&) {}
  S(volatile S&) {}
  S(const volatile S&) {}
  S(const S&, int = 0) {}

  S(S&&) {}
  S(const S&&) {}
  S(volatile S&&) {}
  S(const volatile S&&) {}
  S(const S&&, int = 0) {}

  void fn1() {}
};

static_assert(
    (members_of(^^S) | std::views::filter(std::meta::is_user_provided) |
                      std::views::transform(std::meta::is_default_constructor) |
                      std::ranges::to<std::vector>()) ==
    std::vector {true, true,
                 false, false, false, false, false,
                 false, false, false, false, false,
                 false});

static_assert(
    (members_of(^^S) | std::views::filter(std::meta::is_user_provided) |
                      std::views::transform(std::meta::is_copy_constructor) |
                      std::ranges::to<std::vector>()) ==
    std::vector {false, false,
                 true, true, true, true, true,
                 false, false, false, false, false,
                 false});

static_assert(
    (members_of(^^S) | std::views::filter(std::meta::is_user_provided) |
                      std::views::transform(std::meta::is_move_constructor) |
                      std::ranges::to<std::vector>()) ==
    std::vector {false, false,
                 false, false, false, false, false,
                 true, true, true, true, true,
                 false});
}  // namespace special_constructors

                            // ====================
                            // assignment_operators
                            // ====================

namespace assignment_operators {

struct S {
  void operator=(S&) {}
  S& operator=(const S&) { return *this; }
  S& operator=(volatile S&) { return *this; }
  S& operator=(const volatile S&) { return *this; }

  void operator=(S&&) {}
  S& operator=(const S&&);
  S& operator=(volatile S&&) { return *this; }
  S& operator=(const volatile S&&) { return *this; }

  S& operator=(int) { return *this; }

  void fn1() {}
};

static_assert(
    (members_of(^^S) | std::views::filter(std::meta::is_user_provided) |
                      std::views::transform(std::meta::is_assignment) |
                      std::ranges::to<std::vector>()) ==
    std::vector {true, true, true, true,
                 true, true, true, true,
                 true,
                 false});

static_assert(
    (members_of(^^S) | std::views::filter(std::meta::is_user_provided) |
                      std::views::transform(std::meta::is_copy_assignment) |
                      std::ranges::to<std::vector>()) ==
    std::vector {true, true, true, true,
                 false, false, false, false,
                 false,
                 false});

static_assert(
    (members_of(^^S) | std::views::filter(std::meta::is_user_provided) |
                      std::views::transform(std::meta::is_move_assignment) |
                      std::ranges::to<std::vector>()) ==
    std::vector {false, false, false, false,
                 true, true, true, true,
                 false,
                 false});
}  // namespace assignment_operators

                             // ===================
                             // member_initializers
                             // ===================

namespace member_initializers {

struct S {
  int a;
  int b = 3;
};

static_assert(
    (nonstatic_data_members_of(^^S) |
        std::views::transform(std::meta::has_default_member_initializer) |
        std::ranges::to<std::vector>()) == std::vector {false, true});

static_assert(!has_default_member_initializer(^^int));
static_assert(!has_default_member_initializer(^^::));
}  // namespace member_initializers

                             // ==================
                             // const_and_volatile
                             // ==================

namespace const_and_volatile {

struct S {
    void fn1();
    void fn2() const;
    void fn3() volatile;
};

int v1;

const int v2 = 0;
const int arr1[] = {1, 2};

volatile int v3;
volatile int arr2[] = {1, 2};

static_assert(!is_const(^^S::fn1));
static_assert(is_const(^^S::fn2));
static_assert(!is_const(^^S::fn3));

static_assert(!is_const(^^v1));
static_assert(is_const(^^v2));
static_assert(is_const(^^arr1));
static_assert(is_const(std::meta::reflect_object(arr1[1])));
static_assert(!is_const(^^v3));
static_assert(!is_const(^^arr2));
static_assert(!is_const(std::meta::reflect_object(arr2[1])));

static_assert(!is_volatile(^^S::fn1));
static_assert(!is_volatile(^^S::fn2));
static_assert(is_volatile(^^S::fn3));

static_assert(!is_volatile(^^v1));
static_assert(!is_volatile(^^v2));
static_assert(!is_volatile(^^arr1));
static_assert(!is_volatile(std::meta::reflect_object(arr1[1])));
static_assert(is_volatile(^^v3));
static_assert(is_volatile(^^arr2));
static_assert(is_volatile(std::meta::reflect_object(arr2[1])));

static_assert(!is_const(^^int));
static_assert(is_const(^^const int));
static_assert(is_const(^^const volatile int));
static_assert(!is_const(^^volatile int));
static_assert(is_const(^^void() const));

static_assert(!is_volatile(^^int));
static_assert(!is_volatile(^^const int));
static_assert(is_volatile(^^const volatile int));
static_assert(is_volatile(^^volatile int));
static_assert(is_volatile(^^void() volatile));
}  // namespace const_and_volatile

                     // ==================================
                     // operators_and_conversion_functions
                     // ==================================

namespace operators_and_conversion_functions {
struct S {
  S &operator+(const S&);

  template <typename T>
  S &operator-(const S&);

  operator int();

  void fn();
};

struct T {
  template <typename T>
  operator T();
};

bool operator&&(const S&, const S&);

template <typename T>
bool operator||(const S&, const T&);

int operator""_a(const char *);

template<char...>
int operator""_b();


constexpr auto conversion_template =
    (members_of(^^T) | std::views::filter(std::meta::is_template)).front();

static_assert(is_operator_function(^^S::operator+));
static_assert(is_operator_function(^^operator&&));
static_assert(!is_operator_function(^^operator||));
static_assert(!is_operator_function(^^S::operator-));
static_assert(is_operator_function(^^S::operator-<int>));
static_assert(!is_operator_function(conversion_template));
static_assert(!is_operator_function(^^S::fn));
static_assert(!is_operator_function(^^operator""_a));
static_assert(!is_operator_function(^^operator""_b));

static_assert(!is_operator_function_template(^^S::operator+));
static_assert(!is_operator_function_template(^^operator&&));
static_assert(is_operator_function_template(^^operator||));
static_assert(is_operator_function_template(^^S::operator-));
static_assert(!is_operator_function_template(^^S::operator int));
static_assert(!is_operator_function_template(conversion_template));
static_assert(!is_operator_function_template(^^S::fn));
static_assert(!is_operator_function_template(^^operator""_a));
static_assert(!is_operator_function_template(^^operator""_b));

static_assert(!is_conversion_function(^^S::operator+));
static_assert(!is_conversion_function(^^operator&&));
static_assert(!is_conversion_function(^^operator||));
static_assert(!is_conversion_function(^^S::operator-));
static_assert(is_conversion_function(^^S::operator int));
static_assert(!is_conversion_function(conversion_template));
static_assert(!is_conversion_function(^^S::fn));
static_assert(!is_conversion_function(^^operator""_a));
static_assert(!is_conversion_function(^^operator""_b));

static_assert(!is_conversion_function_template(^^S::operator+));
static_assert(!is_conversion_function_template(^^operator&&));
static_assert(!is_conversion_function_template(^^operator||));
static_assert(!is_conversion_function_template(^^S::operator-));
static_assert(!is_conversion_function_template(^^S::operator int));
static_assert(is_conversion_function_template(conversion_template));
static_assert(!is_conversion_function_template(^^S::fn));
static_assert(!is_conversion_function_template(^^operator""_a));
static_assert(!is_conversion_function_template(^^operator""_b));

static_assert(!is_literal_operator(^^S::operator+));
static_assert(!is_literal_operator(^^operator&&));
static_assert(!is_literal_operator(^^operator||));
static_assert(!is_literal_operator(^^S::operator-));
static_assert(!is_literal_operator(^^S::operator-<int>));
static_assert(!is_literal_operator(conversion_template));
static_assert(!is_literal_operator(^^S::fn));
static_assert(is_literal_operator(^^operator""_a));
static_assert(!is_literal_operator(^^operator""_b));

static_assert(!is_literal_operator_template(^^S::operator+));
static_assert(!is_literal_operator_template(^^operator&&));
static_assert(!is_literal_operator_template(^^operator||));
static_assert(!is_literal_operator_template(^^S::operator-));
static_assert(!is_literal_operator_template(^^S::operator-<int>));
static_assert(!is_literal_operator_template(conversion_template));
static_assert(!is_literal_operator_template(^^S::fn));
static_assert(!is_literal_operator_template(^^operator""_a));
static_assert(is_literal_operator_template(^^operator""_b));

}  // namespace operators_and_conversion_functions

int main() { }
