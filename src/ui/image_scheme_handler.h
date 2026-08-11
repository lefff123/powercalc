#pragma once
#include <QWebEngineUrlSchemeHandler>
#include <QString>

namespace powercalc::ui {
class ImageSchemeHandler : public QWebEngineUrlSchemeHandler {
	Q_OBJECT
public:
	explicit ImageSchemeHandler(QObject *parent = nullptr) : QWebEngineUrlSchemeHandler(parent) {}
	void setImagesDir(const QString& dir) { m_dir = dir; }
	void requestStarted(QWebEngineUrlRequestJob* job) override;
private:
	QString m_dir;
};
}