#include <gtest/gtest.h>
#include "document_parser.h"

using namespace powercalc::document;

static DocumentAst parse(const std::string& s) { return DocumentParser{}.parse(s); }
static int count(const DocumentAst& a, const std::string& code) {
	int c = 0;
	for (const auto& d : a.diagnostics) if (d.code == code) ++c;
	return c;
}
static const Block* first(const DocumentAst& a, BlockKind k) {
	for (const auto& b : a.blocks) if (b.kind == k) return &b;
	return nullptr;
}

TEST(DocumentParser, EmptyDoc) {
	auto a = parse("");
	EXPECT_TRUE(a.blocks.empty());
	EXPECT_TRUE(a.diagnostics.empty());
}

TEST(DocumentParser, TextBlock) {
	auto a = parse("hello\nworld\n");
	ASSERT_EQ(a.blocks.size(), 1u);
	EXPECT_EQ(a.blocks[0].kind, BlockKind::Text);
	EXPECT_EQ(a.blocks[0].lineBegin, 1);
	EXPECT_EQ(a.blocks[0].lineEnd, 2);
}

TEST(DocumentParser, Headings) {
	auto a = parse("# A\n\n## B\n\n### C\n\n#### D\n");
	ASSERT_EQ(a.blocks.size(), 4u);
	EXPECT_EQ(a.blocks[0].kind, BlockKind::Heading);
	EXPECT_EQ(a.blocks[0].level, 1);
	EXPECT_EQ(a.blocks[0].text, "A");
	EXPECT_EQ(a.blocks[1].level, 2);
	EXPECT_EQ(a.blocks[2].level, 3);
	EXPECT_EQ(a.blocks[3].kind, BlockKind::Text); // #### — не заголовок
}

TEST(DocumentParser, FormulaConcat) {
	auto a = parse("$$\nU = 10 *\nI\n$$\n");
	const auto* f = first(a, BlockKind::Formula);
	ASSERT_NE(f, nullptr);
	EXPECT_EQ(f->formula.exprRaw, "U = 10 * I");
	EXPECT_EQ(count(a, "E002"), 0);
}

TEST(DocumentParser, Modifiers) {
	auto a = parse("$$hide\nx = 1\n$$\n\n$$!\ny = 1\n$$\n\n$$hide!\nz = 1\n$$\n");
	std::vector<const Block*> fs;
	for (const auto& b : a.blocks) if (b.kind == BlockKind::Formula) fs.push_back(&b);
	ASSERT_EQ(fs.size(), 3u);
	EXPECT_TRUE(fs[0]->formula.hide);
	EXPECT_FALSE(fs[0]->formula.invertSubstitution);
	EXPECT_FALSE(fs[1]->formula.hide);
	EXPECT_TRUE(fs[1]->formula.invertSubstitution);
	EXPECT_TRUE(fs[2]->formula.hide && fs[2]->formula.invertSubstitution);
	EXPECT_EQ(count(a, "E003"), 0);
}

TEST(DocumentParser, UnknownModifier) {
	auto a = parse("$$foo\nx = 1\n$$\n");
	EXPECT_EQ(count(a, "E003"), 1);
	EXPECT_EQ(a.diagnostics[0].line, 1);
}

TEST(DocumentParser, Units) {
	auto a = parse("$$\nP = 100 & кВт·ч\n$$\n\n$$\nC = 5 & руб/кВт·ч\n$$\n");
	std::vector<const Block*> fs;
	for (const auto& b : a.blocks) if (b.kind == BlockKind::Formula) fs.push_back(&b);
	ASSERT_EQ(fs.size(), 2u);
	EXPECT_EQ(fs[0]->formula.unit, "кВт·ч");
	EXPECT_EQ(fs[0]->formula.exprRaw, "P = 100");
	EXPECT_EQ(fs[1]->formula.unit, "руб/кВт·ч");
}

TEST(DocumentParser, UnitOnlyLastLine) {
	auto a = parse("$$\na = 1 & 2\nb = 3 & м\n$$\n");
	const auto* f = first(a, BlockKind::Formula);
	ASSERT_NE(f, nullptr);
	EXPECT_EQ(f->formula.unit, "м");
	EXPECT_EQ(f->formula.exprRaw, "a = 1 & 2 b = 3");
}

TEST(DocumentParser, Comments) {
	auto a = parse("$$\n# примечание\nx = 1\n# ещё\n$$\n");
	const auto* f = first(a, BlockKind::Formula);
	ASSERT_NE(f, nullptr);
	EXPECT_EQ(f->formula.comments.size(), 2u);
	EXPECT_EQ(f->formula.comments[0].first, 2);
	EXPECT_EQ(f->formula.comments[1].first, 4);
	EXPECT_EQ(f->formula.exprRaw, "x = 1");
}

TEST(DocumentParser, UnclosedFormula) {
	auto a = parse("$$\nx = 1\n");
	EXPECT_EQ(count(a, "E002"), 1);
	EXPECT_EQ(a.diagnostics[0].line, 1);
}

