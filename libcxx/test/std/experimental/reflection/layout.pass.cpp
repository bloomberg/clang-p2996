//===----------------------------------------------------------------------===//
//
// Copyright 2024 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20
// ADDITIONAL_COMPILE_FLAGS: -freflection

// <experimental/reflection>
//
// [reflection]

#include <experimental/meta>


using int_alias = int;

long x;
static_assert(size_of(std::meta::reflect_object(x)) == sizeof(long));
static_assert(size_of(std::meta::reflect_value(38)) == sizeof(int));
static_assert(size_of(^int) == sizeof(int));
static_assert(size_of(^int&) == sizeof(int *));
static_assert(size_of(^int_alias) == sizeof(int));
static_assert(bit_size_of(^int) == 8 * sizeof(int));
static_assert(bit_size_of(^int &) == 8 * sizeof(int *));
static_assert(bit_size_of(^int_alias) == 8 * sizeof(int));
static_assert(alignment_of(^int) == alignof(int));
static_assert(alignment_of(^int &) == alignof(int *));
static_assert(alignment_of(^int_alias) == alignof(int));


struct S1 { char mem; };
static_assert(offset_of(^S1::mem) == 0);
static_assert(size_of(^S1::mem) == 1);
static_assert(bit_offset_of(^S1::mem) == 0);
static_assert(bit_size_of(^S1::mem) == 8);
static_assert(size_of(^S1) == 1);
static_assert(bit_offset_of(^S1::mem) == 0);
static_assert(bit_size_of(^S1::mem) == 8);


struct BitField {
    char bf1 : 1;
    char bf2 : 2;
    char : 0;
    char bf3 : 3;
    int bf4 : 3;
};
static_assert(offset_of(^BitField::bf1) == 0);
static_assert(offset_of(^BitField::bf2) == 0);
static_assert(offset_of(nonstatic_data_members_of(^BitField)[2]) == 1);
static_assert(offset_of(^BitField::bf3) == 1);
static_assert(offset_of(^BitField::bf4) == 1);
static_assert(bit_offset_of(^BitField::bf1) == 0);
static_assert(bit_offset_of(^BitField::bf2) == 1);
static_assert(bit_offset_of(nonstatic_data_members_of(^BitField)[2]) == 0);
static_assert(bit_offset_of(^BitField::bf3) == 0);
static_assert(bit_offset_of(^BitField::bf4) == 3);
static_assert(bit_size_of(^BitField::bf1) == 1);
static_assert(bit_size_of(^BitField::bf2) == 2);
static_assert(bit_size_of(nonstatic_data_members_of(^BitField)[2]) == 0);
static_assert(bit_size_of(^BitField::bf3) == 3);
static_assert(bit_size_of(^BitField::bf4) == 3);
static_assert(size_of(^BitField) == 4);


struct Align {
    alignas(1) char a1;
    alignas(2) char a2;
    alignas(4) char a4;
    alignas(8) char a8;
};
static_assert(alignment_of(^Align::a1) == 1);
static_assert(alignment_of(^Align::a2) == 2);
static_assert(alignment_of(^Align::a4) == 4);
static_assert(alignment_of(^Align::a8) == 8);
static_assert(alignment_of(^Align) == 8);
static_assert(offset_of(^Align::a1) == 0);
static_assert(offset_of(^Align::a2) == 2);
static_assert(offset_of(^Align::a4) == 4);
static_assert(offset_of(^Align::a8) == 8);
static_assert(size_of(^Align::a1) == sizeof(char));
static_assert(size_of(^Align::a2) == sizeof(char));
static_assert(size_of(^Align::a4) == sizeof(char));
static_assert(size_of(^Align::a8) == sizeof(char));
static_assert(size_of(^Align) == 16);

struct alignas(64) AlignedTo64 { int mem; };
static_assert(alignment_of(^AlignedTo64) == 64);
static_assert(size_of(^AlignedTo64) == 64);
static_assert(size_of(^AlignedTo64::mem) == sizeof(int));


int main() { }
