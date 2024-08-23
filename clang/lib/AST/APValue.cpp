//===--- APValue.cpp - Union class for APFloat/APSInt/Complex -------------===//
//
// Copyright 2024 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//  This file implements the APValue class.
//
//===----------------------------------------------------------------------===//

#include "clang/AST/APValue.h"
#include "Linkage.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/CharUnits.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/LocInfoType.h"
#include "clang/AST/Type.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include <iostream>
using namespace clang;

/// The identity of a type_info object depends on the canonical unqualified
/// type only.
TypeInfoLValue::TypeInfoLValue(const Type *T)
    : T(T->getCanonicalTypeUnqualified().getTypePtr()) {}

void TypeInfoLValue::print(llvm::raw_ostream &Out,
                           const PrintingPolicy &Policy) const {
  Out << "typeid(";
  QualType(getType(), 0).print(Out, Policy);
  Out << ")";
}

static_assert(
    1 << llvm::PointerLikeTypeTraits<TypeInfoLValue>::NumLowBitsAvailable <=
        alignof(Type),
    "Type is insufficiently aligned");

APValue::LValueBase::LValueBase(const ValueDecl *P, unsigned I, unsigned V)
    : Ptr(P ? cast<ValueDecl>(P->getCanonicalDecl()) : nullptr), Local{I, V} {}
APValue::LValueBase::LValueBase(const Expr *P, unsigned I, unsigned V)
    : Ptr(P), Local{I, V} {}

APValue::LValueBase APValue::LValueBase::getDynamicAlloc(DynamicAllocLValue LV,
                                                         QualType Type) {
  LValueBase Base;
  Base.Ptr = LV;
  Base.DynamicAllocType = Type.getAsOpaquePtr();
  return Base;
}

APValue::LValueBase APValue::LValueBase::getTypeInfo(TypeInfoLValue LV,
                                                     QualType TypeInfo) {
  LValueBase Base;
  Base.Ptr = LV;
  Base.TypeInfoType = TypeInfo.getAsOpaquePtr();
  return Base;
}

QualType APValue::LValueBase::getType() const {
  if (!*this) return QualType();
  if (const ValueDecl *D = dyn_cast<const ValueDecl*>()) {
    // FIXME: It's unclear where we're supposed to take the type from, and
    // this actually matters for arrays of unknown bound. Eg:
    //
    // extern int arr[]; void f() { extern int arr[3]; };
    // constexpr int *p = &arr[1]; // valid?
    //
    // For now, we take the most complete type we can find.
    for (auto *Redecl = cast<ValueDecl>(D->getMostRecentDecl()); Redecl;
         Redecl = cast_or_null<ValueDecl>(Redecl->getPreviousDecl())) {
      QualType T = Redecl->getType();
      if (!T->isIncompleteArrayType())
        return T;
    }
    return D->getType();
  }

  if (is<TypeInfoLValue>())
    return getTypeInfoType();

  if (is<DynamicAllocLValue>())
    return getDynamicAllocType();

  const Expr *Base = get<const Expr*>();

  // For a materialized temporary, the type of the temporary we materialized
  // may not be the type of the expression.
  if (const MaterializeTemporaryExpr *MTE =
          llvm::dyn_cast<MaterializeTemporaryExpr>(Base)) {
    SmallVector<const Expr *, 2> CommaLHSs;
    SmallVector<SubobjectAdjustment, 2> Adjustments;
    const Expr *Temp = MTE->getSubExpr();
    const Expr *Inner = Temp->skipRValueSubobjectAdjustments(CommaLHSs,
                                                             Adjustments);
    // Keep any cv-qualifiers from the reference if we generated a temporary
    // for it directly. Otherwise use the type after adjustment.
    if (!Adjustments.empty())
      return Inner->getType();
  }

  return Base->getType();
}

unsigned APValue::LValueBase::getCallIndex() const {
  return (is<TypeInfoLValue>() || is<DynamicAllocLValue>()) ? 0
                                                            : Local.CallIndex;
}

unsigned APValue::LValueBase::getVersion() const {
  return (is<TypeInfoLValue>() || is<DynamicAllocLValue>()) ? 0 : Local.Version;
}

QualType APValue::LValueBase::getTypeInfoType() const {
  assert(is<TypeInfoLValue>() && "not a type_info lvalue");
  return QualType::getFromOpaquePtr(TypeInfoType);
}

QualType APValue::LValueBase::getDynamicAllocType() const {
  assert(is<DynamicAllocLValue>() && "not a dynamic allocation lvalue");
  return QualType::getFromOpaquePtr(DynamicAllocType);
}

void APValue::LValueBase::Profile(llvm::FoldingSetNodeID &ID) const {
  ID.AddPointer(Ptr.getOpaqueValue());
  if (is<TypeInfoLValue>() || is<DynamicAllocLValue>())
    return;
  ID.AddInteger(Local.CallIndex);
  ID.AddInteger(Local.Version);
}

namespace clang {
bool operator==(const APValue::LValueBase &LHS,
                const APValue::LValueBase &RHS) {
  if (LHS.Ptr != RHS.Ptr)
    return false;
  if (LHS.is<TypeInfoLValue>() || LHS.is<DynamicAllocLValue>())
    return true;
  return LHS.Local.CallIndex == RHS.Local.CallIndex &&
         LHS.Local.Version == RHS.Local.Version;
}
}

APValue::LValuePathEntry::LValuePathEntry(BaseOrMemberType BaseOrMember) {
  if (const Decl *D = BaseOrMember.getPointer())
    BaseOrMember.setPointer(D->getCanonicalDecl());
  Value = reinterpret_cast<uintptr_t>(BaseOrMember.getOpaqueValue());
}

void APValue::LValuePathEntry::Profile(llvm::FoldingSetNodeID &ID) const {
  ID.AddInteger(Value);
}

APValue::LValuePathSerializationHelper::LValuePathSerializationHelper(
    ArrayRef<LValuePathEntry> Path, QualType ElemTy)
    : Ty((const void *)ElemTy.getTypePtrOrNull()), Path(Path) {}

QualType APValue::LValuePathSerializationHelper::getType() {
  return QualType::getFromOpaquePtr(Ty);
}

namespace {
  struct LVBase {
    APValue::LValueBase Base;
    CharUnits Offset;
    unsigned PathLength;
    bool IsNullPtr : 1;
    bool IsOnePastTheEnd : 1;
  };
}

void *APValue::LValueBase::getOpaqueValue() const {
  return Ptr.getOpaqueValue();
}

bool APValue::LValueBase::isNull() const {
  return Ptr.isNull();
}

APValue::LValueBase::operator bool () const {
  return static_cast<bool>(Ptr);
}

clang::APValue::LValueBase
llvm::DenseMapInfo<clang::APValue::LValueBase>::getEmptyKey() {
  clang::APValue::LValueBase B;
  B.Ptr = DenseMapInfo<const ValueDecl*>::getEmptyKey();
  return B;
}

clang::APValue::LValueBase
llvm::DenseMapInfo<clang::APValue::LValueBase>::getTombstoneKey() {
  clang::APValue::LValueBase B;
  B.Ptr = DenseMapInfo<const ValueDecl*>::getTombstoneKey();
  return B;
}

