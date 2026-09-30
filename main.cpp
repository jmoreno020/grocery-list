#include "grocery_list.hpp"

#include <array>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>

namespace {

const std::array<Category, 6> kCategories = {
    Category::Produce, Category::Grains, Category::Frozen,
    Category::CannedCondiments, Category::MeatDairy, Category::Other};

std::string categoryName(Category category) {
  switch (category) {
    case Category::Produce:
      return "Produce";
    case Category::Grains:
      return "Grains";
    case Category::Frozen:
      return "Frozen";
    case Category::CannedCondiments:
      return "Canned/Condiments";
    case Category::MeatDairy:
      return "Meat/Dairy";
    case Category::Other:
      return "Other";
  }
  return "Unknown";
}

std::optional<std::string> readLine(const std::string& prompt) {
  std::cout << prompt;
  std::string value;
  if (!std::getline(std::cin, value)) {
    return std::nullopt;
  }
  return value;
}

std::optional<int> readNumber(const std::string& prompt) {
  const std::optional<std::string> value = readLine(prompt);
  if (!value.has_value()) {
    return std::nullopt;
  }
  std::istringstream input(*value);
  int number = 0;
  char extra = '\0';
  if (!(input >> number) || (input >> extra)) {
    std::cout << "Enter a whole-number menu choice.\n";
    return std::nullopt;
  }
  return number;
}

std::optional<std::int64_t> readQuantity(const std::string& prompt) {
  const std::optional<std::string> value = readLine(prompt);
  if (!value.has_value()) {
    return std::nullopt;
  }
  std::istringstream input(*value);
  std::int64_t quantity = 0;
  char extra = '\0';
  if (!(input >> quantity) || (input >> extra) || quantity <= 0) {
    std::cout << "Enter a positive whole number.\n";
    return std::nullopt;
  }
  return quantity;
}

void printResult(const OperationResult& result) {
  std::cout << (result.ok ? "Success: " : "Error: ") << result.message << '\n';
}

void printCategorizedList(const GroceryList& list) {
  std::cout << "\n" << list.name << '\n';
  if (list.items.empty()) {
    std::cout << "  (No groceries yet.)\n";
    return;
  }
  for (Category category : kCategories) {
    bool hasItems = false;
    for (const GroceryItem& item : list.items) {
      if (item.category == category) {
        hasItems = true;
        break;
      }
    }
    if (!hasItems) {
      continue;
    }
    std::cout << categoryName(category) << ":\n";
    for (const GroceryItem& item : list.items) {
      if (item.category == category) {
        std::cout << "  " << item.quantity << " " << item.displayName << '\n';
      }
    }
  }
}

std::optional<Category> chooseCategory() {
  std::cout << "Choose a category:\n";
  for (std::size_t index = 0; index < kCategories.size(); ++index) {
    std::cout << "  " << index + 1 << ". " << categoryName(kCategories[index]) << '\n';
  }
  const std::optional<int> choice = readNumber("Category: ");
  if (!choice.has_value() || *choice < 1 ||
      *choice > static_cast<int>(kCategories.size())) {
    std::cout << "Choose a listed category.\n";
    return std::nullopt;
  }
  return kCategories.at(static_cast<std::size_t>(*choice - 1));
}

std::optional<std::string> chooseList(const GroceryListStore& store) {
  const auto& lists = store.lists();
  if (lists.empty()) {
    std::cout << "No lists exist yet.\n";
    return std::nullopt;
  }
  std::cout << "Saved lists:\n";
  for (std::size_t index = 0; index < lists.size(); ++index) {
    std::cout << "  " << index + 1 << ". " << lists[index].name << '\n';
  }
  const std::optional<int> choice = readNumber("List number: ");
  if (!choice.has_value() || *choice < 1 || *choice > static_cast<int>(lists.size())) {
    std::cout << "Choose a listed list.\n";
    return std::nullopt;
  }
  return lists.at(static_cast<std::size_t>(*choice - 1)).name;
}

bool isYes(const std::string& value) {
  return value == "y" || value == "Y" || value == "yes" || value == "YES";
}

void runListMenu(GroceryListStore& store, const std::string& listName) {
  while (true) {
    std::cout << "\nList: " << listName
              << "\n1. View groceries\n2. Add grocery\n3. Edit quantity\n"
                 "4. Change category everywhere\n5. Remove grocery\n0. Back\n";
    const std::optional<int> choice = readNumber("Choice: ");
    if (!choice.has_value()) {
      if (std::cin.eof()) {
        return;
      }
      continue;
    }
    if (*choice == 0) {
      return;
    }
    if (*choice == 1) {
      const auto& lists = store.lists();
      for (const GroceryList& list : lists) {
        if (list.name == listName) {
          printCategorizedList(list);
          break;
        }
      }
      continue;
    }
    if (*choice == 2) {
      const std::optional<std::string> entry = readLine("Grocery (for example, 2 apples): ");
      if (!entry.has_value()) {
        return;
      }
      OperationResult result = store.addItem(listName, *entry);
      if (!result.ok && result.message == "Choose a category for this grocery item.") {
        const std::optional<Category> category = chooseCategory();
        if (!category.has_value()) {
          continue;
        }
        result = store.addItem(listName, *entry, *category);
      }
      printResult(result);
      continue;
    }
    if (*choice == 3) {
      const std::optional<std::string> item = readLine("Item name: ");
      const std::optional<std::int64_t> quantity = readQuantity("Final quantity: ");
      if (!item.has_value() || !quantity.has_value()) {
        continue;
      }
      printResult(store.editItemQuantity(listName, *item, *quantity));
      continue;
    }
    if (*choice == 4) {
      const std::optional<std::string> item = readLine("Item name: ");
      if (!item.has_value()) {
        return;
      }
      const std::optional<Category> category = chooseCategory();
      if (!category.has_value()) {
        continue;
      }
      const std::optional<std::string> confirmation = readLine(
          "This changes the category in every saved list. Continue? (yes/no): ");
      if (!confirmation.has_value()) {
        return;
      }
      if (!isYes(*confirmation)) {
        std::cout << "Category change cancelled.\n";
        continue;
      }
      printResult(store.changeCategory(*item, *category));
      continue;
    }
    if (*choice == 5) {
      const std::optional<std::string> item = readLine("Item name: ");
      if (!item.has_value()) {
        return;
      }
      printResult(store.removeItem(listName, *item));
      continue;
    }
    std::cout << "Choose a listed menu option.\n";
  }
}

}  // namespace

