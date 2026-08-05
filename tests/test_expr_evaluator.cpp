#include <gtest/gtest.h>
#include "expr_evaluator.h"
#include <map>

using namespace powercalc::document;

static std::optional<Value> calc(const std::string& src, const std::map<std::string, Value>& vars,
                                 std::vector<Diagnostic>& diags) {
    auto pf = parseFormulaExpr(src, 1);
    diags = pf.diagnostics;
    if (!pf.tree) return std::nullopt;
    ValueProvider get = [&](const std::string& n) -> std::optional<Value> {
        auto it = vars.find(n);
        return it == vars.end() ? std::nullopt : std::optional<Value>(it->second);
    };
    return evaluate(*pf.tree, get, diags);
}

static int count(const std::vector<Diagnostic>& d, const std::string& code) {
    int c = 0;
    for (const auto& x : d) if (x.code == code) ++c;
    return c;
}

TEST(ExprEvaluator, Arithmetic) {
    std::vector<Diagnostic> d;
    auto v = calc("x = 2 + 3 * 4", {}, d);
    ASSERT_TRUE(v.has_value());
    EXPECT_DOUBLE_EQ(v->real(), 14);
}

TEST(ExprEvaluator, Parens) {
    std::vector<Diagnostic> d;
    EXPECT_DOUBLE_EQ(calc("x = (2 + 3) * 4", {}, d)->real(), 20);
}

TEST(ExprEvaluator, PowerRightAssoc) {
    std::vector<Diagnostic> d;
    EXPECT_DOUBLE_EQ(calc("x = 2 ^ 3 ^ 2", {}, d)->real(), 512);
    EXPECT_DOUBLE_EQ(calc("x = -2 ^ 2", {}, d)->real(), -4);
    EXPECT_DOUBLE_EQ(calc("x = 4 ^ 0.5", {}, d)->real(), 2);
}

TEST(ExprEvaluator, FracSqrt) {
    std::vector<Diagnostic> d;
    EXPECT_DOUBLE_EQ(calc("x = \\frac{1}{2}", {}, d)->real(), 0.5);
    EXPECT_DOUBLE_EQ(calc("x = \\sqrt{16}", {}, d)->real(), 4);
}

TEST(ExprEvaluator, Functions) {
    std::vector<Diagnostic> d;
    EXPECT_DOUBLE_EQ(calc("x = \\sin(0)", {}, d)->real(), 0);
    EXPECT_DOUBLE_EQ(calc("x = \\ln e", {}, d)->real(), 1);
    EXPECT_NEAR(calc("x = \\log(100)", {}, d)->real(), 2, 1e-12);
    EXPECT_DOUBLE_EQ(calc("x = \\abs{-5}", {}, d)->real(), 5);
}

TEST(ExprEvaluator, Complex) {
    std::vector<Diagnostic> d;
    auto v = calc("S = 3 + j * 4", {}, d);
    ASSERT_TRUE(v.has_value());
    EXPECT_DOUBLE_EQ(v->real(), 3);
    EXPECT_DOUBLE_EQ(v->imag(), 4);
}

TEST(ExprEvaluator, ImplicitMultAndAbs) {
    std::vector<Diagnostic> d;
    auto v = calc("m = \\abs{3 + j 4}", {}, d);   // неявное j*4
    ASSERT_TRUE(v.has_value());
    EXPECT_DOUBLE_EQ(v->real(), 5);
}

TEST(ExprEvaluator, ProviderLookup) {
    std::vector<Diagnostic> d;
    auto v = calc("S = P + j Q", {{"P", Value(3, 0)}, {"Q", Value(4, 0)}}, d);
    ASSERT_TRUE(v.has_value());
    EXPECT_DOUBLE_EQ(v->real(), 3);
    EXPECT_DOUBLE_EQ(v->imag(), 4);
}

TEST(ExprEvaluator, SubscriptedVar) {
    std::vector<Diagnostic> d;
    auto pf = parseFormulaExpr("U_1 = 10", 1);
    EXPECT_EQ(pf.lhs, "U_{1}");
    auto v = calc("y = U_{1} * 2", {{"U_{1}", Value(10, 0)}}, d);
    EXPECT_DOUBLE_EQ(v->real(), 20);
}

TEST(ExprEvaluator, E005) {
    std::vector<Diagnostic> d;
    auto v = calc("x = y + 1", {}, d);
    EXPECT_FALSE(v.has_value());
    EXPECT_EQ(count(d, "E005"), 1);
}

TEST(ExprEvaluator, E007) {
    std::vector<Diagnostic> d;
    auto pf = parseFormulaExpr("pi = 3", 1);
    EXPECT_EQ(count(pf.diagnostics, "E007"), 1);
    auto pf2 = parseFormulaExpr("j_1 = 3", 1);
    EXPECT_EQ(count(pf2.diagnostics, "E007"), 1);
}

TEST(ExprEvaluator, E010) {
    std::vector<Diagnostic> d;
    calc("1 + 1", {}, d);
    EXPECT_EQ(count(d, "E010"), 1);
    d.clear();
    calc("2 = x", {}, d);
    EXPECT_EQ(count(d, "E010"), 1);
}

TEST(ExprEvaluator, W001DivByZero) {
    std::vector<Diagnostic> d;
    auto v = calc("x = 1 / 0", {}, d);
    EXPECT_FALSE(v.has_value());
    EXPECT_EQ(count(d, "W001"), 1);
}

TEST(ExprEvaluator, W001UnknownCmd) {
    std::vector<Diagnostic> d;
    auto v = calc("x = \\int 1", {}, d);
    EXPECT_FALSE(v.has_value());
    EXPECT_EQ(count(d, "W001"), 1);
}

TEST(ExprEvaluator, W001NestedIndex) {
    std::vector<Diagnostic> d;
    auto v = calc("x = a_{b_{c}}", {}, d);
    EXPECT_FALSE(v.has_value());
    EXPECT_EQ(count(d, "W001"), 1);
}