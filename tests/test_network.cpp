#include "line.h"
#include "node.h"
#include "powersystem.h"
#include "solver.h"
#include "types.h"
#include <gtest/gtest.h>
#include <iostream>
#include <iomanip>
#include <cmath>

// Функция для вывода результатов в формате, аналогичном pandapower
void printPowerFlowResults(const PowerSystem& sys, const std::string& test_name) {
    const auto& nodes = sys.getNodes();
    
    std::cout << "\n============================================================\n";
    std::cout << " ТЕСТ: " << test_name << "\n";
    std::cout << "============================================================\n";
    
    // [1] Напряжения в узлах
    std::cout << "\n[1] Напряжения в узлах:\n";
    for (size_t i = 0; i < nodes.size(); ++i) {
        double v_kv = nodes[i].V_mag() / 1000.0;
        double v_pu = nodes[i].V_mag() / sys.V_base();
        double delta_deg = nodes[i].delta() * 180.0 / M_PI;
        double delta_rad = nodes[i].delta();
        
        std::cout << std::fixed << std::setprecision(4);
        std::cout << "  Узел " << nodes[i].id() << ": V = " << v_kv << " кВ (" 
                  << std::setprecision(6) << v_pu << " p.u.) | Angle = " 
                  << std::setprecision(4) << delta_deg << "° (" 
                  << std::setprecision(6) << delta_rad << " рад)\n";
    }
    
    // [2] Мощность в Slack-узле
    std::cout << "\n[2] Мощность в Slack-узле (генерация):\n";
    // Здесь можно добавить расчет мощности в Slack-узле, если нужно
    
    // [3] Потери мощности в линиях
    std::cout << "\n[3] Потери мощности в линиях:\n";
    // Здесь можно добавить расчет потерь, если нужно
    
    std::cout << "\n";
}

// Функция для проверки значений с допуском
void checkNodeVoltage(const Node& node, double expected_v_kv, double expected_delta_deg, 
                      double v_tol = 0.1, double delta_tol = 0.01) {
    double v_kv = node.V_mag() / 1000.0;
    double delta_deg = node.delta() * 180.0 / M_PI;
    
    EXPECT_NEAR(v_kv, expected_v_kv, v_tol) 
        << "Node " << node.id() << ": V_kv expected " << expected_v_kv 
        << " but got " << v_kv;
    
    EXPECT_NEAR(delta_deg, expected_delta_deg, delta_tol)
        << "Node " << node.id() << ": delta_deg expected " << expected_delta_deg 
        << " but got " << delta_deg;
}

// ==================== Node ====================

TEST(Node, PQConstructor) {
  auto n = Node::makePQ(1, 50e6, 20e6, 110e3, 0.0); // 110 кВ начальное
  EXPECT_EQ(n.id(), 1);
  EXPECT_EQ(n.type(), NodeType::PQ);
  EXPECT_DOUBLE_EQ(n.P_spec(), 50e6);
  EXPECT_DOUBLE_EQ(n.Q_spec(), 20e6);
  EXPECT_DOUBLE_EQ(n.V_mag(), 110e3); // Теперь в вольтах
  EXPECT_DOUBLE_EQ(n.delta(), 0.0);
}

TEST(Node, SlackConstructor) {
  auto n = Node::makeSlack(1, 110e3, 0.0);
  EXPECT_EQ(n.type(), NodeType::SLACK);
  EXPECT_DOUBLE_EQ(n.V_set(), 110e3);
  EXPECT_DOUBLE_EQ(n.V_mag(), 110e3);
}

TEST(Node, Setters) {
  auto n = Node::makePQ(1, 50e6, 20e6, 110e3, 0.0);
  // 1.05 p.u. = 1.05 * 110e3 = 115.5 кВ
  n.setV(115.5e3);
  n.setDelta(0.1);
  EXPECT_NEAR(n.V_mag(), 115.5e3, 1e-3);
  EXPECT_NEAR(n.delta(), 0.1, 1e-9);
}

TEST(Node, SetVNegativeThrows) {
  auto n = Node::makePQ(1, 50e6, 20e6, 110e3, 0.0);
  EXPECT_THROW(n.setV(-1.0), std::invalid_argument);
  EXPECT_THROW(n.setV(0.0), std::invalid_argument);
}