namespace clang {
llvm::hash_code hash_value(const APValue::LValueBase &Base) {
  if (Base.is<TypeInfoLValue>() || Base.is<DynamicAllocLValue>())
    return llvm::hash_value(Base.getOpaqueValue());
  return llvm::hash_combine(Base.getOpaqueValue(), Base.getCallIndex(),
                            Base.getVersion());
}
}

unsigned llvm::DenseMapInfo<clang::APValue::LValueBase>::getHashValue(
    const clang::APValue::LValueBase &Base) {
  return hash_value(Base);
}

bool llvm::DenseMapInfo<clang::APValue::LValueBase>::isEqual(
    const clang::APValue::LValueBase &LHS,
    const clang::APValue::LValueBase &RHS) {
  return LHS == RHS;
}

struct APValue::LV : LVBase {
  static const unsigned InlinePathSpace =
      (DataSize - sizeof(LVBase)) / sizeof(LValuePathEntry);

  /// Path - The sequence of base classes, fields and array indices to follow to
  /// walk from Base to the subobject. When performing GCC-style folding, there
  /// may not be such a path.
  union {
    LValuePathEntry Path[InlinePathSpace];
    LValuePathEntry *PathPtr;
  };

  LV() { PathLength = (unsigned)-1; }
  ~LV() { resizePath(0); }

  void resizePath(unsigned Length) {
    if (Length == PathLength)
      return;
    if (hasPathPtr())
      delete [] PathPtr;
    PathLength = Length;
    if (hasPathPtr())
      PathPtr = new LValuePathEntry[Length];
  }

  bool hasPath() const { return PathLength != (unsigned)-1; }
  bool hasPathPtr() const { return hasPath() && PathLength > InlinePathSpace; }

  LValuePathEntry *getPath() { return hasPathPtr() ? PathPtr : Path; }
  const LValuePathEntry *getPath() const {
    return hasPathPtr() ? PathPtr : Path;
  }
};

namespace {
  struct MemberPointerBase {
    llvm::PointerIntPair<const ValueDecl*, 1, bool> MemberAndIsDerivedMember;
    unsigned PathLength;
  };
}

struct APValue::MemberPointerData : MemberPointerBase {
  static const unsigned InlinePathSpace =
      (DataSize - sizeof(MemberPointerBase)) / sizeof(const CXXRecordDecl*);
  typedef const CXXRecordDecl *PathElem;
  union {
    PathElem Path[InlinePathSpace];
    PathElem *PathPtr;
  };

  MemberPointerData() { PathLength = 0; }
  ~MemberPointerData() { resizePath(0); }

  void resizePath(unsigned Length) {
    if (Length == PathLength)
      return;
    if (hasPathPtr())
      delete [] PathPtr;
    PathLength = Length;
    if (hasPathPtr())
      PathPtr = new PathElem[Length];
  }

  bool hasPathPtr() const { return PathLength > InlinePathSpace; }

  PathElem *getPath() { return hasPathPtr() ? PathPtr : Path; }
  const PathElem *getPath() const {
    return hasPathPtr() ? PathPtr : Path;
  }
};

// FIXME: Reduce the malloc traffic here.

APValue::Arr::Arr(unsigned NumElts, unsigned Size) :
  Elts(new APValue[NumElts + (NumElts != Size ? 1 : 0)]),
  NumElts(NumElts), ArrSize(Size) {}
APValue::Arr::~Arr() { delete [] Elts; }

APValue::StructData::StructData(unsigned NumBases, unsigned NumFields) :
  Elts(new APValue[NumBases+NumFields]),
  NumBases(NumBases), NumFields(NumFields) {}
APValue::StructData::~StructData() {
  delete [] Elts;
}

APValue::UnionData::UnionData() : Field(nullptr), Value(new APValue) {}
APValue::UnionData::~UnionData () {
  delete Value;
}

APValue::APValue(const APValue &RHS)
    : Kind(None), UnderlyingTy(), ReflectionDepth() {
  switch (RHS.Kind) {
  case None:
  case Indeterminate:
    Kind = RHS.Kind;
    break;
  case Int:
    MakeInt();
    setInt(RHS.getInt());
    break;
  case Float:
    MakeFloat();
    setFloat(RHS.getFloat());
    break;
  case FixedPoint: {
    APFixedPoint FXCopy = RHS.getFixedPoint();
    MakeFixedPoint(std::move(FXCopy));
    break;
  }
  case Vector:
    MakeVector();
    setVector(((const Vec *)(const char *)&RHS.Data)->Elts,
              RHS.getVectorLength());
    break;
  case ComplexInt:
    MakeComplexInt();
    setComplexInt(RHS.getComplexIntReal(), RHS.getComplexIntImag());
    break;
  case ComplexFloat:
    MakeComplexFloat();
    setComplexFloat(RHS.getComplexFloatReal(), RHS.getComplexFloatImag());
    break;
  case LValue:
    MakeLValue();
    if (RHS.hasLValuePath())
      setLValue(RHS.getLValueBase(), RHS.getLValueOffset(), RHS.getLValuePath(),
                RHS.isLValueOnePastTheEnd(), RHS.isNullPointer());
    else
      setLValue(RHS.getLValueBase(), RHS.getLValueOffset(), NoLValuePath(),
                RHS.isNullPointer());
    break;
  case Array:
    MakeArray(RHS.getArrayInitializedElts(), RHS.getArraySize());
    for (unsigned I = 0, N = RHS.getArrayInitializedElts(); I != N; ++I)
      getArrayInitializedElt(I) = RHS.getArrayInitializedElt(I);
    if (RHS.hasArrayFiller())
      getArrayFiller() = RHS.getArrayFiller();
    break;
  case Struct:
    MakeStruct(RHS.getStructNumBases(), RHS.getStructNumFields());
    for (unsigned I = 0, N = RHS.getStructNumBases(); I != N; ++I)
      getStructBase(I) = RHS.getStructBase(I);
    for (unsigned I = 0, N = RHS.getStructNumFields(); I != N; ++I)
      getStructField(I) = RHS.getStructField(I);
    break;
  case Union:
    MakeUnion();
    setUnion(RHS.getUnionField(), RHS.getUnionValue());
    break;
  case MemberPointer:
    MakeMemberPointer(RHS.getMemberPointerDecl(),
                      RHS.isMemberPointerToDerivedMember(),
                      RHS.getMemberPointerPath());
    break;
  case AddrLabelDiff:
    MakeAddrLabelDiff();
    setAddrLabelDiff(RHS.getAddrLabelDiffLHS(), RHS.getAddrLabelDiffRHS());
    break;
  case Reflection:
    MakeReflection();
    setReflection(((const ReflectionData *)(const char *)&RHS.Data)->Kind,
                  RHS.getOpaqueReflectionData());
    break;
  }
  ReflectionDepth = RHS.ReflectionDepth;
  UnderlyingTy = RHS.UnderlyingTy;
}

APValue::APValue(APValue &&RHS)
    : Kind(RHS.Kind), Data(RHS.Data),
      UnderlyingTy(RHS.UnderlyingTy), ReflectionDepth(RHS.ReflectionDepth) {
  RHS.Kind = None;
}

APValue &APValue::operator=(const APValue &RHS) {
  if (this != &RHS)
    *this = APValue(RHS);
  return *this;
}

