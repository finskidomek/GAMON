# Konfiguracja
CXX = clang++
TARGET = gamon
SRC = gamon.cpp

# Flagi: C++20 (dla filesystem), O3 (optymalizacja wydajnosci)
CXXFLAGS = -std=c++20 -O3
LIBS = -lsfml-graphics -lsfml-window -lsfml-system

# Domyslna akcja: kompilacja przez distcc (uzywa Taty i Kryska)
all:
	distcc $(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LIBS)

# Akcja lokalna: gdyby Krysiek byl wylaczony
local:
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LIBS)

# Czyszczenie
clean:
	rm -f $(TARGET)

# Uruchomienie
run: all
	./$(TARGET)
