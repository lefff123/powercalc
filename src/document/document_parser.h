#pragma once
#include "document_ast.h"
#include <string>

namespace powercalc::document {

class DocumentParser {
public:
	DocumentAst parse(const std::string& source) const;
};

} // namespace powercalc::document