#pragma once

#include "matrix.hpp"

#include <cstddef>
#include <random>
#include <string>

using Algorithm = Matrix (*)(const Matrix &, const Matrix &);

struct AlgorithmInfo {
  const char *name;
  Algorithm function;
  const char *csv_name;
};

constexpr std::size_t algorithm_count = 3;

const AlgorithmInfo &algorithm_info(std::size_t index);
Matrix random_matrix(std::size_t size, std::mt19937 &generator);
double measure_ms(Algorithm function, const Matrix &left, const Matrix &right,
                  std::size_t repetitions);
bool run_benchmark(std::size_t first, std::size_t last, std::size_t step,
                   std::size_t repetitions, const std::string &prefix);
