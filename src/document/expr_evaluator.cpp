#include "expr_evaluator.h"
#include "utf8.h"

#include <cmath>
#include <set>

namespace powercalc::document {

namespace {

struct Token {
	enum class Kind { End, Num, Var, Const, Cmd, Op, LParen, RParen, LBrace, RBrace, Error };
	Kind kind = Kind::End;
	std::string text;
	double num = 0;
};

bool isFuncCmd(const std::string& n) {
	return n == "sqrt" || n == "sin" || n == "cos" || n == "tan" ||
		   n == "log" || n == "ln" || n == "exp" || n == "abs";
}

bool isGreek(const std::string& n) {
	static const std::set<std::string> g = {
		"alpha", "beta", "gamma", "delta", "epsilon", "zeta", "eta", "theta", "iota", "kappa",
		"lambda", "mu", "nu", "xi", "rho", "sigma", "tau", "upsilon", "phi", "chi", "psi", "omega",
		"varepsilon", "vartheta", "varphi", "varrho", "varsigma", "varpi",
		"Gamma", "Delta", "Theta", "Lambda", "Xi", "Sigma", "Upsilon", "Phi", "Psi", "Omega"
	};
	return g.count(n) != 0;
}

std::vector<Token> tokenize(const std::string& s, std::vector<Diagnostic>& diags, int line) {
	std::vector<Token> t;
	size_t i = 0;
	auto err = [&](const std::string& msg) {
		diags.push_back({Diagnostic::Level::Warning, "W001", line, msg});
		t.push_back({Token::Kind::Error, "", 0});
	};
	while (i < s.size()) {
		unsigned char c = s[i];
		if (c == ' ' || c == '\t') { ++i; continue; }
		if ((c >= '0' && c <= '9') || (c == '.' && i + 1 < s.size() && s[i + 1] >= '0' && s[i + 1] <= '9')) {
			double v = 0;   // вручную, без stod: не зависим от locale (Windows!)
			while (i < s.size() && s[i] >= '0' && s[i] <= '9') v = v * 10 + (s[i++] - '0');
			if (i < s.size() && s[i] == '.') {
				++i;
				double place = 0.1;
				while (i < s.size() && s[i] >= '0' && s[i] <= '9') { v += (s[i++] - '0') * place; place *= 0.1; }
			}
			t.push_back({Token::Kind::Num, "", v});
			continue;
		}
		if (c == '\\') {
			++i;
			size_t st = i;
			while (i < s.size() && ((s[i] >= 'a' && s[i] <= 'z') || (s[i] >= 'A' && s[i] <= 'Z'))) ++i;
			std::string name = s.substr(st, i - st);
			if (name.empty()) { err("empty LaTeX command"); continue; }
			if (name == "cdot" || name == "times") { t.push_back({Token::Kind::Op, "*", 0}); continue; }
			if (name == "pi") { t.push_back({Token::Kind::Const, "pi", 0}); continue; }
			if (isFuncCmd(name) || name == "frac") { t.push_back({Token::Kind::Cmd, name, 0}); continue; }
			if (isGreek(name)) {
				std::string norm = name;
				if (!utf8::readIndexSuffix(s, i, norm)) { err("nested index in \\" + name); continue; }
				t.push_back({Token::Kind::Var, norm, 0});
				continue;
			}
			// Неизвестная команда (\int, \pm, что угодно) — в Cmd, W001 выпадет в parsePrimary.
			t.push_back({Token::Kind::Cmd, name, 0});
			continue;
		}
		{
			size_t j = i;
			uint32_t cp = utf8::decode(s, j);
			if (utf8::isLetter(cp)) {
				auto ns = utf8::readVariable(s, i);
				if (!ns.ok) {
					err("bad variable reference");
					// пропускаем битый индекс, чтобы не сыпать каскадом ошибок
					if (i < s.size() && s[i] == '_') {
						++i;
						if (i < s.size() && s[i] == '{') {
							int depth = 1;
							++i;
							while (i < s.size() && depth > 0) {
								if (s[i] == '{') ++depth;
								else if (s[i] == '}') --depth;
								++i;
							}
						}
					}
					continue;
				}
				if (ns.normalized == "e" || ns.normalized == "j") t.push_back({Token::Kind::Const, ns.normalized, 0});
				else t.push_back({Token::Kind::Var, ns.normalized, 0});
				continue;
			}
		}
		switch (c) {
			case '+': case '-': case '*': case '/': case '^': case '=':
				t.push_back({Token::Kind::Op, std::string(1, static_cast<char>(c)), 0}); ++i; continue;
			case '(': t.push_back({Token::Kind::LParen, "", 0}); ++i; continue;
			case ')': t.push_back({Token::Kind::RParen, "", 0}); ++i; continue;
			case '{': t.push_back({Token::Kind::LBrace, "", 0}); ++i; continue;
			case '}': t.push_back({Token::Kind::RBrace, "", 0}); ++i; continue;
			default: err(std::string("unexpected character '") + static_cast<char>(c) + "'"); ++i; continue;
		}
	}
	t.push_back({Token::Kind::End, "", 0});
	return t;
}

struct Parser {
	const std::vector<Token>& tk;
	size_t p = 0;
	int line;
	std::vector<Diagnostic>& diags;
	bool emptyRhs = false;

