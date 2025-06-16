//===--- SemaExpand.cpp - Semantic Analysis for Expansion Statements-------===//
//
// Copyright 2024 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//  This file implements semantic analysis for expansion statements.
//
//===----------------------------------------------------------------------===//

#include "clang/AST/Decl.h"
#include "clang/Basic/DiagnosticSema.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Sema/EnterExpressionEvaluationContext.h"
#include "clang/Sema/Initialization.h"
#include "clang/Sema/Lookup.h"
#include "clang/Sema/Overload.h"
#include "clang/Sema/Sema.h"
#include "clang/Sema/Template.h"


using namespace clang;
using namespace sema;

namespace {

VarDecl *ExtractVarDecl(Stmt *S) {
  if (auto *DS = dyn_cast_or_null<DeclStmt>(S); S)
    if (Decl *D = DS->getSingleDecl(); D && !D->isInvalidDecl())
      return dyn_cast<VarDecl>(D);
  return nullptr;
}

unsigned ExtractParmVarDeclDepth(Expr *E) {
  if (auto *DRE = cast<DeclRefExpr>(E))
    if (auto *PVD = cast<NonTypeTemplateParmDecl>(DRE->getDecl()))
      return PVD->getDepth();
  return 0;
}

// Returns how many layers of templates the current scope is nested within.
unsigned ComputeTemplateEmbeddingDepth(Scope *CurScope) {
  int Depth = 0;
  while ((CurScope = CurScope->getParent())) {
    if (CurScope->isTemplateParamScope())
      ++Depth;
  }
  return Depth;
}

ExprResult makeIterableExpansionSizeExpr(Sema &S, VarDecl *RangeVar) {
  auto Ctx = Sema::ExpressionEvaluationContext::PotentiallyEvaluated;
  if (RangeVar->isConstexpr())
    Ctx = Sema::ExpressionEvaluationContext::ImmediateFunctionContext;
  EnterExpressionEvaluationContext ExprEvalCtx(S, Ctx);

  SourceLocation RangeLoc = RangeVar->getBeginLoc();

  DeclarationNameInfo BeginName(&S.PP.getIdentifierTable().get("begin"),
                                RangeLoc);
  LookupResult BeginLR(S, BeginName, Sema::LookupMemberName);
  {
    if (auto *RD = RangeVar->getType()->getAsCXXRecordDecl())
      S.LookupQualifiedName(BeginLR, RD);
  }

  DeclarationNameInfo EndName(&S.PP.getIdentifierTable().get("end"), RangeLoc);
  LookupResult EndLR(S, EndName, Sema::LookupMemberName);
  {
    if (auto *RD = RangeVar->getType()->getAsCXXRecordDecl())
      S.LookupQualifiedName(EndLR, RD);
  }

  Expr *VarRef;
  {
    DeclarationNameInfo Name(RangeVar->getDeclName(), RangeVar->getBeginLoc());
    VarRef = S.BuildDeclRefExpr(RangeVar, RangeVar->getType(), VK_LValue, Name,
                                nullptr, RangeVar);
  }
  assert(VarRef);

  ExprResult BeginResult;
  {
    OverloadCandidateSet CandidateSet(RangeLoc,
                                      OverloadCandidateSet::CSK_Normal);
    Sema::ForRangeStatus Status =
        S.BuildForRangeBeginEndCall(RangeLoc, RangeLoc, BeginName, BeginLR,
                                    &CandidateSet, VarRef, &BeginResult);
    if (Status != Sema::FRS_Success)
      return ExprError();
    assert(!BeginResult.isInvalid());
  }
  Expr *Begin = BeginResult.get();

  ExprResult EndResult;
  {
    OverloadCandidateSet CandidateSet(RangeLoc,
                                      OverloadCandidateSet::CSK_Normal);
    Sema::ForRangeStatus Status =
        S.BuildForRangeBeginEndCall(RangeLoc, RangeLoc, EndName, EndLR,
                                    &CandidateSet, VarRef, &EndResult);
    if (Status != Sema::FRS_Success)
      return ExprError();
    assert(!EndResult.isInvalid());
  }
  Expr *End = EndResult.get();

  OverloadedOperatorKind Op = OO_Minus;
  DeclarationName OpName(S.Context.DeclarationNames.getCXXOperatorName(Op));

  Expr *Args[2] = {EndResult.get(), BeginResult.get()};
  OverloadCandidateSet CandidateSet(RangeLoc,
                                    OverloadCandidateSet::CSK_Operator);
  S.AddArgumentDependentLookupCandidates(OpName, RangeLoc, Args, nullptr,
                                         CandidateSet);

  UnresolvedSet<4> Fns;
  S.AddFunctionCandidates(Fns, Args, CandidateSet);

  return S.MaybeCreateExprWithCleanups(
      S.CreateOverloadedBinOp(RangeLoc, BO_Sub, Fns, End, Begin));
}

// Returns 'true' if the 'Range' is an iterable expression, and 'false'
// otherwise. If 'true', then 'Result' contains the resulting
// 'CXXIterableExpansionSelectExpr' (or error).
bool tryMakeCXXIterableExpansionSelectExpr(
    Sema &S, Expr *Range, Expr *TParamRef, VarDecl *ExpansionVar,
    ArrayRef<MaterializeTemporaryExpr *> LifetimeExtendTemps,
    ExprResult &SelectResult) {
  auto Ctx = Sema::ExpressionEvaluationContext::PotentiallyEvaluated;
  if (ExpansionVar->isConstexpr())
    Ctx = Sema::ExpressionEvaluationContext::ImmediateFunctionContext;
  EnterExpressionEvaluationContext ExprEvalCtx(S, Ctx);

  if (Range->getType()->isArrayType())
    return false;
  SourceLocation RangeLoc = Range->getExprLoc();

  DeclarationNameInfo BeginName(&S.PP.getIdentifierTable().get("begin"),
                                RangeLoc);
  LookupResult BeginLR(S, BeginName, Sema::LookupMemberName);
  {
    if (auto *RD = Range->getType()->getAsCXXRecordDecl())
      S.LookupQualifiedName(BeginLR, RD);
  }

  VarDecl *RangeVar;
  Expr *VarRef;
  {
    assert(isa<ExpansionStmtDecl>(S.CurContext));
    DeclContext *DC = S.CurContext;
    while (isa<ExpansionStmtDecl>(DC))
      DC = DC->getParent();

    IdentifierInfo *II = &S.PP.getIdentifierTable().get("__range");
    QualType QT = Range->getType().withConst();
    TypeSourceInfo *TSI = S.Context.getTrivialTypeSourceInfo(QT);

    RangeVar = VarDecl::Create(S.Context, DC, Range->getBeginLoc(),
                               Range->getBeginLoc(), II, QT, TSI, SC_Auto);
    if (ExpansionVar->isConstexpr())
      RangeVar->setConstexpr(true);
    else if (!LifetimeExtendTemps.empty()) {
      InitializedEntity Entity =
          InitializedEntity::InitializeVariable(RangeVar);
      for (auto *MTE : LifetimeExtendTemps)
        MTE->setExtendingDecl(RangeVar, Entity.allocateManglingNumber());
    }

    S.AddInitializerToDecl(RangeVar, Range, false);
    if (RangeVar->isInvalidDecl())
      return false;

    DeclarationNameInfo Name(II, Range->getBeginLoc());
    VarRef = S.BuildDeclRefExpr(RangeVar, Range->getType(), VK_LValue, Name,
                                nullptr, RangeVar);
  }

  ExprResult BeginResult;
  {
    OverloadCandidateSet CandidateSet(RangeLoc,
                                      OverloadCandidateSet::CSK_Normal);
    Sema::ForRangeStatus Status =
        S.BuildForRangeBeginEndCall(RangeLoc, RangeLoc, BeginName, BeginLR,
                                    &CandidateSet, VarRef, &BeginResult);
    if (Status != Sema::FRS_Success)
      return false;
    assert(!BeginResult.isInvalid());
  }
  Expr *Begin = BeginResult.get();

  // Assume an error, unless we write something else.
  SelectResult = ExprError();

  OverloadedOperatorKind Op = OO_Plus;
  DeclarationName OpName(S.Context.DeclarationNames.getCXXOperatorName(Op));

  Expr *Args[2] = {Begin, TParamRef};
  OverloadCandidateSet CandidateSet(RangeLoc,
                                    OverloadCandidateSet::CSK_Operator);
  S.AddArgumentDependentLookupCandidates(OpName, RangeLoc, Args, nullptr,
                                         CandidateSet);

  UnresolvedSet<4> Fns;
  S.AddFunctionCandidates(Fns, Args, CandidateSet);

  ExprResult ER = S.CreateOverloadedBinOp(RangeLoc, BO_Add, Fns, Begin,
                                          TParamRef);
  if (ER.isInvalid())
    return true;
  Expr *UnaryArg[1] = {ER.get()};

  Op = OO_Star;
  OpName = S.Context.DeclarationNames.getCXXOperatorName(Op);

  CandidateSet.clear(OverloadCandidateSet::CSK_Operator);
  S.AddArgumentDependentLookupCandidates(OpName, RangeLoc, UnaryArg, nullptr,
                                         CandidateSet);

  Fns.clear();
  S.AddFunctionCandidates(Fns, UnaryArg, CandidateSet);

  ExprResult Impl = S.CreateOverloadedUnaryOp(RangeLoc, UO_Deref, Fns,
                                              ER.get());
  if (Impl.isInvalid())
    return true;

  SelectResult = S.BuildCXXIterableExpansionSelectExpr(RangeVar, Impl.get());
  return true;
}

ExprResult makeCXXDestructurableExpansionSelectExpr(
    Sema &S, Expr *Range, Expr *TParamRef, VarDecl *ExpansionVar,
    ArrayRef<MaterializeTemporaryExpr *> LifetimeExtendTemps) {
  auto Ctx = Sema::ExpressionEvaluationContext::PotentiallyEvaluated;
  if (ExpansionVar->isConstexpr())
    Ctx = Sema::ExpressionEvaluationContext::ImmediateFunctionContext;
  EnterExpressionEvaluationContext ExprEvalCtx(S, Ctx);

  UnsignedOrNone Arity = S.GetDecompositionElementCount(Range->getType(),
                                                        Range->getBeginLoc());
  if (!Arity)
    return ExprError();

  QualType QT = S.Context.getAutoDeductType();  // TODO: Add ref support.
  if (ExpansionVar->getType()->isReferenceType())
    QT = S.BuildReferenceType(QT, true, Range->getBeginLoc(),
                              DeclarationName());

  SmallVector<BindingDecl *, 4> Bindings;
  for (size_t k = 0; k < *Arity; ++k) {
    Bindings.push_back(BindingDecl::Create(S.Context, S.CurContext,
                                           Range->getBeginLoc(),
                                           /*IdentifierInfo=*/nullptr, QT));
  }

  TypeSourceInfo *TSI = S.Context.getTrivialTypeSourceInfo(QT);
  DecompositionDecl *DD = DecompositionDecl::Create(S.Context, S.CurContext,
                                                    Range->getBeginLoc(),
                                                    Range->getBeginLoc(),
                                                    QT, TSI,
                                                    SC_Auto, Bindings);
  if (ExpansionVar->isConstexpr())
    DD->setConstexpr(true);

  if (!LifetimeExtendTemps.empty()) {
    InitializedEntity Entity = InitializedEntity::InitializeVariable(DD);
    for (auto *MTE : LifetimeExtendTemps)
      MTE->setExtendingDecl(DD, Entity.allocateManglingNumber());
  }

  S.AddInitializerToDecl(DD, Range, false);

  return CXXDestructurableExpansionSelectExpr::Create(S.Context, DD,
                                                      TParamRef, ExpansionVar);
}
}  // close anonymous namespace

