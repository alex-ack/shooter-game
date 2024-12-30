#include "/opt/homebrew/Cellar/sfml@2/2.6.2/include/SFML/Graphics.hpp"
#include <vector>
#include <memory>
#include <cmath>
#include <random>
#include <list>

// Forward declarations
enum class PowerUpType {
    SpreadShot,
    RapidFire,
    Shield
};

enum class EnemyType {
    Basic,
    Scout,
    Tank,
    Zigzag
};

// Particle System
class Particle {
public:
    sf::Vector2f position;
    sf::Vector2f velocity;
    float lifetime;
    float maxLifetime;
    sf::Color color;
    float size;

    Particle(const sf::Vector2f& pos, const sf::Vector2f& vel, float life, const sf::Color& col, float sz)
        : position(pos), velocity(vel), lifetime(life), maxLifetime(life), color(col), size(sz) {}

    bool update(float deltaTime) {
        position += velocity * deltaTime;
        lifetime -= deltaTime;
        
        // Fade out
        float alpha = (lifetime / maxLifetime) * 255;
        color.a = static_cast<sf::Uint8>(alpha);
        
        return lifetime > 0;
    }
};

class ParticleSystem {
private:
    std::list<Particle> particles;
    std::mt19937& rng;

public:
    ParticleSystem(std::mt19937& rngEngine) : rng(rngEngine) {}

    void addExplosion(const sf::Vector2f& position, const sf::Color& color) {
        std::uniform_real_distribution<float> angleDist(0, 2 * M_PI);
        std::uniform_real_distribution<float> speedDist(50.f, 200.f);
        std::uniform_real_distribution<float> lifeDist(0.5f, 1.0f);
        
        for (int i = 0; i < 20; ++i) {
            float angle = angleDist(rng);
            float speed = speedDist(rng);
            sf::Vector2f velocity(std::cos(angle) * speed, std::sin(angle) * speed);
            particles.emplace_back(position, velocity, lifeDist(rng), color, 2.f);
        }
    }

    void addEngineTrail(const sf::Vector2f& position) {
        std::uniform_real_distribution<float> spreadDist(-10.f, 10.f);
        std::uniform_real_distribution<float> lifeDist(0.2f, 0.4f);
        
        sf::Vector2f trailPos = position;
        trailPos.x += spreadDist(rng);
        sf::Vector2f velocity(0.f, 50.f);
        particles.emplace_back(trailPos, velocity, lifeDist(rng), 
                             sf::Color(255, 150, 50, 255), 1.5f);
    }

    void update(float deltaTime) {
        particles.remove_if([deltaTime](Particle& p) { return !p.update(deltaTime); });
    }

    void draw(sf::RenderWindow& window) {
        sf::CircleShape shape;
        for (const auto& particle : particles) {
            shape.setPosition(particle.position);
            shape.setRadius(particle.size);
            shape.setFillColor(particle.color);
            window.draw(shape);
        }
    }
};

// Star class for background
class Star {
private:
    sf::CircleShape shape;
    float speed;
    float twinkleTimer;
    float twinkleInterval;
    sf::Color baseColor;

public:
    Star(float x, float y, float size, float spd) : speed(spd), twinkleTimer(0.f) {
        shape.setPosition(x, y);
        shape.setRadius(size);
        
        // Random twinkle interval between 0.5 and 2 seconds
        twinkleInterval = 0.5f + (static_cast<float>(rand()) / RAND_MAX) * 1.5f;
        
        // Randomly choose between white, light blue, and light yellow
        int colorChoice = rand() % 3;
        switch(colorChoice) {
            case 0:
                baseColor = sf::Color(255, 255, 255);  // White
                break;
            case 1:
                baseColor = sf::Color(200, 200, 255);  // Light blue
                break;
            case 2:
                baseColor = sf::Color(255, 255, 200);  // Light yellow
                break;
        }
        shape.setFillColor(baseColor);
    }

