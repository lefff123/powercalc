#include <gtest/gtest.h>
#include "matrix.h"
#include <cmath>

// Вспомогательная функция для сравнения матриц с точностью
void expectMatrixNear(const Matrix<double>& a, const Matrix<double>& b, double eps = 1e-9) {
    ASSERT_EQ(a.rows(), b.rows());
    ASSERT_EQ(a.cols(), b.cols());
    for (size_t i = 0; i < a.rows(); ++i) {
        for (size_t j = 0; j < a.cols(); ++j) {
            EXPECT_NEAR(a(i, j), b(i, j), eps) 
                << "Mismatch at (" << i << ", " << j << ")";
        }
    }
}

// ==================== Конструкторы ====================

TEST(MatrixConstructors, DefaultConstructor) {
    Matrix<double> m;
    EXPECT_EQ(m.rows(), 0);
    EXPECT_EQ(m.cols(), 0);
}

TEST(MatrixConstructors, SizeConstructor) {
    Matrix<double> m(3, 4);
    EXPECT_EQ(m.rows(), 3);
    EXPECT_EQ(m.cols(), 4);
    // Все элементы должны быть нулями
    for (size_t i = 0; i < 3; ++i)
        for (size_t j = 0; j < 4; ++j)
            EXPECT_DOUBLE_EQ(m(i, j), 0.0);
}

TEST(MatrixConstructors, SizeWithInitValue) {
    Matrix<double> m(2, 2, 5.0);
    for (size_t i = 0; i < 2; ++i)
        for (size_t j = 0; j < 2; ++j)
            EXPECT_DOUBLE_EQ(m(i, j), 5.0);
}

TEST(MatrixConstructors, From2DVector) {
    std::vector<std::vector<double>> data = {{1, 2}, {3, 4}};
    Matrix<double> m(data);
    EXPECT_EQ(m.rows(), 2);
    EXPECT_EQ(m.cols(), 2);
    EXPECT_DOUBLE_EQ(m(0, 0), 1.0);
    EXPECT_DOUBLE_EQ(m(1, 1), 4.0);
}

TEST(MatrixConstructors, JaggedVectorThrows) {
    std::vector<std::vector<double>> data = {{1, 2}, {3}};
    EXPECT_THROW(Matrix<double>(data), std::invalid_argument);
}

// ==================== Доступ к элементам ====================

TEST(MatrixAccess, ReadWrite) {
    Matrix<double> m(2, 2);
    m(0, 0) = 1.5;
    m(1, 1) = 2.5;
    EXPECT_DOUBLE_EQ(m(0, 0), 1.5);
    EXPECT_DOUBLE_EQ(m(1, 1), 2.5);
}

TEST(MatrixAccess, ConstAccess) {
    Matrix<double> m(2, 2, 3.0);
    const Matrix<double>& cm = m;
    EXPECT_DOUBLE_EQ(cm(0, 0), 3.0);
    // cm(0, 0) = 5.0;  // не должно компилироваться
}

// ==================== Сложение и вычитание ====================

TEST(MatrixArithmetic, Addition) {
    Matrix<double> a(2, 2, 1.0);
    Matrix<double> b(2, 2, 2.0);
    Matrix<double> c = a + b;
    for (size_t i = 0; i < 2; ++i)
        for (size_t j = 0; j < 2; ++j)
            EXPECT_DOUBLE_EQ(c(i, j), 3.0);
}

TEST(MatrixArithmetic, Subtraction) {
    Matrix<double> a(2, 2, 5.0);
    Matrix<double> b(2, 2, 2.0);
    Matrix<double> c = a - b;
    for (size_t i = 0; i < 2; ++i)
        for (size_t j = 0; j < 2; ++j)
            EXPECT_DOUBLE_EQ(c(i, j), 3.0);
}

TEST(MatrixArithmetic, AdditionDimensionMismatch) {
    Matrix<double> a(2, 2);
    Matrix<double> b(3, 3);
    EXPECT_THROW(a + b, std::invalid_argument);
}

// ==================== Умножение ====================

TEST(MatrixArithmetic, ScalarMultiplication) {
    Matrix<double> a(2, 2, 2.0);
    Matrix<double> b = a * 3.0;
    
    // Проверяем, что исходная матрица НЕ изменилась
    EXPECT_DOUBLE_EQ(a(0, 0), 2.0);
    
    // Проверяем результат
    for (size_t i = 0; i < 2; ++i)
        for (size_t j = 0; j < 2; ++j)
            EXPECT_DOUBLE_EQ(b(i, j), 6.0);
}

