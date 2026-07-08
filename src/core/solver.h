#pragma once

#include "powersystem.h"
#include "matrix.h"
#include <complex>
#include <cstddef>
#include <vector>
#include <stdexcept>
#include <cmath>

class Solver
{
public:
    struct Options
    {
        size_t max_iterations;
        double tolerance;

        Options()
            : max_iterations(20), tolerance(1e-6) { }
    };

    struct Result
    {
        bool converged = false;
        size_t iterations = 0;
        double max_mismatch = 0.0;
    };

    Solver(PowerSystem &system, const Options &opts)
        : system_(system), options_(opts)
    {
        size_t n = system_.nodesCount();
        const auto &nodes = system_.getNodes();
        // кэширование данных для быстрого доступа
        V_mag_.resize(n);
        delta_.resize(n);
        P_spec_pu_.resize(n);
        Q_spec_pu_.resize(n);
        types_.resize(n);
        is_slack_.resize(n);
        n_pq_ = 0;
        pq_node_indices_.clear();

        for (size_t i = 0; i < n; ++i) {
            V_mag_[i] = nodes[i].V_mag();
            delta_[i] = nodes[i].delta();
            types_[i] = nodes[i].type();
            is_slack_[i] = (types_[i] == NodeType::SLACK);

            if (is_slack_[i]) {
                P_spec_pu_[i] = 0.0;
                Q_spec_pu_[i] = 0.0;
            } else {
                P_spec_pu_[i] = system_.P_oe(nodes[i]);
                Q_spec_pu_[i] = system_.Q_oe(nodes[i]);
                pq_node_indices_.push_back(i); // ← Запоминаем индекс PQ-узла
                n_pq_++;
            }
        }
    }

    Result solve();

private:
    PowerSystem &system_;
    Options options_;

    // Кэшированные данные для быстрых вычислений
    std::vector<double> V_mag_; // Модули напряжений (p.u.)
    std::vector<double> delta_; // Углы (рад)
    std::vector<double> P_spec_pu_; // Заданная активная мощность (p.u.)
    std::vector<double> Q_spec_pu_; // Заданная реактивная мощность (p.u.)
    std::vector<NodeType> types_; // Типы узлов
    std::vector<bool> is_slack_; // Флаг: узел Slack?
    size_t n_pq_ = 0; // Количество PQ-узлов
    std::vector<size_t> pq_node_indices_; // массив номеров элементов в матрице Якоби

