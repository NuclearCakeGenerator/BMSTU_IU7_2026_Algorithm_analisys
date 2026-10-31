#include "search.hpp"

#include <algorithm>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {

bool read_int(const std::string &prompt, int &value) {
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

bool read_array(std::vector<int> &array) {
  for (std::size_t index = 0; index < array.size(); ++index) {
    if (!read_int("A[" + std::to_string(index) + "] = ", array[index])) {
      return false;
    }
  }
  return true;
}

void print_array(const std::vector<int> &array, const std::string &name) {
  std::cout << name << " = {";
  for (std::size_t index = 0; index < array.size(); ++index) {
    if (index != 0) {
      std::cout << ", ";
    }
    std::cout << array[index];
  }
  std::cout << "}\n";
}

void print_menu() {
  std::cout << "\nМеню:\n"
            << "1. Линейный поиск\n"
            << "2. Бинарный поиск, нерекурсивный, вариант 1\n"
            << "3. Бинарный поиск, нерекурсивный, вариант 2\n"
            << "4. Бинарный поиск, рекурсивный\n"
            << "0. Завершить программу\n";
}

int run_search(int algorithm, const std::vector<int> &original,
               const std::vector<int> &sorted, int value) {
  switch (algorithm) {
  case 1:
    return linear_search(original, value);
  case 2:
    return binary_search_iterative_v1(sorted, value);
  case 3:
    return binary_search_iterative_v2(sorted, value);
  case 4:
    return binary_search_recursive(sorted, value, 0,
                                   static_cast<int>(sorted.size()) - 1);
  default:
    return -1;
  }
}

} // namespace

int main() {
  int size = 0;
  while (true) {
    if (!read_int("Введите размер массива N: ", size)) {
      std::cout << "\nВвод завершен.\n";
      return 0;
    }
    if (size > 0) {
      break;
    }
    std::cout << "Ошибка: размер массива должен быть положительным.\n";
  }

  std::vector<int> original(static_cast<std::size_t>(size));
  if (!read_array(original)) {
    std::cout << "\nВвод завершен.\n";
    return 0;
  }

  std::vector<int> sorted = original;
  std::sort(sorted.begin(), sorted.end());

  print_array(original, "Исходный массив");
  print_array(sorted, "Отсортированная копия");

  while (true) {
    print_menu();

    int algorithm = 0;
    if (!read_int("Выберите алгоритм: ", algorithm)) {
      std::cout << "\nВвод завершен.\n";
      return 0;
    }
    if (algorithm == 0) {
      std::cout << "Программа завершена.\n";
      return 0;
    }
    if (algorithm < 1 || algorithm > 4) {
      std::cout << "Ошибка: выберите пункт меню от 0 до 4.\n";
      continue;
    }

    int value = 0;
    if (!read_int("Введите искомое значение x: ", value)) {
      std::cout << "\nВвод завершен.\n";
      return 0;
    }

    const int result = run_search(algorithm, original, sorted, value);
    if (result == -1) {
      std::cout << "Элемент не найден.\n";
    } else {
      std::cout << "Элемент найден по индексу " << result << ".\n";
      if (algorithm != 1) {
        std::cout << "Индекс указан в отсортированной копии.\n";
      }
    }
  }
}
