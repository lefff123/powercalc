#include "image_scheme_handler.h"
#include <QWebEngineUrlRequestJob>
#include <QFile>
#include <QBuffer>

namespace powercalc::ui {

static QByteArray mimeFor(const QString& name) {
	if (name.endsWith(".svg", Qt::CaseInsensitive)) return "image/svg+xml";
	if (name.endsWith(".jpg", Qt::CaseInsensitive) || name.endsWith(".jpeg", Qt::CaseInsensitive)) return "image/jpeg";
	return "image/png";
}

void ImageSchemeHandler::requestStarted(QWebEngineUrlRequestJob* job) {
	QUrl url = job->requestUrl();
	QString name = url.path();
	if (name.isEmpty() || name == "/") name = url.host();
	if (name.startsWith('/')) name.remove(0, 1);
	QFile* f = new QFile(m_dir + "/" + name);
	if (m_dir.isEmpty() || !f->open(QIODevice::ReadOnly)) {
		delete f;
		job->fail(QWebEngineUrlRequestJob::UrlNotFound);
		return;
	}
	// QWebEngineUrlRequestJob сам удалит device после завершения
	job->reply(mimeFor(name), f);
}

} // namespace powercalc::ui