APValue &APValue::operator=(APValue &&RHS) {
  if (this != &RHS) {
    if (Kind != None && Kind != Indeterminate)
      DestroyDataAndMakeUninit();
    Kind = RHS.Kind;
    Data = RHS.Data;
    UnderlyingTy = RHS.UnderlyingTy;
    ReflectionDepth = RHS.ReflectionDepth;
    RHS.Kind = None;
  }
  return *this;
}

void APValue::DestroyDataAndMakeUninit() {
  if (Kind == Int)
    ((APSInt *)(char *)&Data)->~APSInt();
  else if (Kind == Float)
    ((APFloat *)(char *)&Data)->~APFloat();
  else if (Kind == FixedPoint)
    ((APFixedPoint *)(char *)&Data)->~APFixedPoint();
  else if (Kind == Vector)
    ((Vec *)(char *)&Data)->~Vec();
  else if (Kind == ComplexInt)
    ((ComplexAPSInt *)(char *)&Data)->~ComplexAPSInt();
  else if (Kind == ComplexFloat)
    ((ComplexAPFloat *)(char *)&Data)->~ComplexAPFloat();
  else if (Kind == LValue)
    ((LV *)(char *)&Data)->~LV();
  else if (Kind == Array)
    ((Arr *)(char *)&Data)->~Arr();
  else if (Kind == Struct)
    ((StructData *)(char *)&Data)->~StructData();
  else if (Kind == Union)
    ((UnionData *)(char *)&Data)->~UnionData();
  else if (Kind == MemberPointer)
    ((MemberPointerData *)(char *)&Data)->~MemberPointerData();
  else if (Kind == AddrLabelDiff)
    ((AddrLabelDiffData *)(char *)&Data)->~AddrLabelDiffData();
  else if (Kind == Reflection)
    ((ReflectionData *)(char *)&Data)->~ReflectionData();
  Kind = None;
}

bool APValue::needsCleanup() const {
  switch (Kind) {
  case None:
  case Indeterminate:
  case AddrLabelDiff:
    return false;
  case Struct:
  case Union:
  case Array:
  case Vector:
  case Reflection:
    return true;
  case Int:
    return getInt().needsCleanup();
  case Float:
    return getFloat().needsCleanup();
  case FixedPoint:
    return getFixedPoint().getValue().needsCleanup();
  case ComplexFloat:
    assert(getComplexFloatImag().needsCleanup() ==
               getComplexFloatReal().needsCleanup() &&
           "In _Complex float types, real and imaginary values always have the "
           "same size.");
    return getComplexFloatReal().needsCleanup();
  case ComplexInt:
    assert(getComplexIntImag().needsCleanup() ==
               getComplexIntReal().needsCleanup() &&
           "In _Complex int types, real and imaginary values must have the "
           "same size.");
    return getComplexIntReal().needsCleanup();
  case LValue:
    return reinterpret_cast<const LV *>(&Data)->hasPathPtr();
  case MemberPointer:
    return reinterpret_cast<const MemberPointerData *>(&Data)->hasPathPtr();
  }
  llvm_unreachable("Unknown APValue kind!");
}

void APValue::swap(APValue &RHS) {
  std::swap(Kind, RHS.Kind);
  std::swap(Data, RHS.Data);
  std::swap(UnderlyingTy, RHS.UnderlyingTy);
  std::swap(ReflectionDepth, RHS.ReflectionDepth);
}

/// Profile the value of an APInt, excluding its bit-width.
static void profileIntValue(llvm::FoldingSetNodeID &ID, const llvm::APInt &V) {
  for (unsigned I = 0, N = V.getBitWidth(); I < N; I += 32)
    ID.AddInteger((uint32_t)V.extractBitsAsZExtValue(std::min(32u, N - I), I));
}

static void profileReflection(llvm::FoldingSetNodeID &ID, APValue V) {
  while (V.getReflectionDepth() > 0)
    V = V.Lower();

  ID.AddInteger(static_cast<int>(V.getReflectionKind()));

  switch (V.getReflectionKind()) {
  case ReflectionKind::Null:
    return;
  case ReflectionKind::Type: {
    QualType QT = V.getReflectedType();
    QT.getQualifiers().Profile(ID);

    if (auto *TST = dyn_cast<TemplateSpecializationType>(QT)) {
      // Note: This sugar only kept for alias template specializations.
      ID.AddInteger(Type::TemplateSpecialization);
      ID.AddPointer(TST->getTemplateName().getAsTemplateDecl());
      if (auto *D = QT->getAsRecordDecl())
        ID.AddPointer(D->getCanonicalDecl());
    } else {
      ID.AddInteger(0);
      if (auto *TDT = dyn_cast<TypedefType>(QT)) {
        ID.AddBoolean(true);
        ID.AddPointer(TDT->getDecl());
      } else {
        ID.AddBoolean(false);
        QT.getCanonicalType().Profile(ID);
      }
    }
    return;
  }
  case ReflectionKind::Declaration:
    if (auto *PVD = dyn_cast<ParmVarDecl>(V.getReflectedDecl())) {
      auto *FD = cast<FunctionDecl>(PVD->getDeclContext())->getFirstDecl();
      PVD = FD->getParamDecl(PVD->getFunctionScopeIndex());
      ID.AddPointer(PVD);
    } else {
      ID.AddPointer(V.getReflectedDecl());
    }
    return;
  case ReflectionKind::Template: {
    TemplateDecl *TDecl = V.getReflectedTemplate().getAsTemplateDecl();
    if (auto *RTD = dyn_cast<RedeclarableTemplateDecl>(TDecl))
      TDecl = RTD->getCanonicalDecl();
    ID.AddPointer(TDecl);
    return;
  }
  case ReflectionKind::Namespace:
  case ReflectionKind::BaseSpecifier:
    ID.AddPointer(V.getOpaqueReflectionData());
    return;
  case ReflectionKind::DataMemberSpec: {
    TagDataMemberSpec *TDMS = V.getReflectedDataMemberSpec();
    TDMS->Ty.Profile(ID);
    ID.AddBoolean(TDMS->Name.has_value());
    if (TDMS->Name)
      ID.AddString(TDMS->Name.value());
    ID.AddBoolean(TDMS->Alignment.has_value());
    if (TDMS->Alignment)
      ID.AddInteger(TDMS->Alignment.value());
    ID.AddBoolean(TDMS->BitWidth.has_value());
    if (TDMS->BitWidth)
      ID.AddInteger(TDMS->BitWidth.value());
    return;
  }
  case ReflectionKind::Object:
  case ReflectionKind::Value:
    llvm_unreachable("lowered value should never represent a value or object");
  }
  llvm_unreachable("unknown reflection kind");
}

