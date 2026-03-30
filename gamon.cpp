#include <SFML/Graphics.hpp>
#include <vector>
#include <fstream>
#include <string>
#include <sstream>
#include <optional>
#include <cmath>
#include <iomanip>
#include <filesystem>
#include <thread> // Nowe: do wykrywania liczby rdzeni

// --- STRUKTURY ---
struct CPUState { long u, n, s, i; };

struct Particle {
    sf::RectangleShape shape;
    sf::Vector2f velocity;
    float lifetime = 255.f;
};

struct Square {
    sf::RectangleShape shape;
    sf::Vector2f velocity;
    CPUState lastState = {0, 0, 0, 0};
    float currentLoad = 0.f;
    float currentMHz = 0.f;
    float squashTimer = 0.f;
    int id = -1;
};

// --- FUNKCJE SYSTEMOWE ---
float getCoreMHz(int coreIdx) {
    std::ifstream file("/proc/cpuinfo");
    std::string line;
    int currentCore = 0;
    while (std::getline(file, line)) {
        if (line.find("cpu MHz") != std::string::npos) {
            if (currentCore == coreIdx) return std::stof(line.substr(line.find(":") + 2));
            currentCore++;
        }
    }
    return 0.0f;
}

float getCPUTemp() {
    try {
        for (const auto& entry : std::filesystem::directory_iterator("/sys/class/hwmon/")) {
            std::ifstream nf(entry.path().string() + "/name");
            std::string name;
            if (nf >> name && name == "coretemp") {
                std::ifstream tf(entry.path().string() + "/temp1_input");
                float t; if (tf >> t) return t / 1000.0f;
            }
        }
    } catch (...) {}
    return 0.0f;
}

float getLoad(int coreIdx, CPUState& last) {
    std::ifstream file("/proc/stat");
    std::string line;
    std::string target = (coreIdx == -1) ? "cpu " : "cpu" + std::to_string(coreIdx) + " ";
    while (std::getline(file, line)) {
        if (line.compare(0, target.length(), target) == 0) {
            std::stringstream ss(line);
            std::string name; long u, n, s, i;
            ss >> name >> u >> n >> s >> i;
            long total = (u - last.u) + (n - last.n) + (s - last.s) + (i - last.i);
            float load = (total > 0) ? 100.0f * (float)(total - (i - last.i)) / total : 0.0f;
            last = {u, n, s, i}; return load;
        }
    }
    return 0.0f;
}

void createSparks(std::vector<Particle>& particles, sf::Vector2f pos, sf::Color col, float intensity, bool isMega) {
    int baseCount = static_cast<int>(2 + (intensity * 0.45f));
    int count = isMega ? baseCount * 5 : baseCount;
    for (int i = 0; i < count; ++i) {
        Particle p;
        float size = (intensity > 60 && std::rand() % 10 > 6) ? 6.f : 2.f;
        p.shape.setSize({size, size});
        p.shape.setPosition(pos);
        p.shape.setFillColor(col);
        float sparkSpeed = (0.05f + (intensity * 0.007f)) * (isMega ? 2.5f : 1.0f);
        p.velocity = {((std::rand() % 200) - 100) * sparkSpeed, ((std::rand() % 200) - 100) * sparkSpeed};
        particles.push_back(p);
    }
}

