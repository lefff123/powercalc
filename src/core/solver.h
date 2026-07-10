#pragma once

#include "powersystem.h"
#include "matrix.h"
#include "types.h"
#include <complex>
#include <cstddef>
#include <cstdio>
#include <vector>
#include <stdexcept>
#include <cmath>

#ifndef EPSILON
#define EPSILON 1e-12
#endif

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
		n_pv_ = 0;
		pv_node_indices_.clear();

        for (size_t i = 0; i < n; ++i) {
            V_mag_[i] = nodes[i].V_mag() / system_.V_base(nodes[i].id());
            delta_[i] = nodes[i].delta();
            types_[i] = nodes[i].type();
            is_slack_[i] = (types_[i] == NodeType::SLACK);

            if (types_[i] == NodeType::SLACK) {
                P_spec_pu_[i] = 0.0;
                Q_spec_pu_[i] = 0.0;
            } else if (types_[i] == NodeType::PV) {
				P_spec_pu_[i] += system_.P_oe(nodes[i]);
                Q_spec_pu_[i] = 0.;
                pv_node_indices_.push_back(i); // ← Запоминаем индекс PQ-узла
                n_pv_++;
			} else if (types_[i] == NodeType::PQ) {
                P_spec_pu_[i] -= system_.P_oe(nodes[i]);
                Q_spec_pu_[i] -= system_.Q_oe(nodes[i]);
                pq_node_indices_.push_back(i); // ← Запоминаем индекс PQ-узла
                n_pq_++;
            }
        }
    }
	Solver(PowerSystem &system)
        : system_(system)
    {
		options_ = Options();
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
        n_pv_ = 0;
        pv_node_indices_.clear();

        for (size_t i = 0; i < n; ++i) {
            V_mag_[i] = nodes[i].V_mag() / system_.V_base(nodes[i].id());
            delta_[i] = nodes[i].delta();
            types_[i] = nodes[i].type();
            is_slack_[i] = (types_[i] == NodeType::SLACK);

            if (types_[i] == NodeType::SLACK) {
                P_spec_pu_[i] = 0.0;
                Q_spec_pu_[i] = 0.0;
            } else if (types_[i] == NodeType::PV) {
				P_spec_pu_[i] += system_.P_oe(nodes[i]);
                Q_spec_pu_[i] = 0.;
                pv_node_indices_.push_back(i); // ← Запоминаем индекс PQ-узла
                n_pv_++;
			} else if (types_[i] == NodeType::PQ) {
                P_spec_pu_[i] -= system_.P_oe(nodes[i]);
                Q_spec_pu_[i] -= system_.Q_oe(nodes[i]);
                pq_node_indices_.push_back(i); // ← Запоминаем индекс PQ-узла
                n_pq_++;
            }
        }
    }
	//метод, вызывающий солвер
    Result solve(){
		Matrix<std::complex<double>> Y_bus = system_.buildYBus();
		system_.validate();
		double last_max_mismatch = 0.;
		for (size_t i = 0; i < options_.max_iterations; ++i){
			auto mismatches = calculateMismatches(Y_bus);
			
			// // ОТЛАДКА: вывод невязок
			// std::cout << "\n=== Итерация " << i << " ===" << std::endl;
			// std::cout << "Невязки: ";
			// for (size_t k = 0; k < mismatches.size(); ++k) {
			// 	std::cout << mismatches[k] << " ";
			// }
			// std::cout << std::endl;
			
			double max_mismatch = 0.;
			for (auto mismatch : mismatches){
				if (max_mismatch < std::abs(mismatch)){
					max_mismatch = std::abs(mismatch);
				}
			}
    		// std::cout << "Max mismatch: " << max_mismatch << std::endl;
			
			// Проверяем сходимость СРАЗУ после вычисления невязок
			if (max_mismatch < options_.tolerance){
				if (checkPVLimits(Y_bus)){
					i = 0;
					continue;
				}
				//обновляем pq
				for (size_t j = 0; j < n_pq_; ++j){
					size_t idx = pq_node_indices_[j];
					system_.getNodes()[idx].setV(V_mag_[idx] * system_.V_base(system_.getNodes()[idx].id()));
					system_.getNodes()[idx].setDelta(delta_[idx]);
				}

				//обновляем pv
				for (size_t j = 0; j < n_pv_; ++j){
					size_t idx = pv_node_indices_[j];
					system_.getNodes()[idx].setDelta(delta_[idx]);
				}

				//обновляем Q для pv узлов
				for (auto idx : pv_node_indices_){
					Q_spec_pu_[idx] = calc_Q_calc(Y_bus, idx);
				}
				return Result(true, i, max_mismatch); // i, а не i+1, т.к. итерация не понадобилась
			}
			
			auto J = buildJacobian(Y_bus);
			auto dx = solveLinearSystem(J, mismatches);
			updateVoltages(dx);
			last_max_mismatch = max_mismatch;
			if (checkPVLimits(Y_bus)) {
				i = 0;
				continue;
			}
		}
		
		

		return Result(false, options_.max_iterations, last_max_mismatch);
	}
	
	std::vector<double> calculateMismatches(const Matrix<std::complex<double>> &Y_bus) const
		{
			std::vector<double> mismatches(2 * n_pq_ + n_pv_, 0.);
			size_t pq_indx = 0;
			// считаем p
			for (size_t ii = 0; ii < n_pq_+ n_pv_; ++ii) {
				size_t i = (ii < n_pq_) ? pq_node_indices_[ii] : pv_node_indices_[ii - n_pq_];
				double calced = calc_P_calc(Y_bus, i);
				mismatches[pq_indx] = P_spec_pu_[i] - calced;
				++pq_indx;
			}
			// считаем Q
			for (size_t ii = 0; ii < n_pq_; ++ii) {
				size_t i = pq_node_indices_[ii];
				double calced = calc_Q_calc(Y_bus, i);
				mismatches[pq_indx] = Q_spec_pu_[i] - calced;
				++pq_indx;
			}

			return mismatches;
	}
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
	size_t n_pv_ = 0; //Количество PV-узлов
	std::vector<size_t> pv_node_indices_; //массив номеров PV узлов в матрице Якоби

    // Вспомогательные методы
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
        Matrix<double> J(2 * n_pq_ + n_pv_, 2 * n_pq_ + n_pv_);
		std::vector<size_t> all_non_slack(pq_node_indices_);
		size_t n_non_slack = n_pq_ + n_pv_;
		all_non_slack.reserve(pq_node_indices_.size()+pv_node_indices_.size());
		for (auto el : pv_node_indices_){
			all_non_slack.push_back(el);
		}
        /*
            Якобиан формируется как матрица = (H N
                                               M L)
        */
        // make H
        for (size_t ii = 0; ii < all_non_slack.size(); ++ii) {
            size_t i = all_non_slack[ii];
            for (size_t jj = 0; jj < all_non_slack.size(); ++jj) {
                size_t j = all_non_slack[jj];
                if (ii == jj) {
                    J(ii, jj) = -calc_Q_calc(Y_bus, i) - V_mag_[i] * V_mag_[j] * Y_bus(i, j).imag();
                } else {
                    J(ii, jj) = V_mag_[i] * V_mag_[j] * (Y_bus(i, j).real() * std::sin(delta_[i] - delta_[j]) - Y_bus(i, j).imag() * std::cos(delta_[i] - delta_[j]));
                }
            }
        }
        // make N
        for (size_t ii = 0; ii < all_non_slack.size(); ++ii) {
            size_t i = all_non_slack[ii];
            for (size_t jj = 0; jj < n_pq_; ++jj) {
                size_t j = pq_node_indices_[jj];
                if (i == j) {
                    J(ii, jj + n_non_slack) = calc_P_calc(Y_bus, i) / V_mag_[i] + V_mag_[i] * Y_bus(i, j).real();
                } else {
                    J(ii, jj + n_non_slack) = V_mag_[i] * (Y_bus(i, j).real() * std::cos(delta_[i] - delta_[j]) + Y_bus(i, j).imag() * std::sin(delta_[i] - delta_[j]));
                }
            }
        }
        // make M
        for (size_t ii = 0; ii < n_pq_; ++ii) {
            size_t i = pq_node_indices_[ii];
            for (size_t jj = 0; jj < all_non_slack.size(); ++jj) {
                size_t j = all_non_slack[jj];
                if (i == j) {
                    J(ii + n_non_slack, jj) = calc_P_calc(Y_bus, i) - V_mag_[i] * V_mag_[j] * Y_bus(i, j).real();
                } else {
                    J(ii + n_non_slack, jj) = -V_mag_[i] * V_mag_[j] * (Y_bus(i, j).real() * std::cos(delta_[i] - delta_[j]) + Y_bus(i, j).imag() * std::sin(delta_[i] - delta_[j]));
                }
            }
        }
        // make L
        for (size_t ii = 0; ii < n_pq_; ++ii) {
            size_t i = pq_node_indices_[ii];
            for (size_t jj = 0; jj < n_pq_; ++jj) {
                size_t j = pq_node_indices_[jj];
                if (ii == jj) {
                    J(ii + n_non_slack, jj + n_non_slack) = calc_Q_calc(Y_bus, i) / V_mag_[i] - V_mag_[i] * Y_bus(i, j).imag();
                } else {
                    J(ii + n_non_slack, jj + n_non_slack) =
                            V_mag_[i] * (Y_bus(i, j).real() * std::sin(delta_[i] - delta_[j]) - Y_bus(i, j).imag() * std::cos(delta_[i] - delta_[j]));
                }
            }
        }
        return J;
    }
    void updateVoltages(const std::vector<double> &dx)
    {
        if (dx.size() != 2 * n_pq_ + n_pv_) {
            throw std::invalid_argument("вектор dx не соответствует размеру вектора напряжений!");
        }
        for (size_t ii = 0; ii < n_pq_; ++ii) {
            size_t i = pq_node_indices_[ii];
            delta_[i] += dx[ii];
            V_mag_[i] += dx[ii + n_pq_ + n_pv_];
        }

        for (size_t ii = 0; ii < n_pv_; ++ii) {
			size_t i = pv_node_indices_[ii];
			delta_[i] += dx[ii + n_pq_];
        }
    }
    std::vector<double> solveLinearSystem(Matrix<double> J, std::vector<double> F) const
    {
        std::vector<double> dx(F.size(), 0);

        //_____________________ПРЯМОЙ ХОД____________________________
        for (long col = 0; col < J.cols(); ++col){
            //находим индекс строчки с максимальным элементом в ней
            size_t maxRow = col;
            for (size_t i = col + 1; i < J.rows(); ++i){
                if (std::abs(J(maxRow, col)) < std::abs(J(i, col))) maxRow = i;
            }
            if (std::abs(J(maxRow, col)) < EPSILON) throw std::runtime_error("Matrix is single"); //проверка на вырожденность
            // меняем строки местами
            if (maxRow != col) {
                for (size_t j = 0; j < J.cols(); ++j) {
                    std::swap(J(col, j), J(maxRow, j));
                }
                std::swap(F[col], F[maxRow]);
            }
            
            // обнуляем очередной столбец
            //вычитаем из нижестоящих строчек очередную 
            for (size_t row = col + 1; row < J.rows(); ++row){
                double factor = J(row, col) / J(col, col); //коэффициент домножения
                for (size_t i = col; i < J.cols(); ++i){
                    J(row, i) -= J(col, i) * factor;
                }
                F[row] -= F[col] * factor; //вычитаем и из вектора невязок тоже
            }
             //добавляем +1 к номеру следующего начального ряда
        }
        //_________________________ОБРАТНЫЙ ХОД______________________________
        for (long row = J.rows() - 1; row >= 0; --row){
            double sum_eq = 0.;
            for (size_t i = row + 1; i < J.cols(); ++i){
                sum_eq += J(row, i) * dx[i];
            }
            dx[row] = (F[row] - sum_eq)/J(row, row);
        }
        return dx;
    }

	void convertPVtoPQ(size_t idx, double Q_fixed){
		system_.getNodes()[idx].setType(NodeType::PQ);
    	system_.getNodes()[idx].setQ_spec(Q_fixed);
		
		types_[idx] = NodeType::PQ;
		Q_spec_pu_[idx] = -Q_fixed / system_.S_base();

		auto it = std::find(pv_node_indices_.begin(), pv_node_indices_.end(), idx);
		if (it != pv_node_indices_.end()) {
			pv_node_indices_.erase(it);
		}

		pq_node_indices_.push_back(idx);
		--n_pv_;
		++n_pq_;
		std::sort(pq_node_indices_.begin(), pq_node_indices_.end());
	}

    bool checkPVLimits(const Matrix<std::complex<double>> &Y_bus){
		std::vector<std::pair<size_t, double>> to_convert;
		
		for(auto idx : pv_node_indices_){
			auto Q_curr = calc_Q_calc(Y_bus, idx) * system_.S_base();
			
			std::cout << "PV узел " << idx << ": Q_curr = " << Q_curr / 1e6 << " Мвар, "
					<< "Q_min = " << system_.getNodes()[idx].Q_min() / 1e6 << " Мвар, "
					<< "Q_max = " << system_.getNodes()[idx].Q_max() / 1e6 << " Мвар" << std::endl;
			
			if (Q_curr > system_.getNodes()[idx].Q_max()){
				std::cout << "  → Преобразуем в PQ с Q = " << system_.getNodes()[idx].Q_max() / 1e6 << " Мвар" << std::endl;
				to_convert.push_back({idx, system_.getNodes()[idx].Q_max()});
			}
			else if (Q_curr < system_.getNodes()[idx].Q_min()){
				std::cout << "  → Преобразуем в PQ с Q = " << system_.getNodes()[idx].Q_min() / 1e6 << " Мвар" << std::endl;
				to_convert.push_back({idx, system_.getNodes()[idx].Q_min()});
			}
		}
		
		for (const auto& [idx, Q] : to_convert) {
			convertPVtoPQ(idx, Q);
			std::cout << "  После преобразования: Q_spec = " << system_.getNodes()[idx].Q_spec() / 1e6 << " Мвар" << std::endl;
		}
		
		return !to_convert.empty();
	}
};