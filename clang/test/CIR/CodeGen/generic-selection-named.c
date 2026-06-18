// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -Wno-unused-value -std=c2y -fclangir -emit-cir %s -o %t.cir
// RUN: FileCheck --input-file=%t.cir %s -check-prefix=CIR
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -Wno-unused-value -std=c2y -fclangir -emit-llvm %s -o %t-cir.ll
// RUN: FileCheck --input-file=%t-cir.ll %s -check-prefix=LLVM
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -Wno-unused-value -std=c2y -emit-llvm %s -o %t.ll
// RUN: FileCheck --input-file=%t.ll %s -check-prefix=OGCG

void foo() {
  const int a = 1;
  const int r = _Generic(a, double: 1, float: 2, int x:  x, default: 4);

  int b = 1;
  _Generic(b, double: 1, float: 2, int x:  x, default: 4) = 5;
}

// CIR: %[[A:.*]] = cir.alloca !s32i, !cir.ptr<!s32i>, ["a", init, const]
// CIR: %[[RES:.*]] = cir.alloca !s32i, !cir.ptr<!s32i>, ["r", init, const]
// CIR: %[[X:.*]] = cir.alloca !s32i, !cir.ptr<!s32i>, ["x", init, const]
// CIR: %[[B:.*]] = cir.alloca !s32i, !cir.ptr<!s32i>, ["b", init]
// CIR: %[[X2:.*]] = cir.alloca !cir.ptr<!s32i>, !cir.ptr<!cir.ptr<!s32i>>, ["x", init, const]
// CIR: %[[A_INIT:.*]] = cir.const #cir.int<1> : !s32i
// CIR: cir.store{{.*}} %[[A_INIT]], %[[A]] : !s32i, !cir.ptr<!s32i>
// CIR: %[[A_RELOADED:.*]] = cir.load align(4) %[[A]] : !cir.ptr<!s32i>, !s32i
// CIR: cir.store{{.*}} %[[A_RELOADED]], %[[X]] : !s32i, !cir.ptr<!s32i>
// CIR: %[[X_RELOADED:.*]] = cir.load align(4) %[[X]] : !cir.ptr<!s32i>, !s32i
// CIR: cir.store{{.*}} %[[X_RELOADED:.*]], %[[RES]] : !s32i, !cir.ptr<!s32i>
// CIR: %[[B_INIT:.*]] = cir.const #cir.int<1> : !s32i
// CIR: cir.store{{.*}} %[[B_INIT]], %[[B]] : !s32i, !cir.ptr<!s32i>
// CIR: %[[FIVE:.*]] = cir.const #cir.int<5> : !s32i
// CIR: cir.store{{.*}} %[[B]], %[[X2]] : !cir.ptr<!s32i>, !cir.ptr<!cir.ptr<!s32i>>
// CIR: %[[X2_PTR:.*]] = cir.load %[[X2]] : !cir.ptr<!cir.ptr<!s32i>>, !cir.ptr<!s32i>
// CIR: cir.store{{.*}} %[[FIVE]], %[[X2_PTR]] : !s32i, !cir.ptr<!s32i>

// LLVM: %[[A:.*]] = alloca i32, i64 1, align 4
// LLVM: %[[RES:.*]] = alloca i32, i64 1, align 4
// LLVM: %[[X:.*]] = alloca i32, i64 1, align 4
// LLVM: %[[B:.*]] = alloca i32, i64 1, align 4
// LLVM: %[[X2:.*]] = alloca ptr, i64 1, align 8
// LLVM: store i32 1, ptr %[[A]], align 4
// LLVM: %[[A_RELOADED:.*]] = load i32, ptr %[[A]], align 4
// LLVM: store i32 %[[A_RELOADED]], ptr %[[X]], align 4
// LLVM: %[[X_RELOADED:.*]] = load i32, ptr %[[X]], align 4
// LLVM: store i32 %[[X_RELOADED]], ptr %[[RES]], align 4
// LLVM: store i32 1, ptr %[[B]], align 4
// LLVM: store ptr %[[B]], ptr %[[X2]], align 8
// LLVM: %[[X2_PTR:.*]] = load ptr, ptr %[[X2]], align 8
// LLVM: store i32 5, ptr %[[X2_PTR]], align 4

// OGCG: %[[A:.*]] = alloca i32, align 4
// OGCG: %[[RES:.*]] = alloca i32, align 4
// OGCG: %[[X:.*]] = alloca i32, align 4
// OGCG: %[[B:.*]] = alloca i32, align 4
// OGCG: %[[X2:.*]] = alloca ptr, align 8
// OGCG: store i32 1, ptr %[[A]], align 4
// OGCG: store i32 1, ptr %[[X]], align 4
// OGCG: store i32 1, ptr %[[RES]], align 4
// OGCG: store i32 1, ptr %[[B]], align 4
// OGCG: store ptr %[[B]], ptr %[[X2]], align 8
// OGCG: %[[X2_PTR:.*]] = load ptr, ptr %[[X2]]
// OGCG: store i32 5, ptr %[[X2_PTR]]