    void update(float deltaTime) {
        // Move star
        float newY = shape.getPosition().y + speed * deltaTime;
        if (newY > 600.f) {
            newY = -5.f;
        }
        shape.setPosition(shape.getPosition().x, newY);
        
        // Twinkle effect
        twinkleTimer += deltaTime;
        if (twinkleTimer >= twinkleInterval) {
            twinkleTimer = 0;
            // Randomly adjust brightness
            float brightness = 0.7f + (static_cast<float>(rand()) / RAND_MAX) * 0.3f;
            sf::Color twinkleColor = baseColor;
            twinkleColor.r = static_cast<sf::Uint8>(twinkleColor.r * brightness);
            twinkleColor.g = static_cast<sf::Uint8>(twinkleColor.g * brightness);
            twinkleColor.b = static_cast<sf::Uint8>(twinkleColor.b * brightness);
            shape.setFillColor(twinkleColor);
        }
    }

    void draw(sf::RenderWindow& window) {
        window.draw(shape);
    }
};

class GameObject {
protected:
    sf::Vector2f position;
    sf::Vector2f velocity;
    std::unique_ptr<sf::Sprite> sprite;
    float speed;
    sf::Texture texture;

public:
    GameObject(const sf::Vector2f& pos, float spd) 
        : position(pos), speed(spd), velocity(0.f, 0.f) {}
    
    virtual void update(float deltaTime) = 0;
    
    virtual void draw(sf::RenderWindow& window) {
        if (sprite) {
            sprite->setPosition(position);
            window.draw(*sprite);
        }
    }
    
    virtual ~GameObject() = default;
    
    sf::Vector2f getPosition() const { return position; }
    sf::FloatRect getBounds() const { return sprite->getGlobalBounds(); }
    bool isColliding(const GameObject& other) const {
        return getBounds().intersects(other.getBounds());
    }
    void setVelocity(const sf::Vector2f& vel) { velocity = vel; }
};

class Bullet : public GameObject {
public:
    Bullet(const sf::Vector2f& pos, float spd) : GameObject(pos, spd) {
        texture.loadFromFile("bullet.png");
        sprite = std::make_unique<sf::Sprite>(texture);
        sprite->setScale(0.8f, 0.8f);
        velocity = sf::Vector2f(0.f, -speed);
    }

    void update(float deltaTime) override {
        position += velocity * deltaTime;
    }

    bool isOffScreen() const {
        return position.y < -50.f;
    }
};

class PowerUp : public GameObject {
private:
    PowerUpType type;
    float rotationSpeed;
    float angle;

public:
    PowerUp(const sf::Vector2f& pos, PowerUpType t) 
        : GameObject(pos, 100.f), type(t), rotationSpeed(90.f), angle(0.f) {
        
        texture.loadFromFile("powerup.png");
        sprite = std::make_unique<sf::Sprite>(texture);
        sprite->setScale(0.6f, 0.6f);
        
        // Color based on type
        switch(type) {
            case PowerUpType::SpreadShot:
                sprite->setColor(sf::Color::Yellow);
                break;
            case PowerUpType::RapidFire:
                sprite->setColor(sf::Color::Red);
                break;
            case PowerUpType::Shield:
                sprite->setColor(sf::Color::Blue);
                break;
        }
        
        velocity = sf::Vector2f(0.f, speed);
    }

    void update(float deltaTime) override {
        position += velocity * deltaTime;
        
        // Rotate the powerup
        angle += rotationSpeed * deltaTime;
        sprite->setRotation(angle);
    }

    PowerUpType getType() const { return type; }
    bool isOffScreen() const { return position.y > 650.f; }
};

class Player : public GameObject {
private:
    std::vector<std::unique_ptr<Bullet>> bullets;
    float shootCooldown = 0.2f;
    float currentCooldown = 0.f;
    int lives;
    PowerUpType activePowerUp = PowerUpType::SpreadShot;
    float powerUpTimer = 0.f;
    float powerUpDuration = 10.f;
    bool hasPowerUp = false;
    float invincibilityTimer = 0.f;
    bool isInvincible = false;

public:
    Player(const sf::Vector2f& pos, float spd) : GameObject(pos, spd), lives(3) {
        texture.loadFromFile("player.png");
        sprite = std::make_unique<sf::Sprite>(texture);
        sprite->setScale(0.8f, 0.8f);
    }

