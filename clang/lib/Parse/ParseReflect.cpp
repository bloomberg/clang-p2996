//===--- ParseReflect.cpp - C++2c Reflection Parsing (P2996) --------------===//
//
// Copyright 2024 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//  This file implements parsing for reflection facilities.
//
//===----------------------------------------------------------------------===//

#include "clang/Parse/ParseDiagnostic.h"
#include "clang/Parse/Parser.h"
#include "clang/Parse/RAIIObjectsForParser.h"
#include "clang/Sema/EnterExpressionEvaluationContext.h"
using namespace clang;

ExprResult Parser::ParseCXXReflectExpression(SourceLocation OpLoc) {
  SourceLocation OperandLoc = Tok.getLocation();

  EnterExpressionEvaluationContext EvalContext(
        Actions, Sema::ExpressionEvaluationContext::ReflectionContext);

  // Parse a leading nested-name-specifier, e.g.,
  //
  CXXScopeSpec SS;
  if (ParseOptionalCXXScopeSpecifier(SS, /*ObjectType=*/nullptr,
                                     /*ObjectHasErrors=*/false,
                                     /*EnteringContext=*/false)) {
    SkipUntil(tok::semi, StopAtSemi | StopBeforeMatch);
    return ExprError();
  }

  // Start the tentative parse: This will be reverted if the operand is found
  // to be a type (or rather: a type whose name is more complicated than a
  // single identifier).
  //
  TentativeParsingAction TentativeAction(*this);

  // Next, check for an unqualified-id.
  if (Tok.isOneOf(tok::identifier, tok::kw_operator, tok::kw_template,
                  tok::tilde, tok::annot_template_id)) {
    // Try parsing the operand name as an 'unqualified-id'.

    SourceLocation TemplateKWLoc;
    UnqualifiedId UnqualName;
    if (!ParseUnqualifiedId(SS, ParsedType{}, /*ObjectHadError=*/false,
                            /*EnteringContext=*/false,
                            /*AllowDestructorName=*/true,
                            /*AllowConstructorName=*/false,
                            /*AllowDeductionGuide=*/false,
                            SS.isSet() ? &TemplateKWLoc : nullptr,
                            UnqualName)) {
      bool AssumeType = false;
      if (UnqualName.getKind() == UnqualifiedIdKind::IK_TemplateId &&
          UnqualName.TemplateId->Kind == TNK_Type_template)
        AssumeType = true;
      else if (Tok.isOneOf(tok::l_square, tok::l_paren, tok::star, tok::amp,
                           tok::ampamp))
        AssumeType = true;

      if (!AssumeType) {
        TentativeAction.Commit();
        return Actions.ActOnCXXReflectExpr(OpLoc, TemplateKWLoc, SS,
                                           UnqualName);
      }
    }
  } else if (SS.isValid() &&
             SS.getScopeRep()->getKind() == NestedNameSpecifier::Global) {
    // Check for '^::'.
    TentativeAction.Commit();

    Decl *TUDecl = Actions.getASTContext().getTranslationUnitDecl();
    return Actions.ActOnCXXReflectExpr(OpLoc, SourceLocation(), TUDecl);
  }
  TentativeAction.Revert();

  // Anything else must be a type-id (e.g., 'const int', 'Cls(*)(int)'.
  if (isCXXTypeId(TypeIdAsReflectionOperand)) {
    TypeResult TR = ParseTypeName(nullptr, DeclaratorContext::ReflectOperator);
    if (TR.isInvalid())
      return ExprError();

    return Actions.ActOnCXXReflectExpr(OpLoc, TR);
  }

  Diag(OperandLoc, diag::err_cannot_reflect_operand);
  return ExprError();
}

ExprResult Parser::ParseCXXMetafunctionExpression() {
  assert(Tok.is(tok::kw___metafunction) && "expected '___metafunction'");
  SourceLocation KwLoc = ConsumeToken();

  // Balance any number of arguments in parens.
  BalancedDelimiterTracker Parens(*this, tok::l_paren);
  if (Parens.expectAndConsume())
    return ExprError();

  SmallVector<Expr *, 2> Args;
  do {
    ExprResult Expr = ParseConstantExpression();
    if (Expr.isInvalid()) {
      Parens.skipToEnd();
      return ExprError();
    }
    Args.push_back(Expr.get());
  } while (TryConsumeToken(tok::comma));

  if (Parens.consumeClose())
    return ExprError();

  SourceLocation LPLoc = Parens.getOpenLocation();
  SourceLocation RPLoc = Parens.getCloseLocation();
  return Actions.ActOnCXXMetafunction(KwLoc, LPLoc, Args, RPLoc);
}

