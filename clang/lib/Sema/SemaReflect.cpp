//===--- SemaReflect.cpp - Semantic Analysis for Reflection ---------------===//
//
// Copyright 2024 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//  This file implements semantic analysis for reflection.
//
//===----------------------------------------------------------------------===//

#include "TypeLocBuilder.h"
#include "clang/AST/DeclBase.h"
#include "clang/Basic/DiagnosticSema.h"
#include "clang/Sema/EnterExpressionEvaluationContext.h"
#include "clang/Sema/Lookup.h"
#include "clang/Sema/Metafunction.h"
#include "clang/Sema/ParsedTemplate.h"
#include "clang/Sema/Sema.h"

using namespace clang;
using namespace sema;

namespace {

TemplateArgumentListInfo addLocToTemplateArgs(Sema &S,
                                              ArrayRef<TemplateArgument> Args,
                                              SourceLocation ExprLoc) {
  auto convert = [&](const TemplateArgument &TA) -> TemplateArgumentLoc {
    return S.getTrivialTemplateArgumentLoc(TA,
                                           TA.getNonTypeTemplateArgumentType(),
                                           ExprLoc);
  };

  TemplateArgumentListInfo Result;
  for (const TemplateArgument &Arg : Args)
    if (Arg.getKind() == TemplateArgument::Pack)
      for (const TemplateArgument &TA : Arg.getPackAsArray())
        Result.addArgument(convert(TA));
    else
      Result.addArgument(convert(Arg));

  return Result;
}

Expr *CreateRefToDecl(Sema &S, ValueDecl *D,
                      SourceLocation ExprLoc) {
  NestedNameSpecifierLocBuilder NNSLocBuilder;
  if (const auto *RDC = dyn_cast<RecordDecl>(D->getDeclContext())) {
    QualType QT(RDC->getTypeForDecl(), 0);
    TypeSourceInfo *TSI = S.Context.CreateTypeSourceInfo(QT, 0);
    NNSLocBuilder.Extend(S.Context, SourceLocation(), TSI->getTypeLoc(),
                         ExprLoc);
  }

  ExprValueKind ValueKind = VK_LValue;
  if (auto *VTSD = dyn_cast<VarTemplateSpecializationDecl>(D);
      VTSD && VTSD->getTemplateSpecializationKind() == TSK_Undeclared) {
    const TemplateArgumentList &TAList = VTSD->getTemplateArgs();
    TemplateArgumentListInfo TAListInfo(
            addLocToTemplateArgs(S, TAList.asArray(), ExprLoc));

    CXXScopeSpec SS;
    DeclarationNameInfo DNI(VTSD->getDeclName(), ExprLoc);
    ExprResult ER = S.CheckVarTemplateId(SS, DNI,
                                         VTSD->getSpecializedTemplate(),
                                         VTSD->getSpecializedTemplate(),
                                         ExprLoc, &TAListInfo);
    return ER.get();
  } else {
    QualType QT(D->getType());
    if (isa<EnumConstantDecl>(D))
      ValueKind = VK_PRValue;
    else if (auto *MD = dyn_cast<CXXMethodDecl>(D); MD && !MD->isStatic())
      ValueKind = VK_PRValue;
    else if (auto *RT = dyn_cast<ReferenceType>(QT)) {
      QT = RT->getPointeeType();
      ValueKind = VK_LValue;
    }

    return DeclRefExpr::Create(
        S.Context, NNSLocBuilder.getWithLocInContext(S.Context),
        SourceLocation(), D, false, ExprLoc, QT, ValueKind, D, nullptr);
  }
}
}  // anonymous namespace

