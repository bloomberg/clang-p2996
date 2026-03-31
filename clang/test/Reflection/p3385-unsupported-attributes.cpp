// RUN: %clang_cc1 -std=c++26 -freflection -fattribute-reflection -verify %s

// Test that reflecting over unsupported attributes is ill-formed.

void test() {
  auto a = ^^[[my::stuff("anything")]]; // expected-error {{reflecting over the unsupported 'stuff' attribute is ill-formed}}
  auto b = ^^[[clang::availability(macos,introduced=10.4,deprecated=10.6,obsoleted=10.7)]]; // expected-error {{reflecting over the unsupported 'availability' attribute is ill-formed}}
}
