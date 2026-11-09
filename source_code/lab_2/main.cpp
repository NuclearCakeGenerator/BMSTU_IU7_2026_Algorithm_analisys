#include "benchmark_support.hpp"

#include <iostream>
#include <limits>
#include <string>

namespace {

bool read_integer(const std::string &prompt, int &value) {
  while (true) {
    std::cout << prompt;
    if (std::cin >> value) {
      return true;
    }
    if (std::cin.eof()) {
      return false;
    }
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cout << "Ошибка: введите целое число.\n";
  }
}

bool read_positive_integer(const std::string &prompt, std::size_t &value) {
  int input = 0;
  if (!read_integer(prompt, input)) {
    return false;
  }
  if (input <= 0) {
    std::cout << "Ошибка: значение должно быть положительным.\n";
    return false;
  }
  value = static_cast<std::size_t>(input);
  return true;
}

void print_menu() {
  std::cout << "\nМеню:\n"
            << "1. Выполнить умножение выбранным алгоритмом\n"
            << "2. Выполнить умножение всеми алгоритмами\n"
            << "3. Вывести загруженные матрицы\n"
            << "4. Выбрать алгоритм\n"
            << "5. Загрузить матрицу A из файла\n"
            << "6. Загрузить матрицу B из файла\n"
            << "7. Выполнить замеры процессорного времени\n"
            << "0. Завершить работу\n";
}

bool load_matrix(const std::string &name, Matrix &matrix) {
  std::string path;
  std::cout << "Путь к файлу матрицы " << name << ": ";
  if (!(std::cin >> path)) {
    return false;
  }
  std::string error;
  if (!load_matrix_from_file(path, matrix, error)) {
    std::cout << "Ошибка загрузки: " << error
              << ". Прежняя матрица сохранена.\n";
    return false;
  }
  std::cout << "Матрица " << name << " загружена.\n";
  return true;
}

bool matrices_ready(const Matrix &left, const Matrix &right) {
  if (left.rows() == 0 || right.rows() == 0) {
    std::cout << "Сначала загрузите обе матрицы.\n";
    return false;
  }
  if (left.columns() != right.rows()) {
    std::cout << "Ошибка: матрицы нельзя перемножить: число столбцов A не "
                 "равно числу строк B.\n";
    return false;
  }
  return true;
}

} // namespace

int main() {
  Matrix left;
  Matrix right;
  std::size_t selected = 0;

  while (true) {
    print_menu();
    int command = 0;
    if (!read_integer("Выберите пункт: ", command)) {
      return 0;
    }
    try {
      switch (command) {
      case 1:
        if (matrices_ready(left, right)) {
          const AlgorithmInfo &algorithm = algorithm_info(selected);
          print_matrix(algorithm.function(left, right),
                       "Результат (" + std::string(algorithm.name) + ")");
        }
        break;
      case 2:
        if (matrices_ready(left, right)) {
          for (std::size_t i = 0; i < algorithm_count; ++i) {
            const AlgorithmInfo &algorithm = algorithm_info(i);
            print_matrix(algorithm.function(left, right),
                         "Результат (" + std::string(algorithm.name) + ")");
          }
        }
        break;
      case 3:
        if (left.rows() != 0) {
          print_matrix(left, "Матрица A");
        } else {
          std::cout << "Матрица A не загружена.\n";
        }
        if (right.rows() != 0) {
          print_matrix(right, "Матрица B");
        } else {
          std::cout << "Матрица B не загружена.\n";
        }
        break;
      case 4: {
        int algorithm = 0;
        if (!read_integer("Алгоритм (1-3): ", algorithm)) {
          return 0;
        }
        if (algorithm < 1 || algorithm > static_cast<int>(algorithm_count)) {
          std::cout << "Ошибка: выберите алгоритм от 1 до 3.\n";
        } else {
          selected = static_cast<std::size_t>(algorithm - 1);
        }
        break;
      }
      case 5:
        if (!load_matrix("A", left) && std::cin.eof()) {
          return 0;
        }
        break;
      case 6:
        if (!load_matrix("B", right) && std::cin.eof()) {
          return 0;
        }
        break;
      case 7: {
        std::size_t first = 0;
        std::size_t last = 0;
        std::size_t step = 0;
        std::size_t repetitions = 0;
        if (!read_positive_integer("Начальный размер N: ", first) ||
            !read_positive_integer("Конечный размер N: ", last) ||
            !read_positive_integer("Шаг: ", step) ||
            !read_positive_integer("Число повторений: ", repetitions)) {
          break;
        }
        if (last == std::numeric_limits<std::size_t>::max()) {
          std::cout << "Ошибка: конечный размер слишком велик.\n";
          break;
        }
        run_benchmark(first, last, step, repetitions, "even_");
        run_benchmark(first + 1, last + 1, step, repetitions, "odd_");
        break;
      }
      case 0:
        return 0;
      default:
        std::cout << "Ошибка: неизвестный пункт меню.\n";
      }
    } catch (const std::exception &error) {
      std::cout << "Ошибка: " << error.what() << '\n';
    }
  }
}