ExprResult Sema::ActOnCXXReflectExpr(SourceLocation OpLoc,
                                     SourceLocation TemplateKWLoc,
                                     CXXScopeSpec &SS, UnqualifiedId &Id) {
  TemplateArgumentListInfo TArgBuffer;

  DeclarationNameInfo NameInfo;
  const TemplateArgumentListInfo *TArgs;
  DecomposeUnqualifiedId(Id, TArgBuffer, NameInfo, TArgs);

  LookupResult Found(*this, NameInfo, LookupReflectOperandName);

  if (Id.getKind() == UnqualifiedIdKind::IK_TemplateId &&
      Id.TemplateId->Template.get().getKind() == TemplateName::Template) {
    Found.addDecl(Id.TemplateId->Template.get().getAsTemplateDecl());
  } else if (Id.getKind() == UnqualifiedIdKind::IK_TemplateId &&
             Id.TemplateId->Template.get().getKind() ==
                    TemplateName::DependentTemplate &&
             Id.TemplateId->Template.get().getAsDependentTemplateName()
                                          ->isSpliceSpecifier()) {
    auto *Splice = const_cast<CXXSpliceSpecifierExpr *>(
        Id.TemplateId->Template.get().getAsDependentTemplateName()
                                     ->getSpliceSpecifier());
    ExprResult Result = BuildReflectionSpliceExpr(
            TemplateKWLoc, Splice->getLSpliceLoc(), Splice,
            Splice->getRSpliceLoc(), TArgs, false);
    assert(!Result.isInvalid());  // Should never fail for dependent operands.

    return BuildCXXReflectExpr(OpLoc, Result.get());
  } else if (TemplateKWLoc.isValid() && !TArgs) {
    TemplateTy Template;
    TemplateNameKind TNK = ActOnTemplateName(getCurScope(), SS, TemplateKWLoc,
                                             Id, ParsedType(), false, Template);
    // The other kinds are "undeclared templates" and "non-templates".
    // As far as I can tell, neither can reach this point.
    assert(TNK == TNK_Dependent_template_name || TNK == TNK_Function_template ||
           TNK == TNK_Type_template || TNK == TNK_Var_template ||
           TNK == TNK_Concept_template);

    return BuildCXXReflectExpr(OpLoc, TemplateKWLoc, Template.get());
  } else if (SS.isSet() && SS.getScopeRep()->isDependent()) {
    ExprResult Result = BuildDependentDeclRefExpr(SS, TemplateKWLoc, NameInfo,
                                                  TArgs);
    // This should only fail if 'SS' is invalid, but that should already have
    // been diagnosed.
    assert(!Result.isInvalid());

    return BuildCXXReflectExpr(OpLoc, Result.get());
  } else if (!LookupParsedName(Found, getCurScope(), &SS, QualType()) ||
             Found.empty()) {
    CXXScopeSpec SS;
    DeclFilterCCC<VarDecl> CCC{};
    DiagnoseEmptyLookup(CurScope, SS, Found, CCC);
    return ExprError();
  }

  // Make sure the lookup was neither ambiguous nor resulting in an overload set
  // having more than one candidate.
  if (Found.isAmbiguous()) {
    return ExprError();
  } else if (Found.isOverloadedResult() && Found.end() - Found.begin() > 1) {
    Diag(Id.StartLocation, diag::err_reflect_overload_set);
    return ExprError();
  }

  // Unwrap any 'UsingShadowDecl'-nodes.
  NamedDecl *ND = Found.getRepresentativeDecl();
  while (auto *USD = dyn_cast<UsingShadowDecl>(ND))
    ND = USD->getTargetDecl();

  if (auto *TD = dyn_cast<TypeDecl>(ND)) {
    QualType QT = Context.getTypeDeclType(TD);
    return BuildCXXReflectExpr(OpLoc, NameInfo.getBeginLoc(), QT);
  }

  if (TArgs) {
    assert(isa<TemplateDecl>(ND) && !isa<ClassTemplateDecl>(ND) &&
           !isa<TypeAliasTemplateDecl>(ND));

    ExprResult Result = BuildTemplateIdExpr(SS, TemplateKWLoc, Found,
                                            /*RequiresADL=*/false, TArgs);
    if (Result.isInvalid())
      return ExprError();

    return BuildCXXReflectExpr(OpLoc, Result.get());
  }

  if (isa<NamespaceDecl, NamespaceAliasDecl, TranslationUnitDecl>(ND))
    return BuildCXXReflectExpr(OpLoc, NameInfo.getBeginLoc(), ND);

  if (isa<VarDecl, BindingDecl, FunctionDecl, FieldDecl, EnumConstantDecl,
          NonTypeTemplateParmDecl>(ND)) {
    ExprResult Result = BuildDeclarationNameExpr(SS, Found, false, false);
    if (Result.isInvalid())
      return ExprError();

    return BuildCXXReflectExpr(OpLoc, Result.get());
  }

  if (auto *TD = dyn_cast<TemplateDecl>(ND)) {
    return BuildCXXReflectExpr(OpLoc, NameInfo.getBeginLoc(),
                               TemplateName(TD));
  }

  llvm_unreachable("unknown reflection operand!");
}

ExprResult Sema::ActOnCXXReflectExpr(SourceLocation OpLoc, TypeResult T) {
  ParsedTemplateArgument Arg = ActOnTemplateTypeArgument(T);
  assert(Arg.getKind() == ParsedTemplateArgument::Type);

  return BuildCXXReflectExpr(OpLoc, Arg.getLocation(), T.get().get());
}

ExprResult Sema::ActOnCXXReflectExpr(SourceLocation OpLoc,
                                     SourceLocation ArgLoc, Decl *D) {
  return BuildCXXReflectExpr(OpLoc, ArgLoc, D);
}

ExprResult Sema::ActOnCXXReflectExpr(SourceLocation OpLoc,
                                     ParsedTemplateArgument Template) {
  assert(Template.getKind() == ParsedTemplateArgument::Template);

  ExprResult Result = BuildCXXReflectExpr(OpLoc, Template.getLocation(),
                                          Template.getAsTemplate().get());
  if (!Result.isInvalid() && Template.getEllipsisLoc().isValid())
    Result = ActOnPackExpansion(Result.get(), Template.getEllipsisLoc());

  return Result;
}

ExprResult Sema::ActOnCXXReflectExpr(SourceLocation OperatorLoc,
                                     CXXSpliceExpr *E) {
  return BuildCXXReflectExpr(OperatorLoc, E);
}

/// Returns an expression representing the result of a metafunction operating
/// on a reflection.
ExprResult Sema::ActOnCXXMetafunction(SourceLocation KwLoc,
                                      SourceLocation LParenLoc,
                                      SmallVectorImpl<Expr *> &Args,
                                      SourceLocation RParenLoc) {
  if (Args.empty()) {
    Diag(KwLoc, diag::err_metafunction_empty_args);
    return ExprError();
  }

  // Extract and validate the metafunction ID.
  Expr *FnIDArg = Args[0];
  {
    if (FnIDArg->isTypeDependent() || FnIDArg->isValueDependent())
      return ExprError();

    ExprResult FnIDArgConv = DefaultLvalueConversion(FnIDArg);
    if (FnIDArgConv.isInvalid())
      return ExprError();

    if (!FnIDArg->getType()->isIntegralOrEnumerationType()) {
      Diag(FnIDArg->getExprLoc(), diag::err_metafunction_leading_arg_type);
      return ExprError();
    }
    Args[0] = FnIDArg = FnIDArgConv.get();
  }

  // Evaluate metafunction ID as an RValue.
  Expr::EvalResult FnIDArgRV;
  {
    SmallVector<PartialDiagnosticAt, 4> Diags;
    FnIDArgRV.Diag = &Diags;

    if (!FnIDArg->EvaluateAsRValue(FnIDArgRV, Context, true)) {
      Diag(FnIDArg->getExprLoc(), diag::err_metafunction_not_constexpr);
      for (PartialDiagnosticAt PD : Diags)
        Diag(PD.first, PD.second);
      return ExprError();
    }
  }
  unsigned FnID = static_cast<unsigned>(FnIDArgRV.Val.getInt().getExtValue());

  // Look up the corresponding Metafunction object.
  const Metafunction *Metafn;
  if (Metafunction::Lookup(FnID, Metafn)) {
    Diag(FnIDArg->getExprLoc(), diag::err_unknown_metafunction);
    return ExprError();
  }

  // Validate the remaining arguments.
  if (Args.size() < Metafn->getMinArgs() + 1 ||
      Args.size() > Metafn->getMaxArgs() + 1) {
    Diag(KwLoc, diag::err_metafunction_arity)
        << (Metafn->getMinArgs() + 1)
        << (Metafn->getMaxArgs() + 1)
        << Args.size();
    return ExprError();
  }

  // Find or build a 'std::function' having a lambda with the 'Sema' object
  // (i.e., 'this') and the 'Metafunction' both captured. This will be provided
  // as a callback to evaluate the metafunction at constant evaluation time.
  auto ImplIt = MetafunctionImplCbs.find(FnID);
  if (ImplIt == MetafunctionImplCbs.end()) {
    auto MetafnImpl =
        std::make_unique<CXXMetafunctionExpr::ImplFn>(std::function(
          [this, Metafn](
              APValue &Result, CXXMetafunctionExpr::EvaluateFn EvalFn,
              CXXMetafunctionExpr::DiagnoseFn DiagFn, QualType ResultTy,
              SourceRange Range, ArrayRef<Expr *> Args) -> bool {
            return Metafn->evaluate(Result, *this, EvalFn, DiagFn, ResultTy,
                                    Range, Args);
        }));
    ImplIt = MetafunctionImplCbs.try_emplace(FnID, std::move(MetafnImpl)).first;
  }

  // Return the CXXMetafunctionExpr representation.
  return BuildCXXMetafunctionExpr(KwLoc, LParenLoc, RParenLoc,
                                  FnID, *ImplIt->second, Args);
}

