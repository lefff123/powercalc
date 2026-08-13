#include "aboutdialog.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>

AboutDialog::AboutDialog(QWidget* parent) : QDialog(parent) {
	setWindowTitle("О программе PowerCalc");
	setFixedSize(500, 400);
	
	auto layout = new QVBoxLayout(this);
	
	auto title = new QLabel("<h2>PowerCalc v1.3</h2>");
	title->setAlignment(Qt::AlignCenter);
	layout->addWidget(title);
	
	auto desc = new QLabel(
		"<p>Расчёт установившихся режимов электрических систем с "
		"редактором отчётов и живым превью.</p>"
		"<p><b>Зависимости:</b></p>"
		"<ul>"
		"<li>Qt 6.7.2 (Core, Widgets, WebEngineWidgets, Network) — LGPL 3.0</li>"
		"<li>QuaZip 1.4 (работа с ZIP-архивами) — LGPL 2.1</li>"
		"<li>yaml-cpp 0.8.0 (парсинг YAML) — MIT License</li>"
		"<li>KaTeX (рендеринг LaTeX) — MIT License</li>"
		"<li>qpdf (постобработка PDF для нумерации страниц) — Apache 2.0</li>"
		"</ul>"
		"<p>Лицензия: MIT</p>"
	);
	desc->setWordWrap(true);
	layout->addWidget(desc);
	
	auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
	connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
	layout->addWidget(buttonBox);
}