#include "html_generator.h"
#include "number_format.h"
#include <map>
#include <sstream>
#include <algorithm>

namespace {
std::string trim(const std::string& s) {
	size_t b = 0, e = s.size();
	while (b < e && (s[b] == ' ' || s[b] == '\t' || s[b] == '\r')) ++b;
	while (e > b && (s[e - 1] == ' ' || s[e - 1] == '\t' || s[e - 1] == '\r')) --e;
	return s.substr(b, e - b);
}
static double mmOf(const std::string& s) {
	double v = std::atof(s.c_str());
	if (s.size() > 2 && s.compare(s.size() - 2, 2, "cm") == 0) return v * 10.0;
	return v;
}
static double pageHeightMm(const std::string& ps) {
	if (ps == "A3") return 420; if (ps == "A5") return 210; if (ps == "Letter") return 279.4;
	return 297;
}
} // namespace

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

void renderLine(std::ostringstream& h, const std::string& ln, const std::vector<InlineRef>& refs, int lineNo,
				const std::map<const InlineRef*, InlineValue>* iv) {
	size_t pos = 0;
	for (const auto& ref : refs) {
		if (ref.line != lineNo) continue;
		const size_t st = static_cast<size_t>(ref.col) - 1;
		if (st < pos || st + ref.length > ln.size() + 1) continue;
		h << esc(ln.substr(pos, st - pos));
		if (ref.compute && iv) {
			auto it = iv->find(&ref);
			if (it != iv->end() && it->second.value) {
				h << "\\(";
				if (!it->second.name.empty()) h << esc(it->second.name) << " = ";
				h << esc(formatValue(*it->second.value));
				h << "\\)";
				pos = st + ref.length;
				continue;
			}
		}
		std::string r = ref.raw;
		if (!r.empty() && r[0] == '!') r = trim(r.substr(1));
		h << "<span class=\"pc-inline\">\\(" << esc(r) << "\\)</span>";
		pos = st + ref.length;
	}
	h << esc(ln.substr(pos));
}

