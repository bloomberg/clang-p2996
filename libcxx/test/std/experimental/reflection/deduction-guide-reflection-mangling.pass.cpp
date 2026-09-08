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
// Regression test: a reflection of a DEDUCTION GUIDE used as a template
// argument (a define_static_array element / reflection NTTP) must mangle
// rather than hitting mangleUnqualifiedName's
//   llvm_unreachable("Can't mangle a deduction guide name!")
// (a CXXDeductionGuideName has no <unqualified-name> encoding), and two
// different guides for the same template must mangle DISTINCTLY -- they share
// one DeclarationName, so a name-only encoding would fold their
// specializations at codegen (the same silent failure mode as overloaded
// function templates, different declaration-name kind). members_of over a namespace enumerates guides like
// any other member, so any namespace-walking reflection consumer that lifts
// the member list into static storage hits this. -fsyntax-only does NOT
// reproduce; the assertions below require codegen and a runtime observation.

#include <meta>

#include <cassert>

namespace demo {
template <class E> struct unexpected { unexpected(E); };
template <class E> unexpected(E)  -> unexpected<E>;    // guide #1
template <class E> unexpected(E*) -> unexpected<E*>;   // guide #2 (same DeclarationName)
}  // namespace demo

template <std::meta::info R> int probe() { return 1; }

int main() {
  // The lift itself is the ICE shape: the array backing define_static_array
  // has every element reflection mangled into its linkage name. Taking
  // &probe<m> additionally pins each guide reflection as a function-template
  // NTTP, whose mangled names must be pairwise distinct. The enumeration
  // contains FOUR guides: the two explicit ones plus the two Sema-declared
  // implicit ones (the per-constructor guide -- whose signature is
  // structurally IDENTICAL to explicit guide #1 -- and the copy guide), so
  // distinctness needs more than a structural hash of the declaration.
  int (*addrs[8])() = {};
  int n = 0;
  template for (constexpr auto m : std::define_static_array(
      std::meta::members_of(^^demo, std::meta::access_context::unchecked()))) {
    if constexpr (std::meta::is_function_template(m)) {  // only the guides
      addrs[n++] = &probe<m>;
    }
  }
  assert(n == 4);
  for (int i = 0; i < n; ++i) {
    assert(addrs[i]() == 1);
    // Distinct manglings: without the per-guide discriminator the
    // linkonce_odr specializations silently fold into one symbol.
    for (int j = i + 1; j < n; ++j)
      assert(addrs[i] != addrs[j]);
    // A reflection of a guide stays distinct from a reflection of the
    // deduced class template itself.
    assert((void *)&probe<^^demo::unexpected> != (void *)addrs[i]);
  }
  return 0;
}
