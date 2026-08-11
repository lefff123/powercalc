#include "document_evaluator.h"
#include "expr_evaluator.h"
#include "number_format.h"

namespace {
std::string trim(const std::string& s) {
	size_t b = 0, e = s.size();
	while (b < e && (s[b] == ' ' || s[b] == '\t' || s[b] == '\r')) ++b;
	while (e > b && (s[e - 1] == ' ' || s[e - 1] == '\t' || s[e - 1] == '\r')) --e;
	return s.substr(b, e - b);
}
} // namespace

namespace powercalc::document {

int prec(const Expr& e) {
	if (e.kind == ExprKind::Binary) {
		if (e.op == '+' || e.op == '-') return 1;
		if (e.op == '*' || e.op == '/') return 2;
		if (e.op == '^') return 3;
	}
	return 4;
}

bool subLatex(const Expr& e, const ValueProvider& get, std::string& out);

bool subChild(const Expr& c, const Expr& p, bool right, const ValueProvider& get, std::string& out) {
	std::string s;
	if (!subLatex(c, get, s)) return false;
	if (p.kind == ExprKind::Binary) {
		const int pp = prec(p), pc = prec(c);
		bool paren = false;
		if (p.op == '+' || p.op == '-') paren = pc < pp || (right && pc == pp);
		else if (p.op == '*')           paren = pc < pp;
		else if (p.op == '^' && !right) paren = pc < pp; // экспонента всегда в ^{...}
		if (paren) { out += "(" + s + ")"; return true; }
	}
	out += s;
	return true;
}

bool subLatex(const Expr& e, const ValueProvider& get, std::string& out) {
	switch (e.kind) {
	case ExprKind::Number: out += formatReal(e.value); return true;
	case ExprKind::Constant:
		out += (e.name == "pi") ? std::string("\\pi") : e.name; // e, j как есть
		return true;
	case ExprKind::Variable: {
		auto v = get(e.name);
		if (!v) return false;
		std::string s = formatValue(*v);
		if (std::fabs(v->imag()) >= 1e-9 || v->real() < 0) s = "(" + s + ")";
		out += s;
		return true;
	}
	case ExprKind::Unary: {
		out += "-";
		if (e.args[0]->kind == ExprKind::Binary) {
			std::string s; if (!subLatex(*e.args[0], get, s)) return false;
			out += "(" + s + ")"; return true;
		}
		return subLatex(*e.args[0], get, out);
	}
	case ExprKind::Binary: {
		if (e.op == '/') { // \frac{a}{b} — скобки не нужны
			std::string a, b;
			if (!subLatex(*e.args[0], get, a) || !subLatex(*e.args[1], get, b)) return false;
			out += "\\frac{" + a + "}{" + b + "}"; return true;
		}
		if (e.op == '^') {
			std::string a, b;
			if (!subChild(*e.args[0], e, false, get, a) || !subLatex(*e.args[1], get, b)) return false;
			out += a + "^{" + b + "}"; return true;
		}
		std::string a, b;
		if (!subChild(*e.args[0], e, false, get, a)) return false;
		out += a;
		out += (e.op == '*') ? " \\cdot " : std::string(" ") + e.op + " ";
		return subChild(*e.args[1], e, true, get, out);
	}
	case ExprKind::Frac: {
		std::string a, b;
		if (!subLatex(*e.args[0], get, a) || !subLatex(*e.args[1], get, b)) return false;
		out += "\\frac{" + a + "}{" + b + "}"; return true;
	}
	case ExprKind::Call: {
		std::string a;
		if (!subLatex(*e.args[0], get, a)) return false;
		out += "\\" + e.name + "{" + a + "}"; return true;
	}
	default: return false;
	}
}

EvaluationResult evaluateDocument(const DocumentAst& ast, SymbolTable& st) {
	EvaluationResult res;
	auto provider = st.asProvider();

	auto processInlines = [&](const std::vector<InlineRef>& inls) {
		for (const auto& ref : inls) {
			if (!ref.compute) continue;
			if (!ref.raw.empty() && ref.raw[0] == '!') continue;
			const std::string& raw = ref.raw;
			InlineValue iv;
			if (raw.find('=') == std::string::npos) {
				auto pe = parseExpression(raw, ref.line);
				for (const auto& d : pe.diagnostics) res.diagnostics.push_back(d);
				if (pe.tree) {
					auto v = evaluate(*pe.tree, provider, res.diagnostics);
					if (v) { iv.value = v; res.inlineValues[&ref] = iv; }
				}
			} else {
				auto pf = parseFormulaExpr(raw, ref.line);
				for (const auto& d : pf.diagnostics) res.diagnostics.push_back(d);
				if (!pf.tree) continue;
				if (!pf.lhs.empty()) iv.name = pf.lhs;
				auto v = evaluate(*pf.tree, provider, res.diagnostics);
				if (v) {
					if (!pf.lhs.empty() && !isReservedName(pf.lhs)) {
						st.define(pf.lhs, *v);
						res.definitions.emplace_back(pf.lhs, *v);
					}
					iv.value = v;
				} else if (pf.emptyRhs) {
					iv.value = st.lookup(pf.lhs);
				}
				if (iv.value || !iv.name.empty()) res.inlineValues[&ref] = iv;
			}
		}
	};

	for (const auto& b : ast.blocks) {
		if (b.kind == BlockKind::Formula) {
			auto pf = parseFormulaExpr(b.formula.exprRaw, b.formula.exprLine);
			for (const auto& d : pf.diagnostics) res.diagnostics.push_back(d);
			if (!pf.tree) continue;

			auto val = evaluate(*pf.tree, provider, res.diagnostics);

			BlockEvalResult br;
			br.block = &b;
			br.lhs = pf.lhs;
			br.emptyRhs = pf.emptyRhs;
			if (!pf.lhs.empty() && val.has_value()) {
				br.value = *val;
				if (pf.tree->kind != ExprKind::Number &&
					pf.tree->kind != ExprKind::Variable &&
					pf.tree->kind != ExprKind::Constant) {
					std::string s;
					if (subLatex(*pf.tree, provider, s)) br.substitutedLatex = s;
				}
			}
			res.blocks.push_back(std::move(br));

			if (!pf.lhs.empty()) {
				if (isReservedName(pf.lhs)) continue;
				if (val.has_value()) {
					st.define(pf.lhs, *val);
					res.definitions.emplace_back(pf.lhs, *val);
				}
			}
		}

		if (b.kind == BlockKind::Text) processInlines(b.inlines);
		else if (b.kind == BlockKind::List) {
			for (const auto& it : b.items) processInlines(it.inlines);
		}
		else if (b.kind == BlockKind::Table) {
			for (const auto& r : b.rows)
				for (const auto& c : r.cells)
					processInlines(c.inlines);
		}
	}
	return res;
}
} // namespace powercalc::document