ExprResult Sema::ActOnCXXSpliceSpecifierExpr(SourceLocation TemplateKWLoc,
                                             SourceLocation LSpliceLoc,
                                             Expr *Operand,
                                             SourceLocation RSpliceLoc) {
  return BuildCXXSpliceSpecifierExpr(TemplateKWLoc, LSpliceLoc, Operand,
                                     RSpliceLoc);
}

TypeResult Sema::ActOnCXXSpliceExpectingType(SourceLocation LSpliceLoc,
                                             Expr *Operand,
                                             SourceLocation RSpliceLoc,
                                             bool Complain) {
  TypeLocBuilder TLB;
  QualType SpliceTy = BuildReflectionSpliceTypeLoc(TLB, LSpliceLoc, Operand,
                                                   RSpliceLoc, Complain);
  if (SpliceTy.isNull())
    return TypeError();
  return CreateParsedType(SpliceTy, TLB.getTypeSourceInfo(Context, SpliceTy));
}

ExprResult Sema::ActOnCXXSpliceExpectingExpr(
      SourceLocation TemplateKWLoc, SourceLocation LSpliceLoc, Expr *Operand,
      SourceLocation RSpliceLoc, SourceLocation LAngleLoc,
      ASTTemplateArgsPtr TArgsIn, SourceLocation RAngleLoc,
      bool AllowMemberReference) {
  TemplateArgumentListInfo TArgs;
  if (TArgsIn.size() > 0) {
    TArgs.setLAngleLoc(LAngleLoc);
    TArgs.setRAngleLoc(RAngleLoc);
    translateTemplateArguments(TArgsIn, TArgs);
  }

  return BuildReflectionSpliceExpr(TemplateKWLoc, LSpliceLoc, Operand,
                                   RSpliceLoc, &TArgs, AllowMemberReference);
}

DeclResult Sema::ActOnCXXSpliceExpectingNamespace(SourceLocation LSpliceLoc,
                                                  Expr *Operand,
                                                  SourceLocation RSpliceLoc) {
  return BuildReflectionSpliceNamespace(LSpliceLoc, Operand, RSpliceLoc);
}

Sema::TemplateTy Sema::ActOnCXXSpliceExpectingTemplate(
      SourceLocation LSpliceLoc, Expr *Operand, SourceLocation RSpliceLoc,
      bool Complain) {
  return BuildReflectionSpliceTemplate(LSpliceLoc, Operand, RSpliceLoc,
                                       Complain);
}

ParsedTemplateArgument Sema::ActOnTemplateSpliceSpecifierArgument(
      CXXSpliceSpecifierExpr *Splice) {
  if (Splice->isValueDependent()) {
    return ParsedTemplateArgument(ParsedTemplateArgument::SpliceSpecifier,
                                  Splice, Splice->getExprLoc());
  }

  SmallVector<PartialDiagnosticAt, 4> Diags;
  Expr::EvalResult ER;
  ER.Diag = &Diags;
  if (!Splice->EvaluateAsRValue(ER, Context, true)) {
    return ParsedTemplateArgument();
  }
  assert(ER.Val.getKind() == APValue::Reflection);

  ReflectionValue RV = ER.Val.getReflection();
  if (Splice->getTemplateKWLoc().isValid() &&
     RV.getKind() != ReflectionValue::RK_template) {
    Diag(Splice->getOperand()->getExprLoc(),
         diag::err_unexpected_reflection_kind_in_splice) << 3;
    return ParsedTemplateArgument();
  }

  switch (RV.getKind()) {
  case ReflectionValue::RK_type:
    return ParsedTemplateArgument(ParsedTemplateArgument::Type,
                                  RV.getAsType().getAsOpaquePtr(),
                                  Splice->getExprLoc());
  case ReflectionValue::RK_object: {
    Expr *OVE = new (Context) OpaqueValueExpr(Splice->getExprLoc(),
                                              RV.getResultType(), VK_LValue);
    Expr *CE = ConstantExpr::Create(Context, OVE, RV.getAsObject());
    return ParsedTemplateArgument(ParsedTemplateArgument::NonType, CE,
                                  Splice->getExprLoc());
  }
  case ReflectionValue::RK_value: {
    Expr *OVE = new (Context) OpaqueValueExpr(Splice->getExprLoc(),
                                              RV.getResultType(), VK_PRValue);
    Expr *CE = ConstantExpr::Create(Context, OVE, RV.getAsValue());
    return ParsedTemplateArgument(ParsedTemplateArgument::NonType, CE,
                                  Splice->getExprLoc());
  }
  case ReflectionValue::RK_template: {
    TemplateName TName = RV.getAsTemplate();
    return ParsedTemplateArgument(ParsedTemplateArgument::Template,
                                  TName.getAsTemplateDecl(),
                                  Splice->getExprLoc());
  }
  case ReflectionValue::RK_declaration: {
    Expr *E = CreateRefToDecl(*this, cast<ValueDecl>(RV.getAsDecl()),
                              Splice->getExprLoc());
    return ParsedTemplateArgument(ParsedTemplateArgument::NonType, E,
                                  E->getExprLoc());
  }
  case ReflectionValue::RK_null:
    Diag(Splice->getExprLoc(), diag::err_unsupported_splice_kind)
      << "null reflections" << 0 << 0;
    break;
  case ReflectionValue::RK_namespace:
    Diag(Splice->getExprLoc(), diag::err_unsupported_splice_kind)
      << "namespaces" << 0 << 0;
    break;
  case ReflectionValue::RK_base_specifier:
    Diag(Splice->getExprLoc(), diag::err_unsupported_splice_kind)
      << "base specifiers" << 0 << 0;
    break;
  case ReflectionValue::RK_data_member_spec:
    Diag(Splice->getExprLoc(), diag::err_unsupported_splice_kind)
      << "data member specs" << 0 << 0;
    break;
  }
  return ParsedTemplateArgument();
}

