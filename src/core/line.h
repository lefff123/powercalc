#pragma once

#include "types.h"

class Line {
public:
  // Конструктор линии
  // R, X — в Омах, k_t — коэффициент трансформации (для линии = 1.0)
  Line(LineId id, NodeId from, NodeId to, double R, double X, double k_t = 1.0)
    : id_(id), from_(from), to_(to), R_(R), X_(X), k_t_(k_t) {
    if (from == to) {
      throw std::invalid_argument("Line cannot connect a node to itself");
    }
    if (k_t <= 0) {
      throw std::invalid_argument("Transformation ratio must be positive");
    }
    // R и X могут быть нулевыми (идеальная линия), но не отрицательными
    if (R < 0 || X < 0) {
      throw std::invalid_argument(
          "Resistance and reactance must be non-negative");
    }
  }

  // Геттеры
  LineId id() const{
    return id_;
  }
  NodeId from() const{
    return from_;
  }
  NodeId to() const{
    return to_;
  }

  double R() const{
    return R_;
  }   // Активное сопротивление (Ом)
  double X() const{
    return X_;
  }   // Реактивное сопротивление (Ом)
  double k_t() const{
    return k_t_;
  } // Коэффициент трансформации

private:
  LineId id_;
  NodeId from_;
  NodeId to_;

  double R_;
  double X_;
  double k_t_;
};