#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

enum class Category {
  Produce,
  Grains,
  Frozen,
  CannedCondiments,
  MeatDairy,
  Other,
};

struct GroceryItem {
  std::string canonicalName;
  std::string displayName;
  std::int64_t quantity{};
  Category category{};
};

struct GroceryList {
  std::string name;
  std::vector<GroceryItem> items;
};

struct OperationResult {
  bool ok{};
  std::string message;
};

class GroceryListStore {
 public:
  explicit GroceryListStore(std::string dataPath);

  OperationResult load();
  OperationResult createList(const std::string& name);
  OperationResult renameList(const std::string& currentName, const std::string& newName);
  OperationResult deleteList(const std::string& name);
  OperationResult addItem(const std::string& listName, const std::string& entry,
                          std::optional<Category> selectedCategory = std::nullopt);
  OperationResult editItemQuantity(const std::string& listName, const std::string& itemName,
                                   std::int64_t quantity);
  OperationResult removeItem(const std::string& listName, const std::string& itemName);
  OperationResult changeCategory(const std::string& itemName, Category category);
  std::optional<Category> categoryForEntry(const std::string& entry) const;
  const std::vector<GroceryList>& lists() const;

 private:
  std::string dataPath_;
  std::vector<GroceryList> lists_;
  std::vector<std::pair<std::string, Category>> categoryRegistry_;
};
