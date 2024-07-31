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

/// \brief Representation of a reflection value holding an opaque pointer to one
/// or more entities.
class ReflectionValue {
public:
  /// \brief The kind of construct reflected.
  enum ReflectionKind {
    /// \brief A null reflection.
    ///
    /// Corresponds to no object.
    RK_null = 0,

    /// \brief A reflection of a type.
    ///
    /// Corresponds to a QualType.
    RK_type,

    /// \brief A reflection of an object (i.e., the non-function result of an
    /// lvalue).
    ///
    /// Corresponds to an APValue (plus a QualType).
    RK_object,
    
    /// \brief A reflection of a value (i.e., the result of a prvalue).
    ///
    /// Corresponds to an APValue (plus a QualType).
    RK_value,

    /// \brief A reflection of a language construct that has a declaration in
    /// the Clang AST.
    ///
    /// Corresponds to a ValueDecl, which could be any of:
    /// - a variable (i.e., VarDecl),
    /// - a structured binding (i.e., BindingDecl),
    /// - a function (i.e., FunctionDecl),
    /// - an enumerator (i.e., EnumConstantDecl),
    /// - a non-static data member or unnamed bit-field (i.e., FieldDecl),
    RK_declaration,

    /// \brief A reflection of a template (e.g., class template, variable
    /// template, function template, alias template, concept).
    ///
    /// Corresponds to a TemplateName.
    RK_template,

    /// \brief A reflection of a namespace.
    ///
    /// Corresponds to a Decl, which could be any of:
    /// - the global namespace (i.e., TranslationUnitDecl),
    /// - a non-global namespace (i.e., NamespaceDecl),
    /// - a namespace alias (i.e., NamespaceAliasDecl)
    ///
    /// Somewhat annoyingly, these classes have no nearer common ancestor than
    /// the Decl class.
    RK_namespace,

    /// \brief A reflection of a base class specifier.
    ///
    /// Corresponds to a CXXBaseSpecifier.
    RK_base_specifier,

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
    RK_data_member_spec,
  };

private:
  ReflectionKind Kind;

  void *Entity;
  QualType ResultType;

public:
  ReflectionValue();
  ReflectionValue(ReflectionValue const&Rhs);
  ReflectionValue(ReflectionKind ReflKind, void *Entity,
                  QualType ResultType = {});
  ReflectionValue &operator=(ReflectionValue const& Rhs);

  ReflectionKind getKind() const {
    return Kind;
  }
  void *getOpaqueValue() const {
    return Entity;
  }

  /// Returns whether this is a null reflection.
  bool isNull() const;

  /// Returns this as a type operand.
  QualType getAsType() const;

  /// Returns this as an APValue having an lvalue designating an object.
  const APValue &getAsObject() const;

  /// Returns this as an APValue representing a value.
  const APValue &getAsValue() const;

  /// Returns the type of the object or value represented by the reflection.
  QualType getResultType() const {
    assert(getKind() == RK_value || getKind() == RK_object);
    return ResultType;
  }

  /// Returns this as a declaration that can hold a value.
  ValueDecl *getAsDecl() const {
    assert(getKind() == RK_declaration && "not a declaration reference");
    return reinterpret_cast<ValueDecl *>(Entity);
  }

  /// Returns this as a template name.
  TemplateName getAsTemplate() const {
    assert(getKind() == RK_template && "not a template");
    return TemplateName::getFromVoidPointer(const_cast<void *>(Entity));
  }

  /// Returns this as a namespace declaration.
  Decl *getAsNamespace() const {
    assert(getKind() == RK_namespace && "not a namespace");
    return reinterpret_cast<Decl *>(Entity);
  }

  CXXBaseSpecifier *getAsBaseSpecifier() const {
    assert(getKind() == RK_base_specifier && "not a base class specifier");
    return reinterpret_cast<CXXBaseSpecifier *>(Entity);
  }

  /// Returns this as a struct describing a hypothetical data member.
  TagDataMemberSpec *getAsDataMemberSpec() const {
    assert(getKind() == RK_data_member_spec && "not a data member spec");
    return reinterpret_cast<TagDataMemberSpec *>(Entity);
  }

  void Profile(llvm::FoldingSetNodeID &ID) const;

  bool operator==(ReflectionValue const& Rhs) const;
  bool operator!=(ReflectionValue const& Rhs) const;
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
