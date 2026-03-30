#include <SFML/Graphics.hpp>
#include <vector>
#include <fstream>
#include <string>
#include <sstream>
#include <optional>
#include <cmath>
#include <iomanip>
#include <filesystem>

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
    int count = static_cast<int>((5 + intensity * 0.3f) * (isMega ? 5 : 1));
    for (int i = 0; i < count; ++i) {
        Particle p;
        float size = (std::rand() % 10 > 7) ? 7.f : 2.5f;
        p.shape.setSize({size, size});
        p.shape.setPosition(pos);
        p.shape.setFillColor(col);
        float speedScale = isMega ? 0.7f : 0.35f;
        p.velocity = {((std::rand() % 200) - 100) * speedScale, ((std::rand() % 200) - 100) * speedScale};
        particles.push_back(p);
    }
}

int main() {
    const unsigned int W = 1100, H = 850;
    sf::RenderWindow window(sf::VideoMode({W, H}), "GAMON 1.3 - Kinetic CPU Monitor");
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

    for (int i = 0; i < 6; ++i) {
        Square s; s.id = i;
        s.shape.setSize({95.f, 95.f}); s.shape.setOrigin({47.5f, 47.5f});
        s.shape.setPosition({static_cast<float>(120 + i * 170), 400.f});
        s.velocity = {(float)(std::rand() % 5 + 3), (float)(std::rand() % 5 + 3)};
        coreSquares.push_back(s);
    }

    totalSquare.shape.setSize({280.f, 280.f}); totalSquare.shape.setOrigin({140.f, 140.f});
    totalSquare.shape.setPosition({550.f, 400.f}); totalSquare.velocity = {5.f, 5.f};

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

        if (cpuClock.getElapsedTime().asMilliseconds() > 300) {
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
            screenShake *= 0.93f;
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
            s.shape.move(s.velocity * (0.35f + f * 3.8f));
            s.shape.rotate(sf::degrees(f * 22.f));

            std::uint8_t r = (std::uint8_t)(f < 0.5f ? (f * 510.f) : 255.f);
            std::uint8_t g = (std::uint8_t)(f < 0.5f ? 255.f : (510.f - f * 510.f));
            sf::Color col(r, g, 0);
            s.shape.setFillColor(col);

            if (s.squashTimer > 0) {
                s.squashTimer -= 0.045f;
                float wave = std::sin(s.squashTimer * 14.f) * s.squashTimer;
                s.shape.setScale({1.0f + wave * 0.55f, 1.0f - wave * 0.55f});
            } else s.shape.setScale({1.f, 1.f});

            sf::Vector2f p = s.shape.getPosition();
            float rad = (s.shape.getSize().x * s.shape.getScale().x) / 2.f;
            bool hit = false;
            if (p.x < 47) { s.shape.setPosition({47.f, p.y}); s.velocity.x *= -1; hit = true; }
            if (p.x > W-47) { s.shape.setPosition({W-47, p.y}); s.velocity.x *= -1; hit = true; }
            if (p.y < 47) { s.shape.setPosition({p.x, 47.f}); s.velocity.y *= -1; hit = true; }
            if (p.y > H-120-47) { s.shape.setPosition({p.x, H-120-47}); s.velocity.y *= -1; hit = true; }

            if (hit) {
                s.squashTimer = 1.0f;
                createSparks(particles, s.shape.getPosition(), col, s.currentLoad, (s.id == -1));
                if (s.id == -1) screenShake = f * 8.0f;
            }
            return col;
        };

        if (mode == 1) {
            for (size_t i = 0; i < coreSquares.size(); ++i) {
                sf::Color col = updateSquare(coreSquares[i]);
                for (size_t j = i + 1; j < coreSquares.size(); ++j) {
                    if (coreSquares[i].shape.getGlobalBounds().findIntersection(coreSquares[j].shape.getGlobalBounds())) {
                        std::swap(coreSquares[i].velocity, coreSquares[j].velocity);
                        coreSquares[i].shape.move(coreSquares[i].velocity * 6.f);
                        coreSquares[i].squashTimer = 1.0f; coreSquares[j].squashTimer = 1.0f;
                        createSparks(particles, coreSquares[i].shape.getPosition(), sf::Color::White, 60, false);
                    }
                }
                window.draw(coreSquares[i].shape);
                std::stringstream ss; ss << (int)coreSquares[i].currentLoad << "%\n" << (int)coreSquares[i].currentMHz << "M";
                sf::Text t(font, ss.str(), 16);
                t.setOrigin({t.getLocalBounds().size.x/2, t.getLocalBounds().size.y/2 + 5});
                t.setPosition(coreSquares[i].shape.getPosition()); t.setFillColor(sf::Color::Black);
                window.draw(t);
            }
        } else {
            updateSquare(totalSquare);
            window.draw(totalSquare.shape);
            float avgM = 0; for(int i=0; i<6; ++i) avgM += getCoreMHz(i);
            std::stringstream ss; ss << " TOTAL CPU\n  " << (int)totalSquare.currentLoad << "%\n " << (int)(avgM/6) << "MHz\n  " << std::fixed << std::setprecision(1) << cpuTemp << "C";
            sf::Text t(font, ss.str(), 34);
            t.setOrigin({t.getLocalBounds().size.x/2, t.getLocalBounds().size.y/2 + 5});
            t.setPosition(totalSquare.shape.getPosition()); t.setFillColor(sf::Color::Black);
            window.draw(t);
        }

        sf::Text info(font, "GAMON 1.3 | MODE: " + std::string(mode == 1 ? "CORES" : "TOTAL") + " | BY KOSZMAR & AI", 18);
                info.setPosition({30.f, H - 75.f}); info.setFillColor(sf::Color::Cyan);
        window.draw(info);
        window.display();
    }
    return 0;
}
