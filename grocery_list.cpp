#include "grocery_list.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>

namespace {

constexpr const char* kFileHeader = "GROCERY_LIST_V1";
constexpr std::size_t kMaximumRecords = 100000;

std::string trimAndCollapse(const std::string& value) {
  std::string result;
  bool pendingSpace = false;
  for (unsigned char character : value) {
    if (std::isspace(character)) {
      pendingSpace = !result.empty();
      continue;
    }
    if (pendingSpace) {
      result += ' ';
      pendingSpace = false;
    }
    result += static_cast<char>(character);
  }
  return result;
}

std::string toLower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
    return static_cast<char>(std::tolower(character));
  });
  return value;
}

std::string canonicalizeItem(const std::string& value) {
  std::string canonical = toLower(trimAndCollapse(value));
  if (canonical.size() > 2 && canonical.back() == 's' &&
      canonical[canonical.size() - 2] != 's') {
    canonical.pop_back();
  }
  return canonical;
}

std::string canonicalizeListName(const std::string& value) {
  return toLower(trimAndCollapse(value));
}

OperationResult parseEntry(const std::string& entry, std::int64_t& quantity,
                           std::string& displayName, std::string& canonicalName) {
  std::istringstream input(entry);
  if (!(input >> quantity) || quantity <= 0) {
    return {false, "Enter a positive quantity followed by an item name."};
  }
  std::string remaining;
  std::getline(input, remaining);
  displayName = trimAndCollapse(remaining);
  canonicalName = canonicalizeItem(displayName);
  if (canonicalName.empty()) {
    return {false, "Enter an item name after the quantity."};
  }
  return {true, ""};
}

std::optional<Category> builtInCategory(const std::string& canonicalName) {
  if (canonicalName == "apple" || canonicalName == "banana" ||
      canonicalName == "lettuce" || canonicalName == "tomato" ||
      canonicalName == "carrot" || canonicalName == "onion") {
    return Category::Produce;
  }
  if (canonicalName == "bread" || canonicalName == "rice" ||
      canonicalName == "pasta" || canonicalName == "cereal") {
    return Category::Grains;
  }
  if (canonicalName == "pea" || canonicalName == "ice cream" ||
      canonicalName == "pizza" || canonicalName == "french fry") {
    return Category::Frozen;
  }
  if (canonicalName == "bean" || canonicalName == "ketchup" ||
      canonicalName == "mustard" || canonicalName == "soup") {
    return Category::CannedCondiments;
  }
  if (canonicalName == "milk" || canonicalName == "egg" ||
      canonicalName == "cheese" || canonicalName == "chicken" ||
      canonicalName == "beef" || canonicalName == "yogurt") {
    return Category::MeatDairy;
  }
  return std::nullopt;
}

bool isValidCategory(int value) {
  return value >= static_cast<int>(Category::Produce) &&
         value <= static_cast<int>(Category::Other);
}

OperationResult persistStore(
    const std::string& dataPath, const std::vector<GroceryList>& lists,
    const std::vector<std::pair<std::string, Category>>& categoryRegistry) {
  const std::filesystem::path target(dataPath);
  if (!target.parent_path().empty()) {
    std::error_code directoryError;
    std::filesystem::create_directories(target.parent_path(), directoryError);
    if (directoryError) {
      return {false, "Could not create the data directory; no changes were saved."};
    }
  }

  const std::string temporaryPath = dataPath + ".tmp";
  std::ofstream output(temporaryPath, std::ios::trunc);
  if (!output) {
    return {false, "Could not open the data file; no changes were saved."};
  }
  output << kFileHeader << '\n';
  output << "REGISTRY " << categoryRegistry.size() << '\n';
  for (const auto& entry : categoryRegistry) {
    output << std::quoted(entry.first) << ' ' << static_cast<int>(entry.second) << '\n';
  }
  output << "LISTS " << lists.size() << '\n';
  for (const GroceryList& list : lists) {
    output << "LIST " << std::quoted(list.name) << ' ' << list.items.size() << '\n';
    for (const GroceryItem& item : list.items) {
      output << "ITEM " << std::quoted(item.canonicalName) << ' ' << std::quoted(item.displayName)
             << ' ' << item.quantity << ' ' << static_cast<int>(item.category) << '\n';
    }
  }
  output.flush();
  output.close();
  if (!output) {
    std::remove(temporaryPath.c_str());
    return {false, "Could not write the data file; no changes were saved."};
  }
  if (std::rename(temporaryPath.c_str(), dataPath.c_str()) != 0) {
    std::remove(temporaryPath.c_str());
    return {false, "Could not replace the data file; no changes were saved."};
  }
  return {true, ""};
}

}  // namespace