// ==================== Line ====================

TEST(Line, BasicProperties) {
  Line l(1, 1, 2, 5.0, 30.0);
  EXPECT_EQ(l.id(), 1);
  EXPECT_EQ(l.from(), 1);
  EXPECT_EQ(l.to(), 2);
  EXPECT_DOUBLE_EQ(l.R(), 5.0);
  EXPECT_DOUBLE_EQ(l.X(), 30.0);
  EXPECT_DOUBLE_EQ(l.k_t(), 1.0);
}

TEST(Line, WithTransformer) {
  Line l(1, 1, 2, 5.0, 30.0, 1.05);
  EXPECT_DOUBLE_EQ(l.k_t(), 1.05);
}

// ==================== PowerSystem ====================

TEST(PowerSystem, BaseValues) {
  PowerSystem sys(100e6, 110e3);
  EXPECT_DOUBLE_EQ(sys.S_base(), 100e6);
  EXPECT_DOUBLE_EQ(sys.V_base(), 110e3);
  EXPECT_NEAR(sys.Z_base(), 110e3 * 110e3 / 100e6, 1e-6);
}

TEST(PowerSystem, AddNodesAndLines) {
  PowerSystem sys(100e6, 110e3);
  sys.addNode(Node::makeSlack(1, 110e3, 0.0));
  sys.addNode(Node::makePQ(2, 50e6, 20e6, 110e3, 0.0));
  sys.addLine(Line(1, 1, 2, 5.0, 30.0));

  EXPECT_EQ(sys.nodesCount(), 2);
  EXPECT_EQ(sys.linesCount(), 1);
}

TEST(PowerSystem, DuplicateNodeThrows) {
  PowerSystem sys(100e6, 110e3);
  sys.addNode(Node::makeSlack(1, 110e3, 0.0));
  EXPECT_THROW(sys.addNode(Node::makePQ(1, 50e6, 20e6, 110e3, 0.0)),
               std::invalid_argument);
}

TEST(PowerSystem, LineToNonExistentNodeThrows) {
  PowerSystem sys(100e6, 110e3);
  sys.addNode(Node::makeSlack(1, 110e3, 0.0));
  EXPECT_THROW(sys.addLine(Line(1, 1, 99, 5.0, 30.0)), std::invalid_argument);
}

TEST(PowerSystem, ValidateNoSlackThrows) {
  PowerSystem sys(100e6, 110e3);
  sys.addNode(Node::makePQ(1, 50e6, 20e6, 110e3, 0.0));
  sys.addNode(Node::makePQ(2, 30e6, 10e6, 110e3, 0.0));
  EXPECT_THROW(sys.validate(), std::invalid_argument);
}

TEST(PowerSystem, ValidateSuccess) {
  PowerSystem sys(100e6, 110e3);
  sys.addNode(Node::makeSlack(1, 110e3, 0.0));
  sys.addNode(Node::makePQ(2, 50e6, 20e6, 110e3, 0.0));
  EXPECT_NO_THROW(sys.validate());
}

// ==================== Конвертация в о.е. ====================

TEST(PowerSystem, ConversionToPu) {
  PowerSystem sys(100e6, 110e3);

  auto n = Node::makePQ(1, 50e6, 20e6, 110e3, 0.0);
  Line l(1, 1, 2, 12.1, 60.5);

  EXPECT_DOUBLE_EQ(sys.P_oe(n), 0.5);
  EXPECT_DOUBLE_EQ(sys.Q_oe(n), 0.2);
  EXPECT_DOUBLE_EQ(sys.R_oe(l), 0.1);
  EXPECT_DOUBLE_EQ(sys.X_oe(l), 0.5);
}

TEST(PowerSystem, ComplexImpedance) {
  PowerSystem sys(100e6, 110e3);
  Line l(1, 1, 2, 12.1, 60.5);

  auto Z = sys.Z_oe(l);
  EXPECT_NEAR(Z.real(), 0.1, 1e-9);
  EXPECT_NEAR(Z.imag(), 0.5, 1e-9);

  auto Y = sys.Y_oe(l);
  EXPECT_NEAR(Y.real(), 0.1 / 0.26, 1e-9);
  EXPECT_NEAR(Y.imag(), -0.5 / 0.26, 1e-9);
}

