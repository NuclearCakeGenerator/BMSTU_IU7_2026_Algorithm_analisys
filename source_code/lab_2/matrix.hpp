#pragma once

#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

class Matrix {
public:
  Matrix() = default;
  Matrix(std::size_t rows, std::size_t columns);

  std::size_t rows() const noexcept { return rows_; }
  std::size_t columns() const noexcept { return columns_; }

  int &at(std::size_t row, std::size_t column);
  const int &at(std::size_t row, std::size_t column) const;

  bool operator==(const Matrix &other) const noexcept;

private:
  std::size_t rows_ = 0;
  std::size_t columns_ = 0;
  std::vector<int> values_;
};

Matrix multiply_standard(const Matrix &left, const Matrix &right);
Matrix multiply_winograd(const Matrix &left, const Matrix &right);
Matrix multiply_winograd_optimized(const Matrix &left, const Matrix &right);

bool load_matrix_from_file(const std::string &path, Matrix &destination,
                           std::string &error);
void save_matrix_to_file(const Matrix &matrix, const std::string &path);
void print_matrix(const Matrix &matrix, const std::string &name);
