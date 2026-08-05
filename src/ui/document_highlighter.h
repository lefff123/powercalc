#pragma once
#include <QSyntaxHighlighter>

namespace powercalc::ui {

class DocumentHighlighter : public QSyntaxHighlighter {
public:
	explicit DocumentHighlighter(QTextDocument* parent);

protected:
	void highlightBlock(const QString& text) override;
};

} // namespace powercalc::ui