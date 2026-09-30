# Feature: Persistent Categorized Grocery Lists MVP

## Objective

Create a dependable, personal grocery-list application for a single local user. The user must be able to manage named lists and groceries through a C++ terminal menu, resume work after restarting the program, and shop from quantities grouped into predictable categories. Success means the full flow works without duplicate-item confusion, accidental list deletion, or silent loss of saved data.

## Feature Description

Build a simple terminal-based C++ application that lets a local user create, view, rename, and delete grocery lists. A user can add, update, and remove grocery items inside each list. Grocery quantities for the same normalized item are combined, and items are displayed under five standard shopping categories—Produce, Grains, Frozen, Canned/Condiments, and Meat/Dairy—with a user-selectable Other category for ambiguous items.

List data must survive application restarts in a local file. This is a single-user, local MVP; accounts, sharing, prices, and cloud synchronization are out of scope.

## Tech Stack

- C++17 and the standard library only; no external runtime or package dependency.
- `g++` for compilation.
- Bash for the repository test runner and scripted terminal E2E test.
- A versioned, local plain-text data file using `std::quoted` for delimiter-safe strings.

## Commands

Run these commands from the repository root after implementation:

```bash
# Build the interactive application.
g++ -std=c++17 -Wall -Wextra -Werror main.cpp grocery_list.cpp -o grocery-list

# Run the terminal application.
./grocery-list

# Run focused unit/business-rule tests.
./test_runner.sh

# Run the isolated terminal persistence test.
bash tests/test_grocery_list_e2e.sh
```

## Project Structure

```text
main.cpp                       terminal-menu startup and input/output wiring
grocery_list.hpp/.cpp          domain model, validation, categorization, and persistence
tests/grocery_list_test.cpp    unit and persistence tests
tests/test_grocery_list_e2e.sh scripted end-to-end terminal test
data/grocery_lists.db          runtime data file; ignored by Git
specs/                         feature specifications and implementation plan
```

## Code Style

- Keep menu I/O in `main.cpp`; keep parsing, storage, and business rules in `grocery_list.cpp`.
- Use `PascalCase` for types, `camelCase` for functions/variables, and `constexpr`/enums for fixed categories.
- Return explicit success/error results from domain operations rather than printing or exiting inside them.
- Preserve order intentionally with ordered containers; do not introduce sorting as a side effect.

```cpp
// Business logic reports an outcome; the terminal layer decides how to show it.
OperationResult GroceryListStore::renameList(
    const std::string& existingName,
    const std::string& newName);
```

## Boundaries

- Always: validate user input, preserve data on failed saves or failed loads, run unit and E2E tests before handoff, and keep the specification current when decisions change.
- Ask first: add a dependency, change the local-file format after it ships, add categories beyond the six defined here, or add account/cloud behavior.
- Never: silently reset a corrupt data file, overwrite user data following a failed save, commit `data/grocery_lists.db`, or remove failing tests to make validation pass.

## User Story

As a grocery shopper,
I want to maintain named grocery lists whose duplicate items are combined and grouped by aisle-like category,
so that I can quickly see what and how much to buy, even after closing the app.

## Problem Statement

An unstructured grocery list produces duplicate entries (for example, `1 apple` followed by `2 apples`) and forces shoppers to scan the whole list in the store. The current repository has only an empty C++ entry point and no way to store or organize grocery data.

## Solution Statement

Provide an interactive numbered menu backed by a small domain model and a local, dependency-free persistence format. Parse quantity-prefixed entries, normalize simple singular/plural variations to a canonical item key, then merge quantities. Use a curated item-to-category map for common groceries and prompt the user to select one of the five standard categories or Other for uncatalogued items. Persist category choices globally so they are remembered across lists and restarts; correcting a category updates every matching item in every saved list. Save every successful mutation atomically so the next run loads the same lists.

## Assumptions

