#include "line.h"
#include "node.h"
#include "powersystem.h"
#include "types.h"
#include <gtest/gtest.h>

// ==================== Node ====================

TEST(Node, PQConstructor) {
  auto n = Node::makePQ(1, 50e6, 20e6); // 50 МВт, 20 Мвар
  EXPECT_EQ(n.id(), 1);
  EXPECT_EQ(n.type(), NodeType::PQ);
  EXPECT_DOUBLE_EQ(n.P_spec(), 50e6);
  EXPECT_DOUBLE_EQ(n.Q_spec(), 20e6);
  EXPECT_DOUBLE_EQ(n.V_mag(), 1.0); // Начальное приближение
  EXPECT_DOUBLE_EQ(n.delta(), 0.0);
}

TEST(Node, SlackConstructor) {
  auto n = Node::makeSlack(1, 110e3, 0.0); // 110 кВ, угол 0
  EXPECT_EQ(n.type(), NodeType::SLACK);
  EXPECT_DOUBLE_EQ(n.V_set(), 110e3);
  EXPECT_DOUBLE_EQ(n.V_mag(), 110e3); // Инициализируется как V_set
}

TEST(Node, Setters) {
  auto n = Node::makePQ(1, 50e6, 20e6);
  n.setV(1.05);
  n.setDelta(0.1);
  EXPECT_DOUBLE_EQ(n.V_mag(), 1.05);
  EXPECT_DOUBLE_EQ(n.delta(), 0.1);
}

TEST(Node, SetVNegativeThrows) {
  auto n = Node::makePQ(1, 50e6, 20e6);
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
  Line l(1, 1, 2, 5.0, 30.0, 1.05); // k_t = 1.05
  EXPECT_DOUBLE_EQ(l.k_t(), 1.05);
}

// ==================== PowerSystem ====================

TEST(PowerSystem, BaseValues) {
  PowerSystem sys(100e6, 110e3); // 100 МВА, 110 кВ
  EXPECT_DOUBLE_EQ(sys.S_base(), 100e6);
  EXPECT_DOUBLE_EQ(sys.V_base(), 110e3);
  EXPECT_NEAR(sys.Z_base(), 110e3 * 110e3 / 100e6, 1e-6);
}

TEST(PowerSystem, AddNodesAndLines) {
  PowerSystem sys(100e6, 110e3);
  sys.addNode(Node::makeSlack(1, 110e3, 0.0));
  sys.addNode(Node::makePQ(2, 50e6, 20e6));
  sys.addLine(Line(1, 1, 2, 5.0, 30.0));

  EXPECT_EQ(sys.nodesCount(), 2);
  EXPECT_EQ(sys.linesCount(), 1);
}

TEST(PowerSystem, DuplicateNodeThrows) {
  PowerSystem sys(100e6, 110e3);
  sys.addNode(Node::makeSlack(1, 110e3, 0.0));
  EXPECT_THROW(sys.addNode(Node::makePQ(1, 50e6, 20e6)), std::invalid_argument);
}

TEST(PowerSystem, LineToNonExistentNodeThrows) {
  PowerSystem sys(100e6, 110e3);
  sys.addNode(Node::makeSlack(1, 110e3, 0.0));
  EXPECT_THROW(sys.addLine(Line(1, 1, 99, 5.0, 30.0)), std::invalid_argument);
}

TEST(PowerSystem, ValidateNoSlackThrows) {
  PowerSystem sys(100e6, 110e3);
  sys.addNode(Node::makePQ(1, 50e6, 20e6)); // Только PQ
  sys.addNode(Node::makePQ(2, 30e6, 10e6));
  EXPECT_THROW(sys.validate(), std::invalid_argument);
}

TEST(PowerSystem, ValidateSuccess) {
  PowerSystem sys(100e6, 110e3);
  sys.addNode(Node::makeSlack(1, 110e3, 0.0)); // Slack
  sys.addNode(Node::makePQ(2, 50e6, 20e6));    // PQ
  EXPECT_NO_THROW(sys.validate());
}

// ==================== Конвертация в о.е. ====================

TEST(PowerSystem, ConversionToPu) {
  PowerSystem sys(100e6, 110e3); // S_base = 100 МВА, V_base = 110 кВ
  // Z_base = 110^2 / 100 = 121 Ом

  auto n = Node::makePQ(1, 50e6, 20e6); // 50 МВт, 20 Мвар
  Line l(1, 1, 2, 12.1, 60.5);          // 12.1 Ом, 60.5 Ом

  EXPECT_DOUBLE_EQ(sys.P_oe(n), 0.5); // 50/100 = 0.5 о.е.
  EXPECT_DOUBLE_EQ(sys.Q_oe(n), 0.2); // 20/100 = 0.2 о.е.
  EXPECT_DOUBLE_EQ(sys.R_oe(l), 0.1); // 12.1/121 = 0.1 о.е.
  EXPECT_DOUBLE_EQ(sys.X_oe(l), 0.5); // 60.5/121 = 0.5 о.е.
}

TEST(PowerSystem, ComplexImpedance) {
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