void APValue::Profile(llvm::FoldingSetNodeID &ID) const {
  // Note that our profiling assumes that only APValues of the same type are
  // ever compared. As a result, we don't consider collisions that could only
  // happen if the types are different. (For example, structs with different
  // numbers of members could profile the same.)

  ID.AddInteger(Kind);

  // Profile the reflection depth and underlying type (if any).
  ID.AddInteger(ReflectionDepth);
  if (isReflectedValue())
    UnderlyingTy.getCanonicalType().getUnqualifiedType().Profile(ID);

  switch (Kind) {
  case None:
  case Indeterminate:
    return;

  case AddrLabelDiff:
    ID.AddPointer(getAddrLabelDiffLHS()->getLabel()->getCanonicalDecl());
    ID.AddPointer(getAddrLabelDiffRHS()->getLabel()->getCanonicalDecl());
    return;

  case Struct:
    for (unsigned I = 0, N = getStructNumBases(); I != N; ++I)
      getStructBase(I).Profile(ID);
    for (unsigned I = 0, N = getStructNumFields(); I != N; ++I)
      getStructField(I).Profile(ID);
    return;

  case Union:
    if (!getUnionField()) {
      ID.AddInteger(0);
      return;
    }
    ID.AddInteger(getUnionField()->getFieldIndex() + 1);
    getUnionValue().Profile(ID);
    return;

  case Array: {
    if (getArraySize() == 0)
      return;

    // The profile should not depend on whether the array is expanded or
    // not, but we don't want to profile the array filler many times for
    // a large array. So treat all equal trailing elements as the filler.
    // Elements are profiled in reverse order to support this, and the
    // first profiled element is followed by a count. For example:
    //
    //   ['a', 'c', 'x', 'x', 'x'] is profiled as
    //   [5, 'x', 3, 'c', 'a']
    llvm::FoldingSetNodeID FillerID;
    (hasArrayFiller() ? getArrayFiller()
                      : getArrayInitializedElt(getArrayInitializedElts() - 1))
        .Profile(FillerID);
    ID.AddNodeID(FillerID);
    unsigned NumFillers = getArraySize() - getArrayInitializedElts();
    unsigned N = getArrayInitializedElts();

    // Count the number of elements equal to the last one. This loop ends
    // by adding an integer indicating the number of such elements, with
    // N set to the number of elements left to profile.
    while (true) {
      if (N == 0) {
        // All elements are fillers.
        assert(NumFillers == getArraySize());
        ID.AddInteger(NumFillers);
        break;
      }

      // No need to check if the last element is equal to the last
      // element.
      if (N != getArraySize()) {
        llvm::FoldingSetNodeID ElemID;
        getArrayInitializedElt(N - 1).Profile(ElemID);
        if (ElemID != FillerID) {
          ID.AddInteger(NumFillers);
          ID.AddNodeID(ElemID);
          --N;
          break;
        }
      }

      // This is a filler.
      ++NumFillers;
      --N;
    }

    // Emit the remaining elements.
    for (; N != 0; --N)
      getArrayInitializedElt(N - 1).Profile(ID);
    return;
  }

  case Vector:
    for (unsigned I = 0, N = getVectorLength(); I != N; ++I)
      getVectorElt(I).Profile(ID);
    return;

  case Int:
    profileIntValue(ID, getInt());
    return;

  case Float:
    profileIntValue(ID, getFloat().bitcastToAPInt());
    return;

  case FixedPoint:
    profileIntValue(ID, getFixedPoint().getValue());
    return;

  case ComplexFloat:
    profileIntValue(ID, getComplexFloatReal().bitcastToAPInt());
    profileIntValue(ID, getComplexFloatImag().bitcastToAPInt());
    return;

  case ComplexInt:
    profileIntValue(ID, getComplexIntReal());
    profileIntValue(ID, getComplexIntImag());
    return;

  case LValue:
    getLValueBase().Profile(ID);
    ID.AddInteger(getLValueOffset().getQuantity());
    ID.AddInteger((isNullPointer() ? 1 : 0) |
                  (isLValueOnePastTheEnd() ? 2 : 0) |
                  (hasLValuePath() ? 4 : 0));
    if (hasLValuePath()) {
      ID.AddInteger(getLValuePath().size());
      // For uniqueness, we only need to profile the entries corresponding
      // to union members, but we don't have the type here so we don't know
      // how to interpret the entries.
      for (LValuePathEntry E : getLValuePath())
        E.Profile(ID);
    }
    return;

  case MemberPointer:
    ID.AddPointer(getMemberPointerDecl());
    ID.AddInteger(isMemberPointerToDerivedMember());
    for (const CXXRecordDecl *D : getMemberPointerPath())
      ID.AddPointer(D);
    return;
  case Reflection:
    profileReflection(ID, *this);
    return;
  }

  llvm_unreachable("Unknown APValue kind!");
}

QualType APValue::getTypeOfReflectedResult(const ASTContext &C) const {
  assert((isReflectedValue() || isReflectedObject()) &&
         "not a reflection of a value or object");
  if (getReflectionDepth() == 1)
    return UnderlyingTy;
  return C.MetaInfoTy;
}

ReflectionKind APValue::getReflectionKind() const {
  assert(isReflection() && "not a reflection value");

  switch (getReflectionDepth()) {
    // In case '0', this is a reflection of something other than a value or an
    // object. Its kind is stored in the Data of the APValue.
    case 0: {
      ReflectionKind RK = ((const ReflectionData *)(const char *)&Data)->Kind;
      assert((RK != ReflectionKind::Value && RK != ReflectionKind::Object) &&
             "Value and Object should never be stored as Kind");

      return RK;
    }

    // In case '1', this is either a reflection of a value or of an object.
    // A few cases need to be considered to determine which.
    case 1:
      // If no type was provided, then the type must be inferrable from the
      // LValue. IT must be an object.
      if (UnderlyingTy.isNull()) {
        return ReflectionKind::Object;

        // Any APValue which is not an LValue is assumed to be a value.
      } else if (Kind == LValue) {

        // Handle the odd nullptr_t corner case, which is a value.
        if (getLValueBase().isNull())
          return ReflectionKind::Value;

        // The only other LValue-kind APValues that we consider values are those
        // that are pointers and blocks. For all others, consider it an object.
        else if (!UnderlyingTy->isPointerType() &&
                 !UnderlyingTy->isBlockPointerType())
          return ReflectionKind::Object;

        // We were give a pointer type, and we'll need to do some work to
        // disambiguate between a value and an object.
        //
        // - A pointer value will be an LValue whose "thing it is pointing to"
        //   has a different type than itself (e.g., int * vs int).
        // - An object that happens to have pointer type will be an LValue whose
        //   (normalized) type is the same as its 'UnderlyingTy'.
        const Type *LVTy = nullptr;

        // If we have an LValuePath, use the type of the back-most path element.
        if (hasLValuePath() && getLValuePath().size() > 0) {
          const LValuePathEntry &E = getLValuePath().back();
          if (const auto *D = E.getAsBaseOrMember().getPointer()) {
            if (auto *FD = dyn_cast<FieldDecl>(D))
              LVTy = FD->getType()->getCanonicalTypeUnqualified().getTypePtr();
            else if (auto *TD = dyn_cast<CXXRecordDecl>(D))
              LVTy = TD->getTypeForDecl()
                        ->getCanonicalTypeUnqualified().getTypePtr();
          }
        }

        // Otherwise, infer from the LValueBase.
        if (!LVTy) {
          if (auto *B = getLValueBase().dyn_cast<const ValueDecl *>()) {
            LVTy = B->getType()->getCanonicalTypeUnqualified().getTypePtr();
          } else if (auto *B = getLValueBase().dyn_cast<const Expr *>()) {
            // If the base expression isn't an lvalue, it must be an object.
            if (!B->isLValue())
              return ReflectionKind::Value;

            LVTy = B->getType()->getCanonicalTypeUnqualified().getTypePtr();
          }
        }
        assert(LVTy);

        // Equivalent types means it's an object; otherwise, assume a value.
        if (LVTy == UnderlyingTy->getCanonicalTypeUnqualified().getTypePtr()) {
          return ReflectionKind::Object;
        }
      }
      return ReflectionKind::Value;
    default:
      // Any APValue whose reflection depth is higher than 1 is a reflection of
      // a value; the type is 'std::meta::info'.
      return ReflectionKind::Value;
  }
}

