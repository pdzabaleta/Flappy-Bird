#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>
#include <ctime>
#include <string>
#include <iostream>

// --- HELPER FUNCTIONS ---

// Calculate distance between two points
float getDistance(sf::Vector2f p1, sf::Vector2f p2) {
    return std::sqrt(std::pow(p1.x - p2.x, 2) + std::pow(p1.y - p2.y, 2));
}

// Calculate the precise hitbox for pipes by trimming transparent areas
sf::FloatRect getPipeHitbox(const sf::Sprite& pipe) {
    sf::FloatRect bounds = pipe.getGlobalBounds();
    
    // Trim values based on sprite transparency
    float sideTrim = 140.0f;
    float gapTrim  = 50.0f;

    // Apply lateral trim
    bounds.left += sideTrim;
    bounds.width -= (sideTrim * 2);

    // Apply vertical trim based on pipe orientation (Top vs Bottom)
    if (pipe.getScale().y < 0) {
        bounds.height -= gapTrim;
    } else {
        bounds.top += gapTrim;
        bounds.height -= gapTrim;
    }
    return bounds;
}

// Check collision between Circular Bird and Rectangular Pipe
bool checkCollision(const sf::Sprite& bird, const sf::Sprite& pipe) {
    // Bird treated as a circle (radius reduced to 70% for forgiveness)
    float birdRadius = (bird.getGlobalBounds().width / 2.0f) * 0.7f;
    sf::Vector2f birdCenter = bird.getPosition();
    
    // Pipe treated as a trimmed rectangle
    sf::FloatRect pipeRect = getPipeHitbox(pipe);

    // Find the closest point on the rectangle to the circle's center
    float closestX = std::max(pipeRect.left, std::min(birdCenter.x, pipeRect.left + pipeRect.width));
    float closestY = std::max(pipeRect.top, std::min(birdCenter.y, pipeRect.top + pipeRect.height));

    // Calculate distance to that point
    float distance = getDistance(birdCenter, sf::Vector2f(closestX, closestY));
    
    return distance < birdRadius;
}

