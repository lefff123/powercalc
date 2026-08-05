#pragma once
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace powercalc::document {

struct Diagnostic {
	enum class Level { Error, Warning };
	Level level = Level::Error;
	std::string code;   // "E001"..."W001"
	int line = 0;       // 1-based
	std::string message;
};

struct InlineRef {
	std::string name;   // нормализованное имя
	int line = 0;
	int col = 0;        // 1-based, байтовый
	int length = 0;     // длина вместе с $$
};

enum class BlockKind { Yaml, Heading, Formula, Text };

// --- дерево выражения 
enum class ExprKind { Number, Variable, Constant, Binary, Unary, Call, Frac, Error };

struct Expr {
	ExprKind kind = ExprKind::Number;
	double value = 0;
	std::string name;
	char op = 0;
	std::vector<std::unique_ptr<Expr>> args;
	int line = 0;
};
using ExprPtr = std::unique_ptr<Expr>;

struct FormulaInfo {
	std::string modifierRaw;
	bool hide = false;
	bool invertSubstitution = false;
	std::string unit;
	std::vector<std::pair<int, std::string>> comments; // строка, текст
	std::string exprRaw;   // склеенные строки
	int exprLine = 0;      // строка первого выражения
};

struct Block {
	BlockKind kind = BlockKind::Text;
	int lineBegin = 0, lineEnd = 0;   // 1-based, включительно
	std::string raw;
	int level = 0;        // Heading 1..3
	std::string text;     // Heading/Text
	FormulaInfo formula;
	std::vector<InlineRef> inlines;
};

struct DocumentMeta {
	bool present = false;
	int lineBegin = 0, lineEnd = 0;
	std::string title, author, date;
	bool showSubstitution = true;
	std::string pageSize = "A4";
	std::string marginTop = "2cm", marginBottom = "2cm", marginLeft = "2cm", marginRight = "2cm";
	std::string align = "justify";
	std::string textSize = "14pt";
	std::vector<std::pair<std::string, std::string>> unknownKeys; // путь, raw
};

struct DocumentAst {
	DocumentMeta meta;
	std::vector<Block> blocks;
	std::vector<Diagnostic> diagnostics;
};

} // namespace powercalc::document