bool Parser::ParseCXXSpliceSpecifier(SourceLocation TemplateKWLoc) {
  assert(Tok.is(tok::l_splice) && "expected '[:'");

  BalancedDelimiterTracker SpliceTokens(*this, tok::l_splice);
  if (SpliceTokens.expectAndConsume())
    return true;

  ExprResult ER;
  {
    EnterExpressionEvaluationContext EvalContext(
        Actions, Sema::ExpressionEvaluationContext::ConstantEvaluated);
    ER = ParseConstantExpression();
  }
  if (ER.isInvalid()) {
    SpliceTokens.skipToEnd();
    return true;
  }
  Expr *Operand = ER.get();

  Token end = Tok;
  if (SpliceTokens.consumeClose())
    return true;

  SourceLocation LSplice = SpliceTokens.getOpenLocation();
  SourceLocation RSplice = SpliceTokens.getCloseLocation();

  ER = Actions.ActOnCXXSpliceSpecifierExpr(TemplateKWLoc, LSplice, Operand,
                                           RSplice);
  if (ER.isInvalid() || ER.get()->containsErrors())
    return true;
  Expr *SpliceExpr = ER.get();

  UnconsumeToken(end);
  Tok.setKind(tok::annot_splice);
  setExprAnnotation(Tok, SpliceExpr);
  Tok.setLocation(LSplice);
  Tok.setAnnotationEndLoc(RSplice);
  PP.AnnotateCachedTokens(Tok);

  return false;
}

TypeResult Parser::ParseCXXSpliceAsType(bool AllowDependent,
                                        bool Complain) {
  assert(Tok.is(tok::annot_splice) && "expected annot_splice");

  if (NextToken().is(tok::less)) {
    // TODO(P2996): Handle type constraints.
    if (ParseTemplateAnnotationFromSplice(SourceLocation(), true, false,
                                          /*Complain=*/true))
      return TypeError();
    return ParseTypeName();
  }

  Token Splice = Tok;

  ExprResult ER = getExprAnnotation(Splice);
  assert(!ER.isInvalid());
  Expr *Operand = ER.get();

  if (!AllowDependent)
    if (Operand->isTypeDependent() || Operand->isValueDependent())
      return TypeError();

  TypeResult Result = Actions.ActOnCXXSpliceExpectingType(
          Splice.getLocation(), ER.get(), Splice.getAnnotationEndLoc(),
          Complain);
  if (!Result.isInvalid())
    ConsumeAnnotationToken();

  return Result;
}

ExprResult Parser::ParseCXXSpliceAsExpr(bool AllowMemberReference) {
  assert(Tok.is(tok::annot_splice) && "expected annot_splice");

  ExprResult ER = getExprAnnotation(Tok);
  assert(!ER.isInvalid());

  auto *Splice = cast<CXXSpliceSpecifierExpr>(ER.get());
  SourceLocation TemplateKWLoc = Splice->getTemplateKWLoc();
  SourceLocation RSpliceLoc = Tok.getAnnotationEndLoc();
  SourceLocation LSpliceLoc = ConsumeAnnotationToken();

  ASTTemplateArgsPtr TArgsPtr;
  SourceLocation LAngleLoc, RAngleLoc;
  if (TemplateKWLoc.isValid() && Tok.is(tok::less)) {
    TemplateArgList TArgs;
    if (ParseTemplateIdAfterTemplateName(/*ConsumeLastToken=*/true,
                                         LAngleLoc, TArgs, RAngleLoc,
                                         /*Template=*/nullptr))
      return ExprError();

    TArgsPtr = ASTTemplateArgsPtr(TArgs.data(), TArgs.size());
  }

  return Actions.ActOnCXXSpliceExpectingExpr(TemplateKWLoc, LSpliceLoc, Splice,
                                             RSpliceLoc, LAngleLoc, TArgsPtr,
                                             RAngleLoc, AllowMemberReference);
}

DeclResult Parser::ParseCXXSpliceAsNamespace() {
  assert(Tok.is(tok::annot_splice) && "expected annot_splice");

  Token Splice = Tok;
  ConsumeAnnotationToken();

  ExprResult ER = getExprAnnotation(Splice);
  assert(!ER.isInvalid());

  DeclResult Result = Actions.ActOnCXXSpliceExpectingNamespace(
          Splice.getLocation(), ER.get(), Splice.getAnnotationEndLoc());

  return Result;
}

