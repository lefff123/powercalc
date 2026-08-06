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
	std::string name;   // нормализованное имя (пусто => symbolic)
	int line = 0;
	int col = 0;
	int length = 0;
	bool symbolic = false; // $$выражение$$ -> рендер как LaTeX inline
	bool compute = false; // ячейка таблицы: вычислять выражение
	std::string raw;
};

enum class BlockKind { Yaml, Heading, Formula, Text, List, Table, Image };

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

struct ListItem {
	int level = 0;        // 0..2 (отступ 2 пробела на уровень)
	bool ordered = false; // "1. " vs "- "
	int line = 0;
	std::string text;
	std::vector<InlineRef> inlines;
};

struct TableCell {
	std::string text;
	std::vector<InlineRef> inlines; // col — относительно текста ячейки
	char align = 0;                 // 0 / 'l' / 'c' / 'r'
};

struct TableRow {
	int line = 0;
	bool header = false;
	std::vector<TableCell> cells;
};

struct Block {
	BlockKind kind = BlockKind::Text;
	int lineBegin = 0, lineEnd = 0;   // 1-based, включительно
	std::string raw;
	int level = 0;        // Heading 1..3
	std::string text;     // Heading/Text
	FormulaInfo formula;
	std::vector<InlineRef> inlines;
	// List
	std::vector<ListItem> items;
	// Table
	std::vector<TableRow> rows;
	// Image
	std::string imageAlt;
	std::string imageName;  // имя файла, лежит в images/
	// локальные стили (суффикс {…} первой строки блока)
	std::string localAlign; // "", left/center/right/justify
	std::string localSize;  // "", например "12pt"
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