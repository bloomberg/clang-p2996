// RUN: %clang_cc1 %s -std=c++26 -freflection -fentity-proxy-reflection -fattribute-reflection \
// RUN:     -triple x86_64-pc-windows-msvc -emit-llvm -o - \
// RUN:     | FileCheck %s --check-prefix=MSVC -DMS='@"??$mangled@$M' -DMS_END='E@@YAXXZ"'
// RUN: %clang_cc1 %s -std=c++26 -freflection -fentity-proxy-reflection -fattribute-reflection \
// RUN:     -triple x86_64-pc-linux-gnu -emit-llvm -o - \
// RUN:     | FileCheck %s --check-prefix=ITANIUM -DIT='@_Z7mangledILDm' -DIT_END='EEvv'

using info = decltype(^^int);

template <info R> void mangled() {}

// Null
namespace null {
auto *use = &mangled<info{}>;
// MSVC-DAG: define {{.*}} [[MS]]0[[MS_END]]
// ITANIUM-DAG: define {{.*}} [[IT]]nu[[IT_END]]
} // namespace null

// Type
namespace types {
struct S {};
using A = int;
using A2 = int;

auto *use_int = &mangled<^^int>;
// MSVC-DAG: define {{.*}} [[MS]]t?AH[[MS_END]]
// ITANIUM-DAG: define {{.*}} [[IT]]tyi[[IT_END]]

auto *use_class = &mangled<^^S>;
// MSVC-DAG: define {{.*}} [[MS]]t?AUS@types@@[[MS_END]]
// ITANIUM-DAG: define {{.*}} [[IT]]tyN5types1SE[[IT_END]]

auto *use_ptr = &mangled<^^const int *>;
// MSVC-DAG: define {{.*}} [[MS]]t?APEBH[[MS_END]]
// ITANIUM-DAG: define {{.*}} [[IT]]tyPKi[[IT_END]]

auto *use_ref = &mangled<^^int &>;
// MSVC-DAG: define {{.*}} [[MS]]t?AAEAH[[MS_END]]
// ITANIUM-DAG: define {{.*}} [[IT]]tyRi[[IT_END]]

auto *use_fn = &mangled<^^void(int)>;
// MSVC-DAG: define {{.*}} [[MS]]t?6AXH@Z[[MS_END]]
// ITANIUM-DAG: define {{.*}} [[IT]]tyFviE[[IT_END]]

auto *use_alias = &mangled<^^A>;
// MSVC-DAG: define {{.*}} [[MS]]yAA@types@@[[MS_END]]
// ITANIUM-DAG: define {{.*}} [[IT]]ta5types1A_i[[IT_END]]

auto *use_alias2 = &mangled<^^A2>;
// MSVC-DAG: define {{.*}} [[MS]]yAA2@types@@[[MS_END]]
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
// MSVC-DAG: define {{.*}} [[MS]]d?x@decls@@3HA[[MS_END]]
// ITANIUM-DAG: define {{.*}} [[IT]]vrN5decls1xE[[IT_END]]

auto *use_fn = &mangled<^^f>;
// MSVC-DAG: define {{.*}} [[MS]]d?f@decls@@YAXH@Z[[MS_END]]
// ITANIUM-DAG: define {{.*}} [[IT]]fnN5decls1fEi[[IT_END]]

auto *use_field = &mangled<^^S::m>;
// MSVC-DAG: define {{.*}} [[MS]]dm@S@decls@@[[MS_END]]
// ITANIUM-DAG: define {{.*}} [[IT]]dm5decls1S1m[[IT_END]]

auto *use_static = &mangled<^^S::s>;
// MSVC-DAG: define {{.*}} [[MS]]d?s@S@decls@@2HA[[MS_END]]
// ITANIUM-DAG: define {{.*}} [[IT]]vrN5decls1S1sE[[IT_END]]

auto *use_mem_fn = &mangled<^^S::mf>;
// MSVC-DAG: define {{.*}} [[MS]]d?mf@S@decls@@QEAAXH@Z[[MS_END]]
// ITANIUM-DAG: define {{.*}} [[IT]]fnN5decls1S2mfEi[[IT_END]]

auto *use_op = &mangled<^^S::operator+>;
// MSVC-DAG: define {{.*}} [[MS]]d??HS@decls@@QEBA?AU12@U12@@Z[[MS_END]]
// ITANIUM-DAG: define {{.*}} [[IT]]fnNK5decls1SplES1_[[IT_END]]

auto *use_conv = &mangled<^^S::operator int>;
// MSVC-DAG: define {{.*}} [[MS]]d??BS@decls@@QEBAHXZ[[MS_END]]
// ITANIUM-DAG: define {{.*}} [[IT]]fnNK5decls1ScviEv[[IT_END]]

auto *use_e0 = &mangled<^^e0>;
// MSVC-DAG: define {{.*}} [[MS]]de0@decls@@[[MS_END]]
// ITANIUM-DAG: define {{.*}} [[IT]]en5decls1E2e0[[IT_END]]

auto *use_alias0 = &mangled<^^alias0>;
// MSVC-DAG: define {{.*}} [[MS]]dalias0@decls@@[[MS_END]]
// ITANIUM-DAG: define {{.*}} [[IT]]en5decls1E6alias0[[IT_END]]

auto *use_scoped = &mangled<^^EC::e>;
// MSVC-DAG: define {{.*}} [[MS]]de@EC@decls@@[[MS_END]]
// ITANIUM-DAG: define {{.*}} [[IT]]en5decls2EC1e[[IT_END]]

auto *use_binding = &mangled<^^q>;
// MSVC-DAG: define {{.*}} [[MS]]dq@decls@@[[MS_END]]
// ITANIUM-DAG: define {{.*}} [[IT]]sbN5decls1qE[[IT_END]]

auto *use_var_spec = &mangled<^^v<int>>;
// MSVC-DAG: define {{.*}} [[MS]]d??$v@H@decls@@3HA[[MS_END]]
// ITANIUM-DAG: define {{.*}} [[IT]]vrN5decls1vIiEE[[IT_END]]

auto *use_fn_spec = &mangled<^^g<int>>;
// MSVC-DAG: define {{.*}} [[MS]]d??$g@H@decls@@YAXH@Z[[MS_END]]
// ITANIUM-DAG: define {{.*}} [[IT]]fnN5decls1gIiEEvT_[[IT_END]]

auto *use_anon_union = &mangled<^^U::i>;
// MSVC-DAG: define {{.*}} [[MS]]di@U@decls@@[[MS_END]]
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

// MSVC-DAG: define {{.*}} [[MS]]d?ls@?1??stat@locals@@YAPEAXXZ@4HA[[MS_END]]
// MSVC-DAG: define {{.*}} [[MS]]d?lv@?1??local@locals@@YAPEAXXZ@3HA[[MS_END]]
// MSVC-DAG: define {{.*}} [[MS]]d?pv@?1??param@locals@@YAPEAXH@Z@3HA[[MS_END]]
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
// MSVC-DAG: define {{.*}} [[MS]]TT@templates@@[[MS_END]]
// ITANIUM-DAG: define {{.*}} [[IT]]ct9templates1T[[IT_END]]

auto *use_alias = &mangled<^^Al>;
// MSVC-DAG: define {{.*}} [[MS]]TAl@templates@@[[MS_END]]
// ITANIUM-DAG: define {{.*}} [[IT]]at9templates2Al[[IT_END]]

auto *use_var = &mangled<^^vt>;
// MSVC-DAG: define {{.*}} [[MS]]Tvt@templates@@[[MS_END]]
// ITANIUM-DAG: define {{.*}} [[IT]]vt9templates2vt[[IT_END]]

auto *use_fn = &mangled<^^ft>;
// MSVC-DAG: define {{.*}} [[MS]]Tft@templates@@[[MS_END]]
// ITANIUM-DAG: define {{.*}} [[IT]]ft9templates2ft[[IT_END]]

auto *use_concept = &mangled<^^Co>;
// MSVC-DAG: define {{.*}} [[MS]]TCo@templates@@[[MS_END]]
// ITANIUM-DAG: define {{.*}} [[IT]]co9templates2Co[[IT_END]]
} // namespace templates