    void update(float deltaTime) override {
        // Update power-up timer
        if (hasPowerUp) {
            powerUpTimer -= deltaTime;
            if (powerUpTimer <= 0) {
                hasPowerUp = false;
                shootCooldown = 0.2f;  // Reset to default
            }
        }

        // Update invincibility
        if (isInvincible) {
            invincibilityTimer -= deltaTime;
            if (invincibilityTimer <= 0) {
                isInvincible = false;
                sprite->setColor(sf::Color::White);
            }
            // Make ship blink while invincible
            sprite->setColor(sf::Color(255, 255, 255, 
                static_cast<sf::Uint8>(std::abs(std::sin(invincibilityTimer * 10)) * 255)));
        }

        // Handle keyboard input
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
            velocity.x = -speed;
        }
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
            velocity.x = speed;
        }
        else {
            velocity.x = 0;
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
            velocity.y = -speed;
        }
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
            velocity.y = speed;
        }
        else {
            velocity.y = 0;
        }

        // Keep player in bounds
        position += velocity * deltaTime;
        position.x = std::max(0.f, std::min(position.x, 800.f - sprite->getGlobalBounds().width));
        position.y = std::max(0.f, std::min(position.y, 600.f - sprite->getGlobalBounds().height));

        // Handle shooting
        currentCooldown -= deltaTime;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && currentCooldown <= 0) {
            shoot();
            currentCooldown = shootCooldown;
        }

        // Update bullets
        for (auto it = bullets.begin(); it != bullets.end();) {
            (*it)->update(deltaTime);
            if ((*it)->isOffScreen()) {
                it = bullets.erase(it);
            } else {
                ++it;
            }
        }
    }

    void draw(sf::RenderWindow& window) override {
        GameObject::draw(window);
        for (const auto& bullet : bullets) {
            bullet->draw(window);
        }
    }

    void shoot() {
        sf::Vector2f bulletPos = position + sf::Vector2f(sprite->getGlobalBounds().width / 2, 0);
        
        if (hasPowerUp && activePowerUp == PowerUpType::SpreadShot) {
            // Create 3 bullets in a spread pattern
            bullets.push_back(std::make_unique<Bullet>(bulletPos, 500.f));
            bullets.back()->setVelocity(sf::Vector2f(-100.f, -500.f));
            
            bullets.push_back(std::make_unique<Bullet>(bulletPos, 500.f));
            bullets.back()->setVelocity(sf::Vector2f(0.f, -500.f));
            
            bullets.push_back(std::make_unique<Bullet>(bulletPos, 500.f));
            bullets.back()->setVelocity(sf::Vector2f(100.f, -500.f));
        } else {
            bullets.push_back(std::make_unique<Bullet>(bulletPos, 500.f));
        }
    }

    void activatePowerUp(PowerUpType type) {
        hasPowerUp = true;
        activePowerUp = type;
        powerUpTimer = powerUpDuration;
        
        switch(type) {
            case PowerUpType::SpreadShot:
                // Handled in shoot()
                break;
            case PowerUpType::RapidFire:
                shootCooldown = 0.1f;
                break;
            case PowerUpType::Shield:
                isInvincible = true;
                invincibilityTimer = powerUpDuration;
                break;
        }
    }

    const std::vector<std::unique_ptr<Bullet>>& getBullets() const {
        return bullets;
    }

    int getLives() const { return lives; }
    
    void loseLife() {
        if (!isInvincible) {
            lives--;
            // Temporary invincibility after getting hit
            isInvincible = true;
            invincibilityTimer = 2.0f;
        }
    }
    
    bool isAlive() const { return lives > 0; }
};