static QualType ComputeLValueType(const APValue &V) {
  assert(V.isLValue());
  if (V.getLValueBase().isNull())
    return QualType {};

  SplitQualType SQT = V.getLValueBase().getType().split();

  for (auto p = V.getLValuePath().begin();
       p != V.getLValuePath().end(); ++p) {
    const Decl *D = V.getLValuePath().back().getAsBaseOrMember().getPointer();
    if (D) {  // base or member case
      if (auto *VD = dyn_cast<FieldDecl>(D)) {
        QualType QT = VD->getType();
        SQT.Ty = QT.getTypePtr();

        if (QT.isConstQualified()) SQT.Quals.addConst();
        if (QT.isVolatileQualified()) SQT.Quals.addVolatile();

        continue;
      } else if (auto *TD = dyn_cast<CXXRecordDecl>(D)) {
        SQT.Ty = TD->getTypeForDecl();
        continue;
      }

      llvm_unreachable("unknown lvalue path kind");
    } else { // array case
      QualType QT = cast<ArrayType>(SQT.Ty)->getElementType();
      SQT.Ty = QT.getTypePtr();
      if (QT.isConstQualified()) SQT.Quals.addConst();
      if (QT.isVolatileQualified()) SQT.Quals.addVolatile();
    }
  }
  return QualType(SQT.Ty, SQT.Quals.getAsOpaqueValue());
}

APValue APValue::Lift(QualType ResultType) const {
  assert(ReflectionDepth <
         std::numeric_limits<decltype(ReflectionDepth)>::max());

  // TODO: Special case for lvalues referring to functions?
  //       Should be "promoted" to a reflection of the function declaration.

  APValue Result(*this);
  ++Result.ReflectionDepth;

  if (Result.ReflectionDepth == 1) {
    Result.UnderlyingTy = ResultType;

    if (Result.isReflectedObject())
      Result.UnderlyingTy = ComputeLValueType(*this);
    else
      assert(Result.isReflectedValue() && "not a value or an object?");
  }
  return Result;
}

APValue APValue::Lower() const {
  assert(getReflectionDepth() > 0 && "not a reflection");

  APValue Result(*this);
  --Result.ReflectionDepth;
  return Result;
}

const void *APValue::getOpaqueReflectionData() const {
  assert(isReflection() && "not a reflection value");
  return ((const ReflectionData *)(const void *)&Data)->Data;
}

QualType APValue::getReflectedType() const {
  assert(getReflectionKind() == ReflectionKind::Type &&
         "not a reflection of a type");
  return QualType::getFromOpaquePtr(getOpaqueReflectionData());
}

APValue APValue::getReflectedObject() const {
  assert(getReflectionKind() == ReflectionKind::Object &&
         "not a reflection of an object");
  assert(getReflectionKind() == ReflectionKind::Object);
  return Lower();
}

APValue APValue::getReflectedValue() const {
  assert(getReflectionKind() == ReflectionKind::Value &&
         "not a reflection of a value");
  return Lower();
}

ValueDecl *APValue::getReflectedDecl() const {
  assert(getReflectionKind() == ReflectionKind::Declaration &&
         "not a reflection of a declaration");
  return reinterpret_cast<ValueDecl *>(
          const_cast<void *>(getOpaqueReflectionData()));
}

const TemplateName APValue::getReflectedTemplate() const {
  assert(getReflectionKind() == ReflectionKind::Template &&
         "not a reflection of a template");
  return TemplateName::getFromVoidPointer(
          const_cast<void *>(getOpaqueReflectionData()));
}

Decl *APValue::getReflectedNamespace() const {
  assert(getReflectionKind() == ReflectionKind::Namespace &&
         "not a reflection of a namespace");
  return reinterpret_cast<Decl *>(
          const_cast<void *>(getOpaqueReflectionData()));
}

CXXBaseSpecifier *APValue::getReflectedBaseSpecifier() const {
  assert(getReflectionKind() == ReflectionKind::BaseSpecifier &&
         "not a reflection of a base specifier");
  return reinterpret_cast<CXXBaseSpecifier *>(
          const_cast<void *>(getOpaqueReflectionData()));
}

TagDataMemberSpec *APValue::getReflectedDataMemberSpec() const {
  assert(getReflectionKind() == ReflectionKind::DataMemberSpec &&
         "not a reflection of a description of a data member");
  return reinterpret_cast<TagDataMemberSpec *>(
          const_cast<void *>(getOpaqueReflectionData()));
}

static double GetApproxValue(const llvm::APFloat &F) {
  llvm::APFloat V = F;
  bool ignored;
  V.convert(llvm::APFloat::IEEEdouble(), llvm::APFloat::rmNearestTiesToEven,
            &ignored);
  return V.convertToDouble();
}

static bool TryPrintAsStringLiteral(raw_ostream &Out,
                                    const PrintingPolicy &Policy,
                                    const ArrayType *ATy,
                                    ArrayRef<APValue> Inits) {
  if (Inits.empty())
    return false;

  QualType Ty = ATy->getElementType();
  if (!Ty->isAnyCharacterType())
    return false;

  // Nothing we can do about a sequence that is not null-terminated
  if (!Inits.back().isInt() || !Inits.back().getInt().isZero())
    return false;

  Inits = Inits.drop_back();

  llvm::SmallString<40> Buf;
  Buf.push_back('"');

  // Better than printing a two-digit sequence of 10 integers.
  constexpr size_t MaxN = 36;
  StringRef Ellipsis;
  if (Inits.size() > MaxN && !Policy.EntireContentsOfLargeArray) {
    Ellipsis = "[...]";
    Inits =
        Inits.take_front(std::min(MaxN - Ellipsis.size() / 2, Inits.size()));
  }

  for (auto &Val : Inits) {
    if (!Val.isInt())
      return false;
    int64_t Char64 = Val.getInt().getExtValue();
    if (!isASCII(Char64))
      return false; // Bye bye, see you in integers.
    auto Ch = static_cast<unsigned char>(Char64);
    // The diagnostic message is 'quoted'
    StringRef Escaped = escapeCStyle<EscapeChar::SingleAndDouble>(Ch);
    if (Escaped.empty()) {
      if (!isPrintable(Ch))
        return false;
      Buf.emplace_back(Ch);
    } else {
      Buf.append(Escaped);
    }
  }

  Buf.append(Ellipsis);
  Buf.push_back('"');

  if (Ty->isWideCharType())
    Out << 'L';
  else if (Ty->isChar8Type())
    Out << "u8";
  else if (Ty->isChar16Type())
    Out << 'u';
  else if (Ty->isChar32Type())
    Out << 'U';

  Out << Buf;
  return true;
}