- The requested “simple interface” is an interactive terminal UI because this repository is a C++ console template with no web or desktop UI stack.
- Data is local to one computer and one user; no authentication or synchronization is needed.
- List names are compared after trimming and case-folding, so `Weekend`, ` weekend `, and `WEEKEND` cannot create separate lists. The originally entered trimmed spelling is displayed.
- Lists are displayed in creation order. Items within a category are displayed in the order they were first added to that category.
- Item input follows `positive-integer item name`, such as `2 apples`. Entries with no leading quantity are rejected with an explanation.
- Item equivalence is case-insensitive and whitespace-insensitive; a conservative trailing-`s` singularization handles the stated `apple`/`apples` example. Irregular plurals are not automatically merged in the MVP.
- Category choices are the five requested categories plus Other. Known items are categorized automatically; an unknown item requires a category choice only the first time it is added. The chosen category is saved in a global canonical-item-to-category registry and reused across all lists and later application runs. Other is displayed only in lists that contain an item explicitly assigned to it.
- Editing an item’s quantity sets its final positive quantity. Renaming an item into an existing normalized item merges their quantities. A category correction is available from Edit Grocery, requires an explicit global-update warning, and updates the saved global mapping plus every matching item in every list, including items that previously used a built-in default.
- An unreadable or corrupt data file is preserved and reported; the application must not silently start with an empty store or overwrite that file.

## Relevant Files

- `main.cpp` — current empty application entry point; will be replaced with only startup/menu wiring after application code is split into focused files.
- `test_runner.sh` — current compile-and-run script; will be updated to build and run automated tests without executing the interactive application.
- `README.md` — will document build, run, persistence-file location, and input format.
- `tests/README.md` — existing test directory documentation; preserve its test-only purpose.

### New Files

- `grocery_list.hpp` — list, item, category, and store interfaces.
- `grocery_list.cpp` — item parsing, normalization, categorization, CRUD, and persistence implementation.
- `tests/grocery_list_test.cpp` — focused automated tests for business rules and file round trips.
- `tests/test_grocery_list_e2e.sh` — separate terminal end-to-end test that drives the compiled executable with scripted input and checks displayed, persisted behavior.
- `data/grocery_lists.db` — runtime-created local data file; ignored by Git and not committed.

## Implementation Plan

### Phase 1: Foundation

Define a small `GroceryListStore` model containing named lists, normalized grocery items, and a global custom-category registry. Establish a stable, delimiter-safe plain-text format using C++ `std::quoted` for names and a version header. Save a candidate copy of the state to a sibling temporary file, then replace the saved file only after a complete successful write; promote the candidate in-memory state only if persistence succeeds. This avoids adding a JSON dependency while preserving spaces and punctuation in names.

### Phase 2: Core Implementation

Implement menu-driven list CRUD and item CRUD. The domain service parses input, validates positive quantities, normalizes names, assigns categories, and merges existing matching items instead of adding duplicates. The read view emits category headings in the requested fixed order, omitting empty headings.

### Phase 3: Integration

Wire the store into `main.cpp`: load once at startup, handle recovery from a missing data file, persist only after successful mutations, and show actionable validation errors without losing the current session. Add repeatable unit and terminal E2E coverage, then document execution and storage behavior.

## Step by Step Tasks

### 1. Establish the application and persistence contract

- Add category constants/enums in the fixed display order: Produce, Grains, Frozen, Canned/Condiments, Meat/Dairy, then Other. Render Other only when a list contains an item assigned to it.
- Define `GroceryItem` (`canonicalName`, display name, quantity, category) and `GroceryList` (`name`, items) types.
- Define store operations for create, read/list, rename, delete, add-or-merge item, edit item quantity/name/category, remove item, load, and save. Preserve list creation order and per-category item insertion order. Mutating operations must operate on a candidate state so a failed save leaves both the file and the visible in-memory state unchanged.
- Specify a versioned, line-oriented `std::quoted` data format so names containing spaces are preserved.
- Add the runtime data path to `.gitignore`.

