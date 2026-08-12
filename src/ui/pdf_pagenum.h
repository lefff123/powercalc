#pragma once
#include <QString>

namespace powercalc::ui {
bool addPageNumbers(const QString& src, const QString& dst, int startNumber,
					bool numberFirstPage, double bottomMarginMm, const QString& pageSize,
					QString* err, int* pagesOut = nullptr);
}