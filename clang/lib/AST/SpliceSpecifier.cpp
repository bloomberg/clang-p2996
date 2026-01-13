//===--- SpliceSpecifier.cpp - Class for splice specifiers ------*- C++ -*-===//
//
// Copyright 2025 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//  This file implements the SpliceSpecifier class.
//
//===----------------------------------------------------------------------===//

#include "clang/AST/SpliceSpecifier.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Expr.h"
#include "clang/AST/TemplateBase.h"

namespace clang {

void *SpliceSpecifier::operator new(size_t bytes, const ASTContext &C,
                                    unsigned alignment) {
  return ::operator new(bytes, C, alignment);
}

SpliceSpecifier::SpliceSpecifier(
    SourceLocation LSplice, Expr *Operand, SourceLocation RSplice,
    const ASTTemplateArgumentListInfo *TemplateArgs)
: LSpliceLoc(LSplice), Operand(Operand), RSpliceLoc(RSplice),
  TemplateArgs(TemplateArgs) {
}

SpliceSpecifier *SpliceSpecifier::Create(
    ASTContext &C, SourceLocation LSplice, Expr *Operand,
    SourceLocation RSplice, const ASTTemplateArgumentListInfo *TemplateArgs) {
  // todo [merge:yukino:maybe-revert,"8-byte alignment with StoredKind"]
  return new (C) SpliceSpecifier(LSplice, Operand, RSplice, TemplateArgs);
}

SpliceSpecifierDependence SpliceSpecifier::getDependence() const {
  auto Result = toSpliceSpecifierDependence(Operand->getDependence());
  if (TemplateArgs)
    for (const auto &TArg : TemplateArgs->arguments())
      Result |=
        toSpliceSpecifierDependence(TArg.getArgument().getDependence());

  return Result;
}

SourceLocation SpliceSpecifier::getLAngleLoc() const {
  assert(isSpecialization());
  return TemplateArgs->getLAngleLoc();
}

SourceLocation SpliceSpecifier::getRAngleLoc() const {
  assert(isSpecialization());
  return TemplateArgs->getRAngleLoc();
}

}  // end namespace clang
