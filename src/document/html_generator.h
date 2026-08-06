#pragma once
#include "document_ast.h"
#include "document_evaluator.h"
#include <string>

#ifndef POWERCALC_VERSION
#define POWERCALC_VERSION "1.0"
#endif

namespace powercalc::document {

struct HtmlOptions {
	std::string assetPrefix = "";  // превью: "" (baseUrl qrc:/katex/); экспорт: "katex/"
	bool exportMode = false;       // <!-- PowerCalc <ver> -->
};

std::string generateHtml(const DocumentAst& ast, const EvaluationResult& res,
                         const HtmlOptions& opts = HtmlOptions{});
}