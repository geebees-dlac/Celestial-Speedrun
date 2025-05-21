// main.cpp
#include <iostream>
#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath> 
#include <string>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <limits>
#include <filesystem> 
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include "CollisionSystem.hpp"
#include "Player.hpp"
#include "PlatformBody.hpp"
#include "Tile.hpp"
#include "PhysicsTypes.hpp" // PhysicsTypes.hpp has enum class bodyType
#include "LevelManager.hpp" 

// --- Updated GameState Enum ---
enum class GameState {
    MENU,
    PLAYING,
    LEVEL_TRANSITION, 
    GAME_OVER_WIN,      
    GAME_OVER_LOSE    
};


namespace math { namespace easing {
    float sineEaseInOut(float t, float b, float c, float d) {
        if (d == 0) return b;
        t /= d / 2.0f;
        if (t < 1.0f) return c / 2.0f * t * t * t + b;
        t -= 2.0f; 
        return c / 2.0f * (t * t * t + 2.0f) + b;
    }
}}

// --- Global Variables / Game Context ---
LevelManager levelManager;
LevelData currentLevelData;
std::vector<phys::PlatformBody> bodies; 
std::vector<Tile> tiles;              
phys::DynamicBody playerBody; 

struct ActiveMovingPlatform {
    unsigned int id;
    sf::Vector2f startPosition; 
    char axis;
    float distance;
    float cycleTime; 
    float cycleDuration;
    int direction; 
    sf::Vector2f lastFramePosition; 
};
std::vector<ActiveMovingPlatform> activeMovingPlatforms;

sf::Time vanishingPlatformCycleTimer = sf::Time::Zero;
int oddEvenVanishing = 1;


// --- Function to Setup Level Assets ---
void setupLevelAssets(const LevelData& data, sf::RenderWindow& window) {
    bodies.clear();
    tiles.clear();
    activeMovingPlatforms.clear();

    playerBody.setPosition(data.playerStartPosition);
    playerBody.setVelocity({0.f, 0.f});
    playerBody.setOnGround(false); 
    playerBody.setGroundPlatform(nullptr);
    playerBody.setLastPosition(data.playerStartPosition);

    bodies.reserve(data.platforms.size());
    for (const auto& p_body_template : data.platforms) {
        bodies.push_back(p_body_template);

        if (p_body_template.getType() == phys::bodyType::moving) {
            bool foundDetail = false;
            for(const auto& detail : data.movingPlatformDetails){
                if(detail.id == p_body_template.getID()){
                    activeMovingPlatforms.push_back({
                        detail.id,
                        detail.startPosition, 
                        detail.axis,
                        detail.distance,
                        0.0f, 
                        detail.cycleDuration,
                        detail.initialDirection,
                        detail.startPosition 
                    });
                    foundDetail = true;
                    break;
                }
            }
            if(!foundDetail){
                std::cerr << "Warning: Moving platform ID " << p_body_template.getID() 
                          << " missing movement details in JSON." << std::endl;
            }
        }
    }

    tiles.reserve(bodies.size());
    for (size_t i = 0; i < bodies.size(); ++i) {
        const auto& body = bodies[i];
        Tile newTile(sf::Vector2f(body.getWidth(), body.getHeight()));
        newTile.setPosition(body.getPosition());
        switch (body.getType()) {
            case phys::bodyType::solid:        newTile.setFillColor(sf::Color(100, 100, 100, 255)); break;
            case phys::bodyType::platform:     newTile.setFillColor(sf::Color(70, 150, 200, 180)); break;
            case phys::bodyType::conveyorBelt: newTile.setFillColor(sf::Color(255, 150, 50, 255)); break;
            case phys::bodyType::moving:       newTile.setFillColor(sf::Color(70, 200, 70, 255)); break;
            case phys::bodyType::falling:      newTile.setFillColor(sf::Color(200, 200, 70, 255)); break;
            case phys::bodyType::vanishing:    newTile.setFillColor(sf::Color(200, 70, 200, 255)); break;
            case phys::bodyType::spring:       newTile.setFillColor(sf::Color(255, 255, 0, 255)); break;
            case phys::bodyType::trap:         newTile.setFillColor(sf::Color(255, 0, 0, 255)); break;
            case phys::bodyType::goal:         newTile.setFillColor(sf::Color(0, 255, 0, 100)); break; // Goal sprite color
            case phys::bodyType::none:         newTile.setFillColor(sf::Color(0, 0, 0, 0)); break;
            default:                           newTile.setFillColor(sf::Color(128, 128, 128, 255)); break;
        }
        std::cout << "DOING THIS ";
        std::cout << (int)newTile.getFillColor().r << " "
                    << (int)newTile.getFillColor().g << " "
                    << (int)newTile.getFillColor().b << " "
                    << (int)newTile.getFillColor().a << std::endl;
        tiles.push_back(newTile);
    }
    
    vanishingPlatformCycleTimer = sf::Time::Zero;
    oddEvenVanishing = 1;
}


