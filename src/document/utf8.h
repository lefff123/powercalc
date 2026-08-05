#pragma once
#include <cstdint>
#include <string>

namespace powercalc::document::utf8 {

inline uint32_t decode(const std::string& s, size_t& i) {
	unsigned char c = s[i];
	uint32_t cp = 0; int extra = 0;
	if (c < 0x80) cp = c;
	else if ((c >> 5) == 0x6) { cp = c & 0x1F; extra = 1; }
	else if ((c >> 4) == 0xE) { cp = c & 0x0F; extra = 2; }
	else if ((c >> 3) == 0x1E) { cp = c & 0x07; extra = 3; }
	else { ++i; return 0xFFFD; }
	++i;
	while (extra-- > 0 && i < s.size()) cp = (cp << 6) | (s[i++] & 0x3F);
	return cp;
}

inline bool isLetter(uint32_t cp) {
	return (cp >= 'a' && cp <= 'z') || (cp >= 'A' && cp <= 'Z') || (cp >= 0x400 && cp <= 0x4FF);
}
inline bool isDigit(uint32_t cp) { return cp >= '0' && cp <= '9'; }

inline std::string readName(const std::string& s, size_t& i) {
	size_t start = i, j = i;
	if (j >= s.size() || !isLetter(decode(s, j))) return {};
	i = j;
	while (i < s.size()) {
		j = i;
		uint32_t cp = decode(s, j);
		if (isLetter(cp) || isDigit(cp)) i = j; else break;
	}
	return s.substr(start, i - start);
}

struct NameScan { std::string normalized; bool ok = false; };

// читает optional _x / _{...} после базы, дописывает "_{...}" в out
inline bool readIndexSuffix(const std::string& s, size_t& i, std::string& out) {
	if (i >= s.size() || s[i] != '_') return true;
	size_t j = i + 1;
	if (j < s.size() && s[j] == '{') {
		size_t close = s.find('}', j + 1);
		if (close == std::string::npos) return false;
		std::string inner = s.substr(j + 1, close - j - 1);
		if (inner.empty() || inner.find_first_of("{}") != std::string::npos) return false;
		out += "_{" + inner + "}";
		i = close + 1;
	} else if (j < s.size()) {
		size_t k = j;
		uint32_t cp = decode(s, k);
		if (isLetter(cp) || isDigit(cp)) { out += "_{" + s.substr(j, k - j) + "}"; i = k; }
	}
	return true;
}

// имя + optional _x / _{...}; нормализация к имя_{индекс}
inline NameScan readVariable(const std::string& s, size_t& i) {
	NameScan r;
	std::string base = readName(s, i);
	if (base.empty()) return r;
	r.normalized = base;
	r.ok = true;
	r.ok = readIndexSuffix(s, i, r.normalized);
	return r;
}

} // namespace powercalc::document::utf8