bool Sema::ActOnCXXNestedNameSpecifierReflectionSplice(
    CXXScopeSpec &SS, CXXSpliceSpecifierExpr *Expr,
    SourceLocation ColonColonLoc) {
  assert(SS.isEmpty() && "splice must be leading component of NNS");

  if (!Expr->isValueDependent() && !TryFindDeclContextOf(Expr))
    return true;

  SS.MakeSpliceSpecifier(Context, Expr, ColonColonLoc);
  return false;
}

ExprResult Sema::BuildCXXReflectExpr(SourceLocation OperatorLoc,
                                     SourceLocation OperandLoc, QualType T) {
  ReflectionValue RV(ReflectionValue::RK_type, T.getAsOpaquePtr());
  return CXXReflectExpr::Create(Context, OperatorLoc, OperandLoc, RV);
}

// TODO(P2996): Capture whole SourceRange of declaration naming.
ExprResult Sema::BuildCXXReflectExpr(SourceLocation OperatorLoc,
                                     SourceLocation OperandLoc, Decl *D) {
  bool IsNamespace = isa<TranslationUnitDecl, NamespaceDecl,
                         NamespaceAliasDecl>(D);

  ReflectionValue RV(IsNamespace ? ReflectionValue::RK_namespace :
                                   ReflectionValue::RK_declaration, D);
  return CXXReflectExpr::Create(Context, OperatorLoc,
                                SourceRange(OperandLoc, OperandLoc), RV);
}

// TODO(P2996): Capture whole SourceRange of declaration naming.
ExprResult Sema::BuildCXXReflectExpr(SourceLocation OperatorLoc,
                                     SourceLocation OperandLoc,
                                     const TemplateName Template) {
  if (Template.getKind() == TemplateName::OverloadedTemplate) {
    Diag(OperandLoc, diag::err_reflect_overload_set);
    return ExprError();
  }

  ReflectionValue RV(ReflectionValue::RK_template, Template.getAsVoidPointer());
  return CXXReflectExpr::Create(Context, OperatorLoc,
                                SourceRange(OperandLoc, OperandLoc), RV);
}

ExprResult Sema::BuildCXXReflectExpr(SourceLocation OperatorLoc, Expr *E) {
  // Don't try to evaluate now if it's a value-dependent subexpression.
  if (E->isValueDependent())
    return CXXReflectExpr::Create(Context, OperatorLoc, E);

  // Check if this is a reference to a declared entity.
  if (auto *DRE = dyn_cast<DeclRefExpr>(E))
    return BuildCXXReflectExpr(OperatorLoc, DRE->getExprLoc(), DRE->getDecl());

  // Special case for '^[:splice:]'.
  if (auto *SE = dyn_cast<CXXSpliceExpr>(E))
    return BuildCXXReflectExpr(OperatorLoc, SE);

  // Always allow '^P' where 'P' is a template parameter.
  if (auto *SNTTPE = dyn_cast<SubstNonTypeTemplateParmExpr>(E))
    return BuildCXXReflectExpr(OperatorLoc, SNTTPE);

  // Handles cases like '^fn<int>'.
  if (auto *ULE = dyn_cast<UnresolvedLookupExpr>(E))
    return BuildCXXReflectExpr(OperatorLoc, ULE);

  // All other expressions are disallowed.
  Diag(E->getExprLoc(), diag::err_reflect_general_expression)
      << E->getSourceRange();
  return ExprError();
}

ExprResult Sema::BuildCXXReflectExpr(SourceLocation OperatorLoc,
                                     UnresolvedLookupExpr *E) {
  // If the UnresolvedLookupExpr could refer to multiple candidates, there
  // will be no means of choosing between them. Raise an error indicating
  // lack of support for reflection of overload sets at this time.
  auto *ULE = cast<UnresolvedLookupExpr>(E);

  // On the other hand, a unique candidate Decl might refer to a specialized
  // function template. Begin by inventing a 'VarDecl' for a 'const auto'
  // variable which would be initialized by the operand 'ULE'.
  QualType ConstAutoTy = Context.getAutoDeductType().withConst();
  TypeSourceInfo *TSI = Context.CreateTypeSourceInfo(ConstAutoTy, 0);
  auto *InventedVD = VarDecl::Create(Context, nullptr, SourceLocation(),
                                     E->getExprLoc(), nullptr, ConstAutoTy,
                                     TSI, SC_Auto);

  // Use the 'auto' deduction machinery to infer the operand type.
  if (DeduceVariableDeclarationType(InventedVD, true, ULE)) {
    Diag(E->getExprLoc(), diag::err_reflect_overload_set)
        << E->getSourceRange();
    return ExprError();
  }

  // Now use the type to obtain the unique overload candidate that this can
  // refer to; raise an error in the presence of any ambiguity.
  bool HadMultipleCandidates;
  DeclAccessPair FoundOverload;
  FunctionDecl *FoundDecl =
      ResolveAddressOfOverloadedFunction(ULE, InventedVD->getType(), true,
                                         FoundOverload,
                                         &HadMultipleCandidates);
  if (!FoundDecl) {
    Diag(E->getExprLoc(), diag::err_reflect_overload_set);
    return ExprError();
  }
  ExprResult ER = FixOverloadedFunctionReference(E, FoundOverload, FoundDecl);
  assert(!ER.isInvalid() && "could not fix overloaded function reference");

  return BuildCXXReflectExpr(OperatorLoc, ER.get());
}