void APValue::printPretty(raw_ostream &Out, const ASTContext &Ctx,
                          QualType Ty) const {
  printPretty(Out, Ctx.getPrintingPolicy(), Ty, &Ctx);
}

void APValue::printPretty(raw_ostream &Out, const PrintingPolicy &Policy,
                          QualType Ty, const ASTContext *Ctx) const {
  // There are no objects of type 'void', but values of this type can be
  // returned from functions.
  if (Ty->isVoidType()) {
    Out << "void()";
    return;
  }

  if (const auto *AT = Ty->getAs<AtomicType>())
    Ty = AT->getValueType();

  switch (Kind) {
  case APValue::None:
    Out << "<out of lifetime>";
    return;
  case APValue::Indeterminate:
    Out << "<uninitialized>";
    return;
  case APValue::Int:
    if (Ty->isBooleanType())
      Out << (getInt().getBoolValue() ? "true" : "false");
    else
      Out << getInt();
    return;
  case APValue::Float:
    Out << GetApproxValue(getFloat());
    return;
  case APValue::FixedPoint:
    Out << getFixedPoint();
    return;
  case APValue::Vector: {
    Out << '{';
    QualType ElemTy = Ty->castAs<VectorType>()->getElementType();
    getVectorElt(0).printPretty(Out, Policy, ElemTy, Ctx);
    for (unsigned i = 1; i != getVectorLength(); ++i) {
      Out << ", ";
      getVectorElt(i).printPretty(Out, Policy, ElemTy, Ctx);
    }
    Out << '}';
    return;
  }
  case APValue::ComplexInt:
    Out << getComplexIntReal() << "+" << getComplexIntImag() << "i";
    return;
  case APValue::ComplexFloat:
    Out << GetApproxValue(getComplexFloatReal()) << "+"
        << GetApproxValue(getComplexFloatImag()) << "i";
    return;
  case APValue::LValue: {
    bool IsReference = Ty->isReferenceType();
    QualType InnerTy
      = IsReference ? Ty.getNonReferenceType() : Ty->getPointeeType();
    if (InnerTy.isNull())
      InnerTy = Ty;

    LValueBase Base = getLValueBase();
    if (!Base) {
      if (isNullPointer()) {
        Out << (Policy.Nullptr ? "nullptr" : "0");
      } else if (IsReference) {
        Out << "*(" << InnerTy.stream(Policy) << "*)"
            << getLValueOffset().getQuantity();
      } else {
        Out << "(" << Ty.stream(Policy) << ")"
            << getLValueOffset().getQuantity();
      }
      return;
    }

    if (!hasLValuePath()) {
      // No lvalue path: just print the offset.
      CharUnits O = getLValueOffset();
      CharUnits S = Ctx ? Ctx->getTypeSizeInCharsIfKnown(InnerTy).value_or(
                              CharUnits::Zero())
                        : CharUnits::Zero();
      if (!O.isZero()) {
        if (IsReference)
          Out << "*(";
        if (S.isZero() || O % S) {
          Out << "(char*)";
          S = CharUnits::One();
        }
        Out << '&';
      } else if (!IsReference) {
        Out << '&';
      }

      if (const ValueDecl *VD = Base.dyn_cast<const ValueDecl*>())
        Out << *VD;
      else if (TypeInfoLValue TI = Base.dyn_cast<TypeInfoLValue>()) {
        TI.print(Out, Policy);
      } else if (DynamicAllocLValue DA = Base.dyn_cast<DynamicAllocLValue>()) {
        Out << "{*new "
            << Base.getDynamicAllocType().stream(Policy) << "#"
            << DA.getIndex() << "}";
      } else {
        assert(Base.get<const Expr *>() != nullptr &&
               "Expecting non-null Expr");
        Base.get<const Expr*>()->printPretty(Out, nullptr, Policy);
      }

      if (!O.isZero()) {
        Out << " + " << (O / S);
        if (IsReference)
          Out << ')';
      }
      return;
    }

    // We have an lvalue path. Print it out nicely.
    if (!IsReference)
      Out << '&';
    else if (isLValueOnePastTheEnd())
      Out << "*(&";

    QualType ElemTy = Base.getType();
    if (const ValueDecl *VD = Base.dyn_cast<const ValueDecl*>()) {
      Out << *VD;
    } else if (TypeInfoLValue TI = Base.dyn_cast<TypeInfoLValue>()) {
      TI.print(Out, Policy);
    } else if (DynamicAllocLValue DA = Base.dyn_cast<DynamicAllocLValue>()) {
      Out << "{*new " << Base.getDynamicAllocType().stream(Policy) << "#"
          << DA.getIndex() << "}";
    } else {
      const Expr *E = Base.get<const Expr*>();
      assert(E != nullptr && "Expecting non-null Expr");
      E->printPretty(Out, nullptr, Policy);
    }

    ArrayRef<LValuePathEntry> Path = getLValuePath();
    const CXXRecordDecl *CastToBase = nullptr;
    for (unsigned I = 0, N = Path.size(); I != N; ++I) {
      if (ElemTy->isRecordType()) {
        // The lvalue refers to a class type, so the next path entry is a base
        // or member.
        const Decl *BaseOrMember = Path[I].getAsBaseOrMember().getPointer();
        if (const CXXRecordDecl *RD = dyn_cast<CXXRecordDecl>(BaseOrMember)) {
          CastToBase = RD;
          // Leave ElemTy referring to the most-derived class. The actual type
          // doesn't matter except for array types.
        } else {
          const ValueDecl *VD = cast<ValueDecl>(BaseOrMember);
          Out << ".";
          if (CastToBase)
            Out << *CastToBase << "::";
          Out << *VD;
          ElemTy = VD->getType();
        }
      } else if (ElemTy->isAnyComplexType()) {
        // The lvalue refers to a complex type
        Out << (Path[I].getAsArrayIndex() == 0 ? ".real" : ".imag");
        ElemTy = ElemTy->castAs<ComplexType>()->getElementType();
      } else {
        // The lvalue must refer to an array.
        Out << '[' << Path[I].getAsArrayIndex() << ']';
        ElemTy = ElemTy->castAsArrayTypeUnsafe()->getElementType();
      }
    }

    // Handle formatting of one-past-the-end lvalues.
    if (isLValueOnePastTheEnd()) {
      // FIXME: If CastToBase is non-0, we should prefix the output with
      // "(CastToBase*)".
      Out << " + 1";
      if (IsReference)
        Out << ')';
    }
    return;
  }
  case APValue::Array: {
    const ArrayType *AT = Ty->castAsArrayTypeUnsafe();
    unsigned N = getArrayInitializedElts();
    if (N != 0 && TryPrintAsStringLiteral(Out, Policy, AT,
                                          {&getArrayInitializedElt(0), N}))
      return;
    QualType ElemTy = AT->getElementType();
    Out << '{';
    unsigned I = 0;
    switch (N) {
    case 0:
      for (; I != N; ++I) {
        Out << ", ";
        if (I == 10 && !Policy.EntireContentsOfLargeArray) {
          Out << "...}";
          return;
        }
        [[fallthrough]];
      default:
        getArrayInitializedElt(I).printPretty(Out, Policy, ElemTy, Ctx);
      }
    }
    Out << '}';
    return;
  }
  case APValue::Struct: {
    Out << '{';
    const RecordDecl *RD = Ty->castAs<RecordType>()->getDecl();
    bool First = true;
    if (unsigned N = getStructNumBases()) {
      const CXXRecordDecl *CD = cast<CXXRecordDecl>(RD);
      CXXRecordDecl::base_class_const_iterator BI = CD->bases_begin();
      for (unsigned I = 0; I != N; ++I, ++BI) {
        assert(BI != CD->bases_end());
        if (!First)
          Out << ", ";
        getStructBase(I).printPretty(Out, Policy, BI->getType(), Ctx);
        First = false;
      }
    }
    for (const auto *FI : RD->fields()) {
      if (!First)
        Out << ", ";
      if (FI->isUnnamedBitField())
        continue;
      getStructField(FI->getFieldIndex()).
        printPretty(Out, Policy, FI->getType(), Ctx);
      First = false;
    }
    Out << '}';
    return;
  }
  case APValue::Union:
    Out << '{';
    if (const FieldDecl *FD = getUnionField()) {
      Out << "." << *FD << " = ";
      getUnionValue().printPretty(Out, Policy, FD->getType(), Ctx);
    }
    Out << '}';
    return;
  case APValue::MemberPointer:
    // FIXME: This is not enough to unambiguously identify the member in a
    // multiple-inheritance scenario.
    if (const ValueDecl *VD = getMemberPointerDecl()) {
      Out << '&' << *cast<CXXRecordDecl>(VD->getDeclContext()) << "::" << *VD;
      return;
    }
    Out << "0";
    return;
  case APValue::AddrLabelDiff:
    Out << "&&" << getAddrLabelDiffLHS()->getLabel()->getName();
    Out << " - ";
    Out << "&&" << getAddrLabelDiffRHS()->getLabel()->getName();
    return;
  case APValue::Reflection:
    std::string Repr("unknown-reflection");
    switch (getReflectionKind()) {
    case ReflectionKind::Null:
      Repr = "null";
      break;
    case ReflectionKind::Type:
      Repr = "type";
      break;
    case ReflectionKind::Object:
      Repr = "object";
      break;
    case ReflectionKind::Value:
      Repr = "value";
      break;
    case ReflectionKind::Declaration:
      Repr = "declaration";
      break;
    case ReflectionKind::Template:
      Repr = "template";
      break;
    case ReflectionKind::Namespace:
      Repr = "namespace";
      break;
    case ReflectionKind::BaseSpecifier:
      Repr = "base-specifier";
      break;
    case ReflectionKind::DataMemberSpec:
      Repr = "data-member-spec";
      break;
    }
    Out << "^(" << Repr << ")";
    return;
  }
  llvm_unreachable("Unknown APValue kind!");
}

