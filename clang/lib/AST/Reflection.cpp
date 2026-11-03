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
#include "clang/AST/APValue.h"
#include "llvm/ADT/FoldingSet.h"

namespace clang {

bool TagDataMemberSpec::operator==(TagDataMemberSpec const &Rhs) const {
  if (Ty != Rhs.Ty || Alignment != Rhs.Alignment || BitWidth != Rhs.BitWidth ||
      Name != Rhs.Name || NoUniqueAddress != Rhs.NoUniqueAddress ||
      Annotations.size() != Rhs.Annotations.size())
    return false;

  for (size_t I = 0; I < Annotations.size(); ++I) {
    llvm::FoldingSetNodeID LHSID, RHSID;
    Annotations[I].Profile(LHSID);
    Rhs.Annotations[I].Profile(RHSID);
    if (LHSID != RHSID)
      return false;
  }
  return true;
}

bool TagDataMemberSpec::operator!=(TagDataMemberSpec const &Rhs) const {
  return !(*this == Rhs);
}

}  // end namespace clang
