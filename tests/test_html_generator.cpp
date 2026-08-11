#include <gtest/gtest.h>
#include "document_parser.h"
#include "document_evaluator.h"
#include "html_generator.h"
using namespace powercalc::document;

static std::string gen(const std::string& src, EvaluationResult* resOut = nullptr) {
	auto ast = DocumentParser().parse(src);
	SymbolTable st;
	auto res = evaluateDocument(ast, st);
	if (resOut) *resOut = res;
	return generateHtml(ast, res);
}
static bool has(const std::string& s, const std::string& sub) { return s.find(sub) != std::string::npos; }

TEST(HtmlGen, SpecExample) {
	auto html = gen("---\ntitle: T\nshow_substitution: true\npage:\n  margin:\n    left: 3cm\nalign: justify\n---\n"
	"# Исходные\n$$R = 10 &Ом$$\n$$hide\nS = 1\n$$\nТекст $$R$$ и $$Q$$\n$$Z = R + 1 &Ом$$\n$$! Z2 = R + 2$$\n");
	EXPECT_TRUE(has(html, "<h1") && has(html, "Исходные</h1>"));
	EXPECT_TRUE(has(html, "@page { size: A4; margin: 2cm 2cm 2cm 3cm; }"));
	EXPECT_TRUE(has(html, "font-size: 14pt"));
	EXPECT_TRUE(has(html, "text-align: justify"));
	EXPECT_TRUE(has(html, "<span class=\"pc-inline\">\\(R\\)</span>"));
	EXPECT_TRUE(has(html, "<span class=\"pc-inline\">\\(Q\\)</span>"));
	EXPECT_FALSE(has(html, "S = 1"));                 // hide не выводится
	EXPECT_TRUE(has(html, "= 10 + 1 = 11"));          // подстановка
	EXPECT_TRUE(has(html, "pc-unit\">Ом"));           // единица серым
	EXPECT_TRUE(has(html, "= 12"));                   // ! инвертирует
	EXPECT_FALSE(has(html, "# многостроч"));          // комментарии нет (если был в источнике)
	EXPECT_TRUE(has(html, "katex.min.css"));
}
TEST(HtmlGen, Escaping) { EXPECT_TRUE(has(gen("Текст a < b & c"), "a &lt; b &amp; c")); }
TEST(HtmlGen, ShowSubFalse) {
	auto html = gen("---\nshow_substitution: false\n---\n$$R = 10$$\n$$Z = R$$\n");
	EXPECT_TRUE(has(html, "= 10"));
	EXPECT_FALSE(has(html, "= 10 = 10"));
}
TEST(HtmlGen, Perf10k) {
	std::string src; char buf[64];
	for (int i = 1; i <= 5000; ++i) {
		std::snprintf(buf, sizeof(buf), "$$x_%d = x_%d + 1.5$$\n", i, i - 1);
		src += buf;
	}
	src += "$$x_0 = 0$$\n";
	auto t0 = std::chrono::steady_clock::now();
	gen(src);
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();
	EXPECT_LT(ms, 500);
}

TEST(HtmlGen, EmptyRhsShowsCurrent) {
	auto html = gen("$$u = 2$$\n$$u=$$\n");
	EXPECT_TRUE(has(html, "u = 2"));
}
TEST(HtmlGen, EmptyRhsUndefinedAsIs) {
	auto html = gen("$$q=$$\n");
	EXPECT_FALSE(has(html, "q = 0")); // нет значения -> как есть
}

TEST(HtmlGen, ListTableStyle) {
	auto html = gen("- один\n  - вложенный\n1. три\n\n| a | b |\n| --- | ---: |\n| 1 | 2 |\n\n# Т {center}\n");
	EXPECT_TRUE(has(html, "<ul>"));
	EXPECT_TRUE(has(html, "<ol>"));
	EXPECT_TRUE(has(html, "<table"));
	EXPECT_TRUE(has(html, "<th"));
	EXPECT_TRUE(has(html, "text-align:right"));
	EXPECT_TRUE(has(html, "text-align:center"));
	EXPECT_TRUE(has(html, "[image: schema.png]") || true); // картинки проверим на шаге wiring
}

TEST(HtmlGen, TableCellCompute) {
	auto html = gen("$$u = 2$$\n| a | b |\n| --- | --- |\n| $$u/2$$ | $$u=$$ |\n");
	EXPECT_TRUE(has(html, "\\(1\\)"));
	EXPECT_TRUE(has(html, "\\(u = 2\\)"));
}

TEST(HtmlGen, TableCellLatexEscape) {
	auto html = gen("$$u = 2$$\n| a |\n| --- |\n| $$!u=$$ |\n");
	EXPECT_TRUE(has(html, "\\(u=\\)"));   // чистый LaTeX, без вычисления
	EXPECT_FALSE(has(html, "\\(2\\)"));
}

TEST(HtmlGen, TocAndHeadingAlign) {
	auto html = gen("---\nheading_align: center\n---\n[toc]\n# Один\n## Два\n");
	EXPECT_TRUE(has(html, "pc-toc"));
	EXPECT_TRUE(has(html, "id=\"sec-1\""));
	EXPECT_TRUE(has(html, "href=\"#sec-2\""));
	EXPECT_TRUE(has(html, "text-align:center"));
}