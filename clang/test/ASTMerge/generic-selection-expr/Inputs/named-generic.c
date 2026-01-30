void f(void) {
  int x = 1;
  double y = 1;
  _Static_assert(1 == _Generic(x, int __generic1_z: __generic1_z),
                 "Named generic assert succeeds");
  _Static_assert(1 == _Generic(x,
                         const int __generic2_const_z: __generic2_const_z,
                         int __generic2_z: __generic2_z + 1),
                 "Named generic assert succeeds");
  _Static_assert(1 == _Generic(y,
                         const int: 5,
                         default __generic3_default_z: __generic3_default_z),
                 "Named generic assert succeeds");
}