int main() {
    const unsigned int W = 1100, H = 850;
    sf::RenderWindow window(sf::VideoMode({W, H}), "GAMON 1.4 - Kinetic Soft-Body");
    window.setFramerateLimit(60);
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    sf::Font font;
    if (!font.openFromFile("/usr/share/fonts/liberation-fonts/LiberationMono-Bold.ttf")) return -1;

    int mode = 1;
    std::vector<Square> coreSquares;
    Square totalSquare;
    std::vector<Particle> particles;
    CPUState globalLast = {0,0,0,0};
    float screenShake = 0.f;
    float cpuTemp = 0.f;

    // NOWOŚĆ: Automatyczne wykrywanie liczby rdzeni
    const int coreCount = std::thread::hardware_concurrency();

    for (int i = 0; i < coreCount; ++i) {
        Square s; s.id = i;
        float squareSize = (coreCount > 12) ? 60.f : 85.f; // Mniejsze kwadraty dla wielu rdzeni
        s.shape.setSize({squareSize, squareSize});
        s.shape.setOrigin({squareSize/2.f, squareSize/2.f});

        // Układanie w rzędach po 8
        float posX = 120.f + (i % 8) * (W / 9.f);
        float posY = 150.f + (i / 8) * 150.f;
        s.shape.setPosition({posX, posY});

        s.velocity = {(float)(std::rand() % 5 + 3), (float)(std::rand() % 5 + 3)};
        coreSquares.push_back(s);
    }

    totalSquare.shape.setSize({280.f, 280.f}); totalSquare.shape.setOrigin({140.f, 140.f});
    totalSquare.shape.setPosition({W/2.f, H/2.f}); totalSquare.velocity = {4.f, 4.f};

    sf::Clock cpuClock;
    sf::View defaultView = window.getDefaultView();

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();
            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->code == sf::Keyboard::Key::Num1) mode = 1;
                if (keyPressed->code == sf::Keyboard::Key::Num2) mode = 2;
            }
        }

        if (cpuClock.getElapsedTime().asMilliseconds() > 250) {
            cpuTemp = getCPUTemp();
            if (mode == 1) {
                for (auto& s : coreSquares) {
                    s.currentLoad = getLoad(s.id, s.lastState);
                    s.currentMHz = getCoreMHz(s.id);
                }
            } else {
                totalSquare.currentLoad = getLoad(-1, globalLast);
            }
            cpuClock.restart();
        }

        if (screenShake > 0) {
            sf::View shakeView = defaultView;
            shakeView.move({((std::rand()%20-10)/2.f) * screenShake, ((std::rand()%20-10)/2.f) * screenShake});
            window.setView(shakeView);
            screenShake *= 0.92f;
            if (screenShake < 0.1f) { screenShake = 0; window.setView(defaultView); }
        }

        for (auto it = particles.begin(); it != particles.end(); ) {
            it->shape.move(it->velocity); it->lifetime -= 5.f;
            if (it->lifetime <= 0) it = particles.erase(it);
            else {
                sf::Color c = it->shape.getFillColor();
                it->shape.setFillColor(sf::Color(c.r, c.g, c.b, (std::uint8_t)it->lifetime));
                ++it;
            }
        }

        window.clear(sf::Color::Black);
        for (auto& p : particles) window.draw(p.shape);

        auto updateSquare = [&](Square& s) {
            float f = s.currentLoad / 100.f;
            s.shape.move(s.velocity * (0.3f + f * 4.0f));
            s.shape.rotate(sf::degrees(f * 25.f));

            std::uint8_t r = (std::uint8_t)(f < 0.5f ? (f * 510.f) : 255.f);
            std::uint8_t g = (std::uint8_t)(f < 0.5f ? 255.f : (510.f - f * 510.f));
            sf::Color currentColor(r, g, 0);
            s.shape.setFillColor(currentColor);

            if (s.squashTimer > 0) {
                s.squashTimer -= 0.04f;
                float wave = std::sin(s.squashTimer * 12.f) * s.squashTimer * f;
                s.shape.setScale({1.0f + wave * 0.7f, 1.0f - wave * 0.7f});
            } else s.shape.setScale({1.f, 1.f});

            sf::Vector2f p = s.shape.getPosition();
            float rad = (s.shape.getSize().x * s.shape.getScale().x) / 2.f;
            if (p.x < rad) { s.shape.setPosition({rad, p.y}); s.velocity.x *= -1; s.squashTimer = 1.0f; createSparks(particles, s.shape.getPosition(), currentColor, s.currentLoad, (s.id == -1)); }
            if (p.x > W-rad) { s.shape.setPosition({W-rad, p.y}); s.velocity.x *= -1; s.squashTimer = 1.0f; createSparks(particles, s.shape.getPosition(), currentColor, s.currentLoad, (s.id == -1)); }
            if (p.y < rad) { s.shape.setPosition({p.x, rad}); s.velocity.y *= -1; s.squashTimer = 1.0f; createSparks(particles, s.shape.getPosition(), currentColor, s.currentLoad, (s.id == -1)); }
            if (p.y > H-120-rad) { s.shape.setPosition({p.x, H-120-rad}); s.velocity.y *= -1; s.squashTimer = 1.0f; createSparks(particles, s.shape.getPosition(), currentColor, s.currentLoad, (s.id == -1)); }

            if (s.id == -1 && s.squashTimer > 0.9f) screenShake = f * 8.0f;
            return currentColor;
        };

        if (mode == 1) {
            for (size_t i = 0; i < coreSquares.size(); ++i) {
                updateSquare(coreSquares[i]);
                for (size_t j = i + 1; j < coreSquares.size(); ++j) {
                    if (coreSquares[i].shape.getGlobalBounds().findIntersection(coreSquares[j].shape.getGlobalBounds())) {
                        std::swap(coreSquares[i].velocity, coreSquares[j].velocity);
                        float totalE = (coreSquares[i].currentLoad + coreSquares[j].currentLoad) / 200.f;
                        float power = 1.2f + totalE * 8.f;
                        coreSquares[i].shape.move(coreSquares[i].velocity * power);
                        coreSquares[j].shape.move(coreSquares[j].velocity * power);
                        coreSquares[i].squashTimer = 1.0f; coreSquares[j].squashTimer = 1.0f;
                        createSparks(particles, coreSquares[i].shape.getPosition(), sf::Color::White,
                                     coreSquares[i].currentLoad + coreSquares[j].currentLoad, false);
                    }
                }
                window.draw(coreSquares[i].shape);
                std::stringstream ss; ss << (int)coreSquares[i].currentLoad << "%\n" << (int)coreSquares[i].currentMHz << "M";
                sf::Text t(font, ss.str(), (coreCount > 12) ? 10 : 14);
                t.setOrigin({t.getLocalBounds().size.x/2, t.getLocalBounds().size.y/2 + 5});
                t.setPosition(coreSquares[i].shape.getPosition()); t.setFillColor(sf::Color::Black);
                window.draw(t);
            }
        } else {
            updateSquare(totalSquare);
            window.draw(totalSquare.shape);
            float avgM = 0; for(int i=0; i<coreCount; ++i) avgM += getCoreMHz(i);
            std::stringstream ss; ss << " TOTAL CPU\n  " << (int)totalSquare.currentLoad << "%\n " << (int)(avgM/coreCount) << "MHz\n  " << std::fixed << std::setprecision(1) << cpuTemp << "C";
            sf::Text t(font, ss.str(), 32);
            t.setOrigin({t.getLocalBounds().size.x/2, t.getLocalBounds().size.y/2 + 5});
            t.setPosition(totalSquare.shape.getPosition()); t.setFillColor(sf::Color::Black);
            window.draw(t);
        }

        sf::Text info(font, "CORES: " + std::to_string(coreCount) + " | TEMP: " + std::to_string((int)cpuTemp) + "C | 1/2: MODE | GAMON 1.4", 18);
        info.setPosition({30.f, H - 75.f}); info.setFillColor(sf::Color::Cyan);
        window.draw(info);
        window.display();
    }
    return 0;
}
