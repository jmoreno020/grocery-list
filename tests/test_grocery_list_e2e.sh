#!/bin/bash
set -euo pipefail

project_root="$(cd "$(dirname "$0")/.." && pwd)"
test_dir="$(mktemp -d)"
trap 'rm -rf "$test_dir"' EXIT

g++ -std=c++17 -Wall -Wextra -Werror "$project_root/main.cpp" \
  "$project_root/grocery_list.cpp" -o "$test_dir/grocery-list"

first_run="$(cd "$test_dir" && printf '1\nWeekend\n2\n1\n2\n1 apple\n2\n2 apples\n1\n0\n0\n' | ./grocery-list)"
printf '%s\n' "$first_run" | grep -F 'Produce:'
printf '%s\n' "$first_run" | grep -F '3 apple'

second_run="$(cd "$test_dir" && printf '2\n1\n1\n0\n4\n1\nno\n0\n' | ./grocery-list)"
printf '%s\n' "$second_run" | grep -F 'Weekend'
printf '%s\n' "$second_run" | grep -F '3 apple'
printf '%s\n' "$second_run" | grep -F 'Deletion cancelled.'
