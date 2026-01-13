//===- NestedNameSpecifier.h - C++ nested name specifiers -------*- C++ -*-===//
//
// Copyright 2024 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//  This file completes the definition of the NestedNameSpecifier class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_AST_NESTEDNAMESPECIFIER_H
#define LLVM_CLANG_AST_NESTEDNAMESPECIFIER_H

#include "Expr.h"

#include "clang/AST/Decl.h"
#include "clang/AST/NestedNameSpecifierBase.h"
#include "clang/AST/Type.h"
#include "clang/AST/TypeLoc.h"
#include "llvm/ADT/DenseMapInfo.h"

namespace clang {

auto NestedNameSpecifier::getKind() const -> Kind {
  if (!isStoredKind()) {
    switch (getFlagKind()) {
    case FlagKind::Null:
      return Kind::Null;
    case FlagKind::Global:
      return Kind::Global;
    case FlagKind::Invalid:
      llvm_unreachable("use of invalid NestedNameSpecifier");
    }
    llvm_unreachable("unhandled FlagKind");
  }
  switch (auto [K, Ptr] = getStored(); K) {
  case StoredKind::Type:
    return Kind::Type;
  case StoredKind::NamespaceWithGlobal:
  case StoredKind::NamespaceWithNamespace:
    return Kind::Namespace;
  case StoredKind::NamespaceOrSuper:
    switch (static_cast<const Decl *>(Ptr)->getKind()) {
    case Decl::Namespace:
    case Decl::NamespaceAlias:
      return Kind::Namespace;
    case Decl::CXXRecord:
    case Decl::ClassTemplateSpecialization:
    case Decl::ClassTemplatePartialSpecialization:
      return Kind::MicrosoftSuper;
    default:
      llvm_unreachable("unexpected decl kind");
    }
  // todo [merge:yukino:maybe-revert]
  case StoredKind::SpliceSpecifier:
    return Kind::Splice;
  case StoredKind::SpliceSpecifierWithTemplate:
    return Kind::SpliceWithTemplate;
  case StoredKind::NumKinds:;
  }
  llvm_unreachable("unknown StoredKind");
}

NestedNameSpecifier::NestedNameSpecifier(const Type *T)
    : NestedNameSpecifier({StoredKind::Type, T}) {
  assert(getKind() == Kind::Type);
}

auto NestedNameSpecifier::MakeNamespacePtrKind(
    const ASTContext &Ctx, const NamespaceBaseDecl *Namespace,
    NestedNameSpecifier Prefix) -> PtrKind {
  switch (auto Kind = Prefix.getKind()) {
  case Kind::Null:
    return {StoredKind::NamespaceOrSuper, Namespace};
  case Kind::Global:
    return {StoredKind::NamespaceWithGlobal, Namespace};
  case Kind::Namespace:
    return {StoredKind::NamespaceWithNamespace,
            MakeNamespaceAndPrefixStorage(Ctx, Namespace, Prefix)};
  // todo [merge:yukino:maybe-revert,"should be same as namespaces"]
  case Kind::Splice:
    [[fallthrough]];
  case Kind::SpliceWithTemplate: {
    auto SK = Kind == Kind::Splice ? StoredKind::SpliceSpecifier
                                   : StoredKind::SpliceSpecifierWithTemplate;
    if (Prefix.getAsNamespaceAndPrefix().Namespace) {
      return {SK, MakeNamespaceAndPrefixStorage(Ctx, Namespace, Prefix)};
    }
    return {SK, Prefix.getSplice()};
  }
  case Kind::MicrosoftSuper:
  case Kind::Type:
    llvm_unreachable("invalid prefix for namespace");
  }
  llvm_unreachable("unhandled kind");
}

/// Builds a nested name specifier that names a namespace.
NestedNameSpecifier::NestedNameSpecifier(const ASTContext &Ctx,
                                         const NamespaceBaseDecl *Namespace,
                                         NestedNameSpecifier Prefix)
    : NestedNameSpecifier(MakeNamespacePtrKind(Ctx, Namespace, Prefix)) {
  assert(getKind() == Kind::Namespace);
}

/// Builds a nested name specifier that names a class through microsoft's
/// __super specifier.
NestedNameSpecifier::NestedNameSpecifier(CXXRecordDecl *RD)
    : NestedNameSpecifier({StoredKind::NamespaceOrSuper, RD}) {
  assert(getKind() == Kind::MicrosoftSuper);
}

// todo [merge:yukino:maybe-revert]
NestedNameSpecifier::NestedNameSpecifier(/*const ASTContext &Context,*/
                                         bool Template,
                                         const SpliceSpecifier *Splice)
    : NestedNameSpecifier(PtrKind{Template
                                      ? StoredKind::SpliceSpecifierWithTemplate
                                      : StoredKind::SpliceSpecifier,
                                  Splice}) {
  assert(Splice && "Splice cannot be NULL");
  // todo [merge:yukino:maybe-revert,"It doesn't seem like Context is useful
  //   here anymore"]
  // (void)Context;
}

CXXRecordDecl *NestedNameSpecifier::getAsRecordDecl() const {
  switch (getKind()) {
  case Kind::MicrosoftSuper:
    return getAsMicrosoftSuper();
  case Kind::Type:
    return getAsType()->getAsCXXRecordDecl();
  case Kind::Global:
  case Kind::Namespace:
    return nullptr;
  // todo [merge:yukino:maybe-revert]
  // If the splice resolved to a Type, it would be Kind::Type.
  // If it is still Kind::Splice, it is likely dependent or unresolved.
  // We cannot resolve it to a RecordDecl without ASTContext evaluation,
  // and this function signature does not provide ASTContext.
  case Kind::Splice:
  case Kind::SpliceWithTemplate:
    return nullptr;
  case Kind::Null:
    return nullptr;
  }
  llvm_unreachable("Invalid NNS Kind!");
}

NestedNameSpecifier NestedNameSpecifier::getCanonical() const {
  switch (getKind()) {
  case NestedNameSpecifier::Kind::Null:
  case NestedNameSpecifier::Kind::Global:
  case NestedNameSpecifier::Kind::MicrosoftSuper:
    // These are canonical and unique.
    return *this;
  case NestedNameSpecifier::Kind::Namespace: {
    // A namespace is canonical; build a nested-name-specifier with
    // this namespace and no prefix.
    const NamespaceBaseDecl *ND = getAsNamespaceAndPrefix().Namespace;
    return NestedNameSpecifier(
        {StoredKind::NamespaceOrSuper, ND->getNamespace()->getCanonicalDecl()});
  }
  case NestedNameSpecifier::Kind::Type:
    return NestedNameSpecifier(
        getAsType()->getCanonicalTypeInternal().getTypePtr());

  // todo [merge:yukino:maybe-revert]
  // Splices should always be unique & canonical.
  case NestedNameSpecifier::Kind::Splice:
  case NestedNameSpecifier::Kind::SpliceWithTemplate:
    return *this;
  }
  llvm_unreachable("unhandled kind");
}

bool NestedNameSpecifier::isCanonical() const {
  return *this == getCanonical();
}

TypeLoc NestedNameSpecifierLoc::castAsTypeLoc() const {
  return TypeLoc(Qualifier.getAsType(), LoadPointer(/*Offset=*/0));
}

TypeLoc NestedNameSpecifierLoc::getAsTypeLoc() const {
  if (Qualifier.getKind() != NestedNameSpecifier::Kind::Type)
    return TypeLoc();
  return castAsTypeLoc();
}

unsigned
NestedNameSpecifierLoc::getLocalDataLength(NestedNameSpecifier Qualifier) {
  // Location of the trailing '::'.
  unsigned Length = sizeof(SourceLocation::UIntTy);

  switch (Qualifier.getKind()) {
  case NestedNameSpecifier::Kind::Global:
    // Nothing more to add.
    break;

  case NestedNameSpecifier::Kind::Namespace:
  case NestedNameSpecifier::Kind::MicrosoftSuper:
    // The location of the identifier or namespace name.
    Length += sizeof(SourceLocation::UIntTy);
    break;

  case NestedNameSpecifier::Kind::Type:
    // The "void*" that points at the TypeLoc data.
    // Note: the 'template' keyword is part of the TypeLoc.
    Length += sizeof(void *);
    break;

  // todo [merge:yukino:maybe-revert]
  case NestedNameSpecifier::Kind::SpliceWithTemplate:
    Length += sizeof(SourceLocation::UIntTy);
    [[fallthrough]];
  case NestedNameSpecifier::Kind::Splice:
    // The "void*" that points at the TypeLoc or Expr data.
    // Note: the 'template' keyword is part of the TypeLoc.
    Length += sizeof(void *);
    break;

  case NestedNameSpecifier::Kind::Null:
    llvm_unreachable("Expected a non-NULL qualifier");
  }
  return Length;
}

NamespaceAndPrefixLoc NestedNameSpecifierLoc::castAsNamespaceAndPrefix() const {
  auto [Namespace, Prefix] = Qualifier.getAsNamespaceAndPrefix();
  return {Namespace, NestedNameSpecifierLoc(Prefix, Data)};
}

NamespaceAndPrefixLoc NestedNameSpecifierLoc::getAsNamespaceAndPrefix() const {
  if (Qualifier.getKind() != NestedNameSpecifier::Kind::Namespace)
    return {};
  return castAsNamespaceAndPrefix();
}

unsigned NestedNameSpecifierLoc::getDataLength(NestedNameSpecifier Qualifier) {
  unsigned Length = 0;
  for (; Qualifier; Qualifier = Qualifier.getAsNamespaceAndPrefix().Prefix) {
    Length += getLocalDataLength(Qualifier);
    // todo [merge:yukino:maybe-revert,"why the break?"]
    if (Qualifier.getKind() != NestedNameSpecifier::Kind::Namespace)
      break;
  }
  return Length;
}

unsigned NestedNameSpecifierLoc::getDataLength() const {
  return getDataLength(Qualifier);
}

SourceRange NestedNameSpecifierLoc::getLocalSourceRange() const {
  switch (auto Kind = Qualifier.getKind()) {
  case NestedNameSpecifier::Kind::Null:
    return SourceRange();
  case NestedNameSpecifier::Kind::Global:
    return LoadSourceLocation(/*Offset=*/0);
  case NestedNameSpecifier::Kind::Namespace:
  case NestedNameSpecifier::Kind::MicrosoftSuper: {
    unsigned Offset =
        Kind == NestedNameSpecifier::Kind::Namespace
            ? getDataLength(Qualifier.getAsNamespaceAndPrefix().Prefix)
            : 0;
    return SourceRange(
        LoadSourceLocation(Offset),
        LoadSourceLocation(Offset + sizeof(SourceLocation::UIntTy)));
  }
  case NestedNameSpecifier::Kind::Type: {
    // The "void*" that points at the TypeLoc data.
    // Note: the 'template' keyword is part of the TypeLoc.
    void *TypeData = LoadPointer(/*Offset=*/0);
    TypeLoc TL(Qualifier.getAsType(), TypeData);
    return SourceRange(TL.getBeginLoc(), LoadSourceLocation(sizeof(void *)));
  }

  // todo [merge:yukino:maybe-revert]
  case NestedNameSpecifier::Kind::Splice: {
    // unsigned Offset = getDataLength(Qualifier.getAsNamespaceAndPrefix().Prefix);
    const SpliceSpecifier *Splice =
        reinterpret_cast<SpliceSpecifier *>(LoadPointer(/*Offset=*/0));
    return SourceRange(Splice->getBeginLoc(),
                       LoadSourceLocation(/*Offset=*/0 + sizeof(void *)));
  }
  case NestedNameSpecifier::Kind::SpliceWithTemplate: {
    // unsigned Offset = getDataLength(Qualifier.getAsNamespaceAndPrefix().Prefix);
    (void)LoadPointer(/*Offset=*/0);
    return SourceRange(LoadSourceLocation(/*Offset=*/0 + sizeof(void *)),
                       LoadSourceLocation(/*Offset=*/0 + sizeof(void *) +
                                          sizeof(SourceLocation::UIntTy)));
  }
  }

  llvm_unreachable("Invalid NNS Kind!");
}

// todo [merge:yukino:maybe-revert]
/// Retrieve the splice specifier stored in this nested name specifier
const SpliceSpecifier *NestedNameSpecifier::getAsSplice() const {
  auto [Kind, Ptr] = getStored();
  if (Kind == StoredKind::SpliceSpecifier ||
      Kind == StoredKind::SpliceSpecifierWithTemplate) {
    assert(Ptr != nullptr);
    return static_cast<const SpliceSpecifier *>(const_cast<void *>(Ptr));
  }
  llvm_unreachable("trying to get SpliceSpecifier from an incompatible store.");
}

// todo [merge:yukino:maybe-revert]
const SpliceSpecifier *NestedNameSpecifier::getSplice() const {
  auto Kind = getKind();
  if (Kind != Kind::Splice && Kind != Kind::SpliceWithTemplate)
    return nullptr;
  return getAsSplice();
}

SourceRange NestedNameSpecifierLoc::getSourceRange() const {
  return SourceRange(getBeginLoc(), getEndLoc());
}

SourceLocation NestedNameSpecifierLoc::getEndLoc() const {
  return getLocalSourceRange().getEnd();
}

/// Retrieve the location of the beginning of this
/// component of the nested-name-specifier.
SourceLocation NestedNameSpecifierLoc::getLocalBeginLoc() const {
  return getLocalSourceRange().getBegin();
}

/// Retrieve the location of the end of this component of the
/// nested-name-specifier.
SourceLocation NestedNameSpecifierLoc::getLocalEndLoc() const {
  return getLocalSourceRange().getEnd();
}

SourceRange NestedNameSpecifierLocBuilder::getSourceRange() const {
  return NestedNameSpecifierLoc(Representation, Buffer).getSourceRange();
}

} // namespace clang