int main() {
  GroceryListStore store("data/grocery_lists.db");
  const OperationResult loaded = store.load();
  if (!loaded.ok) {
    std::cerr << "Error: " << loaded.message << '\n';
    return 1;
  }

  while (true) {
    std::cout << "\nGrocery Lists\n1. Create list\n2. View or edit list\n"
                 "3. Rename list\n4. Delete list\n0. Exit\n";
    const std::optional<int> choice = readNumber("Choice: ");
    if (!choice.has_value()) {
      if (std::cin.eof()) {
        return 0;
      }
      continue;
    }
    if (*choice == 0) {
      return 0;
    }
    if (*choice == 1) {
      const std::optional<std::string> name = readLine("New list name: ");
      if (!name.has_value()) {
        return 0;
      }
      printResult(store.createList(*name));
      continue;
    }
    if (*choice == 2) {
      const std::optional<std::string> name = chooseList(store);
      if (name.has_value()) {
        runListMenu(store, *name);
      }
      continue;
    }
    if (*choice == 3) {
      const std::optional<std::string> currentName = chooseList(store);
      if (!currentName.has_value()) {
        continue;
      }
      const std::optional<std::string> newName = readLine("New list name: ");
      if (!newName.has_value()) {
        return 0;
      }
      printResult(store.renameList(*currentName, *newName));
      continue;
    }
    if (*choice == 4) {
      const std::optional<std::string> name = chooseList(store);
      if (!name.has_value()) {
        continue;
      }
      const std::optional<std::string> confirmation =
          readLine("Delete this list permanently? (yes/no): ");
      if (!confirmation.has_value()) {
        return 0;
      }
      if (isYes(*confirmation)) {
        printResult(store.deleteList(*name));
      } else {
        std::cout << "Deletion cancelled.\n";
      }
      continue;
    }
    std::cout << "Choose a listed menu option.\n";
  }
}