ExprResult Sema::BuildCXXReflectExpr(SourceLocation OperatorLoc,
                                     SubstNonTypeTemplateParmExpr *E) {
  // Evaluate the replacement expression.
  SmallVector<PartialDiagnosticAt, 4> Diags;
  Expr::EvalResult ER;
  ER.Diag = &Diags;

  if (!E->EvaluateAsConstantExpr(ER, Context)) {
    Diag(E->getExprLoc(), diag::err_splice_operand_not_constexpr);
    for (PartialDiagnosticAt PD : Diags)
      Diag(PD.first, PD.second);
    return ExprError();
  }

  // "Promote" function references to the function declarations.
  // There is no analogous distinction between "objects" and "variables" for
  // functions.
  if (E->isLValue() && E->getType()->isFunctionType()) {
    const ValueDecl *VD = ER.Val.getLValueBase().get<const ValueDecl *>();
    return BuildCXXReflectExpr(OperatorLoc, E->getExprLoc(),
                               const_cast<ValueDecl *>(VD));
  }

  ReflectionValue RV;
  if (E->isLValue())
    RV = {ReflectionValue::RK_object, new (Context) APValue(ER.Val)};
  else
    RV = {ReflectionValue::RK_value, new (Context) APValue(ER.Val),
          E->getType()};
  return CXXReflectExpr::Create(Context, OperatorLoc, E->getSourceRange(),
                                RV);
}

ExprResult Sema::BuildCXXReflectExpr(SourceLocation OperatorLoc,
                                     CXXSpliceExpr *E) {
  assert(!E->isValueDependent());

  Expr *ToEval = E->getOperand();
  if (auto *ULE = dyn_cast<UnresolvedLookupExpr>(ToEval)) {
    ExprResult Result = BuildCXXReflectExpr(OperatorLoc, ULE);
    if (Result.isInvalid())
      return ExprError();

    ToEval = Result.get();
  }

  // Evaluate the replacement expression.
  SmallVector<PartialDiagnosticAt, 4> Diags;
  Expr::EvalResult ER;
  ER.Diag = &Diags;

  if (!ToEval->EvaluateAsConstantExpr(ER, Context)) {
    Diag(E->getExprLoc(), diag::err_splice_operand_not_constexpr);
    for (PartialDiagnosticAt PD : Diags)
      Diag(PD.first, PD.second);
    return ExprError();
  }

  return CXXReflectExpr::Create(Context, OperatorLoc, E->getSourceRange(),
                                ER.Val.getReflection());
}

ExprResult Sema::BuildCXXMetafunctionExpr(
      SourceLocation KwLoc, SourceLocation LParenLoc, SourceLocation RParenLoc,
      unsigned MetaFnID, const CXXMetafunctionExpr::ImplFn &Impl,
      SmallVectorImpl<Expr *> &Args) {
  // Look up the corresponding Metafunction object.
  const Metafunction *MetaFn;
  if (Metafunction::Lookup(MetaFnID, MetaFn)) {
    Diag(Args[0]->getExprLoc(), diag::err_unknown_metafunction);
    return ExprError();
  }

  // Derive the result type from the ResultKind of the metafunction.
  auto DeriveResultTy = [&](QualType &Result) -> bool {
    switch (MetaFn->getResultKind()) {
    case Metafunction::MFRK_bool:
      Result = Context.BoolTy;
      return false;
    case Metafunction::MFRK_metaInfo:
      Result = Context.MetaInfoTy;
      return false;
    case Metafunction::MFRK_sizeT:
      Result = Context.getSizeType();
      return false;
    case Metafunction::MFRK_sourceLoc: {
      RecordDecl *SourceLocDecl = lookupStdSourceLocationImpl(KwLoc);
      if (SourceLocDecl)
        Result = Context.getPointerType(
                              Context.getRecordType(SourceLocDecl).withConst());
      return SourceLocDecl == nullptr;
    }
    case Metafunction::MFRK_spliceFromArg: {
      Expr *TyRefl = Args[1];
      if (TyRefl->isTypeDependent() || TyRefl->isValueDependent()) {
        Result = Context.DependentTy;
        return false;
      }

      SmallVector<PartialDiagnosticAt, 4> Diags;
      Expr::EvalResult ER;
      ER.Diag = &Diags;

      if (!TyRefl->EvaluateAsRValue(ER, Context, true)) {
        Diag(TyRefl->getExprLoc(), diag::err_splice_operand_not_constexpr);
        for (PartialDiagnosticAt PD : Diags)
          Diag(PD.first, PD.second);
        return true;
      }

      if (!ER.Val.isReflection()) {
        Diag(TyRefl->getExprLoc(), diag::err_splice_operand_not_reflection);
        return true;
      }
      ReflectionValue& R = ER.Val.getReflection();

      if (R.getKind() != ReflectionValue::RK_type) {
        Diag(TyRefl->getExprLoc(), diag::err_unexpected_reflection_kind) << 0;
        return true;
      }

      Result = R.getAsType().getCanonicalType();
      return false;
    }
    }
    llvm_unreachable("unknown metafunction result kind");
  };
  QualType ResultTy;
  if (DeriveResultTy(ResultTy))
    return ExprError();
  return CXXMetafunctionExpr::Create(Context, MetaFnID, Impl, ResultTy, Args,
                                     KwLoc, LParenLoc, RParenLoc);
}

