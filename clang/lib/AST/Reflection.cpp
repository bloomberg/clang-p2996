//===--- Reflection.cpp - Classes for representing reflection ---*- C++ -*-===//
//
// Copyright 2024 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//  This file implements the ReflectionValue class.
//
//===----------------------------------------------------------------------===//

#include "clang/AST/Reflection.h"

namespace clang {

bool TagDataMemberSpec::operator==(TagDataMemberSpec const &Rhs) const {
  return (Ty == Ty &&
          Alignment == Rhs.Alignment &&
          BitWidth == Rhs.BitWidth &&
          Name == Rhs.Name);
}

bool TagDataMemberSpec::operator!=(TagDataMemberSpec const &Rhs) const {
  return !(*this == Rhs);
}

}  // end namespace clang
