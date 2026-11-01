#!/usr/bin/env bash

set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
build_dir="${script_dir}/build"

mkdir -p "${build_dir}"

cxx="${CXX:-g++}"
common_flags=(-std=c++20 -Wall -Wextra -Wpedantic -O0)

"${cxx}" "${common_flags[@]}" \
    "${script_dir}/search.cpp" \
    "${script_dir}/main.cpp" \
    -o "${build_dir}/search_app"

"${cxx}" "${common_flags[@]}" \
    "${script_dir}/search.cpp" \
    "${script_dir}/test.cpp" \
    -o "${build_dir}/search_tests"

"${build_dir}/search_tests"
exec "${build_dir}/search_app"
