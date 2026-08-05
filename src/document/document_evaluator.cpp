#include "document_evaluator.h"
#include "expr_evaluator.h"

namespace powercalc::document {

EvaluationResult evaluateDocument(const DocumentAst& ast, SymbolTable& st) {
	EvaluationResult res;
	auto provider = st.asProvider();

	for (const auto& b : ast.blocks) {
		if (b.kind == BlockKind::Formula) {
			auto pf = parseFormulaExpr(b.formula.exprRaw, b.formula.exprLine);
			for (const auto& d : pf.diagnostics) res.diagnostics.push_back(d);
			if (!pf.tree) continue;

			auto val = evaluate(*pf.tree, provider, res.diagnostics);

			if (!pf.lhs.empty()) {
				if (isReservedName(pf.lhs)) continue;   // E007 уже в диагностикаx
				if (val.has_value()) {
					st.define(pf.lhs, *val);
					res.definitions.emplace_back(pf.lhs, *val);
				}
				// при ошибке значение НЕ кладём — старое остаётся живым
			}
			continue;
		}

		if (b.kind == BlockKind::Text) {
			for (const auto& ref : b.inlines) {
				if (!st.lookup(ref.name)) {
					res.diagnostics.push_back({Diagnostic::Level::Error, "E005",
						ref.line, "undefined variable " + ref.name});
				}
			}
		}
	}
	return res;
}

} // namespace powercalc::document