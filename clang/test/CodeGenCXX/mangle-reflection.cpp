// RUN: %clang_cc1 %s -std=c++26 -freflection -fentity-proxy-reflection -fattribute-reflection \
// RUN:     -triple x86_64-pc-linux-gnu -emit-llvm -o - \
// RUN:     | FileCheck %s --check-prefix=ITANIUM -DIT='@_Z7mangledILDm' -DIT_END='EEvv'

using info = decltype(^^int);

template <info R> void mangled() {}

// Null
namespace null {
auto *use = &mangled<info{}>;
// ITANIUM-DAG: define {{.*}} [[IT]]nu[[IT_END]]
} // namespace null

// Type
namespace types {
struct S {};
using A = int;
using A2 = int;

auto *use_int = &mangled<^^int>;
// ITANIUM-DAG: define {{.*}} [[IT]]tyi[[IT_END]]

auto *use_class = &mangled<^^S>;
// ITANIUM-DAG: define {{.*}} [[IT]]tyN5types1SE[[IT_END]]

auto *use_ptr = &mangled<^^const int *>;
// ITANIUM-DAG: define {{.*}} [[IT]]tyPKi[[IT_END]]

auto *use_ref = &mangled<^^int &>;
// ITANIUM-DAG: define {{.*}} [[IT]]tyRi[[IT_END]]

auto *use_fn = &mangled<^^void(int)>;
// ITANIUM-DAG: define {{.*}} [[IT]]tyFviE[[IT_END]]

auto *use_alias = &mangled<^^A>;
// ITANIUM-DAG: define {{.*}} [[IT]]ta5types1A_i[[IT_END]]

auto *use_alias2 = &mangled<^^A2>;
// ITANIUM-DAG: define {{.*}} [[IT]]ta5types2A2_i[[IT_END]]
} // namespace types

// Declaration
namespace decls {
int x;
void f(int);
struct S {
  int m;
  static int s;
  void mf(int);
  S operator+(S) const;
  operator int() const;
};
enum E { e0 = 0, alias0 = 0 };
enum class EC { e };
struct P { int a, b; };
auto [q, r] = P{1, 2};
template <class T> int v = 0;
template <class T> void g(T);
struct U { union { int i; }; };

auto *use_var = &mangled<^^x>;
// ITANIUM-DAG: define {{.*}} [[IT]]vrN5decls1xE[[IT_END]]

auto *use_fn = &mangled<^^f>;
// ITANIUM-DAG: define {{.*}} [[IT]]fnN5decls1fEi[[IT_END]]

auto *use_field = &mangled<^^S::m>;
// ITANIUM-DAG: define {{.*}} [[IT]]dm5decls1S1m[[IT_END]]

auto *use_static = &mangled<^^S::s>;
// ITANIUM-DAG: define {{.*}} [[IT]]vrN5decls1S1sE[[IT_END]]

auto *use_mem_fn = &mangled<^^S::mf>;
// ITANIUM-DAG: define {{.*}} [[IT]]fnN5decls1S2mfEi[[IT_END]]

auto *use_op = &mangled<^^S::operator+>;
// ITANIUM-DAG: define {{.*}} [[IT]]fnNK5decls1SplES1_[[IT_END]]

auto *use_conv = &mangled<^^S::operator int>;
// ITANIUM-DAG: define {{.*}} [[IT]]fnNK5decls1ScviEv[[IT_END]]

auto *use_e0 = &mangled<^^e0>;
// ITANIUM-DAG: define {{.*}} [[IT]]en5decls1E2e0[[IT_END]]

auto *use_alias0 = &mangled<^^alias0>;
// ITANIUM-DAG: define {{.*}} [[IT]]en5decls1E6alias0[[IT_END]]

auto *use_scoped = &mangled<^^EC::e>;
// ITANIUM-DAG: define {{.*}} [[IT]]en5decls2EC1e[[IT_END]]

auto *use_binding = &mangled<^^q>;
// ITANIUM-DAG: define {{.*}} [[IT]]sbN5decls1qE[[IT_END]]

auto *use_var_spec = &mangled<^^v<int>>;
// ITANIUM-DAG: define {{.*}} [[IT]]vrN5decls1vIiEE[[IT_END]]

auto *use_fn_spec = &mangled<^^g<int>>;
// ITANIUM-DAG: define {{.*}} [[IT]]fnN5decls1gIiEEvT_[[IT_END]]

auto *use_anon_union = &mangled<^^U::i>;
// ITANIUM-DAG: define {{.*}} [[IT]]dm5decls1U1i[[IT_END]]
} // namespace decls