// Namespace
namespace namespaces {
namespace inner {}
namespace al = inner;
inline namespace inl {}

auto *use_ns = &mangled<^^inner>;
// MSVC-DAG: define {{.*}} [[MS]]ninner@namespaces@@[[MS_END]]
// ITANIUM-DAG: define {{.*}} [[IT]]ns10namespaces5inner[[IT_END]]

auto *use_global = &mangled<^^::>;
// MSVC-DAG: define {{.*}} [[MS]]n[[MS_END]]
// ITANIUM-DAG: define {{.*}} [[IT]]gs[[IT_END]]

auto *use_alias = &mangled<^^al>;
// MSVC-DAG: define {{.*}} [[MS]]nal@namespaces@@[[MS_END]]
// ITANIUM-DAG: define {{.*}} [[IT]]na10namespaces2al[[IT_END]]

auto *use_inline = &mangled<^^inl>;
// MSVC-DAG: define {{.*}} [[MS]]ninl@namespaces@@[[MS_END]]
// ITANIUM-DAG: define {{.*}} [[IT]]ns10namespaces3inl[[IT_END]]
} // namespace namespaces

// EntityProxy (needs -fentity-proxy-reflection)
namespace proxies {
namespace m { int x; }
using m::x;
struct Base { void f(); };
struct Derived : Base { using Base::f; };

auto *use_ns_using = &mangled<^^x>;
// MSVC-DAG: define {{.*}} [[MS]]ux@proxies@@[[MS_END]]
// ITANIUM-DAG: define {{.*}} [[IT]]ep7proxies1x[[IT_END]]

auto *use_member_using = &mangled<^^Derived::f>;
// MSVC-DAG: define {{.*}} [[MS]]uf@Derived@proxies@@[[MS_END]]
// ITANIUM-DAG: define {{.*}} [[IT]]ep7proxies7Derived1f[[IT_END]]
} // namespace proxies

// Attribute (needs -fattribute-reflection)
namespace attributes {
auto *use_plain = &mangled<^^[[nodiscard]]>;
// MSVC-DAG: define {{.*}} [[MS]]anodiscard@[[MS_END]]
// ITANIUM-DAG: define {{.*}} [[IT]]ar9nodiscard[[IT_END]]

auto *use_scoped = &mangled<^^[[gnu::cold]]>;
// MSVC-DAG: define {{.*}} [[MS]]aSgnu@cold@[[MS_END]]
// ITANIUM-DAG: define {{.*}} [[IT]]arS3gnu4cold[[IT_END]]
} // namespace attributes

// Not tested here: these kinds can only be produced through <meta>
// (reflect_object, reflect_constant, parameters_of, bases_of,
// data_member_spec, annotations_of, enumerator_spec), which clang tests
// cannot include. They are reachable via __metafunction(id, ...) with
// hard-coded ids, but those ids are unstable, so they are left untested.
//   ReflectionKind::Object, Value, Parameter, BaseSpecifier, DataMemberSpec,
//   Annotation, EnumeratorSpec
// Likewise constructors, destructors and unnamed fields (Declaration), which
// are only reachable through members_of.
