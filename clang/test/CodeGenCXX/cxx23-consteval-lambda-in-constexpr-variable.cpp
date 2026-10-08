// RUN: %clang_cc1 -std=c++23 -triple x86_64-linux-gnu -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -std=c++26 -triple x86_64-linux-gnu -freflection -emit-llvm -o - %s | FileCheck %s

// The initializer of a constexpr or constinit variable is an immediate
// function context, but the body of a lambda appearing in it is not: its
// immediate invocations have to be evaluated, not emitted as calls.

consteval int only_ct(int x) { return x; }
struct S { consteval S(int) {} };

// CHECK-NOT: call {{.*}}@_Z7only_cti
// CHECK-NOT: call {{.*}}@_ZN1SC

int local() {
  constexpr auto l = [](int id) { S s(1); return only_ct(2) + id; };
  return l(4);
}

int local_static() {
  static constexpr auto l = [](int id) { return only_ct(2) + id; };
  return l(4);
}

constexpr auto global = [](int id) { return only_ct(2) + id; };
int from_global() { return global(4); }

constinit auto init = [](int id) { return only_ct(2) + id; };
int from_constinit() { return init(4); }

template <typename T>
int instantiated() {
  constexpr auto l = [](T id) { return only_ct(2) + id; };
  return l(4);
}
int from_template() { return instantiated<int>(); }

constexpr auto nested = [] {
  return [](int id) { return only_ct(2) + id; };
};
int from_nested() { return nested()(4); }

// A lambda in a default argument of an immediate function still is in an
// immediate function context, as are its callers.
consteval int with_default(int (*f)() = [] { return only_ct(2); }) { return f(); }
int from_default() { return with_default(); }
