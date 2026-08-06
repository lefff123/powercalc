#include "document_parser.h"
#include "utf8.h"

#include <yaml-cpp/yaml.h>

#include <regex>

namespace powercalc::document {

namespace {

std::string trim(const std::string& s) {
	size_t b = 0, e = s.size();
	while (b < e && (s[b] == ' ' || s[b] == '\t' || s[b] == '\r')) ++b;
	while (e > b && (s[e - 1] == ' ' || s[e - 1] == '\t' || s[e - 1] == '\r')) --e;
	return s.substr(b, e - b);
}

bool isEmpty(const std::string& s) { return trim(s).empty(); }

std::vector<std::string> splitLines(const std::string& src) {
	std::vector<std::string> out;
	size_t start = 0;
	while (true) {
		size_t nl = src.find('\n', start);
		std::string line = src.substr(start, nl == std::string::npos ? nl : nl - start);
		if (!line.empty() && line.back() == '\r') line.pop_back();
		out.push_back(line);
		if (nl == std::string::npos) break;
		start = nl + 1;
	}
	return out;
}

int headingLevel(const std::string& line) {
	size_t h = 0;
	while (h < line.size() && line[h] == '#') ++h;
	if (h == 0 || h > 3) return 0;
	if (h >= line.size() || line[h] != ' ') return 0;
	return static_cast<int>(h);
}

// строка открывает блок формулы, если начинается с $$ и это не inline
bool isFormulaOpen(const std::string& line) {
	if (line.rfind("$$", 0) != 0) return false;
	size_t close = line.find("$$", 2);
	if (close == std::string::npos) return true;
	if (!trim(line.substr(close + 2)).empty()) return false;
	std::string inner = trim(line.substr(2, close - 2));
	if (inner.empty()) return true;
	// начинается с модификатора -> это блок, а не inline
	for (const std::string& m : {"hide!", "hide", "!"}) {
		if (inner == m || (inner.rfind(m, 0) == 0 &&
			(inner[m.size()] == ' ' || inner[m.size()] == '\t')))
			return true;
	}
	size_t k = 0;
	if (inner[0] == '\\') k = 1;
	auto ns = utf8::readVariable(inner, k);
	return !(ns.ok && k == inner.size());
}

void addDiag(DocumentAst& ast, Diagnostic::Level lv, const std::string& code, int line, std::string msg) {
	ast.diagnostics.push_back({lv, code, line, std::move(msg)});
}

template <class T>
bool yamlAs(const YAML::Node& n, T& out) {
	try { out = n.as<T>(); return true; } catch (const YAML::Exception&) { return false; }
}

const std::regex reMargin(R"(^\d+(\.\d+)?(cm|mm)$)");
const std::regex rePt(R"(^\d+(\.\d+)?pt$)");

void fillMargin(const YAML::Node& m, DocumentMeta& meta, DocumentAst& ast, int yamlLine) {
	static const char* keys[4] = {"top", "bottom", "left", "right"};
	std::string* dst[4] = {&meta.marginTop, &meta.marginBottom, &meta.marginLeft, &meta.marginRight};
	if (!m.IsMap()) { addDiag(ast, Diagnostic::Level::Error, "E001", yamlLine, "page.margin must be a map"); return; }
	for (const auto& kv : m) {
		std::string key;
		if (!yamlAs(kv.first, key)) continue;
		std::string val;
		bool ok = yamlAs(kv.second, val) && std::regex_match(val, reMargin);
		for (int i = 0; i < 4; ++i)
			if (key == keys[i]) {
				if (ok) *dst[i] = val;
				else addDiag(ast, Diagnostic::Level::Error, "E001", yamlLine, "bad margin value: " + val);
			}
		// неизвестные ключи margin игнорируются (сохранятся raw-текстом)
	}
}

void fillMeta(const YAML::Node& root, DocumentAst& ast, int yamlLine) {
	DocumentMeta& meta = ast.meta;
	for (const auto& kv : root) {
		std::string key;
		if (!yamlAs(kv.first, key)) { addDiag(ast, Diagnostic::Level::Error, "E001", yamlLine, "bad YAML key"); continue; }
		const YAML::Node& v = kv.second;
		if (key == "title" || key == "author" || key == "date") {
			std::string s;
			if (!yamlAs(v, s)) addDiag(ast, Diagnostic::Level::Error, "E001", yamlLine, key + " must be a string");
			else if (key == "title") meta.title = s;
			else if (key == "author") meta.author = s;
			else meta.date = s;
		} else if (key == "show_substitution") {
			bool b;
			if (!yamlAs(v, b)) addDiag(ast, Diagnostic::Level::Error, "E001", yamlLine, "show_substitution must be bool");
			else meta.showSubstitution = b;
		} else if (key == "align") {
			std::string a;
			if (!yamlAs(v, a) || (a != "left" && a != "center" && a != "right" && a != "justify"))
				addDiag(ast, Diagnostic::Level::Error, "E001", yamlLine, "bad align: " + a);
			else meta.align = a;
		} else if (key == "page") {
			if (!v.IsMap()) { addDiag(ast, Diagnostic::Level::Error, "E001", yamlLine, "page must be a map"); continue; }
			for (const auto& pk : v) {
				std::string pkey;
				if (!yamlAs(pk.first, pkey)) continue;
				if (pkey == "size") { std::string s; if (yamlAs(pk.second, s)) meta.pageSize = s; }
				else if (pkey == "margin") fillMargin(pk.second, meta, ast, yamlLine);
				// неизвестные page.* игнорируются
			}
		} else if (key == "text") {
			if (!v.IsMap()) { addDiag(ast, Diagnostic::Level::Error, "E001", yamlLine, "text must be a map"); continue; }
			for (const auto& tk : v) {
				std::string tkey;
				if (!yamlAs(tk.first, tkey)) continue;
				if (tkey == "size") {
					std::string s;
					if (yamlAs(tk.second, s) && std::regex_match(s, rePt)) meta.textSize = s;
					else addDiag(ast, Diagnostic::Level::Error, "E001", yamlLine, "bad text.size: " + s);
				}
			}
		} else {
			// неизвестный ключ (включая base_voltage): игнорируем, но запоминаем
			YAML::Emitter em; em << v;
			meta.unknownKeys.emplace_back(key, em.c_str());
		}
	}
}

int parseYaml(const std::vector<std::string>& lines, int start, DocumentAst& ast) {
	const int n = static_cast<int>(lines.size());
	Block b;
	b.kind = BlockKind::Yaml;
	b.lineBegin = start + 1;
	int close = -1;
	for (int j = start + 1; j < n; ++j)
		if (trim(lines[j]) == "---") { close = j; break; }
	const int end = close == -1 ? n : close;
	b.lineEnd = close == -1 ? n : close + 1;
	if (close == -1) addDiag(ast, Diagnostic::Level::Error, "E002", start + 1, "unclosed YAML block");

	std::string inner;
	for (int j = start + 1; j < end; ++j) {
		const std::string& l = lines[j];
		for (char c : l) {
			if (c == '\t') { addDiag(ast, Diagnostic::Level::Error, "E001", j + 1, "tab in YAML indentation"); break; }
			if (c != ' ') break;
		}
		inner += l + "\n";
	}

	ast.meta.present = true;
	ast.meta.lineBegin = b.lineBegin;
	ast.meta.lineEnd = b.lineEnd;
	try {
		YAML::Node root = YAML::Load(inner);
		if (root.IsNull()) { /* пустой YAML — ок */ }
		else if (!root.IsMap()) addDiag(ast, Diagnostic::Level::Error, "E001", start + 1, "YAML root must be a map");
		else fillMeta(root, ast, start + 1);
	} catch (const YAML::Exception& e) {
		int l = start + 1 + (e.mark.line >= 0 ? static_cast<int>(e.mark.line) : 0);
		addDiag(ast, Diagnostic::Level::Error, "E001", l, std::string("YAML: ") + e.msg);
	}
	for (int j = start; j < (close == -1 ? n : close + 1); ++j)
		b.raw += (b.raw.empty() ? "" : "\n") + lines[j];
	ast.blocks.push_back(std::move(b));
	return close == -1 ? n : close + 1;
}

int parseFormula(const std::vector<std::string>& lines, int start, DocumentAst& ast) {
	const int n = static_cast<int>(lines.size());
	Block b;
	b.kind = BlockKind::Formula;
	b.lineBegin = start + 1;
	FormulaInfo& f = b.formula;

	std::vector<std::pair<int, std::string>> content;
	bool closed = false;
	int endLine = n - 1;

	// обрабатывает строку li с офсета off; true => блок закрылся на этой строке
	auto processLine = [&](int li, size_t off) -> bool {
	const std::string& s = lines[li];
	size_t i = off;
	std::string acc;
	while (i < s.size()) {
		if (s.compare(i, 2, "$$") == 0) {
			std::string t = trim(acc);
			if (!t.empty()) content.emplace_back(li + 1, t);
			endLine = li;
			return true;
		}
		if (s[i] == '#') {
			std::string t = trim(acc);
			if (!t.empty()) content.emplace_back(li + 1, t);
			size_t d = s.find("$$", i);
			if (d == std::string::npos) { 
				f.comments.emplace_back(li + 1, s.substr(i)); 
				return false;  // <-- было break
			}
			f.comments.emplace_back(li + 1, s.substr(i, d - i));
			endLine = li;
			return true;
		}
		size_t h = s.find('#', i);
		size_t d = s.find("$$", i);
		size_t stop = s.size();
		if (h != std::string::npos) stop = h;
		if (d != std::string::npos && d < stop) stop = d;
		acc += s.substr(i, stop - i);
		i = stop;
		}
		std::string t = trim(acc);
		if (!t.empty()) content.emplace_back(li + 1, t);
		return false;
	};

	// открывающая строка: модификатор + возможно контент
	{
		const std::string& s = lines[start];
		size_t i = 2;
		while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) ++i;
		size_t j = i;
		while (j < s.size() && s[j] != ' ' && s[j] != '\t' && s[j] != '$') ++j;
		std::string tok = s.substr(i, j - i);
		if (tok == "hide" || tok == "!" || tok == "hide!") {
			f.modifierRaw = tok;
			if (tok != "!") f.hide = true;
			if (tok != "hide") f.invertSubstitution = true;
			closed = processLine(start, j);
		} else if (!tok.empty() && trim(s.substr(j)).empty() &&
				   tok.find_first_of("=#\\&") == std::string::npos) {
			addDiag(ast, Diagnostic::Level::Error, "E003", start + 1, "unknown modifier: " + tok);
		} else {
			closed = processLine(start, i);
		}
	}