### 2. Add business-rule tests before the menu

- Create `tests/grocery_list_test.cpp` with a minimal assertion harness compatible with the repository’s direct `g++` workflow.
- Cover list-name validation and create/rename/delete behavior, including duplicate and missing list handling.
- Cover quantity parsing, case/whitespace normalization, `1 apple` + `2 apples` becoming one `apple` item with quantity `3`, category assignment, creation/insertion display order, global category reuse across two lists and a save/load round trip, global category correction propagating to every existing list, item updates/removals, and deletion confirmation at the terminal boundary.
- Replace the current wildcard compile-and-run behavior in `test_runner.sh` with an explicit C++17 test command (`grocery_list.cpp` plus `tests/grocery_list_test.cpp`) so it neither starts the interactive app nor creates duplicate `main` symbols. Return a nonzero status on any failed assertion.

### 3. Implement store behavior and safe local persistence

- Implement validation and CRUD methods without menu I/O, returning structured success/error results the UI can display. Treat trimmed, case-folded list names as unique and keep a separate canonical item key for merge comparisons.
- Add a curated initial category catalog for common groceries (for example, apple/banana → Produce, rice/bread → Grains, peas/ice cream → Frozen, beans/ketchup → Canned/Condiments, milk/eggs/chicken → Meat/Dairy).
- When an item is not in the catalog, have the caller select a valid category, including Other, then store that canonical item/category pair in a globally persisted registry. Subsequent additions of that item in any list use the remembered category without prompting. A category correction changes that registry and propagates the new category to every matching item in all stored lists.
- Load an absent data file as an empty store; reject corrupt files with a clear error, preserve the existing file, and exit without presenting an empty replacement store.
- Write changes to a temporary sibling file and rename it only after a complete successful write. Keep the previous data file intact on any write, flush, close, or rename failure; report the failure and do not adopt the candidate state in memory.

### 4. Build the terminal interface

- Replace the placeholder `main.cpp` with a main menu for create, view, rename, delete, and exit. Display lists in creation order. Require an explicit yes/no confirmation before permanently deleting a list.
- Add a list-detail menu for add grocery, edit grocery, remove grocery, view categorized list, and return to the main menu. Category correction in Edit Grocery must show that the change applies everywhere before accepting a yes/no response.
- Render each list grouped in the required category order, with quantities and readable item names. Preserve first-added order within each category. Render Other last and only when the list contains an Other item.
- Re-prompt on malformed menu choices, blank names, duplicate list names, zero/negative quantities, unknown item selections, and missing lists/items.
- Save after each successful create, rename, delete, add/merge, edit, or remove operation; show a confirmation or error message.

### 5. Create a terminal E2E test

- Create `tests/test_grocery_list_e2e.sh` early enough to guide the menu’s stable prompts and output; invoke it explicitly with `bash` so the test does not depend on executable-file metadata.
- Run the application in a temporary working directory so test data cannot affect a user’s saved lists.
- Script creation of named lists in a nonalphabetical order, addition of `1 apple` and `2 apples`, viewing the list, exiting, relaunching, and viewing again. Also add an unknown item to one list, assign it a category, then add it to a second list and verify no category prompt appears; correct its category with the global-update confirmation.
- Assert that lists display in creation order; output contains one Produce entry with `3 apple`; the named list is still available after relaunch; the corrected item moves category in every affected list; and Other appears only in a list where the user selected Other.

### 6. Document and run validation

- Update `README.md` with compiler requirements, `./test_runner.sh`, application launch command, data-file path, and the supported item-input format.
- Run all validation commands below from a clean working directory and fix failures before handoff.

## Testing Strategy

### Unit Tests

