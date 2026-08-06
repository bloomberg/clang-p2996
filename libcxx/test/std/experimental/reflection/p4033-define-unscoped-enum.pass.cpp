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
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest -Wno-deprecated-declarations

#include <meta>

using namespace std::meta;


                       // =========================
                       // basic: simple enumerators
                       // =========================

namespace basic {

enum Color : int;
consteval {
  define_unscoped_enum(^^Color, {
    enumerator_spec({.name = "Red"}),
    enumerator_spec({.name = "Green"}),
    enumerator_spec({.name = "Blue"}),
  });
}

static_assert(static_cast<int>(Color::Red) == 0);
static_assert(static_cast<int>(Color::Green) == 1);
static_assert(static_cast<int>(Color::Blue) == 2);
static_assert(is_complete_type(^^Color));

// Unscoped-enum specific: enumerators are visible unqualified in the
// enclosing scope, and implicitly convert to their underlying type.
static_assert(Red == Color::Red);
static_assert(static_cast<int>(Red) == 0);
constexpr int implicitlyConverted = Green;
static_assert(implicitlyConverted == 1);

}  // namespace basic


                    // ====================
                    // user-provided values
                    // ====================

namespace explicit_values {

enum Status : int;
consteval {
  define_unscoped_enum(^^Status, {
    enumerator_spec({.name = "OK",      .value = reflect_constant(0)}),
    enumerator_spec({.name = "Warning", .value = reflect_constant(10)}),
    enumerator_spec({.name = "Error",   .value = reflect_constant(20)}),
  });
}

static_assert(static_cast<int>(Status::OK) == 0);
static_assert(static_cast<int>(Status::Warning) == 10);
static_assert(static_cast<int>(Status::Error) == 20);

}  // namespace explicit_values


                   // ============
                   // mixed_values
                   // ============

namespace mixed_values {

enum Mixed : int;
consteval {
  define_unscoped_enum(^^Mixed, {
    enumerator_spec({.name = "A"}),                    // 0
    enumerator_spec({.name = "B"}),                    // 1
    enumerator_spec({.name = "C", .value = reflect_constant(10)}),     // 10
    enumerator_spec({.name = "D"}),                    // 11
  });
}

static_assert(static_cast<int>(Mixed::A) == 0);
static_assert(static_cast<int>(Mixed::B) == 1);
static_assert(static_cast<int>(Mixed::C) == 10);
static_assert(static_cast<int>(Mixed::D) == 11);

}  // namespace mixed_values


                      // =======================
                      // single_enumerator
                      // =======================

namespace single_enumerator {

enum Singleton : int;
consteval {
  define_unscoped_enum(^^Singleton, {
    enumerator_spec({.name = "Only"}),
  });
}

static_assert(static_cast<int>(Singleton::Only) == 0);

}  // namespace single_enumerator


                       // ==========
                       // empty enum
                       // ==========

namespace empty_enum {

enum Empty : int;
consteval {
  define_unscoped_enum(^^Empty, std::vector<info>{});
}

static_assert(is_complete_type(^^Empty));

}  // namespace empty_enum


                    // ========
                    // bitflags
                    // ========

namespace bitflags {

enum Flags : unsigned;
consteval {
  define_unscoped_enum(^^Flags, {
    enumerator_spec({.name = "None",    .value = reflect_constant(0u)}),
    enumerator_spec({.name = "Read",    .value = reflect_constant(1u)}),
    enumerator_spec({.name = "Write",   .value = reflect_constant(2u)}),
    enumerator_spec({.name = "Execute", .value = reflect_constant(4u)}),
  });
}

// Unscoped bitflags do not even need a static_cast to combine.
constexpr unsigned rw = Read | Write;
static_assert(rw == 3);

}  // namespace bitflags


                    // ===================================
                    // reflect enumerators of defined enum
                    // ===================================

namespace reflect_after_define {

enum Dir : int;
consteval {
  define_unscoped_enum(^^Dir, {
    enumerator_spec({.name = "North"}),
    enumerator_spec({.name = "South"}),
    enumerator_spec({.name = "East"}),
    enumerator_spec({.name = "West"}),
  });
}

static_assert(is_enumerator(^^Dir::North));
static_assert(is_enumerator(^^Dir::West));
static_assert(is_type(^^Dir));
static_assert(!is_scoped_enum_type(^^Dir));
static_assert(enumerators_of(^^Dir).size() == 4);
static_assert(identifier_of(^^Dir::East) == "East");

}  // namespace reflect_after_define


              // ========================================
              // templated: define_unscoped_enum inside a
              // class template
              // ========================================

namespace templated {

template <typename T>
struct Holder {
  enum kind : int;
  consteval {
    define_unscoped_enum(^^kind, {
      enumerator_spec({.name = "First"}),
      enumerator_spec({.name = "Second"}),
    });
  }
};

// Each instantiation gets its own independent enum.
using H1 = Holder<int>;
using H2 = Holder<float>;

static_assert(static_cast<int>(H1::kind::First) == 0);
static_assert(static_cast<int>(H1::kind::Second) == 1);
static_assert(static_cast<int>(H2::kind::First) == 0);
static_assert(static_cast<int>(H2::kind::Second) == 1);

// Unscoped member enum: enumerators are also visible directly through the
// enclosing class, without going through the enum's own name.
static_assert(static_cast<int>(H1::First) == 0);
static_assert(static_cast<int>(H1::Second) == 1);

}  // namespace templated


                 // =============
                 // dynamic_names
                 // =============

namespace dynamic_names {

struct MessageA {};
struct MessageB {};
struct MessageC {};

template <typename... Ts>
struct Registry {
  enum kind : int;
  consteval {
    std::vector<info> specs;
    for (auto type : {^^Ts...}) {
      specs.push_back(enumerator_spec({
        .name = std::string(identifier_of(type)),
      }));
    }
    define_unscoped_enum(^^kind, specs);
  }
};

using R = Registry<MessageA, MessageB, MessageC>;

static_assert(static_cast<int>(R::kind::MessageA) == 0);
static_assert(static_cast<int>(R::kind::MessageB) == 1);
static_assert(static_cast<int>(R::kind::MessageC) == 2);
static_assert(identifier_of(^^R::kind::MessageC) == "MessageC");

}  // namespace dynamic_names

int main() {
  return 0;
}