GroceryListStore::GroceryListStore(std::string dataPath) : dataPath_(std::move(dataPath)) {}

OperationResult GroceryListStore::load() {
  if (!std::filesystem::exists(dataPath_)) {
    return {true, "No saved lists found; starting with an empty store."};
  }

  std::ifstream input(dataPath_);
  std::string header;
  if (!input || !std::getline(input, header) || header != kFileHeader) {
    return {false, "Saved data is unreadable and was left unchanged."};
  }

  std::string marker;
  std::size_t registryCount = 0;
  if (!(input >> marker >> registryCount) || marker != "REGISTRY" ||
      registryCount > kMaximumRecords) {
    return {false, "Saved data is unreadable and was left unchanged."};
  }
  std::vector<std::pair<std::string, Category>> registry;
  for (std::size_t index = 0; index < registryCount; ++index) {
    std::string itemName;
    int categoryValue = 0;
    if (!(input >> std::quoted(itemName) >> categoryValue) || itemName.empty() ||
        !isValidCategory(categoryValue)) {
      return {false, "Saved data is unreadable and was left unchanged."};
    }
    registry.push_back({itemName, static_cast<Category>(categoryValue)});
  }

  std::size_t listCount = 0;
  if (!(input >> marker >> listCount) || marker != "LISTS" || listCount > kMaximumRecords) {
    return {false, "Saved data is unreadable and was left unchanged."};
  }
  std::vector<GroceryList> loadedLists;
  for (std::size_t listIndex = 0; listIndex < listCount; ++listIndex) {
    GroceryList list;
    std::size_t itemCount = 0;
    if (!(input >> marker >> std::quoted(list.name) >> itemCount) || marker != "LIST" ||
        canonicalizeListName(list.name).empty() || itemCount > kMaximumRecords) {
      return {false, "Saved data is unreadable and was left unchanged."};
    }
    for (std::size_t itemIndex = 0; itemIndex < itemCount; ++itemIndex) {
      GroceryItem item;
      int categoryValue = 0;
      if (!(input >> marker >> std::quoted(item.canonicalName) >> std::quoted(item.displayName) >>
            item.quantity >> categoryValue) || marker != "ITEM" || item.canonicalName.empty() ||
          item.displayName.empty() || item.quantity <= 0 || !isValidCategory(categoryValue)) {
        return {false, "Saved data is unreadable and was left unchanged."};
      }
      item.category = static_cast<Category>(categoryValue);
      list.items.push_back(item);
    }
    loadedLists.push_back(list);
  }
  std::string trailing;
  if (input >> trailing) {
    return {false, "Saved data is unreadable and was left unchanged."};
  }
  lists_ = std::move(loadedLists);
  categoryRegistry_ = std::move(registry);
  return {true, "Saved lists loaded."};
}

OperationResult GroceryListStore::createList(const std::string& name) {
  const std::string displayName = trimAndCollapse(name);
  const std::string canonicalName = canonicalizeListName(name);
  if (canonicalName.empty()) {
    return {false, "List names cannot be blank."};
  }
  const auto duplicate = std::find_if(lists_.begin(), lists_.end(), [&](const GroceryList& list) {
    return canonicalizeListName(list.name) == canonicalName;
  });
  if (duplicate != lists_.end()) {
    return {false, "A list with that name already exists."};
  }
  std::vector<GroceryList> candidateLists = lists_;
  candidateLists.push_back({displayName, {}});
  OperationResult saved = persistStore(dataPath_, candidateLists, categoryRegistry_);
  if (!saved.ok) {
    return saved;
  }
  lists_ = std::move(candidateLists);
  return {true, "List created."};
}

