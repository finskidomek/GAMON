# 1. Inteligentne wykrywanie kompilatora
# Szukamy clang++, jeśli nie ma - bierzemy g++
CXX_BIN := $(shell command -v clang++ || echo g++)

# 2. Nazwy plików
TARGET = gamon
SRC = gamon.cpp

# 3. Flagi i biblioteki
CXXFLAGS = -std=c++20 -O3
LIBS = -lsfml-graphics -lsfml-window -lsfml-system

# --- AKCJE ---

# Domyslna akcja: uzywa distcc TYLKO u Ciebie (jeśli masz distcc w systemie)
# Jeśli ktos inny wpisze 'make', uzyje po prostu wykrytego kompilatora
all:
	@echo "Sprawdzam kompilator: $(CXX_BIN)"
	@if command -v distcc > /dev/null; then \
		echo "Wykryto distcc! Kompilacja klastrowa..."; \
		distcc $(CXX_BIN) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LIBS); \
	else \
		echo "Brak distcc, kompilacja lokalna..."; \
		$(CXX_BIN) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LIBS); \
	fi

# Wymuszona kompilacja lokalna
local:
	@echo "Kompilacja lokalna za pomoca: $(CXX_BIN)"
	$(CXX_BIN) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LIBS)

clean:
	rm -f $(TARGET)

run: all
	./$(TARGET)
