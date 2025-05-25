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
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/Attr.h"
#include "clang/AST/DeclBase.h"
#include "clang/AST/MetaActions.h"
#include "clang/AST/Metafunction.h"
#include "clang/AST/Reflection.h"
#include "clang/Basic/DiagnosticSema.h"
#include "clang/Sema/EnterExpressionEvaluationContext.h"
#include "clang/Sema/Lookup.h"
#include "clang/Sema/Ownership.h"
#include "clang/Sema/ParsedAttr.h"
#include "clang/Sema/ParsedTemplate.h"
#include "clang/Sema/Sema.h"
#include "clang/Sema/Template.h"
#include "clang/Sema/TemplateDeduction.h"
#include "llvm/Support/raw_ostream.h"

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

Expr *CreateRefToDecl(Sema &S, ValueDecl *D, SourceLocation ExprLoc) {
  CXXScopeSpec SS;
  if (const auto *RDC = dyn_cast<RecordDecl>(D->getDeclContext())) {
    QualType QT(RDC->getTypeForDecl(), 0);
    TypeSourceInfo *TSI = S.Context.CreateTypeSourceInfo(QT, 0);
    SS.Extend(S.Context, SourceLocation(), TSI->getTypeLoc(), ExprLoc);
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

    return S.BuildDeclRefExpr(D, QT, ValueKind, ExprLoc, &SS);
  }
}

Decl *findInjectionCone(Decl *ContainingDecl) {
  for (Decl *Ctx = ContainingDecl; Ctx;
       Ctx = cast<Decl>(Ctx->getDeclContext())) {
    if (isa<RecordDecl, FunctionDecl, TranslationUnitDecl>(Ctx))
      return Ctx;
  }
  llvm_unreachable("should have terminated at a TranslationUnitDecl");
}

bool CheckReflectVar(Sema &S, VarDecl *VD, SourceRange Range) {
  // Reflections of 'init-capture's are always ill-formed.
  if (VD->isInitCapture()) {
    S.Diag(Range.getBegin(), diag::err_reflect_init_capture) << Range;
    return true;
  }

  if (isa<RequiresExprBodyDecl>(VD->getDeclContext())) {
    assert(isa<ParmVarDecl>(VD));
    S.Diag(Range.getBegin(), diag::err_reflect_local_requires_param) << Range;
    return true;
  }

  // All other cases that aren't local entities are fine.
  if (!VD->isLocalVarDeclOrParm() || VD->isStaticLocal())
    return false;

  // Check for an intervening lambda scope.
  for (DeclContext *DC = S.CurContext; DC != VD->getDeclContext();
       DC = DC->getParent()) {
    assert(DC && "Var context not a parent of the current context");
    if (auto *RD = dyn_cast<CXXRecordDecl>(DC); RD && RD->isLambda()) {
      S.Diag(Range.getBegin(), diag::err_reflect_intervening_lambda) << Range;
      return true;
    }
  }
  return false;
}

bool CheckSpliceVar(Sema &S, VarDecl *VD, SourceRange Range) {
  // All non-local entities are fine.
  if (!VD->isLocalVarDeclOrParm() || VD->isStaticLocal())
    return false;

  // Unevaluated contexts are fine.
  //
  // We should also ignore any enclosing 'typeid' expressions, but clang (as far
  // as I can tell) doesn't implement that for lambda captures either, so we
  // likewise ignore that here.
  if (!S.currentEvaluationContext().isPotentiallyEvaluated())
    return false;

  // Check for an intervening lambda scope.
  for (DeclContext *DC = S.CurContext; DC != VD->getDeclContext();
       DC = DC->getParent()) {
    assert(DC && "Var context not a parent of the current context");
    if (auto *RD = dyn_cast<CXXRecordDecl>(DC); RD && RD->isLambda()) {
      S.Diag(Range.getBegin(), diag::err_splice_intervening_lambda)
          << VD << Range;
      S.Diag(VD->getLocation(), diag::note_entity_declared_at) << VD;
      return true;
    }
  }
  return false;
}

APValue MaybeUnproxy(ASTContext &C, APValue RV) {
  assert(RV.isReflection());

  if (!RV.isReflectedEntityProxy())
    return RV;

  NamedDecl *ND = RV.getReflectedEntityProxy()->getTargetDecl();
  if (auto *T = dyn_cast<TypedefNameDecl>(ND)) {
    QualType QT = T->getUnderlyingType();
    return APValue(ReflectionKind::Type, QT.getAsOpaquePtr());
  } else if (auto *T = dyn_cast<TypeDecl>(ND)) {
    QualType QT = C.getTypeDeclType(T);
    return APValue(ReflectionKind::Type, QT.getAsOpaquePtr());
  } else if (auto *T = dyn_cast<TemplateDecl>(ND)) {
    return APValue(ReflectionKind::Template, T);
  }

  return APValue(ReflectionKind::Declaration, ND);
}

class MetaActionsImpl : public MetaActions {
  Sema &S;

  void populateTemplateArgumentListInfo(TemplateArgumentListInfo &TAListInfo,
                                        ArrayRef<TemplateArgument> TArgs,
                                        SourceLocation InstantiateLoc) {
    // Non-type template arguments are constant-expressions, so make sure any
    // expressions are formed are considered in an immediate function context.
    EnterExpressionEvaluationContext Ctx(
        S, Sema::ExpressionEvaluationContext::ImmediateFunctionContext);

    for (const TemplateArgument &Arg : TArgs)
      TAListInfo.addArgument(
           S.getTrivialTemplateArgumentLoc(Arg,
                                           Arg.getNonTypeTemplateArgumentType(),
                                           InstantiateLoc));
    TAListInfo.setLAngleLoc(InstantiateLoc);
    TAListInfo.setRAngleLoc(InstantiateLoc);
  }

public:
  MetaActionsImpl(Sema &S) : MetaActions(), S(S) { }

  Decl *CurrentCtx() const override {
    return cast<Decl>(S.CurContext);
  }

  bool IsAccessible(NamedDecl *Target, DeclContext *Ctx,
                    CXXRecordDecl *NamingCls) override {
    bool Result = false;

    // If 'Target' is a (possibly nested) anonymous struct/union or unscoped
    // enumerator, replace it with its parent recursively until it's no longer
    // such a member.
    while (Target && Target->getDeclContext() &&
           Target->getDeclContext() != NamingCls &&
           [](DeclContext *DC) {
             if (auto *RD = dyn_cast<CXXRecordDecl>(DC))
               return RD->isAnonymousStructOrUnion();
             else return DC->isTransparentContext();
           }(Target->getDeclContext()))
      if (isa<TranslationUnitDecl>(Target->getDeclContext()))
        // Can happen if Target was a member of a static anonymous union at
        // namespace scope.
        return true;
      else
        Target = cast<NamedDecl>(Target->getDeclContext());

    if (auto *Cls = dyn_cast_or_null<CXXRecordDecl>(Target->getDeclContext())) {
      if (Cls != NamingCls &&
          !S.IsDerivedFrom(SourceLocation{},
                           QualType(NamingCls->getTypeForDecl(), 0),
                           QualType(Cls->getTypeForDecl(), 0)))
        return false;
      else if (NamingCls->isAnonymousStructOrUnion())
        // Clang's access-checking machinery isn't equipped to deal with checks
        // where the "naming" class (ha!) is anonymous - can't imagine why!
        return true;

      DeclContext *PreviousDC = S.CurContext;
      {
        S.CurContext = Ctx;
        Result = S.IsSimplyAccessible(Target, NamingCls, QualType{});
        S.CurContext = PreviousDC;
      }
    }
    return Result;
  }

  bool IsAccessibleBase(QualType BaseTy, QualType DerivedTy,
                        const CXXBasePath &Path,
                        DeclContext *Ctx, SourceLocation AccessLoc) override {
    Sema::AccessResult Result;

    DeclContext *PreviousDC = S.CurContext;
    {
      S.CurContext = Ctx;
      Result = S.CheckBaseClassAccess(AccessLoc, BaseTy, DerivedTy, Path, 0,
                                      /*ForceCheck=*/true,
                                      /*ForceUnprivileged=*/false);
      S.CurContext = PreviousDC;
    }
    return (Result == Sema::AR_accessible);
  }

