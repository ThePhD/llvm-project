// RUN: %clang_cc1 -std=c2y -emit-pch -o %t.ast %S/Inputs/named-generic.c
// RUN: %clang_cc1 -std=c2y -ast-merge %t.ast -fsyntax-only -verify %s
// expected-no-diagnostics
