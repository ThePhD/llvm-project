// RUN: %clang_cc1 -verify=expected,c2y -std=c2y %s
// RUN: %clang_cc1 -verify -std=c2y -triple x86_64 -emit-llvm -o - %s -DNOERRORS | FileCheck %s

/* WG14 N38X3: Yes
 * Expression Evaluation and Access in _Generic
 *
 */

static void anchor() {}

static void association_with_id(void) {
  constexpr int rr = 5;
  constexpr int r = _Generic(rr,
      constexpr int a: a,
      double: 0xBAD,
      default: 0xBAD);
  _Static_assert(r == 5);
}

static void default_with_id(void) {
  constexpr int r = _Generic(5,
      double: 0xDAB,
      default d: 0xBAD);
  static_assert(r == 0xBAD);
}

struct foo {
  char *name;
  int n;
};

#define writethru(x) _Generic(x, int a: a, struct foo v: v.name, default: 0)

// CHECK-LABEL: define {{.*}} void @lvalue()
static void lvalue(void) {
  // CHECK: %f = alloca %struct.foo, align 8
  // CHECK-NEXT: %v = alloca ptr, align 8
  // CHECK-NEXT: %w = alloca i32, align 4
  // CHECK-NEXT: %a = alloca ptr, align 8
  // CHECK-NEXT: call void @llvm.memcpy.p0.p0.i64(ptr align 8 %f, ptr align 8 @__const.lvalue.f, i64 16, i1 false)
  struct foo f = {"test"};

  // CHECK-NEXT: store ptr %f, ptr %v, align 8
  // CHECK-NEXT: %0 = load ptr, ptr %v, align 8
  // CHECK-NEXT: %name = getelementptr inbounds nuw %struct.foo, ptr %0, i32 0, i32 0
  // CHECK-NEXT: store ptr @.str.1, ptr %name, align 8
  writethru(f) = "something";

  // CHECK-NEXT: store i32 0, ptr %w, align 4
  // CHECK-NEXT: store ptr %w, ptr %a, align 8
  // CHECK-NEXT: %1 = load ptr, ptr %a, align 8
  int w = 0;
  // CHECK-NEXT: store i32 8, ptr %1, align 4
  writethru(w) = 8;
}

#define choose_const(z) _Generic(z, constexpr int: 1, int: 2, constexpr double: 3, double: 4, default x: 0)
#define type_check_generic(z) _Generic(z, int x: choose_const(x), default x: choose_const(x))

static void type_check(void) {
  // The type of x should be const, even though we had to decay to
  // find a match.
  constexpr int value = 0;
  constexpr int result = type_check_generic(value);
  static_assert(result == 1);

  // The type of x should be const in default.
  constexpr double value1 = 0;
  constexpr int result1 = type_check_generic(value1);
  static_assert(result1 == 3);

  // The type of x should not be const.
  int value2 = 0;
  constexpr int result2 = type_check_generic(value2);
  static_assert(result2 == 2);

  // The type of x should not be const in default.
  double value3 = 0;
  constexpr int result3 = type_check_generic(value3);
  static_assert(result3 == 4);
}

static void always_consider_const() {
  // In C2y, constness is always considered!
  constexpr int value = 0;
  static_assert(1 == _Generic(value, constexpr int: 1, int: 2, default: 3));

  static_assert(2 == _Generic(value, int: 2, default: 3));

  static_assert(5 == _Generic(value,
    int : 0,
    const int : 1,
    default : 2)
  +
  _Generic(typeof(value),
    int : 3,
    const int : 4,
    default : 5));

}

#define assign_generic(x) _Generic(x, int x: x = 8, default: 0)

#ifndef NOERRORS
static void const_assignment_fails(void) {

  // The type of x should be const-qualified even though
  // there was a decay in order to match the association.
  constexpr int value = 0;
  constexpr int result = assign_generic(value); /* expected-error {{cannot assign to variable 'value' with const-qualified type 'const int'}}
                                                   expected-note {{declared const here}}
                                                */
  int value2 = 0;
  int result2 = assign_generic(value2);
}

static void no_type_operand_with_generic_association_names(void) {
  constexpr int value = 0;

  constexpr int result = _Generic(void, int a: a); // expected-error {{use of type as generic controlling operand is incompatible with assocations with generic association names}}
  constexpr int result2 = _Generic(void, default a: a); // expected-error {{use of type as generic controlling operand is incompatible with assocations with generic association names}}
}
#endif

#define decay_generic(x) _Generic(x, constexpr int a: a, int: 1, double: 2, default: 0)