int main() {
    // Window Setup
    sf::RenderWindow window(sf::VideoMode(400, 600), "Flappy Bird - Final");
    window.setFramerateLimit(60);
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    // --- LOAD RESOURCES ---
    sf::Texture birdTex, pipeTex;
    sf::Font font;

    if (!birdTex.loadFromFile("bird.png") ||
        !pipeTex.loadFromFile("pipe.png") ||
        !font.loadFromFile("font.ttf")) {
        std::cerr << "Error: Could not load resources (bird.png, pipe.png, or font.ttf)" << std::endl;
        return -1;
    }

    // Bird Setup
    sf::Sprite bird(birdTex);
    bird.setScale(0.12f, 0.12f);
    bird.setOrigin(bird.getLocalBounds().width / 2, bird.getLocalBounds().height / 2);
    bird.setPosition(70, 300);

    // Game Variables
    float velocity = 0;
    float gravity = 0.6f;
    float jumpHeight = -8.5f;
    bool gameStarted = false;
    bool isGameOver = false;
    
    std::vector<sf::Sprite> pipes;
    sf::Clock pipeSpawnClock;

    // Score Variables
    int score = 0;
    int highScore = 0;
    bool newRecordSet = false;

    // --- UI SETUP ---
    
    // Live Score
    sf::Text scoreText;
    scoreText.setFont(font);
    scoreText.setCharacterSize(50);
    scoreText.setFillColor(sf::Color::White);
    scoreText.setOutlineColor(sf::Color::Black);
    scoreText.setOutlineThickness(3);
    scoreText.setPosition(180, 50);

    // Game Over Title
    sf::Text gameOverText("GAME OVER", font, 50);
    gameOverText.setFillColor(sf::Color::Red);
    gameOverText.setOutlineColor(sf::Color::Black);
    gameOverText.setOutlineThickness(3);
    sf::FloatRect textRect = gameOverText.getLocalBounds();
    gameOverText.setOrigin(textRect.left + textRect.width/2.0f, textRect.top + textRect.height/2.0f);
    gameOverText.setPosition(200, 150);

    // Final Score Display
    sf::Text finalScoreText("", font, 30);
    finalScoreText.setFillColor(sf::Color::White);
    finalScoreText.setOutlineColor(sf::Color::Black);
    finalScoreText.setOutlineThickness(2);

    // Restart Prompt
    sf::Text restartText("Press SPACE\nto Restart", font, 25);
    restartText.setFillColor(sf::Color::Yellow);
    restartText.setOutlineColor(sf::Color::Black);
    restartText.setOutlineThickness(2);
    sf::FloatRect restartRect = restartText.getLocalBounds();
    restartText.setOrigin(restartRect.left + restartRect.width/2.0f, restartRect.top + restartRect.height/2.0f);
    restartText.setPosition(200, 450);

    // --- GAME LOOP ---
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();
            
            // Handle Jump and Restart
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Space) {
                if (isGameOver) {
                    // Reset Game State
                    isGameOver = false;
                    gameStarted = false;
                    bird.setPosition(70, 300);
                    bird.setRotation(0);
                    pipes.clear();
                    velocity = 0;
                    score = 0;
                    newRecordSet = false;
                } else {
                    gameStarted = true;
                    velocity = jumpHeight;
                }
            }
        }

        // --- UPDATE LOGIC ---
        if (gameStarted && !isGameOver) {
            // Bird Physics
            velocity += gravity;
            bird.move(0, velocity);
            bird.setRotation(velocity * 3.0f);

            // Screen Boundaries Check
            if (bird.getPosition().y < 0 || bird.getPosition().y > 600) isGameOver = true;

            // Spawn Pipes
            if (pipeSpawnClock.getElapsedTime().asSeconds() > 1.6f) {
                float gapY = std::rand() % 250 + 150;
                
                // Top Pipe (Inverted)
                sf::Sprite topP(pipeTex);
                topP.setScale(0.8f, -1.8f);
                topP.setPosition(450, gapY);
                pipes.push_back(topP);

                // Bottom Pipe
                sf::Sprite botP(pipeTex);
                botP.setScale(0.8f, 1.8f);
                botP.setPosition(450, gapY + 160.0f);
                pipes.push_back(botP);
                
                pipeSpawnClock.restart();
            }

            // Move and Update Pipes
            for (size_t i = 0; i < pipes.size(); ) {
                pipes[i].move(-3.5f, 0);

                // Collision Detection
                if (checkCollision(bird, pipes[i])) isGameOver = true;

                // Score Update (Check center crossing)
                if (pipes[i].getScale().y > 0) { // Check only bottom pipes
                    float pipeCenter = pipes[i].getPosition().x + (pipes[i].getGlobalBounds().width / 2);
                    if (pipeCenter < 70 && pipeCenter > 70 - 3.5f) {
                        score++;
                    }
                }

                // Remove off-screen pipes
                if (pipes[i].getGlobalBounds().left + pipes[i].getGlobalBounds().width < 0) {
                    pipes.erase(pipes.begin() + i);
                } else { i++; }
            }
            
            scoreText.setString(std::to_string(score));
        }

        // --- GAME OVER LOGIC ---
        if (isGameOver) {
            if (score > highScore) {
                highScore = score;
                newRecordSet = true;
            }
            
            std::string finalText = "Score: " + std::to_string(score) + "\nBest: " + std::to_string(highScore);
            if(newRecordSet) finalText += "\n!NEW RECORD!";
            
            finalScoreText.setString(finalText);
            
            // Center the final score text
            sf::FloatRect fRect = finalScoreText.getLocalBounds();
            finalScoreText.setOrigin(fRect.left + fRect.width/2.0f, fRect.top + fRect.height/2.0f);
            finalScoreText.setPosition(200, 280);
        }

        // --- RENDER ---
        window.clear(sf::Color(135, 206, 235)); // Sky Blue Background
        
        for (const auto& p : pipes) window.draw(p);
        window.draw(bird);

        if (isGameOver) {
            // Dark Overlay
            sf::RectangleShape overlay(sf::Vector2f(400, 600));
            overlay.setFillColor(sf::Color(0, 0, 0, 150));
            window.draw(overlay);

            // UI Elements
            window.draw(gameOverText);
            window.draw(finalScoreText);
            window.draw(restartText);
        } else {
            window.draw(scoreText);
        }

        window.display();
    }
    return 0;
}