class Enemy : public GameObject {
protected:
    EnemyType type;
    float healthPoints;
    float zigzagTimer = 0.f;
    float zigzagFrequency = 2.f;
    float originalX;

public:
    Enemy(const sf::Vector2f& pos, float spd, EnemyType t) 
        : GameObject(pos, spd), type(t), originalX(pos.x) {
        texture.loadFromFile("enemy.png");
        sprite = std::make_unique<sf::Sprite>(texture);
        
        switch(type) {
            case EnemyType::Basic:
                healthPoints = 1.f;
                sprite->setScale(0.8f, 0.8f);
                velocity = sf::Vector2f(0.f, speed);
                break;
            case EnemyType::Scout:
                healthPoints = 1.f;
                sprite->setScale(0.6f, 0.6f);
                speed *= 1.5f;
                velocity = sf::Vector2f(0.f, speed);
                sprite->setColor(sf::Color(150, 255, 150));  // Light green
                break;
            case EnemyType::Tank:
                healthPoints = 3.f;
                sprite->setScale(1.0f, 1.0f);
                speed *= 0.7f;
                velocity = sf::Vector2f(0.f, speed);
                sprite->setColor(sf::Color(255, 150, 150));  // Light red
                break;
            case EnemyType::Zigzag:
                healthPoints = 1.f;
                sprite->setScale(0.8f, 0.8f);
                velocity = sf::Vector2f(0.f, speed);
                sprite->setColor(sf::Color(150, 150, 255));  // Light blue
                break;
        }
    }

    void update(float deltaTime) override {
        if (type == EnemyType::Zigzag) {
            zigzagTimer += deltaTime;
            float xOffset = std::sin(zigzagTimer * zigzagFrequency) * 100.f;
            position.x = originalX + xOffset;
            position.y += velocity.y * deltaTime;
        } else {
            position += velocity * deltaTime;
        }
    }

    bool hit() {
        healthPoints--;
        return healthPoints <= 0;
    }

    int getScoreValue() const {
        switch(type) {
            case EnemyType::Scout: return 150;
            case EnemyType::Tank: return 200;
            case EnemyType::Zigzag: return 175;
            default: return 100;
        }
    }

    bool isOffScreen() const {
        return position.y > 650.f;
    }
};

class Game {
private:
    sf::RenderWindow window;
    std::unique_ptr<Player> player;
    std::vector<std::unique_ptr<Enemy>> enemies;
    std::vector<std::unique_ptr<PowerUp>> powerUps;
    std::vector<Star> stars;
    ParticleSystem particles;
    sf::Clock clock;
    float enemySpawnTimer = 0.f;
    float enemySpawnInterval = 1.5f;
    float powerUpSpawnTimer = 0.f;
    float powerUpSpawnInterval = 15.f;
    std::mt19937 rng{std::random_device{}()};
    int score = 0;
    int wave = 1;
    sf::Font font;
    sf::Text scoreText;
    sf::Text livesText;
    sf::Text waveText;
    float screenShakeTime = 0.f;
    sf::Vector2f screenShakeOffset;

    void initStars() {
        std::uniform_real_distribution<float> xDist(0.f, 800.f);
        std::uniform_real_distribution<float> yDist(0.f, 600.f);
        std::uniform_real_distribution<float> speedDist(30.f, 120.f);
        
        // Create three layers of stars
        for (int i = 0; i < 80; ++i) {
            stars.emplace_back(xDist(rng), yDist(rng), 1.f, speedDist(rng) * 0.5f);
        }
        for (int i = 0; i < 40; ++i) {
            stars.emplace_back(xDist(rng), yDist(rng), 2.f, speedDist(rng));
        }
        for (int i = 0; i < 15; ++i) {
            stars.emplace_back(xDist(rng), yDist(rng), 3.f, speedDist(rng) * 1.5f);
        }
    } 
    void addScreenShake(float duration = 0.2f, float intensity = 5.f) {
        screenShakeTime = duration;
        std::uniform_real_distribution<float> shakeDist(-intensity, intensity);
        screenShakeOffset = sf::Vector2f(shakeDist(rng), shakeDist(rng));
    }
    