  bool EnsureInstantiated(Decl *D, SourceRange Range) override {
    auto validateConstraints = [&](TemplateDecl *TDecl,
                                 ArrayRef<TemplateArgument> TArgs) {
      MultiLevelTemplateArgumentList MLTAL(TDecl, TArgs, false);
      if (S.EnsureTemplateArgumentListConstraints(TDecl, MLTAL, Range))
        return false;

      return true;
    };

    // Cover case of static variables in a specialization not yet referenced.
    if (auto *VD = dyn_cast<VarDecl>(D); VD && VD->hasGlobalStorage())
      S.MarkVariableReferenced(Range.getBegin(), VD);

    if (auto *CTSD = dyn_cast<ClassTemplateSpecializationDecl>(D);
        CTSD && !CTSD->isCompleteDefinition()) {
      if (!validateConstraints(CTSD->getSpecializedTemplate(),
                               CTSD->getTemplateArgs().asArray()))
        return true;

      if (S.InstantiateClassTemplateSpecialization(
              Range.getBegin(), CTSD, TSK_ExplicitInstantiationDefinition, false,
              false))
        return false;

      S.InstantiateClassTemplateSpecializationMembers(
              Range.getBegin(), CTSD, TSK_ExplicitInstantiationDefinition);
    } else if (auto *VTSD = dyn_cast<VarTemplateSpecializationDecl>(D);
        VTSD && !VTSD->isCompleteDefinition()) {
      if (!validateConstraints(VTSD->getSpecializedTemplate(),
                               VTSD->getTemplateArgs().asArray()))
        return true;

      S.InstantiateVariableDefinition(Range.getBegin(), VTSD, true, true);
    } else if (auto *FD = dyn_cast<FunctionDecl>(D);
               FD && FD->isTemplateInstantiation()) {
      if (FD->getTemplateSpecializationArgs())
        if (!validateConstraints(FD->getPrimaryTemplate(),
                                 FD->getTemplateSpecializationArgs()->asArray()))
        return true;

      S.InstantiateFunctionDefinition(Range.getBegin(), FD, true, true);
    }
    return true;
  }

  bool HasSatisfiedConstraints(FunctionDecl *FD) override {
    bool Result = true;
    if (FD->getTrailingRequiresClause()) {
      ConstraintSatisfaction Sat;
      Result = !S.CheckFunctionConstraints(FD, Sat, SourceLocation{}, false) &&
               Sat.IsSatisfied;
    }
    return Result;
  }

  bool CheckTemplateArgumentList(TemplateDecl *TD,
                                 SmallVectorImpl<TemplateArgument> &TArgs,
                                 bool SuppressDiagnostics,
                                 SourceLocation InstantiateLoc) override {
    TemplateArgumentListInfo TAListInfo;
    populateTemplateArgumentListInfo(TAListInfo, TArgs, InstantiateLoc);

    DefaultArguments DefaultArgs;
    Sema::CheckTemplateArgumentInfo CompletedTArgs;

    auto check = [&]() {
      return !S.CheckTemplateArgumentList(TD, InstantiateLoc, TAListInfo,
                                          DefaultArgs, false, CompletedTArgs,
                                          true);
    };
    bool Result;
    if (SuppressDiagnostics) {
      Sema::SuppressDiagnosticsRAII NoDiagnostics(S);
      Result = check();
    } else {
      Result = check();
    }
    TArgs = CompletedTArgs.CanonicalConverted;
    return Result;
  }

  void EnsureDeclarationOfImplicitMembers(CXXRecordDecl *RD) override {
    S.ForceDeclarationOfImplicitMembers(RD);
  }

  QualType Substitute(TypeAliasTemplateDecl *TD,
                      ArrayRef<TemplateArgument> TArgs,
                      SourceLocation InstantiateLoc) override {
    TemplateArgumentListInfo TAListInfo;
    populateTemplateArgumentListInfo(TAListInfo, TArgs, InstantiateLoc);

    // TODO(P2996): Calling 'substitute' should substitute without
    // instantiation. Should a lighter weight call be used?
    TemplateName TName(TD);
    return S.CheckTemplateIdType(TName, InstantiateLoc, TAListInfo);
  }

  FunctionDecl *Substitute(FunctionTemplateDecl *TD,
                           ArrayRef<TemplateArgument> TArgs,
                           SourceLocation InstantiateLoc) override {
    void *InsertPos;
    FunctionDecl *Spec = TD->findSpecialization(TArgs, InsertPos);
    if (!Spec) {
      auto *TArgsCopy = TemplateArgumentList::CreateCopy(S.Context, TArgs);

      // TODO(P2996): Calling 'substitute' should substitute without
      // instantiation. Should a lighter weight call be used?
      Spec = S.InstantiateFunctionDeclaration(TD, TArgsCopy, InstantiateLoc);

      // Only instantiate the body if the signature has an undeduced type.
      if (Spec->getType()->isUndeducedType())
        S.InstantiateFunctionDefinition(InstantiateLoc, Spec);
    }
    return Spec;
  }

  VarDecl *Substitute(VarTemplateDecl *TD, ArrayRef<TemplateArgument> TArgs,
                      SourceLocation InstantiateLoc) override {
    void *InsertPos;
    VarDecl *Spec = TD->findSpecialization(TArgs, InsertPos);
    if (!Spec) {
      TemplateArgumentListInfo TAListInfo;
      populateTemplateArgumentListInfo(TAListInfo, TArgs, InstantiateLoc);

      DeclResult Result = S.CheckVarTemplateId(TD, InstantiateLoc,
                                               InstantiateLoc, TAListInfo);
      if (Result.isInvalid())
        return nullptr;
      Spec = cast<VarTemplateSpecializationDecl>(Result.get());

      if (!Spec->getTemplateSpecializationKind())
        Spec->setTemplateSpecializationKind(TSK_ImplicitInstantiation);
    }
    return Spec;
  }

  Expr *Substitute(ConceptDecl *TD, ArrayRef<TemplateArgument> TArgs,
                   SourceLocation InstantiateLoc) override {
    TemplateArgumentListInfo TAListInfo;
    populateTemplateArgumentListInfo(TAListInfo, TArgs, InstantiateLoc);

    CXXScopeSpec SS;
    DeclarationNameInfo DNI(TD->getDeclName(), InstantiateLoc);

    ExprResult Result = S.CheckConceptTemplateId(SS, InstantiateLoc, DNI, TD,
                                                 TD, &TAListInfo);
    return Result.get();
  }

  Expr *
  SynthesizeDirectMemberAccess(Expr *Obj, CXXReflectExpr *Mem,
                               SourceLocation PlaceholderLoc) override {
    SpliceResult SR = S.BuildSpliceSpecifier(PlaceholderLoc, Mem,
                                             PlaceholderLoc, nullptr);
    if (SR.isInvalid())
      return nullptr;
    SpliceSpecifier *SS = SR.get();

    ExprResult SpliceExpr = S.BuildReflectionSpliceExpr(SourceLocation(), SS,
                                                        true);
    if (SpliceExpr.isInvalid())
      return nullptr;

    tok::TokenKind TK = Obj->getType()->isPointerType() ? tok::arrow :
                                                          tok::period;
    ExprResult Result = S.ActOnMemberAccessExpr(
        S.getCurScope(), Obj, Obj->getExprLoc(), TK,
        cast<CXXSpliceExpr>(SpliceExpr.get()));
    return Result.get();
  }

  FunctionDecl *DeduceSpecialization(FunctionTemplateDecl *TD,
                                     ArrayRef<TemplateArgument> TArgs,
                                     ArrayRef<Expr *> Args,
                                     SourceLocation InstantiateLoc) override {
    TemplateArgumentListInfo TAListInfo;
    populateTemplateArgumentListInfo(TAListInfo, TArgs, InstantiateLoc);

    sema::TemplateDeductionInfo DeductionInfo(InstantiateLoc,
                                              TD->getTemplateDepth());

    FunctionDecl *Spec;
    TemplateDeductionResult Result = S.DeduceTemplateArguments(
          TD, &TAListInfo, Args, Spec, DeductionInfo, false, true, false,
          QualType{}, Expr::Classification(),
          [](ArrayRef<QualType>) { return false; });
    if (Result != TemplateDeductionResult::Success)
      return nullptr;

    return Spec;
  }

  Expr *SynthesizeCallExpr(Expr *Fn, MutableArrayRef<Expr *> Args) override {
    EnterExpressionEvaluationContext Ctx(
        S, Sema::ExpressionEvaluationContext::ConstantEvaluated);

    SourceRange Range(Fn->getExprLoc(),
                      Args.size() > 0 ? Args.back()->getEndLoc() :
                                        Fn->getEndLoc());
    if (auto *DRE = dyn_cast<DeclRefExpr>(Fn))
      if (auto *Ctor = dyn_cast<CXXConstructorDecl>(DRE->getDecl())) {
        QualType ClsTy(Ctor->getParent()->getTypeForDecl(), 0);
        ExprResult Result = S.BuildCXXConstructExpr(
              Fn->getExprLoc(), ClsTy, Ctor, false, Args, false, false, false,
              false, CXXConstructionKind::Complete, Range);

        return Result.get();
      }

    ExprResult Result = S.ActOnCallExpr(S.getCurScope(), Fn, Fn->getExprLoc(),
                                        Args, Range.getEnd(), nullptr);
    return Result.get();
  }

