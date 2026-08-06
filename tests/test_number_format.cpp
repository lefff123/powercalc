#include <gtest/gtest.h>
#include "number_format.h"
using namespace powercalc::document;

TEST(NumberFormat, Real) {
	EXPECT_EQ(formatReal(2.5), "2.5");
	EXPECT_EQ(formatReal(3.0), "3");
	EXPECT_EQ(formatReal(0.1235), "0.124");
	EXPECT_EQ(formatReal(-0.0), "0");
}
TEST(NumberFormat, Complex) {
	EXPECT_EQ(formatValue(Value(3, 4)), "3 + j4");
	EXPECT_EQ(formatValue(Value(0, -0.5)), "-j0.5");
	EXPECT_EQ(formatValue(Value(0, 1)), "j");
	EXPECT_EQ(formatValue(Value(0, -1)), "-j1");
	EXPECT_EQ(formatValue(Value(2, 1e-10)), "2");   // порог 1e-9
	EXPECT_EQ(formatValue(Value(1e-10, 0)), "0");
}