static void decay(void) {
  // Should not decay and will select const int (value a).
  constexpr int value = 3;
  constexpr int result = decay_generic(value);
  static_assert(result == 3);

  // Should decay and select int (value 1).
  int value1 = 0;
  constexpr int result1 = decay_generic(value1);
  static_assert(result1 == 1);

  // Should decay back to double and select double (value 2).
  constexpr double value2 = 3;
  constexpr double result2 = decay_generic(value2);
  static_assert(result2 == 2);

  // Should decay and select the default.
  constexpr char value3 = 0;
  constexpr char result3 = decay_generic(value3);
  static_assert(result3 == 0);
}

// Use the same identifier in all cases to
// test that the scoping is correct.
#define scope_generic(X) _Generic(X, constexpr int b: b * b, constexpr double b: b, default: 27.2)

static void limited_scope() {
  // Test the int path.
  constexpr int ax = 3;
  constexpr int a = scope_generic(ax);
  static_assert(a == 9);

  // Test the double path.
  constexpr double cx = 3.1;
  constexpr double c = scope_generic(cx);
  static_assert(c == 3.1);

  // Test the default path.
  constexpr double dx = scope_generic((float)(3.1));
  static_assert(dx == 27.2);
}

static int maybe_called(void) {
  return 1;
}

// CHECK-LABEL: define {{.*}} void @operand_evaluated()
static void operand_evaluated(void) {
// CHECK: %a = alloca i32, align 4
// CHECK-NEXT: %call = call i32 @maybe_called()
  _Generic(maybe_called(), 
      int a: a, // expected-warning {{expression result unused}}
      double: 0xBAD,
      default: 0xBAD);
}

// CHECK-LABEL: define {{.*}} void @operand_evaluated_void()
static void operand_evaluated_void(void) {
// CHECK: %a = alloca i32, align 4
// CHECK-NEXT: %call = call i32 @maybe_called()
// CHECK-NEXT: store i32 %call, ptr %a, align 4

  // maybe_called() should still be called -- a void expression
  // when the generic association name is unused.
  _Generic(maybe_called(), 
      int a: 0, // expected-warning {{expression result unused}}
      double: 0xBAD,
      default: 0xBAD);

  // maybe_called() should not be called -- there are no
  // generic association names.
  _Generic(maybe_called(), 
      int: 0, // expected-warning {{expression result unused}}
      double: 0xBAD,
      default: 0xBAD);

// CHECK-NEXT: call void @anchor()
  anchor();
}

// CHECK-LABEL: define {{.*}} void @operand_evaluated_once()
static void operand_evaluated_once(void) {
// CHECK: %a = alloca i32, align 4
// CHECK-NEXT: %call = call i32 @maybe_called()
  _Generic(maybe_called(), 
      int a: (a * a) / 2, // expected-warning {{expression result unused}}
      double: 0xBAD,
      default: 0xBAD);
}

// CHECK-LABEL: define {{.*}} void @operand_evaluated_unused()
static void operand_evaluated_unused(void) {
// CHECK: %a = alloca i32, align 4
// CHECK-NEXT: %call = call i32 @maybe_called()
  _Generic(maybe_called(), 
      int a: 2, // expected-warning {{expression result unused}}
      double: 0xBAD,
      default: 0xBAD);
}

// CHECK-LABEL: define {{.*}} void @operand_not_evaluated_no_name()
static void operand_not_evaluated_no_name(void) {
// CHECK: ret void
  _Generic(maybe_called(), 
      int: 2, // expected-warning {{expression result unused}}
      double: 0xBAD,
      default: 0xBAD);
}

// CHECK-LABEL: define {{.*}} void @operand_not_evaluated_no_match()
static void operand_not_evaluated_no_match(void) {
// CHECK: ret void
  _Generic(maybe_called(), 
      double d: 0xBAD, 
      default: 0xBAD); // expected-warning {{expression result unused}}
}


typedef typeof(sizeof(0)) size_t;
size_t my_strlen (const char* s) { return 5; }

struct MyStringBuffer {
    const char *ptr;
    size_t len;
};

#define string_length(x) _Generic(x,           \
	double: (int)0.0,                            \
	const int s: s,                              \
	const char *foo            : my_strlen(foo), \
	struct MyStringBuffer *bar : bar->len)

size_t string_length_test_a(const char *p) { return string_length(p); }
size_t string_length_test_b(struct MyStringBuffer *p) { return (string_length(p) += 1); }
size_t string_length_test_c(const int v) { return string_length(v); }

// TODO: Check codegen here.
static void operands_pointers(void) {
	const int x = string_length_test_a("aaa");
	const int y = string_length_test_b(&(struct MyStringBuffer){.ptr = "bb", .len = 2});
	const int z = string_length_test_c(1);
}

int main() {
  lvalue();
  operand_evaluated();
  operand_evaluated_void();
  operand_evaluated_once();
  operand_evaluated_unused();
  operand_not_evaluated_no_name();
  operand_not_evaluated_no_match();
  return 1;
}