  CXXRecordDecl *DefineAggregate(CXXRecordDecl *IncompleteDecl,
                                 ArrayRef<TagDataMemberSpec *> MemberSpecs,
                                 Decl *ContainingDecl,
                                 SourceLocation DefinitionLoc) override {
    class RestoreDeclContextTy {
      Sema &S;
      DeclContext *DC;
    public:
      RestoreDeclContextTy(Sema &S) : S(S), DC(S.CurContext) {}
      ~RestoreDeclContextTy() { S.CurContext = DC; }
    } RestoreDC(S);
    S.CurContext = IncompleteDecl->getDeclContext();

    Scope ClsScope(S.getCurScope(), Scope::ClassScope | Scope::DeclScope,
                   S.Diags);
    ClsScope.setEntity(IncompleteDecl->getDeclContext());

    DeclResult NewDeclResult;
    SmallVector<TemplateParameterList *, 1> MTP;
    {
      unsigned TypeSpec = TST_struct;
      if (IncompleteDecl->getTagKind() == TagTypeKind::Class)
        TypeSpec = TST_class;
      else if (IncompleteDecl->getTagKind() == TagTypeKind::Union)
        TypeSpec = TST_union;

      if (auto *CTSD = dyn_cast<ClassTemplateSpecializationDecl>(
                                                              IncompleteDecl)) {
        TemplateName TName(CTSD->getSpecializedTemplate());
        ParsedTemplateTy ParsedTemplate = ParsedTemplateTy::make(TName);

        SmallVector<TemplateArgument, 4> TArgs;
        {
          for (const TemplateArgument &Arg : CTSD->getTemplateArgs().asArray())
            if (Arg.getKind() == TemplateArgument::Pack)
              for (const TemplateArgument &TA : Arg.getPackAsArray())
                TArgs.push_back(TA);
            else
              TArgs.push_back(Arg);
        }

        CXXScopeSpec SS;
        SmallVector<ParsedTemplateArgument, 4> ParsedTArgs;
        for (const TemplateArgument &TArg : TArgs) {
          switch (TArg.getKind()) {
          case TemplateArgument::Type:
            ParsedTArgs.emplace_back(ParsedTemplateArgument::Type,
                                    TArg.getAsType().getAsOpaquePtr(),
                                    SourceLocation());
            break;
          case TemplateArgument::Integral: {
            IntegerLiteral *IL = IntegerLiteral::Create(S.Context,
                                                        TArg.getAsIntegral(),
                                                        TArg.getIntegralType(),
                                                        DefinitionLoc);
            ParsedTArgs.emplace_back(ParsedTemplateArgument::NonType, IL,
                                    SourceLocation());
            break;
          }
          case TemplateArgument::Template: {
            ParsedTemplateTy P = ParsedTemplateTy::make(TArg.getAsTemplate());
            ParsedTArgs.emplace_back(SS, P, SourceLocation());
            break;
          }
          // TODO(P2996): Handle other kinds of TemplateArgument
          // (e.g., structural).
          default:
            llvm_unreachable("unimplemented");
          }
        }

        SmallVector<TemplateIdAnnotation *, 1> CleanupList;
        TemplateIdAnnotation *TAnnot = TemplateIdAnnotation::Create(
              SourceLocation{}, SourceLocation{},
              IncompleteDecl->getIdentifier(), OO_None, ParsedTemplate,
              TNK_Type_template, SourceLocation{}, SourceLocation{},
              ParsedTArgs, false, CleanupList);

        MTP.push_back(
                S.ActOnTemplateParameterList(0, SourceLocation{},
                                             SourceLocation{}, SourceLocation{},
                                             std::nullopt, SourceLocation{},
                                             nullptr));

        NewDeclResult = S.ActOnClassTemplateSpecialization(
                &ClsScope, TypeSpec, TagUseKind::Definition, DefinitionLoc,
                SourceLocation{}, SS, *TAnnot, ParsedAttributesView::none(),
                MTP, nullptr);

        MTP.clear();
        for (auto *TAnnot : CleanupList)
          TAnnot->Destroy();
      } else {
        // Inject the tag declaration that is to be completed into
        // the current scope. This is needed to ensure that the created Decl is
        // constructed as a redeclaration of the provided incomplete Decl.
        //
        // A more robust design might allow 'ActOnTag' to take a 'PrevDecl' as
        // an input, rather than require that it be found by name lookup.
        Scope *Sc = S.getCurScope();
        for (; Sc; Sc = Sc->getParent())
          if (Sc->isDeclScope(IncompleteDecl))
            break;
        if (!Sc)
          Sc = S.getCurScope();
        S.IdResolver.AddDecl(IncompleteDecl);
        Sc->AddDecl(IncompleteDecl);

        // Create the new tag in the current scope.
        CXXScopeSpec SS;
        TypeResult TR;
        bool OwnedDecl = true, IsDependent = false;

        NewDeclResult = S.ActOnTag(
                Sc, TypeSpec, TagUseKind::Definition,
                DefinitionLoc, SS, IncompleteDecl->getIdentifier(),
                IncompleteDecl->getBeginLoc(), ParsedAttributesView::none(),
                AS_none, SourceLocation{}, MTP, OwnedDecl, IsDependent,
                SourceLocation{}, false, TR, false, false, Sema::OOK_Outside,
                nullptr);

        // The new tag -should- declare the same entity as the original tag.
        assert((NewDeclResult.isInvalid() ||
                declaresSameEntity(IncompleteDecl, NewDeclResult.get())) &&
               "New tag should declare same entity as original tag "
               "(scope problem?)");
      }
    }
    if (NewDeclResult.isInvalid())
      return nullptr;
    CXXRecordDecl *NewDecl = cast<CXXRecordDecl>(NewDeclResult.get());

    // Start the new definition.
    S.ActOnTagStartDefinition(&ClsScope, NewDecl);
    S.ActOnStartCXXMemberDeclarations(&ClsScope, NewDecl, SourceLocation{},
                                      false, false, SourceLocation{});

    // Derive member visibility.
    AccessSpecifier MemberAS = AS_public;

    AttributeFactory AttrFactory;
    AttributePool AttrPool(AttrFactory);

    // Iterate over member specs.
    unsigned AnonMemCtr = 0;
    for (TagDataMemberSpec *MemberSpec : MemberSpecs) {
      // Build the member declaration.
      unsigned DiagID;
      const char *PrevSpec;

      DeclSpec DS(AttrFactory);
      DS.SetStorageClassSpec(S, DeclSpec::SCS_unspecified, DefinitionLoc,
                             PrevSpec, DiagID, S.Context.getPrintingPolicy());

      ParsedType MemberTy = ParsedType::make(MemberSpec->Ty);
      DS.SetTypeSpecType(TST_typename, DefinitionLoc, PrevSpec, DiagID,
                         MemberTy, S.Context.getPrintingPolicy());

      // Create any attributes specified (i.e., alignas, no_unique_address).
      ParsedAttributesView MemberAttrs;
      if (MemberSpec->Alignment) {
        IdentifierInfo &II = S.Context.Idents.get("alignas");
        IntegerLiteral *IL = IntegerLiteral::Create(
                S.Context,
                llvm::APSInt::getUnsigned(MemberSpec->Alignment.value()),
                S.Context.getSizeType(), DefinitionLoc);

        ArgsUnion Args(IL);
        ParsedAttr::Form Form(tok::kw_alignas);

        SourceRange Range(DefinitionLoc, DefinitionLoc);
        MemberAttrs.addAtEnd(AttrPool.create(&II, Range, nullptr,
                                             SourceLocation{}, nullptr, 0,
                                             Form));
      }
      if (MemberSpec->NoUniqueAddress) {
        IdentifierInfo &II = S.Context.Idents.get("no_unique_address");

        SourceRange Range(DefinitionLoc, DefinitionLoc);
        MemberAttrs.addAtEnd(AttrPool.create(&II, Range, nullptr,
                                             SourceLocation{}, nullptr, 0,
                                             ParsedAttr::Form::CXX11()));
      }

      // Create declarator for the member.
      Declarator MemberDeclarator(DS, MemberAttrs, DeclaratorContext::Member);

      // Set the identifier, unless this is a zero-width bit-field.
      if (!MemberSpec->BitWidth || *MemberSpec->BitWidth > 0) {
        std::string MemberName = MemberSpec->Name.value_or(
              "__" + llvm::toString(llvm::APSInt::get(AnonMemCtr++), 10));
        IdentifierInfo &II = S.Context.Idents.get(MemberName);

        MemberDeclarator.SetIdentifier(&II, DefinitionLoc);
      }

      // Create an expression for bit-field width, if any.
      Expr *BitWidthCE = nullptr;
      if (MemberSpec->BitWidth) {
        BitWidthCE = IntegerLiteral::Create(
              S.Context, llvm::APSInt::getUnsigned(*MemberSpec->BitWidth),
              S.Context.getSizeType(), DefinitionLoc);
      }

      VirtSpecifiers VS;
      S.ActOnCXXMemberDeclarator(&ClsScope, MemberAS, MemberDeclarator, MTP,
                                 BitWidthCE, VS, ICIS_NoInit);
    }

    // Finish the member-specification and the class definition.
    S.ActOnFinishCXXMemberSpecification(&ClsScope, NewDecl->getBeginLoc(),
                                        NewDecl, SourceLocation{},
                                        SourceLocation{},
                                        ParsedAttributesView::none());
    S.ActOnTagFinishDefinition(&ClsScope, NewDecl, DefinitionLoc);
    S.ActOnPopScope(DefinitionLoc, &ClsScope);

    Decl *ExprCone = findInjectionCone(ContainingDecl);
    Decl *NewDeclCone = findInjectionCone(
            cast<Decl>(NewDecl->getDeclContext()));

    if (ExprCone != NewDeclCone && !declaresSameEntity(NewDecl, ExprCone)) {
      Decl *ProblemScope = isa<TranslationUnitDecl>(ExprCone) ? NewDeclCone
                                                              : ExprCone;

      // TODO(P2996): Implement diagnostic printing of 'ConstevalBlockDecl's.
      if (isa<ConstevalBlockDecl>(ContainingDecl)) {
        std::string Repr;
        llvm::raw_string_ostream ReprOut(Repr);

        SourceLocation Loc = ContainingDecl->getLocation();
        ReprOut << "'(consteval-block at "
                << Loc.printToString(S.Context.getSourceManager()) << ")'";

        S.Diag(DefinitionLoc, diag::err_injected_decl_outside_cone)
            << cast<NamedDecl>(NewDecl) << Repr
            << (isa<FunctionDecl>(ProblemScope) ? 1 : 0)
            << cast<NamedDecl>(ProblemScope);
      } else {
        S.Diag(DefinitionLoc, diag::err_injected_decl_outside_cone)
            << cast<NamedDecl>(NewDecl) << cast<NamedDecl>(ContainingDecl)
            << (isa<FunctionDecl>(ProblemScope) ? 1 : 0)
            << cast<NamedDecl>(ProblemScope);
      }
    }

    return NewDecl;
  }