namespace llvm {

template <> struct DenseMapInfo<clang::NestedNameSpecifier> {
  static clang::NestedNameSpecifier getEmptyKey() { return std::nullopt; }

  static clang::NestedNameSpecifier getTombstoneKey() {
    return clang::NestedNameSpecifier::getInvalid();
  }

  static unsigned getHashValue(const clang::NestedNameSpecifier &V) {
    return hash_combine(V.getAsVoidPointer());
  }
};

template <> struct DenseMapInfo<clang::NestedNameSpecifierLoc> {
  using FirstInfo = DenseMapInfo<clang::NestedNameSpecifier>;
  using SecondInfo = DenseMapInfo<void *>;

  static clang::NestedNameSpecifierLoc getEmptyKey() {
    return clang::NestedNameSpecifierLoc(FirstInfo::getEmptyKey(),
                                         SecondInfo::getEmptyKey());
  }

  static clang::NestedNameSpecifierLoc getTombstoneKey() {
    return clang::NestedNameSpecifierLoc(FirstInfo::getTombstoneKey(),
                                         SecondInfo::getTombstoneKey());
  }

  static unsigned getHashValue(const clang::NestedNameSpecifierLoc &PairVal) {
    return hash_combine(
        FirstInfo::getHashValue(PairVal.getNestedNameSpecifier()),
        SecondInfo::getHashValue(PairVal.getOpaqueData()));
  }

  static bool isEqual(const clang::NestedNameSpecifierLoc &LHS,
                      const clang::NestedNameSpecifierLoc &RHS) {
    return LHS == RHS;
  }
};
} // namespace llvm

#endif // LLVM_CLANG_AST_NESTEDNAMESPECIFIER_H