TEST(MatrixArithmetic, MatrixMultiplication) {
    // [1 2] * [5 6] = [1*5+2*7  1*6+2*8] = [19 22]
    // [3 4]   [7 8]   [3*5+4*7  3*6+4*8]   [43 50]
    Matrix<double> a({{1, 2}, {3, 4}});
    Matrix<double> b({{5, 6}, {7, 8}});
    Matrix<double> c = a * b;
    
    EXPECT_DOUBLE_EQ(c(0, 0), 19.0);
    EXPECT_DOUBLE_EQ(c(0, 1), 22.0);
    EXPECT_DOUBLE_EQ(c(1, 0), 43.0);
    EXPECT_DOUBLE_EQ(c(1, 1), 50.0);
}

TEST(MatrixArithmetic, MatrixMultiplicationDimensionMismatch) {
    Matrix<double> a(2, 3);
    Matrix<double> b(2, 2);
    EXPECT_THROW(a * b, std::invalid_argument);
}

// ==================== Транспонирование ====================

TEST(MatrixOperations, Transpose) {
    Matrix<double> a({{1, 2, 3}, {4, 5, 6}});  // 2x3
    Matrix<double> t = a.transpose();           // 3x2
    
    EXPECT_EQ(t.rows(), 3);
    EXPECT_EQ(t.cols(), 2);
    EXPECT_DOUBLE_EQ(t(0, 0), 1.0);
    EXPECT_DOUBLE_EQ(t(0, 1), 4.0);
    EXPECT_DOUBLE_EQ(t(2, 0), 3.0);
    EXPECT_DOUBLE_EQ(t(2, 1), 6.0);
}

TEST(MatrixOperations, DoubleTranspose) {
    Matrix<double> a({{1, 2}, {3, 4}});
    Matrix<double> tt = a.transpose().transpose();
    expectMatrixNear(a, tt);
}

// ==================== Обратная матрица ====================

TEST(MatrixOperations, Inverse2x2) {
    // A = [2 1; 1 3]
    // A^-1 = [0.6 -0.2; -0.2 0.4]
    Matrix<double> a({{2, 1}, {1, 3}});
    Matrix<double> a_inv = a.inverse();
    
    EXPECT_NEAR(a_inv(0, 0), 0.6, 1e-9);
    EXPECT_NEAR(a_inv(0, 1), -0.2, 1e-9);
    EXPECT_NEAR(a_inv(1, 0), -0.2, 1e-9);
    EXPECT_NEAR(a_inv(1, 1), 0.4, 1e-9);
}

TEST(MatrixOperations, InverseTimesOriginalIsIdentity) {
    Matrix<double> a({{4, 7}, {2, 6}});
    Matrix<double> a_inv = a.inverse();
    Matrix<double> identity = a * a_inv;
    
    // Должна получиться единичная матрица
    Matrix<double> expected(2, 2);
    expected(0, 0) = 1.0;
    expected(1, 1) = 1.0;
    
    expectMatrixNear(identity, expected, 1e-9);
}

TEST(MatrixOperations, Inverse3x3) {
    Matrix<double> a({
        {1, 2, 3},
        {0, 1, 4},
        {5, 6, 0}
    });
    Matrix<double> a_inv = a.inverse();
    Matrix<double> identity = a * a_inv;
    
    Matrix<double> expected(3, 3);
    expected(0, 0) = 1.0;
    expected(1, 1) = 1.0;
    expected(2, 2) = 1.0;
    
    expectMatrixNear(identity, expected, 1e-9);
}

TEST(MatrixOperations, InverseNonSquareThrows) {
    Matrix<double> a(2, 3);
    EXPECT_THROW(a.inverse(), std::invalid_argument);
}

TEST(MatrixOperations, InverseSingularThrows) {
    // Вырожденная матрица (строки линейно зависимы)
    Matrix<double> a({{1, 2}, {2, 4}});
    EXPECT_THROW(a.inverse(), std::runtime_error);
}

// ==================== Интеграционные тесты ====================

TEST(MatrixIntegration, SolveLinearSystem) {
    // Система: 2x + y = 5, x + 3y = 7
    // Решение: x = 1.6, y = 1.8
    // Через обратную матрицу: [x,y]^T = A^-1 * b
    Matrix<double> A({{2, 1}, {1, 3}});
    Matrix<double> A_inv = A.inverse();
    
    Matrix<double> b(std::vector<std::vector<double>>{{5}, {7}});
    Matrix<double> x = A_inv * b;
    
    EXPECT_NEAR(x(0, 0), 1.6, 1e-9);
    EXPECT_NEAR(x(1, 0), 1.8, 1e-9);
}

TEST(MatrixIntegration, ChainOperations) {
    Matrix<double> a({{1, 2}, {3, 4}});
    Matrix<double> b({{5, 6}, {7, 8}});
    
    // (A + B) * 2 - A
    Matrix<double> result = (a + b) * 2.0 - a;
    
    // Проверка: (A+B)*2 - A = 2A + 2B - A = A + 2B
    Matrix<double> expected = a + b * 2.0;
    expectMatrixNear(result, expected);
}