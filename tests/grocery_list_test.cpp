#include "../grocery_list.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace {

void require(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    std::exit(1);
  }
}

void mergesSimpleSingularPluralEntries() {
  const std::string path = "/tmp/grocery-list-merge-test.db";
  std::filesystem::remove(path);
  GroceryListStore store(path);
  require(store.createList("Weekend").ok, "creates a list");
  require(store.addItem("Weekend", "1 apple", Category::Produce).ok,
          "adds a singular item");
  require(store.addItem("Weekend", "2 apples", Category::Produce).ok,
          "adds a plural item");

  const auto& items = store.lists().at(0).items;
  require(items.size() == 1, "merges singular and plural items");
  require(items.at(0).canonicalName == "apple", "stores canonical item name");
  require(items.at(0).quantity == 3, "adds duplicate quantities");
  std::filesystem::remove(path);
}

void remembersChosenCategoriesAndPersistsLists() {
  const std::string path = "/tmp/grocery-list-persistence-test.db";
  std::filesystem::remove(path);

  GroceryListStore store(path);
  require(store.createList("First").ok, "creates first persisted list");
  require(store.addItem("First", "2 tortillas", Category::Other).ok,
          "stores a chosen category");
  require(store.createList("Second").ok, "creates second persisted list");
  require(store.categoryForEntry("1 tortilla") == Category::Other,
          "remembers a chosen category globally");
  require(store.addItem("Second", "1 tortilla").ok,
          "adds a globally categorized item without a prompt");

  GroceryListStore reloaded(path);
  require(reloaded.load().ok, "loads saved data");
  require(reloaded.lists().size() == 2, "restores lists in creation order");
  require(reloaded.lists().at(1).items.at(0).category == Category::Other,
          "restores globally chosen category across a restart");
  std::filesystem::remove(path);
}

void supportsListAndItemCrudWithGlobalCategoryCorrections() {
  const std::string path = "/tmp/grocery-list-crud-test.db";
  std::filesystem::remove(path);
  GroceryListStore store(path);
  require(store.createList("Weekday").ok, "creates weekday list");
  require(store.createList("Weekend").ok, "creates weekend list");
  require(!store.createList(" weekday ").ok, "rejects case-insensitive duplicate lists");
  require(store.renameList("Weekend", "Trip").ok, "renames a list");
  require(store.addItem("Weekday", "2 milk", Category::MeatDairy).ok,
          "adds an item to first list");
  require(store.addItem("Trip", "1 milk").ok, "adds remembered item to second list");
  require(store.editItemQuantity("Weekday", "milk", 5).ok, "sets final item quantity");
  require(store.lists().at(0).items.at(0).quantity == 5, "stores final quantity");
  require(store.changeCategory("milk", Category::Other).ok, "changes category globally");
  require(store.lists().at(0).items.at(0).category == Category::Other,
          "changes first list category");
  require(store.lists().at(1).items.at(0).category == Category::Other,
          "changes second list category");
  GroceryListStore reloadedAfterCategoryChange(path);
  require(reloadedAfterCategoryChange.load().ok, "loads a global category correction");
  require(reloadedAfterCategoryChange.lists().at(0).items.at(0).category == Category::Other,
          "persists a global category correction");
  require(store.removeItem("Trip", "milk").ok, "removes an item");
  require(store.deleteList("Trip").ok, "deletes a list");
  require(store.lists().size() == 1 && store.lists().at(0).name == "Weekday",
          "preserves remaining creation order");
  std::filesystem::remove(path);
}

void preservesCorruptDataInsteadOfResettingIt() {
  const std::string path = "/tmp/grocery-list-corrupt-test.db";
  {
    std::ofstream output(path, std::ios::trunc);
    output << "not a grocery-list data file\n";
  }
  GroceryListStore store(path);
  require(!store.load().ok, "rejects corrupt saved data");
  std::ifstream input(path);
  std::string contents;
  std::getline(input, contents);
  require(contents == "not a grocery-list data file", "preserves corrupt saved data");
  std::filesystem::remove(path);
}

}  // namespace

int main() {
  mergesSimpleSingularPluralEntries();
  remembersChosenCategoriesAndPersistsLists();
  supportsListAndItemCrudWithGlobalCategoryCorrections();
  preservesCorruptDataInsteadOfResettingIt();
  std::cout << "All grocery-list tests passed.\n";
}