OperationResult GroceryListStore::renameList(const std::string& currentName,
                                             const std::string& newName) {
  const std::string currentKey = canonicalizeListName(currentName);
  const std::string newKey = canonicalizeListName(newName);
  const std::string displayName = trimAndCollapse(newName);
  if (newKey.empty()) {
    return {false, "List names cannot be blank."};
  }
  std::vector<GroceryList> candidateLists = lists_;
  auto target = std::find_if(candidateLists.begin(), candidateLists.end(),
                             [&](const GroceryList& list) {
                               return canonicalizeListName(list.name) == currentKey;
                             });
  if (target == candidateLists.end()) {
    return {false, "List not found."};
  }
  const auto duplicate = std::find_if(candidateLists.begin(), candidateLists.end(),
                                      [&](const GroceryList& list) {
                                        return &list != &*target &&
                                               canonicalizeListName(list.name) == newKey;
                                      });
  if (duplicate != candidateLists.end()) {
    return {false, "A list with that name already exists."};
  }
  target->name = displayName;
  OperationResult saved = persistStore(dataPath_, candidateLists, categoryRegistry_);
  if (!saved.ok) {
    return saved;
  }
  lists_ = std::move(candidateLists);
  return {true, "List renamed."};
}

OperationResult GroceryListStore::deleteList(const std::string& name) {
  const std::string key = canonicalizeListName(name);
  std::vector<GroceryList> candidateLists = lists_;
  const auto target = std::find_if(candidateLists.begin(), candidateLists.end(),
                                   [&](const GroceryList& list) {
                                     return canonicalizeListName(list.name) == key;
                                   });
  if (target == candidateLists.end()) {
    return {false, "List not found."};
  }
  candidateLists.erase(target);
  OperationResult saved = persistStore(dataPath_, candidateLists, categoryRegistry_);
  if (!saved.ok) {
    return saved;
  }
  lists_ = std::move(candidateLists);
  return {true, "List deleted."};
}

OperationResult GroceryListStore::addItem(const std::string& listName, const std::string& entry,
                                          std::optional<Category> selectedCategory) {
  const std::string listKey = canonicalizeListName(listName);
  std::vector<GroceryList> candidateLists = lists_;
  std::vector<std::pair<std::string, Category>> candidateRegistry = categoryRegistry_;
  auto list = std::find_if(candidateLists.begin(), candidateLists.end(), [&](const GroceryList& current) {
    return canonicalizeListName(current.name) == listKey;
  });
  if (list == candidateLists.end()) {
    return {false, "List not found."};
  }
  std::int64_t quantity = 0;
  std::string displayName;
  std::string canonicalName;
  OperationResult parsed = parseEntry(entry, quantity, displayName, canonicalName);
  if (!parsed.ok) {
    return parsed;
  }
  std::optional<Category> category = categoryForEntry(entry);
  if (!category.has_value()) {
    category = selectedCategory;
  }
  if (!category.has_value()) {
    return {false, "Choose a category for this grocery item."};
  }
  if (selectedCategory.has_value()) {
    auto saved = std::find_if(candidateRegistry.begin(), candidateRegistry.end(),
                              [&](const auto& savedEntry) {
                                return savedEntry.first == canonicalName;
                              });
    if (saved == candidateRegistry.end()) {
      candidateRegistry.push_back({canonicalName, *selectedCategory});
    } else {
      saved->second = *selectedCategory;
    }
    category = selectedCategory;
  }
  auto existing = std::find_if(list->items.begin(), list->items.end(),
                               [&](const GroceryItem& item) {
                                 return item.canonicalName == canonicalName;
                               });
  const bool mergedItem = existing != list->items.end();
  if (existing != list->items.end()) {
    if (existing->quantity > std::numeric_limits<std::int64_t>::max() - quantity) {
      return {false, "Quantity is too large."};
    }
    existing->quantity += quantity;
  } else {
    list->items.push_back({canonicalName, displayName, quantity, *category});
  }
  OperationResult saved = persistStore(dataPath_, candidateLists, candidateRegistry);
  if (!saved.ok) {
    return saved;
  }
  lists_ = std::move(candidateLists);
  categoryRegistry_ = std::move(candidateRegistry);
  return {true, mergedItem ? "Item quantity updated." : "Item added."};
}

