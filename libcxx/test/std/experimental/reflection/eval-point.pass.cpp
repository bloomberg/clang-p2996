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
// [meta.reflection.scope]/3: the point at which a scope query is evaluated.

#include <meta>

using namespace std::meta;

                              // ===============
                              // consteval_block
                              // ===============

// [meta.reflection.scope]/3.5: a point in a consteval block is evaluated at
// the point inhabited by the outermost consteval block; the function call
// operator of its closure type is transparent to parent_of
// ([meta.reflection.queries]/51.4.1).
namespace consteval_block {
consteval { static_assert(access_context::current().scope() == ^^consteval_block); }
consteval { static_assert(current_namespace() == ^^consteval_block); }
consteval {
  [[maybe_unused]] int local = 0;
  static_assert(parent_of(^^local) == ^^consteval_block);
  consteval {
    static_assert(access_context::current().scope() == ^^consteval_block);
    [[maybe_unused]] int nested = 0;
    static_assert(parent_of(^^nested) == ^^consteval_block);
  }
  // A lambda in a consteval block introduces its own scope.
  []() {
    static_assert(access_context::current().scope() != ^^consteval_block);
  }();
}

struct C {
  consteval { static_assert(access_context::current().scope() == ^^C); }
  consteval { static_assert(current_class() == ^^C); }
};

void fn() {
  consteval { static_assert(access_context::current().scope() == ^^fn); }
  consteval { static_assert(current_function() == ^^fn); }
}

template <typename T> struct TC {
  consteval { static_assert(access_context::current().scope() == ^^TC<T>); }
  consteval {
    [[maybe_unused]] int local = 0;
    static_assert(parent_of(^^local) == ^^TC<T>);
  }
};
template struct TC<int>;
}  // namespace consteval_block

                          // =======================
                          // lambda_trailing_return
                          // =======================

// [meta.reflection.scope]/3.4: a point in the trailing-return-type of a
// lambda-expression is evaluated at the point of the lambda-introducer.
namespace lambda_trailing_return {
void fn() {
  auto l = []() -> [: access_context::current().scope() == ^^fn ? ^^int
                                                                 : ^^void :] {
    static_assert(access_context::current().scope() != ^^fn);
    return 0;
  };
  (void)l;
}
struct C {
  void mfn() {
    auto l = []() -> [: current_function() == ^^C::mfn ? ^^int : ^^void :] {
      return 0;
    };
    (void)l;
  }
};
[[maybe_unused]] constexpr auto at_namespace_scope =
    []() -> [: is_namespace(access_context::current().scope()) ? ^^int
                                                                : ^^void :] {
  return 0;
};
}  // namespace lambda_trailing_return

                          // ========================
                          // trailing_requires_clause
                          // ========================

// [meta.reflection.scope]/3.3: a point in the trailing requires-clause of a
// function declaration is evaluated in the scope enclosing the declaration.
namespace trailing_requires_clause {
template <typename T>
void fn() requires (access_context::current().scope() ==
                    ^^trailing_requires_clause) {}
template <typename T>
void fn2() requires (!is_function(access_context::current().scope())) {}
void use() { fn<int>(); fn2<int>(); }

struct C {
  template <typename T>
  static consteval bool mfn()
      requires (access_context::current().scope() == ^^C) { return true; }
};
static_assert(C::mfn<int>());
}  // namespace trailing_requires_clause

int main() {}