StmtResult Sema::ActOnCXXExpansionStmt(
    NonTypeTemplateParmDecl *NTTP, SourceLocation TemplateKWLoc,
    SourceLocation ForLoc, SourceLocation LParenLoc, Stmt *Init,
    Stmt *ExpansionVarStmt, SourceLocation ColonLoc, Expr *Range,
    SourceLocation RParenLoc, BuildForRangeKind Kind,
    ArrayRef<MaterializeTemporaryExpr *> LifetimeExtendTemps) {
  if (!Range || Kind == BFRK_Check)
    return StmtError();

  // Build a 'DeclRefExpr' designating the template parameter '__N'.
  ExprResult ER = BuildDeclRefExpr(NTTP, Context.getSizeType(), VK_PRValue,
                                   ColonLoc);
  if (ER.isInvalid())
    return StmtError();
  Expr *TParamRef = ER.get();

  VarDecl *ExpansionVar = ExtractVarDecl(ExpansionVarStmt);
  if (!ExpansionVar)
    return StmtError();

  ER = BuildCXXExpansionSelectExpr(Range, TParamRef, ExpansionVar,
                                   LifetimeExtendTemps);
  if (ER.isInvalid())
    return StmtError();
  Expr *Select = ER.get();

  assert(!ExpansionVar->getInit());
  AddInitializerToDecl(ExpansionVar, Select, false);
  if (ExpansionVar->isInvalidDecl())
    return StmtError();

  return BuildCXXExpansionStmt(TemplateKWLoc, ForLoc, LParenLoc, Init,
                               ExpansionVarStmt, ColonLoc, RParenLoc,
                               TParamRef);
}

