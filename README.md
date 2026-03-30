# 🚀 GAMON v1.4 - Kinetic CPU Monitor for Gentoo
**(Gentoo Advanced Monitoring Of Nodes)**

**GAMON** to dynamiczny, oparty na fizyce monitor obciążenia procesora, stworzony specjalnie dla systemów **Gentoo Linux**. Każdy rdzeń Twojego CPU jest reprezentowany przez "gumowy" kwadrat, którego energia, prędkość i kolor zależą bezpośrednio od aktualnego obciążenia systemu.

---
### 🎓 Cel projektu
Projekt został stworzony w **celach edukacyjnych**, aby zademonstrować integrację nowoczesnego C++20, biblioteki SFML 3.0 oraz systemowych mechanizmów monitorowania zasobów Linuxa (`/proc/stat`).

### 🧠 Autorzy i Wsparcie
Program powstał przy ścisłej współpracy:
- **Koszmara** (Główny Architekt, Pomysłodawca & Gamon)
- **GHOST-AI** (Asystent Kodowania)
- Kompilacja wspierana przez klaster **distcc** (Maszyny: Tata & Krysiek)
---

## ✨ Główne Funkcje

- **Fizyka Kinetyczna**: Kwadraty reagują na obciążenie CPU – im wyższy *load*, tym szybciej wirują i gwałtowniej się odbijają.
- **Efekt "Soft-Body"**: Kwadraty zachowują się jak gumowe obiekty – wibrują i odkształcają się przy zderzeniach.
- **System Cząsteczek (Iskry)**: Każda kolizja generuje deszcz iskier, których ilość i rozmiar zależą od energii uderzenia (loadu).
- **Płynna Paleta Barw**: Od zieleni (Idle 0%) przez żółty, aż po krwistą czerwień (Stress 100%).
- **Dwa Tryby Pracy**:
  - `Klawisz 1`: Widok wszystkich rdzeni (Cores Mode).
  - `Klawisz 2`: Mega-Dashboard (Suma obciążenia całego CPU).
- **Dane w czasie rzeczywistym**: Wyświetla MHz każdego rdzenia oraz temperaturę procesora (z `coretemp`).
- **Screen Shake**: Drżenie ekranu przy potężnych uderzeniach w trybie Total (im większy load, tym mocniej trzęsie).

## 🛠️ Wymagania

- **System**: Gentoo Linux
- **Biblioteki**: SFML 3.0+
- **Kompilator**: Clang 21 (z obsługą C++20)
- **Czcionka**: Liberation Mono Bold (`/usr/share/fonts/liberation-fonts/`)

## 🏗️ Kompilacja i Uruchomienie

Użyj dołączonego pliku `Makefile`:

```bash
make          # Kompilacja przez klastr distcc
./gamon       # Uruchomienie
