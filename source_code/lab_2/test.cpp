#include "matrix.hpp"

#include <cassert>
#include <cstdio>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

Matrix make_matrix(std::initializer_list<std::initializer_list<int>> rows) {
  const std::size_t row_count = rows.size();
  if (row_count == 0) {
    throw std::invalid_argument("тестовая матрица не может быть пустой");
  }
  const std::size_t column_count = rows.begin()->size();
  Matrix result(row_count, column_count);
  std::size_t i = 0;
  for (const auto &row : rows) {
    assert(row.size() == column_count);
    std::size_t j = 0;
    for (int value : row) {
      result.at(i, j++) = value;
    }
    ++i;
  }
  return result;
}

void assert_all_algorithms(const Matrix &left, const Matrix &right,
                           const Matrix &expected) {
  assert(multiply_standard(left, right) == expected);
  assert(multiply_winograd(left, right) == expected);
  assert(multiply_winograd_optimized(left, right) == expected);
}

} // namespace

int main() {
  // Проверка умножения прямоугольных матриц.
  const Matrix left = make_matrix({{1, 2, 3}, {4, 5, 6}});
  const Matrix right = make_matrix({{7, 8}, {9, 10}, {11, 12}});
  const Matrix expected = make_matrix({{58, 64}, {139, 154}});

  assert_all_algorithms(left, right, expected);

  // Проверка общего случая с нечётным внутренним размером.
  const Matrix odd_left = make_matrix({{2, -1, 4}, {0, 3, 5}});
  const Matrix odd_right = make_matrix({{1, 2}, {-2, 0}, {3, 4}});
  const Matrix odd_expected = make_matrix({{16, 20}, {9, 20}});
  assert_all_algorithms(odd_left, odd_right, odd_expected);

  // Проверка свойств единичных матриц.
  const Matrix rectangular = make_matrix({{2, -1, 4}, {0, 3, 5}});
  const Matrix identity_2 = make_matrix({{1, 0}, {0, 1}});
  const Matrix identity_3 = make_matrix(
      {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}});
  assert_all_algorithms(identity_2, rectangular, rectangular);
  assert_all_algorithms(rectangular, identity_3, rectangular);

  // Проверка умножения с нулевой матрицей.
  const Matrix zero_2x3 = make_matrix({{0, 0, 0}, {0, 0, 0}});
  const Matrix zero_3x2 = make_matrix({{0, 0}, {0, 0}, {0, 0}});
  const Matrix zero_2x2 = make_matrix({{0, 0}, {0, 0}});
  assert_all_algorithms(zero_2x3, right, zero_2x2);
  assert_all_algorithms(left, zero_3x2, zero_2x2);

  // Проверка обратных матриц.
  const Matrix matrix = make_matrix({{1, 1}, {0, 1}});
  const Matrix inverse = make_matrix({{1, -1}, {0, 1}});
  const Matrix identity = make_matrix({{1, 0}, {0, 1}});
  assert_all_algorithms(matrix, inverse, identity);
  assert_all_algorithms(inverse, matrix, identity);

  // Проверка матриц размера 1x1.
  const Matrix scalar = make_matrix({{-1}});
  const Matrix scalar_inverse = make_matrix({{-1}});
  const Matrix scalar_identity = make_matrix({{1}});
  assert_all_algorithms(scalar, scalar_inverse, scalar_identity);
  assert_all_algorithms(scalar_inverse, scalar, scalar_identity);

  // Проверка ошибки при несовместимых размерах.
  bool dimension_error = false;
  try {
    (void)multiply_standard(Matrix(2, 3), Matrix(2, 2));
  } catch (const std::invalid_argument &) {
    dimension_error = true;
  }
  assert(dimension_error);

  // Проверка сохранения матрицы при ошибке загрузки.
  Matrix destination = make_matrix({{42}});
  std::string error;
  assert(!load_matrix_from_file("does-not-exist.txt", destination, error));
  assert(destination == make_matrix({{42}}));

  {
    std::ofstream invalid_file("matrix-invalid-test.txt");
    invalid_file << "1 2\n3 wrong\n";
  }
  assert(!load_matrix_from_file("matrix-invalid-test.txt", destination, error));
  assert(destination == make_matrix({{42}}));
  std::remove("matrix-invalid-test.txt");

  std::cout << "Все модульные тесты пройдены.\n";
}
