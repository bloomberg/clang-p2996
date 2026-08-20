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
// Regression test: `extern "C" { typedef struct X X; }` creates an implicit
// tag whose SEMANTIC context is the enclosing file scope while it sits
// LEXICALLY inside the linkage-spec block. Stepping the member walk from that
// tag via the semantic chain walked backward out of the block and silently
// ENDED the enumeration -- every member declared after the block vanished
// from members_of (Python.h's pytypedefs.h truncated members_of(^^::)
// mid-header, hiding all later global declarations from reflection).

#include <experimental/meta>

extern "C" {
typedef struct PMD PMD;          // implicit tag: semantic ctx = file scope
}
extern "C" {
struct PMD { int x; };           // defined in a later block (Python.h shape)
}

struct After { int y; };                                  // must stay visible
inline bool operator==(const After &, const After &) { return true; }

consteval int count_after_entities() {
  int n = 0;
  for (auto m : std::meta::members_of(^^::,
                                      std::meta::access_context::unchecked())) {
    // Identifier filter FIRST: probing is_class_type across every TU member
    // would also exercise the (separately fixed) NEON-typedef mangling gap.
    if (std::meta::has_identifier(m) &&
        std::meta::identifier_of(m) == "After" &&
        std::meta::is_type(m) && std::meta::is_class_type(m))
      ++n;
    if (std::meta::is_function(m) && !std::meta::is_template(m) &&
        std::meta::is_operator_function(m)) {
      for (auto p : std::meta::parameters_of(m)) {
        auto t = std::meta::remove_cvref(std::meta::type_of(p));
        if (std::meta::is_same_type(t, ^^After)) {
          ++n;
          break;
        }
      }
    }
  }
  return n;
}

// The same shape inside a namespace must also keep walking.
namespace wrapped {
extern "C" {
typedef struct WPMD WPMD;
}
struct NsAfter { int z; };
} // namespace wrapped

consteval bool ns_sees_after() {
  for (auto m : std::meta::members_of(^^wrapped,
                                      std::meta::access_context::unchecked()))
    if (std::meta::has_identifier(m) &&
        std::meta::identifier_of(m) == "NsAfter" &&
        std::meta::is_type(m) && std::meta::is_class_type(m))
      return true;
  return false;
}

int main() {
  static_assert(count_after_entities() == 2);   // the class + its operator==
  static_assert(ns_sees_after());
  return 0;
}
