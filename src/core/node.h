#pragma once

#include "types.h"

class Node {
public:

  // Фабрики
  static Node makePQ(NodeId id, double P_spec, double Q_spec,
                     double V_init = 1.0, double delta_init = 0.0);
  static Node makeSlack(NodeId id, double V_set_volts, double delta);

  // Геттеры
  NodeId id() const{
    return id_;
  }
  NodeType type() const{
    return type_;
  }

  double P_spec() const{
    return P_spec_;
  } // Активная мощность (Вт)
  double Q_spec() const{
    return Q_spec_;
  } // Реактивная мощность (вар)
  double V_set() const{
    return V_set_;
  } // Заданное напряжение (В) — для Slack

  double V_mag() const{
    return V_mag_;
  } // Модуль напряжения (p.u.)
  double delta() const{
    return delta_;
  } // Фаза напряжения (рад)

  // Сеттеры (для солвера)
  void setV(double V) {
    if (V <= 0)
      throw std::invalid_argument("Voltage must be positive");
    V_mag_ = V;
  }
  void setDelta(double delta) {
    // Нормализация в диапазон [-π, π]
    delta = std::fmod(delta + M_PI, 2 * M_PI);
    if (delta < 0)
      delta += 2 * M_PI;
    delta_ = delta - M_PI;
  }

private:
  Node(NodeId id, NodeType type, double P_spec, double Q_spec, double V_set,
       double V_mag, double delta)
      : id_(id), type_(type), P_spec_(P_spec), Q_spec_(Q_spec), V_set_(V_set),
        V_mag_(V_mag), delta_(delta) {}

  NodeId id_;
  NodeType type_;

  double P_spec_;
  double Q_spec_;
  double V_set_;

  double V_mag_;
  double delta_;
};

inline Node Node::makePQ(NodeId id, double P_spec, double Q_spec, double V_init,
                         double delta_init) {
  return Node(id, NodeType::PQ, P_spec, Q_spec, 0.0, V_init, delta_init);
}

inline Node Node::makeSlack(NodeId id, double V_set_volts, double delta) {
  return Node(id, NodeType::SLACK, 0.0, 0.0, V_set_volts, V_set_volts, delta);
}