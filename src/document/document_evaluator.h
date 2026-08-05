#pragma once
#include "document_ast.h"
#include "symbol_table.h"
#include <vector>

namespace powercalc::document {

struct EvaluationResult {
	std::vector<Diagnostic> diagnostics;
	std::vector<std::pair<std::string, Value>> definitions; // в порядке вычисления
};

EvaluationResult evaluateDocument(const DocumentAst& ast, SymbolTable& st);

} // namespace powercalc::document