OperationResult GroceryListStore::changeCategory(const std::string& itemName, Category category) {
  const std::string canonicalName = canonicalizeItem(itemName);
  if (canonicalName.empty()) {
    return {false, "Item name cannot be blank."};
  }
  std::vector<GroceryList> candidateLists = lists_;
  std::vector<std::pair<std::string, Category>> candidateRegistry = categoryRegistry_;
  bool found = false;
  for (GroceryList& list : candidateLists) {
    for (GroceryItem& item : list.items) {
      if (item.canonicalName == canonicalName) {
        item.category = category;
        found = true;
      }
    }
  }
  if (!found) {
    return {false, "Item not found in any list."};
  }
  auto saved = std::find_if(candidateRegistry.begin(), candidateRegistry.end(),
                            [&](const auto& savedEntry) {
                              return savedEntry.first == canonicalName;
                            });
  if (saved == candidateRegistry.end()) {
    candidateRegistry.push_back({canonicalName, category});
  } else {
    saved->second = category;
  }
  OperationResult persisted = persistStore(dataPath_, candidateLists, candidateRegistry);
  if (!persisted.ok) {
    return persisted;
  }
  lists_ = std::move(candidateLists);
  categoryRegistry_ = std::move(candidateRegistry);
  return {true, "Category updated everywhere."};
}

OperationResult GroceryListStore::editItemQuantity(const std::string& listName,
                                                    const std::string& itemName,
                                                    std::int64_t quantity) {
  if (quantity <= 0) {
    return {false, "Quantity must be positive."};
  }
  const std::string listKey = canonicalizeListName(listName);
  const std::string itemKey = canonicalizeItem(itemName);
  std::vector<GroceryList> candidateLists = lists_;
  auto list = std::find_if(candidateLists.begin(), candidateLists.end(),
                           [&](const GroceryList& current) {
                             return canonicalizeListName(current.name) == listKey;
                           });
  if (list == candidateLists.end()) {
    return {false, "List not found."};
  }
  auto item = std::find_if(list->items.begin(), list->items.end(),
                           [&](const GroceryItem& current) {
                             return current.canonicalName == itemKey;
                           });
  if (item == list->items.end()) {
    return {false, "Item not found."};
  }
  item->quantity = quantity;
  OperationResult saved = persistStore(dataPath_, candidateLists, categoryRegistry_);
  if (!saved.ok) {
    return saved;
  }
  lists_ = std::move(candidateLists);
  return {true, "Item quantity updated."};
}

OperationResult GroceryListStore::removeItem(const std::string& listName,
                                              const std::string& itemName) {
  const std::string listKey = canonicalizeListName(listName);
  const std::string itemKey = canonicalizeItem(itemName);
  std::vector<GroceryList> candidateLists = lists_;
  auto list = std::find_if(candidateLists.begin(), candidateLists.end(),
                           [&](const GroceryList& current) {
                             return canonicalizeListName(current.name) == listKey;
                           });
  if (list == candidateLists.end()) {
    return {false, "List not found."};
  }
  const auto item = std::find_if(list->items.begin(), list->items.end(),
                                 [&](const GroceryItem& current) {
                                   return current.canonicalName == itemKey;
                                 });
  if (item == list->items.end()) {
    return {false, "Item not found."};
  }
  list->items.erase(item);
  OperationResult saved = persistStore(dataPath_, candidateLists, categoryRegistry_);
  if (!saved.ok) {
    return saved;
  }
  lists_ = std::move(candidateLists);
  return {true, "Item removed."};
}

std::optional<Category> GroceryListStore::categoryForEntry(const std::string& entry) const {
  std::int64_t quantity = 0;
  std::string displayName;
  std::string canonicalName;
  if (!parseEntry(entry, quantity, displayName, canonicalName).ok) {
    return std::nullopt;
  }
  const auto saved = std::find_if(categoryRegistry_.begin(), categoryRegistry_.end(),
                                  [&](const auto& value) { return value.first == canonicalName; });
  if (saved != categoryRegistry_.end()) {
    return saved->second;
  }
  return builtInCategory(canonicalName);
}

const std::vector<GroceryList>& GroceryListStore::lists() const { return lists_; }