  void Appertain(Decl *TargetDecl, ParsedAttr * attr,
                 SourceLocation DefinitionLoc) override
  {
    size_t nbAttr = TargetDecl->hasAttrs() ? TargetDecl->getAttrs().size() : 0;

    ParsedAttributesView view;
    view.addAtEnd(attr);
    S.ProcessDeclAttributeList(nullptr /*scope */, TargetDecl, view);

    size_t nbAttrPost = TargetDecl->hasAttrs() ? TargetDecl->getAttrs().size() : 0;
    if (++nbAttr != nbAttrPost) {
      S.Diag(DefinitionLoc, diag::p3385_sema_appertain_failed)
        << attr->getAttrName();
    }
  }

  CXX26AnnotationAttr *Annotate(Decl *TargetDecl, const APValue &Value,
                                Decl *ContainingDecl,
                                SourceLocation DefinitionLoc) override {
    Decl *ExprCone = findInjectionCone(ContainingDecl);
    Decl *TargetDeclCone = findInjectionCone(
            cast<Decl>(TargetDecl->getDeclContext()));

    if (ExprCone != TargetDeclCone &&
        !declaresSameEntity(TargetDecl, ExprCone)) {
      Decl *ProblemScope = isa<TranslationUnitDecl>(ExprCone) ? TargetDeclCone
                                                              : ExprCone;

      // TODO(P2996): Implement diagnostic printing of 'ConstevalBlockDecl's.
      if (isa<ConstevalBlockDecl>(ContainingDecl)) {
        std::string Repr;
        llvm::raw_string_ostream ReprOut(Repr);

        SourceLocation Loc = ContainingDecl->getLocation();
        ReprOut << "'(consteval-block at "
                << Loc.printToString(S.Context.getSourceManager()) << ")'";

        S.Diag(DefinitionLoc, diag::err_injected_decl_outside_cone)
            << cast<NamedDecl>(TargetDecl) << Repr
            << (isa<FunctionDecl>(ProblemScope) ? 1 : 0)
            << cast<NamedDecl>(ProblemScope);
      } else {
        S.Diag(DefinitionLoc, diag::err_injected_decl_outside_cone)
            << cast<NamedDecl>(TargetDecl) << cast<NamedDecl>(ContainingDecl)
            << (isa<FunctionDecl>(ProblemScope) ? 1 : 0)
            << cast<NamedDecl>(ProblemScope);
      }
    }

    CXX26AnnotationAttr *Annot;
    {
      Expr *OVE = new (S.Context) OpaqueValueExpr(
            DefinitionLoc,
            Value.getTypeOfReflectedResult(S.Context),
            VK_PRValue);
      Expr *CE = ConstantExpr::Create(S.Context, OVE,
                                      Value.getReflectedValue());

      AttributeFactory AttrFactory;
      ParsedAttributes ParsedAttrs(AttrFactory);

      SourceRange Range(DefinitionLoc, DefinitionLoc);
      IdentifierInfo &II = S.Context.Idents.get("__annotation_placeholder");
      AttributeCommonInfo *ACI = ParsedAttrs.addNew(
            &II, Range, nullptr, DefinitionLoc, nullptr, 0,
            ParsedAttr::Form::Annotation());

      Annot = CXX26AnnotationAttr::Create(S.Context, CE, *ACI);
      Annot->setValue(Value.getReflectedValue());
      Annot->setEqLoc(DefinitionLoc);
    }

    TargetDecl->addAttr(Annot);
    return Annot;
  }

  AttributeCommonInfo *SynthesizeAnnotation(Expr *CE,
                                            SourceLocation Loc) override {
    AttributeFactory AttrFactory;
    ParsedAttributes ParsedAttrs(AttrFactory);

    SourceRange Range(Loc, Loc);
    IdentifierInfo &II = S.Context.Idents.get("__annotation_placeholder");
    return ParsedAttrs.addNew(&II, Range, nullptr, Loc, nullptr, 0,
                              ParsedAttr::Form::Annotation());
  }
};
}  // anonymous namespace

Sema::ConstevalOnlyRecorder::ConstevalOnlyRecorder(Sema &S)
    : S(S), TheExpr(nullptr) { }

Sema::ConstevalOnlyRecorder::~ConstevalOnlyRecorder() {
  assert(S.ExprEvalContexts.size() > 0 && "no evaluation context?");

  if (!TheExpr)
    return;

  if (!S.isUnevaluatedContext() && !S.isImmediateFunctionContext() &&
      !S.isConstantEvaluatedContext() &&
      !S.isCheckingDefaultArgumentOrInitializer() &&
      !S.RebuildingImmediateInvocation && !TheExpr->isValueDependent())
    S.ExprEvalContexts.back().ConstevalOnly.insert(TheExpr);
}