StmtResult Sema::BuildCXXExpansionStmt(SourceLocation TemplateKWLoc,
                                       SourceLocation ForLoc,
                                       SourceLocation LParenLoc, Stmt *Init,
                                       Stmt *ExpansionVarStmt,
                                       SourceLocation ColonLoc,
                                       SourceLocation RParenLoc,
                                       Expr *TParamRef) {
  VarDecl *ExpansionVar = ExtractVarDecl(ExpansionVarStmt);
  if (!ExpansionVar)
    return StmtError();
  Expr *Select = ExpansionVar->getInit();
  assert(Select);

  if (auto *WithCleanups = dyn_cast<ExprWithCleanups>(Select))
    Select = WithCleanups->getSubExpr();

  if (isa<CXXExpansionInitListSelectExpr>(Select)) {
    return BuildCXXInitListExpansionStmt(TemplateKWLoc, ForLoc, LParenLoc,
                                         Init, ExpansionVarStmt, ColonLoc,
                                         RParenLoc, TParamRef);
  } else if (isa<CXXIndeterminateExpansionSelectExpr>(Select)) {
    return BuildCXXIndeterminateExpansionStmt(TemplateKWLoc, ForLoc, LParenLoc,
                                              Init, ExpansionVarStmt, ColonLoc,
                                              RParenLoc, TParamRef);
  } else if (isa<CXXDestructurableExpansionSelectExpr>(Select)) {
    return BuildCXXDestructurableExpansionStmt(TemplateKWLoc, ForLoc, LParenLoc,
                                               Init, ExpansionVarStmt, ColonLoc,
                                               RParenLoc, TParamRef);
  } else if (auto *IESE = dyn_cast<CXXIterableExpansionSelectExpr>(Select)) {
    ExprResult Size = makeIterableExpansionSizeExpr(*this, IESE->getRangeVar());
    if (Size.isInvalid()) {
      Diag(IESE->getExprLoc(), diag::err_compute_expansion_size_index) << 0;
      return StmtError();
    }
    return BuildCXXIterableExpansionStmt(TemplateKWLoc, ForLoc, LParenLoc, Init,
                                         ExpansionVarStmt, ColonLoc, RParenLoc,
                                         TParamRef, Size.get());
  }
  llvm_unreachable("unknown expansion select expression");
}

