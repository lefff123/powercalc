#pragma once
#include "document_ast.h"
#include "document_evaluator.h"
#include <string>
#include <functional>

#ifndef POWERCALC_VERSION
#define POWERCALC_VERSION "1.0"
#endif

namespace powercalc::document {

struct HtmlOptions {
	std::string assetPrefix = "";
	bool exportMode = false;
	std::function<std::string(const std::string&)> imageResolver; // имя файла -> data URI (пока заглушка в UI)
};

std::string generateHtml(const DocumentAst& ast, const EvaluationResult& res,
                         const HtmlOptions& opts = HtmlOptions{});
}