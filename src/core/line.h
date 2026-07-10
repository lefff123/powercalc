#pragma once

#include "types.h"
#include <stdexcept>
#include <complex>

class Line
{
public:
    // Конструктор линии
    // R, X — в Омах, k_t — коэффициент трансформации (для линии = 1.0)
    Line(LineId id, NodeId from, NodeId to, double R, double X, std::complex<double> k_t = 1.0, std::complex<double> Y = std::complex<double>(0.,0.), bool is_transformer = false)
        : id_(id), from_(from), to_(to), R_(R), X_(X), k_t_(k_t), Y_(Y), is_transformer_(is_transformer)
    {
        if (from == to) {
            throw std::invalid_argument("Line cannot connect a node to itself");
        }
        if (k_t.real() <= 0) {
            throw std::invalid_argument("Transformation ratio must be positive");
        }
        // R и X могут быть нулевыми (идеальная линия), но не отрицательными
        if (R < 0 || X < 0) {
            throw std::invalid_argument(
                    "Resistance and reactance must be non-negative");
        }
    }

    // Геттеры
    LineId id() const { return id_; }
    NodeId from() const { return from_; }
    NodeId to() const { return to_; }

    double R() const { return R_; } // Активное сопротивление (Ом)
    double X() const { return X_; } // Реактивное сопротивление (Ом)
    std::complex<double> k_t() const { return k_t_; } // Коэффициент трансформации
	std::complex<double> Y() const { return Y_; }

    // для проверки на включенность
    void disconnect() { enabled_ = false; }
    void connect() { enabled_ = true; }
    bool isEnabled() const { return enabled_; }
	bool istransformer () const { return is_transformer_; }

private:
    LineId id_;
    NodeId from_;
    NodeId to_;

    double R_;
    double X_;
	std::complex<double> Y_;
    std::complex<double> k_t_;

    bool enabled_ = true;
	bool is_transformer_ = false;
};