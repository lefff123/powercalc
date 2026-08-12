#include "pdf_pagenum.h"
#include <cstdio>
#include <QProcess>
#include <QPdfWriter>
#include <QPainter>
#include <QPageSize>
#include <QFile>
#include <QCoreApplication>
#include <QStandardPaths>

namespace powercalc::ui {

static QString findQpdf() {
	QString p = QStandardPaths::findExecutable("qpdf");
	if (p.isEmpty()) p = QStandardPaths::findExecutable("qpdf.exe");
	if (p.isEmpty()) {
		const QString local = QCoreApplication::applicationDirPath() + "/qpdf.exe";
		if (QFile::exists(local)) p = local;
	}
	return p;
}

bool addPageNumbers(const QString& src, const QString& dst, int startNumber,
					bool numberFirstPage, double bottomMarginMm, const QString& pageSize,
					QString* err, int* pagesOut)
{
	const QString qpdf = findQpdf();
	if (qpdf.isEmpty()) {
		if (err) *err = "qpdf не найден: установите qpdf (Linux) или положите qpdf.exe рядом с программой (Windows)";
		return false;
	}

	QProcess np;
	np.start(qpdf, QStringList() << "--show-npages" << src);
	np.waitForFinished(5000);
	const int count = np.readAllStandardOutput().trimmed().toInt();
	if (count <= 0) { if (err) *err = "qpdf: cannot get page count"; return false; }
	if (pagesOut) *pagesOut = count;
	fprintf(stderr, "[pagenum] pages=%d\n", count);

	const QString numPdf = src + ".numbers.pdf";
	{
		QPdfWriter w(numPdf);
		QPageSize ps(QPageSize::A4);
		if (pageSize == "A3") ps = QPageSize(QPageSize::A3);
		else if (pageSize == "A5") ps = QPageSize(QPageSize::A5);
		else if (pageSize == "Letter") ps = QPageSize(QPageSize::Letter);
		w.setPageSize(ps);
		w.setPageMargins(QMarginsF(0, 0, 0, 0));
		w.setResolution(300);
		QPainter p(&w);
		p.setFont(QFont("Times New Roman", 12));
		p.setPen(Qt::black);
		const double dpi = 300.0;
		for (int i = 0; i < count; ++i) {
			if (i > 0) w.newPage();
			if (i == 0 && !numberFirstPage) continue;
			const qreal bottomPx = bottomMarginMm / 25.4 * dpi;
			const QRectF r(0, w.height() - bottomPx * 0.75, w.width(), bottomPx * 0.5);
			p.drawText(r, Qt::AlignCenter, QString::number(startNumber + i));
			fprintf(stderr, "[pagenum] page %d -> '%d'\n", i + 1, startNumber + i);
		}
		p.end();
	}

	QProcess q;
	q.start(qpdf, QStringList() << src << "--overlay" << numPdf << "--" << dst);
	q.waitForFinished(10000);
	const QString qerr = QString::fromUtf8(q.readAllStandardError());
	QFile::remove(numPdf);
	if (q.exitCode() != 0) {
		if (err) *err = "qpdf overlay failed: " + qerr;
		return false;
	}
	fprintf(stderr, "[pagenum] saved %s\n", dst.toUtf8().constData());
	return true;
}

} // namespace powercalc::ui