ExprResult Sema::BuildCXXSpliceSpecifierExpr(SourceLocation TemplateKWLoc,
                                             SourceLocation LSpliceLoc,
                                             Expr *Operand,
                                             SourceLocation RSpliceLoc) {
  ExprResult Result = DefaultLvalueConversion(Operand);
  if (Result.isInvalid())
    return ExprError();
  Operand = Result.get();

  if (!Operand->isValueDependent() && !Operand->isTypeDependent() &&
      Operand->getType() != Context.MetaInfoTy) {
    Result = PerformImplicitConversion(Operand, Context.MetaInfoTy,
                                       AA_Converting, false);
    if (Result.isInvalid())
      return ExprError();
    Operand = Result.get();
  }
  Operand = CXXSpliceSpecifierExpr::Create(Context, TemplateKWLoc, LSpliceLoc,
                                           Operand, RSpliceLoc);

  return Operand;
}

QualType Sema::BuildReflectionSpliceType(SourceLocation LSplice,
                                         Expr *Operand,
                                         SourceLocation RSplice,
                                         bool Complain) {
  if (Operand->isTypeDependent() || Operand->isValueDependent()) {
    return Context.getReflectionSpliceType(Operand, Context.DependentTy);
  }

  SmallVector<PartialDiagnosticAt, 4> Diags;
  Expr::EvalResult ER;
  ER.Diag = &Diags;

  if (!Operand->EvaluateAsRValue(ER, Context, true)) {
    Diag(Operand->getExprLoc(), diag::err_splice_operand_not_constexpr);
    for (PartialDiagnosticAt PD : Diags)
      Diag(PD.first, PD.second);
    return QualType();
  }

  if (!ER.Val.isReflection()) {
    Diag(Operand->getExprLoc(), diag::err_splice_operand_not_reflection);
    return QualType();
  }
  ReflectionValue &R = ER.Val.getReflection();

  if (R.getKind() == ReflectionValue::RK_template) {
    return Context.getDeducedTemplateSpecializationType(R.getAsTemplate(),
                                                        QualType(),
                                                        false);
  } else if (R.getKind() != ReflectionValue::RK_type) {
    if (Complain)
      Diag(Operand->getExprLoc(),
           diag::err_unexpected_reflection_kind_in_splice) << 0;
    return QualType();
  }

  QualType ReflectedTy = R.getAsType();

  // Check if the type refers to a substituted but uninstantiated template.
  if (auto *TT = dyn_cast<TagType>(ReflectedTy))
    if (auto *CTD = dyn_cast<ClassTemplateSpecializationDecl>(TT->getDecl());
        CTD && CTD->getSpecializationKind() == TSK_Undeclared) {
      TemplateName TName(CTD->getSpecializedTemplate());

      const TemplateArgumentList &TAList =
              CTD->getTemplateInstantiationArgs();
      TemplateArgumentListInfo TAListInfo(
              addLocToTemplateArgs(*this, TAList.asArray(),
                                   Operand->getExprLoc()));

      ReflectedTy = CheckTemplateIdType(TName, Operand->getExprLoc(),
                                        TAListInfo);
      if (ReflectedTy.isNull())
        return QualType();
    }

  return Context.getReflectionSpliceType(Operand, ReflectedTy);
}

QualType Sema::BuildReflectionSpliceTypeLoc(TypeLocBuilder &TLB,
                                            SourceLocation LSpliceLoc,
                                            Expr *E,
                                            SourceLocation RSpliceLoc,
                                            bool Complain) {
  QualType SpliceTy = BuildReflectionSpliceType(LSpliceLoc, E, RSpliceLoc,
                                                Complain);
  if (SpliceTy.isNull())
    return QualType();
  else if (isa<TemplateSpecializationType>(SpliceTy)) {
    auto TL = TLB.push<TemplateSpecializationTypeLoc>(SpliceTy);
    TL.setTemplateNameLoc(LSpliceLoc);
    return SpliceTy;
  } else if (isa<DeducedTemplateSpecializationType>(SpliceTy)) {
    auto TL = TLB.push<DeducedTemplateSpecializationTypeLoc>(SpliceTy);
    TL.setTemplateNameLoc(LSpliceLoc);
    return SpliceTy;
  }

  auto TL = TLB.push<ReflectionSpliceTypeLoc>(SpliceTy);
  TL.setLSpliceLoc(LSpliceLoc);
  TL.setRSpliceLoc(RSpliceLoc);

  return SpliceTy;
}