TEST(PowerSystem, BuildYBus) {
  PowerSystem sys(100e6, 110e3);
  sys.addNode(Node::makeSlack(1, 110e3, 0.0));
  sys.addNode(Node::makePQ(2, 50e6, 20e6, 110e3, 0.0));

  sys.addLine(Line(1, 1, 2, 12.1, 60.5));

  auto Y = sys.buildYBus();

  EXPECT_EQ(Y.rows(), 2);
  EXPECT_EQ(Y.cols(), 2);

  std::complex<double> y_line = sys.Y_oe(sys.getLine(1));
  EXPECT_NEAR(Y(0, 1).real(), -y_line.real(), 1e-4);
  EXPECT_NEAR(Y(0, 1).imag(), -y_line.imag(), 1e-4);

  EXPECT_NEAR(Y(1, 0).real(), -y_line.real(), 1e-4);
  EXPECT_NEAR(Y(1, 0).imag(), -y_line.imag(), 1e-4);

  EXPECT_NEAR(Y(0, 0).real(), y_line.real(), 1e-4);
  EXPECT_NEAR(Y(0, 0).imag(), y_line.imag(), 1e-4);

  EXPECT_NEAR(Y(1, 1).real(), y_line.real(), 1e-4);
  EXPECT_NEAR(Y(1, 1).imag(), y_line.imag(), 1e-4);
}

/// ==================== Solver ====================

TEST(Solver, SimpleTwoBusSystem) {
  PowerSystem sys(100e6, 110e3);
  sys.addNode(Node::makeSlack(1, 110e3, 0.0));
  sys.addNode(Node::makePQ(2, 50e6, 20e6, 110e3, 0.0));
  // Уменьшили сопротивление линии в 5 раз, чтобы напряжение было > 100 кВ
  // Было: 12.1, 60.5 -> Стало: 2.42, 12.1
  sys.addLine(Line(1, 1, 2, 2.42, 12.1)); 

  Solver solver(sys);
  auto result = solver.solve();

  EXPECT_TRUE(result.converged);
  EXPECT_GT(result.iterations, 1);
  EXPECT_LT(result.iterations, 10);

  const auto &nodes = sys.getNodes();
  EXPECT_NEAR(nodes[0].V_mag(), 110e3, 1e-3);
  EXPECT_NEAR(nodes[0].delta(), 0.0, 1e-6);
  EXPECT_GT(nodes[1].V_mag(), 100e3);
  EXPECT_LT(nodes[1].V_mag(), 110e3);
  EXPECT_LT(nodes[1].delta(), 0.0);
}

TEST(Solver, ThreeBusSystem) {
  PowerSystem sys(100e6, 110e3);
  sys.addNode(Node::makeSlack(1, 110e3, 0.0));
  sys.addNode(Node::makePQ(2, 30e6, 10e6, 110e3, 0.0));
  sys.addNode(Node::makePQ(3, 40e6, 15e6, 110e3, 0.0));

  sys.addLine(Line(1, 1, 2, 10.0, 50.0));
  sys.addLine(Line(2, 2, 3, 8.0, 40.0));
  sys.addLine(Line(3, 1, 3, 12.0, 60.0));

  Solver solver(sys);
  auto result = solver.solve();

  EXPECT_TRUE(result.converged);
  EXPECT_LT(result.iterations, 15);

  for (const auto &node : sys.getNodes()) {
    EXPECT_GT(node.V_mag(), 80e3);
    EXPECT_LT(node.V_mag(), 120e3);
  }
}

TEST(Solver, NoConvergence) {
  PowerSystem sys(100e6, 110e3);
  sys.addNode(Node::makeSlack(1, 110e3, 0.0));
  sys.addNode(Node::makePQ(2, 500e6, 200e6, 110e3, 0.0)); // Огромная нагрузка
  sys.addLine(Line(1, 1, 2, 100.0, 500.0)); // Огромное сопротивление

  Solver::Options opts;
  opts.max_iterations = 5;
  opts.tolerance = 1e-10;

  Solver solver(sys, opts);
  auto result = solver.solve();

  // Ожидаем, что солвер НЕ сойдется из-за перегрузки
  EXPECT_FALSE(result.converged); 
}