std::string APValue::getAsString(const ASTContext &Ctx, QualType Ty) const {
  std::string Result;
  llvm::raw_string_ostream Out(Result);
  printPretty(Out, Ctx, Ty);
  Out.flush();
  return Result;
}

bool APValue::toIntegralConstant(APSInt &Result, QualType SrcTy,
                                 const ASTContext &Ctx) const {
  if (isInt()) {
    Result = getInt();
    return true;
  }

  if (isLValue() && isNullPointer()) {
    Result = Ctx.MakeIntValue(Ctx.getTargetNullPointerValue(SrcTy), SrcTy);
    return true;
  }

  if (isLValue() && !getLValueBase()) {
    Result = Ctx.MakeIntValue(getLValueOffset().getQuantity(), SrcTy);
    return true;
  }

  return false;
}

const APValue::LValueBase APValue::getLValueBase() const {
  assert(Kind == LValue && "Invalid accessor");
  return ((const LV *)(const void *)&Data)->Base;
}

bool APValue::isLValueOnePastTheEnd() const {
  assert(Kind == LValue && "Invalid accessor");
  return ((const LV *)(const void *)&Data)->IsOnePastTheEnd;
}

CharUnits &APValue::getLValueOffset() {
  assert(Kind == LValue && "Invalid accessor");
  return ((LV *)(void *)&Data)->Offset;
}

bool APValue::hasLValuePath() const {
  assert(Kind == LValue && "Invalid accessor");
  return ((const LV *)(const char *)&Data)->hasPath();
}

ArrayRef<APValue::LValuePathEntry> APValue::getLValuePath() const {
  assert(Kind == LValue && hasLValuePath() && "Invalid accessor");
  const LV &LVal = *((const LV *)(const char *)&Data);
  return llvm::ArrayRef(LVal.getPath(), LVal.PathLength);
}

unsigned APValue::getLValueCallIndex() const {
  assert(Kind == LValue && "Invalid accessor");
  return ((const LV *)(const char *)&Data)->Base.getCallIndex();
}

unsigned APValue::getLValueVersion() const {
  assert(Kind == LValue && "Invalid accessor");
  return ((const LV *)(const char *)&Data)->Base.getVersion();
}

bool APValue::isNullPointer() const {
  assert(Kind == LValue && "Invalid usage");
  return ((const LV *)(const char *)&Data)->IsNullPtr;
}

void APValue::setLValue(LValueBase B, const CharUnits &O, NoLValuePath,
                        bool IsNullPtr) {
  assert(isLValue() && "Invalid accessor");
  LV &LVal = *((LV *)(char *)&Data);
  LVal.Base = B;
  LVal.IsOnePastTheEnd = false;
  LVal.Offset = O;
  LVal.resizePath((unsigned)-1);
  LVal.IsNullPtr = IsNullPtr;
}

MutableArrayRef<APValue::LValuePathEntry>
APValue::setLValueUninit(LValueBase B, const CharUnits &O, unsigned Size,
                         bool IsOnePastTheEnd, bool IsNullPtr) {
  assert(isLValue() && "Invalid accessor");
  LV &LVal = *((LV *)(char *)&Data);
  LVal.Base = B;
  LVal.IsOnePastTheEnd = IsOnePastTheEnd;
  LVal.Offset = O;
  LVal.IsNullPtr = IsNullPtr;
  LVal.resizePath(Size);
  return {LVal.getPath(), Size};
}

void APValue::setLValue(LValueBase B, const CharUnits &O,
                        ArrayRef<LValuePathEntry> Path, bool IsOnePastTheEnd,
                        bool IsNullPtr) {
  MutableArrayRef<APValue::LValuePathEntry> InternalPath =
      setLValueUninit(B, O, Path.size(), IsOnePastTheEnd, IsNullPtr);
  if (Path.size()) {
    memcpy(InternalPath.data(), Path.data(),
           Path.size() * sizeof(LValuePathEntry));
  }
}

void APValue::setUnion(const FieldDecl *Field, const APValue &Value) {
  assert(isUnion() && "Invalid accessor");
  ((UnionData *)(char *)&Data)->Field =
      Field ? Field->getCanonicalDecl() : nullptr;
  *((UnionData *)(char *)&Data)->Value = Value;
}

const ValueDecl *APValue::getMemberPointerDecl() const {
  assert(Kind == MemberPointer && "Invalid accessor");
  const MemberPointerData &MPD =
      *((const MemberPointerData *)(const char *)&Data);
  return MPD.MemberAndIsDerivedMember.getPointer();
}

