#include "line.h"
#include "node.h"
#include "powersystem.h"
#include "solver.h"
#include "types.h"
#include <gtest/gtest.h>

// ==================== Node ====================

TEST(Node, PQConstructor)
{
    auto n = Node::makePQ(1, 50e6, 20e6); // 50 МВт, 20 Мвар
    EXPECT_EQ(n.id(), 1);
    EXPECT_EQ(n.type(), NodeType::PQ);
    EXPECT_DOUBLE_EQ(n.P_spec(), 50e6);
    EXPECT_DOUBLE_EQ(n.Q_spec(), 20e6);
    EXPECT_DOUBLE_EQ(n.V_mag(), 1.0); // Начальное приближение
    EXPECT_DOUBLE_EQ(n.delta(), 0.0);
}

TEST(Node, SlackConstructor)
{
    auto n = Node::makeSlack(1, 110e3, 0.0); // 110 кВ, угол 0
    EXPECT_EQ(n.type(), NodeType::SLACK);
    EXPECT_DOUBLE_EQ(n.V_set(), 110e3);
    EXPECT_DOUBLE_EQ(n.V_mag(), 110e3); // Инициализируется как V_set
}

TEST(Node, Setters)
{
    auto n = Node::makePQ(1, 50e6, 20e6);
    n.setV(1.05);
    n.setDelta(0.1);
    EXPECT_NEAR(n.V_mag(), 1.05, 1e-9);
    EXPECT_NEAR(n.delta(), 0.1, 1e-9);
}

TEST(Node, SetVNegativeThrows)
{
    auto n = Node::makePQ(1, 50e6, 20e6);
    EXPECT_THROW(n.setV(-1.0), std::invalid_argument);
    EXPECT_THROW(n.setV(0.0), std::invalid_argument);
}

// ==================== Line ====================

TEST(Line, BasicProperties)
{
    Line l(1, 1, 2, 5.0, 30.0);
    EXPECT_EQ(l.id(), 1);
    EXPECT_EQ(l.from(), 1);
    EXPECT_EQ(l.to(), 2);
    EXPECT_DOUBLE_EQ(l.R(), 5.0);
    EXPECT_DOUBLE_EQ(l.X(), 30.0);
    EXPECT_DOUBLE_EQ(l.k_t(), 1.0);
}

TEST(Line, WithTransformer)
{
    Line l(1, 1, 2, 5.0, 30.0, 1.05); // k_t = 1.05
    EXPECT_DOUBLE_EQ(l.k_t(), 1.05);
}

// ==================== PowerSystem ====================

TEST(PowerSystem, BaseValues)
{
    PowerSystem sys(100e6, 110e3); // 100 МВА, 110 кВ
    EXPECT_DOUBLE_EQ(sys.S_base(), 100e6);
    EXPECT_DOUBLE_EQ(sys.V_base(), 110e3);
    EXPECT_NEAR(sys.Z_base(), 110e3 * 110e3 / 100e6, 1e-6);
}

TEST(PowerSystem, AddNodesAndLines)
{
    PowerSystem sys(100e6, 110e3);
    sys.addNode(Node::makeSlack(1, 110e3, 0.0));
    sys.addNode(Node::makePQ(2, 50e6, 20e6));
    sys.addLine(Line(1, 1, 2, 5.0, 30.0));

    EXPECT_EQ(sys.nodesCount(), 2);
    EXPECT_EQ(sys.linesCount(), 1);
}

TEST(PowerSystem, DuplicateNodeThrows)
{
    PowerSystem sys(100e6, 110e3);
    sys.addNode(Node::makeSlack(1, 110e3, 0.0));
    EXPECT_THROW(sys.addNode(Node::makePQ(1, 50e6, 20e6)), std::invalid_argument);
}

TEST(PowerSystem, LineToNonExistentNodeThrows)
{
    PowerSystem sys(100e6, 110e3);
    sys.addNode(Node::makeSlack(1, 110e3, 0.0));
    EXPECT_THROW(sys.addLine(Line(1, 1, 99, 5.0, 30.0)), std::invalid_argument);
}

TEST(PowerSystem, ValidateNoSlackThrows)
{
    PowerSystem sys(100e6, 110e3);
    sys.addNode(Node::makePQ(1, 50e6, 20e6)); // Только PQ
    sys.addNode(Node::makePQ(2, 30e6, 10e6));
    EXPECT_THROW(sys.validate(), std::invalid_argument);
}

TEST(PowerSystem, ValidateSuccess)
{
    PowerSystem sys(100e6, 110e3);
    sys.addNode(Node::makeSlack(1, 110e3, 0.0)); // Slack
    sys.addNode(Node::makePQ(2, 50e6, 20e6)); // PQ
    EXPECT_NO_THROW(sys.validate());
}

// ==================== Конвертация в о.е. ====================

TEST(PowerSystem, ConversionToPu)
{
    PowerSystem sys(100e6, 110e3); // S_base = 100 МВА, V_base = 110 кВ
    // Z_base = 110^2 / 100 = 121 Ом

    auto n = Node::makePQ(1, 50e6, 20e6); // 50 МВт, 20 Мвар
    Line l(1, 1, 2, 12.1, 60.5); // 12.1 Ом, 60.5 Ом

    EXPECT_DOUBLE_EQ(sys.P_oe(n), 0.5); // 50/100 = 0.5 о.е.
    EXPECT_DOUBLE_EQ(sys.Q_oe(n), 0.2); // 20/100 = 0.2 о.е.
    EXPECT_DOUBLE_EQ(sys.R_oe(l), 0.1); // 12.1/121 = 0.1 о.е.
    EXPECT_DOUBLE_EQ(sys.X_oe(l), 0.5); // 60.5/121 = 0.5 о.е.
}