StmtResult
Sema::BuildCXXIndeterminateExpansionStmt(SourceLocation TemplateKWLoc,
                                         SourceLocation ForLoc,
                                         SourceLocation LParenLoc,
                                         Stmt *Init, Stmt *ExpansionVarStmt,
                                         SourceLocation ColonLoc,
                                         SourceLocation RParenLoc,
                                         Expr *TParamRef) {
  return CXXIndeterminateExpansionStmt::Create(
          Context, Init, cast<DeclStmt>(ExpansionVarStmt), TemplateKWLoc,
          ForLoc, LParenLoc, ColonLoc, RParenLoc, TParamRef);
}

StmtResult
Sema::BuildCXXIterableExpansionStmt(SourceLocation TemplateKWLoc,
                                    SourceLocation ForLoc,
                                    SourceLocation LParenLoc,
                                    Stmt *Init, Stmt *ExpansionVarStmt,
                                    SourceLocation ColonLoc,
                                    SourceLocation RParenLoc,
                                    Expr *TParamRef, Expr *SizeExpr) {
  unsigned NumExpansions = 0;
  if (!SizeExpr->isValueDependent()) {
    Expr::EvalResult ER;
    SmallVector<PartialDiagnosticAt, 4> Notes;
    ER.Diag = &Notes;

    if (SizeExpr->EvaluateAsInt(ER, Context, Expr::SE_NoSideEffects,
                                /*InConstantContext=*/true))
      NumExpansions = ER.Val.getInt().getZExtValue();
    else {
      Diag(SizeExpr->getExprLoc(), diag::err_compute_expansion_size_index) << 0;
      for (auto D : Notes)
        Diag(D.first, D.second);

      return StmtError();
    }
  }

  return CXXIterableExpansionStmt::Create(
          Context, Init, cast<DeclStmt>(ExpansionVarStmt), SizeExpr,
          NumExpansions, TemplateKWLoc, ForLoc, LParenLoc, ColonLoc, RParenLoc,
          TParamRef);
}