	int next;
	if (closed) next = endLine + 1;
	else {
		next = n;
		for (int li = start + 1; li < n; ++li)
			if (processLine(li, 0)) { closed = true; next = endLine + 1; break; }
	}
	if (!closed) {
		addDiag(ast, Diagnostic::Level::Error, "E002", start + 1, "unclosed formula block");
		endLine = n - 1;
		next = n;
	}
	b.lineEnd = endLine + 1;

	std::string expr;
	for (size_t ci = 0; ci < content.size(); ++ci) {
		std::string t = content[ci].second;
		if (ci + 1 == content.size()) {
			size_t amp = t.find('&');
			if (amp != std::string::npos) { f.unit = trim(t.substr(amp + 1)); t = trim(t.substr(0, amp)); }
		}
		if (!t.empty()) { if (!expr.empty()) expr += ' '; expr += t; }
	}
	f.exprRaw = expr;
	f.exprLine = content.empty() ? start + 1 : content.front().first;

	for (int j = start; j <= endLine && j < n; ++j)
		b.raw += (b.raw.empty() ? "" : "\n") + lines[j];
	ast.blocks.push_back(std::move(b));
	return next;
}

void scanInlines(Block& b, const std::string& line, int lineNo, DocumentAst& ast) {
	size_t p = 0;
	while ((p = line.find("$$", p)) != std::string::npos) {
		size_t close = line.find("$$", p + 2);
		if (close == std::string::npos) break;
		std::string content = trim(line.substr(p + 2, close - p - 2));
		if (content.empty()) {
			addDiag(ast, Diagnostic::Level::Warning, "W001", lineNo, "bad inline formula: " + content);
			p = close + 2;
			continue;
		}
		InlineRef ref;
		ref.line = lineNo;
		ref.col = static_cast<int>(p) + 1;
		ref.length = static_cast<int>(close - p) + 2;
		ref.symbolic = true;
		ref.raw = content;
		b.inlines.push_back(ref);
		p = close + 2;
	}
}

} // namespace

