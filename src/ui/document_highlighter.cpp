#include "document_highlighter.h"
#include <QTextCharFormat>
#include <QFont>

namespace powercalc::ui {

DocumentHighlighter::DocumentHighlighter(QTextDocument* parent)
	: QSyntaxHighlighter(parent) {}

void DocumentHighlighter::highlightBlock(const QString& text) {
	int prevState = previousBlockState();
	
	// Внутри YAML-блока
	if (prevState == 1) {
		if (text.trimmed() == "---") {
			setCurrentBlockState(0);
			setFormat(0, text.length(), QColor(Qt::darkGray));
		} else {
			setCurrentBlockState(1);
			QTextCharFormat fmt;
			setFormat(0, text.length(), fmt);
		}
		return;
	}
	
	// Внутри блока формулы
	if (prevState == 2) {
		if (text.trimmed() == "$$") {
			setCurrentBlockState(0);
			setFormat(0, text.length(), QColor(Qt::blue));
		} else {
			setCurrentBlockState(2);
			QString trimmed = text.trimmed();
			if (trimmed.startsWith("#")) {
				setFormat(0, text.length(), QColor(Qt::darkGreen));
			} else {
				setFormat(0, text.length(), QColor(Qt::blue));
			}
		}
		return;
	}
	
	// Новое состояние
	QString trimmed = text.trimmed();
	
	if (trimmed == "---") {
		setCurrentBlockState(1);
		setFormat(0, text.length(), QColor(Qt::darkGray));
		return;
	}
	
	if (trimmed.startsWith("$$")) {
		setCurrentBlockState(2);
		setFormat(0, text.length(), QColor(Qt::blue));
		return;
	}
	
	if (trimmed.startsWith("# ") || trimmed.startsWith("## ") || trimmed.startsWith("### ")) {
		setCurrentBlockState(0);
		QTextCharFormat fmt;
		fmt.setFontWeight(QFont::Bold);
		setFormat(0, text.length(), fmt);
		return;
	}
	setCurrentBlockState(0);
	setFormat(0, text.length(), QColor(Qt::white));
}

} // namespace powercalc::ui