#pragma once
#include "document_ast.h"
#include "symbol_table.h"
#include <optional>
#include <vector>

namespace powercalc::document {

struct BlockEvalResult {
	const Block* block = nullptr;
	std::optional<Value> value;
	std::string substitutedLatex;
	std::string lhs;          // для блоков с пустой RHS
	bool emptyRhs = false;
};

struct EvaluationResult {
	std::vector<Diagnostic> diagnostics;
	std::vector<std::pair<std::string, Value>> definitions; // в порядке вычисления
	std::vector<BlockEvalResult> blocks;
};

EvaluationResult evaluateDocument(const DocumentAst& ast, SymbolTable& st);

} // namespace powercalc::document