std::string styleAttr(const Block& b, const std::string& fallbackAlign, const std::string& extra = "") {
	std::string align = b.localAlign.empty() ? fallbackAlign : b.localAlign;
	std::string s = extra + "text-align:" + align + ";";
	if (!b.localSize.empty()) s += "font-size:" + b.localSize + ";";
	return " style=\"" + s + "\"";
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
	  << ".pc-toc ul { list-style: none; padding-left: 0; margin: 0.4em 0; }\n"
	  << ".pc-toc li { display: flex; align-items: baseline; margin: 0.25em 0; }\n"
	  << ".pc-toc a { color: inherit; text-decoration: none; }\n"
	  << ".pc-toc .dots { flex: 1; border-bottom: 1.5px dotted #000000; margin: 0 0.4em; transform: translateY(-0.25em); }\n"
	  << ".pc-toc .pg { white-space: nowrap; }\n"
	  << "ul, ol { margin: 0.4em 0; padding-left: 2em; }\n"
	  << "table { border-collapse: collapse; margin: 0.6em auto; }\n"
	  << "td, th { border: 1px solid black; padding: 4px 8px; }\n"
	  << "th { font-weight: normal; }\n"
	  << "img { max-width: 100%; }\n"
	  << ".pc-img-cap { margin-top: 0.3em; }\n"
	  << ".pc-missing { color: gray; }\n"
	  << "p, .pc-formula { overflow-wrap: break-word; line-height: 1.9; }\n"
	  << "p { hyphens: auto; }\n"
	  << "</style>\n<body>\n";

	struct HeadInfo { int level; std::string text; int id; };
	std::vector<HeadInfo> heads;
	std::vector<int> skipIds;
	{
		int sec = 0;
		for (size_t bi = 0; bi < ast.blocks.size(); ++bi) {
			const Block& bb = ast.blocks[bi];
			if (bb.kind == BlockKind::Heading) heads.push_back({bb.level, bb.text, ++sec});
			else if (bb.kind == BlockKind::Toc && bi > 0 && ast.blocks[bi - 1].kind == BlockKind::Heading)
				skipIds.push_back(sec);
		}
	}
	int headCounter = 0;

		bool breakNext = false;
	for (const Block& b : ast.blocks) {
		const std::string brk = breakNext ? "page-break-before: always;" : "";
		breakNext = false;
		switch (b.kind) {
		case BlockKind::Yaml: break;
		case BlockKind::Heading: {
			++headCounter;
			const std::string fall = m.headingAlign.empty() ? m.align : m.headingAlign;
			h << "<h" << b.level << " id=\"sec-" << headCounter << "\"" << styleAttr(b, fall, brk)
			  << ">" << esc(b.text) << "</h" << b.level << ">\n";
			break;
		}

		case BlockKind::Toc: {
			std::string st = brk;
			if (!m.tocSize.empty()) st += "font-size:" + esc(m.tocSize) + ";";
			h << "<div class=\"pc-toc\"" << (st.empty() ? std::string() : " style=\"" + st + "\"") << "><ul>";
			for (const auto& hd : heads) {
				if (std::find(skipIds.begin(), skipIds.end(), hd.id) != skipIds.end()) continue;
				h << "<li style=\"padding-left:" << (hd.level - 1) * 1.2 << "em;\">"
				  << "<a href=\"#sec-" << hd.id << "\">" << esc(hd.text) << "</a>"
				  << "<span class=\"dots\"></span><span class=\"pg\" data-sec=\"" << hd.id << "\"></span></li>";
			}
			h << "</ul></div>\n";
			break;
		}
		case BlockKind::Text: {
			h << "<p" << styleAttr(b, m.align, brk) << ">";
			const auto ls = lines(b.text);
			for (size_t li = 0; li < ls.size(); ++li) {
				if (li) h << "<br>";
				renderLine(h, ls[li], b.inlines, b.lineBegin + static_cast<int>(li), &res.inlineValues);
			}
			h << "</p>\n";
			break;
		}
		case BlockKind::List: {
			std::string st = brk + "text-align:left;";
			if (!b.localSize.empty()) st += "font-size:" + b.localSize + ";";
			h << "<div style=\"" << st << "\">";
			std::vector<bool> stack;
			bool liOpen = false;
			for (const auto& it : b.items) {
				while (static_cast<int>(stack.size()) > it.level + 1) {
					if (liOpen) { h << "</li>"; liOpen = false; }
					h << (stack.back() ? "</ol>" : "</ul>");
					stack.pop_back();
				}
				if (static_cast<int>(stack.size()) == it.level + 1) {
					if (liOpen) { h << "</li>"; liOpen = false; }
					if (stack.back() != it.ordered) {
						h << (stack.back() ? "</ol>" : "</ul>");
						stack.pop_back();
					}
				}
				while (static_cast<int>(stack.size()) < it.level + 1) {
					h << (it.ordered ? "<ol>" : "<ul>");
					stack.push_back(it.ordered);
				}
				h << "<li>";
				liOpen = true;
				renderLine(h, it.text, it.inlines, it.line, &res.inlineValues);
			}
			while (!stack.empty()) {
				if (liOpen) { h << "</li>"; liOpen = false; }
				h << (stack.back() ? "</ol>" : "</ul>");
				stack.pop_back();
			}
			h << "</div>\n";
			break;
		}
		case BlockKind::Table: {
			h << "<table" << (brk.empty() ? std::string() : " style=\"" + brk + "\"") << ">\n";
			for (const auto& r : b.rows) {
				h << "<tr>";
				for (const auto& c : r.cells) {
					h << (r.header ? "<th" : "<td");
					const char* al = c.align == 'l' ? "left" : c.align == 'r' ? "right" : "center";
					h << " style=\"text-align:" << al << "\"";
					h << ">";
					renderLine(h, c.text, c.inlines, r.line, &res.inlineValues);
					h << (r.header ? "</th>" : "</td>");
				}
				h << "</tr>\n";
			}
			h << "</table>\n";
			break;
		}
		case BlockKind::Image: {
			std::string uri = opts.imageResolver ? opts.imageResolver(b.imageName) : "";
			if (uri.empty()) {
				h << "<p><span class=\"pc-missing\">[image: " << esc(b.imageName) << "]</span></p>\n";
			} else {
				h << "<div style=\"" << brk << "text-align:center\"><img src=\"" << uri << "\" alt=\"" << esc(b.imageAlt) << "\">";
				if (!b.imageAlt.empty()) h << "<div class=\"pc-img-cap\">" << esc(b.imageAlt) << "</div>";
				h << "</div>\n";
			}
			break;
		}
		case BlockKind::Formula: {
			if (b.formula.hide) break;
			auto it = perBlock.find(&b);
			if (it != perBlock.end() && it->second->emptyRhs) {
				auto v = values.find(it->second->lhs);
				if (v != values.end()) {
					h << "<div class=\"pc-formula\"" << styleAttr(b, m.align, brk) << ">\\(\\displaystyle "
					  << esc(it->second->lhs) << " = " << esc(formatValue(v->second)) << "\\)</div>\n";
					break;
				}
			}
			h << "<div class=\"pc-formula\"" << styleAttr(b, m.align, brk) << ">\\(\\displaystyle "
			  << esc(b.formula.exprRaw) << "\\)";
			if (it != perBlock.end() && it->second->value) {
				const BlockEvalResult& br = *it->second;
				bool showSub = m.showSubstitution != b.formula.invertSubstitution;
				if (b.localSubstitution == 1) showSub = true;
				else if (b.localSubstitution == -1) showSub = false;
				if (showSub && !br.substitutedLatex.empty())
					h << " \\(\\displaystyle = " << esc(br.substitutedLatex)
					  << " = " << esc(formatValue(*br.value)) << "\\)";
				else
					h << " \\(\\displaystyle = " << esc(formatValue(*br.value)) << "\\)";
				if (!b.formula.unit.empty())
					h << " <span class=\"pc-unit\">" << esc(b.formula.unit) << "</span>";
			}
			h << "</div>\n";
			break;
		}
		case BlockKind::PageBreak:
			breakNext = true;
			break;
		}
	}

	h << "<script src=\"" << opts.assetPrefix << "katex.min.js\"></script>\n"
	  << "<script src=\"" << opts.assetPrefix << "contrib/auto-render.min.js\"></script>\n"
	  << "<script>renderMathInElement(document.body,{strict:false,delimiters:[{left:\"$$\",right:\"$$\",display:true},"
	  << "{left:\"\\\\(\",right:\"\\\\)\",display:false}]});</script>\n</body>\n";
	h << "<script>(function(){var mm=96/25.4;var ph=(" << (pageHeightMm(m.pageSize) - mmOf(m.marginTop) - mmOf(m.marginBottom))
	  << ")*mm;var first=null;document.querySelectorAll('.pc-toc .pg').forEach(function(el){"
	  << "var t=document.getElementById('sec-'+el.getAttribute('data-sec'));if(!t)return;"
	  << "var top=t.getBoundingClientRect().top+window.scrollY;if(first===null)first=top;"
	  << "el.textContent=Math.max(1,Math.floor((top-first)/ph)+1);});})();</script>\n";
	return h.str();
}
} //namespace powercalc::document