ExprResult Sema::ConstevalOnlyRecorder::RecordAndReturn(ExprResult Res) {
  assert(!TheExpr && "TheExpr was already set");
  if (Res.isInvalid())
    return Res;

  Expr *E = Res.get();
  assert(E->getType()->isConstevalOnly() &&
         "expected an expression of consteval-only type");
  TheExpr = E;
  return Res;
}

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
    auto *Splice = const_cast<SpliceSpecifier *>(
        Id.TemplateId->Template.get().getAsDependentTemplateName()
                                     ->getSpliceSpecifier());

    ExprResult Result;
    if (TArgs) {
      const ASTTemplateArgumentListInfo *ASTTArgs =
            ASTTemplateArgumentListInfo::Create(Context, *TArgs);
      SpliceResult SR = BuildSpliceSpecifier(Splice->getLSpliceLoc(),
                                             Splice->getOperand(),
                                             Splice->getRSpliceLoc(),
                                             ASTTArgs);
      assert(!SR.isInvalid());
      Result = BuildReflectionSpliceExpr(TemplateKWLoc, SR.get(), false);
    } else {
      Result = BuildReflectionSpliceExpr(TemplateKWLoc, Splice, false);
    }
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
    Diag(Id.StartLocation, diag::err_reflect_overload_set)
        << Id.getSourceRange();
    return ExprError();
  }

  NamedDecl *ND = Found.getRepresentativeDecl();

  if (auto *USD = dyn_cast<UsingShadowDecl>(ND)) {
    if (getLangOpts().EntityProxyReflection)
      return BuildCXXReflectExpr(OpLoc, NameInfo.getBeginLoc(), USD);
    else do
      ND = cast<UsingShadowDecl>(ND)->getTargetDecl();
    while (isa<UsingShadowDecl>(ND));
  }

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

  if (auto *VD = dyn_cast<VarDecl>(ND);
      VD && CheckReflectVar(*this, VD, Id.getSourceRange()))
    return ExprError();

  if (isa<VarDecl, BindingDecl, FunctionDecl, FieldDecl, EnumConstantDecl,
          NonTypeTemplateParmDecl, UnresolvedUsingValueDecl>(ND))
    return BuildCXXReflectExpr(OpLoc, NameInfo.getBeginLoc(), ND);

  if (auto *TD = dyn_cast<TemplateDecl>(ND))
    return BuildCXXReflectExpr(OpLoc, NameInfo.getBeginLoc(),
                               TemplateName(TD));

  if (auto *IFD = dyn_cast<IndirectFieldDecl>(ND))
    return BuildCXXReflectExpr(OpLoc, NameInfo.getBeginLoc(),
                               IFD->getAnonField());

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

ExprResult Sema::ActOnCXXReflectExpr(SourceLocation OperatorLoc,
                                     ParsedAttr *A) {
  return BuildCXXReflectExpr(OperatorLoc, A);
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
  const auto &ImplIt = getMetafunctionCb(FnID);

  // Return the CXXMetafunctionExpr representation.
  return BuildCXXMetafunctionExpr(KwLoc, LParenLoc, RParenLoc,
                                  FnID, ImplIt, Args);
}

const CXXMetafunctionExpr::ImplFn &Sema::getMetafunctionCb(unsigned FnID) {
  auto ImplIt = MetafunctionImplCbs.find(FnID);
  if (ImplIt == MetafunctionImplCbs.end()) {
    const Metafunction *Metafn;
    Metafunction::Lookup(FnID, Metafn);

    assert(Metafn);
    auto MetafnImpl = std::make_unique<CXXMetafunctionExpr::ImplFn>(
        std::function(
            [this, Metafn](APValue &Result,
                           CXXMetafunctionExpr::EvaluateFn EvalFn,
                           CXXMetafunctionExpr::DiagnoseFn DiagFn,
                           bool AllowInjection, QualType ResultTy,
                           SourceRange Range, ArrayRef<Expr *> Args,
                           Decl *ContainingDecl) -> bool {
              MetaActionsImpl Actions(*this);
              return Metafn->evaluate(Result, Context, Actions, EvalFn, DiagFn,
                                      AllowInjection, ResultTy, Range, Args,
                                      ContainingDecl);
            }));
    ImplIt = MetafunctionImplCbs.try_emplace(FnID, std::move(MetafnImpl)).first;
  }

  return *ImplIt->second;
}

SpliceResult Sema::ActOnSpliceSpecifier(SourceLocation LSpliceLoc,
                                        Expr *Operand,
                                        SourceLocation RSpliceLoc) {
  return BuildSpliceSpecifier(LSpliceLoc, Operand, RSpliceLoc, nullptr);
}

SpliceResult Sema::ActOnSpliceSpecifier(SourceLocation LSpliceLoc,
                                        Expr *Operand,
                                        SourceLocation RSpliceLoc,
                                        SourceLocation LAngleLoc,
                                        ASTTemplateArgsPtr TArgsPtr,
                                        SourceLocation RAngleLoc) {
  TemplateArgumentListInfo TAListInfo;
  translateTemplateArguments(TArgsPtr, TAListInfo);

  TAListInfo.setLAngleLoc(LAngleLoc);
  TAListInfo.setRAngleLoc(RAngleLoc);

  const ASTTemplateArgumentListInfo *ASTTArgs =
      ASTTemplateArgumentListInfo::Create(Context, TAListInfo);

  return BuildSpliceSpecifier(LSpliceLoc, Operand, RSpliceLoc, ASTTArgs);
}

ExprResult Sema::ActOnCXXSpliceExpression(SourceLocation TemplateKWLoc,
                                          SpliceSpecifier *Splice,
                                          bool AllowMemberReference) {
  return BuildReflectionSpliceExpr(TemplateKWLoc, Splice, AllowMemberReference);
}

TypeResult Sema::ActOnCXXSpliceTypeSpecifier(SourceLocation TypenameLoc,
                                             SpliceSpecifier *Splice,
                                             bool Complain) {
  TypeLocBuilder TLB;
  QualType SpliceTy = BuildReflectionSpliceTypeLoc(TLB, TypenameLoc, Splice,
                                                   Complain);
  if (SpliceTy.isNull())
    return TypeError();
  return CreateParsedType(SpliceTy, TLB.getTypeSourceInfo(Context, SpliceTy));
}

DeclResult Sema::ActOnCXXSpliceExpectingNamespace(SpliceSpecifier *Splice) {
  assert(!Splice->isSpecialization() &&
         "splice-specialization-specifier cannot represent a namespace");

  return BuildReflectionSpliceNamespace(Splice);
}

ParsedTemplateArgument
Sema::ActOnSpliceTemplateArgument(SpliceSpecifier *Splice) {
  assert(!Splice->isSpecialization() &&
         "splice-template-argument cannot be a specialization");

  if (Splice->getDependence() != SpliceSpecifierDependence::None) {
    return ParsedTemplateArgument(ParsedTemplateArgument::Splice, Splice,
                                  Splice->getBeginLoc());
  }

  SmallVector<PartialDiagnosticAt, 4> Diags;
  Expr::EvalResult ER;
  ER.Diag = &Diags;
  if (!Splice->getOperand()->EvaluateAsRValue(ER, Context, true)) {
    return ParsedTemplateArgument();
  }
  assert(ER.Val.getKind() == APValue::Reflection);

  APValue Refl = MaybeUnproxy(Context, ER.Val);
  switch (Refl.getReflectionKind()) {
  case ReflectionKind::Type:
    return ParsedTemplateArgument(ParsedTemplateArgument::Type,
                                  const_cast<void *>(
                                      Refl.getOpaqueReflectionData()),
                                  Splice->getBeginLoc());
  case ReflectionKind::Object: {
    QualType ResultTy = Refl.getTypeOfReflectedResult(Context);
    Expr *OVE = new (Context) OpaqueValueExpr(Splice->getBeginLoc(), ResultTy,
                                              VK_LValue);
    Expr *CE = ConstantExpr::Create(Context, OVE, Refl.getReflectedObject());
    return ParsedTemplateArgument(ParsedTemplateArgument::NonType, CE,
                                  Splice->getBeginLoc());
  }
  case ReflectionKind::Value: {
    QualType ResultTy = Refl.getTypeOfReflectedResult(Context);
    Expr *OVE = new (Context) OpaqueValueExpr(Splice->getBeginLoc(), ResultTy,
                                              VK_PRValue);
    Expr *CE = ConstantExpr::Create(Context, OVE, Refl.getReflectedValue());
    return ParsedTemplateArgument(ParsedTemplateArgument::NonType, CE,
                                  Splice->getBeginLoc());
  }
  case ReflectionKind::Template: {
    TemplateName TName = Refl.getReflectedTemplate();
    return ParsedTemplateArgument(ParsedTemplateArgument::Template,
                                  TName.getAsTemplateDecl(),
                                  Splice->getBeginLoc());
  }
  case ReflectionKind::Declaration: {
    Expr *E = CreateRefToDecl(*this, cast<ValueDecl>(Refl.getReflectedDecl()),
                              Splice->getBeginLoc());
    return ParsedTemplateArgument(ParsedTemplateArgument::NonType, E,
                                  E->getBeginLoc());
  }
  case ReflectionKind::Null:
    Diag(Splice->getBeginLoc(), diag::err_unsupported_splice_kind)
      << "null reflections" << 0 << 0;
    break;
  case ReflectionKind::Namespace:
    Diag(Splice->getBeginLoc(), diag::err_unsupported_splice_kind)
      << "namespaces" << 0 << 0;
    break;
  case ReflectionKind::BaseSpecifier:
    Diag(Splice->getBeginLoc(), diag::err_unsupported_splice_kind)
      << "base specifiers" << 0 << 0;
    break;
  case ReflectionKind::DataMemberSpec:
    Diag(Splice->getBeginLoc(), diag::err_unsupported_splice_kind)
      << "data member specs" << 0 << 0;
    break;
  case ReflectionKind::Attribute:
    Diag(Splice->getBeginLoc(), diag::err_unsupported_splice_kind)
    << "attribute" << 0 << 0;
    break;
  case ReflectionKind::Annotation:
    Diag(Splice->getBeginLoc(), diag::err_unsupported_splice_kind)
      << "annotations" << 0 << 0;
    break;
  case ReflectionKind::EntityProxy:
    llvm_unreachable("proxies should already have been unwrapped");
  }
  return ParsedTemplateArgument();
}