TEST(DocumentParser, YamlFull) {
	auto a = parse("---\ntitle: T\nauthor: A\ndate: D\nshow_substitution: false\n"
				  "page:\n  size: A5\n  margin:\n    top: 1cm\n    bottom: 20mm\n    left: 3cm\n    right: 4cm\n"
				  "align: center\ntext:\n  size: 12pt\n---\n\n# H\n");
	EXPECT_EQ(count(a, "E001"), 0);
	const auto& m = a.meta;
	EXPECT_TRUE(m.present);
	EXPECT_EQ(m.title, "T");
	EXPECT_EQ(m.author, "A");
	EXPECT_EQ(m.date, "D");
	EXPECT_FALSE(m.showSubstitution);
	EXPECT_EQ(m.pageSize, "A5");
	EXPECT_EQ(m.marginTop, "1cm");
	EXPECT_EQ(m.marginBottom, "20mm");
	EXPECT_EQ(m.marginLeft, "3cm");
	EXPECT_EQ(m.marginRight, "4cm");
	EXPECT_EQ(m.align, "center");
	EXPECT_EQ(m.textSize, "12pt");
}

TEST(DocumentParser, YamlDefaults) {
	auto a = parse("---\ntitle: T\n---\n");
	const auto& m = a.meta;
	EXPECT_TRUE(m.showSubstitution);
	EXPECT_EQ(m.pageSize, "A4");
	EXPECT_EQ(m.marginTop, "2cm");
	EXPECT_EQ(m.marginBottom, "2cm");
	EXPECT_EQ(m.align, "justify");
	EXPECT_EQ(m.textSize, "14pt");
}

TEST(DocumentParser, YamlUnknownKeys) {
	auto a = parse("---\ntitle: T\nfoo: 1\nbase_voltage: 110\n---\n");
	EXPECT_EQ(count(a, "E001"), 0);
	bool hasFoo = false, hasBv = false;
	for (const auto& kv : a.meta.unknownKeys) {
		if (kv.first == "foo") hasFoo = true;
		if (kv.first == "base_voltage") hasBv = true;
	}
	EXPECT_TRUE(hasFoo);
	EXPECT_TRUE(hasBv);
}

TEST(DocumentParser, YamlTab) {
	auto a = parse("---\ntitle: T\n\tauthor: A\n---\n");
	EXPECT_GE(count(a, "E001"), 1);
	EXPECT_EQ(a.diagnostics[0].line, 3);
}

TEST(DocumentParser, YamlBadSyntax) {
	auto a = parse("---\na: [1,\n---\n");
	EXPECT_EQ(count(a, "E001"), 1);
}

TEST(DocumentParser, YamlNotFirst) {
	auto a = parse("# H\n\n---\nx: 1\n---\n");
	EXPECT_EQ(first(a, BlockKind::Yaml), nullptr);
	EXPECT_FALSE(a.meta.present);
}

TEST(DocumentParser, Inlines) {
	auto a = parse("U = $$U_1$$ и $$Ток$$\n");
	const auto* t = first(a, BlockKind::Text);
	ASSERT_NE(t, nullptr);
	ASSERT_EQ(t->inlines.size(), 2u);
	EXPECT_TRUE(t->inlines[0].symbolic);
	EXPECT_EQ(t->inlines[0].raw, "U_1");
	EXPECT_EQ(t->inlines[0].line, 1);
	EXPECT_TRUE(t->inlines[1].symbolic);
	EXPECT_EQ(t->inlines[1].raw, "Ток");
}

TEST(DocumentParser, InlineNormalization) {
	auto a = parse("$$R_x$$ и $$R_{x}$$\n");
	const auto* t = first(a, BlockKind::Text);
	ASSERT_EQ(t->inlines.size(), 2u);
	EXPECT_EQ(t->inlines[0].name, t->inlines[1].name);
}

TEST(DocumentParser, InlineBad) {
	auto a = parse("x $$a+b$$ y\n");
	// В v1.2: $$a+b$$ — это symbolic inline (LaTeX), не ошибка
	EXPECT_EQ(count(a, "W001"), 0);
	// Проверим, что inline попал в блок
	EXPECT_EQ(a.blocks.size(), 1u);
	EXPECT_EQ(a.blocks[0].inlines.size(), 1u);
	EXPECT_TRUE(a.blocks[0].inlines[0].symbolic);
}

TEST(DocumentParser, InlineNested) {
	auto a = parse("Текст $$R_{a_{b}}$$ здесь\n");
	// В v1.2: вложенный индекс — это symbolic inline (LaTeX), не ошибка
	EXPECT_EQ(count(a, "W001"), 0);
	EXPECT_EQ(a.blocks.size(), 1u);
	EXPECT_EQ(a.blocks[0].inlines.size(), 1u);
	EXPECT_TRUE(a.blocks[0].inlines[0].symbolic);
}