    void updateScreenShake(float deltaTime) {
        if (screenShakeTime > 0) {
            screenShakeTime -= deltaTime;
            if (screenShakeTime <= 0) {
                screenShakeOffset = sf::Vector2f(0, 0);
            } else {
                std::uniform_real_distribution<float> shakeDist(-5.f, 5.f);
                screenShakeOffset = sf::Vector2f(shakeDist(rng), shakeDist(rng));
            }
        }
    }
    
public:
    Game() : window(sf::VideoMode(800, 600), "Space Shooter"), particles(rng) {
        window.setFramerateLimit(60);
        player = std::make_unique<Player>(sf::Vector2f(400.f, 500.f), 300.f);
        
        if (!font.loadFromFile("/System/Library/Fonts/Supplemental/Arial.ttf")) {
            font.loadFromFile("/System/Library/Fonts/Helvetica.ttc");
        }
        
        scoreText.setFont(font);
        scoreText.setCharacterSize(24);
        scoreText.setFillColor(sf::Color::White);
        scoreText.setPosition(10, 10);
        
        livesText.setFont(font);
        livesText.setCharacterSize(24);
        livesText.setFillColor(sf::Color::White);
        livesText.setPosition(10, 40);
        
        waveText.setFont(font);
        waveText.setCharacterSize(24);
        waveText.setFillColor(sf::Color::White);
        waveText.setPosition(10, 70);
        
        initStars();
        updateHUD();
    }
    
