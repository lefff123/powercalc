#include "html_generator.h"
#include "number_format.h"
#include <map>
#include <sstream>

namespace powercalc::document {
namespace {

std::string esc(const std::string& s) {
	std::string out; out.reserve(s.size());
	for (char c : s) {
		if (c == '&') out += "&amp;";
		else if (c == '<') out += "&lt;";
		else if (c == '>') out += "&gt;";
		else out += c;
	}
	return out;
}

std::vector<std::string> lines(const std::string& s) {
	std::vector<std::string> out; size_t st = 0;
	while (true) {
		size_t p = s.find('\n', st);
		out.push_back(s.substr(st, p == std::string::npos ? p : p - st));
		if (p == std::string::npos) return out;
		st = p + 1;
	}
}

} // namespace

std::string generateHtml(const DocumentAst& ast, const EvaluationResult& res, const HtmlOptions& opts) {
	std::map<const Block*, const BlockEvalResult*> perBlock;
	for (const auto& br : res.blocks) perBlock[br.block] = &br;
	std::map<std::string, Value> values; // последнее определение побеждает
	for (const auto& d : res.definitions) values[d.first] = d.second;

	const DocumentMeta& m = ast.meta;
	std::ostringstream h;
	h << "<!doctype html>\n<meta charset=\"utf-8\">\n" << "<html lang=\"ru\">\n";
	if (opts.exportMode) h << "<!-- PowerCalc " << POWERCALC_VERSION << " -->\n";
	h << "<link rel=\"stylesheet\" href=\"" << opts.assetPrefix << "katex.min.css\">\n";
	h << "<style>\n"
	  << "@page { size: " << esc(m.pageSize) << "; margin: "
	  << m.marginTop << ' ' << m.marginRight << ' ' << m.marginBottom << ' ' << m.marginLeft << "; }\n"
	  << "body { font-family: \"Times New Roman\", Times, serif; font-size: " << esc(m.textSize)
	  << "; text-align: " << m.align << "; }\n"
	  << ".pc-formula { margin: 0.6em 0; }\n"
	  << ".pc-missing { color: gray; }\n"
	  << "p, .pc-formula { overflow-wrap: break-word; line-height: 1.9; }\n"
	  << "p { hyphens: auto; }\n"
	  << "</style>\n<body>\n";

	for (const Block& b : ast.blocks) {
		switch (b.kind) {
		case BlockKind::Yaml: break;
		case BlockKind::Heading:
			h << "<h" << b.level << ">" << esc(b.text) << "</h" << b.level << ">\n";
			break;
		case BlockKind::Text: {
			h << "<p style=\"text-align:" << m.align << "\">";
			const auto ls = lines(b.text);
			for (size_t li = 0; li < ls.size(); ++li) {
				if (li) h << "<br>";
				const int lineNo = b.lineBegin + static_cast<int>(li);
				const std::string& ln = ls[li];
				size_t pos = 0;
				for (const auto& ref : b.inlines) {
					if (ref.line != lineNo) continue;
					const size_t st = static_cast<size_t>(ref.col) - 1;
					if (st < pos || st + ref.length > ln.size()) continue;
					h << esc(ln.substr(pos, st - pos));
					if (ref.symbolic) {
						h << "<span class=\"pc-inline\">\\(" << esc(ref.raw) << "\\)</span>";
					} else {
						auto it = values.find(ref.name);
						if (it == values.end()) h << "<span class=\"pc-missing\">?""?</span>";
						else h << "<span class=\"pc-inline\">" << esc(formatValue(it->second)) << "</span>";
					}
					pos = st + ref.length;
				}
				h << esc(ln.substr(pos));
			}
			h << "</p>\n";
			break;
		}
		case BlockKind::Formula: {
			if (b.formula.hide) break;
			auto it = perBlock.find(&b);
			// $$u=$$ : присваивание с пустой RHS -> "u = текущее значение"
			if (it != perBlock.end() && it->second->emptyRhs) {
				auto v = values.find(it->second->lhs);
				if (v != values.end()) {
					h << "<div class=\"pc-formula\">\\(\\displaystyle " << esc(it->second->lhs)
					<< " = " << esc(formatValue(v->second)) << "\\)</div>\n";
					break;
				}
			}
			h << "<div class=\"pc-formula\">\\(\\displaystyle " << esc(b.formula.exprRaw) << "\\)";
			if (it != perBlock.end() && it->second->value) {
				const BlockEvalResult& br = *it->second;
				const bool showSub = m.showSubstitution != b.formula.invertSubstitution;
				if (showSub && !br.substitutedLatex.empty())
					h << " \\(\\displaystyle = " << esc(br.substitutedLatex)
					<< " = " << esc(formatValue(*br.value)) << "\\)";
				else if (!showSub)
					h << " \\(\\displaystyle = " << esc(formatValue(*br.value)) << "\\)";
				if (!b.formula.unit.empty())
					h << " <span class=\"pc-unit\">" << esc(b.formula.unit) << "</span>";
			}
			h << "</div>\n";
			break;
		}
		}
	}

	h << "<script src=\"" << opts.assetPrefix << "katex.min.js\"></script>\n"
	  << "<script src=\"" << opts.assetPrefix << "contrib/auto-render.min.js\"></script>\n"
	  << "<script>renderMathInElement(document.body,{strict:false,delimiters:[{left:\"$$\",right:\"$$\",display:true},"
	  << "{left:\"\\\\(\",right:\"\\\\)\",display:false}]});</script>\n</body>\n";
	return h.str();
}
}