ExprResult Sema::BuildReflectionSpliceExpr(
      SourceLocation TemplateKWLoc, SourceLocation LSplice, Expr *Operand,
      SourceLocation RSplice, const TemplateArgumentListInfo *TArgs,
      bool AllowMemberReference) {
  if (isa<CXXSpliceSpecifierExpr>(Operand) &&
      !Operand->isTypeDependent() && !Operand->isValueDependent()) {
    auto *SpliceOp = cast<CXXSpliceSpecifierExpr>(Operand);

    SmallVector<PartialDiagnosticAt, 4> Diags;
    Expr::EvalResult ER;
    ER.Diag = &Diags;

    if (!Operand->EvaluateAsRValue(ER, Context, true)) {
      Diag(Operand->getExprLoc(), diag::err_splice_operand_not_constexpr);
      for (PartialDiagnosticAt PD : Diags)
        Diag(PD.first, PD.second);
      return ExprError();
    }

    if (!ER.Val.isReflection()) {
      Diag(Operand->getExprLoc(), diag::err_splice_operand_not_reflection);
      return ExprError();
    }
    ReflectionValue RV = ER.Val.getReflection();

    bool RequireTemplate = TemplateKWLoc.isValid() ||
                           TArgs->getLAngleLoc().isValid();
    if (RequireTemplate && RV.getKind() != ReflectionValue::RK_template) {
      Diag(Operand->getExprLoc(),
           diag::err_unexpected_reflection_kind_in_splice) << 3;
      return ExprError();
    }

    switch (RV.getKind()) {
    case ReflectionValue::RK_declaration: {
      Decl *TheDecl = RV.getAsDecl();

      // Class members may not be implicitly referenced through a splice.
      if (!AllowMemberReference &&
          (isa<FieldDecl>(TheDecl) ||
           (isa<CXXMethodDecl>(TheDecl) &&
            dyn_cast<CXXMethodDecl>(TheDecl)->isInstance()))) {
        Diag(Operand->getExprLoc(),
             diag::err_dependent_splice_implicit_member_reference)
          << Operand->getSourceRange();
        Diag(Operand->getExprLoc(),
             diag::note_dependent_splice_explicit_this_may_fix);
        return ExprError();
      }

      if (auto *FD = dyn_cast<FieldDecl>(TheDecl);
          FD && FD->isUnnamedBitField()) {
        Diag(Operand->getExprLoc(), diag::err_splice_unnamed_bit_field);
        return ExprError();
      }

      // Create a new DeclRefExpr, since the operand of the reflect expression
      // was parsed in an unevaluated context (but a splice expression is not
      // necessarily, and frequently not, in such a context).
      Operand = CreateRefToDecl(*this, cast<ValueDecl>(TheDecl),
                                Operand->getExprLoc());
      MarkDeclRefReferenced(cast<DeclRefExpr>(Operand), nullptr);
      Operand = CXXSpliceExpr::Create(Context, Operand->getValueKind(),
                                      TemplateKWLoc, LSplice, Operand, RSplice,
                                      TArgs, AllowMemberReference);
      break;
    }
    case ReflectionValue::RK_object: {
      Expr *OVE = new (Context) OpaqueValueExpr(SpliceOp->getExprLoc(),
                                                RV.getResultType(), VK_LValue);
      Expr *CE = ConstantExpr::Create(Context, OVE, RV.getAsObject());

      Operand = CXXSpliceExpr::Create(Context, VK_LValue, TemplateKWLoc,
                                      LSplice, CE, RSplice, TArgs,
                                      AllowMemberReference);
      break;
    }
    case ReflectionValue::RK_value: {
      Expr *OVE = new (Context) OpaqueValueExpr(SpliceOp->getExprLoc(),
                                                RV.getResultType(), VK_PRValue);
      Expr *CE = ConstantExpr::Create(Context, OVE, RV.getAsValue());

      Operand = CXXSpliceExpr::Create(Context, VK_PRValue, TemplateKWLoc,
                                      LSplice, CE, RSplice, TArgs,
                                      AllowMemberReference);
      break;
    }
    case ReflectionValue::RK_template: {
      if (SpliceOp->getTemplateKWLoc().isInvalid()) {
        Diag(SpliceOp->getOperand()->getExprLoc(),
             diag::err_unexpected_reflection_kind_in_splice)
          << 1 << SpliceOp->getOperand()->getSourceRange();
        return ExprError();
      }

      TemplateName TName = RV.getAsTemplate();
      assert(!TName.isDependent());

      TemplateDecl *TDecl = TName.getAsTemplateDecl();
      DeclarationNameInfo DeclNameInfo(TDecl->getDeclName(),
                                       Operand->getExprLoc());

      CXXScopeSpec SS;
      if (auto *RD = dyn_cast<CXXRecordDecl>(TDecl->getDeclContext())) {
        TypeSourceInfo *TSI = Context.getTrivialTypeSourceInfo(
                QualType(RD->getTypeForDecl(), 0), Operand->getExprLoc());
        SS.Extend(Context, SourceLocation(), TSI->getTypeLoc(),
                  Operand->getExprLoc());
      }

      if (auto *FTD = dyn_cast<FunctionTemplateDecl>(TDecl); FTD && TArgs) {
        SmallVector<TemplateArgument> Ignored;

        bool ConstraintFailure = false;
        if (CheckTemplateArgumentList(
                FTD, TemplateKWLoc,
                *const_cast<TemplateArgumentListInfo *>(TArgs), true, Ignored,
                Ignored, false, &ConstraintFailure) ||
            ConstraintFailure)
          return ExprError();
      } else if (auto *VTD = dyn_cast<VarTemplateDecl>(TDecl)) {
        ExprResult ER = CheckVarTemplateId(SS, DeclNameInfo, VTD, VTD,
                                           Operand->getExprLoc(), TArgs);
        if (ER.isInvalid())
          return ExprError();
        Operand = ER.get();
        Operand = CXXSpliceExpr::Create(Context, VK_LValue, TemplateKWLoc,
                                        LSplice, Operand, RSplice, TArgs,
                                        AllowMemberReference);
        break;
      } else if (auto *CD = dyn_cast<ConceptDecl>(TDecl)) {
        ExprResult ER = CheckConceptTemplateId(SS, SourceLocation(),
                                               DeclNameInfo, CD, CD, TArgs);
        if (ER.isInvalid())
          return ExprError();
        Operand = ER.get();
        Operand = CXXSpliceExpr::Create(Context, VK_PRValue, TemplateKWLoc,
                                        LSplice, Operand, RSplice, TArgs,
                                        AllowMemberReference);
        break;
      } else if (isa<ClassTemplateDecl>(TDecl) ||
                 isa<TypeAliasTemplateDecl>(TDecl)) {
        Diag(Operand->getExprLoc(),
             diag::err_unexpected_reflection_template_kind) << 1;
        return ExprError();
      }

      CXXRecordDecl *NamingCls = nullptr;
      NestedNameSpecifierLocBuilder NNSLocBuilder;
      if (auto *RD = dyn_cast<CXXRecordDecl>(TDecl->getDeclContext())) {
        TypeSourceInfo *TSI = Context.getTrivialTypeSourceInfo(
                QualType(RD->getTypeForDecl(), 0), Operand->getExprLoc());
        NNSLocBuilder.Extend(Context, SourceLocation(),
                             TSI->getTypeLoc(), Operand->getExprLoc());
      }

      UnresolvedSet<1> DeclSet;
      DeclSet.addDecl(TDecl);
      Operand = UnresolvedLookupExpr::Create(Context, NamingCls,
                                             SS.getWithLocInContext(Context),
                                             SourceLocation(), DeclNameInfo,
                                             false, TArgs, DeclSet.begin(),
                                             DeclSet.end(), false);

      Operand = CXXSpliceExpr::Create(Context, VK_LValue, TemplateKWLoc,
                                      LSplice, Operand, RSplice, TArgs,
                                      AllowMemberReference);
      break;
    }
    case ReflectionValue::RK_null:
    case ReflectionValue::RK_type:
    case ReflectionValue::RK_namespace:
    case ReflectionValue::RK_base_specifier:
    case ReflectionValue::RK_data_member_spec:
      Diag(SpliceOp->getOperand()->getExprLoc(),
           diag::err_unexpected_reflection_kind_in_splice)
          << 1 << SpliceOp->getOperand()->getSourceRange();
      return ExprError();
    }
    return Operand;
  }
  return CXXSpliceExpr::Create(Context, Operand->getValueKind(),
                               TemplateKWLoc, LSplice, Operand, RSplice,
                               TArgs, AllowMemberReference);
}