bool Sema::ActOnCXXSpliceScopeSpecifier(CXXScopeSpec &SS,
                                        SourceLocation TemplateKWLoc,
                                        SpliceSpecifier *Splice,
                                        SourceLocation ColonColonLoc) {
  assert(SS.isEmpty() && "splice must be leading component of NNS");

  auto *DC = TryFindDeclContextOf(Splice);
  if (Splice->getDependence() == SpliceSpecifierDependence::None && !DC)
    return true;

  SS.MakeSpliceScopeSpecifier(Context, TemplateKWLoc, Splice, ColonColonLoc);
  return false;
}

Decl *Sema::ActOnConstevalBlockDeclaration(SourceLocation ConstevalLoc,
                                           Expr *EvaluatingExpr) {
  return BuildConstevalBlockDeclaration(ConstevalLoc, EvaluatingExpr);
}

ExprResult Sema::BuildCXXReflectExpr(SourceLocation OperatorLoc,
                                     SourceLocation OperandLoc, QualType T) {
  if (auto *UT = dyn_cast<UsingType>(T)) {
    if (Context.getLangOpts().EntityProxyReflection)
      return BuildCXXReflectExpr(OperatorLoc, OperandLoc, UT->getFoundDecl());
    else
      T = UT->getUnderlyingType();
  }

  APValue RV(ReflectionKind::Type, T.getAsOpaquePtr());
  return CXXReflectExpr::Create(Context, OperatorLoc, OperandLoc, RV);
}

// TODO(P2996): Capture whole SourceRange of declaration naming.
ExprResult Sema::BuildCXXReflectExpr(SourceLocation OperatorLoc,
                                     SourceLocation OperandLoc, Decl *D) {
  // This case can happen after transforming a dependent reflection naming a
  // using-declarator.
  if (auto *UD = dyn_cast<UsingDecl>(D)) {
    if (UD->shadow_size() > 1) {
      Diag(OperandLoc, diag::err_reflect_overload_set);
      return ExprError();
    }
    D = *UD->shadow_begin();
  }

  D = D->getCanonicalDecl();

  ReflectionKind RK = ReflectionKind::Declaration;
  if (isa<TranslationUnitDecl, NamespaceDecl, NamespaceAliasDecl>(D))
    RK = ReflectionKind::Namespace;
  else if (isa<UsingShadowDecl>(D))
    RK = ReflectionKind::EntityProxy;

  APValue RV(RK, D);
  if (!getLangOpts().EntityProxyReflection)
    RV = MaybeUnproxy(Context, RV);
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

  APValue RV(ReflectionKind::Template, Template.getAsVoidPointer());
  return CXXReflectExpr::Create(Context, OperatorLoc,
                                SourceRange(OperandLoc, OperandLoc), RV);
}

ExprResult Sema::BuildCXXReflectExpr(SourceLocation OperatorLoc, Expr *E) {
  // Don't try to evaluate now if it's a value-dependent subexpression.
  if (E->isValueDependent()) {
    if (auto *DRE = dyn_cast<DeclRefExpr>(E);
        DRE && isa<NonTypeTemplateParmDecl>(DRE->getDecl())) {
      Diag(E->getExprLoc(), diag::err_reflect_nttp) << E->getSourceRange();
      return ExprError();
    }
    return CXXReflectExpr::Create(Context, OperatorLoc, E);
  }

  // Check if this is a reference to a declared entity.
  if (auto *DRE = dyn_cast<DeclRefExpr>(E)) {
    Decl *D = DRE->getDecl();
    if (auto *F = DRE->getFoundDecl(); isa<UsingShadowDecl>(F))
      D = F;

    return BuildCXXReflectExpr(OperatorLoc, DRE->getExprLoc(), D);
  }

  // Special case for '^[:splice:]'.
  if (auto *SE = dyn_cast<CXXSpliceExpr>(E))
    return BuildCXXReflectExpr(OperatorLoc, SE);

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

  APValue RV = ER.Val.Lift(E->getType());
  return CXXReflectExpr::Create(Context, OperatorLoc, E->getSourceRange(), RV);
}

ExprResult Sema::BuildCXXReflectExpr(SourceLocation OperatorLoc,
                                     CXXSpliceExpr *E) {
  assert(!E->isValueDependent());

  Expr *ToEval = E->getModel();
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
                                ER.Val);
}

