# Konfiguracja GAMON v1.4
CXX = $(shell command -v clang++ || echo g++)
TARGET = gamon
SRC = gamon.cpp
PREFIX = /usr/local

# Flagi: C++20, O3 dla wydajnosci
CXXFLAGS = -std=c++20 -O3
LIBS = -lsfml-graphics -lsfml-window -lsfml-system

# Kompilacja przez distcc (klaster Tata + Krysiek)
all:
	@echo "Kompilacja za pomoca: $(CXX)"
	@if command -v distcc > /dev/null; then \
		distcc $(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LIBS); \
	else \
		$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LIBS); \
	fi

# Instalacja do systemu
install: all
	@echo "Instalowanie $(TARGET) w $(PREFIX)/bin..."
	@install -D -m 755 $(TARGET) $(PREFIX)/bin/$(TARGET)
	@echo "Gotowe! Mozesz teraz uruchomic program komenda: $(TARGET)"

# Odinstalowanie
uninstall:
	@echo "Usuwanie $(TARGET) z $(PREFIX)/bin..."
	@rm -f $(PREFIX)/bin/$(TARGET)
	@echo "Usunieto."

clean:
	rm -f $(TARGET)

run: all
	./$(TARGET)