// Declaration (function-local)
namespace locals {
// Function-local entities can only be named from inside their function.
inline void *stat() {
  static int ls;
  return reinterpret_cast<void *>(&mangled<^^ls>);
}
inline void *local() {
  int lv = 0;
  (void)lv;
  return reinterpret_cast<void *>(&mangled<^^lv>);
}
inline void *param(int pv) {
  (void)pv;
  return reinterpret_cast<void *>(&mangled<^^pv>);
}
void *use_stat = stat(), *use_local = local(), *use_param = param(0);

// ITANIUM-DAG: define {{.*}} [[IT]]vrZN6locals4statEvE2ls[[IT_END]]
// ITANIUM-DAG: define {{.*}} [[IT]]vrZN6locals5localEvE2lv[[IT_END]]
// ITANIUM-DAG: define {{.*}} [[IT]]vrZN6locals5paramEiE2pv[[IT_END]]
} // namespace locals

// Template
namespace templates {
template <class> struct T {};
template <class X> using Al = X;
template <class X> int vt = 0;
template <class X> void ft(X);
template <class X> concept Co = true;

auto *use_class = &mangled<^^T>;
// ITANIUM-DAG: define {{.*}} [[IT]]ct9templates1T[[IT_END]]

auto *use_alias = &mangled<^^Al>;
// ITANIUM-DAG: define {{.*}} [[IT]]at9templates2Al[[IT_END]]

auto *use_var = &mangled<^^vt>;
// ITANIUM-DAG: define {{.*}} [[IT]]vt9templates2vt[[IT_END]]

auto *use_fn = &mangled<^^ft>;
// ITANIUM-DAG: define {{.*}} [[IT]]ft9templates2ft[[IT_END]]

auto *use_concept = &mangled<^^Co>;
// ITANIUM-DAG: define {{.*}} [[IT]]co9templates2Co[[IT_END]]
} // namespace templates

// Namespace
namespace namespaces {
namespace inner {}
namespace al = inner;
inline namespace inl {}

auto *use_ns = &mangled<^^inner>;
// ITANIUM-DAG: define {{.*}} [[IT]]ns10namespaces5inner[[IT_END]]

auto *use_global = &mangled<^^::>;
// ITANIUM-DAG: define {{.*}} [[IT]]gs[[IT_END]]

auto *use_alias = &mangled<^^al>;
// ITANIUM-DAG: define {{.*}} [[IT]]na10namespaces2al[[IT_END]]

auto *use_inline = &mangled<^^inl>;
// ITANIUM-DAG: define {{.*}} [[IT]]ns10namespaces3inl[[IT_END]]
} // namespace namespaces

// EntityProxy (needs -fentity-proxy-reflection)
namespace proxies {
namespace m { int x; }
using m::x;
struct Base { void f(); };
struct Derived : Base { using Base::f; };

auto *use_ns_using = &mangled<^^x>;
// ITANIUM-DAG: define {{.*}} [[IT]]ep7proxies1x[[IT_END]]

auto *use_member_using = &mangled<^^Derived::f>;
// ITANIUM-DAG: define {{.*}} [[IT]]ep7proxies7Derived1f[[IT_END]]
} // namespace proxies

// Attribute (needs -fattribute-reflection)
namespace attributes {
auto *use_plain = &mangled<^^[[nodiscard]]>;
// ITANIUM-DAG: define {{.*}} [[IT]]ar9nodiscard[[IT_END]]

auto *use_scoped = &mangled<^^[[gnu::cold]]>;
// ITANIUM-DAG: define {{.*}} [[IT]]arS3gnu4cold[[IT_END]]
} // namespace attributes