ExprResult Sema::BuildCXXReflectExpr(SourceLocation OperatorLoc,
                                     ParsedAttr *A) {
  return CXXReflectExpr::Create(
      Context, OperatorLoc, A->getRange(),
      APValue{ReflectionKind::Attribute, static_cast<void *>(A)});
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

      if (!ER.Val.isReflectedType()) {
        Diag(TyRefl->getExprLoc(), diag::err_unexpected_reflection_kind) << 0;
        return true;
      }

      Result = ER.Val.getReflectedType().getCanonicalType();
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

ExprResult Sema::BuildExplDependentCallExpr(CallExpr *SubExpr,
                                            unsigned TemplateDepth) {
  return ExplDependentCallExpr::Create(Context, SubExpr, TemplateDepth);
}

SpliceResult
Sema::BuildSpliceSpecifier(SourceLocation LSpliceLoc, Expr *Operand,
                           SourceLocation RSpliceLoc,
                           const ASTTemplateArgumentListInfo *TArgs) {
  ExprResult Result = DefaultLvalueConversion(Operand);
  if (Result.isInvalid())
    return SpliceError();
  Operand = Result.get();

  auto Dep = toSpliceSpecifierDependence(Operand->getDependence());
  if (Dep == SpliceSpecifierDependence::None &&
      Operand->getType() != Context.MetaInfoTy) {
    Result = PerformImplicitConversion(Operand, Context.MetaInfoTy,
                                       AssignmentAction::Converting, false);
    if (Result.isInvalid())
      return SpliceError();
    Operand = Result.get();
  }
  return SpliceSpecifier::Create(Context, LSpliceLoc, Operand, RSpliceLoc,
                                 TArgs);
}

QualType Sema::BuildReflectionSpliceType(SourceLocation TypenameKWLoc,
                                         SpliceSpecifier *Splice,
                                         bool Complain) {
  if (Splice->getDependence() != SpliceSpecifierDependence::None) {
    return Context.getReflectionSpliceType(TypenameKWLoc, Splice,
                                           Context.DependentTy);
  }

  SmallVector<PartialDiagnosticAt, 4> Diags;
  Expr::EvalResult ER;
  ER.Diag = &Diags;

  if (!Splice->getOperand()->EvaluateAsRValue(ER, Context, true)) {
    Diag(Splice->getBeginLoc(), diag::err_splice_operand_not_constexpr);
    for (PartialDiagnosticAt PD : Diags)
      Diag(PD.first, PD.second);
    return QualType();
  }

  if (!ER.Val.isReflection()) {
    Diag(Splice->getBeginLoc(), diag::err_splice_operand_not_reflection);
    return QualType();
  }
  APValue Refl = MaybeUnproxy(Context, ER.Val);

  QualType ReflectedTy;
  if (Refl.isReflectedTemplate() &&
      !isa<ConceptDecl>(Refl.getReflectedTemplate().getAsTemplateDecl())) {
    if (Splice->isSpecialization()) {
      TemplateArgumentListInfo TAListInfo;
      for (const auto &TArg : Splice->getTemplateArgs()->arguments())
        TAListInfo.addArgument(TArg);
      ReflectedTy =
          CheckTemplateIdType(Refl.getReflectedTemplate(),
                              Splice->getBeginLoc(), TAListInfo);
      if (ReflectedTy.isNull()) {
        return QualType();
      }
    } else {
      ReflectedTy =
          Context.getDeducedTemplateSpecializationType(
              Refl.getReflectedTemplate(), QualType(), false);
    }
  } else if (!Refl.isReflectedType()) {
    if (Complain)
      Diag(Splice->getBeginLoc(),
           diag::err_unexpected_reflection_kind_in_splice) << 0;
    return QualType();
  } else {
    ReflectedTy = Refl.getReflectedType();
  }

  // Check if the type refers to a substituted but uninstantiated template.
  if (auto *TT = dyn_cast<TagType>(ReflectedTy)) {
    if (auto *CTD = dyn_cast<ClassTemplateSpecializationDecl>(TT->getDecl());
        CTD && CTD->getSpecializationKind() == TSK_Undeclared) {
      TemplateName TName(CTD->getSpecializedTemplate());

      const TemplateArgumentList &TAList =
              CTD->getTemplateInstantiationArgs();
      TemplateArgumentListInfo TAListInfo(
              addLocToTemplateArgs(*this, TAList.asArray(),
                                   Splice->getBeginLoc()));

      ReflectedTy = CheckTemplateIdType(TName, Splice->getBeginLoc(),
                                        TAListInfo);
      if (ReflectedTy.isNull())
        return QualType();
    }
  }

  return Context.getReflectionSpliceType(TypenameKWLoc, Splice, ReflectedTy);
}

QualType Sema::BuildReflectionSpliceTypeLoc(TypeLocBuilder &TLB,
                                            SourceLocation TypenameKWLoc,
                                            SpliceSpecifier *Splice,
                                            bool Complain) {
  QualType SpliceTy = BuildReflectionSpliceType(TypenameKWLoc, Splice,
                                                Complain);
  if (SpliceTy.isNull())
    return QualType();
  SourceLocation Loc =
      cast<ReflectionSpliceType>(SpliceTy)->getSplice()->getBeginLoc();

  if (isa<TemplateSpecializationType>(SpliceTy)) {
    auto TL = TLB.push<TemplateSpecializationTypeLoc>(SpliceTy);
    TL.setTemplateNameLoc(Loc);
    return SpliceTy;
  } else if (isa<DeducedTemplateSpecializationType>(SpliceTy)) {
    auto TL = TLB.push<DeducedTemplateSpecializationTypeLoc>(SpliceTy);
    TL.setTemplateNameLoc(Loc);
    return SpliceTy;
  }

  TLB.push<ReflectionSpliceTypeLoc>(SpliceTy);

  return SpliceTy;
}

ExprResult Sema::BuildReflectionSpliceExpr(SourceLocation TemplateKWLoc,
                                           SpliceSpecifier *Splice,
                                           bool AllowMemberReference) {
  if (Splice->getDependence() == SpliceSpecifierDependence::None) {
    SmallVector<PartialDiagnosticAt, 4> Diags;
    Expr::EvalResult ER;
    ER.Diag = &Diags;

    if (!Splice->getOperand()->EvaluateAsConstantExpr(ER, Context)) {
      Diag(Splice->getBeginLoc(), diag::err_splice_operand_not_constexpr);
      for (PartialDiagnosticAt PD : Diags)
        Diag(PD.first, PD.second);
      return ExprError();
    }

    if (!ER.Val.isReflection()) {
      Diag(Splice->getBeginLoc(), diag::err_splice_operand_not_reflection);
      return ExprError();
    }
    APValue Refl = MaybeUnproxy(Context, ER.Val);

    bool RequireTemplate = TemplateKWLoc.isValid() ||
                           Splice->isSpecialization();
    if (RequireTemplate && !Refl.isReflectedTemplate()) {
      Diag(Splice->getBeginLoc(),
           diag::err_unexpected_reflection_kind_in_splice) << 3;
      return ExprError();
    }

    Expr *Result = nullptr;
    switch (Refl.getReflectionKind()) {
    case ReflectionKind::Declaration: {
      Decl *TheDecl = Refl.getReflectedDecl();

      // Class members may not be implicitly referenced through a splice.
      if (!AllowMemberReference &&
          (isa<FieldDecl>(TheDecl) ||
           (isa<CXXMethodDecl>(TheDecl) &&
            dyn_cast<CXXMethodDecl>(TheDecl)->isInstance()))) {
        Diag(Splice->getBeginLoc(),
             diag::err_dependent_splice_implicit_member_reference)
          << Splice->getSourceRange();
        Diag(Splice->getBeginLoc(),
             diag::note_dependent_splice_explicit_this_may_fix)
          << Splice->getSourceRange();
        return ExprError();
      }

      if (auto *FD = dyn_cast<FieldDecl>(TheDecl);
          FD && FD->isUnnamedBitField()) {
        Diag(Splice->getBeginLoc(), diag::err_splice_unnamed_bit_field);
        return ExprError();
      }

      if (auto *VD = dyn_cast<VarDecl>(TheDecl);
          VD && CheckSpliceVar(*this, VD, Splice->getSourceRange()))
        return ExprError();

      // Create a new DeclRefExpr, since the operand of the reflect expression
      // was parsed in an unevaluated context (but a splice expression is not
      // necessarily, and frequently not, in such a context).
      Result = CreateRefToDecl(*this, cast<ValueDecl>(TheDecl),
                               Splice->getBeginLoc());
      MarkDeclRefReferenced(cast<DeclRefExpr>(Result), nullptr);
      Result = CXXSpliceExpr::Create(Context, Result->getValueKind(),
                                     TemplateKWLoc, Splice, Result,
                                     AllowMemberReference);
      break;
    }
    case ReflectionKind::Object: {
      QualType QT = Refl.getTypeOfReflectedResult(Context);
      Expr *OVE = new (Context) OpaqueValueExpr(Splice->getBeginLoc(), QT,
                                                VK_LValue);
      Expr *CE = ConstantExpr::Create(Context, OVE,
                                      Refl.getReflectedObject());

      Result = CXXSpliceExpr::Create(Context, VK_LValue, TemplateKWLoc,
                                     Splice, CE, AllowMemberReference);
      break;
    }
    case ReflectionKind::Value: {
      QualType QT = Refl.getTypeOfReflectedResult(Context);
      Expr *OVE = new (Context) OpaqueValueExpr(Splice->getBeginLoc(), QT,
                                                VK_PRValue);
      Expr *CE = ConstantExpr::Create(Context, OVE, Refl.getReflectedValue());

      Result = CXXSpliceExpr::Create(Context, VK_PRValue, TemplateKWLoc,
                                     Splice, CE, AllowMemberReference);
      break;
    }
    case ReflectionKind::Template: {
      if (TemplateKWLoc.isInvalid()) {
        Diag(Splice->getBeginLoc(),
             diag::err_unexpected_reflection_kind_in_splice)
          << 1 << Splice->getSourceRange();
        return ExprError();
      }

      TemplateName TName = Refl.getReflectedTemplate();
      assert(!TName.isDependent());

      TemplateDecl *TDecl = TName.getAsTemplateDecl();
      DeclarationNameInfo DeclNameInfo(TDecl->getDeclName(),
                                       Splice->getBeginLoc());

      CXXScopeSpec ScopeSpec;
      if (auto *RD = dyn_cast<CXXRecordDecl>(TDecl->getDeclContext())) {
        TypeSourceInfo *TSI = Context.getTrivialTypeSourceInfo(
                QualType(RD->getTypeForDecl(), 0), Splice->getBeginLoc());
        ScopeSpec.Extend(Context, SourceLocation(), TSI->getTypeLoc(),
                         Splice->getBeginLoc());
      }

      // TODO(P2996): Would be nice not to have to copy these here.
      TemplateArgumentListInfo TAListInfo;
      if (Splice->isSpecialization()) {
        TAListInfo.setLAngleLoc(Splice->getLAngleLoc());
        TAListInfo.setRAngleLoc(Splice->getRAngleLoc());
        for (const auto &Arg : Splice->getTemplateArgs()->arguments())
          TAListInfo.addArgument(Arg);
      }

      if (auto *FTD = dyn_cast<FunctionTemplateDecl>(TDecl);
          FTD && Splice->isSpecialization()) {
        CheckTemplateArgumentInfo Ignored;
        DefaultArguments DefaultArgs;

        bool ConstraintFailure = false;
        if (CheckTemplateArgumentList(FTD, TemplateKWLoc, TAListInfo,
                                      DefaultArgs, true, Ignored, false,
                                      &ConstraintFailure) ||
            ConstraintFailure)
          return ExprError();
      } else if (auto *VTD = dyn_cast<VarTemplateDecl>(TDecl)) {
        ExprResult ER = CheckVarTemplateId(ScopeSpec, DeclNameInfo, VTD, VTD,
                                           Splice->getBeginLoc(), &TAListInfo);
        if (ER.isInvalid())
          return ExprError();
        Result = ER.get();
        Result = CXXSpliceExpr::Create(Context, VK_LValue, TemplateKWLoc,
                                       Splice, Result, AllowMemberReference);
        break;
      } else if (auto *CD = dyn_cast<ConceptDecl>(TDecl)) {
        ExprResult ER = CheckConceptTemplateId(ScopeSpec, SourceLocation(),
                                               DeclNameInfo, CD, CD,
                                               &TAListInfo);
        if (ER.isInvalid())
          return ExprError();
        Result = ER.get();
        Result = CXXSpliceExpr::Create(Context, VK_PRValue, TemplateKWLoc,
                                       Splice, Result, AllowMemberReference);
        break;
      } else if (isa<ClassTemplateDecl>(TDecl) ||
                 isa<TypeAliasTemplateDecl>(TDecl)) {
        Diag(Splice->getBeginLoc(),
             diag::err_unexpected_reflection_template_kind) << 1;
        return ExprError();
      }

      UnresolvedSet<1> DeclSet;
      DeclSet.addDecl(TDecl);
      Result = UnresolvedLookupExpr::Create(
          Context, nullptr, ScopeSpec.getWithLocInContext(Context),
          SourceLocation(), DeclNameInfo, false, &TAListInfo, DeclSet.begin(),
          DeclSet.end(), false, false);

      Result = CXXSpliceExpr::Create(Context, VK_LValue, TemplateKWLoc,
                                     Splice, Result, AllowMemberReference);
      break;
    }
    case ReflectionKind::Null:
    case ReflectionKind::Type:
    case ReflectionKind::Namespace:
    case ReflectionKind::BaseSpecifier:
    case ReflectionKind::DataMemberSpec:
    case ReflectionKind::Annotation:
    case ReflectionKind::Attribute:
      Diag(Splice->getBeginLoc(),
           diag::err_unexpected_reflection_kind_in_splice)
          << 1 << Splice->getSourceRange();
      return ExprError();
    case ReflectionKind::EntityProxy:
      llvm_unreachable("proxies should already have been unwrapped");
    }
    return Result;
  }
  return CXXSpliceExpr::Create(Context, VK_PRValue, TemplateKWLoc,
                               Splice, nullptr, AllowMemberReference);
}

DeclResult Sema::BuildReflectionSpliceNamespace(SpliceSpecifier *Splice) {
  if (Splice->getDependence() != SpliceSpecifierDependence::None) {
    return DependentNamespaceDecl::Create(Context, CurContext, Splice);
  }

  SmallVector<PartialDiagnosticAt, 4> Diags;
  Expr::EvalResult ER;
  ER.Diag = &Diags;

  if (!Splice->getOperand()->EvaluateAsRValue(ER, Context, true)) {
    Diag(Splice->getBeginLoc(), diag::err_splice_operand_not_constexpr);
    for (PartialDiagnosticAt PD : Diags)
      Diag(PD.first, PD.second);
    return DeclError();
  }

  if (!ER.Val.isReflection()) {
    Diag(Splice->getBeginLoc(), diag::err_splice_operand_not_reflection);
    return DeclError();
  }

  if (!ER.Val.isReflectedNamespace()) {
    Diag(Splice->getBeginLoc(), diag::err_unexpected_reflection_kind) << 2;
    return DeclError();
  } else if (isa<TranslationUnitDecl>(ER.Val.getReflectedNamespace())) {
    Diag(Splice->getBeginLoc(),
         diag::err_splice_global_scope_as_namespace);
    return DeclError();
  }

  return ER.Val.getReflectedNamespace();
}

Sema::TemplateTy Sema::BuildReflectionSpliceTemplate(SpliceSpecifier *Splice,
                                                     bool Complain) {
  if (Splice->getDependence() != SpliceSpecifierDependence::None)
    return TemplateTy::make(Context.getDependentTemplateName(Splice));

  SmallVector<PartialDiagnosticAt, 4> Diags;
  Expr::EvalResult ER;
  ER.Diag = &Diags;

  if (!Splice->getOperand()->EvaluateAsRValue(ER, Context, true)) {
    Diag(Splice->getBeginLoc(),
        diag::err_splice_operand_not_constexpr) << Splice->getOperand();
    for (PartialDiagnosticAt PD : Diags)
      Diag(PD.first, PD.second);
    return TemplateTy();
  }

  if (!ER.Val.isReflection()) {
    Diag(Splice->getBeginLoc(),
         diag::err_splice_operand_not_reflection) << Splice->getSourceRange();
    return TemplateTy();
  }

  if (!ER.Val.isReflectedTemplate()) {
    if (Complain)
      Diag(Splice->getBeginLoc(), diag::err_unexpected_reflection_kind)
          << 3 << Splice->getSourceRange();
    return TemplateTy();
  }

  return TemplateTy::make(ER.Val.getReflectedTemplate());
}

Decl *Sema::BuildConstevalBlockDeclaration(SourceLocation ConstevalLoc,
                                           Expr *EvaluatingExpr) {
  Decl *Result = ConstevalBlockDecl::Create(Context, CurContext, ConstevalLoc,
                                            EvaluatingExpr);
  CurContext->addDecl(Result);

  if (!EvaluatingExpr->isTypeDependent() &&
      !EvaluatingExpr->isValueDependent()) {
    SmallVector<PartialDiagnosticAt, 4> Diags;
    Expr::EvalResult ER;
    ER.Diag = &Diags;

    ConstantExprKind Kind = ConstantExprKind::PlainlyConstantEvaluated;
    if (!EvaluatingExpr->EvaluateAsConstantExpr(ER, Context, Kind, Result)) {
      Diag(ConstevalLoc, diag::err_consteval_block_not_constexpr);
      for (PartialDiagnosticAt PD : Diags)
        Diag(PD.first, PD.second);
    }
  }
  return Result;
}

DeclContext *Sema::TryFindDeclContextOf(SpliceSpecifier *Splice) {
  if (Splice->getDependence() != SpliceSpecifierDependence::None)
    return nullptr;

  SmallVector<PartialDiagnosticAt, 4> Diags;
  Expr::EvalResult ER;
  ER.Diag = &Diags;

  if (!Splice->getOperand()->EvaluateAsRValue(ER, Context, true)) {
    Diag(Splice->getBeginLoc(), diag::err_splice_operand_not_constexpr);
    for (PartialDiagnosticAt PD : Diags)
      Diag(PD.first, PD.second);
    return nullptr;
  }
  APValue Refl = MaybeUnproxy(Context, ER.Val);

  switch (Refl.getReflectionKind()) {
  case ReflectionKind::Type: {
    QualType QT = Refl.getReflectedType();
    if (auto *RD = QT->getAsTagDecl())
      return RD;

    Diag(Splice->getBeginLoc(), diag::err_expected_class_or_namespace)
        << QT << getLangOpts().CPlusPlus;
    return nullptr;
  }
  case ReflectionKind::Namespace: {
    if (Splice->isSpecialization()) {
      Diag(Splice->getBeginLoc(), diag::err_unexpected_splice_specialization)
          << Splice->getSourceRange();
      return nullptr;
    }

    Decl *NS = Refl.getReflectedNamespace();
    if (auto *A = dyn_cast<NamespaceAliasDecl>(NS))
      NS = A->getNamespace();
    return cast<DeclContext>(NS);
  }
  case ReflectionKind::Template: {
    if (!Splice->isSpecialization()) {
      Diag(Splice->getBeginLoc(),
           diag::err_unexpected_reflection_kind_in_splice)
          << 3 << Splice->getSourceRange();
      return nullptr;
    }

    TemplateArgumentListInfo TAListInfo;
    for (const auto &TArg : Splice->getTemplateArgs()->arguments())
      TAListInfo.addArgument(TArg);
    QualType QT = CheckTemplateIdType(Refl.getReflectedTemplate(),
                                      SourceLocation(), TAListInfo);
    if (QT.isNull())
      return nullptr;
    else if (auto *RD = QT->getAsTagDecl())
      return RD;

    Diag(Splice->getBeginLoc(), diag::err_expected_class_or_namespace)
        << QT << getLangOpts().CPlusPlus;
    return nullptr;
  }
  case ReflectionKind::Null:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::Declaration:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
  case ReflectionKind::Annotation:
  case ReflectionKind::Attribute:
    Diag(Splice->getBeginLoc(), diag::err_expected_class_or_namespace)
        << "spliced entity" << getLangOpts().CPlusPlus;
    return nullptr;
  case ReflectionKind::EntityProxy:
    llvm_unreachable("proxies should already have been unwrapped");
  }
  llvm_unreachable("unknown reflection kind");
}
