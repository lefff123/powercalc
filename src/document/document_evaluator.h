#pragma once
#include "document_ast.h"
#include "symbol_table.h"
#include <optional>
#include <vector>
#include <map>

namespace powercalc::document {

struct InlineValue {
	std::string name;              // "" => просто значение; иначе "name = value"
	std::optional<Value> value;
};


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
	std::map<const InlineRef*, InlineValue> inlineValues;
};

EvaluationResult evaluateDocument(const DocumentAst& ast, SymbolTable& st);

} // namespace powercalc::document