TEST(DocumentParser, CollectsAllErrors) {
	auto a = parse("$$foo\nx = 1\n$$\n\n$$bar\ny = 2\n");
	EXPECT_EQ(count(a, "E003"), 2);
	EXPECT_EQ(count(a, "E002"), 1);
}

TEST(DocumentParser, SingleLineFormula) {
	auto a = parse("$$x = 1 # коммент$$\n");
	const auto* f = first(a, BlockKind::Formula);
	ASSERT_NE(f, nullptr);
	EXPECT_EQ(f->formula.exprRaw, "x = 1");
	EXPECT_EQ(f->formula.comments.size(), 1u);
}

TEST(DocumentParser, SingleLineUnit) {
	auto a = parse("$$P = 100 & кВт$$\n");
	const auto* f = first(a, BlockKind::Formula);
	ASSERT_NE(f, nullptr);
	EXPECT_EQ(f->formula.exprRaw, "P = 100");
	EXPECT_EQ(f->formula.unit, "кВт");
}

TEST(DocumentParser, WholeLineInlineStillText) {
	auto a = parse("$$U$$\n");
	EXPECT_NE(first(a, BlockKind::Text), nullptr);
	EXPECT_EQ(first(a, BlockKind::Formula), nullptr);
}

TEST(DocumentParser, OpeningLineContent) {
	auto a = parse("$$ empty = 1 #dddsf\n$$\n");
	const auto* f = first(a, BlockKind::Formula);
	ASSERT_NE(f, nullptr);
	EXPECT_EQ(f->formula.exprRaw, "empty = 1");
	EXPECT_EQ(f->formula.comments.size(), 1u);
	EXPECT_EQ(count(a, "E003"), 0);
}

TEST(DocumentParser, OpeningLineModifierAndContent) {
	auto a = parse("$$hide x = 2\n$$\n");
	const auto* f = first(a, BlockKind::Formula);
	ASSERT_NE(f, nullptr);
	EXPECT_TRUE(f->formula.hide);
	EXPECT_EQ(f->formula.exprRaw, "x = 2");
}

TEST(DocumentParser, ClosingAtEndOfContentLine) {
	auto a = parse("$$\nide = 1 #asdsa $$\n");
	const auto* f = first(a, BlockKind::Formula);
	ASSERT_NE(f, nullptr);
	EXPECT_EQ(f->formula.exprRaw, "ide = 1");
	EXPECT_EQ(f->formula.comments.size(), 1u);
	EXPECT_EQ(count(a, "E002"), 0);
	EXPECT_EQ(f->lineEnd, 2);
}

TEST(DocumentParser, BlockAfterInlineClose) {
	auto a = parse("$$\nx = 1 $$\n\ntext after\n");
	EXPECT_EQ(count(a, "E002"), 0);
	const auto* t = first(a, BlockKind::Text);
	ASSERT_NE(t, nullptr);
	EXPECT_EQ(t->text, "text after");
}

TEST(DocumentParser, ListBasic) {
	auto a = parse("- один\n- два\n  - два.1\n1. три\n");
	ASSERT_EQ(a.blocks.size(), 1u);
	EXPECT_EQ(a.blocks[0].kind, BlockKind::List);
	EXPECT_EQ(a.blocks[0].items.size(), 4u);
	EXPECT_EQ(a.blocks[0].items[2].level, 1);
	EXPECT_TRUE(a.blocks[0].items[3].ordered);
}
TEST(DocumentParser, TableBasic) {
	auto a = parse("| a | b |\n| --- | ---: |\n| 1 | 2 |\n");
	ASSERT_EQ(a.blocks.size(), 1u);
	EXPECT_EQ(a.blocks[0].kind, BlockKind::Table);
	EXPECT_EQ(a.blocks[0].rows.size(), 2u);
	EXPECT_TRUE(a.blocks[0].rows[0].header);
	EXPECT_EQ(a.blocks[0].rows[1].cells[1].align, 'r');
}
TEST(DocumentParser, ImageAndStyle) {
	auto a = parse("![Схема](schema.png)\n# Титл {center,12pt}\n");
	ASSERT_EQ(a.blocks.size(), 2u);
	EXPECT_EQ(a.blocks[0].kind, BlockKind::Image);
	EXPECT_EQ(a.blocks[0].imageName, "schema.png");
	EXPECT_EQ(a.blocks[1].kind, BlockKind::Heading);
	EXPECT_EQ(a.blocks[1].localAlign, "center");
	EXPECT_EQ(a.blocks[1].localSize, "12pt");
	EXPECT_EQ(a.blocks[1].text, "Титл");
}

TEST(DocumentParser, TocBlock) {
	auto a = parse("[toc]\n# Один\n");
	ASSERT_EQ(a.blocks.size(), 2u);
	EXPECT_EQ(a.blocks[0].kind, BlockKind::Toc);
}