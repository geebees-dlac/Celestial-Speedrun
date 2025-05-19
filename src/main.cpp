#include <iostream>
#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>
#include <string>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <limits>
#include <filesystem> // when needed for levels, i think but now its not needed
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include "CollisionSystem.hpp"
#include "Player.hpp"
#include "PlatformBody.hpp"
#include "Tile.hpp"
#include "Optimizer.hpp" // i have abandoned this for now, i will come back to this later
#include "PhysicsTypes.hpp"
#include "Player.hpp" // i have abandoned this for now, i will come back to this later
#include "LevelManager.hpp" // i have abandoned this for now, i will come back to this later

enum class GameState {
    MENU,
    PLAYING
};

int main(void) {
    const sf::Vector2f tileSize(32.f, 32.f);
    const int NUM_PLATFORM_OBJECTS = 24; // Adjust depending on how many we make, should be needed for file lvl 12345
    const float PLAYER_MOVE_SPEED = 200.f;
    const float JUMP_INITIAL_VELOCITY = -450.f;
    const float GRAVITY_ACCELERATION = 1200.f;
    const float MAX_FALL_SPEED = 700.f;
    const sf::Time MAX_JUMP_HOLD_TIME = sf::seconds(0.18f);

    sf::RenderWindow window(sf::VideoMode({800, 600}), "Project - T (Updated Physics)", sf::Style::Default);
    window.setKeyRepeatEnabled(false);
    window.setVerticalSyncEnabled(true); //better than framrate cuz this acually exists, thank you brain for looking this up lmaooooo
    GameState currentState = GameState::MENU;

    sf::Font menuFont;
    std::string fontPath = "../assets/fonts/ARIALBD.TTF"; 
    if (!menuFont.openFromFile(fontPath)) {
        std::cerr << "Failed to load font: " << fontPath << std::endl;
        fontPath = "ARIALBD.TTF";
        if (!menuFont.openFromFile(fontPath)) {
             std::cerr << "Failed to load font from local: " << fontPath << std::endl;
            return -1;
        } //this is to avoid the incident... catdespair
    }
    sf::Text startButtonText(menuFont), settingsButtonText(menuFont), exitButtonText(menuFont);
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

    sf::Color defaultButtonColor = sf::Color::White;
    sf::Color hoverButtonColor = sf::Color::Yellow;
    sf::Color clickedButtonColor = sf::Color::Green;
    sf::Color exitButtonColor = sf::Color::Red;

    sf::View mainView(sf::FloatRect({0.f, 0.f}, {800.f, 600.f}));

    // Level loading 
    rapidjson::Document* level1;
    level1 = readFile();
    savetest(level1);

    std::vector<phys::PlatformBody> bodies;
    bodies.reserve(NUM_PLATFORM_OBJECTS);
    bodies.clear();
    unsigned int current_id = 0;

    // --- Main Solid Floor ---
    bodies.emplace_back(current_id++,
                        sf::Vector2f(0.f, window.getSize().y - tileSize.y),
                        window.getSize().x, tileSize.y,
                        phys::bodyType::solid, false, sf::Vector2f(0.f, 0.f));

    float leftSectionX = tileSize.x * 3.f;
    // --- One-Way Platform ---
    bodies.emplace_back(current_id++,
                        sf::Vector2f(leftSectionX, window.getSize().y - tileSize.y * 5.f),
                        tileSize.x * 3.f, tileSize.y / 4.f,
                        phys::bodyType::platform, false, sf::Vector2f(0.f, 0.f));

    // --- Standard Solid Platform---
    bodies.emplace_back(current_id++,
                        sf::Vector2f(leftSectionX + tileSize.x, window.getSize().y - tileSize.y * 8.f),
                        tileSize.x * 4.f, tileSize.y,
                        phys::bodyType::solid, false, sf::Vector2f(0.f, 0.f));

    float middleSectionX = window.getSize().x / 2.f - (tileSize.x * 5.f);
    // ID 3 - Conveyor Belt
    bodies.emplace_back(current_id++,
                        sf::Vector2f(middleSectionX, window.getSize().y - tileSize.y * 3.f),
                        tileSize.x * 10.f, tileSize.y,
                        phys::bodyType::conveyorBelt, false, sf::Vector2f(70.f, 0.f));

    // ID 4 - Moving Platform , these fuckers behave like solids ok, not platforms, since im not sure how to make them work yet
    // For now, assume type::moving is multi-directional solid like type::solid.
    size_t movingPlatformID = current_id; // Store the ID if needed later
    bodies.emplace_back(current_id++,
                        sf::Vector2f(middleSectionX + tileSize.x * 2.f, window.getSize().y - tileSize.y * 7.f),
                        tileSize.x * 5.f, tileSize.y / 2.f,
                        phys::bodyType::moving, false, sf::Vector2f(0.f, 0.f));


    float rightSectionX = window.getSize().x - tileSize.x * 10.f;
    for (int i = 0; i < 3; ++i) {
        bodies.emplace_back(current_id++,
                            sf::Vector2f(rightSectionX + (i * (tileSize.x + tileSize.x/2.f)), window.getSize().y - tileSize.y * 6.f),
                            tileSize.x, tileSize.y,
                            phys::bodyType::falling, false, sf::Vector2f(0.f, 0.f));
    }

    float vanishingY = window.getSize().y - tileSize.y * 10.f;
    for (int i = 0; i < 3; ++i) {
        bodies.emplace_back(current_id++,
                            sf::Vector2f(rightSectionX + tileSize.x * 1.5f + (i * (tileSize.x + tileSize.x/2.f)), vanishingY),
                            tileSize.x, tileSize.y,
                            phys::bodyType::vanishing, false, sf::Vector2f(0.f, 0.f));
    }
    // --- Solid Platform ---
    bodies.emplace_back(current_id++,
                        sf::Vector2f(window.getSize().x / 2.f - tileSize.x * 2.f, tileSize.y * 5.f),
                        tileSize.x * 4.f, tileSize.y,
                        phys::bodyType::solid, false, sf::Vector2f(0.f, 0.f));


    // Fill remaining with ::none type
    // This is to ensure we have a fixed number of bodies for the simulation
    // This is useful for debugging and ensuring we have a consistent number of bodies in the simulation if we need to add more later
    // This is a placeholder, you can adjust the number of bodies as needed and sprites 
    while (bodies.size() < NUM_PLATFORM_OBJECTS) {
        bodies.emplace_back(current_id++,
                            sf::Vector2f(-10000.f, -10000.f), 1.f, 1.f,
                            phys::bodyType::none, false, sf::Vector2f(0.f, 0.f));
    } // yes just copy past this [;es]


    std::vector<Tile> tiles;
    tiles.reserve(bodies.size());
    std::vector<size_t> tile_to_body_map; //moving tiles
    tile_to_body_map.resize(bodies.size());

    for (size_t i = 0; i < bodies.size(); ++i) {
        const auto& body = bodies[i];
        tile_to_body_map[i] = i; // Simple 1-to-1 map for now will adjust later

        Tile newTile(sf::Vector2f(body.getWidth(), body.getHeight()));
        newTile.setPosition({body.getPosition()});
        switch (body.getType()) {
            // i like how geometry dash inspired me to make it like this
            case phys::bodyType::solid:        newTile.setFillColor(sf::Color(0, 0, 0, 255)); break; // Dark Gray for solid
            case phys::bodyType::platform:     newTile.setFillColor(sf::Color(70, 150, 200, 180)); break; // Light blue for one-way
            case phys::bodyType::conveyorBelt: newTile.setFillColor(sf::Color(255, 150, 50, 255)); break; //orange
            case phys::bodyType::moving:       newTile.setFillColor(sf::Color(70, 200, 70, 255)); break; // Green for moving
            case phys::bodyType::falling:      newTile.setFillColor(sf::Color(200, 200, 70, 255)); break; // Yellow for falling
            case phys::bodyType::vanishing:    newTile.setFillColor(sf::Color(200, 70, 200, 255)); break; // Purple for vanishing
            case phys::bodyType::none:         newTile.setFillColor(sf::Color(0, 0, 0, 0)); break; // Transparent for none
            case phys::bodyType::spring:       newTile.setFillColor(sf::Color(255, 255, 0, 255)); break; // Yellow for spring
            case phys::bodyType::trap:         newTile.setFillColor(sf::Color(255, 0, 0, 255)); break; // Red for trap
            default:                           newTile.setFillColor(sf::Color(128, 128, 128, 255)); break; // Default Gray for unknown debugging purposes so if u find gray tell me
        }
        tiles.push_back(newTile);
    }

    freeData(level1);

    phys::DynamicBody playerBody(
        {window.getSize().x / 4.f, window.getSize().y / 2.f - 100.f}, // the experimental ground here starts at left
        //i cant believe im putting comments at the bottom it looks bad anyways
        // ypu can adjust as you see fit on where ypu want to player to start on other levels
        //cuz like i said u do you
        tileSize.x, tileSize.y, {0.f, 0.f}
    );

    sf::RectangleShape playerShape;
    playerShape.setFillColor(sf::Color(220, 220, 250, 255));
    playerShape.setSize(sf::Vector2f(playerBody.getWidth(), playerBody.getHeight()));
    playerShape.setPosition({playerBody.getPosition()});

    sf::Clock tickClock;
    sf::Time timeSinceLastUpdate = sf::Time::Zero;
    const sf::Time TIME_PER_FRAME = sf::seconds(1.f / 60.f); 

    sf::Time movingPlatformCycleTimer = sf::Time::Zero;
    float movingPlatformTotalDist = tileSize.x * 6.f; 
    int platformDir = 1;
    sf::Vector2f movingPlatformStartPos = {middleSectionX + tileSize.x * 2.f, window.getSize().y - tileSize.y * 7.f};


    sf::Time vanishingPlatformCycleTimer = sf::Time::Zero;
    int oddEvenVanishing = 1;

    bool jumpKeyHeld = false;
    sf::Time currentJumpHoldDuration = sf::Time::Zero;     // playerIsGrounded is now primarily managed by playerBody.isOnGround() after collisions
    int turboMultiplier = 1; //in case you read the pdf and the website

    // Debug, should put here for any debugs you make ok..... if its on main or in the drawing window below
    // and and i forgor what to say ill come back here later (the comment after the ... was the one i was looking for)
    sf::Text debugText(menuFont);
    debugText.setFont(menuFont);
    debugText.setCharacterSize(14);
    debugText.setFillColor(sf::Color::White);
    debugText.setPosition({10.f, 10.f});


    bool running = true;
    while (running) {
       //sf::Event event;
        while (const std::optional event=window.pollEvent()) {
            if (event->is<sf::Event::Closed>()){
                running = false; window.close();
            }
            if (currentState == GameState::MENU) { // Menu event i didnt change it just relocated it
                if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()){
                    if (keyPressed->scancode==sf::Keyboard::Scancode::Escape){
                        running = false; window.close();
                    }
                }

                if (const auto* mouseButtonReleased = event->getIf<sf::Event::MouseButtonReleased>()){
                    if (mouseButtonReleased->button == sf::Mouse::Button::Left){
                        sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window), window.getDefaultView());
                        if (startButtonText.getGlobalBounds().contains(mousePos)) {
                            currentState = GameState::PLAYING;
                            timeSinceLastUpdate = sf::Time::Zero; // Reset simulation time
                            tickClock.restart();
                            window.setTitle("Project T (Playing)");
                        } else if (settingsButtonText.getGlobalBounds().contains(mousePos)) {
                            std::cout << "Settings pressed (NYI)" << std::endl;
                        } else if (exitButtonText.getGlobalBounds().contains(mousePos)) {
                            running = false; window.close();
                        }
                    }
                }
                
            } else if (currentState == GameState::PLAYING) { // Playing event handling
                if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()){ 
                    if (keyPressed->scancode == sf::Keyboard::Scancode::Escape) {
                        currentState = GameState::MENU; // Pause to menu
                        window.setTitle("Project T (Menu - Paused)");
                    }
                    if (keyPressed->scancode == sf::Keyboard::Scancode::R) {
                        playerBody.setPosition({window.getSize().x / 4.f, window.getSize().y / 2.f - 150.f});
                        playerBody.setVelocity({0.f, 0.f});
                    }
                }
            }
        }

        if (!running) break;


        if (currentState == GameState::MENU) {
            sf::Vector2f mousePosView = window.mapPixelToCoords(sf::Mouse::getPosition(window), window.getDefaultView());
            bool mousePressedDown = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);

            startButtonText.setFillColor(defaultButtonColor);
            settingsButtonText.setFillColor(defaultButtonColor);
            exitButtonText.setFillColor(defaultButtonColor);

            if (startButtonText.getGlobalBounds().contains(mousePosView)) {
                startButtonText.setFillColor(mousePressedDown ? clickedButtonColor : hoverButtonColor);
            } else if (settingsButtonText.getGlobalBounds().contains(mousePosView)) {
                settingsButtonText.setFillColor(mousePressedDown ? clickedButtonColor : hoverButtonColor);
            } else if (exitButtonText.getGlobalBounds().contains(mousePosView)) {
                exitButtonText.setFillColor(mousePressedDown ? exitButtonColor : hoverButtonColor);
            }

            window.clear(sf::Color(30, 30, 70));
            window.setView(window.getDefaultView()); // Use default view for UI
            window.draw(startButtonText);
            window.draw(settingsButtonText);
            window.draw(exitButtonText);
            window.display();
            continue; 
        }

        // --- PLAYING STATE LOGIC ---
        sf::Time elapsedTime = tickClock.restart();
        timeSinceLastUpdate += elapsedTime;

        while (timeSinceLastUpdate >= TIME_PER_FRAME) {
            timeSinceLastUpdate -= TIME_PER_FRAME;
            playerBody.setLastPosition(playerBody.getPosition()); // Store before any physics

            // --- 1. INPUT ---
            // escape is pause menu and r is to reset the player
            //shift is for run but i will remove depending on u guys
            //wasd for movement you alr know this and spacebar w or arrowkeys

            float horizontalInput = 0.f;
            bool jumpIntentThisFrame = false; // Key press for jump
            bool dropIntentThisFrame = false; // Key press for dropping

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


            // --- PLATFORM BEHAVIORS ---
            // Find the moving platform
            phys::PlatformBody* movingPlatformPtr = nullptr;
            size_t movingPlatformTileIndex = (size_t)-1; // Index for the visual tile
            for(size_t i=0; i < bodies.size(); ++i) {
                if(bodies[i].getID() == movingPlatformID && bodies[i].getType() == phys::bodyType::moving) {
                    movingPlatformPtr = &bodies[i];
                    movingPlatformTileIndex = i; // Assuming tiles are in same order
                    break;
                }
            }
            sf::Vector2f movingPlatformFrameDisplacement = {0.f,0.f};

            if (movingPlatformPtr) {
                movingPlatformCycleTimer += TIME_PER_FRAME;
                float cycleDuration = 4.f; // e.g., 2s to move, 2s pause, 2s back, 2s pause change this as you see fit
                float moveTime = cycleDuration / 2.f;

                if (movingPlatformCycleTimer.asSeconds() >= cycleDuration) {
                     movingPlatformCycleTimer -= sf::seconds(cycleDuration); // Reset cycle idk if it removes the remainders or not
                }

                float currentCycleTime = movingPlatformCycleTimer.asSeconds();
                float targetPosOffset = 0.f;

                if(currentCycleTime < moveTime / 2.f) { // Moving one way
                    targetPosOffset = math::easing::sineEaseInOut(currentCycleTime, 0, platformDir * movingPlatformTotalDist, moveTime/2.f);
                } else if (currentCycleTime < moveTime) { // Pause at one end
                    targetPosOffset = platformDir * movingPlatformTotalDist;
                } else if (currentCycleTime < moveTime + moveTime/2.f) { // Moving back
                    targetPosOffset = math::easing::sineEaseInOut(currentCycleTime - moveTime, platformDir * movingPlatformTotalDist, -platformDir * movingPlatformTotalDist, moveTime/2.f);
                } else { // Pause at start
                    targetPosOffset = 0;
                }

                sf::Vector2f newPos = movingPlatformStartPos + sf::Vector2f(targetPosOffset, 0.f);
                movingPlatformFrameDisplacement = newPos - movingPlatformPtr->getPosition();
                movingPlatformPtr->setPosition({newPos});

                if(movingPlatformTileIndex < tiles.size()) {
                    tiles[movingPlatformTileIndex].setPosition({movingPlatformPtr->getPosition()});
                }
            }

            // Falling & Vanishing
             for (size_t i = 0; i < bodies.size(); ++i) {
                 if (tiles.size() <=i) continue;

                tiles[i].update(TIME_PER_FRAME); // Update visual tile

                if (bodies[i].getType() == phys::bodyType::falling) {
                    bool playerOnThisPlatform = playerBody.isOnGround() &&
                                                playerBody.getGroundPlatform() != nullptr &&
                                                playerBody.getGroundPlatform()->getID() == bodies[i].getID();

                    if (playerOnThisPlatform && !bodies[i].isFalling() && !tiles[i].isFalling() && !tiles[i].hasFallen()) {
                        tiles[i].startFalling(sf::seconds(0.25f));
 
                    }
                    if (tiles[i].isFalling() && !bodies[i].isFalling()) { // Tile has started visual fall
                         bodies[i].setFalling(true); // Mark physics body
                         // Visual changes in Tile.update or when starting
                    }
                    if (tiles[i].hasFallen()) { // Visually off-screen
                         bodies[i].setPosition({-9999.f, -9999.f}); // Move physics body out
                         if(!bodies[i].isFalling()) bodies[i].setFalling(true);
                    }
                } else if (bodies[i].getType() == phys::bodyType::vanishing) {
                    bool is_even_platform = (bodies[i].getID() % 2 == 0); // Example logic
                    bool should_be_vanishing_phase = (oddEvenVanishing == 1 && is_even_platform) || (oddEvenVanishing == -1 && !is_even_platform);
                    float t_vanish_cycle_normalized = vanishingPlatformCycleTimer.asSeconds() / 2.f; // Normalize over 2s cycle

                    sf::Color originalColor = sf::Color(200, 70, 200, 255); // Store original color
                    float alpha_val;

                    if (should_be_vanishing_phase) { // Fade out
                        alpha_val = math::easing::sineEaseInOut(t_vanish_cycle_normalized, 255.f, -255.f, 1.f);
                        alpha_val = std::max(0.f, std::min(255.f, alpha_val)); // Clamp
                        tiles[i].setFillColor(sf::Color(originalColor));
                        if (alpha_val <= 10) { // Fully (or mostly) faded
                            bodies[i].setPosition({-9999.f, -9999.f});
                        }
                    } else { // Fade in
                        alpha_val = math::easing::sineEaseInOut(t_vanish_cycle_normalized, 0.f, 255.f, 1.f);
                        alpha_val = std::max(0.f, std::min(255.f, alpha_val)); // Clamp
                        tiles[i].setFillColor(sf::Color(originalColor));
                        if (alpha_val > 10 && bodies[i].getPosition().x < -9000.f) {
                            bodies[i].setPosition({tiles[i].getPosition()});
                        }
                    }
                }
            }
            vanishingPlatformCycleTimer += TIME_PER_FRAME;
            if (vanishingPlatformCycleTimer.asSeconds() >= 2.f) {
                vanishingPlatformCycleTimer -= sf::seconds(2.f);
                oddEvenVanishing *= -1;
            }

            // --- 2. PLAYER PHYSICS ---
            sf::Vector2f currentPlayerVelocity = playerBody.getVelocity();
            currentPlayerVelocity.x = horizontalInput * PLAYER_MOVE_SPEED * turboMultiplier;

            if (!playerBody.isOnGround()) { // Check player's ground state
                currentPlayerVelocity.y += GRAVITY_ACCELERATION * TIME_PER_FRAME.asSeconds();
                if (currentPlayerVelocity.y > MAX_FALL_SPEED) currentPlayerVelocity.y = MAX_FALL_SPEED;
            }

            if (newJumpPressThisFrame) { 
                currentPlayerVelocity.y = JUMP_INITIAL_VELOCITY;
                currentJumpHoldDuration = sf::microseconds(1);
            } else if (jumpIntentThisFrame && currentJumpHoldDuration > sf::Time::Zero && currentJumpHoldDuration < MAX_JUMP_HOLD_TIME) {
                currentPlayerVelocity.y = JUMP_INITIAL_VELOCITY; 
                currentJumpHoldDuration += TIME_PER_FRAME;
            } else if (!jumpIntentThisFrame || currentJumpHoldDuration >= MAX_JUMP_HOLD_TIME) {
                currentJumpHoldDuration = sf::Time::Zero; 
            }
            playerBody.setVelocity(currentPlayerVelocity);

            // --- 3. COLLISION DETECTION AND RESOLUTION ---
            // Position update is now part of resolveCollisions' iterative process.
            // We do NOT call playerBody.setPosition(playerBody.getPosition() + playerBody.getVelocity() * TIME_PER_FRAME.asSeconds()); here.
            // That full movement is handled internally by the iterative resolver over deltaTime.
            phys::CollisionResolutionInfo resolutionResult = phys::CollisionSystem::resolveCollisions(playerBody, bodies, TIME_PER_FRAME.asSeconds());

            // --- 4. POST-COLLISION ADJUSTMENTS ---
            currentPlayerVelocity = playerBody.getVelocity(); // Get velocity modified by CollisionSystem

            if (playerBody.isOnGround()) {
                // Velocity.y should be 0 if on ground and not a special surface.
                // This is mostly handled by CollisionSystem. ApplyCollisionResponse, but a final clamp can be useful.
                if (currentPlayerVelocity.y > 0.f && playerBody.getGroundPlatform() &&
                    playerBody.getGroundPlatform()->getType() != phys::bodyType::conveyorBelt &&
                    playerBody.getGroundPlatform()->getType() != phys::bodyType::moving // Moving platforms might impart small Y if angled
                    ) {
                   // currentPlayerVelocity.y = 0.f; // Collision system should handle this zeroing.
                }
                currentJumpHoldDuration = sf::Time::Zero; // Reset jump hold when grounded

                if (playerBody.getGroundPlatform()) {
                    const phys::PlatformBody& collidedPf = *playerBody.getGroundPlatform();
                    if (collidedPf.getType() == phys::bodyType::conveyorBelt) {
                        // Apply surface velocity *as an additional movement*. Player should not get X-velocity set directly from conveyor.
                        playerBody.setPosition({playerBody.getPosition() + collidedPf.getSurfaceVelocity() * TIME_PER_FRAME.asSeconds()});
                    } else if (movingPlatformPtr && collidedPf.getID() == movingPlatformPtr->getID()) {
                        // Player is on the specific moving platform found earlier.
                        // Add the platform's displacement for THIS frame.
                        playerBody.setPosition({playerBody.getPosition() + movingPlatformFrameDisplacement});
                    }
                }
            }
            if (resolutionResult.hitCeiling) {
                if (currentPlayerVelocity.y < 0.f) currentPlayerVelocity.y = 0.f; // Should be handled by CollisionSystem
                currentJumpHoldDuration = sf::Time::Zero;
            }
            playerBody.setVelocity(currentPlayerVelocity); // Set final velocity after all adjustments
        } // End of fixed timestep loop

        // --- 5. Update Visuals & View ---
        playerShape.setPosition({playerBody.getPosition()});

        // Camera follow
        mainView.setCenter({playerBody.getPosition().x + playerBody.getWidth()/2.f,
                           playerBody.getPosition().y + playerBody.getHeight()/2.f - 50.f}); // Center on player, slightly offset Y

        // Clamp view to prevent seeing too far outside level boundaries (optional)
        // Example: float minViewX = 0, maxViewX = LEVEL_WIDTH - mainView.getSize().x;
        // if (mainView.getCenter().x - mainView.getSize().x / 2.f < minViewX) mainView.setCenter(minViewX + mainView.getSize().x / 2.f, mainView.getCenter().y);
        // ... similar for max and Y axis


        // Drawing
        window.clear(sf::Color(20, 20, 40));
        window.setView(mainView);
        for (const auto& t : tiles) {
            if (t.getFillColor().a > 5) window.draw(t); // Only draw visible tiles
        }
        window.draw(playerShape);

        // UI / Debug Text on Default View
        window.setView(window.getDefaultView());
        debugText.setString(
            "Player Pos: " + std::to_string(static_cast<int>(playerBody.getPosition().x)) + "," + std::to_string(static_cast<int>(playerBody.getPosition().y)) +
            "\nPlayer Vel: " + std::to_string(static_cast<int>(playerBody.getVelocity().x)) + "," + std::to_string(static_cast<int>(playerBody.getVelocity().y)) +
            "\nOnGround: " + (playerBody.isOnGround() ? "Yes" : "No") +
            (playerBody.getGroundPlatform() ? (" (ID: " + std::to_string(playerBody.getGroundPlatform()->getID()) + ")") : "") +
            "\nTryingToDrop: " + (playerBody.isTryingToDropFromPlatform() ? "Yes" : "No") +
             (playerBody.getGroundPlatformTemporarilyIgnored() ? "\nTempIgnoring: Yes" : "")
        );
        window.draw(debugText);


        window.display();
    } // End of game loop

    return 0;
}