//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// RUN: %clang_cc1 %s -std=c++26 -freflection -ftoken-injection \
// RUN:   -triple x86_64-unknown-linux-gnu -emit-llvm -o - | FileCheck %s

#include "Inputs/token-injection.h"

using namespace std::meta;

// Declarations injected at namespace scope are emitted exactly once.

consteval { queue_injection(^^{ int queued_global = 1; }); }
// CHECK-DAG: @queued_global = global i32 1

consteval { queue_injection(^^{ int queued_function() { return 2; } }); }
// CHECK-DAG: define {{.*}}i32 @_Z15queued_functionv()

namespace open {
consteval { namespace_inject(^^{ int injected_open = 3; }); }
// CHECK-DAG: @_ZN4open13injected_openE = global i32 3
consteval { queue_injection(^^{ int queued_open() { return 4; } }); }
// CHECK-DAG: define {{.*}}i32 @_ZN4open11queued_openEv()
}  // namespace open

namespace closed { namespace inner {} }
consteval {
  namespace_inject(^^closed, ^^{ int injected_closed = 5; });
  namespace_inject(^^closed::inner, ^^{ int injected_inner() { return 6; } });
  namespace_inject(^^::, ^^{ int injected_global = 7; });
}
// CHECK-DAG: @_ZN6closed15injected_closedE = global i32 5
// CHECK-DAG: define {{.*}}i32 @_ZN6closed5inner14injected_innerEv()
// CHECK-DAG: @injected_global = global i32 7

// From within a function, and from within a class.
void host() {
  consteval { namespace_inject(^^::, ^^{ int from_function = 8; }); }
  consteval { queue_injection(^^{ static int local_static = 9; (void)local_static; }); }
}
// CHECK-DAG: @from_function = global i32 8
// CHECK-DAG: @_ZZ4hostvE12local_static = internal global i32 9

struct Host {
  consteval { namespace_inject(^^closed, ^^{ int from_class = 10; }); }
  consteval { queue_injection(^^{ int member() { return 11; } static int smember; }); }
};
int Host::smember = 12;
int use(Host h) { return h.member(); }
// CHECK-DAG: @_ZN6closed10from_classE = global i32 10
// CHECK-DAG: @_ZN4Host7smemberE = global i32 12
// CHECK-DAG: define {{.*}}i32 @_ZN4Host6memberEv(

// Members injected into an instantiated class.
template <typename T>
struct Tmpl {
  consteval { queue_injection(^^{ \[:^^T:] get() { return 13; } }); }
};
int use(Tmpl<int> t) { return t.get(); }
// CHECK-DAG: define {{.*}}i32 @_ZN4TmplIiE3getEv(

// Values and strings.
consteval { queue_injection(^^{ long value_global = \val(14L); const char *string_global = \str("text"); }); }
// CHECK-DAG: @value_global = global i64 14
// CHECK-DAG: c"text\00"

// None of them is emitted twice.
// CHECK-NOT: @queued_global.{{[0-9]}}
