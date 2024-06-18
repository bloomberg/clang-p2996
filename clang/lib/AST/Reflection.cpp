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

#include "clang/AST/APValue.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclTemplate.h"
#include "clang/AST/Expr.h"
#include "clang/AST/LocInfoType.h"
#include "clang/AST/Reflection.h"

namespace clang {

ReflectionValue::ReflectionValue() : Kind(RK_null), Entity(nullptr) { }

ReflectionValue::ReflectionValue(ReflectionKind Kind, void *Entity)
    : Kind(Kind), Entity(Entity) {
}

ReflectionValue::ReflectionValue(ReflectionValue const& Rhs)
    : Kind(Rhs.Kind), Entity(Rhs.Entity) {
}

ReflectionValue &ReflectionValue::operator=(ReflectionValue const&Rhs) {
  Kind = Rhs.Kind;
  Entity = Rhs.Entity;

  return *this;
}

bool ReflectionValue::isNull() const { return Kind == RK_null; }

QualType ReflectionValue::getAsType() const {
  assert(getKind() == RK_type && "not a type");

  QualType QT = QualType::getFromOpaquePtr(Entity);

  bool UnwrapAliases = false;

  void *AsPtr;
  do {
    AsPtr = QT.getAsOpaquePtr();

    if (const auto *LIT = dyn_cast<LocInfoType>(QT))
      QT = LIT->getType();
    if (const auto *ET = dyn_cast<ElaboratedType>(QT)) {
      QualType New = ET->getNamedType();
      New.setLocalFastQualifiers(QT.getLocalFastQualifiers());
      QT = New;
    }
    if (const auto *STTPT = dyn_cast<SubstTemplateTypeParmType>(QT);
        STTPT && !STTPT->isDependentType())
      QT = STTPT->getReplacementType();
    if (const auto *RST = dyn_cast<ReflectionSpliceType>(QT);
        RST && !RST->isDependentType())
      QT = RST->getUnderlyingType();
    if (const auto *DTT = dyn_cast<DecltypeType>(QT)) {
      QT = DTT->desugar();
      UnwrapAliases = true;
    }
    if (const auto *UT = dyn_cast<UsingType>(QT);
        UT && UnwrapAliases)
      QT = UT->desugar();
    if (const auto *TDT = dyn_cast<TypedefType>(QT);
        TDT && UnwrapAliases)
      QT = TDT->desugar();
  } while (QT.getAsOpaquePtr() != AsPtr);
  return QT;
}

void ReflectionValue::Profile(llvm::FoldingSetNodeID &ID) const {
  ID.AddInteger(Kind);
  switch (Kind) {
  case RK_null:
    break;
  case RK_type: {
    QualType QT = getAsType();
    QT.Profile(ID);
    break;
  }
  case RK_expr_result:
    if (auto *RD = getAsExprResult()->getType()->getAsRecordDecl();
        RD && RD->isLambda()) {
      QualType(RD->getTypeForDecl(), 0).Profile(ID);
    }
    getAsExprResult()->getAPValueResult().Profile(ID);
    break;
  case RK_declaration:
    ID.AddPointer(getAsDecl());
    break;
  case RK_template:
    getAsTemplate().Profile(ID);
    break;
  case RK_namespace:
    ID.AddPointer(getAsNamespace());
    break;
  case RK_base_specifier:
    ID.AddPointer(getAsBaseSpecifier());
    break;
  case RK_data_member_spec: {
    TagDataMemberSpec *TDMS = getAsDataMemberSpec();
    TDMS->Ty.Profile(ID);
    ID.AddBoolean(TDMS->Name.has_value());
    if (TDMS->Name)
      ID.AddString(TDMS->Name.value());
    ID.AddBoolean(TDMS->IsStatic);
    ID.AddBoolean(TDMS->Alignment.has_value());
    if (TDMS->Alignment)
      ID.AddBoolean(TDMS->Alignment.value());
    ID.AddBoolean(TDMS->BitWidth.has_value());
    if (TDMS->BitWidth)
      ID.AddBoolean(TDMS->BitWidth.value());
    break;
  }
  }
}

bool ReflectionValue::operator==(ReflectionValue const& Rhs) const {
  if (getKind() != Rhs.getKind())
    return false;

  switch (getKind()) {
  case RK_null:
    return true;
  case RK_type: {
    QualType LQT = getAsType(), RQT = Rhs.getAsType();
    if (LQT.getQualifiers() != RQT.getQualifiers())
      return false;

    if (auto *LTST = dyn_cast<TemplateSpecializationType>(LQT)) {
      if (LTST->isTypeAlias()) {
        auto *RTST = dyn_cast<TemplateSpecializationType>(RQT);
        if (!RTST || !RTST->isTypeAlias() ||
            LTST->getTemplateName().getAsTemplateDecl() !=
                  RTST->getTemplateName().getAsTemplateDecl())
          return false;
      }
      return declaresSameEntity(LQT->getAsRecordDecl(), RQT->getAsRecordDecl());
    }

    if (LQT->isTypedefNameType() || RQT->isTypedefNameType())
      return LQT == RQT;

    return LQT.getCanonicalType() == RQT.getCanonicalType();
  }
  case RK_expr_result: {
    APValue LV = getAsExprResult()->getAPValueResult();
    APValue RV = Rhs.getAsExprResult()->getAPValueResult();

    llvm::FoldingSetNodeID LID, RID;
    getAsExprResult()->getType()
        .getCanonicalType().getUnqualifiedType().Profile(LID);
    LV.Profile(LID);
    Rhs.getAsExprResult()->getType()
        .getCanonicalType().getUnqualifiedType().Profile(RID);
    RV.Profile(RID);

    return LID == RID;
  }
  case RK_declaration:
    return declaresSameEntity(getAsDecl(), Rhs.getAsDecl());
  case RK_template:
    return declaresSameEntity(getAsTemplate().getAsTemplateDecl(),
                              Rhs.getAsTemplate().getAsTemplateDecl());
  case RK_namespace:
    return declaresSameEntity(getAsNamespace(), Rhs.getAsNamespace());
  case RK_base_specifier:
    return getAsBaseSpecifier() == Rhs.getAsBaseSpecifier();
  case RK_data_member_spec:
    return getAsDataMemberSpec() == Rhs.getAsDataMemberSpec();
  }
  llvm_unreachable("unknown reflection kind");
}

bool ReflectionValue::operator!=(ReflectionValue const& Rhs) const {
  return !(*this == Rhs);
}

bool TagDataMemberSpec::operator==(TagDataMemberSpec const &Rhs) const {
  return (Ty == Ty &&
          IsStatic == Rhs.IsStatic &&
          Alignment == Rhs.Alignment &&
          BitWidth == Rhs.BitWidth &&
          Name == Rhs.Name);
}

bool TagDataMemberSpec::operator!=(TagDataMemberSpec const &Rhs) const {
  return !(*this == Rhs);
}

}  // end namespace clang