- List CRUD rejects blank and duplicate names, preserves creation order after rename, and handles attempts to modify nonexistent lists.
- Quantity parsing accepts positive integers and rejects missing, zero, negative, nonnumeric, and overflow quantities.
- List-name normalization prevents case-only or surrounding-whitespace duplicates while preserving a readable display name.
- Normalization merges casing, repeated whitespace, and ordinary trailing-`s` variants; different item names remain separate.
- Categories render in the fixed order Produce, Grains, Frozen, Canned/Condiments, Meat/Dairy, then Other. Other is omitted when unused, each category contains only its own items, and members retain their first-added order.
- Item CRUD covers add-or-merge, edit, remove, and missing-item failures.
- Serialization preserves multiple lists, names with spaces, categories, quantities, and normalized merge keys across restart. A deliberately failed save leaves the previously loaded store and data file usable.

### Edge Cases

- App starts before any data file exists.
- User quits immediately without changes.
- A list/item name contains leading/trailing spaces or internal spaces.
- `apple` and `apples` merge, while `apple` and `pineapple` do not.
- An unknown grocery has a valid user-selected category and subsequent matching entries in another list and after restart are not re-prompted.
- Correcting a category after its global-update confirmation changes every matching existing item across all lists, including entries originally categorized by the built-in catalog.
- Other is never shown unless the user assigned an item in that list to Other.
- List deletion requires a confirmation response; a negative response preserves the list.
- Editing an item into a name already present in the list follows the documented merge behavior rather than creating two duplicate entries.
- A malformed persistence file produces an error instead of silently discarding data.
- An empty category does not render as a misleading empty section.

## Success Criteria

- A user can create, view, rename, and delete named lists through the terminal interface.
- Lists remain in their creation order, and groceries retain first-added order within their displayed category.
- Lists persist in a local data file and are available after closing and reopening the application.
- A user can add, edit, and remove grocery entries from an existing list.
- Adding `1 apple` and then `2 apples` displays one apple entry with quantity `3`.
- Every displayed grocery belongs to exactly one of Produce, Grains, Frozen, Canned/Condiments, Meat/Dairy, or Other. Standard categories follow that sequence; Other is last and appears only when used.
- A user-selected category for an unknown grocery is remembered globally across lists and application restarts.
- A confirmed category correction updates every matching grocery in every saved list; declining the warning changes nothing.
- Deleting a list requires confirmation and a declined confirmation leaves it unchanged.
- Invalid input does not crash the app or overwrite persisted data.
- Unit tests and the E2E persistence scenario pass.

## Open Questions

None. The user explicitly confirmed the interface, persistence, ordering, category, matching, correction, and recovery behavior on 2026-09-30.

## Validation Commands

Execute every command from the repository root after implementation:

```bash
./test_runner.sh
g++ -std=c++17 -Wall -Wextra -Werror main.cpp grocery_list.cpp -o grocery-list
bash tests/test_grocery_list_e2e.sh
```

The E2E script must compile/use the application in an isolated temporary directory and finish with exit code `0`.

## Notes

- No external libraries are required for the MVP.
- The category catalog should be defined in one location so a later MVP iteration can add a category editor or richer food taxonomy without changing menu flow.
- Automatic plural normalization is intentionally conservative. Broad English stemming would incorrectly merge unrelated groceries; users can use edit/delete for exceptional names in this MVP.
- The application should accept an explicit data-file path internally (with `data/grocery_lists.db` as the production default) so unit and E2E tests can isolate their fixture files without touching user data.
- The confirmed intent is a terminal-menu C++ application; no web or desktop interface is planned for this MVP.

## Plan Review Resolutions

- Persistence is transactional at the application level: a mutation is not shown as successful until its candidate state is saved. This closes the original plan’s risk of showing data that disappears on restart after a disk failure.
- The test runner now has an explicit compilation contract, preventing the existing `*.cpp` command from either running the interactive menu during tests or failing once a test `main` is added.
- List-name uniqueness, unknown-category behavior, and item-edit collision behavior are now explicit, so separate implementation agents will not make incompatible choices.
