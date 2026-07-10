#pragma once

#include <cmath>
#include <cstddef>

// Используем size_t для ID, чтобы соответствовать std::vector
using NodeId = size_t;
using LineId = size_t;

enum class NodeType {
    PQ, // Нагрузочный узел: заданы P и Q
    SLACK, // Балансирующий узел: заданы V и δ (обычно δ=0)
	PV
};