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
// ADDITIONAL_COMPILE_FLAGS: -freflection -freflection-new-syntax
// ADDITIONAL_COMPILE_FLAGS: -Wno-unneeded-internal-declaration -Wno-unused-variable -Wno-unused-value

// <experimental/reflection>
//
// [reflection]

#include <experimental/meta>

namespace NS {
struct A {
  consteval int fn() const { return 24; }
};
} // namespace NS

struct A {
  consteval int fn() const { return 42; }

  consteval void void_fn() const {
    // no-op
  };
};

struct B {};

int main() {
              // ======================
              // non-static member functions
              // ======================
 constexpr A expectedClass{};
 reflect_invoke(^^A::fn, {^^expectedClass}); // ok

 reflect_invoke(^^A::void_fn, {^^expectedClass});
 // expected-error-re@-1 {{call to consteval function 'std::meta::reflect_invoke<{{.*}}>' is not a constant expression}}
 // expected-note@-2 {{cannot invoke reflection of void-returning function}}

 reflect_invoke(^^A::fn, {});
 // expected-error-re@-1 {{call to consteval function 'std::meta::reflect_invoke<{{.*}}>' is not a constant expression}}
 // expected-note@-2 {{expected related object reflection as a first argument for invoking non-static member function}}

 reflect_invoke(^^A::fn, {std::meta::reflect_value(42)});
 // expected-error-re@-1 {{call to consteval function 'std::meta::reflect_invoke<{{.*}}>' is not a constant expression}}
 // expected-note@-2 {{expected related object reflection as a first argument for invoking non-static member function}}

 constexpr B differentClass{};
 reflect_invoke(^^A::fn, {^^differentClass});
 // expected-error-re@-1 {{call to consteval function 'std::meta::reflect_invoke<{{.*}}>' is not a constant expression}}
 // expected-note@-2 {{method is not a member of given object reflection}}

 constexpr NS::A differentNamespaceClass{};
 reflect_invoke(^^A::fn, {^^differentNamespaceClass});
 // expected-error-re@-1 {{call to consteval function 'std::meta::reflect_invoke<{{.*}}>' is not a constant expression}}
 // expected-note@-2 {{method is not a member of given object reflection}}
}
