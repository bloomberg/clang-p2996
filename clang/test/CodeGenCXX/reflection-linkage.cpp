// RUN: %clang_cc1 %s -std=c++26 -freflection -triple x86_64-pc-linux-gnu -emit-llvm -o - | FileCheck %s
// RUN: %clang_cc1 %s -std=c++26 -freflection -triple x86_64-pc-windows-msvc -emit-llvm -o - | FileCheck %s

using info = decltype(^^int);
template <info R> void mangled() {}

namespace { int anon_var; struct AnonType {}; }
static int static_var;
namespace ns { int shared_var; }

void *a = (void *)&mangled<^^anon_var>;
void *b = (void *)&mangled<^^AnonType>;
void *c = (void *)&mangled<^^static_var>;
void *d = (void *)&mangled<^^ns::shared_var>;

// CHECK-DAG: define internal {{.*}}anon_var
// CHECK-DAG: define internal {{.*}}AnonType
// CHECK-DAG: define internal {{.*}}static_var
// CHECK-DAG: define linkonce_odr {{.*}}shared_var

template <class X> concept GlobalConcept = true;
namespace { template <class X> concept AnonConcept = true; }
void *e = (void *)&mangled<^^GlobalConcept>;
void *f = (void *)&mangled<^^AnonConcept>;
// CHECK-DAG: define linkonce_odr {{.*}}GlobalConcept
// CHECK-DAG: define internal {{.*}}AnonConcept
