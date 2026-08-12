#include "document_highlighter.h"
#include "utf8.h"
#include <QTextCharFormat>
#include <QFont>

namespace powercalc::ui {

namespace {

bool closesOnLine(const QString& s, int from) {
	for (int i = from; i < s.length(); ++i) {
		if (s.mid(i, 2) == "$$") return true;
		if (s[i] == '#') return s.indexOf("$$", i) != -1;
	}
	return false;
}

bool isFormulaOpenLine(const QString& line) {
	if (!line.startsWith("$$")) return false;
	int close = line.indexOf("$$", 2);
	if (close == -1) return true;
	if (!line.mid(close + 2).trimmed().isEmpty()) return false;
	QString inner = line.mid(2, close - 2).trimmed();
	if (inner.isEmpty()) return true;
	std::string s = inner.toStdString();
	size_t k = 0;
	if (!s.empty() && s[0] == '\\') k = 1;
	auto ns = powercalc::document::utf8::readVariable(s, k);
	return !(ns.ok && k == s.size());
}

} // namespace

DocumentHighlighter::DocumentHighlighter(QTextDocument* parent)
	: QSyntaxHighlighter(parent) {}

void DocumentHighlighter::highlightBlock(const QString& text) {
	int prev = previousBlockState();

	if (prev == 1) {
		if (text.trimmed() == "---") { setCurrentBlockState(0); setFormat(0, text.length(), QColor(Qt::darkGray)); }
		else {
			setCurrentBlockState(1);
			QTextCharFormat fmt;
			fmt.setBackground(QColor(87, 87, 87));
			setFormat(0, text.length(), fmt);
		}
		return;
	}

	if (prev == 2 || isFormulaOpenLine(text)) {
		bool closes = closesOnLine(text, prev == 2 ? 0 : 2);
		setFormat(0, text.length(), QColor(Qt::cyan));
		int c = text.indexOf('#');
		if (c != -1) {
			int d = text.indexOf("$$", c);
			setFormat(c, (d == -1 ? text.length() : d) - c, QColor(Qt::darkCyan));
		}
		setCurrentBlockState(closes ? 0 : 2);
		return;
	}

	QString trimmed = text.trimmed();
	if (trimmed == "---") { setCurrentBlockState(1); setFormat(0, text.length(), QColor(Qt::darkGray)); return; }
	if (trimmed.startsWith("# ") || trimmed.startsWith("## ") || trimmed.startsWith("### ")) {
		setCurrentBlockState(0);
		QTextCharFormat fmt;
		fmt.setFontWeight(QFont::Bold);
		setFormat(0, text.length(), fmt);
		return;
	}
	// inline-формулы в тексте
	int p = 0;
	while ((p = text.indexOf("$$", p)) != -1) {
		int c = text.indexOf("$$", p + 2);
		if (c == -1) break;
		setFormat(p, c - p + 2, QColor(Qt::cyan));
		p = c + 2;
	}
	setCurrentBlockState(0);
}

} // namespace powercalc::ui