StmtResult
Sema::BuildCXXDestructurableExpansionStmt(SourceLocation TemplateKWLoc,
                                          SourceLocation ForLoc,
                                          SourceLocation LParenLoc,
                                          Stmt *Init, Stmt *ExpansionVarStmt,
                                          SourceLocation ColonLoc,
                                          SourceLocation RParenLoc,
                                          Expr *TParamRef) {
  return CXXDestructurableExpansionStmt::Create(
          Context, Init, cast<DeclStmt>(ExpansionVarStmt), TemplateKWLoc,
          ForLoc, LParenLoc, ColonLoc, RParenLoc, TParamRef);
}

StmtResult Sema::BuildCXXInitListExpansionStmt(SourceLocation TemplateKWLoc,
                                               SourceLocation ForLoc,
                                               SourceLocation LParenLoc,
                                               Stmt *Init,
                                               Stmt *ExpansionVarStmt,
                                               SourceLocation ColonLoc,
                                               SourceLocation RParenLoc,
                                               Expr *TParamRef) {
  return CXXInitListExpansionStmt::Create(Context, Init,
                                          cast<DeclStmt>(ExpansionVarStmt),
                                          TemplateKWLoc, ForLoc, LParenLoc,
                                          ColonLoc, RParenLoc, TParamRef);
}

ExprResult Sema::BuildCXXExpansionSelectExpr(
    Expr *Range, Expr *TParamRef, VarDecl *ExpansionVar,
    ArrayRef <MaterializeTemporaryExpr *> LifetimeExtendTemps) {
  if (Range->containsErrors())
    return ExprError();

  if (auto *EILE = dyn_cast<CXXExpansionInitListExpr>(Range))
    return BuildCXXExpansionInitListSelectExpr(EILE, TParamRef);

  if (Range->isTypeDependent())
    return BuildCXXIndeterminateExpansionSelectExpr(Range, TParamRef,
                                                    ExpansionVar,
                                                    LifetimeExtendTemps);

  ExprResult IterableExprResult;
  if (tryMakeCXXIterableExpansionSelectExpr(*this, Range, TParamRef,
                                            ExpansionVar, LifetimeExtendTemps,
                                            IterableExprResult))
    return IterableExprResult;

  return makeCXXDestructurableExpansionSelectExpr(*this, Range, TParamRef,
                                                  ExpansionVar,
                                                  LifetimeExtendTemps);
}

