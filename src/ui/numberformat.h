#pragma once

#include <QString>

// до тысячных; недостающие знаки обрезаются (120.000 -> "120", 0.010472 -> "0.01")
inline QString formatDouble(double v)
{
	QString s = QString::number(v, 'f', 3);
	if (s.contains('.')) {
		while (s.endsWith('0'))
			s.chop(1);
		if (s.endsWith('.'))
			s.chop(1);
	}
	return s;
}