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

class PowerSystem
{
public:
	struct LineFlows {
		size_t line_id;
		size_t from_node;
		size_t to_node;
		std::complex<double> S_from;  // Мощность в начале линии (ВА)
		std::complex<double> S_to;    // Мощность в конце линии (ВА)
		std::complex<double> S_loss;  // Потери (ВА) = S_from + S_to
	};

    PowerSystem(double S_base, double V_base)
        : S_base_(S_base), V_base_(V_base), Z_base_((V_base * V_base / S_base)), Y_base_(1 / Z_base_)
    {
        if (S_base_ <= 0) {
            throw std::invalid_argument("Base power should be not below 0!");
        }
        if (V_base_ < 0) {
            throw std::invalid_argument("Base voltage should be not below 0!");
        }
    }

    // Добавление элементов
    void addNode(const Node &node)
    {
        if (id_to_index_.count(node.id())) {
            throw std::invalid_argument("Node with id " + std::to_string(node.id()) + " already exists");
        }
        id_to_index_[node.id()] = nodes_.size();
        nodes_.push_back(node);
    }

    void addLine(const Line &line)
    {
        if (findLineIndex(line.id()).has_value()) {
            throw std::invalid_argument("Line with id " + std::to_string(line.id()) + " already exists");
        }
        if (!hasNode(line.from())) {
            throw std::invalid_argument("Line references non-existent node: " + std::to_string(line.from()));
        }
        if (!hasNode(line.to())) {
            throw std::invalid_argument("Line references non-existent node: " + std::to_string(line.to()));
        }
        lines_.push_back(line);
    }

    // Доступ к элементам
    const Node &getNode(NodeId id) const
    {
        auto idx = getNodeIndex(id);
        return nodes_[idx];
    }
    Node &getNode(NodeId id)
    {
        auto idx = getNodeIndex(id);
        return nodes_[idx];
    }

    const Line &getLine(LineId id) const
    {
        auto idx = findLineIndex(id);
        if (!idx.has_value()) {
            throw std::out_of_range("Node not found: " + std::to_string(id));
        }
        return lines_[*idx]; // *idx — разыменовываем optional
    }
    std::vector<Node> &getNodes()
    {
        return nodes_;
    }
	const std::vector<Node> &getNodes() const
    {
        return nodes_;
    }
    const std::vector<Line> &getLines() const
    {
        return lines_;
    }

    size_t nodesCount() const
    {
        return nodes_.size();
    }
    size_t linesCount() const
    {
        return lines_.size();
    }

    // Базисные величины
    double S_base() const
    {
        return S_base_;
    }
    double V_base() const
    {
        return V_base_;
    }
    double Z_base() const
    {
        return Z_base_;
    }
    double Y_base() const
    {
        return Y_base_;
    }

    // Конвертация в o.e.
    double R_oe(const Line &line) const
    {
        return line.R() / Z_base_;
    }
    double X_oe(const Line &line) const
    {
        return line.X() / Z_base_;
    }
    std::complex<double> Z_oe(const Line &line) const
    {
        return std::complex<double>(R_oe(line), X_oe(line));
    }
    std::complex<double> Y_oe(const Line &line) const
    {
        return std::complex<double>(1) / Z_oe(line);
    }

    double P_oe(const Node &node) const
    {
        return node.P_spec() / S_base_;
    }
    double Q_oe(const Node &node) const
    {
        return node.Q_spec() / S_base_;
    }
    double V_oe(const Node &node) const
    {
        return node.V_mag() / V_base_;
    }

    // Валидация сети
    void validate() const
    {
        if (nodes_.empty()) {
            throw std::invalid_argument("Network has no nodes");
        }
        for (const auto &node : nodes_) {
            if (node.type() == NodeType::SLACK)
                return; // Нашли Slack — всё ок
        }
        throw std::invalid_argument("Network must have at least one Slack node");
    }