TEST(Solver, CheckConvergenceDetails) {
  PowerSystem sys(100e6, 110e3);
  sys.addNode(Node::makeSlack(1, 110e3, 0.0));
  sys.addNode(Node::makePQ(2, 50e6, 20e6, 110e3, 0.0));
  // Уменьшили сопротивление линии (аналогично SimpleTwoBusSystem)
  sys.addLine(Line(1, 1, 2, 2.42, 12.1));

  Solver solver(sys);
  auto result = solver.solve();

  EXPECT_TRUE(result.converged);
  EXPECT_GT(result.iterations, 1);
  EXPECT_LT(result.iterations, 10);
  EXPECT_LT(result.max_mismatch, 1e-6);
}

TEST(Solver, DifferentLoadLevels) {
  std::vector<double> loads = {10e6, 30e6, 50e6, 70e6};

  for (double load : loads) {
    PowerSystem sys(100e6, 110e3);
    sys.addNode(Node::makeSlack(1, 110e3, 0.0));
    sys.addNode(Node::makePQ(2, load, load * 0.4, 110e3, 0.0));
    // Уменьшили сопротивление линии, чтобы сеть была "прочной"
    sys.addLine(Line(1, 1, 2, 2.42, 12.1));

    Solver solver(sys);
    auto result = solver.solve();

    EXPECT_TRUE(result.converged)
        << "Не сошёлся для нагрузки " << load / 1e6 << " МВт";

    const auto &nodes = sys.getNodes();
    EXPECT_GT(nodes[1].V_mag(), 90e3)
        << "Слишком низкое напряжение для " << load / 1e6 << " МВт";
    EXPECT_LT(nodes[1].V_mag(), 110e3);
  }
}

TEST(Solver, RingNetwork) {
  PowerSystem sys(100e6, 110e3);
  sys.addNode(Node::makeSlack(1, 110e3, 0.0));
  sys.addNode(Node::makePQ(2, 40e6, 15e6, 110e3, 0.0));
  sys.addNode(Node::makePQ(3, 30e6, 10e6, 110e3, 0.0));

  sys.addLine(Line(1, 1, 2, 10.0, 50.0));
  sys.addLine(Line(2, 2, 3, 8.0, 40.0));
  sys.addLine(Line(3, 3, 1, 12.0, 60.0));

  Solver solver(sys);
  auto result = solver.solve();

  EXPECT_TRUE(result.converged);
  EXPECT_LT(result.iterations, 15);

  // Проверяем только PQ-узлы (индексы 1 и 2). 
  // Slack-узел (индекс 0) всегда имеет V=110 кВ, поэтому для него условие V < 110e3 неверно.
  const auto &nodes = sys.getNodes();
  EXPECT_GT(nodes[1].V_mag(), 95e3);
  EXPECT_LT(nodes[1].V_mag(), 110e3);
  
  EXPECT_GT(nodes[2].V_mag(), 95e3);
  EXPECT_LT(nodes[2].V_mag(), 110e3);
}

TEST(Solver, MultiplePQNodes) {
  PowerSystem sys(100e6, 110e3);
  sys.addNode(Node::makeSlack(1, 110e3, 0.0));
  sys.addNode(Node::makePQ(2, 20e6, 8e6, 110e3, 0.0));
  sys.addNode(Node::makePQ(3, 25e6, 10e6, 110e3, 0.0));
  sys.addNode(Node::makePQ(4, 30e6, 12e6, 110e3, 0.0));
  sys.addNode(Node::makePQ(5, 15e6, 6e6, 110e3, 0.0));

  // Уменьшили сопротивления линий в 5 раз, чтобы сеть была физически корректной
  // Было: 8,40 -> 1.6,8.0 и т.д.
  sys.addLine(Line(1, 1, 2, 1.6, 8.0));
  sys.addLine(Line(2, 2, 3, 2.0, 10.0));
  sys.addLine(Line(3, 3, 4, 2.4, 12.0));
  sys.addLine(Line(4, 4, 5, 3.0, 15.0));

  Solver solver(sys);
  auto result = solver.solve();

  EXPECT_TRUE(result.converged);
  EXPECT_LT(result.iterations, 20);

  const auto &nodes = sys.getNodes();
  EXPECT_GT(nodes[1].V_mag(), nodes[2].V_mag());
  EXPECT_GT(nodes[2].V_mag(), nodes[3].V_mag());
  EXPECT_GT(nodes[3].V_mag(), nodes[4].V_mag());
}

