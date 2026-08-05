#pragma once

#include "types.h"
#include <stdexcept>

class Node
{
public:
	// Фабрики
	static Node makePQ(NodeId id, double P_spec, double Q_spec,
					   double V_init, double delta_init = 0.0, double V_nom = 110e3);
	static Node makeSlack(NodeId id, double V_set_volts, double delta, double V_nom = 110e3);
	static Node makePV(NodeId id, double P_spec, double V_set_volts, 
				   double V_init_volts, double delta_init, double V_nom = 110e3, double Q_min = -1e9, double Q_max = 1e9);
	// Геттеры
	NodeId id() const
	{
		return id_;
	}
	NodeType type() const
	{
		return type_;
	}

	double P_spec() const
	{
		return P_spec_;
	} // Активная мощность (Вт)
	double Q_spec() const
	{
		return Q_spec_;
	} // Реактивная мощность (вар)
	double Q_max() const{
		return Q_max_;
	}
	double Q_min() const{
		return Q_min_;
	}
	double V_set() const
	{
		return V_set_;
	} // Заданное напряжение (В) — для Slack
	double V_mag() const
	{
		return V_mag_;
	} // Модуль напряжения (В)
	double delta() const
	{
		return delta_;
	} // Фаза напряжения (рад)
	double V_nom() const{
		return V_nom_;
	}
	// Сеттеры (для солвера)
	void setV(double V)
	{
		if (V <= 0)
			throw std::invalid_argument("Voltage must be positive");
		V_mag_ = V;
	}
	void setDelta(double delta)
	{
		// Нормализация в диапазон [-π, π]
		delta = std::fmod(delta + M_PI, 2 * M_PI);
		if (delta < 0)
			delta += 2 * M_PI;
		delta_ = delta - M_PI;
	}
	// Сеттеры (для солвера)
	void setType(NodeType type) { type_ = type; }
	void setQ_spec(double Q_spec) { Q_spec_ = Q_spec; }
	void setP_spec(double P) { P_spec_ = P; }
	void setV_set(double V) { V_set_ = V; }
	void setQ_min(double Q) { Q_min_ = Q; }
	void setQ_max(double Q) { Q_max_ = Q; }
	// для проверки на включенность
	void disconnect() { enabled_ = false; }
	void connect() { enabled_ = true; }
	bool isEnabled() const { return enabled_; }

private:
	private:
	Node(NodeId id, NodeType type, double P_spec, double Q_spec, double V_set,
		 double V_mag, double delta, double V_nom)
		: id_(id), type_(type), P_spec_(P_spec), Q_spec_(Q_spec), 
		  Q_min_(-1e9), Q_max_(1e9),  // ✅ Инициализируем сразу после Q_spec_
		  V_set_(V_set), V_nom_(V_nom), V_mag_(V_mag), delta_(delta) { }

	NodeId id_;
	NodeType type_;

	double P_spec_;
	double Q_spec_;
	double Q_min_;
	double Q_max_;
	double V_set_;
	double V_nom_;

	double V_mag_;
	double delta_;

	bool enabled_ = true;
};

inline Node Node::makePQ(NodeId id, double P_spec, double Q_spec, double V_init,
						 double delta_init, double V_nom) {
  return Node(id, NodeType::PQ, P_spec, Q_spec, 0.0, V_init, delta_init, V_nom);
}

inline Node Node::makeSlack(NodeId id, double V_set_volts, double delta, double V_nom)
{
	return Node(id, NodeType::SLACK, 0.0, 0.0, V_set_volts, V_set_volts, delta, V_nom);
}

inline Node Node::makePV(NodeId id, double P_spec, double V_set_volts,
						 double V_init_volts, double delta_init, double V_nom, double Q_min, double Q_max) {
	Node n =  Node(id, NodeType::PV, P_spec, 0.0, V_set_volts, V_init_volts, delta_init, V_nom);
	n.Q_min_ = Q_min;
	n.Q_max_ = Q_max;
	return n;
}