    // Создание матрицы проводимостей
    Matrix<std::complex<double>> buildYBus() const
    {
        Matrix<std::complex<double>> Y_bus(nodes_.size(), nodes_.size());
        // заполняем проводимостями
        for (const auto &line : lines_) {
            if (!line.isEnabled())
                continue;
            auto idx_from = getNodeIndex(line.from());
            auto idx_to = getNodeIndex(line.to());

            // не-диагональные элементы матрицы
            Y_bus(idx_from, idx_to) -= Y_oe(line);
            Y_bus(idx_to, idx_from) -= Y_oe(line);

            // диагональные элементы
            Y_bus(idx_to, idx_to) += Y_oe(line);
            Y_bus(idx_from, idx_from) += Y_oe(line);
        }
        return Y_bus;
    }

    size_t getNodeIndex(NodeId id) const
    {
        auto it = id_to_index_.find(id);
        if (it == id_to_index_.end()) {
            throw std::out_of_range("Node not found: " + std::to_string(id));
        }
        return it->second;
    }

    bool hasNode(NodeId id) const
    {
        return id_to_index_.count(id) > 0;
    }
	
	std::vector<LineFlows> calculateLineFlows() const{
		std::vector<LineFlows> flows;
		flows.reserve(lines_.size());
		for (auto line : lines_){
			//находим индексы точек по id
			size_t i = getNodeIndex(line.from());
			size_t j = getNodeIndex(line.to());
			//вычисляем комплексные напряжения в точках
			std::complex<double> V_i = (nodes_[i].V_mag() / V_base_) * std::complex<double>(std::cos(nodes_[i].delta()), std::sin(nodes_[i].delta()));
			std::complex<double> V_j = (nodes_[j].V_mag() / V_base_) * std::complex<double>(std::cos(nodes_[j].delta()), std::sin(nodes_[j].delta()));
			//вычисляем ток и мощности
			auto I_from = Y_oe(line) * (V_i - V_j);
			auto I_to = Y_oe(line) * (V_j - V_i );
			auto S_from_pu = V_i * std::conj(I_from);
			auto S_to_pu = V_j * std::conj(I_to);
			flows.push_back(LineFlows(line.id(), line.from(), line.to(), S_from_pu * S_base_, S_to_pu * S_base_, (S_from_pu + S_to_pu) * S_base_));
		}
		return flows;
	}

	// Расчёт комплексной мощности в Slack-узле
	std::complex<double> calculateSlackPower() const {
		auto Y_bus = buildYBus();
		size_t n = nodes_.size();
		
		// Находим Slack-узел
		size_t slack_idx = 0;
		for (size_t i = 0; i < n; ++i) {
			if (nodes_[i].type() == NodeType::SLACK) {
				slack_idx = i;
				break;
			}
		}
		
		// Собираем комплексные напряжения
		std::vector<std::complex<double>> V(n);
		for (size_t i = 0; i < n; ++i) {
			double v_pu = nodes_[i].V_mag() / V_base_;
			double delta = nodes_[i].delta();
			V[i] = v_pu * std::complex<double>(std::cos(delta), std::sin(delta));
		}
		
		// S_slack = V_slack * conj(sum(Y_slack,j * V_j))
		std::complex<double> I_slack(0.0, 0.0);
		for (size_t j = 0; j < n; ++j) {
			I_slack += Y_bus(slack_idx, j) * V[j];
		}
		
		std::complex<double> S_slack_pu = V[slack_idx] * std::conj(I_slack);
		
		// Переводим в ВА
		return S_slack_pu * S_base_;
	}
private:
    std::vector<Node> nodes_;
    std::vector<Line> lines_;
    std::unordered_map<NodeId, size_t> id_to_index_;

    double S_base_;
    double V_base_;
    double Z_base_;
    double Y_base_;

    std::optional<size_t> findLineIndex(LineId id) const
    {
        for (size_t i = 0; i < lines_.size(); ++i) {
            if (id == lines_[i].id()) {
                return i;
            }
        }
        return std::nullopt;
    }
};
