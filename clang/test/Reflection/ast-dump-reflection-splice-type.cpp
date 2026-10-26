// Test without serialization:
// RUN: %clang_cc1 -triple x86_64-unknown-unknown -Wno-unused-value -std=c++26 -freflection  \
// RUN:            -ast-dump %s -ast-dump-filter Test \
// RUN: | FileCheck --strict-whitespace --match-full-lines %s
//
// Test with serialization:
// RUN: %clang_cc1 -triple x86_64-unknown-unknown -Wno-unused-value -std=c++26 -freflection -emit-pch -o %t %s
// RUN: %clang_cc1 -x c++ -triple x86_64-unknown-unknown -Wno-unused-value -std=c++26 -freflection  \
// RUN:           -include-pch %t -ast-dump-all -ast-dump-filter Test /dev/null \
// RUN: | sed -e "s/ <undeserialized declarations>//" -e "s/ imported//" \
// RUN: | FileCheck --strict-whitespace --match-full-lines %s

int Test() {
    using Ty = [:^^int:];
    // CHECK:  | `-TypeAliasDecl {{.*}} <col:{{.*}}, col:{{.*}}> col:{{.*}} referenced Ty 'int'
    // CHECK-NEXT:  |   `-ReflectionSpliceType {{.*}} 'int' sugar
    // CHECK-NEXT:  |     |-SpliceSpecifier {{.*}} <col:{{.*}}, col:{{.*}}>
    // CHECK-NEXT:  |     | `-CXXReflectExpr {{.*}} <col:{{.*}}, col:{{.*}}> 'meta::info'
    // CHECK-NEXT:  |     `-BuiltinType {{.*}} 'int'

    Ty x = 1;
    return x;
    // CHECK:  `-ReturnStmt {{.*}} <line:{{.*}}, col:{{.*}}>
    // CHECK-NEXT:    `-ImplicitCastExpr {{.*}} <col:{{.*}}> 'Ty':'int' <LValueToRValue>
    // CHECK-NEXT:      `-ImplicitCastExpr {{.*}} <col:{{.*}}> 'Ty':'int' xvalue <NoOp>
    // CHECK-NEXT:        `-DeclRefExpr {{.*}} <col:{{.*}}> 'Ty':'int' lvalue Var {{.*}} 'x' 'Ty':'int'
}
