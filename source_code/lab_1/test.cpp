#include "search.hpp"

#include <cassert>
#include <iostream>
#include <vector>

namespace {

void check_searches(const std::vector<int> &array, int value,
                    int expected_value) {
  assert(linear_search(array, value) == expected_value);
  assert(binary_search_iterative_v1(array, value) == expected_value);
  assert(binary_search_iterative_v2(array, value) == expected_value);

  const int right = static_cast<int>(array.size()) - 1;
  assert(binary_search_recursive(array, value, 0, right) == expected_value);
}

void test_empty_array() {
  const std::vector<int> array;
  check_searches(array, 0, -1);
  assert(binary_search_recursive(array, 0, 0, -1) == -1);
}

void test_fixed_array() {
  const std::vector<int> array{-8, -1, 0, 5, 7, 15, 25, 90};

  check_searches(array, -8, 0);
  check_searches(array, 5, 3);
  check_searches(array, 90, 7);
  check_searches(array, -10, -1);
  check_searches(array, 100, -1);
  check_searches(array, 6, -1);
}

void test_single_element_array() {
  const std::vector<int> array{16};

  check_searches(array, 16, 0);
  check_searches(array, 0, -1);
}

} // namespace

int main() {
  test_empty_array();
  test_fixed_array();
  test_single_element_array();

  std::cout << "All search tests passed.\n";
  return 0;
}
