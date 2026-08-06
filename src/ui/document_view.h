#pragma once
#include <QWebEngineView>
#include <QWebEnginePage>
#include <QWebEngineSettings>
#include <QDesktopServices>

namespace powercalc::ui {

class DocumentViewPage : public QWebEnginePage {
	Q_OBJECT
public:
	using QWebEnginePage::QWebEnginePage;
	bool acceptNavigationRequest(const QUrl& url, NavigationType, bool) override {
		const QString sch = url.scheme();
		if (sch == "qrc" || sch == "about" || sch == "data") return true; // data: — это наш setHtml
		QDesktopServices::openUrl(url); // http/mailto и т.п. — во внешний браузер
		return false;
	}
};

class DocumentView : public QWebEngineView {
	Q_OBJECT
public:
	explicit DocumentView(QWidget* parent = nullptr) : QWebEngineView(parent) {
		setPage(new DocumentViewPage(this));
		settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, false);
		connect(this, &QWebEngineView::loadFinished, this, [this](bool) {
			page()->runJavaScript(QString("window.scrollTo(0,%1)").arg(m_pendingScroll));
		});
	}
	void showHtml(const QString& html) {
		page()->runJavaScript("window.scrollY", [this, html](const QVariant& v) {
			m_pendingScroll = v.toInt();
			setHtml(html, QUrl("qrc:/katex/"));
		});
	}
private:
	int m_pendingScroll = 0;
};
}