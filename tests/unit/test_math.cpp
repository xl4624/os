#include <math.h>

#include "../framework/test.h"

// ============================================================
// Helpers
// ============================================================

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kE = 2.71828182845904523536;

// Absolute-value epsilon comparison for doubles.
bool near(double a, double b, double eps = 1e-9) {
  double diff = a - b;
  return (diff >= -eps) && (diff <= eps);
}

}  // namespace

// ====================================
// fabs
// ====================================

TEST(math, fabs_positive) { ASSERT(near(fabs(3.5), 3.5)); }

TEST(math, fabs_negative) { ASSERT(near(fabs(-3.5), 3.5)); }

TEST(math, fabs_zero) { ASSERT(near(fabs(0.0), 0.0)); }

TEST(math, fabs_large) { ASSERT(near(fabs(-1e15), 1e15, 1.0)); }

// ====================================
// sqrt
// ====================================

TEST(math, sqrt_zero) { ASSERT(near(sqrt(0.0), 0.0)); }

TEST(math, sqrt_one) { ASSERT(near(sqrt(1.0), 1.0)); }

TEST(math, sqrt_four) { ASSERT(near(sqrt(4.0), 2.0)); }

TEST(math, sqrt_nine) { ASSERT(near(sqrt(9.0), 3.0)); }

TEST(math, sqrt_two) { ASSERT(near(sqrt(2.0), 1.41421356237, 1e-9)); }

TEST(math, sqrt_large) { ASSERT(near(sqrt(1e10), 1e5, 1e-4)); }

// ====================================
// sin
// ====================================

TEST(math, sin_zero) { ASSERT(near(sin(0.0), 0.0)); }

TEST(math, sin_half_pi) { ASSERT(near(sin(kPi / 2.0), 1.0)); }

TEST(math, sin_pi) { ASSERT(near(sin(kPi), 0.0, 1e-9)); }

TEST(math, sin_three_half_pi) { ASSERT(near(sin(3.0 * kPi / 2.0), -1.0)); }

TEST(math, sin_negative) { ASSERT(near(sin(-kPi / 2.0), -1.0)); }

TEST(math, sin_small) { ASSERT(near(sin(0.1), 0.09983341664682815)); }

// ====================================
// cos
// ====================================

TEST(math, cos_zero) { ASSERT(near(cos(0.0), 1.0)); }

TEST(math, cos_half_pi) { ASSERT(near(cos(kPi / 2.0), 0.0, 1e-9)); }

TEST(math, cos_pi) { ASSERT(near(cos(kPi), -1.0)); }

TEST(math, cos_two_pi) { ASSERT(near(cos(2.0 * kPi), 1.0, 1e-9)); }

TEST(math, cos_negative) { ASSERT(near(cos(-kPi), -1.0)); }

// Pythagorean identity: sin^2 + cos^2 == 1
TEST(math, sin_cos_identity) {
  double angle = 1.2345;
  double s = sin(angle);
  double c = cos(angle);
  ASSERT(near((s * s) + (c * c), 1.0, 1e-12));
}

// ====================================
// atan2
// ====================================

TEST(math, atan2_positive_x_axis) { ASSERT(near(atan2(0.0, 1.0), 0.0)); }

TEST(math, atan2_positive_y_axis) { ASSERT(near(atan2(1.0, 0.0), kPi / 2.0)); }

TEST(math, atan2_negative_x_axis) { ASSERT(near(atan2(0.0, -1.0), kPi)); }

TEST(math, atan2_negative_y_axis) { ASSERT(near(atan2(-1.0, 0.0), -kPi / 2.0)); }

TEST(math, atan2_first_quadrant) {
  // atan2(1, 1) = pi/4
  ASSERT(near(atan2(1.0, 1.0), kPi / 4.0));
}

TEST(math, atan2_second_quadrant) {
  // atan2(1, -1) = 3*pi/4
  ASSERT(near(atan2(1.0, -1.0), 3.0 * kPi / 4.0));
}

TEST(math, atan2_third_quadrant) {
  // atan2(-1, -1) = -3*pi/4
  ASSERT(near(atan2(-1.0, -1.0), -3.0 * kPi / 4.0));
}

TEST(math, atan2_fourth_quadrant) {
  // atan2(-1, 1) = -pi/4
  ASSERT(near(atan2(-1.0, 1.0), -kPi / 4.0));
}