    // Вспомогательные методы
    std::vector<double> calculateMismatches(const Matrix<std::complex<double>> &Y_bus) const
    {
        std::vector<double> mismatches(2 * n_pq_, 0.);
        size_t pq_indx = 0;
        // считаем p
        for (size_t ii = 0; ii < n_pq_; ++ii) {
            size_t i = pq_node_indices_[ii];
            if (is_slack_[i])
                continue; // пропускаем базу
            double calced = calc_P_calc(Y_bus, i);
            mismatches[pq_indx] = P_spec_pu_[i] - calced;
            ++pq_indx;
        }
        // считаем Q
        for (size_t ii = 0; ii < n_pq_; ++ii) {
            size_t i = pq_node_indices_[ii];
            if (is_slack_[i])
                continue; // пропускаем базу
            double calced = calc_Q_calc(Y_bus, i);
            mismatches[pq_indx] = Q_spec_pu_[i] - calced;
            ++pq_indx;
        }

        return mismatches;
    }
    double calc_P_calc(const Matrix<std::complex<double>> &Y_bus, const size_t i) const
    {
        double calced = 0.;
        for (size_t j = 0; j < system_.nodesCount(); ++j) {
            double V = V_mag_[i] * V_mag_[j];
            double braces = Y_bus(i, j).real() * std::cos(delta_[i] - delta_[j]) + Y_bus(i, j).imag() * std::sin(delta_[i] - delta_[j]);
            calced += V * braces;
        }
        return calced;
    }
    double calc_Q_calc(const Matrix<std::complex<double>> &Y_bus, const size_t i) const
    {
        double calced = 0.;
        for (size_t j = 0; j < system_.nodesCount(); ++j) {
            double V = V_mag_[i] * V_mag_[j];
            double braces = Y_bus(i, j).real() * std::sin(delta_[i] - delta_[j]) - Y_bus(i, j).imag() * std::cos(delta_[i] - delta_[j]);
            calced += V * braces;
        }
        return calced;
    }
    Matrix<double> buildJacobian(const Matrix<std::complex<double>> &Y_bus) const
    {
        Matrix<double> J(2 * n_pq_, 2 * n_pq_);
        /*
            Якобиан формируется как матрица = (H N
                                               M L)
        */
        // make H
        for (size_t ii = 0; ii < n_pq_; ++ii) {
            size_t i = pq_node_indices_[ii];
            for (size_t jj = 0; jj < n_pq_; ++jj) {
                size_t j = pq_node_indices_[jj];
                if (ii == jj) {
                    J(ii, jj) = -calc_Q_calc(Y_bus, i) - V_mag_[i] * V_mag_[j] * Y_bus(i, j).imag();
                } else {
                    J(ii, jj) = V_mag_[i] * V_mag_[j] * (Y_bus(i, j).real() * std::sin(delta_[i] - delta_[j]) - Y_bus(i, j).imag() * std::cos(delta_[i] - delta_[j]));
                }
            }
        }
        // make N
        for (size_t ii = 0; ii < n_pq_; ++ii) {
            size_t i = pq_node_indices_[ii];
            for (size_t jj = 0; jj < n_pq_; ++jj) {
                size_t j = pq_node_indices_[jj];
                if (ii == jj) {
                    J(ii, jj + n_pq_) = calc_P_calc(Y_bus, i) / V_mag_[i] + V_mag_[i] * Y_bus(i, j).real();
                } else {
                    J(ii, jj + n_pq_) = V_mag_[i] * (Y_bus(i, j).real() * std::cos(delta_[i] - delta_[j]) + Y_bus(i, j).imag() * std::sin(delta_[i] - delta_[j]));
                }
            }
        }
        // make M
        for (size_t ii = 0; ii < n_pq_; ++ii) {
            size_t i = pq_node_indices_[ii];
            for (size_t jj = 0; jj < n_pq_; ++jj) {
                size_t j = pq_node_indices_[jj];
                if (ii == jj) {
                    J(ii + n_pq_, jj) = calc_P_calc(Y_bus, i) - V_mag_[i] * V_mag_[j] * Y_bus(i, j).real();
                } else {
                    J(ii + n_pq_, jj) = -V_mag_[i] * V_mag_[j] * (Y_bus(i, j).real() * std::cos(delta_[i] - delta_[j]) + Y_bus(i, j).imag() * std::sin(delta_[i] - delta_[j]));
                }
            }
        }
        // make L
        for (size_t ii = 0; ii < n_pq_; ++ii) {
            size_t i = pq_node_indices_[ii];
            for (size_t jj = 0; jj < n_pq_; ++jj) {
                size_t j = pq_node_indices_[jj];
                if (ii == jj) {
                    J(ii + n_pq_, jj + n_pq_) = calc_Q_calc(Y_bus, i) / V_mag_[i] - V_mag_[i] * Y_bus(i, j).imag();
                } else {
                    J(ii + n_pq_, jj + n_pq_) =
                            V_mag_[i] * (Y_bus(i, j).real() * std::sin(delta_[i] - delta_[j]) - Y_bus(i, j).imag() * std::cos(delta_[i] - delta_[j]));
                }
            }
        }
        return J;
    }
    void updateVoltages(const std::vector<double> &dx)
    {
        if (dx.size() != 2 * n_pq_) {
            throw std::invalid_argument("вектор dx не соответствует размеру вектора напряжений!");
        }
        for (size_t ii = 0; ii < n_pq_ - 1; ++ii) {
            size_t i = pq_node_indices_[ii];
            if (is_slack_[i])
                continue;
            delta_[i] += dx[ii];
            V_mag_[i] += dx[ii + n_pq_];
        }
    }
    std::vector<double> solveLinearSystem(Matrix<double> J, std::vector<double> F) const
    {
        auto J_internal = J;
        auto F_internal = F;
        std::vector<double> dx;

        return dx;
    }
};