int main(void) {
    const sf::Vector2f tileSize(32.f, 32.f); 
    const float PLAYER_MOVE_SPEED = 200.f;
    const float JUMP_INITIAL_VELOCITY = -450.f;
    const float GRAVITY_ACCELERATION = 1200.f;
    const float MAX_FALL_SPEED = 700.f;
    const sf::Time MAX_JUMP_HOLD_TIME = sf::seconds(0.18f);

    sf::RenderWindow window(sf::VideoMode({800, 600}), "Project - T", sf::Style::Default);
    window.setKeyRepeatEnabled(false); 
    window.setVerticalSyncEnabled(true);
    
    GameState currentState = GameState::MENU;
    levelManager.setMaxLevels(5); 

    playerBody = phys::DynamicBody(
        {0.f, 0.f}, 
        tileSize.x, tileSize.y, {0.f, 0.f}
    );

    sf::Font menuFont;
    std::string fontPath = "../assets/fonts/ARIALBD.TTF"; 
    if (!menuFont.openFromFile(fontPath)) {
        std::cerr << "Failed to load font: " << fontPath << std::endl;
        fontPath = "ARIALBD.TTF";
        if (!menuFont.openFromFile(fontPath)) {
             std::cerr << "Failed to load font from local: " << fontPath << ". Exiting." << std::endl;
            return -1;
        }
    }
    sf::Text startButtonText(menuFont), settingsButtonText(menuFont), exitButtonText(menuFont), gameStatusText(menuFont);
    auto setupButton = [&](sf::Text& text, const sf::String& str, float yPos, float xPosOffset = 0.f) {
        text.setFont(menuFont);
        text.setString(str);
        text.setCharacterSize(30);
        text.setFillColor(sf::Color::White);
        sf::FloatRect bounds = text.getLocalBounds();
        text.setOrigin({bounds.position.x + bounds.size.x / 2.f, bounds.position.y + bounds.size.y / 2.f});
        text.setPosition({window.getSize().x / 2.f + xPosOffset, yPos});
    };
    setupButton(startButtonText, "Start Game", 250.f);
    setupButton(settingsButtonText, "Settings (NYI)", 300.f);
    setupButton(exitButtonText, "Exit", 350.f);
    setupButton(gameStatusText, "", 200.f); 

    sf::Color defaultButtonColor = sf::Color::White;
    sf::Color hoverButtonColor = sf::Color::Yellow;
    sf::Color clickedButtonColor = sf::Color::Green;
    sf::Color exitButtonColor = sf::Color::Red;

    sf::View mainView(sf::FloatRect({0.f, 0.f}, {800.f, 600.f}));

    sf::RectangleShape playerShape;
    playerShape.setFillColor(sf::Color(220, 220, 250, 255));
    playerShape.setSize(sf::Vector2f(playerBody.getWidth(), playerBody.getHeight()));

    sf::Clock tickClock;
    sf::Time timeSinceLastUpdate = sf::Time::Zero;
    const sf::Time TIME_PER_FRAME = sf::seconds(1.f / 60.f); 

    // bool jumpKeyHeld = false; // This variable was unused in your provided main.cpp
    sf::Time currentJumpHoldDuration = sf::Time::Zero;
    int turboMultiplier = 1;
    bool interactKeyPressedThisFrame = false; 

    sf::Text debugText(menuFont);
    debugText.setFont(menuFont);
    debugText.setCharacterSize(14);
    debugText.setFillColor(sf::Color::White);
    debugText.setPosition({10.f, 10.f});

    bool running = true;
    while (running) {
        interactKeyPressedThisFrame = false; 

        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                running = false; window.close();
            }
            if (currentState == GameState::MENU || currentState == GameState::GAME_OVER_WIN || currentState == GameState::GAME_OVER_LOSE) {
                if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()){
                    if (keyPressed->scancode == sf::Keyboard::Scancode::Escape){
                        running = false; window.close();
                    }
                }
                if (const auto* mouseButtonReleased = event->getIf<sf::Event::MouseButtonReleased>()) {
                    if (mouseButtonReleased->button == sf::Mouse::Button::Left) {
                        sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window), window.getDefaultView());
                        if (startButtonText.getGlobalBounds().contains(mousePos)) {
                            if (levelManager.loadLevel(1, currentLevelData)) { 
                                setupLevelAssets(currentLevelData, window);
                                currentState = GameState::PLAYING;
                                timeSinceLastUpdate = TIME_PER_FRAME; // TEMP FIX 
                                tickClock.restart();
                                window.setTitle("Project T - Level " + std::to_string(currentLevelData.levelNumber));
                            } else {
                                std::cerr << "FATAL: Failed to load level 1. Check JSON and paths." << std::endl;
                                gameStatusText.setString("Error: Could not load level 1!");
                            }
                        } else if (settingsButtonText.getGlobalBounds().contains(mousePos) && currentState == GameState::MENU) {
                            std::cout << "Settings pressed (NYI)" << std::endl;
                        } else if (exitButtonText.getGlobalBounds().contains(mousePos)) {
                            running = false; window.close();
                        }
                    }
                }
            } 
            else if (currentState == GameState::PLAYING) {
                 if (const auto keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                    if (keyPressed->scancode == sf::Keyboard::Scancode::Escape) {
                        currentState = GameState::MENU; 
                        window.setTitle("Project T (Menu - Paused)");
                    } else if (keyPressed->scancode == sf::Keyboard::Scancode::R) {
                        if (levelManager.loadLevel(levelManager.getCurrentLevelNumber(), currentLevelData)) {
                            setupLevelAssets(currentLevelData, window);
                            window.setTitle("Project T - Level " + std::to_string(currentLevelData.levelNumber));
                            tickClock.restart(); 
                            timeSinceLastUpdate = sf::Time::Zero;
                        } else {
                            std::cerr << "Failed to reload current level (" << levelManager.getCurrentLevelNumber() << ")!" << std::endl;
                            currentState = GameState::MENU; 
                        }
                    } else if (keyPressed->scancode == sf::Keyboard::Scancode::E) { 
                        interactKeyPressedThisFrame = true;
                    }
                 }
            }
        }

        if (!running) break;

        if (currentState == GameState::MENU || currentState == GameState::GAME_OVER_WIN || currentState == GameState::GAME_OVER_LOSE) {
            sf::Vector2f mousePosView = window.mapPixelToCoords(sf::Mouse::getPosition(window), window.getDefaultView());
            bool mousePressedDown = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);

            startButtonText.setFillColor(defaultButtonColor);
            settingsButtonText.setFillColor(defaultButtonColor);
            exitButtonText.setFillColor(defaultButtonColor);
            settingsButtonText.setString("Settings (NYI)"); 

            if(currentState == GameState::GAME_OVER_WIN) {
                gameStatusText.setString("You Won! All Levels Cleared!");
                startButtonText.setString("Play Again?");
            } else if (currentState == GameState::GAME_OVER_LOSE) {
                 gameStatusText.setString("Game Over! Try Again?");
                 startButtonText.setString("Try Again?");
            } else { 
                gameStatusText.setString(""); 
                startButtonText.setString("Start Game");
            }

            if (startButtonText.getGlobalBounds().contains(mousePosView)) {
                startButtonText.setFillColor(mousePressedDown ? clickedButtonColor : hoverButtonColor);
            } else if (settingsButtonText.getGlobalBounds().contains(mousePosView) && currentState == GameState::MENU) {
                settingsButtonText.setFillColor(mousePressedDown ? clickedButtonColor : hoverButtonColor);
            } else if (exitButtonText.getGlobalBounds().contains(mousePosView)) {
                exitButtonText.setFillColor(mousePressedDown ? exitButtonColor : hoverButtonColor);
            }

            window.clear(sf::Color(30, 30, 70)); 
            window.setView(window.getDefaultView());
            if (!gameStatusText.getString().isEmpty()) window.draw(gameStatusText);
            window.draw(startButtonText);
            if (currentState == GameState::MENU) window.draw(settingsButtonText);
            window.draw(exitButtonText);
            window.display();
            continue; 
        }
        else if (currentState == GameState::LEVEL_TRANSITION) {
            // Future placeholder
        }
        else if (currentState == GameState::PLAYING) {
            sf::Time elapsedTime = tickClock.restart();
            timeSinceLastUpdate += elapsedTime;

            std::cout << elapsedTime.asSeconds() << " "
            << timeSinceLastUpdate.asSeconds() << " " << TIME_PER_FRAME.asSeconds() << std::endl;

            if (timeSinceLastUpdate < TIME_PER_FRAME) continue;

            /*while (timeSinceLastUpdate < TIME_PER_FRAME){
                std::cout << "LOADING..." << elapsedTime.asSeconds() << " "
            << timeSinceLastUpdate.asSeconds() << " " << TIME_PER_FRAME.asSeconds() << std::endl;
                timeSinceLastUpdate += elapsedTime;
            }*/

            while (timeSinceLastUpdate >= TIME_PER_FRAME) {
                std::cout << "Loop entered" << std::endl;
                timeSinceLastUpdate -= TIME_PER_FRAME;
                playerBody.setLastPosition(playerBody.getPosition()); 

                float horizontalInput = 0.f;
                bool jumpIntentThisFrame = false; 
                bool dropIntentThisFrame = false; 

                
                turboMultiplier = (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::RShift)) ? 2 : 1;

                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Left)) horizontalInput = -1.f;
                else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Right)) horizontalInput = 1.f;

                jumpIntentThisFrame = (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Up) || sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Space));
                dropIntentThisFrame = sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Down);

                bool newJumpPressThisFrame = false;
                if (jumpIntentThisFrame && playerBody.isOnGround() && currentJumpHoldDuration == sf::Time::Zero) {
                    newJumpPressThisFrame = true;
                }
                playerBody.setTryingToDrop(dropIntentThisFrame && playerBody.isOnGround());
                
                for(auto& activePlat : activeMovingPlatforms) {
                    phys::PlatformBody* movingBodyPtr = nullptr;
                    size_t movingTileIndex = (size_t)-1;
                    for(size_t i = 0; i < bodies.size(); ++i) {
                        if (bodies[i].getID() == activePlat.id) { movingBodyPtr = &bodies[i]; movingTileIndex = i; break; }
                    }
                    if (movingBodyPtr) {
                        activePlat.lastFramePosition = movingBodyPtr->getPosition(); 
                        activePlat.cycleTime += TIME_PER_FRAME.asSeconds();
                        if (activePlat.cycleDuration > 0 && activePlat.cycleTime >= activePlat.cycleDuration) {
                            activePlat.cycleTime -= activePlat.cycleDuration; 
                        }
                        float singleMovePhaseDuration = (activePlat.cycleDuration > 0) ? (activePlat.cycleDuration / 2.0f) : 0.f;
                        float offset = 0.f;
                        if (singleMovePhaseDuration > 0) { 
                            if (activePlat.cycleTime < singleMovePhaseDuration) { 
                                offset = math::easing::sineEaseInOut(activePlat.cycleTime, 0, activePlat.direction * activePlat.distance, singleMovePhaseDuration);
                            } else { 
                                offset = math::easing::sineEaseInOut(activePlat.cycleTime - singleMovePhaseDuration, activePlat.direction * activePlat.distance, -activePlat.direction * activePlat.distance, singleMovePhaseDuration);
                            }
                        }
                        sf::Vector2f newPos = activePlat.startPosition; 
                        if(activePlat.axis == 'x') newPos.x += offset; else if(activePlat.axis == 'y') newPos.y += offset;
                        movingBodyPtr->setPosition(newPos);
                        if (movingTileIndex < tiles.size()) { tiles[movingTileIndex].setPosition(movingBodyPtr->getPosition());}
                    }
                }

                for (size_t i = 0; i < bodies.size(); ++i) {
                    if (tiles.size() <= i || (bodies[i].getType() == phys::bodyType::none && tiles[i].getFillColor().a == 0)) continue; 
                    tiles[i].update(TIME_PER_FRAME); 
                    if (bodies[i].getType() == phys::bodyType::falling) {
                        bool playerOnThisPlatform = playerBody.isOnGround() && playerBody.getGroundPlatform() && playerBody.getGroundPlatform()->getID() == bodies[i].getID();
                        if (playerOnThisPlatform && !bodies[i].isFalling() && !tiles[i].isFalling() && !tiles[i].hasFallen()) { tiles[i].startFalling(sf::seconds(0.25f)); }
                        if (tiles[i].isFalling() && !bodies[i].isFalling()) { bodies[i].setFalling(true);  }
                        if (tiles[i].hasFallen()) { 
                            if(bodies[i].getPosition().x > -9000.f) { bodies[i].setPosition({-9999.f, -9999.f});  }
                            if(!bodies[i].isFalling()) bodies[i].setFalling(true); 
                        }
                    } else if (bodies[i].getType() == phys::bodyType::vanishing) {
                        bool is_even_platform = (bodies[i].getID() % 2 == 0); 
                        bool should_be_vanishing_phase = (oddEvenVanishing == 1 && is_even_platform) || (oddEvenVanishing == -1 && !is_even_platform);
                        float currentPhaseTime = vanishingPlatformCycleTimer.asSeconds();
                        if (currentPhaseTime >= 2.f) currentPhaseTime -= 2.f;                        
                        sf::Color originalColor = sf::Color(200, 70, 200, 255); float alpha_val;
                        if (should_be_vanishing_phase) { 
                            alpha_val = math::easing::sineEaseInOut(currentPhaseTime, 255.f, -255.f, 1.f); 
                            alpha_val = std::max(0.f, std::min(255.f, alpha_val));
                            tiles[i].setFillColor(sf::Color(originalColor.r, originalColor.g, originalColor.b, alpha_val));
                            if (alpha_val <= 10 && bodies[i].getPosition().x > -9000.f) { bodies[i].setPosition({-9999.f, -9999.f});}
                        } else { 
                            alpha_val = math::easing::sineEaseInOut(currentPhaseTime, 0.f, 255.f, 1.f); 
                            alpha_val = std::max(0.f, std::min(255.f, alpha_val));
                            tiles[i].setFillColor(sf::Color(originalColor.r, originalColor.g, originalColor.b, alpha_val));
                            if (alpha_val > 10 && bodies[i].getPosition().x < -9000.f) { 
                                bool foundOrigPos = false;
                                for(const auto& pb_template : currentLevelData.platforms) {
                                    if(pb_template.getID() == bodies[i].getID()){
                                        bodies[i].setPosition(pb_template.getPosition());
                                        tiles[i].setPosition(pb_template.getPosition()); 
                                        foundOrigPos = true; break;
                                    }
                                }
                                if(!foundOrigPos) std::cerr << "Vanishing platform ID " << bodies[i].getID() << " original pos not found for restore.\n";
                            }
                        }
                    }
                }
                vanishingPlatformCycleTimer += TIME_PER_FRAME;
                if (vanishingPlatformCycleTimer.asSeconds() >= 2.f) { 
                    vanishingPlatformCycleTimer -= sf::seconds(2.f); oddEvenVanishing *= -1;
                }

                sf::Vector2f currentPlayerVelocity = playerBody.getVelocity();
                currentPlayerVelocity.x = horizontalInput * PLAYER_MOVE_SPEED * turboMultiplier;
                if (!playerBody.isOnGround()) { 
                    currentPlayerVelocity.y += GRAVITY_ACCELERATION * TIME_PER_FRAME.asSeconds();
                    if (currentPlayerVelocity.y > MAX_FALL_SPEED) currentPlayerVelocity.y = MAX_FALL_SPEED;
                }
                if (newJumpPressThisFrame) { 
                    currentPlayerVelocity.y = JUMP_INITIAL_VELOCITY; currentJumpHoldDuration = sf::microseconds(1); 
                } else if (jumpIntentThisFrame && currentJumpHoldDuration > sf::Time::Zero && currentJumpHoldDuration < MAX_JUMP_HOLD_TIME) {
                    currentPlayerVelocity.y = JUMP_INITIAL_VELOCITY; currentJumpHoldDuration += TIME_PER_FRAME;
                } else if (!jumpIntentThisFrame || currentJumpHoldDuration >= MAX_JUMP_HOLD_TIME) {
                    currentJumpHoldDuration = sf::Time::Zero; 
                }
                playerBody.setVelocity(currentPlayerVelocity);

                phys::CollisionResolutionInfo resolutionResult = phys::CollisionSystem::resolveCollisions(playerBody, bodies, TIME_PER_FRAME.asSeconds());
                
                currentPlayerVelocity = playerBody.getVelocity(); 
                if (playerBody.isOnGround()) {
                    currentJumpHoldDuration = sf::Time::Zero; 
                    if (playerBody.getGroundPlatform()) { 
                        const phys::PlatformBody& collidedPf = *playerBody.getGroundPlatform();
                        if (collidedPf.getType() == phys::bodyType::conveyorBelt) {
                            playerBody.setPosition(playerBody.getPosition() + collidedPf.getSurfaceVelocity() * TIME_PER_FRAME.asSeconds());
                        } 
                        else if (collidedPf.getType() == phys::bodyType::moving) {
                             for(const auto& activePlat : activeMovingPlatforms) {
                                if (activePlat.id == collidedPf.getID()) {
                                     phys::PlatformBody* currentMovingBody = nullptr;
                                     for(auto& b : bodies){ if(b.getID() == activePlat.id){ currentMovingBody = &b; break; } }
                                    if(currentMovingBody){
                                        sf::Vector2f platformDisplacement = currentMovingBody->getPosition() - activePlat.lastFramePosition;
                                        playerBody.setPosition(playerBody.getPosition() + platformDisplacement);
                                    }
                                    break; 
                                }
                            }
                        }
                    }
                }
                if (resolutionResult.hitCeiling) {
                    if (currentPlayerVelocity.y < 0.f) currentPlayerVelocity.y = 0.f; 
                    currentJumpHoldDuration = sf::Time::Zero; 
                }
                playerBody.setVelocity(currentPlayerVelocity); 

                if (interactKeyPressedThisFrame) { 
                    for (const auto& platform : bodies) {
                        // --- MODIFICATION: Specifically check for goal by type or by ID 10 (E sprite ID) ---
                        if (platform.getType() == phys::bodyType::goal) { 
                        // if (platform.getID() == 10) { // Or, if ID 10 is *always* your E sprite/goal interaction object
                            if (playerBody.getAABB().findIntersection(platform.getAABB())) {
                                if (levelManager.hasNextLevel()) {
                                    if (levelManager.loadNextLevel(currentLevelData)) {
                                        setupLevelAssets(currentLevelData, window);
                                        window.setTitle("Project T - Level " + std::to_string(currentLevelData.levelNumber));
                                        timeSinceLastUpdate = sf::Time::Zero; 
                                        tickClock.restart();
                                    } else {
                                        std::cerr << "Failed to load next level (" << levelManager.getCurrentLevelNumber() + 1 << ")!" << std::endl;
                                        currentState = GameState::MENU; 
                                    }
                                } else {
                                    currentState = GameState::GAME_OVER_WIN;
                                    window.setTitle("Project T - You Win!");
                                }
                                if (currentState != GameState::PLAYING) {
                                     // interactKeyPressedThisFrame = false; // Key press consumed by state change
                                     break; 
                                }
                            }
                        // } // End ID 10 specific check
                        } // End type goal check
                    }
                    if (currentState != GameState::PLAYING) break; 
                }


                if (playerBody.getPosition().y > 2000.f) { 
                    currentState = GameState::GAME_OVER_LOSE; 
                    window.setTitle("Project T - Game Over");
                    if (currentState != GameState::PLAYING) break;
                }

            } 
            
            if (currentState != GameState::PLAYING) continue;

            playerShape.setPosition(playerBody.getPosition());
            mainView.setCenter({playerBody.getPosition().x + playerBody.getWidth()/2.f,
                               playerBody.getPosition().y + playerBody.getHeight()/2.f - 50.f}); 

            
            try{
                std::cout << (int)currentLevelData.backgroundColor.r << " " <<
                            (int)currentLevelData.backgroundColor.g << " " <<
                            (int)currentLevelData.backgroundColor.b << " " <<
                            (int)currentLevelData.backgroundColor.a << std::endl;

                window.clear(currentLevelData.backgroundColor); 
                window.setView(mainView);

                sf::Vector2f center = mainView.getCenter();
                sf::Vector2f size = mainView.getSize();


                for (const auto& t : tiles) { 
                    if (t.getFillColor().a > 5) window.draw(t);
                }
                window.draw(playerShape);
            } catch (const std::exception& e){
                std::cerr << "Exception during render: " << e.what() << std::endl;
            }

            std::cout << "ERROR NOT HERE!!" << std::endl;
            window.setView(window.getDefaultView()); 
            debugText.setString(
                "Player Pos: " + std::to_string(static_cast<int>(playerBody.getPosition().x)) + "," + std::to_string(static_cast<int>(playerBody.getPosition().y)) +
                "\nPlayer Vel: " + std::to_string(static_cast<int>(playerBody.getVelocity().x)) + "," + std::to_string(static_cast<int>(playerBody.getVelocity().y)) +
                "\nOnGround: " + (playerBody.isOnGround() ? "Yes" : "No") +
                (playerBody.getGroundPlatform() ? (" (ID: " + std::to_string(playerBody.getGroundPlatform()->getID()) + ")") : "") +
                "\nLevel: " + std::to_string(levelManager.getCurrentLevelNumber())
            );
            window.draw(debugText);

            window.display();
            
        } 
    } 

    return 0;
}