// atan2(y, x) == atan2(-y, -x) + pi  (for y > 0)
TEST(math, atan2_symmetry) { ASSERT(near(atan2(2.0, 3.0), atan2(-2.0, -3.0) + kPi, 1e-9)); }

// ====================================
// floor
// ====================================

TEST(math, floor_positive) { ASSERT(near(floor(2.9), 2.0)); }

TEST(math, floor_positive_half) { ASSERT(near(floor(2.5), 2.0)); }

TEST(math, floor_negative) { ASSERT(near(floor(-2.1), -3.0)); }

TEST(math, floor_negative_half) { ASSERT(near(floor(-2.5), -3.0)); }

TEST(math, floor_exact_integer) { ASSERT(near(floor(3.0), 3.0)); }

TEST(math, floor_zero) { ASSERT(near(floor(0.0), 0.0)); }

TEST(math, floor_small_positive) { ASSERT(near(floor(0.99), 0.0)); }

TEST(math, floor_small_negative) { ASSERT(near(floor(-0.01), -1.0)); }

// ====================================
// ceil
// ====================================

TEST(math, ceil_positive) { ASSERT(near(ceil(2.1), 3.0)); }

TEST(math, ceil_positive_half) { ASSERT(near(ceil(2.5), 3.0)); }

TEST(math, ceil_negative) { ASSERT(near(ceil(-2.9), -2.0)); }

TEST(math, ceil_negative_half) { ASSERT(near(ceil(-2.5), -2.0)); }

TEST(math, ceil_exact_integer) { ASSERT(near(ceil(3.0), 3.0)); }

TEST(math, ceil_zero) { ASSERT(near(ceil(0.0), 0.0)); }

TEST(math, ceil_small_positive) { ASSERT(near(ceil(0.01), 1.0)); }

TEST(math, ceil_small_negative) { ASSERT(near(ceil(-0.99), 0.0)); }

// floor(x) <= x < floor(x) + 1 for all x
TEST(math, floor_ceil_relationship) {
  double x = 3.7;
  ASSERT(floor(x) <= x);
  ASSERT(ceil(x) >= x);
  ASSERT(near(ceil(x) - floor(x), 1.0));
}

// ====================================
// log
// ====================================

TEST(math, log_one) { ASSERT(near(log(1.0), 0.0)); }

TEST(math, log_e) { ASSERT(near(log(kE), 1.0)); }

TEST(math, log_e_squared) { ASSERT(near(log(kE * kE), 2.0)); }

TEST(math, log_half) { ASSERT(near(log(0.5), -0.6931471805599453)); }

TEST(math, log_ten) { ASSERT(near(log(10.0), 2.302585092994046)); }

// log is the inverse of exp: log(e^x) == x
TEST(math, log_inverse_exp) {
  // We don't have exp(), but pow(e, x) = e^x
  double x = 3.0;
  double ex = pow(kE, x);
  ASSERT(near(log(ex), x, 1e-6));
}

// ====================================
// pow
// ====================================

TEST(math, pow_zero_exponent) { ASSERT(near(pow(5.0, 0.0), 1.0)); }

TEST(math, pow_one_exponent) { ASSERT(near(pow(5.0, 1.0), 5.0)); }

TEST(math, pow_two_exponent) { ASSERT(near(pow(3.0, 2.0), 9.0)); }

TEST(math, pow_three_exponent) { ASSERT(near(pow(2.0, 3.0), 8.0)); }

TEST(math, pow_half_exponent) {
  // 4^0.5 = sqrt(4) = 2
  ASSERT(near(pow(4.0, 0.5), 2.0));
}

TEST(math, pow_negative_exponent) {
  // 2^(-1) = 0.5
  ASSERT(near(pow(2.0, -1.0), 0.5));
}

TEST(math, pow_fractional_base) {
  // 0.5^2 = 0.25
  ASSERT(near(pow(0.5, 2.0), 0.25));
}

TEST(math, pow_one_base) {
  // 1^anything = 1
  ASSERT(near(pow(1.0, 100.0), 1.0));
}

TEST(math, pow_large) {
  // 2^10 = 1024
  ASSERT(near(pow(2.0, 10.0), 1024.0, 1e-6));
}

TEST(math, pow_e_to_one) {
  // e^1 = e
  ASSERT(near(pow(kE, 1.0), kE, 1e-9));
}

TEST(math, pow_consistency_with_sqrt) {
  // x^0.5 == sqrt(x) for positive x
  double x = 7.0;
  ASSERT(near(pow(x, 0.5), sqrt(x), 1e-9));
}
