// RUN: %clang_cc1 -std=c++26 -fexpansion-statements -x c++ %s -verify

struct S {
  int x;
  constexpr S(int x) : x{x} {}
};

void g(int); // #g
template <int n> constexpr int tg() { return n; }

void f1() {
  template for (auto x : {}) static_assert(false, "discarded");
  template for (constexpr auto x : {}) static_assert(false, "discarded");
  template for (auto x : {1}) g(x);
  template for (auto x : {1, 2, 3}) g(x);
  template for (constexpr auto x : {1}) g(x);
  template for (constexpr auto x : {1, 2, 3}) g(x);
  template for (constexpr auto x : {1}) tg<x>();
  template for (constexpr auto x : {1, 2, 3})
    static_assert(tg<x>());

  template for (int x : {1, 2, 3}) g(x);
  template for (S x : {1, 2, 3}) g(x.x);
  template for (constexpr S x : {1, 2, 3}) tg<x.x>();

  template for (int x : {"1", S(1), {1, 2}}) { // expected-error {{cannot initialize a variable of type 'int' with an lvalue of type 'const char[2]'}} \
                                                  expected-error {{no viable conversion from 'S' to 'int'}} \
                                                  expected-error {{excess elements in scalar initializer}} \
                                                  expected-note 3 {{in instantiation of expansion statement requested here}}
    g(x);
  }

  template for (constexpr auto x : {1, 2, 3, 4}) { // expected-note 3 {{in instantiation of expansion statement requested here}}
    static_assert(tg<x>() == 4); // expected-error 3 {{static assertion failed due to requirement 'tg<x>() == 4'}} \
                                    expected-note {{expression evaluates to '1 == 4'}} \
                                    expected-note {{expression evaluates to '2 == 4'}} \
                                    expected-note {{expression evaluates to '3 == 4'}}
  }


  template for (constexpr auto x : {1, 2}) { // expected-note 2 {{in instantiation of expansion statement requested here}}
    static_assert(false, "not discarded"); // expected-error 2 {{static assertion failed: not discarded}}
  }
}

struct T {
  int i;
  short s;
};

consteval long f2(T s) {
  long result = 0;
  template for (auto x : s) {
    result += sizeof(x);
  }
  return result;
}
static_assert(f2(T{}) == sizeof(int) + sizeof(short));
