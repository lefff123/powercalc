#pragma once

#include "line.h"
#include "node.h"
#include "types.h"
#include "matrix.h"
#include <complex>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include <queue>

struct LineFlows {
	size_t line_id;
	size_t from_node;
	size_t to_node;
	std::complex<double> S_from;  // Мощность в начале линии (ВА)
	std::complex<double> S_to;    // Мощность в конце линии (ВА)
	std::complex<double> S_loss;  // Потери (ВА) = S_from + S_to
};

class PowerSystem
{
public:
	PowerSystem(double S_base, double V_base);

	// Добавление элементов
	void addNode(const Node &node);

	void addLine(const Line &line);
	// Доступ к элементам
	const Node &getNode(NodeId id) const;

	Node& getNode(NodeId id);

	const Line &getLine(LineId id) const;

	Line &getLine(LineId id);

	std::vector<Node> &getNodes();

	const std::vector<Node> &getNodes() const;

	const std::vector<Line> &getLines() const;

	size_t nodesCount() const;

	size_t linesCount() const;

	// Базисные величины
	double S_base() const;

	double V_base() const;

	//Возвращаем базу для каждого узла отдельно
	double V_base(NodeId id) const;

	double Z_base() const;
	
	double Y_base() const;

	// Конвертация в o.e.
	double R_oe(const Line &line) const;

	double X_oe(const Line &line) const;

	std::complex<double> Z_oe(const Line &line) const;

	std::complex<double> Y_oe(const Line &line) const;

	double P_oe(const Node &node) const;

	double Q_oe(const Node &node) const;

	double V_oe(const Node &node) const;

	std::complex<double> Y_shunt_from_oe(const Line& line) const;

	std::complex<double> Y_shunt_to_oe(const Line& line) const;

	// Валидация сети
	void validate() const;

	// Создание матрицы проводимостей
	Matrix<std::complex<double>> buildYBus() const;

	size_t getNodeIndex(NodeId id) const;

	bool hasNode(NodeId id) const;
	
	std::vector<LineFlows> calculateLineFlows() const;

	// Расчёт комплексной мощности в Slack-узле
	std::complex<double> calculateSlackPower() const;

	// Отключение линии с инвалидацией кэша
	void disconnectLine(LineId id);

	// Включение линии с инвалидацией кэша
	void connectLine(LineId id);

	void clear();

private:
	std::vector<Node> nodes_;
	std::vector<Line> lines_;
	std::unordered_map<NodeId, size_t> id_to_index_;
	mutable std::vector<double> V_base_per_node_;
	mutable bool base_voltages_valid_ = false;

	double S_base_;
	double V_base_;
	double Z_base_;
	double Y_base_;

	std::optional<size_t> findLineIndex(LineId id) const;

	void recalculateBaseVoltages() const;
};