ExprResult
Sema::BuildCXXIndeterminateExpansionSelectExpr(
    Expr *Range, Expr *TParamRef, VarDecl *ExpansionVar,
    ArrayRef<MaterializeTemporaryExpr *> LifetimeExtendTemps) {
  return CXXIndeterminateExpansionSelectExpr::Create(Context, Range, TParamRef,
                                                     ExpansionVar,
                                                     LifetimeExtendTemps);
}

ExprResult
Sema::BuildCXXIterableExpansionSelectExpr(VarDecl *RangeVar, Expr *Impl) {
  if (Impl->isValueDependent())
    return CXXIterableExpansionSelectExpr::Create(Context, RangeVar, Impl);

  return Impl;
}

ExprResult
Sema::BuildCXXDestructurableExpansionSelectExpr(DecompositionDecl *DD,
                                                Expr *Idx,
                                                VarDecl *ExpansionVar) {
  if (Idx->isValueDependent())
    return CXXDestructurableExpansionSelectExpr::Create(Context, DD, Idx,
                                                        ExpansionVar);

  Expr::EvalResult ER;
  if (!Idx->EvaluateAsInt(ER, Context))
    return ExprError();
  unsigned I = ER.Val.getInt().getZExtValue();
  assert(I < DD->bindings().size());

  MarkAnyDeclReferenced(Idx->getBeginLoc(), DD, true);
  if (auto *BD = DD->bindings()[I]; auto *HVD = BD->getHoldingVar())
    return HVD->getInit();
  else
    return BD->getBinding();
}

ExprResult
Sema::BuildCXXExpansionInitListSelectExpr(CXXExpansionInitListExpr *Range,
                                          Expr *Idx) {
  // Use 'CXXExpansionInitListSelectExpr' as a placeholder until tree transform.
  if (Range->containsPack() || Idx->isValueDependent())
    return CXXExpansionInitListSelectExpr::Create(Context, Range, Idx);
  ArrayRef<Expr *> SubExprs = Range->getSubExprs();

  // Evaluate the index.
  Expr::EvalResult ER;

  SmallVector<PartialDiagnosticAt, 4> Notes;
  ER.Diag = &Notes;

  if (!Idx->EvaluateAsInt(ER, Context)) {
    Diag(Idx->getExprLoc(), diag::err_compute_expansion_size_index) << 1;
    for (auto D : Notes)
      Diag(D.first, D.second);

    return ExprError();
  }
  size_t I = ER.Val.getInt().getZExtValue();
  assert(I < SubExprs.size());

  return SubExprs[I];
}