DocumentAst DocumentParser::parse(const std::string& source) const {
	DocumentAst ast;
	const auto lines = splitLines(source);
	const int n = static_cast<int>(lines.size());

	int firstNonEmpty = -1;
	for (int i = 0; i < n; ++i)
		if (!isEmpty(lines[i])) { firstNonEmpty = i; break; }

	int i = 0;
	while (i < n) {
		const std::string& line = lines[i];
		if (isEmpty(line)) { ++i; continue; }

		if (i == firstNonEmpty && trim(line) == "---") { i = parseYaml(lines, i, ast); continue; }

		int lvl = headingLevel(line);
		if (lvl) {
			Block b;
			b.kind = BlockKind::Heading;
			b.lineBegin = b.lineEnd = i + 1;
			b.level = lvl;
			b.text = trim(line.substr(lvl));
			b.raw = line;
			ast.blocks.push_back(std::move(b));
			++i;
			continue;
		}

		if (isFormulaOpen(line)) { i = parseFormula(lines, i, ast); continue; }

		Block b;
		b.kind = BlockKind::Text;
		const int start = i;
		std::string raw;
				while (i < n && !isEmpty(lines[i])) {
			if (headingLevel(lines[i])) break;
			if (isFormulaOpen(lines[i])) break;
			scanInlines(b, lines[i], i + 1, ast);
			raw += (raw.empty() ? "" : "\n") + lines[i];
			++i;
		}
		b.lineBegin = start + 1;
		b.lineEnd = i;
		b.raw = raw;
		b.text = raw;
		ast.blocks.push_back(std::move(b));
	}   // конец while (i < n)
	return ast;
}

} // namespace powercalc::document