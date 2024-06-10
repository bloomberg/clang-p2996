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
class ConstantExpr;
class NamespaceDecl;
class ValueDecl;

struct TagDataMemberSpec;

/// \brief Representation of a reflection value holding an opaque pointer to one
/// or more entities.
class ReflectionValue {
public:
  /// \brief The kind of construct reflected.
  enum ReflectionKind {
    /// \brief A null reflection. Corresponds to no object.
    RK_null = 0,

    /// \brief A reflection of a type. Corresponds to an object of type
    /// QualType.
    RK_type = 1,

    /// \brief A reflection of the result of an expression. Corresponds to an
    /// object of type ConstantExpr.
    RK_expr_result = 2,

    /// \brief A reflection of an expression referencing an entity (e.g.,
    /// variable, function, member function, etc). Corresponds to an object of
    /// type Decl.
    RK_declaration = 3,

    /// \brief A reflection of a template (e.g., class template, variable
    /// template, function template, alias template, concept).
    RK_template = 4,

    /// \brief A reflection of a namespace. Corresponds to an object of type
    /// Decl.
    ///
    /// A namespace could be represented as a TranslationUnitDecl (for the
    /// global namespace), a NamespaceAliasDecl (for namespace aliases), or a
    /// NamespaceDecl (for all other namespaces). Somewhat annoyingly, these
    /// classes have no nearer common ancestor than the base Decl class.
    RK_namespace = 5,

    /// \brief A reflection of a base class specifier. Corresponds to an object
    /// of type CXXBaseSpecifier.
    RK_base_specifier = 6,

    /// \brief A reflection of a description of a hypothetical data member
    /// (static or nonstatic) that might belong to a class or union. Corresponds
    /// to an object of type TagDataMemberSpec.
    ///
    /// This is specifically used for 'std::meta::data_member_description' and
    /// 'std::meta::define_class'. If the surface area of 'define_class' grows
    /// (i.e., supports additional types of "descriptions", e.g., for member
    /// functions), it would be nice to find a more generic way to do this. One
    /// idea is to allow a reflection of a type erased struct, but this seemed
    /// like a tolerable idea for the time being.
    RK_data_member_spec = 7,
  };

private:
  ReflectionKind Kind;
  void *Entity;

public:
  ReflectionValue();
  ReflectionValue(ReflectionValue const&Rhs);
  ReflectionValue(ReflectionKind ReflKind, void *Entity);
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

  /// Returns this as an expression.
  ConstantExpr *getAsExprResult() const {
    assert(getKind() == RK_expr_result && "not an expression result");
    return reinterpret_cast<ConstantExpr *>(Entity);
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
  bool IsStatic;
  std::optional<size_t> Alignment;
  std::optional<size_t> BitWidth;

  bool operator==(TagDataMemberSpec const& Rhs) const;
  bool operator!=(TagDataMemberSpec const& Rhs) const;
};

} // namespace clang

#endif