    void run() {
        while (window.isOpen()) {
            handleEvents();
            if (player->isAlive()) {
                update();
            }
            render();
        }
    }
    
private:
    void handleEvents() {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }
    }

    void spawnPowerUp() {
        std::uniform_real_distribution<float> xDist(50.f, 750.f);
        std::uniform_int_distribution<int> typeDist(0, 2);
        
        PowerUpType type;
        switch(typeDist(rng)) {
            case 0: type = PowerUpType::SpreadShot; break;
            case 1: type = PowerUpType::RapidFire; break;
            default: type = PowerUpType::Shield; break;
        }
        
        powerUps.push_back(std::make_unique<PowerUp>(
            sf::Vector2f(xDist(rng), -50.f), type));
    }
    
    void spawnEnemy() {
        std::uniform_real_distribution<float> xDist(50.f, 750.f);
        std::uniform_int_distribution<int> typeDist(0, 3);
        float x = xDist(rng);
        
        EnemyType type;
        // As wave increases, increase chance of special enemies
        if (wave > 3) {
            typeDist = std::uniform_int_distribution<int>(0, 6);
            switch(typeDist(rng)) {
                case 0: 
                case 1: type = EnemyType::Basic; break;
                case 2:
                case 3: type = EnemyType::Scout; break;
                case 4:
                case 5: type = EnemyType::Zigzag; break;
                case 6: type = EnemyType::Tank; break;
            }
        } else {
            switch(typeDist(rng)) {
                case 0: type = EnemyType::Scout; break;
                case 1: type = EnemyType::Tank; break;
                case 2: type = EnemyType::Zigzag; break;
                default: type = EnemyType::Basic; break;
            }
        }
        
        enemies.push_back(std::make_unique<Enemy>(
            sf::Vector2f(x, -50.f), 150.f, type));
    }
    void update() {
        float deltaTime = clock.restart().asSeconds();
        
        // Update screen shake
        updateScreenShake(deltaTime);
        
        // Update particles
        particles.update(deltaTime);
        
        // Add engine trail
        particles.addEngineTrail(player->getPosition() + sf::Vector2f(
            player->getBounds().width / 2,
            player->getBounds().height));
        
        // Update stars
        for (auto& star : stars) {
            star.update(deltaTime);
        }
        
        // Update player
        player->update(deltaTime);
        
        // Spawn enemies
        enemySpawnTimer += deltaTime;
        if (enemySpawnTimer >= enemySpawnInterval) {
            spawnEnemy();
            enemySpawnTimer = 0;
            
            // Gradually decrease spawn interval with waves
            enemySpawnInterval = std::max(0.5f, 1.5f - (wave - 1) * 0.1f);
        }
        
        // Spawn power-ups
        powerUpSpawnTimer += deltaTime;
        if (powerUpSpawnTimer >= powerUpSpawnInterval) {
            spawnPowerUp();
            powerUpSpawnTimer = 0;
        }
        
        // Update enemies
        for (auto it = enemies.begin(); it != enemies.end();) {
            (*it)->update(deltaTime);
            if ((*it)->isOffScreen()) {
                player->loseLife();
                updateHUD();
                it = enemies.erase(it);
            } else {
                ++it;
            }
        }
        
        // Update power-ups
        for (auto it = powerUps.begin(); it != powerUps.end();) {
            (*it)->update(deltaTime);
            if ((*it)->isOffScreen()) {
                it = powerUps.erase(it);
            } else {
                ++it;
            }
        }
        
        // Check collisions
        checkCollisions();
        
        // Check for wave advancement
        if (score >= wave * 1000) {
            wave++;
            updateHUD();
        }
    }
    
    void render() {
        window.clear(sf::Color(0, 0, 20));
        
        // Apply screen shake
        sf::View view = window.getDefaultView();
        view.move(screenShakeOffset);
        window.setView(view);
        
        // Draw stars
        for (auto& star : stars) {
            star.draw(window);
        }
        
        // Draw particles
        particles.draw(window);
        
        // Draw game objects
        player->draw(window);
        for (const auto& enemy : enemies) {
            enemy->draw(window);
        }
        for (const auto& powerUp : powerUps) {
            powerUp->draw(window);
        }
        
        // Reset view for HUD
        window.setView(window.getDefaultView());
        
        // Draw HUD
        window.draw(scoreText);
        window.draw(livesText);
        window.draw(waveText);
        
        // Draw game over message if player is dead
        if (!player->isAlive()) {
            sf::Text gameOverText;
            gameOverText.setFont(font);
            gameOverText.setString("GAME OVER\nFinal Score: " + std::to_string(score) +
                                 "\nWaves Survived: " + std::to_string(wave));
            gameOverText.setCharacterSize(48);
            gameOverText.setFillColor(sf::Color::Red);
            
            sf::FloatRect textBounds = gameOverText.getLocalBounds();
            gameOverText.setPosition(
                (800 - textBounds.width) / 2,
                (600 - textBounds.height) / 2
            );
            
            window.draw(gameOverText);
        }
        
        window.display();
    }
    
    void checkCollisions() {
        // Check bullet-enemy collisions
        const auto& bullets = player->getBullets();
        for (auto enemyIt = enemies.begin(); enemyIt != enemies.end();) {
            bool enemyDestroyed = false;
            for (const auto& bullet : bullets) {
                if ((*enemyIt)->isColliding(*bullet)) {
                    if ((*enemyIt)->hit()) {  // Returns true if enemy is destroyed
                        // Add explosion particles
                        particles.addExplosion((*enemyIt)->getPosition(),
                            sf::Color(255, 200, 100));
                        
                        // Add screen shake
                        addScreenShake();
                        
                        // Add score
                        score += (*enemyIt)->getScoreValue();
                        updateHUD();
                        enemyDestroyed = true;
                    }
                    break;
                }
            }
            if (enemyDestroyed) {
                enemyIt = enemies.erase(enemyIt);
            } else {
                ++enemyIt;
            }
        }
        
        // Check player-powerup collisions
        for (auto it = powerUps.begin(); it != powerUps.end();) {
            if ((*it)->isColliding(*player)) {
                player->activatePowerUp((*it)->getType());
                it = powerUps.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    void updateHUD() {
        scoreText.setString("Score: " + std::to_string(score));
        livesText.setString("Lives: " + std::to_string(player->getLives()));
        waveText.setString("Wave: " + std::to_string(wave));
    }
};

int main() {
    Game game;
    game.run();
    return 0;
}
// ./SpaceShooter