//===----------------------------------------------------------------------===//
//
// Copyright 2024 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// RUN: %clang_cc1 %s -std=c++23 -freflection -freflection-new-syntax

using info = decltype(^^int);

namespace myns {
namespace inner {}

struct Test {
  using type = int;
};
constexpr info rTest = ^^Test::type;
}

struct Test {
  using type = int;
};

void fn() {}
void fn2() {}

int var = 0;
int var2 = 0;

struct WithMembers {
  int datamem;
  void memfn() {}

  static int staticdatamem;

  struct inner {};

  template <typename T>
  void tmemfn() {}
};

enum Enum { A, B, C };
enum class EnumCls { A, B, C };

template <typename T> void tfn() {}
template <typename T> void tfn2() {}
template <typename T> struct TCls {};
template <typename T> struct TCls2 {};
template <typename T> constexpr int tvar = 0;
template <typename T> constexpr int tvar2 = 0;
template <typename T> concept Concept = requires() { true; };
template <typename T> concept Concept2 = requires() { true; };

using int_alias = int;

namespace NsAlias = ::myns;


// Ensure that different entities compare equally to themselves.
constexpr info null1;     // default-initialized
constexpr info null2 {};  // zero-initialized
constexpr info refl = ^^int;

static_assert(null1 == null1);
static_assert(null2 == null2);
static_assert(null1 == null2);

static_assert(^^int == ^^int);
static_assert(^^int == refl);
static_assert(^^A == ^^A);
static_assert(^^EnumCls::A == ^^EnumCls::A);
static_assert(^^Enum == ^^Enum);
static_assert(^^EnumCls == ^^EnumCls);
static_assert(^^int_alias == ^^int_alias);
static_assert(^^Test::type == ^^Test::type);
static_assert(^^Test == ^^Test);
static_assert(^^Test == ^^::Test);
static_assert(^^myns::Test::type == ^^myns::Test::type);
static_assert(^^myns::Test::type == myns::rTest);
static_assert(^^myns::rTest == ^^myns::rTest);
static_assert(^^:: == ^^::);
static_assert(^^myns == ^^myns);
static_assert(^^myns == ^^::myns);
static_assert(^^myns::inner == ^^::myns::inner);
static_assert(^^NsAlias == ^^NsAlias);
static_assert(^^fn == ^^fn);
static_assert(^^var == ^^var);
static_assert(^^tfn == ^^tfn);
static_assert(^^TCls == ^^TCls);
static_assert(^^tvar == ^^tvar);
static_assert(^^Concept == ^^Concept);
static_assert(^^WithMembers::datamem == ^^WithMembers::datamem);
static_assert(^^WithMembers::memfn == ^^WithMembers::memfn);
static_assert(^^WithMembers::staticdatamem == ^^WithMembers::staticdatamem);
static_assert(^^WithMembers::inner == ^^WithMembers::inner);
static_assert(^^WithMembers::tmemfn == ^^WithMembers::tmemfn);
static_assert(^^myns::rTest == ^^myns::rTest);

// Check equality semantics of types and type aliases.
using int_alias = int;
static_assert(^^int != null1);
static_assert(^^int != ^^void);
static_assert(^^int != ^^unsigned int);
static_assert(^^int != ^^Enum);
static_assert(^^int != ^^EnumCls);
static_assert(^^int_alias != ^^int);
static_assert(^^int_alias != ^^Test::type);
static_assert(^^int_alias != ^^myns::Test::type);
static_assert(^^Test::type != ^^myns::Test::type);
static_assert(^^decltype(int_alias{}) == ^^int);
static_assert(^^decltype(int_alias{}) != ^^int_alias);

// Check equality semantics of enumerations and enumerators.
static_assert(^^Enum::A != ^^Enum::B);
static_assert(^^Enum::A != ^^EnumCls::A);
static_assert(^^EnumCls::A != ^^EnumCls::B);

// Check equality semantics of some different entities.
static_assert(^^fn != ^^fn2);
static_assert(^^fn != ^^var);
static_assert(^^fn != ^^tvar);
static_assert(^^decltype(&fn) == ^^void(*)());
static_assert(^^var != ^^var2);
static_assert(^^tfn<int> != ^^tfn<void>);

// Check equality semantics of templates.
static_assert(^^tfn != ^^tfn2);
static_assert(^^tfn != ^^TCls);
static_assert(^^TCls != ^^TCls2);
static_assert(^^tfn != ^^tvar);
static_assert(^^tvar != ^^tvar2);
static_assert(^^Concept != ^^Concept2);

// Check equality semantics of namespaces.
static_assert(^^:: != ^^::myns);
static_assert(^^::myns != ^^NsAlias);
static_assert(^^::myns != ^^::myns::inner);

constexpr int i = 42;
constexpr auto i_refl = ^^i;
constexpr auto i_refl_copy = i_refl;
static_assert(i_refl == ^^i);
static_assert(i_refl == i_refl_copy);

constexpr int j = 42;
static_assert(^^i != ^^j);

consteval info local_var_reflection() {
    int i;
    return ^^i;
}
static_assert(local_var_reflection() == local_var_reflection());

// Compare reflections of the same local variable in different stack frames.
namespace local_variables_different_stack_frames {
consteval bool local_var_in_diff_frames_equal(info inf, int call_depth = 0) {
    int lcl = call_depth;
    if (call_depth > 0) {
        // These should not be equal.
        // They are different variables with different values, which
        // can be accessed with value_of()
        return inf == ^^lcl;
    }
    else {
        return local_var_in_diff_frames_equal(^^lcl, 1);
    }
}

}  // namespace local_variables_different_stack_frames