	const Token& cur() const { return tk[p]; }
	bool opIs(const std::string& o) const { return tk[p].kind == Token::Kind::Op && tk[p].text == o; }
	ExprPtr mk(ExprKind k) { auto e = std::make_unique<Expr>(); e->kind = k; e->line = line; return e; }
	ExprPtr mkError(const std::string& msg) {
		diags.push_back({Diagnostic::Level::Warning, "W001", line, msg});
		return mk(ExprKind::Error);
	}

	ExprPtr parseAssign(std::string& lhs) {
		ExprPtr left = parseExpr();
		if (opIs("=")) {
			++p;
			if (left && left->kind == ExprKind::Variable) lhs = left->name;
			else diags.push_back({Diagnostic::Level::Error, "E010", line, "left side must be a variable"});
			ExprPtr right;
			if (cur().kind == Token::Kind::End) {
				diags.push_back({Diagnostic::Level::Warning, "W001", line, "empty right-hand side"});
				emptyRhs = true;
				right = mk(ExprKind::Error);
			} else {
				right = parseExpr();
				if (cur().kind != Token::Kind::End) mkError("unexpected tokens after expression");
			}
			if (!lhs.empty() && isReservedName(lhs))
				diags.push_back({Diagnostic::Level::Error, "E007", line, "reserved name: " + lhs});
			return right;
		}
		if (cur().kind != Token::Kind::End) mkError("unexpected tokens after expression");
		diags.push_back({Diagnostic::Level::Error, "E010", line, "formula must be an assignment"});
		return left;
	}

	ExprPtr parseExpr() {
		ExprPtr e = parseTerm();
		while (opIs("+") || opIs("-")) {
			char o = tk[p].text[0]; ++p;
			auto b = mk(ExprKind::Binary); b->op = o;
			b->args.push_back(std::move(e));
			b->args.push_back(parseTerm());
			e = std::move(b);
		}
		return e;
	}

	ExprPtr parseTerm() {
		ExprPtr e = parseUnary();
		while (true) {
			if (opIs("*") || opIs("/")) {
				char o = tk[p].text[0]; ++p;
				auto b = mk(ExprKind::Binary); b->op = o;
				b->args.push_back(std::move(e));
				b->args.push_back(parseUnary());
				e = std::move(b);
				continue;
			}
			Token::Kind k = cur().kind;   // неявное умножение
			if (k == Token::Kind::Var || k == Token::Kind::Const || k == Token::Kind::Num ||
				k == Token::Kind::LParen || k == Token::Kind::LBrace || k == Token::Kind::Cmd) {
				auto b = mk(ExprKind::Binary); b->op = '*';
				b->args.push_back(std::move(e));
				b->args.push_back(parseUnary());
				e = std::move(b);
				continue;
			}
			return e;
		}
	}

	ExprPtr parseUnary() {
		if (opIs("-")) { ++p; auto u = mk(ExprKind::Unary); u->op = '-'; u->args.push_back(parseUnary()); return u; }
		if (opIs("+")) { ++p; return parseUnary(); }
		return parsePower();
	}

	ExprPtr parsePower() {
		ExprPtr base = parsePrimary();
		if (opIs("^")) {
			++p;
			ExprPtr ex;
			if (cur().kind == Token::Kind::LBrace) {
				++p; ex = parseExpr();
				if (cur().kind != Token::Kind::RBrace) return mkError("expected }");
				++p;
			} else ex = parseUnary();
			auto b = mk(ExprKind::Binary); b->op = '^';
			b->args.push_back(std::move(base));
			b->args.push_back(std::move(ex));
			return b;
		}
		return base;
	}

	ExprPtr parseArg() {
		if (cur().kind == Token::Kind::LBrace) {
			++p;
			ExprPtr e = parseExpr();
			if (cur().kind != Token::Kind::RBrace) mkError("expected }"); else ++p;
			return e;
		}
		return parseUnary();
	}