DeclResult Sema::BuildReflectionSpliceNamespace(SourceLocation LSplice,
                                                Expr *Operand,
                                                SourceLocation RSplice) {
  if (Operand->isValueDependent()) {
    auto *Splice = cast<CXXSpliceSpecifierExpr>(Operand);
    return DependentNamespaceDecl::Create(Context, CurContext, Splice);
  }

  SmallVector<PartialDiagnosticAt, 4> Diags;
  Expr::EvalResult ER;
  ER.Diag = &Diags;

  if (!Operand->EvaluateAsRValue(ER, Context, true)) {
    Diag(Operand->getExprLoc(), diag::err_splice_operand_not_constexpr);
    for (PartialDiagnosticAt PD : Diags)
      Diag(PD.first, PD.second);
    return DeclError();
  }

  if (!ER.Val.isReflection()) {
    Diag(Operand->getExprLoc(), diag::err_splice_operand_not_reflection);
    return DeclError();
  }
  ReflectionValue &R = ER.Val.getReflection();

  if (R.getKind() != ReflectionValue::RK_namespace) {
    Diag(Operand->getExprLoc(), diag::err_unexpected_reflection_kind) << 2;
    return DeclError();
  } else if (isa<TranslationUnitDecl>(R.getAsNamespace())) {
    Diag(Operand->getExprLoc(),
         diag::err_splice_global_scope_as_namespace);
    return DeclError();
  }

  return R.getAsNamespace();
}

Sema::TemplateTy Sema::BuildReflectionSpliceTemplate(SourceLocation LSplice,
                                                     Expr *Operand,
                                                     SourceLocation RSplice,
                                                     bool Complain) {
  assert(isa<CXXSpliceSpecifierExpr>(Operand));
  auto *SpliceOp = cast<CXXSpliceSpecifierExpr>(Operand);

  if (Operand->isValueDependent())
    return TemplateTy::make(
        Context.getDependentTemplateName(
            cast<CXXSpliceSpecifierExpr>(Operand)));

  SmallVector<PartialDiagnosticAt, 4> Diags;
  Expr::EvalResult ER;
  ER.Diag = &Diags;

  if (!Operand->EvaluateAsRValue(ER, Context, true)) {
    Diag(SpliceOp->getOperand()->getExprLoc(),
        diag::err_splice_operand_not_constexpr) << SpliceOp->getOperand();
    for (PartialDiagnosticAt PD : Diags)
      Diag(PD.first, PD.second);
    return TemplateTy();
  }

  if (!ER.Val.isReflection()) {
    Diag(SpliceOp->getOperand()->getExprLoc(),
         diag::err_splice_operand_not_reflection) << SpliceOp->getSourceRange();
    return TemplateTy();
  }
  ReflectionValue &R = ER.Val.getReflection();

  if (R.getKind() != ReflectionValue::RK_template) {
    if (Complain)
      Diag(SpliceOp->getOperand()->getExprLoc(),
           diag::err_unexpected_reflection_kind)
          << 3 << SpliceOp->getSourceRange();
    return TemplateTy();
  }

  return TemplateTy::make(R.getAsTemplate());
}

DeclContext *Sema::TryFindDeclContextOf(const Expr *E) {
  if (E->isTypeDependent() || E->isValueDependent())
    return nullptr;

  SmallVector<PartialDiagnosticAt, 4> Diags;
  Expr::EvalResult ER;
  ER.Diag = &Diags;

  Expr::EvalResult Result;
  if (!E->EvaluateAsRValue(Result, Context, true)) {
    Diag(E->getExprLoc(), diag::err_splice_operand_not_constexpr);
    for (PartialDiagnosticAt PD : Diags)
      Diag(PD.first, PD.second);
    return nullptr;
  }

  ReflectionValue Reflection = Result.Val.getReflection();
  switch (Reflection.getKind()) {
  case ReflectionValue::RK_type: {
    QualType QT = Reflection.getAsType();
    if (const TagType *TT = QT->getAs<TagType>())
      return TT->getDecl();

    Diag(E->getExprLoc(), diag::err_expected_class_or_namespace)
        << QT << getLangOpts().CPlusPlus;
    return nullptr;
  }
  case ReflectionValue::RK_namespace: {
    Decl *NS = Reflection.getAsNamespace();
    if (auto *A = dyn_cast<NamespaceAliasDecl>(NS))
      NS = A->getNamespace();
    return cast<DeclContext>(NS);
  }
  case ReflectionValue::RK_null:
  case ReflectionValue::RK_object:
  case ReflectionValue::RK_value:
  case ReflectionValue::RK_declaration:
  case ReflectionValue::RK_template:
  case ReflectionValue::RK_base_specifier:
  case ReflectionValue::RK_data_member_spec:
    Diag(E->getExprLoc(), diag::err_expected_class_or_namespace)
        << "spliced entity" << getLangOpts().CPlusPlus;
    return nullptr;
  }
  llvm_unreachable("unknown reflection kind");
}
