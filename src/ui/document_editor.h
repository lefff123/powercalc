#pragma once
#include <QSplitter>
#include <QTextEdit>
#include <QListWidget>
#include <QTimer>
#include <QKeyEvent>

#include "document_ast.h"
#include "document_parser.h"
#include "document_evaluator.h"
#include "document_highlighter.h"
#include "document_ast.h"

namespace powercalc::ui {

class DocumentEditor : public QSplitter {
	Q_OBJECT

public:
	explicit DocumentEditor(QWidget* parent = nullptr);

	QString text() const;
	void setText(const QString& text);
	const powercalc::document::DocumentAst& ast() const { return m_ast; }
	const powercalc::document::EvaluationResult& evalResult() const { return m_evalResult; }

signals:
	void documentChanged();
	void diagnosticCountChanged(int errors, int warnings);

protected:
	void keyPressEvent(QKeyEvent* e) override;

private slots:
	void reparse();
	void onErrorClicked(QListWidgetItem* item);

private:
	void insertAtLineStart(const QString& prefix);
	void insertFormulaTemplate();
	void toggleHideModifier();
	void updateErrorHighlights();
	void updateErrorList();

	QTextEdit* m_edit = nullptr;
	QListWidget* m_errorList = nullptr;
	QTimer* m_timer = nullptr;
	DocumentHighlighter* m_highlighter = nullptr;
	std::vector<powercalc::document::Diagnostic> m_allDiags;

	powercalc::document::DocumentParser m_parser;
	powercalc::document::DocumentAst m_ast;
	powercalc::document::SymbolTable m_symbolTable;
	powercalc::document::EvaluationResult m_evalResult;
};

} // namespace powercalc::ui