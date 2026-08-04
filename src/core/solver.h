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

class Solver
{
public:
	

	Solver(PowerSystem &system, const Options &opts);

	Solver(PowerSystem &system);

	//метод, вызывающий солвер
	Result solve();
	
	std::vector<double> calculateMismatches(const Matrix<std::complex<double>> &Y_bus) const;

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
	double calc_P_calc(const Matrix<std::complex<double>> &Y_bus, const size_t i) const;

	double calc_Q_calc(const Matrix<std::complex<double>> &Y_bus, const size_t i) const;

	Matrix<double> buildJacobian(const Matrix<std::complex<double>> &Y_bus) const;

	void updateVoltages(const std::vector<double> &dx);

	std::vector<double> solveLinearSystem(Matrix<double> J, std::vector<double> F) const;

	void convertPVtoPQ(size_t idx, double Q_fixed);

	bool checkPVLimits(const Matrix<std::complex<double>> &Y_bus);
};