int main() {
  int a = 1;
  int b = a + 1;
  int c = b + a + 1;
  float aa = 1.0;
  float bb = aa + 1;
  float cc = bb + aa + 1.;
  // int d[10] = {};

  if (a > 0) {
    a = a - 1;
    aa = aa + a;
  }

  while (b > 0) {
    b = b - 1;
    bb = bb + b;
  }

  while (c > 0) {
    c = c - 1;
    cc = cc + c;
  }

  return 0;
  /* performance performance */
}