Parser::TemplateTy Parser::ParseCXXSpliceAsTemplate() {
  assert(Tok.is(tok::annot_splice) && "expected annot_splice");

  Token Splice = Tok;
  ConsumeAnnotationToken();

  ExprResult ER = getExprAnnotation(Splice);
  assert(!ER.isInvalid());

  return Actions.ActOnCXXSpliceExpectingTemplate(
          Splice.getLocation(), ER.get(), Splice.getAnnotationEndLoc(),
          /*Complain=*/true);
}

static TemplateNameKind classifyTemplateDecl(TemplateName TName) {
  if (TName.isDependent())
    return TNK_Dependent_template_name;

  TemplateDecl *D = TName.getAsTemplateDecl();
  if (isa<FunctionTemplateDecl>(D))
    return TNK_Function_template;
  if (isa<ClassTemplateDecl>(D) || isa<TypeAliasTemplateDecl>(D))
    return TNK_Type_template;
  if (isa<VarTemplateDecl>(D))
    return TNK_Var_template;
  if (isa<ConceptDecl>(D))
    return TNK_Concept_template;

  llvm_unreachable("unknown template kind");
}

bool Parser::ParseTemplateAnnotationFromSplice(SourceLocation TemplateKWLoc,
                                               bool AllowTypeAnnotation,
                                               bool TypeConstraint,
                                               bool Complain) {
  assert(Tok.is(tok::annot_splice) && "expected annot_splice");

  Token Splice = Tok;

  ExprResult ER = getExprAnnotation(Splice);
  assert(!ER.isInvalid());
  ConsumeAnnotationToken();

  TemplateTy Template = Actions.ActOnCXXSpliceExpectingTemplate(
          Splice.getLocation(), ER.get(), Splice.getAnnotationEndLoc(),
          Complain);
  if (!Template)
    return true;
  bool IsDependent = Template.get().isDependent();
  TemplateDecl *TDecl = Template.get().getAsTemplateDecl();
  assert((IsDependent || TDecl) && "no template decl??");

  assert((Tok.is(tok::less) || TypeConstraint) && "not a template-id?");
  assert(!(TypeConstraint && AllowTypeAnnotation) &&
         "type-constraint can't be a type annotation");
  assert((!TypeConstraint || IsDependent || isa<ConceptDecl>(TDecl)) &&
         "type-constraint must accompany a concept name");

  SourceLocation TemplateNameLoc = Splice.getLocation();
  SourceLocation LAngleLoc, RAngleLoc;
  TemplateArgList TArgs;
  bool ArgsInvalid = false;
  if (!TypeConstraint || Tok.is(tok::less)) {
    ArgsInvalid = ParseTemplateIdAfterTemplateName(false, LAngleLoc, TArgs,
                                                   RAngleLoc, Template);
    if (RAngleLoc.isInvalid())
      return true;
  }

  // Build the annotation token.
  if (AllowTypeAnnotation && (IsDependent || isa<ClassTemplateDecl>(TDecl) ||
                              isa<TypeAliasTemplateDecl>(TDecl))) {
    CXXScopeSpec SS;
    ASTTemplateArgsPtr TArgsPtr(TArgs);

    TypeResult Type = ArgsInvalid
                          ? TypeError()
                          : Actions.ActOnTemplateIdType(
                                getCurScope(), SS, TemplateKWLoc, Template,
                                nullptr, TemplateNameLoc, LAngleLoc, TArgsPtr,
                                RAngleLoc);

    Tok.setKind(tok::annot_typename);
    setTypeAnnotation(Tok, Type);
  } else {
    // Build template-id annotation that can be processed later.
    Tok.setKind(tok::annot_template_id);

    TemplateNameKind TNK = classifyTemplateDecl(Template.get());
    TemplateIdAnnotation *TemplateId = TemplateIdAnnotation::Create(
        TemplateKWLoc, TemplateNameLoc, nullptr, OO_None, Template, TNK,
        LAngleLoc, RAngleLoc, TArgs, ArgsInvalid, TemplateIds);

    Tok.setAnnotationValue(TemplateId);
  }
  Tok.setAnnotationEndLoc(RAngleLoc);
  if (TemplateKWLoc.isValid())
    Tok.setLocation(TemplateKWLoc);
  else
    Tok.setLocation(TemplateNameLoc);

  // In case tokens were cached, ensure that Preprocessor replaces them with the
  // annotation token.
  PP.AnnotateCachedTokens(Tok);
  return false;
}
