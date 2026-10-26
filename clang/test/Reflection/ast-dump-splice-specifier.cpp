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

using info = decltype(^^int);

constexpr int f() { return 42; }
template <int T> constexpr int g() { return T; }

void Test() {
    int x = 11;
    int y = [:^^x:];
    // CHECK:  | `-VarDecl {{.*}} <col:{{.*}}, col:{{.*}}> col:{{.*}} {{.*}} x 'int' cinit
    // CHECK-NEXT:  |   `-IntegerLiteral {{.*}} <col:{{.*}}> 'int' 11

    // CHECK:  | `-VarDecl {{.*}} <col:{{.*}}, col:{{.*}}> col:{{.*}} y 'int' cinit
    // CHECK-NEXT:  |   `-ImplicitCastExpr {{.*}} <col:{{.*}}, col:{{.*}}> 'int' <LValueToRValue>
    // CHECK-NEXT:  |     `-CXXSpliceExpr {{.*}} <col:{{.*}}, col:{{.*}}> 'int' lvalue
    // CHECK-NEXT:  |       |-SpliceSpecifier {{.*}} <col:{{.*}}, col:{{.*}}>
    // CHECK-NEXT:  |       | `-CXXReflectExpr {{.*}} <col:{{.*}}, col:{{.*}}> 'meta::info'
    // CHECK-NEXT:  |       `-DeclRefExpr {{.*}} <col:{{.*}}> 'int' lvalue Var {{.*}} 'x' 'int'

    int w = [:^^f:]();
    // CHECK:  | `-VarDecl {{.*}} <col:{{.*}}, col:{{.*}}> col:{{.*}} w 'int' cinit
    // CHECK-NEXT:  |   `-CallExpr {{.*}} <col:{{.*}}, col:{{.*}}> 'int'
    // CHECK-NEXT:  |     `-ImplicitCastExpr {{.*}} <col:{{.*}}, col:{{.*}}> 'int (*)()' <FunctionToPointerDecay>
    // CHECK-NEXT:  |       `-CXXSpliceExpr {{.*}} <col:{{.*}}, col:{{.*}}> 'int ()' lvalue
    // CHECK-NEXT:  |         |-SpliceSpecifier {{.*}} <col:{{.*}}, col:{{.*}}>
    // CHECK-NEXT:  |         | `-CXXReflectExpr {{.*}} <col:{{.*}}, col:{{.*}}> 'meta::info'
    // CHECK-NEXT:  |         `-DeclRefExpr {{.*}} <col:{{.*}}> 'int ()' lvalue Function {{.*}} 'f' 'int ()'

    int z = [:^^g<42>:]();
    // CHECK:    `-VarDecl {{.*}} <col:{{.*}}, col:{{.*}}> col:{{.*}} z 'int' cinit
    // CHECK-NEXT:      `-CallExpr {{.*}} <col:{{.*}}, col:{{.*}}> 'int'
    // CHECK-NEXT:        `-ImplicitCastExpr {{.*}} <col:{{.*}}, col:{{.*}}> 'int (*)()' <FunctionToPointerDecay>
    // CHECK-NEXT:          `-CXXSpliceExpr {{.*}} <col:{{.*}}, col:{{.*}}> 'int ()' lvalue
    // CHECK-NEXT:            |-SpliceSpecifier {{.*}} <col:{{.*}}, col:{{.*}}>
    // CHECK-NEXT:            | `-CXXReflectExpr {{.*}} <col:{{.*}}, col:{{.*}}> 'meta::info'
    // CHECK-NEXT:            `-DeclRefExpr {{.*}} <col:{{.*}}> 'int ()' lvalue Function {{.*}} 'g' 'int ()'

}