bool APValue::isMemberPointerToDerivedMember() const {
  assert(Kind == MemberPointer && "Invalid accessor");
  const MemberPointerData &MPD =
      *((const MemberPointerData *)(const char *)&Data);
  return MPD.MemberAndIsDerivedMember.getInt();
}

ArrayRef<const CXXRecordDecl*> APValue::getMemberPointerPath() const {
  assert(Kind == MemberPointer && "Invalid accessor");
  const MemberPointerData &MPD =
      *((const MemberPointerData *)(const char *)&Data);
  return llvm::ArrayRef(MPD.getPath(), MPD.PathLength);
}

void APValue::MakeLValue() {
  assert(isAbsent() && "Bad state change");
  static_assert(sizeof(LV) <= DataSize, "LV too big");
  new ((void *)(char *)&Data) LV();
  Kind = LValue;
}

void APValue::MakeArray(unsigned InitElts, unsigned Size) {
  assert(isAbsent() && "Bad state change");
  new ((void *)(char *)&Data) Arr(InitElts, Size);
  Kind = Array;
}

MutableArrayRef<APValue::LValuePathEntry>
setLValueUninit(APValue::LValueBase B, const CharUnits &O, unsigned Size,
                bool OnePastTheEnd, bool IsNullPtr);

MutableArrayRef<const CXXRecordDecl *>
APValue::setMemberPointerUninit(const ValueDecl *Member, bool IsDerivedMember,
                                unsigned Size) {
  assert(isAbsent() && "Bad state change");
  MemberPointerData *MPD = new ((void *)(char *)&Data) MemberPointerData;
  Kind = MemberPointer;
  MPD->MemberAndIsDerivedMember.setPointer(
      Member ? cast<ValueDecl>(Member->getCanonicalDecl()) : nullptr);
  MPD->MemberAndIsDerivedMember.setInt(IsDerivedMember);
  MPD->resizePath(Size);
  return {MPD->getPath(), MPD->PathLength};
}

void APValue::MakeMemberPointer(const ValueDecl *Member, bool IsDerivedMember,
                                ArrayRef<const CXXRecordDecl *> Path) {
  MutableArrayRef<const CXXRecordDecl *> InternalPath =
      setMemberPointerUninit(Member, IsDerivedMember, Path.size());
  for (unsigned I = 0; I != Path.size(); ++I)
    InternalPath[I] = Path[I]->getCanonicalDecl();
}

LinkageInfo LinkageComputer::getLVForValue(const APValue &V,
                                           LVComputationKind computation) {
  LinkageInfo LV = LinkageInfo::external();

  auto MergeLV = [&](LinkageInfo MergeLV) {
    LV.merge(MergeLV);
    return LV.getLinkage() == Linkage::Internal;
  };
  auto Merge = [&](const APValue &V) {
    return MergeLV(getLVForValue(V, computation));
  };

  switch (V.getKind()) {
  case APValue::None:
  case APValue::Indeterminate:
  case APValue::Int:
  case APValue::Float:
  case APValue::FixedPoint:
  case APValue::ComplexInt:
  case APValue::ComplexFloat:
  case APValue::Vector:
  case APValue::Reflection:
    break;

  case APValue::AddrLabelDiff:
    // Even for an inline function, it's not reasonable to treat a difference
    // between the addresses of labels as an external value.
    return LinkageInfo::internal();

  case APValue::Struct: {
    for (unsigned I = 0, N = V.getStructNumBases(); I != N; ++I)
      if (Merge(V.getStructBase(I)))
        break;
    for (unsigned I = 0, N = V.getStructNumFields(); I != N; ++I)
      if (Merge(V.getStructField(I)))
        break;
    break;
  }

  case APValue::Union:
    if (V.getUnionField())
      Merge(V.getUnionValue());
    break;

  case APValue::Array: {
    for (unsigned I = 0, N = V.getArrayInitializedElts(); I != N; ++I)
      if (Merge(V.getArrayInitializedElt(I)))
        break;
    if (V.hasArrayFiller())
      Merge(V.getArrayFiller());
    break;
  }

  case APValue::LValue: {
    if (!V.getLValueBase()) {
      // Null or absolute address: this is external.
    } else if (const auto *VD =
                   V.getLValueBase().dyn_cast<const ValueDecl *>()) {
      if (VD && MergeLV(getLVForDecl(VD, computation)))
        break;
    } else if (const auto TI = V.getLValueBase().dyn_cast<TypeInfoLValue>()) {
      if (MergeLV(getLVForType(*TI.getType(), computation)))
        break;
    } else if (const Expr *E = V.getLValueBase().dyn_cast<const Expr *>()) {
      // Almost all expression bases are internal. The exception is
      // lifetime-extended temporaries.
      // FIXME: These should be modeled as having the
      // LifetimeExtendedTemporaryDecl itself as the base.
      // FIXME: If we permit Objective-C object literals in template arguments,
      // they should not imply internal linkage.
      auto *MTE = dyn_cast<MaterializeTemporaryExpr>(E);
      if (!MTE || MTE->getStorageDuration() == SD_FullExpression)
        return LinkageInfo::internal();
      if (MergeLV(getLVForDecl(MTE->getExtendingDecl(), computation)))
        break;
    } else {
      assert(V.getLValueBase().is<DynamicAllocLValue>() &&
             "unexpected LValueBase kind");
      return LinkageInfo::internal();
    }
    // The lvalue path doesn't matter: pointers to all subobjects always have
    // the same visibility as pointers to the complete object.
    break;
  }

  case APValue::MemberPointer:
    if (const NamedDecl *D = V.getMemberPointerDecl())
      MergeLV(getLVForDecl(D, computation));
    // Note that we could have a base-to-derived conversion here to a member of
    // a derived class with less linkage/visibility. That's covered by the
    // linkage and visibility of the value's type.
    break;
  }

  return LV;
}

static QualType unwrapReflectedType(QualType QT) {
  bool UnwrapAliases = false;
  bool IsConst = QT.isConstQualified();
  bool IsVolatile = QT.isVolatileQualified();

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
    if (const auto *TST = dyn_cast<TemplateSpecializationType>(QT);
        TST && !TST->isTypeAlias()) {
      QT = TST->desugar();
    }
    if (const auto *DTST = dyn_cast<DeducedTemplateSpecializationType>(QT))
      QT = DTST->getDeducedType();
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

  if (IsConst)
    QT = QT.withConst();
  if (IsVolatile)
    QT = QT.withVolatile();

  return QT;
}

void APValue::setReflection(ReflectionKind RK, const void *Ptr) {
  ReflectionData &SelfData = *((ReflectionData *)(char *)&Data);
  switch (RK) {
  case ReflectionKind::Null:
    SelfData.Kind = RK;
    return;
  case ReflectionKind::Type: {
    QualType QT = unwrapReflectedType(QualType::getFromOpaquePtr(Ptr));

    SelfData.Kind = RK;
    SelfData.Data = QT.getAsOpaquePtr();
    return;
  }
  case ReflectionKind::Declaration:
  case ReflectionKind::Template:
  case ReflectionKind::Namespace:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
    SelfData.Kind = RK;
    SelfData.Data = Ptr;
    return;
  case ReflectionKind::Object:
  case ReflectionKind::Value:
    return;
  }
  assert(RK == ReflectionKind::Null && "unknown reflection kind");
}