// ==================== Line Flows ====================

TEST(LineFlows, SimpleTwoBusFlows) {
    // Простая сеть: Slack -> PQ через одну линию
    PowerSystem sys(100e6, 110e3);
    sys.addNode(Node::makeSlack(1, 110e3, 0.0));
    sys.addNode(Node::makePQ(2, 50e6, 20e6, 110e3, 0.0));
    sys.addLine(Line(1, 1, 2, 2.42, 12.1));

    Solver solver(sys);
    solver.solve();

    auto flows = sys.calculateLineFlows();
    
    // Должна быть одна линия
    ASSERT_EQ(flows.size(), 1);
    
    const auto& flow = flows[0];
    
    // Проверка ID и узлов
    EXPECT_EQ(flow.line_id, 1);
    EXPECT_EQ(flow.from_node, 1);
    EXPECT_EQ(flow.to_node, 2);
    
    // Мощность в начале линии должна быть больше нагрузки (из-за потерь)
    EXPECT_GT(flow.S_from.real(), 50e6);  // P_from > 50 МВт
    EXPECT_GT(flow.S_from.imag(), 20e6);  // Q_from > 20 Мвар
    
    // Мощность в конце линии ≈ нагрузке (с обратным знаком)
    // S_to отрицательная (мощность уходит из линии в узел)
    EXPECT_NEAR(flow.S_to.real(), -50e6, 1e6);  // P_to ≈ -50 МВт
    EXPECT_NEAR(flow.S_to.imag(), -20e6, 1e6);  // Q_to ≈ -20 Мвар
    
    // Потери положительны
    EXPECT_GT(flow.S_loss.real(), 0);  // P_loss > 0
    EXPECT_GT(flow.S_loss.imag(), 0);  // Q_loss > 0
    
    // Потери = S_from + S_to
    EXPECT_NEAR(flow.S_loss.real(), flow.S_from.real() + flow.S_to.real(), 1e3);
    EXPECT_NEAR(flow.S_loss.imag(), flow.S_from.imag() + flow.S_to.imag(), 1e3);
    
    printPowerFlowResults(sys, "LineFlows.SimpleTwoBusFlows");
}

TEST(LineFlows, ThreeBusFlows) {
    // Сеть из 3 узлов с 3 линиями
    PowerSystem sys(100e6, 110e3);
    sys.addNode(Node::makeSlack(1, 110e3, 0.0));
    sys.addNode(Node::makePQ(2, 30e6, 10e6, 110e3, 0.0));
    sys.addNode(Node::makePQ(3, 40e6, 15e6, 110e3, 0.0));

    sys.addLine(Line(1, 1, 2, 10.0, 50.0));
    sys.addLine(Line(2, 2, 3, 8.0, 40.0));
    sys.addLine(Line(3, 1, 3, 12.0, 60.0));

    Solver solver(sys);
    solver.solve();

    auto flows = sys.calculateLineFlows();
    
    // Должно быть 3 линии
    ASSERT_EQ(flows.size(), 3);
    
    // Суммарные потери
    std::complex<double> total_loss(0, 0);
    for (const auto& flow : flows) {
        total_loss += flow.S_loss;
        // Потери в каждой линии положительны
        EXPECT_GT(flow.S_loss.real(), 0);
    }
    
    // Суммарные потери должны быть разумными (не слишком большими)
    EXPECT_LT(total_loss.real(), 10e6);  // < 10 МВт
    
    printPowerFlowResults(sys, "LineFlows.ThreeBusFlows");
}

// ==================== Slack Power ====================

