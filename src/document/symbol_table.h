#pragma once
#include "expr_evaluator.h"
#include <optional>
#include <string>
#include <unordered_map>

namespace powercalc::document {

class SymbolTable {
public:
	void clear() { docLayer_.clear(); }
	void define(const std::string& name, Value v) { docLayer_[name] = v; }

	std::optional<Value> lookup(const std::string& name) const {
		auto it = docLayer_.find(name);
		if (it != docLayer_.end()) return it->second;
		if (external_) return external_(name);
		return std::nullopt;
	}

	// Слой 0 (внешний, PowerSystem) — в 2.2 nullptr, интерфейс готов для 2.3
	void setExternalProvider(ValueProvider p) { external_ = std::move(p); }

	ValueProvider asProvider() const {
		return [this](const std::string& n) -> std::optional<Value> { return lookup(n); };
	}

private:
	std::unordered_map<std::string, Value> docLayer_;
	ValueProvider external_;
};

} // namespace powercalc::document