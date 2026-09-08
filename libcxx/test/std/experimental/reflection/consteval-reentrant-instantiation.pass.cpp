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
// Regression test: evaluating a deferred immediate invocation may REENTER Sema
// through reflection metafunctions — substitute() instantiates the named
// specialization (and, for an undeduced signature, its body), and each nested
// instantiation pushes expression evaluation contexts. Sema used to hold a
// reference into ExprEvalContexts (a SmallVector) across that evaluation in
// PopExpressionEvaluationContext / HandleImmediateInvocations; once the vector
// reallocated, the reference dangled — a use-after-free whose crash was heap-
// layout dependent (deterministic under allocators that scribble freed memory,
// e.g. macOS MallocScribble=1, or ASan).
//
// This test builds a 64-deep instantiation cascade that runs entirely inside
// the deferred evaluation: start(n) takes its depth as an evaluation-time
// value, so nothing instantiates at parse time, and chain<N>'s deduced return
// type forces substitute() to instantiate each body, whose dependent
// splice-of-substitute recurses one level deeper. The stack of evaluation
// contexts crosses several SmallVector growth thresholds while the popped
// record is held.

#include <meta>

#include <cassert>
#include <vector>

template <int N>
consteval auto chain() {  // deduced return type => substitute() instantiates the body
  if constexpr (N <= 0) {
    return 0;
  } else {
    return [:std::meta::substitute(
        ^^chain,
        std::vector<std::meta::info>{std::meta::reflect_constant(N - 1)}):]() + 1;
  }
}

consteval int start(int n) {
  return std::meta::extract<int (*)()>(std::meta::substitute(
      ^^chain, std::vector<std::meta::info>{std::meta::reflect_constant(n)}))();
}

int main() {
  // Runtime context: start(64) is deferred as an immediate-invocation candidate
  // and evaluated during PopExpressionEvaluationContext.
  int x = start(64);
  assert(x == 64);
  return 0;
}
