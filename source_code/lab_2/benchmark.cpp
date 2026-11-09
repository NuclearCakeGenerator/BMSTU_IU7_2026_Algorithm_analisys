#include "benchmark_support.hpp"

#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {

std::size_t argument(int argc, char **argv, int index, std::size_t fallback) {
  if (argc <= index) {
    return fallback;
  }
  return std::stoull(argv[index]);
}

} // namespace

int main(int argc, char **argv) {
  try {
    const std::size_t first = argument(argc, argv, 1, 10);
    const std::size_t last = argument(argc, argv, 2, 100);
    const std::size_t step = argument(argc, argv, 3, 10);
    const std::size_t repetitions = argument(argc, argv, 4, 10);
    if (last == std::numeric_limits<std::size_t>::max()) {
      throw std::invalid_argument("конечный размер слишком велик");
    }
    if (!run_benchmark(first, last, step, repetitions, "even_") ||
        !run_benchmark(first + 1, last + 1, step, repetitions, "odd_")) {
      return 1;
    }
  } catch (const std::exception &error) {
    std::cerr << "Ошибка параметров замеров: " << error.what() << ".\n";
    return 1;
  }
}
