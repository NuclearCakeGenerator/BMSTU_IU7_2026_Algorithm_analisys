#include "benchmark_support.hpp"

#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace {

#define BENCHMARK_DIR "./data/"

const AlgorithmInfo algorithms[algorithm_count] = {
    {"стандартный", multiply_standard, "standard.csv"},
    {"Виноград", multiply_winograd, "winograd.csv"},
    {"оптимизированный Виноград", multiply_winograd_optimized,
     "winograd_optimized.csv"}};

} // namespace

const AlgorithmInfo &algorithm_info(std::size_t index) {
  if (index >= algorithm_count) {
    throw std::out_of_range("неизвестный алгоритм");
  }
  return algorithms[index];
}

Matrix random_matrix(std::size_t size, std::mt19937 &generator) {
  Matrix matrix(size, size);
  std::uniform_int_distribution<int> distribution(-1000, 1000);
  for (std::size_t i = 0; i < size; ++i) {
    for (std::size_t j = 0; j < size; ++j) {
      matrix.at(i, j) = distribution(generator);
    }
  }
  return matrix;
}

double measure_ms(Algorithm function, const Matrix &left, const Matrix &right,
                  std::size_t repetitions) {
  if (repetitions == 0) {
    throw std::invalid_argument("число повторений должно быть положительным");
  }
  const std::clock_t start = std::clock();
  for (std::size_t i = 0; i < repetitions; ++i) {
    volatile Matrix result = function(left, right);
    (void)result;
  }
  const std::clock_t elapsed = std::clock() - start;
  return 1000.0 * static_cast<double>(elapsed) / CLOCKS_PER_SEC /
         static_cast<double>(repetitions);
}

bool run_benchmark(std::size_t first, std::size_t last, std::size_t step,
                   std::size_t repetitions, const std::string &prefix) {
  if (first == 0 || first > last || step == 0 || repetitions == 0) {
    std::cout << "Ошибка: некорректные параметры замеров.\n";
    return false;
  }

  std::ofstream outputs[algorithm_count];
  for (std::size_t i = 0; i < algorithm_count; ++i) {
    const std::filesystem::path output_path =
        std::filesystem::path(BENCHMARK_DIR) /
        (prefix + algorithm_info(i).csv_name);
    const std::filesystem::path output_directory = output_path.parent_path();
    std::error_code directory_error;
    if (!output_directory.empty() &&
        !std::filesystem::create_directories(output_directory,
                                             directory_error) &&
        directory_error) {
      std::cout << "Ошибка: не удалось создать каталог для результатов \""
                << output_directory.string()
                << "\": " << directory_error.message() << ".\n";
      return false;
    }

    outputs[i].open(output_path);
    if (!outputs[i]) {
      std::cout << "Ошибка: не удалось создать файл результатов \""
                << output_path.string() << "\".\n";
      return false;
    }
    outputs[i] << "size,time_ms\n";
  }

  std::mt19937 generator(42);
  for (std::size_t size = first; size <= last;) {
    const Matrix left = random_matrix(size, generator);
    const Matrix right = random_matrix(size, generator);
    std::cout << "N = " << size << '\n';
    for (std::size_t i = 0; i < algorithm_count; ++i) {
      const AlgorithmInfo &algorithm = algorithm_info(i);
      const double time =
          measure_ms(algorithm.function, left, right, repetitions);
      outputs[i] << size << ',' << time << '\n';
      std::cout << "  " << algorithm.name << ": " << time << " мс\n";
    }
    if (last - size < step) {
      break;
    }
    size += step;
  }
  std::cout << "Результаты сохранены в файлы с префиксом \"" << prefix
            << "\".\n";
  return true;
}
