#include <SFML/Graphics.hpp>
#include <vector>
#include <fstream>
#include <string>
#include <sstream>
#include <optional>
#include <cmath>
#include <iomanip>
#include <filesystem>
#include <thread>
#include <algorithm>

// --- STRUKTURY ---
struct CPUState { long u, n, s, i; };
struct Particle { sf::RectangleShape shape; sf::Vector2f velocity; float lifetime = 255.f; };
struct Square {
    sf::RectangleShape shape; sf::Vector2f velocity;
    CPUState lastState = {0, 0, 0, 0};
    float currentLoad = 0.f; float currentMHz = 0.f; float currentTemp = 0.f;
    float squashTimer = 0.f; int id = -1;
};

// --- FUNKCJE SYSTEMOWE ---
float getCoreMHz(int coreIdx) {
    std::ifstream file("/proc/cpuinfo"); std::string line; int currentCore = 0;
    while (std::getline(file, line)) {
        if (line.find("cpu MHz") != std::string::npos) {
            if (currentCore == coreIdx) return std::stof(line.substr(line.find(":") + 2));
            currentCore++;
        }
    }
    return 0.0f;
}

float getCoreTemp(int coreIdx) {
    try {
        for (const auto& entry : std::filesystem::directory_iterator("/sys/class/hwmon/")) {
            std::ifstream nf(entry.path().string() + "/name"); std::string name;
            if (nf >> name && name == "coretemp") {
                std::ifstream tf(entry.path().string() + "/temp" + std::to_string(coreIdx + 2) + "_input");
                float t; if (tf >> t) return t / 1000.0f;
            }
        }
    } catch (...) {}
    return 0.0f;
}

float getPackageTemp() {
    try {
        for (const auto& entry : std::filesystem::directory_iterator("/sys/class/hwmon/")) {
            std::ifstream nf(entry.path().string() + "/name"); std::string name;
            if (nf >> name && name == "coretemp") {
                std::ifstream tf(entry.path().string() + "/temp1_input");
                float t; if (tf >> t) return t / 1000.0f;
            }
        }
    } catch (...) {}
    return 0.0f;
}

