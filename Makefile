CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Werror
TARGET := grocery-list
SOURCES := main.cpp grocery_list.cpp

.PHONY: all run

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)
