#include <gtest/gtest.h>
#include "document_parser.h"
#include "document_evaluator.h"
using namespace powercalc::document;

static EvaluationResult run(const std::string& src, DocumentAst& ast) {
	ast = DocumentParser().parse(src);
	SymbolTable st;
	return evaluateDocument(ast, st);
}

TEST(Substitution, Basic) {
	DocumentAst ast;
	auto res = run("$$R = 10$$\n$$X = 50$$\n$$Z = \\sqrt{R^2 + X^2}$$\n", ast);
	ASSERT_EQ(res.blocks.size(), 3u);
	const auto& z = res.blocks[2];
	ASSERT_TRUE(z.value.has_value());
	EXPECT_NE(z.substitutedLatex.find("10"), std::string::npos);
	EXPECT_NE(z.substitutedLatex.find("50"), std::string::npos);
	EXPECT_EQ(z.substitutedLatex, "\\sqrt{10^{2} + 50^{2}}");
}
TEST(Substitution, RedefineLaterKeepsOld) {
	DocumentAst ast;
	auto res = run("$$R = 10$$\n$$Z = R + 1$$\n$$R = 99$$\n", ast);
	EXPECT_NE(res.blocks[1].substitutedLatex.find("10"), std::string::npos);
	EXPECT_EQ(res.blocks[1].substitutedLatex.find("99"), std::string::npos);
}
TEST(Substitution, NegativeInParens) {
	DocumentAst ast;
	auto res = run("$$R = -5$$\n$$Z = R * 2$$\n", ast);
	EXPECT_EQ(res.blocks[1].substitutedLatex, "(-5) \\cdot 2");
}
TEST(Substitution, W001Empty) {
	DocumentAst ast;
	auto res = run("$$A = \\int x$$\n", ast); // неизвестная команда -> W001
	ASSERT_EQ(res.blocks.size(), 1u);
	EXPECT_FALSE(res.blocks[0].value.has_value());
	EXPECT_TRUE(res.blocks[0].substitutedLatex.empty());
}