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
// ADDITIONAL_COMPILE_FLAGS: -fannotation-attributes

// <experimental/reflection>
//
// [reflection]

#include <meta>


                            // =====================
                            // consteval_block_tuple
                            // =====================

namespace consteval_block_tuple {

template<typename... Ts> struct Tuple {
  struct storage;
  consteval {
    define_aggregate(^^storage, {data_member_spec(^^Ts, {.name="_"})...});
  }
  storage data;
};

Tuple<int, bool, char> tup;
}  // namespace consteval_block_tuple

                         // ===========================
                         // consteval_block_annotations
                         // ===========================

namespace consteval_block_annotations {

struct S {
  consteval {
    annotate(^^S, std::meta::reflect_constant(42));
  }
};

static_assert(extract<int>(annotations_of(^^S)[0]) == 42);

}  // namespace consteval_block_annotations

                             // ==================
                             // non_void_returning
                             // ==================

namespace non_void_returning {
consteval { return 1; }  // expected-error {{should not return a value}}

}  // namespace non_void_returning

int main() { }