float getLoad(int coreIdx, CPUState& last) {
    std::ifstream file("/proc/stat"); std::string line;
    std::string target = (coreIdx == -1) ? "cpu " : "cpu" + std::to_string(coreIdx) + " ";
    while (std::getline(file, line)) {
        if (line.compare(0, target.length(), target) == 0) {
            std::stringstream ss(line); std::string name; long u, n, s, i;
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
        Particle p; float size = (intensity > 60 && std::rand() % 10 > 6) ? 6.f : 2.f;
        p.shape.setSize({size, size}); p.shape.setPosition(pos); p.shape.setFillColor(col);
        float sparkSpeed = (0.05f + (intensity * 0.007f)) * (isMega ? 2.5f : 1.0f);
        p.velocity = {((std::rand() % 200) - 100) * sparkSpeed, ((std::rand() % 200) - 100) * sparkSpeed};
        particles.push_back(p);
    }
}

int main(int argc, char* argv[]) {
    float alertThreshold = 80.0f;
    if (argc > 1) { try { alertThreshold = std::stof(argv[1]); } catch (...) {} }

    const unsigned int W = 1100, H = 850;
    sf::RenderWindow window(sf::VideoMode({W, H}), "GAMON 1.7.4 - Panic Alert");
    window.setFramerateLimit(60); std::srand(static_cast<unsigned>(std::time(nullptr)));
    sf::Font font; if (!font.openFromFile("/usr/share/fonts/liberation-fonts/LiberationMono-Bold.ttf")) return -1;

    int viewMode = 1; int dataSource = 1;
    const int coreCount = std::thread::hardware_concurrency();
    std::vector<Square> coreSquares; Square totalSquare; std::vector<Particle> particles;
    CPUState globalLast = {0,0,0,0}; float screenShake = 0.f, pkgTemp = 0.f;

    for (int i = 0; i < coreCount; ++i) {
        Square s; s.id = i; float sz = (coreCount > 12) ? 65.f : 95.f;
        s.shape.setSize({sz, sz}); s.shape.setOrigin({sz/2.f, sz/2.f});
        s.shape.setPosition({120.f + (i % 8) * (W / 9.f), 150.f + (i / 8) * 160.f});
        s.velocity = {(float)(std::rand() % 5 + 3), (float)(std::rand() % 5 + 3)};
        coreSquares.push_back(s);
    }
    totalSquare.shape.setSize({280.f, 280.f}); totalSquare.shape.setOrigin({140.f, 140.f});
    totalSquare.shape.setPosition({W/2.f, H/2.f}); totalSquare.velocity = {4.f, 4.f};

    sf::Clock cpuClock; sf::View defaultView = window.getDefaultView();

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();
            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->code == sf::Keyboard::Key::Num1) viewMode = 1;
                if (keyPressed->code == sf::Keyboard::Key::Num2) viewMode = 2;
                if (keyPressed->code == sf::Keyboard::Key::Num3) dataSource = (dataSource == 1) ? 2 : 1;
            }
        }

        if (cpuClock.getElapsedTime().asMilliseconds() > 250) {
            pkgTemp = getPackageTemp();
            for (auto& s : coreSquares) {
                s.currentLoad = getLoad(s.id, s.lastState); s.currentMHz = getCoreMHz(s.id); s.currentTemp = getCoreTemp(s.id);
            }
            totalSquare.currentLoad = getLoad(-1, globalLast); totalSquare.currentTemp = pkgTemp;
            cpuClock.restart();
        }

        if (screenShake > 0) {
            sf::View v = defaultView; v.move({((std::rand()%20-10)/2.f)*screenShake, ((std::rand()%20-10)/2.f)*screenShake});
            window.setView(v); screenShake *= 0.92f; if (screenShake < 0.1f) { screenShake = 0; window.setView(defaultView); }
        }

        bool isOverheat = (pkgTemp > alertThreshold);
        if (isOverheat) {
            float pulse = std::sin(cpuClock.getElapsedTime().asSeconds() * 8.f) * 40.f + 40.f;
            window.clear(sf::Color(static_cast<std::uint8_t>(pulse), 0, 0));
        } else { window.clear(sf::Color::Black); }

        for (auto& p : particles) {
            p.shape.move(p.velocity); p.lifetime -= 5.f;
            sf::Color c = p.shape.getFillColor(); p.shape.setFillColor(sf::Color(c.r, c.g, c.b, (std::uint8_t)p.lifetime));
            window.draw(p.shape);
        }
        particles.erase(std::remove_if(particles.begin(), particles.end(), [](const Particle& p){ return p.lifetime <= 0; }), particles.end());

        auto updateSquare = [&](Square& s) {
            float f = (dataSource == 2) ? (s.currentTemp - 30.f) / (alertThreshold - 30.f) : (s.currentLoad / 100.f);
            if (f < 0.f) f = 0.f; if (f > 1.f) f = 1.f;
            s.shape.move(s.velocity * (0.35f + f * 4.0f));
            s.shape.rotate(sf::degrees(f * 25.f));

            sf::Color col;
            if (f < 0.2f) { float t = f / 0.2f; col = sf::Color(static_cast<uint8_t>(173 * (1 - t)), 216 + static_cast<uint8_t>(39 * t), 230); }
            else if (f < 0.4f) { float t = (f - 0.2f) / 0.2f; col = sf::Color(0, 255, static_cast<uint8_t>(230 * (1 - t))); }
            else if (f < 0.6f) { float t = (f - 0.4f) / 0.2f; col = sf::Color(static_cast<uint8_t>(255 * t), 255, 0); }
            else if (f < 0.8f) { float t = (f - 0.6f) / 0.2f; col = sf::Color(255, static_cast<uint8_t>(255 - (127 * t)), 0); }
            else { float t = (f - 0.8f) / 0.2f; col = sf::Color(255, static_cast<uint8_t>(128 - (128 * t)), 0); }
            s.shape.setFillColor(col);

            if (s.squashTimer > 0) { s.squashTimer -= 0.04f; float wave = std::sin(s.squashTimer * 12.f) * s.squashTimer * f; s.shape.setScale({1.0f + wave * 0.7f, 1.0f - wave * 0.7f}); }
            else s.shape.setScale({1.f, 1.f});

            sf::Vector2f p = s.shape.getPosition(); float rad = (s.shape.getSize().x * s.shape.getScale().x) / 2.f;
            if (p.x < rad) { s.shape.setPosition({rad, p.y}); s.velocity.x *= -1; s.squashTimer = 1.0f; createSparks(particles, s.shape.getPosition(), col, (f*100.f), (s.id == -1)); }
            if (p.x > W-rad) { s.shape.setPosition({W-rad, p.y}); s.velocity.x *= -1; s.squashTimer = 1.0f; createSparks(particles, s.shape.getPosition(), col, (f*100.f), (s.id == -1)); }
            if (p.y < rad) { s.shape.setPosition({p.x, rad}); s.velocity.y *= -1; s.squashTimer = 1.0f; createSparks(particles, s.shape.getPosition(), col, (f*100.f), (s.id == -1)); }
            if (p.y > H-120-rad) { s.shape.setPosition({p.x, H-120-rad}); s.velocity.y *= -1; s.squashTimer = 1.0f; createSparks(particles, s.shape.getPosition(), col, (f*100.f), (s.id == -1)); }
            if (s.id == -1 && s.squashTimer > 0.9f) screenShake = f * 8.0f;
            return col;
        };

        if (viewMode == 1) {
            for (size_t i = 0; i < coreSquares.size(); ++i) {
                updateSquare(coreSquares[i]);
                for (size_t j = i + 1; j < coreSquares.size(); ++j) {
                    if (coreSquares[i].shape.getGlobalBounds().findIntersection(coreSquares[j].shape.getGlobalBounds())) {
                        std::swap(coreSquares[i].velocity, coreSquares[j].velocity);
                        float totalE = (coreSquares[i].currentLoad + coreSquares[j].currentLoad) / 200.f;
                        float power = 1.2f + totalE * 8.f;
                        coreSquares[i].shape.move(coreSquares[i].velocity * power); coreSquares[j].shape.move(coreSquares[j].velocity * power);
                        coreSquares[i].squashTimer = 1.0f; coreSquares[j].squashTimer = 1.0f;
                        createSparks(particles, coreSquares[i].shape.getPosition(), sf::Color::White, 50, false);
                    }
                }
                window.draw(coreSquares[i].shape);
                std::stringstream ss;
                if (dataSource == 2) ss << (int)coreSquares[i].currentTemp << "C\n" << (int)coreSquares[i].currentMHz << "M";
                else ss << (int)coreSquares[i].currentLoad << "%\n" << (int)coreSquares[i].currentMHz << "M";
                sf::Text t(font, ss.str(), (coreCount > 12 ? 10 : 14));
                t.setOrigin({t.getLocalBounds().size.x / 2.f, t.getLocalBounds().size.y / 2.f + 5.f});
                t.setPosition(coreSquares[i].shape.getPosition()); t.setFillColor(sf::Color::Black); window.draw(t);
            }
        } else {
            updateSquare(totalSquare); window.draw(totalSquare.shape);
            float avgM = 0; for(int i=0; i<coreCount; ++i) avgM += getCoreMHz(i);
            std::stringstream ss;
            if (dataSource == 2) ss << " MODE: TEMP\n  " << std::fixed << std::setprecision(1) << pkgTemp << "C\n " << (int)(avgM/coreCount) << "MHz\n  " << (int)totalSquare.currentLoad << "%";
            else ss << " MODE: LOAD\n  " << (int)totalSquare.currentLoad << "%\n " << (int)(avgM/coreCount) << "MHz\n  " << std::fixed << std::setprecision(1) << pkgTemp << "C";
            sf::Text t(font, ss.str(), 32); t.setOrigin({t.getLocalBounds().size.x / 2.f, t.getLocalBounds().size.y / 2.f + 5.f});
            t.setPosition(totalSquare.shape.getPosition()); t.setFillColor(sf::Color::Black); window.draw(t);
        }

        // WIELKI NAPIS ALARMOWY
        if (isOverheat) {
            sf::Text panic(font, "!!! ALARM: SYSTEM OVERHEAT !!!", 50);
            panic.setFillColor(sf::Color::White);
            sf::FloatRect b = panic.getLocalBounds();
            panic.setOrigin({b.size.x/2.f, b.size.y/2.f});
            panic.setPosition({W/2.f, H/2.f - 50.f});
            // Efekt mrugania napisu
            if ((int)(cpuClock.getElapsedTime().asMilliseconds() / 300) % 2 == 0) window.draw(panic);
        }

        sf::Text info(font, "MODE: " + std::string(dataSource == 1 ? "LOAD" : "TEMP") + " | PKG: " + std::to_string((int)pkgTemp) + "C | LIMIT: " + std::to_string((int)alertThreshold) + "C", 18);
        info.setPosition({30.f, H - 75.f}); info.setFillColor(sf::Color::Cyan); window.draw(info);
        window.display();
    }
    return 0;
}