StmtResult Sema::FinishCXXExpansionStmt(Stmt *Heading, Stmt *Body) {
  if (!Heading || !Body)
    return StmtError();

  CXXExpansionStmt *Expansion = cast<CXXExpansionStmt>(Heading);
  Expansion->setBody(Body);

  if (Expansion->hasDependentSize())
    return Expansion;

  // Return an empty statement if the range is empty.
  if (Expansion->getNumInstantiations() == 0)
     return Expansion;

  // Create a compound statement binding loop and body.
  Stmt *VarAndBody[] = {Expansion->getExpansionVarStmt(), Body};
  Stmt *CombinedBody = CompoundStmt::Create(Context, VarAndBody,
                                            FPOptionsOverride(),
                                            Expansion->getBeginLoc(),
                                            Expansion->getEndLoc());

  ExpansionStmtDecl *StmtDecl = cast<ExpansionStmtDecl>(CurContext);
  DeclContext *DC = CurContext;
  while (isa<ExpansionStmtDecl>(DC))
    DC = DC->getParent();

  // Expand the body for each instantiation.
  SmallVector<Stmt *, 4> Instantiations;
  while (Instantiations.size() < Expansion->getNumInstantiations()) {
    ContextRAII CtxGuard(*this, DC, /*NewThis=*/false);

    TemplateArgument TArgs[] = {
        { Context, llvm::APSInt::get(Instantiations.size()),
          Context.getSizeType() }
    };
    MultiLevelTemplateArgumentList MTArgList(StmtDecl, TArgs, true);
    MTArgList.addOuterRetainedLevels(
            ExtractParmVarDeclDepth(Expansion->getTParamRef()));

    LocalInstantiationScope LIScope(*this, /*CombineWithOuterScope=*/true);
    InstantiatingTemplate Inst(*this, Body->getBeginLoc(), Expansion, TArgs,
                               Body->getSourceRange());

    StmtResult Instantiation = SubstStmt(CombinedBody, MTArgList);

    if (Instantiation.isInvalid())
      return StmtError();
    Instantiations.push_back(Instantiation.get());
  }

  // Allocate a more permanent buffer to hold pointers to Stmts.
  Stmt **StmtStorage = new (Context) Stmt *[Instantiations.size()];
  std::memcpy(StmtStorage, Instantiations.data(),
              Instantiations.size() * sizeof(Stmt *));

  // Attach Stmt buffer to the CXXExpansionStmt, and return.
  Expansion->setInstantiations(StmtStorage);
  return Expansion;
}

ExprResult Sema::ActOnCXXExpansionInitList(SourceLocation LBraceLoc,
                                           MultiExprArg SubExprs,
                                           SourceLocation RBraceLoc) {
  return BuildCXXExpansionInitList(LBraceLoc, SubExprs, RBraceLoc);
}

ExprResult Sema::BuildCXXExpansionInitList(SourceLocation LBraceLoc,
                                           MultiExprArg SubExprs,
                                           SourceLocation RBraceLoc) {
  Expr **SubExprList = new (Context) Expr *[SubExprs.size()];
  std::memcpy(SubExprList, SubExprs.data(), SubExprs.size() * sizeof(Expr *));

  return CXXExpansionInitListExpr::Create(Context, SubExprList,
                                          SubExprs.size(), LBraceLoc,
                                          RBraceLoc);
}

Decl *Sema::ActOnExpansionStmtDeclaration(Scope *S,
                                          SourceLocation TemplateKWLoc) {
  // Compute how many layers of template parameters wrap this statement.
  unsigned TemplateDepth = ComputeTemplateEmbeddingDepth(S);

  // Create a template parameter '__N'.
  IdentifierInfo *ParmName = &Context.Idents.get("__N");
  QualType ParmTy = Context.getSizeType();
  TypeSourceInfo *ParmTI = Context.getTrivialTypeSourceInfo(ParmTy,
                                                            TemplateKWLoc);

  NonTypeTemplateParmDecl *TParam =
        NonTypeTemplateParmDecl::Create(Context,
                                        Context.getTranslationUnitDecl(),
                                        TemplateKWLoc, TemplateKWLoc,
                                        TemplateDepth, /*Position=*/0, ParmName,
                                        ParmTy, false, ParmTI);

  return BuildExpansionStmtDeclaration(TemplateKWLoc, TParam);
}

Decl *Sema::BuildExpansionStmtDeclaration(SourceLocation TemplateKWLoc,
                                          NonTypeTemplateParmDecl *NTTP) {
  TemplateParameterList *TParamList =
        TemplateParameterList::Create(Context, TemplateKWLoc, TemplateKWLoc,
                                      {NTTP}, TemplateKWLoc, nullptr);

  Decl *Result = ExpansionStmtDecl::Create(Context, CurContext, TemplateKWLoc,
                                           nullptr, TParamList);

  CurContext->addDecl(Result);
  return Result;
}
