#include "document_editor.h"
#include <QTextCursor>
#include <QTextBlock>
#include <QVBoxLayout>

namespace powercalc::ui {

DocumentEditor::DocumentEditor(QWidget* parent)
	: QSplitter(Qt::Vertical, parent)
{
	m_edit = new QTextEdit(this);
	m_edit->setAcceptRichText(false);
	m_edit->setFont(QFont("Courier New", 10));
	
	m_errorList = new QListWidget(this);
	m_errorList->setVisible(false);
	m_errorList->setMaximumHeight(150);
	
	addWidget(m_edit);
	addWidget(m_errorList);
	
	m_highlighter = new DocumentHighlighter(m_edit->document());
	
	m_timer = new QTimer(this);
	m_timer->setSingleShot(true);
	connect(m_timer, &QTimer::timeout, this, &DocumentEditor::reparse);
	connect(m_edit, &QTextEdit::textChanged, this, [this]() {
		m_timer->start(300);
	});
	
	connect(m_errorList, &QListWidget::itemClicked, this, &DocumentEditor::onErrorClicked);
}

QString DocumentEditor::text() const {
	return m_edit->toPlainText();
}

void DocumentEditor::setText(const QString& text) {
	m_edit->setPlainText(text);
	reparse();
}

void DocumentEditor::keyPressEvent(QKeyEvent* e) {
	if (e->modifiers() & Qt::ControlModifier) {
		if (e->key() == Qt::Key_1) { insertAtLineStart("# "); e->accept(); return; }
		if (e->key() == Qt::Key_2) { insertAtLineStart("## "); e->accept(); return; }
		if (e->key() == Qt::Key_3) { insertAtLineStart("### "); e->accept(); return; }
		if (e->key() == Qt::Key_F) { insertFormulaTemplate(); e->accept(); return; }
		if (e->key() == Qt::Key_H) { toggleHideModifier(); e->accept(); return; }
	}
	QSplitter::keyPressEvent(e);
}

void DocumentEditor::insertAtLineStart(const QString& prefix) {
	QTextCursor cursor = m_edit->textCursor();
	cursor.movePosition(QTextCursor::StartOfBlock);
	cursor.insertText(prefix);
	m_edit->setTextCursor(cursor);
}

void DocumentEditor::insertFormulaTemplate() {
	QTextCursor cursor = m_edit->textCursor();
	cursor.insertText("$$\n\n$$");
	cursor.movePosition(QTextCursor::Up);
	m_edit->setTextCursor(cursor);
}

void DocumentEditor::toggleHideModifier() {
	QTextCursor cursor = m_edit->textCursor();
	int blockNum = cursor.blockNumber();
	
	QTextBlock block = m_edit->document()->findBlockByNumber(blockNum);
	while (block.isValid()) {
		QString text = block.text().trimmed();
		if (text.startsWith("$$") && text != "$$") {
			QTextCursor modCursor(block);
			if (text == "$$hide") {
				modCursor.movePosition(QTextCursor::EndOfBlock);
				modCursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
				modCursor.insertText("$$");
			}
			return;
		}
		if (text == "$$") {
			QTextCursor modCursor(block);
			modCursor.movePosition(QTextCursor::EndOfBlock);
			modCursor.insertText("hide");
			return;
		}
		block = block.previous();
	}
}

void DocumentEditor::reparse() {
	QString text = m_edit->toPlainText();
	std::string utf8 = text.toStdString();
	m_ast = m_parser.parse(utf8);
	
	m_symbolTable.clear();
	m_evalResult = evaluateDocument(m_ast, m_symbolTable);
	
	updateErrorHighlights();
	updateErrorList();
	
	int errors = 0, warnings = 0;
	for (const auto& d : m_evalResult.diagnostics) {
		if (d.level == powercalc::document::Diagnostic::Level::Error) ++errors;
		else ++warnings;
	}
	emit diagnosticCountChanged(errors, warnings);
	emit documentChanged();
}

void DocumentEditor::onErrorClicked(QListWidgetItem* item) {
	int idx = m_errorList->row(item);
	if (idx < 0 || idx >= static_cast<int>(m_evalResult.diagnostics.size())) return;
	int line = m_evalResult.diagnostics[idx].line;
	QTextBlock block = m_edit->document()->findBlockByNumber(line - 1);
	if (block.isValid()) {
		QTextCursor cursor(block);
		m_edit->setTextCursor(cursor);
		m_edit->ensureCursorVisible();
	}
}

void DocumentEditor::updateErrorHighlights() {
	QList<QTextEdit::ExtraSelection> extras;
	for (const auto& diag : m_evalResult.diagnostics) {
		if (diag.level != powercalc::document::Diagnostic::Level::Error) continue;
		QTextBlock block = m_edit->document()->findBlockByNumber(diag.line - 1);
		if (!block.isValid()) continue;
		
		QTextEdit::ExtraSelection sel;
		sel.cursor = QTextCursor(block);
		sel.cursor.select(QTextCursor::LineUnderCursor);
		sel.format.setUnderlineColor(Qt::red);
		sel.format.setUnderlineStyle(QTextCharFormat::WaveUnderline);
		extras.append(sel);
	}
	m_edit->setExtraSelections(extras);
}

void DocumentEditor::updateErrorList() {
	m_errorList->clear();
	if (m_evalResult.diagnostics.empty()) {
		m_errorList->setVisible(false);
		return;
	}
	m_errorList->setVisible(true);
	for (const auto& diag : m_evalResult.diagnostics) {
		QString item = QString("%1 — %2 — %3")
			.arg(diag.line)
			.arg(QString::fromStdString(diag.code))
			.arg(QString::fromStdString(diag.message));
		m_errorList->addItem(item);
	}
}

} // namespace powercalc::ui