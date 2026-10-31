#pragma once

#include <vector>

int linear_search(const std::vector<int>& array, int value);
int binary_search_iterative_v1(const std::vector<int>& array, int value);
int binary_search_iterative_v2(const std::vector<int>& array, int value);
int binary_search_recursive(const std::vector<int>& array, int value,
                            int left, int right);