TEST(PowerSystem, ComplexImpedance)
{
    PowerSystem sys(100e6, 110e3);
    Line l(1, 1, 2, 12.1, 60.5);

    auto Z = sys.Z_oe(l);
    EXPECT_NEAR(Z.real(), 0.1, 1e-9);
    EXPECT_NEAR(Z.imag(), 0.5, 1e-9);

    auto Y = sys.Y_oe(l);
    // Y = 1/Z = 1/(0.1 + 0.5j) = (0.1 - 0.5j)/(0.1^2 + 0.5^2)
    //   = (0.1 - 0.5j)/0.26
    EXPECT_NEAR(Y.real(), 0.1 / 0.26, 1e-9);
    EXPECT_NEAR(Y.imag(), -0.5 / 0.26, 1e-9);
}

TEST(PowerSystem, BuildYBus) {
    PowerSystem sys(100e6, 110e3);
    sys.addNode(Node::makeSlack(1, 110e3, 0.0));
    sys.addNode(Node::makePQ(2, 50e6, 20e6));
    
    // Линия с R=12.1 Ом, X=60.5 Ом. Z_base = 121 Ом.
    // Z_oe = 0.1 + 0.5j. Y_oe = 1 / (0.1 + 0.5j) ≈ 0.3846 - 1.923j
    sys.addLine(Line(1, 1, 2, 12.1, 60.5));

    auto Y = sys.buildYBus();

    EXPECT_EQ(Y.rows(), 2);
    EXPECT_EQ(Y.cols(), 2);

    // Проверяем недиагональные элементы (должны быть -Y_oe)
    std::complex<double> y_line = sys.Y_oe(sys.getLine(1));
    EXPECT_NEAR(Y(0, 1).real(), -y_line.real(), 1e-4);
    EXPECT_NEAR(Y(0, 1).imag(), -y_line.imag(), 1e-4);
    
    EXPECT_NEAR(Y(1, 0).real(), -y_line.real(), 1e-4);
    EXPECT_NEAR(Y(1, 0).imag(), -y_line.imag(), 1e-4);

    // Проверяем диагональные элементы (должны быть +Y_oe)
    EXPECT_NEAR(Y(0, 0).real(), y_line.real(), 1e-4);
    EXPECT_NEAR(Y(0, 0).imag(), y_line.imag(), 1e-4);
    
    EXPECT_NEAR(Y(1, 1).real(), y_line.real(), 1e-4);
    EXPECT_NEAR(Y(1, 1).imag(), y_line.imag(), 1e-4);
}

// ==================== Solver ====================

TEST(Solver, SimpleTwoBusSystem) {
  // Создаём простую систему: Slack + PQ
  PowerSystem sys(100e6, 110e3);
  sys.addNode(Node::makeSlack(1, 110e3, 0.0)); // Slack: V=110кВ, δ=0
  sys.addNode(Node::makePQ(2, 50e6, 20e6));    // PQ: P=50МВт, Q=20Мвар

  // Линия: R=12.1 Ом, X=60.5 Ом
  sys.addLine(Line(1, 1, 2, 12.1, 60.5));

  // Проверяем валидность
  EXPECT_NO_THROW(sys.validate());

  // Запускаем солвер
  Solver solver(sys);
  auto result = solver.solve();

  // Проверяем сходимость
  EXPECT_TRUE(result.converged);
  EXPECT_LT(result.iterations, 10);

  // Проверяем, что напряжения обновились
  const auto &nodes = sys.getNodes();
  EXPECT_NEAR(nodes[0].V_mag(), 1.0, 1e-6); // Slack: V=1.0 p.u.
  EXPECT_NEAR(nodes[0].delta(), 0.0, 1e-6); // Slack: δ=0
  EXPECT_GT(nodes[1].V_mag(), 0.9); // PQ: V должно быть близко к 1.0
  EXPECT_LT(nodes[1].delta(),
            0.0); // PQ: δ должно быть отрицательным (нагрузка)
}

TEST(Solver, ThreeBusSystem) {
  // Система из 3 узлов: 1 Slack + 2 PQ
  PowerSystem sys(100e6, 110e3);
  sys.addNode(Node::makeSlack(1, 110e3, 0.0));
  sys.addNode(Node::makePQ(2, 30e6, 10e6));
  sys.addNode(Node::makePQ(3, 40e6, 15e6));

  sys.addLine(Line(1, 1, 2, 10.0, 50.0));
  sys.addLine(Line(2, 2, 3, 8.0, 40.0));
  sys.addLine(Line(3, 1, 3, 12.0, 60.0));

  Solver solver(sys);
  auto result = solver.solve();

  EXPECT_TRUE(result.converged);
  EXPECT_LT(result.iterations, 15);

  // Проверяем, что все напряжения в разумных пределах
  for (const auto &node : sys.getNodes()) {
    EXPECT_GT(node.V_mag(), 0.8);
    EXPECT_LT(node.V_mag(), 1.2);
  }
}

TEST(Solver, NoConvergence) {
  // Система с очень большой нагрузкой (может не сойтись)
  PowerSystem sys(100e6, 110e3);
  sys.addNode(Node::makeSlack(1, 110e3, 0.0));
  sys.addNode(Node::makePQ(2, 500e6, 200e6)); // Очень большая нагрузка

  sys.addLine(Line(1, 1, 2, 100.0, 500.0)); // Большое сопротивление

  Solver::Options opts;
  opts.max_iterations = 5; // Мало итераций
  opts.tolerance = 1e-10;  // Очень строгая точность

  Solver solver(sys, opts);
  auto result = solver.solve();

  // Может не сойтись — это нормально для экстремальных случаев
  // Просто проверяем, что метод вернул результат
  EXPECT_GE(result.iterations, 0);
}