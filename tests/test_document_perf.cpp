#include <gtest/gtest.h>
#include <chrono>
#include <sstream>
#include <algorithm>
#include <cmath>

#include "document_parser.h"
#include "document_evaluator.h"

using namespace powercalc::document;

static std::string makeDoc(int blocks) {
    std::ostringstream os;
    os << "---\ntitle: perf\n---\n\n";
    for (int i = 0; i < blocks; ++i) {
        os << "# Section " << i << "\n\n";
        os << "$$\nv_{" << i << "} = " << i << " * 2 + \\sqrt{" << i << "}\n$$\n\n";
        os << "Значение $$v_{" << i << "}$$ в тексте.\n\n";
    }
    return os.str();
}

static long runMs(const std::string& doc, int blocks) {
    auto t0 = std::chrono::steady_clock::now();
    auto ast = DocumentParser{}.parse(doc);
    SymbolTable st;
    auto res = evaluateDocument(ast, st);
    auto t1 = std::chrono::steady_clock::now();

    int formulas = 0;
    for (const auto& b : ast.blocks)
        if (b.kind == BlockKind::Formula) ++formulas;
    EXPECT_EQ(formulas, blocks);
    EXPECT_TRUE(ast.diagnostics.empty());
    EXPECT_TRUE(res.diagnostics.empty());

    const int last = blocks - 1;
    auto v = st.lookup("v_{" + std::to_string(last) + "}");
    EXPECT_TRUE(v.has_value());
    if (v) EXPECT_NEAR(v->real(), last * 2 + std::sqrt(static_cast<double>(last)), 1e-9);

    return std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
}

TEST(DocumentPerf, ThousandLines) {
    std::string doc = makeDoc(125);
    EXPECT_GE(std::count(doc.begin(), doc.end(), '\n'), 1000);
    EXPECT_LT(runMs(doc, 125), 100);
}

TEST(DocumentPerf, TenThousandLines) {
    std::string doc = makeDoc(1250);
    EXPECT_GE(std::count(doc.begin(), doc.end(), '\n'), 10000);
    EXPECT_LT(runMs(doc, 1250), 1000);
}