	ExprPtr parsePrimary() {
		const Token& t = cur();
		switch (t.kind) {
			case Token::Kind::Num: { auto e = mk(ExprKind::Number); e->value = t.num; ++p; return e; }
			case Token::Kind::Var: { auto e = mk(ExprKind::Variable); e->name = t.text; ++p; return e; }
			case Token::Kind::Const: { auto e = mk(ExprKind::Constant); e->name = t.text; ++p; return e; }
			case Token::Kind::LParen: {
				++p; ExprPtr e = parseExpr();
				if (cur().kind != Token::Kind::RParen) return mkError("expected )");
				++p; return e;
			}
			case Token::Kind::LBrace: {
				++p; ExprPtr e = parseExpr();
				if (cur().kind != Token::Kind::RBrace) return mkError("expected }");
				++p; return e;
			}
			case Token::Kind::Cmd: {
				std::string name = t.text; ++p;
				if (name == "frac") {
					ExprPtr num, den;
					if (cur().kind != Token::Kind::LBrace) return mkError("\\frac expects {num}{den}");
					++p; num = parseExpr();
					if (cur().kind != Token::Kind::RBrace) return mkError("\\frac expects {num}{den}");
					++p;
					if (cur().kind != Token::Kind::LBrace) return mkError("\\frac expects {num}{den}");
					++p; den = parseExpr();
					if (cur().kind != Token::Kind::RBrace) return mkError("\\frac expects {num}{den}");
					++p;
					auto f = mk(ExprKind::Frac);
					f->args.push_back(std::move(num));
					f->args.push_back(std::move(den));
					return f;
				}
				if (!isFuncCmd(name)) return mkError("unknown command \\" + name);
				auto c = mk(ExprKind::Call); c->name = name;
				c->args.push_back(parseArg());
				return c;
			}
			case Token::Kind::Error: ++p; return mk(ExprKind::Error);
			default: return mkError("unexpected token in expression");
		}
	}
};

} // namespace

bool isReservedName(const std::string& normalized) {
	static const std::set<std::string> r = {"sqrt", "frac", "sin", "cos", "tan", "log", "ln", "exp", "abs", "pi", "e", "j"};
	std::string base = normalized.substr(0, normalized.find('_'));
	return r.count(base) != 0;
}

ParsedFormula parseFormulaExpr(const std::string& src, int line) {
	ParsedFormula res;
	auto tokens = tokenize(src, res.diagnostics, line);
	if (tokens.front().kind == Token::Kind::End) {
		res.diagnostics.push_back({Diagnostic::Level::Error, "E010", line, "empty formula"});
		return res;
	}
	Parser pr{tokens, 0, line, res.diagnostics};
	res.tree = pr.parseAssign(res.lhs);
	res.emptyRhs = pr.emptyRhs;
	return res;
}

std::optional<Value> evaluate(const Expr& e, const ValueProvider& get, std::vector<Diagnostic>& diags) {
	auto warn = [&](const std::string& msg) {
		diags.push_back({Diagnostic::Level::Warning, "W001", e.line, msg});
		return std::nullopt;
	};
	switch (e.kind) {
		case ExprKind::Number: return Value(e.value, 0);
		case ExprKind::Constant:
			if (e.name == "pi") return Value(std::acos(-1.0), 0);   // не M_PI: на MSVC без дефайнов нет
			if (e.name == "e") return Value(std::exp(1.0), 0);
			if (e.name == "j") return Value(0, 1);
			return warn("unknown constant " + e.name);
		case ExprKind::Variable: {
			auto v = get(e.name);
			if (!v) diags.push_back({Diagnostic::Level::Error, "E005", e.line, "undefined variable " + e.name});
			return v;
		}
		case ExprKind::Unary: {
			auto a = evaluate(*e.args[0], get, diags);
			if (!a) return std::nullopt;
			return -*a;
		}
		case ExprKind::Binary: {
			auto a = evaluate(*e.args[0], get, diags);
			auto b = evaluate(*e.args[1], get, diags);
			if (!a || !b) return std::nullopt;
			switch (e.op) {
				case '+': return *a + *b;
				case '-': return *a - *b;
				case '*': return *a * *b;
				case '/':
					if (*b == Value(0, 0)) return warn("division by zero");
					return *a / *b;
				case '^': return std::pow(*a, *b);
			}
			return warn("unsupported operator");
		}
		case ExprKind::Frac: {
			auto a = evaluate(*e.args[0], get, diags);
			auto b = evaluate(*e.args[1], get, diags);
			if (!a || !b) return std::nullopt;
			if (*b == Value(0, 0)) return warn("division by zero");
			return *a / *b;
		}
		case ExprKind::Call: {
			auto a = evaluate(*e.args[0], get, diags);
			if (!a) return std::nullopt;
			const std::string& n = e.name;
			if (n == "sqrt") return std::sqrt(*a);
			if (n == "sin") return std::sin(*a);
			if (n == "cos") return std::cos(*a);
			if (n == "tan") return std::tan(*a);
			if (n == "ln") return std::log(*a);
			if (n == "log") return std::log(*a) / std::log(Value(10, 0));
			if (n == "exp") return std::exp(*a);
			if (n == "abs") return Value(std::abs(*a), 0);
			return warn("unknown function " + n);
		}
		case ExprKind::Error: return std::nullopt;
	}
	return std::nullopt;
}

} // namespace powercalc::document