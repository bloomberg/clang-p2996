// This covers PCH serialization of ExplDependentCallExpr (bloomberg/clang-p2996#329).
// RUN: %clang_cc1 -std=c++2c -freflection -ast-dump %s | FileCheck %s
// RUN: %clang_cc1 -std=c++2c -freflection -emit-pch %s -o %t
// RUN: %clang_cc1 -std=c++2c -freflection -include-pch %t -verify %s
// expected-no-diagnostics

// CHECK: ExplDependentCallExpr

#ifndef HEADER_INCLUDED
#define HEADER_INCLUDED

struct AccessContext {
  [[clang::instantiation_dependent]]
  static consteval int current() { return 42; }
};

template <typename T>
consteval int call_current(T) {
  return AccessContext::current();
}

#else

static_assert(call_current(0) == 42);

#endif
