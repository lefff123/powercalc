#include "document_editor.h"
#include <QTextCursor>
#include <QTextBlock>
#include <QVBoxLayout>
#include <algorithm>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QBuffer>
#include <QImage>
#include <QKeyEvent>
#include <QKeySequence>
#include <QApplication>
#include <QClipboard>
#include <QFile>

namespace powercalc::ui {

DocumentEditor::DocumentEditor(QWidget* parent)
	: QSplitter(Qt::Vertical, parent)
{
	m_edit = new QTextEdit(this);
	m_edit->setAcceptRichText(false);
	m_edit->setFont(QFont("Courier New", 10));
	m_edit->installEventFilter(this);
	
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

	m_edit->setAcceptDrops(true);
}

void DocumentEditor::insertAtCursor(const QString& text) {
	m_edit->insertPlainText(text);
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

	m_allDiags = m_ast.diagnostics;
	m_allDiags.insert(m_allDiags.end(), m_evalResult.diagnostics.begin(), m_evalResult.diagnostics.end());
	std::sort(m_allDiags.begin(), m_allDiags.end(),
			  [](const powercalc::document::Diagnostic& a, const powercalc::document::Diagnostic& b) {
				  return a.line < b.line;
			  });

	updateErrorHighlights();
	updateErrorList();

	int errors = 0, warnings = 0;
	for (const auto& d : m_allDiags) {
		if (d.level == powercalc::document::Diagnostic::Level::Error) ++errors;
		else ++warnings;
	}
	emit diagnosticCountChanged(errors, warnings);
	emit documentChanged();
}

void DocumentEditor::onErrorClicked(QListWidgetItem* item) {
	int idx = m_errorList->row(item);
	if (idx < 0 || idx >= static_cast<int>(m_allDiags.size())) return;
	int line = m_allDiags[idx].line;
	QTextBlock block = m_edit->document()->findBlockByNumber(line - 1);
	if (block.isValid()) {
		QTextCursor cursor(block);
		m_edit->setTextCursor(cursor);
		m_edit->ensureCursorVisible();
	}
}

void DocumentEditor::updateErrorHighlights() {
	QList<QTextEdit::ExtraSelection> extras;
	for (const auto& diag : m_allDiags) {
		QTextBlock block = m_edit->document()->findBlockByNumber(diag.line - 1);
		if (!block.isValid()) continue;
		QTextEdit::ExtraSelection sel;
		sel.cursor = QTextCursor(block);
		sel.cursor.select(QTextCursor::LineUnderCursor);
		sel.format.setUnderlineStyle(QTextCharFormat::WaveUnderline);
		sel.format.setUnderlineColor(
			diag.level == powercalc::document::Diagnostic::Level::Error
				? Qt::red : QColor(255, 165, 0));
		extras.append(sel);
	}
	m_edit->setExtraSelections(extras);
}

void DocumentEditor::updateErrorList() {
	m_errorList->clear();
	if (m_allDiags.empty()) {
		m_errorList->setVisible(false);
		return;
	}
	m_errorList->setVisible(true);
	for (const auto& diag : m_allDiags) {
		QString item = QString("%1 — %2 — %3")
			.arg(diag.line)
			.arg(QString::fromStdString(diag.code))
			.arg(QString::fromStdString(diag.message));
		m_errorList->addItem(item);
	}
}

static bool hasLocalImage(const QMimeData* md) {
	if (!md->hasUrls()) return false;
	for (const QUrl& u : md->urls()) {
		if (!u.isLocalFile()) return false;
		const QString f = u.toLocalFile().toLower();
		if (!(f.endsWith(".png") || f.endsWith(".jpg") || f.endsWith(".jpeg") || f.endsWith(".svg")))
			return false;
	}
	return true;
}

static bool looksLikeImage(const QString& path) {
	const QString f = path.toLower();
	if (f.endsWith(".png") || f.endsWith(".jpg") || f.endsWith(".jpeg") || f.endsWith(".svg") ||
		f.endsWith(".webp") || f.endsWith(".gif") || f.endsWith(".bmp")) return true;
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly)) return false;
	const QByteArray h = file.read(12);
	return h.startsWith("\x89PNG") || h.startsWith("\xff\xd8") || h.startsWith("GIF8") ||
		   h.startsWith("BM") || h.contains("<svg") || h.contains("<?xml") ||
		   (h.startsWith("RIFF") && h.mid(8, 4) == "WEBP");
}

bool DocumentEditor::eventFilter(QObject* o, QEvent* e)
{
	if (o == m_edit) {
		// Ctrl+V со скриншотом в буфере
		if (e->type() == QEvent::KeyPress) {
			auto* ke = static_cast<QKeyEvent*>(e);
			if (ke->matches(QKeySequence::Paste)) {
				const QMimeData* md = QApplication::clipboard()->mimeData();
				if (md->hasImage()) {
					QImage img = qvariant_cast<QImage>(md->imageData());
					QByteArray ba;
					QBuffer buf(&ba);
					buf.open(QIODevice::WriteOnly);
					img.save(&buf, "PNG");
					emit imageDataDropped(ba);
					return true;
				}
			}
		}
		const QMimeData* md = nullptr;
		if (e->type() == QEvent::DragEnter) md = static_cast<QDragEnterEvent*>(e)->mimeData();
		else if (e->type() == QEvent::DragMove) md = static_cast<QDragMoveEvent*>(e)->mimeData();
		else if (e->type() == QEvent::Drop) md = static_cast<QDropEvent*>(e)->mimeData();
		if (md && (hasLocalImage(md) || md->hasImage())) {
			if (e->type() == QEvent::Drop) {
				if (hasLocalImage(md)) {
					QStringList paths;
					for (const QUrl& u : md->urls()) paths << u.toLocalFile();
					emit imageDropRequested(paths);
				} else {
					QImage img = qvariant_cast<QImage>(md->imageData());
					QByteArray ba;
					QBuffer buf(&ba);
					buf.open(QIODevice::WriteOnly);
					img.save(&buf, "PNG");
					emit imageDataDropped(ba);
				}
				static_cast<QDropEvent*>(e)->acceptProposedAction();
			} else {
				e->accept();
			}
			return true;
		}
		if (md && md->hasUrls()) {
			QStringList http;
			for (const QUrl& u : md->urls())
				if (u.scheme().startsWith("http")) http << u.toString();
			if (!http.isEmpty()) {
				if (e->type() == QEvent::Drop) {
					emit urlDropRequested(http);
					static_cast<QDropEvent*>(e)->acceptProposedAction();
				} else {
					e->accept();
				}
				return true;
			}
			e->ignore(); // не-картинковые ссылки не вставляем
			return true;
		}
	}
	return QSplitter::eventFilter(o, e);
}
} // namespace powercalc::ui