#!/bin/bash
set -euo pipefail

g++ -std=c++17 -Wall -Wextra -Werror grocery_list.cpp tests/grocery_list_test.cpp -o grocery_list_tests
./grocery_list_tests
