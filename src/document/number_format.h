#pragma once
#include "expr_evaluator.h"
#include <string>

namespace powercalc::document {
std::string formatReal(double v);
std::string formatValue(const Value& v);
}