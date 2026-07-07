#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

template <typename T = double>
class Matrix
{
public:
    // ==================== Конструкторы ====================
    Matrix() : rows_(0), cols_(0) { }

    Matrix(size_t rows, size_t cols)
        : data_(rows, std::vector<T>(cols, T{ })), rows_(rows), cols_(cols) { }

    Matrix(size_t rows, size_t cols, T init_val)
        : data_(rows, std::vector<T>(cols, init_val)), rows_(rows), cols_(cols) { }

    Matrix(const std::vector<std::vector<T>> &data)
        : data_(data), rows_(data.size()), cols_(data.empty() ? 0 : data[0].size())
    {
        for (const auto &row : data) {
            if (row.size() != cols_) {
                throw std::invalid_argument("Matrix: all rows must have the same length");
            }
        }
    }

    Matrix(const Matrix &) = default;
    Matrix &operator=(const Matrix &) = default;
    ~Matrix() = default;

    // ==================== Размеры и свойства ====================
    size_t rows() const { return rows_; }
    size_t cols() const { return cols_; }
    bool isSquare() const { return rows_ == cols_; }
    bool empty() const { return rows_ == 0 || cols_ == 0; }

    // ==================== Доступ к элементам ====================
    T &operator()(size_t i, size_t j) { return data_[i][j]; }
    const T &operator()(size_t i, size_t j) const { return data_[i][j]; }

    // ==================== Математические операции ====================

    Matrix<T> operator+(const Matrix<T> &other) const
    {
        checkSameDimensions(other, "operator+");
        Matrix<T> result(rows_, cols_);
        for (size_t i = 0; i < rows_; ++i)
            for (size_t j = 0; j < cols_; ++j)
                result.data_[i][j] = data_[i][j] + other.data_[i][j];
        return result;
    }

    Matrix<T> operator-(const Matrix<T> &other) const
    {
        checkSameDimensions(other, "operator-");
        Matrix<T> result(rows_, cols_);
        for (size_t i = 0; i < rows_; ++i)
            for (size_t j = 0; j < cols_; ++j)
                result.data_[i][j] = data_[i][j] - other.data_[i][j];
        return result;
    }

    Matrix<T> operator*(const Matrix<T> &other) const
    {
        if (cols_ != other.rows_) {
            throw std::invalid_argument("operator*: dimension mismatch");
        }
        Matrix<T> result(rows_, other.cols_);
        for (size_t i = 0; i < rows_; ++i)
            for (size_t k = 0; k < cols_; ++k) {
                T tmp = data_[i][k];
                for (size_t j = 0; j < other.cols_; ++j)
                    result.data_[i][j] += tmp * other.data_[k][j];
            }
        return result;
    }

    Matrix<T> operator*(T scalar) const
    {
        Matrix<T> result(rows_, cols_);
        for (size_t i = 0; i < rows_; ++i)
            for (size_t j = 0; j < cols_; ++j)
                result.data_[i][j] = data_[i][j] * scalar;
        return result;
    }

    Matrix<T> transpose() const
    {
        Matrix<T> result(cols_, rows_);
        for (size_t i = 0; i < rows_; ++i)
            for (size_t j = 0; j < cols_; ++j)
                result.data_[j][i] = data_[i][j];
        return result;
    }

    Matrix<T> inverse() const
    {
        if (!isSquare())
            throw std::invalid_argument("inverse: matrix must be square");

        const size_t n = rows_;
        Matrix<T> aug(n, 2 * n);

        // Формируем [A | I]
        for (size_t i = 0; i < n; ++i) {
            for (size_t j = 0; j < n; ++j)
                aug.data_[i][j] = data_[i][j];
            aug.data_[i][n + i] = T{ 1 };
        }

        // Прямой и обратный ход
        for (size_t col = 0; col < n; ++col) {
            // Поиск главного элемента
            size_t maxRow = col;
            auto maxVal = std::abs(aug.data_[col][col]);
            for (size_t row = col + 1; row < n; ++row) {
                auto curVal = std::abs(aug.data_[row][col]);
                if (curVal > maxVal) {
                    maxVal = curVal;
                    maxRow = row;
                }
            }

            if (maxVal < 1e-12)
                throw std::runtime_error("inverse: matrix is singular");

            // Перестановка строк (обмен целыми векторами — быстро)
            if (maxRow != col)
                std::swap(aug.data_[col], aug.data_[maxRow]);

            // Нормировка строки
            T pivot = aug.data_[col][col];
            for (auto &val : aug.data_[col])
                val /= pivot;

            // Обнуление столбца во всех остальных строках
            for (size_t row = 0; row < n; ++row) {
                if (row == col)
                    continue;
                T factor = aug.data_[row][col];
                for (size_t j = 0; j < 2 * n; ++j)
                    aug.data_[row][j] -= factor * aug.data_[col][j];
            }
        }

        // Извлекаем обратную матрицу
        Matrix<T> result(n, n);
        for (size_t i = 0; i < n; ++i)
            for (size_t j = 0; j < n; ++j)
                result.data_[i][j] = aug.data_[i][n + j];
        return result;
    }

    // ==================== Утилиты ====================
    void print(std::ostream &os = std::cout, int precision = 4) const
    {
        os << std::fixed << std::setprecision(precision);
        for (size_t i = 0; i < rows_; ++i) {
            os << "[ ";
            for (size_t j = 0; j < cols_; ++j)
                os << data_[i][j] << " ";
            os << "]\n";
        }
    }

private:
    std::vector<std::vector<T>> data_;
    size_t rows_;
    size_t cols_;

    void checkSameDimensions(const Matrix<T> &other, const char *op) const
    {
        if (rows_ != other.rows_ || cols_ != other.cols_)
            throw std::invalid_argument(std::string(op) + ": dimension mismatch");
    }
};