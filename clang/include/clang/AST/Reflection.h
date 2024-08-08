//===--- Reflection.h - Classes for representing reflection -----*- C++ -*-===//
//
// Copyright 2024 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief Defines facilities for representing reflected entities.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_AST_REFLECTION_H
#define LLVM_CLANG_AST_REFLECTION_H

#include "clang/AST/Type.h"
#include "llvm/ADT/FoldingSet.h"
#include <optional>
#include <string>

namespace clang {

class APValue;
class ASTContext;
class CXXBaseSpecifier;
class NamespaceDecl;
class ValueDecl;

struct TagDataMemberSpec;

/// \brief The kind of construct reflected.
enum class ReflectionKind {
  /// \brief A null reflection.
  ///
  /// Corresponds to no object.
  Null = 0,

  /// \brief A reflection of a type.
  ///
  /// Corresponds to a QualType.
  Type,

  /// \brief A reflection of an object (i.e., the non-function result of an
  /// lvalue).
  ///
  /// Corresponds to an APValue (plus a QualType).
  Object,
  
  /// \brief A reflection of a value (i.e., the result of a prvalue).
  ///
  /// Corresponds to an APValue (plus a QualType).
  Value,

  /// \brief A reflection of a language construct that has a declaration in
  /// the Clang AST.
  ///
  /// Corresponds to a ValueDecl, which could be any of:
  /// - a variable (i.e., VarDecl),
  /// - a structured binding (i.e., BindingDecl),
  /// - a function (i.e., FunctionDecl),
  /// - an enumerator (i.e., EnumConstantDecl),
  /// - a non-static data member or unnamed bit-field (i.e., FieldDecl),
  Declaration,

  /// \brief A reflection of a template (e.g., class template, variable
  /// template, function template, alias template, concept).
  ///
  /// Corresponds to a TemplateName.
  Template,

  /// \brief A reflection of a namespace.
  ///
  /// Corresponds to a Decl, which could be any of:
  /// - the global namespace (i.e., TranslationUnitDecl),
  /// - a non-global namespace (i.e., NamespaceDecl),
  /// - a namespace alias (i.e., NamespaceAliasDecl)
  ///
  /// Somewhat annoyingly, these classes have no nearer common ancestor than
  /// the Decl class.
  Namespace,

  /// \brief A reflection of a base class specifier.
  ///
  /// Corresponds to a CXXBaseSpecifier.
  BaseSpecifier,

  /// \brief A reflection of a description of a hypothetical data member
  /// (static or nonstatic) that might belong to a class or union.
  ///
  /// Corresponds to a TagDataMemberSpec.
  ///
  /// This is specifically used for the 'std::meta::data_member_spec' and
  /// 'std::meta::define_class' metafunctions. If the surface area of
  /// 'define_class' grows (i.e., supports additional types of "descriptions",
  /// e.g., for member functions), it would be nice to find a more generic way
  /// to do this. One idea is to allow a reflection of a type erased struct,
  /// but the current design seems tolerable for now.
  DataMemberSpec,
};


/// \brief Representation of a hypothetical data member, which could be used to
/// complete an incomplete class definition using the 'std::meta::define_class'
/// standard library function.
struct TagDataMemberSpec {
  QualType Ty;

  std::optional<std::string> Name;
  std::optional<size_t> Alignment;
  std::optional<size_t> BitWidth;
  bool NoUniqueAddress;

  bool operator==(TagDataMemberSpec const& Rhs) const;
  bool operator!=(TagDataMemberSpec const& Rhs) const;
};
} // namespace clang

#endif
