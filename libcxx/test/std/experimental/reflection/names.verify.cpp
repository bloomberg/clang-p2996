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
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

// <experimental/reflection>
//
// [reflection]

#include <meta>

constexpr auto ctx = std::meta::access_context::unchecked();

                            // ==================
                            // cv_qualified_types
                            // ==================

namespace cv_qualified_types {
struct S {};
enum E { e };
using CS = const S;
template <typename> struct TBase {};
struct D : TBase<int> {};

// [meta.reflection.names]/1.4, /4: a cv-qualified class or enumeration type
// has no identifier.
constexpr auto r1 = std::meta::identifier_of(^^const S);
  // expected-error@-1 {{must be initialized by a constant expression}} \
  // expected-note@-1 {{cv-qualified type 'const cv_qualified_types::S' has no associated identifier}}
constexpr auto r2 = std::meta::identifier_of(^^volatile E);
  // expected-error@-1 {{must be initialized by a constant expression}} \
  // expected-note@-1 {{cv-qualified type 'volatile cv_qualified_types::E' has no associated identifier}}
constexpr auto r3 = std::meta::identifier_of(^^const CS);
  // expected-error@-1 {{must be initialized by a constant expression}} \
  // expected-note@-1 {{cv-qualified type 'const cv_qualified_types::S' has no associated identifier}}

// /1.12, /3.5: a direct base class relationship whose base is a template
// specialization has no identifier.
constexpr auto r4 = std::meta::identifier_of(std::meta::bases_of(^^D, ctx)[0]);
  // expected-error@-1 {{must be initialized by a constant expression}} \
  // expected-note@-1 {{names of template specializations are not identifiers}}
}  // namespace cv_qualified_types