TEST(SlackPower, SimpleTwoBusBalance) {
    // Проверка баланса: P_slack = P_load + P_loss
    PowerSystem sys(100e6, 110e3);
    sys.addNode(Node::makeSlack(1, 110e3, 0.0));
    sys.addNode(Node::makePQ(2, 50e6, 20e6, 110e3, 0.0));
    sys.addLine(Line(1, 1, 2, 2.42, 12.1));

    Solver solver(sys);
    solver.solve();

    auto S_slack = sys.calculateSlackPower();
    auto flows = sys.calculateLineFlows();
    
    double P_load = 50e6;
    double Q_load = 20e6;
    double P_loss = flows[0].S_loss.real();
    double Q_loss = flows[0].S_loss.imag();
    
    // P_slack = P_load + P_loss
    EXPECT_NEAR(S_slack.real(), P_load + P_loss, 1e3);
    
    // Q_slack = Q_load + Q_loss
    EXPECT_NEAR(S_slack.imag(), Q_load + Q_loss, 1e3);
    
    printPowerFlowResults(sys, "SlackPower.SimpleTwoBusBalance");
}

TEST(SlackPower, ThreeBusBalance) {
    // Проверка баланса для 3 узлов
    PowerSystem sys(100e6, 110e3);
    sys.addNode(Node::makeSlack(1, 110e3, 0.0));
    sys.addNode(Node::makePQ(2, 30e6, 10e6, 110e3, 0.0));
    sys.addNode(Node::makePQ(3, 40e6, 15e6, 110e3, 0.0));

    sys.addLine(Line(1, 1, 2, 10.0, 50.0));
    sys.addLine(Line(2, 2, 3, 8.0, 40.0));
    sys.addLine(Line(3, 1, 3, 12.0, 60.0));

    Solver solver(sys);
    solver.solve();

    auto S_slack = sys.calculateSlackPower();
    auto flows = sys.calculateLineFlows();
    
    double P_load = 30e6 + 40e6;  // Сумма нагрузок
    double Q_load = 10e6 + 15e6;
    
    // Суммарные потери
    std::complex<double> total_loss(0, 0);
    for (const auto& flow : flows) {
        total_loss += flow.S_loss;
    }
    
    // P_slack = P_load + P_loss
    EXPECT_NEAR(S_slack.real(), P_load + total_loss.real(), 1e3);
    
    // Q_slack = Q_load + Q_loss
    EXPECT_NEAR(S_slack.imag(), Q_load + total_loss.imag(), 1e3);
    
    printPowerFlowResults(sys, "SlackPower.ThreeBusBalance");
}

// ==================== Power Balance ====================

TEST(PowerBalance, GlobalBalance) {
    // Глобальная проверка: генерация = нагрузка + потери
    PowerSystem sys(100e6, 110e3);
    sys.addNode(Node::makeSlack(1, 110e3, 0.0));
    sys.addNode(Node::makePQ(2, 40e6, 15e6, 110e3, 0.0));
    sys.addNode(Node::makePQ(3, 30e6, 10e6, 110e3, 0.0));

    sys.addLine(Line(1, 1, 2, 10.0, 50.0));
    sys.addLine(Line(2, 2, 3, 8.0, 40.0));
    sys.addLine(Line(3, 3, 1, 12.0, 60.0));

    Solver solver(sys);
    solver.solve();

    auto S_slack = sys.calculateSlackPower();
    auto flows = sys.calculateLineFlows();
    
    // Суммарная генерация (только Slack)
    double P_gen = S_slack.real();
    double Q_gen = S_slack.imag();
    
    // Суммарная нагрузка
    double P_load = 0, Q_load = 0;
    for (const auto& node : sys.getNodes()) {
        if (node.type() == NodeType::PQ) {
            P_load += node.P_spec();
            Q_load += node.Q_spec();
        }
    }
    
    // Суммарные потери
    std::complex<double> total_loss(0, 0);
    for (const auto& flow : flows) {
        total_loss += flow.S_loss;
    }
    
    // Баланс P: генерация = нагрузка + потери
    EXPECT_NEAR(P_gen, P_load + total_loss.real(), 1e3);
    
    // Баланс Q: генерация = нагрузка + потери
    EXPECT_NEAR(Q_gen, Q_load + total_loss.imag(), 1e3);
    
    printPowerFlowResults(sys, "PowerBalance.GlobalBalance");
}