//===- AttributeScratchpad.h - Synthesized ParsedAttr storage ---*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Defines the storage used by the P3385 attribute reflection metafunctions to
// hold the ParsedAttr nodes they synthesize from semantic Attrs.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_LIB_AST_ATTRIBUTESCRATCHPAD_H
#define LLVM_CLANG_LIB_AST_ATTRIBUTESCRATCHPAD_H

#include "clang/Sema/ParsedAttr.h"

namespace clang {

/// Owns the ParsedAttr nodes synthesized while evaluating attribute
/// reflection metafunctions.
///
/// Neither AttributeFactory nor AttributePool is thread-safe, and the
/// ParsedAttrs handed out here embed pointers (Exprs, IdentifierInfos) into
/// the ASTContext that produced them. One scratchpad therefore belongs to one
/// ASTContext, which owns it and destroys it along with the AST that refers
/// to those attributes; see ASTContext::getAttributeScratchpad().
struct AttributeScratchpad {
  AttributeFactory factory;
  AttributePool pool;
  AttributeScratchpad() : factory(), pool(factory) {}
};

} // namespace clang

#endif // LLVM_CLANG_LIB_AST_ATTRIBUTESCRATCHPAD_H
