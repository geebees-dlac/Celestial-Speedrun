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
#include "dynamicBody.hpp"
#include "platformBody.hpp"
#include "tile.hpp"
#include "interpolate.hpp"
#include "PhysicsTypes.hpp"

enum class GameState {
    MENU,
    PLAYING
};

int main(void) {
    const phys::PlatformBody* lastFrameGroundPlatform = nullptr;
    const sf::Vector2f tileSize(32.f, 32.f);
    const int NUM_PLATFORM_OBJECTS = 24;

    const float PLAYER_MOVE_SPEED = 200.f;
    const float JUMP_INITIAL_VELOCITY = -450.f;
    const float GRAVITY_ACCELERATION = 1200.f;
    const float MAX_FALL_SPEED = 700.f;
    const sf::Time MAX_JUMP_HOLD_TIME = sf::seconds(0.18f);

    sf::RenderWindow window(sf::VideoMode(800, 600), "Project - T (Corrected)", sf::Style::Default);
    window.setKeyRepeatEnabled(false);
    window.setVerticalSyncEnabled(true);
    window.setFramerateLimit(60);
    sf::Event e;

    GameState currentState = GameState::MENU;

    sf::Font menuFont;
    std::string fontPath = "assets/ARIALBD.TTF";
    if (!menuFont.loadFromFile(fontPath)) {
        std::cerr << "Failed to load font: " << fontPath << std::endl;
        return -1;
    }
    sf::Text startButtonText, settingsButtonText, exitButtonText;
    auto setupButton = [&](sf::Text& text, const sf::String& str, float yPos, float xPosOffset = 0.f) {
        text.setFont(menuFont);
        text.setString(str);
        text.setCharacterSize(30);
        text.setFillColor(sf::Color::White);
        sf::FloatRect bounds = text.getLocalBounds();
        text.setOrigin(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
        text.setPosition(window.getSize().x / 2.f + xPosOffset, yPos);
    };
    setupButton(startButtonText, "Start Game", 250.f);
    setupButton(settingsButtonText, "Settings (NYI)", 300.f);
    setupButton(exitButtonText, "Exit", 350.f);

    sf::View mainView(sf::FloatRect(0.f, 0.f, 800.f, 600.f));

    std::vector<phys::PlatformBody> bodies;
    bodies.reserve(NUM_PLATFORM_OBJECTS); 

    // --- Platform Initialization - PROVIDING ALL 7 ARGUMENTS (MORE TO COME)---
    // Order: id, position, width, height, type, initiallyFalling, surfaceVelocity

    // will be useful on creating the level series as we go on, and the pase section menu.
    // for now my focus is this and the applicaacility of the sprites on the game and its possibility of it being intergrated
    //onto the game as early as possible.

    // the guide onto this is checking the bodyType enum class in the PhysicsTypes.hpp file
    // and the platformBody.hpp file for the constructor of the PlatformBody class.

    // but we can use it as a guide to create the level series as we go on, and the pase section menu.
    // and the death effects have not been included yet nor the respawn mechanics for the player

     // ID 0 - Standard Platform
    bodies.emplace_back(static_cast<unsigned int>(0), sf::Vector2f(0.f, window.getSize().y - 32.f), tileSize.x * 4, tileSize.y, phys::bodyType::platform, false, sf::Vector2f(0.f, 0.f));
    // ID 1 - Standard Platform
    bodies.emplace_back(static_cast<unsigned int>(1), sf::Vector2f(0.f, 356.f), tileSize.x * 4, tileSize.y, phys::bodyType::platform, false, sf::Vector2f(0.f, 0.f));
    // ID 2 - Standard Platform
    bodies.emplace_back(static_cast<unsigned int>(2), sf::Vector2f(480.f, window.getSize().y - 192.f), tileSize.x * 4, tileSize.y, phys::bodyType::platform, false, sf::Vector2f(0.f, 0.f));
    // ID 3 - Standard Platform
    bodies.emplace_back(static_cast<unsigned int>(3), sf::Vector2f(0.f, 96.f), tileSize.x * 4, tileSize.y, phys::bodyType::platform, false, sf::Vector2f(0.f, 0.f));
    // ID 4 - Standard Platform
    bodies.emplace_back(static_cast<unsigned int>(4), sf::Vector2f(640.f, 224.f), tileSize.x * 4, tileSize.y, phys::bodyType::platform, false, sf::Vector2f(0.f, 0.f));
    // ID 5 - Standard Platform
    bodies.emplace_back(static_cast<unsigned int>(5), sf::Vector2f(480.f, window.getSize().y - 32.f), tileSize.x * 4, tileSize.y, phys::bodyType::platform, false, sf::Vector2f(0.f, 0.f));
    // ID 6 - Standard Platform
    bodies.emplace_back(static_cast<unsigned int>(6), sf::Vector2f(672.f, window.getSize().y - 260.f), tileSize.x * 4, tileSize.y, phys::bodyType::platform, false, sf::Vector2f(0.f, 0.f));
    // ID 7 - Standard Platform
    bodies.emplace_back(static_cast<unsigned int>(7), sf::Vector2f(96.f, window.getSize().y - 128.f), tileSize.x * 4, tileSize.y, phys::bodyType::platform, false, sf::Vector2f(0.f, 0.f));
    // ID 8 - Standard Platform
    bodies.emplace_back(static_cast<unsigned int>(8), sf::Vector2f(640.f, window.getSize().y - 160.f), tileSize.x * 4, tileSize.y, phys::bodyType::platform, false, sf::Vector2f(0.f, 0.f));
    // ID 9 - Standard Platform
    bodies.emplace_back(static_cast<unsigned int>(9), sf::Vector2f(640.f, 64.f), tileSize.x * 4, tileSize.y, phys::bodyType::platform, false, sf::Vector2f(0.f, 0.f));
    // ID 10 - Standard Platform
    bodies.emplace_back(static_cast<unsigned int>(10), sf::Vector2f(292.f, window.getSize().y - 128.f), tileSize.x * 4, tileSize.y, phys::bodyType::platform, false, sf::Vector2f(0.f, 0.f));

    // ID 11 (Conveyor)
    bodies.emplace_back(static_cast<unsigned int>(11), sf::Vector2f(window.getSize().x / 4.f, window.getSize().y / 2.f),
                        tileSize.x * 10, tileSize.y, phys::bodyType::conveyorBelt,
                        false, sf::Vector2f(70.f, 0.f));
    // ID 12 (Moving)
    bodies.emplace_back(static_cast<unsigned int>(12), sf::Vector2f(window.getSize().x / 4.f, window.getSize().y / 4.f),
                        tileSize.x * 5, tileSize.y / 8.f, phys::bodyType::moving, false, sf::Vector2f(0.f, 0.f));
    // ID 13 (Jump-through)
    bodies.emplace_back(static_cast<unsigned int>(13), sf::Vector2f(160.f, window.getSize().y - 224.f),
                        tileSize.x, tileSize.y / 8.f, phys::bodyType::jumpthrough, false, sf::Vector2f(0.f, 0.f));
    // ID 14 (Jump-through)
    bodies.emplace_back(static_cast<unsigned int>(14), sf::Vector2f(576.f, window.getSize().y - 288.f),
                        tileSize.x, tileSize.y / 8.f, phys::bodyType::jumpthrough, false, sf::Vector2f(0.f, 0.f));

    // IDs 15-19 (Falling)
    for(int id_val_int=15; id_val_int<=19; ++id_val_int) { // Use a temp int for loop
        bodies.emplace_back(static_cast<unsigned int>(id_val_int), sf::Vector2f(192.f + (id_val_int-15)*tileSize.x, 64.f),
                            tileSize.x, tileSize.y, phys::bodyType::falling, false, sf::Vector2f(0.f, 0.f));
    }
    // IDs 20-23 (Vanishing)
    for(int id_val_int=20; id_val_int<=23; ++id_val_int) { // Use a temp int for loop
        bodies.emplace_back(static_cast<unsigned int>(id_val_int), sf::Vector2f(192.f + (id_val_int-20)*tileSize.x, -32.f),
                            tileSize.x, tileSize.y, phys::bodyType::vanishing, false, sf::Vector2f(0.f, 0.f));
    }

    unsigned int next_id_filler_uint = bodies.size(); // Use unsigned int here too
    while (bodies.size() < NUM_PLATFORM_OBJECTS) {
        bodies.emplace_back(next_id_filler_uint++, sf::Vector2f(-10000.f, -10000.f), 1.f, 1.f, phys::bodyType::none, false, sf::Vector2f(0.f, 0.f));
    }

    std::vector<Tile> tiles;
    tiles.reserve(bodies.size());
    for (const auto& body : bodies) {
        Tile newTile(sf::Vector2f(body.getWidth(), body.getHeight()));
        newTile.setPosition(body.getPosition());
        switch (body.getType()) {
            case phys::bodyType::platform:     newTile.setFillColor(sf::Color(200, 70, 70, 255)); break;
            case phys::bodyType::conveyorBelt: newTile.setFillColor(sf::Color(255, 150, 50, 255)); break;
            case phys::bodyType::moving:       newTile.setFillColor(sf::Color(70, 200, 70, 255)); break;
            case phys::bodyType::jumpthrough:  newTile.setFillColor(sf::Color(70, 150, 200, 180)); break;
            case phys::bodyType::falling:      newTile.setFillColor(sf::Color(200, 200, 70, 255)); break;
            case phys::bodyType::vanishing:    newTile.setFillColor(sf::Color(200, 70, 200, 255)); break;
            default:                           newTile.setFillColor(sf::Color(128, 128, 128, 255)); break;
        }
        tiles.push_back(newTile);
    }

    phys::DynamicBody playerBody(
        {window.getSize().x / 2.f, window.getSize().y / 2.f - 100.f},
        tileSize.x, tileSize.y, {0.f, 0.f}
    );

    sf::RectangleShape playerShape;
    playerShape.setFillColor(sf::Color(220, 220, 250, 255));
    playerShape.setSize(sf::Vector2f(playerBody.getWidth(), playerBody.getHeight()));
    playerShape.setPosition(playerBody.getPosition());

    sf::Clock tickClock;
    sf::Time timeSinceLastUpdate = sf::Time::Zero;
    const sf::Time TIME_PER_FRAME = sf::seconds(1.f / 60.f);

    sf::Time movingPlatformCycleTimer = sf::Time::Zero;
    int platformDir = 1;
    float platformFrameVelocity = 0.f;

    sf::Time vanishingPlatformCycleTimer = sf::Time::Zero;
    int oddEvenVanishing = 1;

    bool jumpKeyHeld = false;
    sf::Time currentJumpHoldDuration = sf::Time::Zero;
    bool playerIsGrounded = false;

    int turboMultiplier = 1;
    int debugMode = -1;

    bool running = true;
    while (running) {
        if (currentState == GameState::MENU) {
            // (Menu input handling)
            sf::Event menuEvent;
            while(window.pollEvent(menuEvent)) {
                 if (menuEvent.type == sf::Event::Closed) { running = false; window.close(); }
                 if (menuEvent.type == sf::Event::KeyPressed && menuEvent.key.code == sf::Keyboard::Escape) { running = false; window.close(); }
                 // Add your menu button logic (hover, click based on mousePosView and menuEvent.MouseButtonReleased)
            }
            // (Menu drawing)
            window.clear(sf::Color(30,30,70));
            window.setView(window.getDefaultView());
            window.draw(startButtonText);
            window.draw(settingsButtonText);
            window.draw(exitButtonText);
            window.display();
            if(!running) break;
            continue;
        }

        // --- PLAYING STATE ---
        // ... (event polling as before) ...

        sf::Time elapsedTime = tickClock.restart();
        timeSinceLastUpdate += elapsedTime;

        while (timeSinceLastUpdate >= TIME_PER_FRAME) {
            timeSinceLastUpdate -= TIME_PER_FRAME;
            playerBody.setLastPosition(playerBody.getPosition());

            // --- 1. INPUT PROCESSING ---
            // ... (input logic as before to set horizontalInput, jumpKeyHeld, newJumpPressThisFrame) ...
            float horizontalInput = 0.f; // Reset
            bool newJumpPressThisFrame = false; // Reset
            jumpKeyHeld = false; // Reset
            turboMultiplier = 1; // Reset

            // Your full input logic to set these variables
            if (sf::Joystick::isConnected(0)) {
                float joyX = sf::Joystick::getAxisPosition(0, sf::Joystick::X);
                if (std::abs(joyX) > 20.f) horizontalInput = joyX / 100.f;
                if (sf::Joystick::isButtonPressed(0, 0)) jumpKeyHeld = true;
                if (sf::Joystick::isButtonPressed(0, 2)) turboMultiplier = 2;
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) horizontalInput = -1.f;
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) horizontalInput = 1.f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Up) || sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) jumpKeyHeld = true;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift)) turboMultiplier = 2;

            if (jumpKeyHeld && playerIsGrounded && currentJumpHoldDuration == sf::Time::Zero) {
                newJumpPressThisFrame = true;
            }


            // --- PLATFORM BEHAVIORS ---
            // Moving Platform
            phys::PlatformBody* movingPlatformPtr = nullptr; // Find the moving platform
            for(auto& pf : bodies) { if(pf.getID() == 12 && pf.getType() == phys::bodyType::moving) { movingPlatformPtr = &pf; break; } }

            if (movingPlatformPtr) {
                movingPlatformCycleTimer += TIME_PER_FRAME;
                if (movingPlatformCycleTimer.asSeconds() >= 4.f) {
                    platformDir *= -1; movingPlatformCycleTimer = sf::Time::Zero;
                }
                float moveCycleProgress = movingPlatformCycleTimer.asSeconds();
                if (moveCycleProgress <= 3.f) {
                    platformFrameVelocity = platformDir * 200.f * math::easing::quadraticEaseInOut(moveCycleProgress, 0.f, 1.f, 3.f) * TIME_PER_FRAME.asSeconds();
                } else { platformFrameVelocity = 0.f; }
                movingPlatformPtr->setPosition(movingPlatformPtr->getPosition() + sf::Vector2f(platformFrameVelocity, 0.f));
                // Sync corresponding Tile: Find tile matching movingPlatformPtr->getID() or by index if consistent
                for(auto& tile_vis : tiles) { if(tile_vis.getPosition() == movingPlatformPtr->getPosition() - sf::Vector2f(platformFrameVelocity, 0.f) ) { tile_vis.setPosition(movingPlatformPtr->getPosition()); break; }} //This search is inefficient. Link tiles and bodies better.
                 // A better way: if (tiles.size() > 12 && bodies[12].getID() == 12) tiles[12].setPosition(bodies[12].getPosition()); ASSUMING bodies[12] IS THE ONE.
            }


            // Falling & Vanishing Platforms
            for (size_t i = 0; i < bodies.size(); ++i) {
                 if (tiles.size() <=i) continue; // Safety

                tiles[i].update(TIME_PER_FRAME); // ** ALWAYS UPDATE THE TILE **

                if (bodies[i].getType() == phys::bodyType::falling) {
                    bool playerOnThisPlatform = playerIsGrounded &&
                                                lastFrameGroundPlatform != nullptr &&
                                                lastFrameGroundPlatform->getID() == bodies[i].getID();

                    if (playerOnThisPlatform && !bodies[i].isFalling() && !tiles[i].isFalling() && !tiles[i].hasFallen()) {
                        tiles[i].startFalling(sf::seconds(0.25f)); // Tile handles its delay and m_isFalling state
                         // You might still change visual cues here on the tile
                         // tiles[i].setFillColor(sf::Color::Yellow);
                    }

                    // If Tile says it IS visually falling (after its delay timer)
                    // AND the physics body isn't yet marked as such
                    if (tiles[i].isFalling() && !bodies[i].isFalling()) {
                         bodies[i].setFalling(true); // Sync physics body state
                         // tiles[i].setFillColor(sf::Color(255,100,0)); // Visually falling color
                    }

                    // If Tile has visually completed its fall (e.g., went off-screen)
                    if (tiles[i].hasFallen()) {
                         bodies[i].setPosition({-9999.f, -9999.f}); // Make physics body non-collidable
                         if (!bodies[i].isFalling()) bodies[i].setFalling(true); // Ensure state consistency
                    }
                } else if (bodies[i].getType() == phys::bodyType::vanishing) {
                    bool is_even_platform = (bodies[i].getID() % 2 == 0);
                    bool should_be_vanishing_phase = (oddEvenVanishing == 1 && is_even_platform) || (oddEvenVanishing == -1 && !is_even_platform);
                    float t_vanish = vanishingPlatformCycleTimer.asSeconds();

                    if (should_be_vanishing_phase) {
                        float alpha_val = math::easing::sineEaseIn(t_vanish, 0.f, 255.f, 1.f);
                        tiles[i].setFillColor(sf::Color(200, 70, 200, 255 - static_cast<sf::Uint8>(alpha_val)));
                        if (tiles[i].getFillColor().a <= 10) {
                            bodies[i].setPosition({-9999.f, -9999.f});
                        }
                    } else {
                        float alpha_val = math::easing::sineEaseIn(t_vanish, 0.f, 255.f, 1.f);
                        tiles[i].setFillColor(sf::Color(200, 70, 200, static_cast<sf::Uint8>(alpha_val)));
                        // Check if the body *was* off-screen before making it reappear
                        if (tiles[i].getFillColor().a > 10 && bodies[i].getPosition().x < -9000.f) {

                            bodies[i].setPosition(tiles[i].getPosition()); // Restore based on current tile visual pos
                        }
                    }
                }
            }
            vanishingPlatformCycleTimer += TIME_PER_FRAME;
            if (vanishingPlatformCycleTimer.asSeconds() > 2.f) {
                vanishingPlatformCycleTimer = sf::Time::Zero;
                oddEvenVanishing *= -1;
            }

            // --- 2. PLAYER PHYSICS --- (Using getters/setters for playerBody)
            sf::Vector2f currentPlayerVelocity = playerBody.getVelocity();
            currentPlayerVelocity.x = horizontalInput * PLAYER_MOVE_SPEED * turboMultiplier;
            if (!playerIsGrounded) {
                currentPlayerVelocity.y += GRAVITY_ACCELERATION * TIME_PER_FRAME.asSeconds();
                if (currentPlayerVelocity.y > MAX_FALL_SPEED) currentPlayerVelocity.y = MAX_FALL_SPEED;
            }
            if (newJumpPressThisFrame && playerIsGrounded) {
                currentPlayerVelocity.y = JUMP_INITIAL_VELOCITY;
                currentJumpHoldDuration = sf::microseconds(1);
            } else if (jumpKeyHeld && currentJumpHoldDuration > sf::Time::Zero && currentJumpHoldDuration < MAX_JUMP_HOLD_TIME) {
                currentPlayerVelocity.y = JUMP_INITIAL_VELOCITY;
                currentJumpHoldDuration += TIME_PER_FRAME;
            } else if (!jumpKeyHeld || currentJumpHoldDuration >= MAX_JUMP_HOLD_TIME) {
                currentJumpHoldDuration = sf::Time::Zero;
            }
            playerBody.setVelocity(currentPlayerVelocity);

            // --- 3. APPLY VELOCITY TO POSITION ---
            playerBody.setPosition(playerBody.getPosition() + playerBody.getVelocity() * TIME_PER_FRAME.asSeconds());

            // --- 4. COLLISION DETECTION AND RESOLUTION ---
            phys::CollisionResolutionInfo resolutionResult = phys::CollisionSystem::resolveCollisions(playerBody, bodies, TIME_PER_FRAME.asSeconds());

            // --- 5. POST-COLLISION ADJUSTMENTS & STATE UPDATE ---
            playerIsGrounded = resolutionResult.onGround;
            lastFrameGroundPlatform = resolutionResult.groundPlatform; // Store for next iteration's platform logic

            currentPlayerVelocity = playerBody.getVelocity(); // Get velocity modified by CollisionSystem

            if (resolutionResult.onGround) {
                if (currentPlayerVelocity.y > 0.f) currentPlayerVelocity.y = 0.f;
                currentJumpHoldDuration = sf::Time::Zero;
                if (resolutionResult.groundPlatform) {
                    const phys::PlatformBody& collidedPf = *resolutionResult.groundPlatform;
                    if (collidedPf.getType() == phys::bodyType::conveyorBelt) {
                        playerBody.setPosition(playerBody.getPosition() + collidedPf.getSurfaceVelocity() * TIME_PER_FRAME.asSeconds());
                    } else if (movingPlatformPtr && collidedPf.getID() == movingPlatformPtr->getID()) { // Check against the found moving platform
                        playerBody.setPosition(playerBody.getPosition() + sf::Vector2f(platformFrameVelocity, 0.f));
                    }
                }
            }
            if (resolutionResult.hitCeiling) {
                if (currentPlayerVelocity.y < 0.f) currentPlayerVelocity.y = 0.f;
                currentJumpHoldDuration = sf::Time::Zero;
            }
            playerBody.setVelocity(currentPlayerVelocity);

            playerShape.setPosition(playerBody.getPosition());

            if (debugMode == 1) { // Debug mode 1: Show player zoom zoom
                std::cout << "Player Velocity: " << playerBody.getVelocity().x << ", " << playerBody.getVelocity().y << std::endl;
            } else if (debugMode == 2) { // Debug mode 2: Show platform ID to check for trolls later
                std::cout << "Platform ID: " << (resolutionResult.groundPlatform ? resolutionResult.groundPlatform->getID() : -1) << std::endl;
            }
            // --- 6. VIEW UPDATE ---

        } 
        mainView.setCenter(playerBody.getPosition().x, playerBody.getPosition().y - 50.f);
        window.clear(sf::Color(20, 20, 40));
        window.setView(mainView);
        for (const auto& t : tiles) { if (t.getFillColor().a > 5) window.draw(t); }
        window.draw(playerShape);
        window.setView(window.getDefaultView());
        window.display();
    }
    return 0;
}