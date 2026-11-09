#include "matrix.hpp"

#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace {

void check_multiplication_dimensions(const Matrix &left, const Matrix &right) {
  if (left.columns() != right.rows()) {
    throw std::invalid_argument(
        "число столбцов первой матрицы должно быть равно числу строк второй");
  }
}

Matrix multiply_winograd_impl(const Matrix &left, const Matrix &right,
                              bool optimized) {
  check_multiplication_dimensions(left, right);

  const std::size_t rows = left.rows();
  const std::size_t inner = left.columns();
  const std::size_t columns = right.columns();
  const std::size_t pair_count = inner / 2;
  Matrix result(rows, columns);
  std::vector<int> row_factors(rows, 0);
  std::vector<int> column_factors(columns, 0);

  for (std::size_t i = 0; i < rows; ++i) {
    if (optimized) {
      for (std::size_t k = 0; k < inner; k += 2) {
        if (k + 1 >= inner) {
          break;
        }
        row_factors[i] += left.at(i, k) * left.at(i, k + 1);
      }
    } else {
      for (std::size_t k = 0; k < pair_count; ++k) {
        row_factors[i] += left.at(i, 2 * k) * left.at(i, 2 * k + 1);
      }
    }
  }

  for (std::size_t j = 0; j < columns; ++j) {
    if (optimized) {
      for (std::size_t k = 0; k < inner; k += 2) {
        if (k + 1 >= inner) {
          break;
        }
        column_factors[j] += right.at(k, j) * right.at(k + 1, j);
      }
    } else {
      for (std::size_t k = 0; k < pair_count; ++k) {
        column_factors[j] += right.at(2 * k, j) * right.at(2 * k + 1, j);
      }
    }
  }

  for (std::size_t i = 0; i < rows; ++i) {
    for (std::size_t j = 0; j < columns; ++j) {
      int value = -row_factors[i] - column_factors[j];
      if (optimized) {
        for (std::size_t k = 0; k < inner; k += 2) {
          if (k + 1 >= inner) {
            break;
          }
          value += (left.at(i, k) + right.at(k + 1, j)) *
                   (left.at(i, k + 1) + right.at(k, j));
        }
      } else {
        for (std::size_t k = 0; k < pair_count; ++k) {
          value += (left.at(i, 2 * k) + right.at(2 * k + 1, j)) *
                   (left.at(i, 2 * k + 1) + right.at(2 * k, j));
        }
      }
      if (inner % 2 != 0) {
        value += left.at(i, inner - 1) * right.at(inner - 1, j);
      }
      result.at(i, j) = value;
    }
  }
  return result;
}

} // namespace

Matrix::Matrix(std::size_t rows, std::size_t columns)
    : rows_(rows), columns_(columns), values_(rows * columns, 0) {}

int &Matrix::at(std::size_t row, std::size_t column) {
  if (row >= rows_ || column >= columns_) {
    throw std::out_of_range("индекс матрицы находится за границами");
  }
  return values_[row * columns_ + column];
}

const int &Matrix::at(std::size_t row, std::size_t column) const {
  if (row >= rows_ || column >= columns_) {
    throw std::out_of_range("индекс матрицы находится за границами");
  }
  return values_[row * columns_ + column];
}

bool Matrix::operator==(const Matrix &other) const noexcept {
  return rows_ == other.rows_ && columns_ == other.columns_ &&
         values_ == other.values_;
}

Matrix multiply_standard(const Matrix &left, const Matrix &right) {
  check_multiplication_dimensions(left, right);
  Matrix result(left.rows(), right.columns());
  for (std::size_t i = 0; i < left.rows(); ++i) {
    for (std::size_t j = 0; j < right.columns(); ++j) {
      for (std::size_t k = 0; k < left.columns(); ++k) {
        result.at(i, j) += left.at(i, k) * right.at(k, j);
      }
    }
  }
  return result;
}

Matrix multiply_winograd(const Matrix &left, const Matrix &right) {
  return multiply_winograd_impl(left, right, false);
}

Matrix multiply_winograd_optimized(const Matrix &left, const Matrix &right) {
  return multiply_winograd_impl(left, right, true);
}

bool load_matrix_from_file(const std::string &path, Matrix &destination,
                           std::string &error) {
  std::ifstream input(path);
  if (!input) {
    error = "не удалось открыть файл '" + path + "'";
    return false;
  }

  std::vector<std::vector<int>> rows;
  std::string line;
  std::size_t line_number = 0;
  std::size_t column_count = 0;
  while (std::getline(input, line)) {
    ++line_number;
    std::istringstream line_stream(line);
    std::vector<int> row;
    long long value = 0;
    while (line_stream >> value) {
      if (value < std::numeric_limits<int>::min() ||
          value > std::numeric_limits<int>::max()) {
        error = "строка " + std::to_string(line_number) +
                " содержит число вне диапазона int";
        return false;
      }
      row.push_back(static_cast<int>(value));
    }
    if (line_stream.fail() && !line_stream.eof()) {
      error = "строка " + std::to_string(line_number) +
              " содержит нецелое значение";
      return false;
    }
    if (row.empty()) {
      error = "строка " + std::to_string(line_number) + " пуста";
      return false;
    }
    if (rows.empty()) {
      column_count = row.size();
    } else if (row.size() != column_count) {
      error = "строки матрицы имеют разное число элементов";
      return false;
    }
    rows.push_back(std::move(row));
  }

  if (rows.empty()) {
    error = "файл не содержит матрицу";
    return false;
  }

  Matrix loaded(rows.size(), column_count);
  for (std::size_t i = 0; i < rows.size(); ++i) {
    for (std::size_t j = 0; j < column_count; ++j) {
      loaded.at(i, j) = rows[i][j];
    }
  }
  destination = std::move(loaded);
  return true;
}

void save_matrix_to_file(const Matrix &matrix, const std::string &path) {
  std::ofstream output(path);
  if (!output) {
    throw std::runtime_error("не удалось открыть файл '" + path + "'");
  }
  for (std::size_t i = 0; i < matrix.rows(); ++i) {
    for (std::size_t j = 0; j < matrix.columns(); ++j) {
      if (j != 0) {
        output << ' ';
      }
      output << matrix.at(i, j);
    }
    output << '\n';
  }
}

void print_matrix(const Matrix &matrix, const std::string &name) {
  std::cout << name << " (" << matrix.rows() << " x " << matrix.columns()
            << "):\n";
  for (std::size_t i = 0; i < matrix.rows(); ++i) {
    for (std::size_t j = 0; j < matrix.columns(); ++j) {
      std::cout << std::setw(8) << matrix.at(i, j);
    }
    std::cout << '\n';
  }
}
