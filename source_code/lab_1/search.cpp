#include "search.hpp"

int linear_search(const std::vector<int>& array, int value) {
    for (int index = 0; index < static_cast<int>(array.size()); ++index) {
        if (array[index] == value) {
            return index;
        }
    }

    return -1;
}

int binary_search_iterative_v1(const std::vector<int>& array, int value) {
    int left = 0;
    int right = static_cast<int>(array.size()) - 1;

    while (left <= right) {
        const int middle = left + (right - left) / 2;

        if (array[middle] == value) {
            return middle;
        }

        if (array[middle] < value) {
            left = middle + 1;
        } else {
            right = middle - 1;
        }
    }

    return -1;
}

int binary_search_iterative_v2(const std::vector<int>& array, int value) {
    if (array.empty()) {
        return -1;
    }

    int left = 0;
    int right = static_cast<int>(array.size()) - 1;

    while (left < right) {
        const int middle = left + (right - left) / 2;

        if (array[middle] < value) {
            left = middle + 1;
        } else {
            right = middle;
        }
    }

    return array[left] == value ? left : -1;
}

int binary_search_recursive(const std::vector<int>& array, int value,
                            int left, int right) {
    if (left > right || left < 0 ||
        right >= static_cast<int>(array.size())) {
        return -1;
    }

    const int middle = left + (right - left) / 2;

    if (array[middle] == value) {
        return middle;
    }

    if (array[middle] < value) {
        return binary_search_recursive(array, value, middle + 1, right);
    }

    return binary_search_recursive(array, value, left, middle - 1);
}
