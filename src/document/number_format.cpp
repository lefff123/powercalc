#include "number_format.h"
#include <cmath>
#include <cstdio>

namespace powercalc::document {
namespace {
std::string fmt(double v) {
	long long n = std::llround((std::fabs(v) + 1e-9) * 1000.0); // half-up: 0.1235 -> 0.124
	long long ip = n / 1000, fp = n % 1000;
	char buf[64];
	const char* sign = (v < 0 && n != 0) ? "-" : "";
	if (fp == 0) std::snprintf(buf, sizeof(buf), "%s%lld", sign, ip);
	else {
		std::snprintf(buf, sizeof(buf), "%s%lld.%03lld", sign, ip, fp);
		std::string s(buf);
		while (!s.empty() && s.back() == '0') s.pop_back();
		if (!s.empty() && s.back() == '.') s.pop_back();
		return s;
	}
	return buf;
}
}

std::string formatReal(double v) { return fmt(v); }

std::string formatValue(const Value& v) {
	const double re = v.real(), im = v.imag();
	if (std::fabs(im) < 1e-9) return fmt(re);
	std::string rs = fmt(re);
	std::string ms = fmt(std::fabs(im));
	std::string jt;
	if (im > 0 && ms == "1") jt = "j";        // положительный коэффициент 1 опускается
	else jt = "j" + ms;                         // отрицательный коэффициент -1 остаётся
	if (rs == "0") return (im < 0 ? "-" : "") + jt;
	return rs + (im < 0 ? " - " : " + ") + jt;
}
}