# Persistent Grocery Lists

A single-user C++17 terminal application for named grocery lists. It saves data locally, combines simple singular/plural duplicates such as `apple` and `apples`, and groups groceries into shopping categories.

## Run locally

```bash
make run
```

Use `make` on its own to build the `grocery-list` executable without running it.

The application writes its local data to `data/grocery_lists.db`. Do not edit that file while the program is running. If the file is unreadable, the app preserves it and exits with an error rather than replacing it.

Use grocery entries in the form `positive-quantity item-name`, for example `2 apples`. Unknown groceries prompt for one of Produce, Grains, Frozen, Canned/Condiments, Meat/Dairy, or Other. The selected category is remembered globally for later lists.

## Test

```bash
./test_runner.sh
bash tests/test_grocery_list_e2e.sh
```

The end-to-end test compiles the app and uses a temporary directory, so it never modifies your local grocery-list data.

## Container setup

## Getting Started

This repository is compatible with [cpp-container](https://github.com/ChicoState/cpp-container). If not already built on your machine, clone and build it.

Run the container:

```bash
docker run -v "$(pwd)":/usr/src -it cpp-container
```

Run the application interactively in a shell:

```bash
docker run -v "$(pwd)":/usr/src -it cpp-container sh
```

## Structure

* `.agents` - AI agent configurations and skills (in `/skills` subdirectory) for this project
* `.` - The root directory contains the C++ code for the application as well as necessary scripts
* `specs` - Specification documentation
* `tests` - Test code
