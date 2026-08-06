#pragma once
#include "document_ast.h"
#include <complex>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace powercalc::document {

using Value = std::complex<double>;
using ValueProvider = std::function<std::optional<Value>(const std::string&)>;

struct ParsedFormula {
	ExprPtr tree;
	std::string lhs;   // нормализованное имя; пусто => не присваивание
	bool emptyRhs = false; // "u =" без правой части
	std::vector<Diagnostic> diagnostics;
};

ParsedFormula parseFormulaExpr(const std::string& src, int line);
std::optional<Value> evaluate(const Expr& tree, const ValueProvider& get, std::vector<Diagnostic>& diags);
bool isReservedName(const std::string& normalized);

} // namespace powercalc::document