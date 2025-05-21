// ... (Includes are fine) ...
#include "CollisionSystem.hpp"
#include "Player.hpp"
#include "PlatformBody.hpp"
#include "Tile.hpp"
#include "PhysicsTypes.hpp"
#include "LevelManager.hpp"
#include "Optimizer.hpp" // Your easing functions

// ... (GameState, GameSettings, Globals for Window/Resolution are fine) ...

// --- Global Game Objects (Modifications/Additions) ---
LevelManager levelManager; // Already global
LevelData currentLevelData;
phys::DynamicBody playerBody;
std::vector<phys::PlatformBody> bodies;
std::vector<Tile> tiles;

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

// ADDED for Interactible Platform runtime state
struct ActiveInteractiblePlatform {
    unsigned int id;
    std::string interactionType;
    phys::bodyType targetBodyTypeEnum; // Resolved enum value
    sf::Color targetTileColor;
    bool hasTargetTileColor;
    bool oneTime;
    float cooldown; // Original cooldown from JSON

    // Runtime state
    bool hasBeenInteractedThisSession; // For oneTime logic
    float currentCooldownTimer;        // Countdown timer
};
std::map<unsigned int, ActiveInteractiblePlatform> activeInteractibles; // Map ID to its active state

sf::Time vanishingPlatformCycleTimer = sf::Time::Zero;
int oddEvenVanishing = 1;

GameSettings gameSettings;

// ... (Audio objects and Asset Paths are fine) ...
// ... (playSfx, loadAudio are fine) ...

// Helper to get a default Tile color for a bodyType if not specified by interaction
sf::Color getTileColorForBodyType(phys::bodyType type, const sf::Color& defaultColorIfUnknown = sf::Color::Magenta) {
    switch (type) {
        case phys::bodyType::solid:        return sf::Color(100, 100, 100, 255);
        case phys::bodyType::platform:     return sf::Color(70, 150, 200, 180);
        case phys::bodyType::conveyorBelt: return sf::Color(255, 150, 50, 255);
        case phys::bodyType::moving:       return sf::Color(70, 200, 70, 255);
        case phys::bodyType::falling:      return sf::Color(200, 200, 70, 255);
        case phys::bodyType::vanishing:    return sf::Color(200, 70, 200, 255); // Vanishing itself might be complex to target
        case phys::bodyType::spring:       return sf::Color(255, 255, 0, 255);
        case phys::bodyType::trap:         return sf::Color(255, 20, 20, 255);
        case phys::bodyType::goal:         return sf::Color(20, 255, 20, 128);
        case phys::bodyType::interactible: return sf::Color(180, 180, 220, 200); // Color *before* interaction
        case phys::bodyType::none:         return sf::Color(0, 0, 0, 0); // Transparent
        default:                           return defaultColorIfUnknown;
    }
}


void setupLevelAssets(const LevelData& data, sf::RenderWindow& window) {
    bodies.clear();
    tiles.clear();
    activeMovingPlatforms.clear();
    activeInteractibles.clear(); // <-- CLEAR NEW MAP

    playerBody.setPosition(data.playerStartPosition);
    playerBody.setVelocity({0.f, 0.f});
    playerBody.setOnGround(false);
    playerBody.setGroundPlatform(nullptr);
    playerBody.setLastPosition(data.playerStartPosition);

    std::cout << "Setting up assets for level: " << data.levelName << " (Number: " << data.levelNumber << ")" << std::endl;
    std::cout << "  Player Start: (" << data.playerStartPosition.x << ", " << data.playerStartPosition.y << ")" << std::endl;
    std::cout << "  Platform Count: " << data.platforms.size() << std::endl;

    bodies.reserve(data.platforms.size());
    for (const auto& p_body_template : data.platforms) {
        bodies.push_back(p_body_template);

        if (p_body_template.getType() == phys::bodyType::moving) {
            // ... (existing moving platform setup logic is fine) ...
            bool foundDetail = false;
            for(const auto& detail : data.movingPlatformDetails){
                if(detail.id == p_body_template.getID()){
                    sf::Vector2f initial_platform_pos = p_body_template.getPosition();
                    float initialOffsetValue = 0.f;
                    if (detail.cycleDuration > 0 && detail.distance != 0) {
                         initialOffsetValue = math::easing::sineEaseInOut(0.f, 0.f, detail.initialDirection * detail.distance, detail.cycleDuration / 2.0f);
                    }
                    activeMovingPlatforms.push_back({
                        detail.id, detail.startPosition, detail.axis, detail.distance,
                        0.0f, detail.cycleDuration, detail.initialDirection, initial_platform_pos
                    });
                    sf::Vector2f actualInitialPos = detail.startPosition;
                     if (detail.axis == 'x') actualInitialPos.x += initialOffsetValue;
                     else if (detail.axis == 'y') actualInitialPos.y += initialOffsetValue;

                     if (std::abs(bodies.back().getPosition().x - actualInitialPos.x) > 0.1f ||
                         std::abs(bodies.back().getPosition().y - actualInitialPos.y) > 0.1f) {
                         bodies.back().setPosition(actualInitialPos);
                         activeMovingPlatforms.back().lastFramePosition = actualInitialPos;
                     }
                    foundDetail = true;
                    break;
                }
            }
            if(!foundDetail){
                std::cerr << "Warning: Moving platform ID " << p_body_template.getID()
                          << " listed in platforms, but missing movement details." << std::endl;
            }
        }
        // SETUP ACTIVE INTERACTIBLES
        else if (p_body_template.getType() == phys::bodyType::interactible) {
            bool foundDetail = false;
            for (const auto& detail : data.interactiblePlatformDetails) {
                if (detail.id == p_body_template.getID()) {
                    ActiveInteractiblePlatform activeDetail;
                    activeDetail.id = detail.id;
                    activeDetail.interactionType = detail.interactionType;
                    // Resolve targetBodyTypeEnum using the global levelManager instance
                    activeDetail.targetBodyTypeEnum = levelManager.stringToBodyType(detail.targetBodyTypeStr);
                    activeDetail.targetTileColor = detail.targetTileColor;
                    activeDetail.hasTargetTileColor = detail.hasTargetTileColor;
                    activeDetail.oneTime = detail.oneTime;
                    activeDetail.cooldown = detail.cooldown;
                    activeDetail.hasBeenInteractedThisSession = false; // Initial runtime state
                    activeDetail.currentCooldownTimer = 0.f;       // Initial runtime state

                    activeInteractibles[detail.id] = activeDetail;
                    foundDetail = true;
                    std::cout << "  Setup interactible platform ID " << detail.id
                              << " to target type: " << detail.targetBodyTypeStr
                              << " (Enum: " << static_cast<int>(activeDetail.targetBodyTypeEnum) << ")" << std::endl;
                    break;
                }
            }
            if (!foundDetail) {
                std::cerr << "Warning: Interactible platform ID " << p_body_template.getID()
                          << " listed in platforms, but missing interaction details." << std::endl;
            }
        }
    }

    tiles.reserve(bodies.size());
    for (size_t i = 0; i < bodies.size(); ++i) {
        const auto& body = bodies[i];
        Tile newTile(sf::Vector2f(body.getWidth(), body.getHeight()));
        newTile.setPosition(body.getPosition());
        // Use the helper function for default colors
        newTile.setFillColor(getTileColorForBodyType(body.getType()));
        tiles.push_back(newTile);
    }

    vanishingPlatformCycleTimer = sf::Time::Zero;
    oddEvenVanishing = 1;
}

// ... (updateResolutionDisplayText is fine) ...

int main(void) {
    // ... (Initial setup: resolutions, constants, window creation is fine) ...
    // ... (GameState, levelManager setup, loadAudio is fine) ...
    // ... (playerBody, Font, UI Text setup is fine) ...
    // ... (playerShape, clocks, turboMultiplier, interactKeyPressedThisFrame, debugText fine) ...
    // ... (Music play, running loop start fine) ...

    while (running) {
        interactKeyPressedThisFrame = false; // Reset at the start of each frame
        sf::Time frameDeltaTime = gameClock.restart();

        sf::Event event;
        while (window.pollEvent(event)) {
            // ... (Existing event handling is mostly fine) ...
            // In PLAYING state, the 'E' key for interactKeyPressedThisFrame is already captured.
             if (event.type == sf::Event::Closed) {
                running = false; window.close();
            }
             if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::F4) { // Cheat to next level
                if(currentState == GameState::PLAYING && levelManager.requestLoadNextLevel(currentLevelData)){
                     currentState = GameState::TRANSITIONING; playSfx("goal"); // Or a generic "cheat" sound
                } else if (currentState == GameState::PLAYING) { // No next level, treat as win
                    currentState = GameState::GAME_OVER_WIN; gameMusic.stop(); menuMusic.play();
                }
            }
            // ... (Rest of your event switch cases for MENU, SETTINGS, etc.)
            // Ensure PLAYING case captures interactKeyPressedThisFrame = true on E press.
            // Your existing code:
            // case GameState::PLAYING:
            //     if (event.type == sf::Event::KeyPressed) {
            //         ...
            //         } else if (event.key.code == sf::Keyboard::E) {
            //             interactKeyPressedThisFrame = true; // This is correct
            //         }
            //     }
            //     break;
            sf::Vector2i pixelPos = sf::Mouse::getPosition(window);
            sf::Vector2f worldPosUi = window.mapPixelToCoords(pixelPos, uiView);

            switch(currentState) {
                case GameState::MENU:
                    if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
                        playSfx("click");
                        if (startButtonText.getGlobalBounds().contains(worldPosUi)) {
                            levelManager.setCurrentLevelNumber(0); // Start from level 1
                            if (levelManager.requestLoadNextLevel(currentLevelData)) { // This will load level 1
                                currentState = GameState::TRANSITIONING;
                                menuMusic.stop(); gameMusic.play(); 
                                window.setTitle("Project T - Loading...");
                            } else { std::cerr << "MENU: Failed request to load initial level." << std::endl; }
                        } else if (settingsButtonText.getGlobalBounds().contains(worldPosUi)) {
                            currentState = GameState::SETTINGS;
                            updateResolutionDisplayText(); 
                        } else if (creditsButtonText.getGlobalBounds().contains(worldPosUi)) {
                            currentState = GameState::CREDITS;
                        } else if (exitButtonText.getGlobalBounds().contains(worldPosUi)) {
                            running = false; window.close();
                        }
                    }
                    if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) { running = false; window.close(); }
                    break;
                case GameState::SETTINGS:
                     if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
                        playSfx("click");
                        if (settingsBackText.getGlobalBounds().contains(worldPosUi)) currentState = GameState::MENU;
                        else if (musicVolDownText.getGlobalBounds().contains(worldPosUi)) {
                            gameSettings.musicVolume = std::max(0.f, gameSettings.musicVolume - 10.f);
                            menuMusic.setVolume(gameSettings.musicVolume); gameMusic.setVolume(gameSettings.musicVolume);
                        } else if (musicVolUpText.getGlobalBounds().contains(worldPosUi)) {
                            gameSettings.musicVolume = std::min(100.f, gameSettings.musicVolume + 10.f);
                            menuMusic.setVolume(gameSettings.musicVolume); gameMusic.setVolume(gameSettings.musicVolume);
                        }
                        else if (sfxVolDownText.getGlobalBounds().contains(worldPosUi)) gameSettings.sfxVolume = std::max(0.f, gameSettings.sfxVolume - 10.f);
                        else if (sfxVolUpText.getGlobalBounds().contains(worldPosUi)) gameSettings.sfxVolume = std::min(100.f, gameSettings.sfxVolume + 10.f);
                        else if (resolutionPrevText.getGlobalBounds().contains(worldPosUi)) {
                            if (!isFullscreen && !availableResolutions.empty()) {
                                currentResolutionIndex--; if (currentResolutionIndex < 0) currentResolutionIndex = availableResolutions.size() - 1;
                                applyAndRecreateWindow(window, uiView, mainView); updateResolutionDisplayText();
                            }
                        } else if (resolutionNextText.getGlobalBounds().contains(worldPosUi)) {
                             if (!isFullscreen && !availableResolutions.empty()) {
                                currentResolutionIndex++; if (currentResolutionIndex >= availableResolutions.size()) currentResolutionIndex = 0;
                                applyAndRecreateWindow(window, uiView, mainView); updateResolutionDisplayText();
                            }
                        } else if (fullscreenToggleText.getGlobalBounds().contains(worldPosUi)) {
                            isFullscreen = !isFullscreen;
                            applyAndRecreateWindow(window, uiView, mainView); updateResolutionDisplayText();
                        }
                     }
                     if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) currentState = GameState::MENU;
                    break;
                case GameState::CREDITS:
                    if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
                        playSfx("click");
                        if (creditsBackText.getGlobalBounds().contains(worldPosUi)) currentState = GameState::MENU;
                    }
                    if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) currentState = GameState::MENU;
                    break;
                case GameState::PLAYING:
                    if (event.type == sf::Event::KeyPressed) {
                        if (event.key.code == sf::Keyboard::Escape) {
                            currentState = GameState::MENU;
                            gameMusic.pause(); menuMusic.play();
                            window.setTitle("Project T (Menu - Paused)");
                        } else if (event.key.code == sf::Keyboard::R) {
                            playSfx("click");
                            if (levelManager.requestRespawnCurrentLevel(currentLevelData)) {
                                currentState = GameState::TRANSITIONING;
                                window.setTitle("Project T - Respawning...");
                            } else {std::cerr << "PLAYING: Failed respawn request.\n";}
                        } else if (event.key.code == sf::Keyboard::E) {
                            interactKeyPressedThisFrame = true; // This is correctly set
                        }
                    }
                    break;
                 case GameState::GAME_OVER_WIN:
                 case GameState::GAME_OVER_LOSE_FALL:
                 case GameState::GAME_OVER_LOSE_DEATH:
                    if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
                        playSfx("click");
                        if (gameOverOption1Text.getGlobalBounds().contains(worldPosUi)) {
                            if (currentState == GameState::GAME_OVER_LOSE_FALL || currentState == GameState::GAME_OVER_LOSE_DEATH) {
                                if (levelManager.requestRespawnCurrentLevel(currentLevelData)) { // Respawn current
                                    currentState = GameState::TRANSITIONING; gameMusic.play();
                                } else { currentState = GameState::MENU; gameMusic.stop(); menuMusic.play(); levelManager.setCurrentLevelNumber(0); }
                            } else if (currentState == GameState::GAME_OVER_WIN) { // Play Again (from level 1)
                                levelManager.setCurrentLevelNumber(0); // Reset to allow loading level 1
                                if (levelManager.requestLoadNextLevel(currentLevelData)) { // This will load level 1
                                    currentState = GameState::TRANSITIONING; menuMusic.stop(); gameMusic.play();
                                } else { currentState = GameState::MENU; menuMusic.play(); }
                            }
                        } else if (gameOverOption2Text.getGlobalBounds().contains(worldPosUi)) { // Main Menu
                            currentState = GameState::MENU; gameMusic.stop(); menuMusic.play();
                            levelManager.setCurrentLevelNumber(0); // Reset current level
                        }
                    }
                     if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
                        currentState = GameState::MENU; gameMusic.stop(); menuMusic.play(); levelManager.setCurrentLevelNumber(0);
                     }
                    break;
                case GameState::TRANSITIONING: // No input during transitions
                    break;
                default:
                    std::cerr << "Warning: Unhandled GameState in event loop: " << static_cast<int>(currentState) << std::endl;
                    currentState = GameState::MENU;
                    break;
            }
        }


        if (!running) break;

        timeSinceLastFixedUpdate += frameDeltaTime;

        if (currentState == GameState::PLAYING) {
            while (timeSinceLastFixedUpdate >= TIME_PER_FIXED_UPDATE) {
                timeSinceLastFixedUpdate -= TIME_PER_FIXED_UPDATE;
                playerBody.setLastPosition(playerBody.getPosition());

                // ... (Player input, movement, jump logic fine) ...
                float horizontalInput = 0.f;
                turboMultiplier = (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift)) ? 2 : 1;
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) horizontalInput = -1.f;
                else if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) horizontalInput = 1.f;

                bool jumpIntentThisFrame = (sf::Keyboard::isKeyPressed(sf::Keyboard::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Up) || sf::Keyboard::isKeyPressed(sf::Keyboard::Space));
                bool dropIntentThisFrame = (sf::Keyboard::isKeyPressed(sf::Keyboard::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Down));
                bool newJumpPressThisFrame = (jumpIntentThisFrame && playerBody.isOnGround() && currentJumpHoldDuration == sf::Time::Zero);
                if (newJumpPressThisFrame) playSfx("jump");
                playerBody.setTryingToDrop(dropIntentThisFrame && playerBody.isOnGround());


                // --- Update Moving Platforms ---
                for(auto& activePlat : activeMovingPlatforms) {
                    // ... (Existing moving platform update logic fine) ...
                    phys::PlatformBody* movingBodyPtr = nullptr; size_t tileIdx = (size_t)-1; 
                    for(size_t i=0; i < bodies.size(); ++i) if(bodies[i].getID() == activePlat.id) {movingBodyPtr = &bodies[i]; tileIdx = i; break;}

                    if (movingBodyPtr) {
                        activePlat.lastFramePosition = movingBodyPtr->getPosition();
                        activePlat.cycleTime += TIME_PER_FIXED_UPDATE.asSeconds();
                        float effectiveCycleDur = activePlat.cycleDuration > 0 ? activePlat.cycleDuration : 1.f;
                        activePlat.cycleTime = std::fmod(activePlat.cycleTime, effectiveCycleDur);

                        float singleMovePhaseDur = effectiveCycleDur / 2.0f;
                        float offset = 0.f;
                        if (singleMovePhaseDur > 0) {
                            float currentPhaseTime = activePlat.cycleTime;
                            if (currentPhaseTime < singleMovePhaseDur) {
                                offset = math::easing::sineEaseInOut(currentPhaseTime, 0.f, activePlat.direction * activePlat.distance, singleMovePhaseDur);
                            } else {
                                currentPhaseTime -= singleMovePhaseDur;
                                offset = math::easing::sineEaseInOut(currentPhaseTime, activePlat.direction * activePlat.distance, -activePlat.direction * activePlat.distance, singleMovePhaseDur);
                            }
                        }
                        sf::Vector2f newPos = activePlat.startPosition;
                        if(activePlat.axis == 'x') newPos.x += offset; else newPos.y += offset;
                        movingBodyPtr->setPosition(newPos);
                        if (tileIdx != (size_t)-1 && tileIdx < tiles.size()) { 
                            tiles[tileIdx].setPosition(newPos);
                        }
                    }
                }

                // --- Update Tiles (Falling, Vanishing) & Interactible Cooldowns ---
                for (auto& pair : activeInteractibles) { // Update cooldowns first
                    ActiveInteractiblePlatform& interactible = pair.second;
                    if (interactible.currentCooldownTimer > 0.f) {
                        interactible.currentCooldownTimer -= TIME_PER_FIXED_UPDATE.asSeconds();
                        if (interactible.currentCooldownTimer < 0.f) {
                            interactible.currentCooldownTimer = 0.f;
                        }
                    }
                }
                for (size_t i = 0; i < bodies.size(); ++i) {
                    // ... (Existing falling/vanishing tile logic fine) ...
                     if (tiles.size() <= i || (bodies[i].getType() == phys::bodyType::none && tiles[i].getFillColor().a == 0)) continue;
                    tiles[i].update(TIME_PER_FIXED_UPDATE); // Tile's own animation/fall
                    if (bodies[i].getType() == phys::bodyType::falling) {
                        bool playerOnThis = playerBody.isOnGround() && playerBody.getGroundPlatform() && playerBody.getGroundPlatform()->getID() == bodies[i].getID();
                        if (playerOnThis && !tiles[i].isFalling() && !tiles[i].hasFallen()) tiles[i].startFalling(sf::seconds(0.25f));
                        if (tiles[i].isFalling() && !bodies[i].isFalling()) bodies[i].setFalling(true); // Sync physics body
                        if (tiles[i].hasFallen() && bodies[i].getPosition().x > -9000.f) { // If tile visually fell, remove physics body
                            bodies[i].setPosition({-9999.f, -9999.f});
                            bodies[i].setType(phys::bodyType::none); // Also mark as none
                        }
                    } else if (bodies[i].getType() == phys::bodyType::vanishing) {
                        bool is_even_id = (bodies[i].getID() % 2 == 0);
                        bool should_vanish_now = (oddEvenVanishing == 1 && is_even_id) || (oddEvenVanishing == -1 && !is_even_id);
                        float phaseTime = std::fmod(vanishingPlatformCycleTimer.asSeconds(), 1.0f);
                        sf::Color originalColor = getTileColorForBodyType(phys::bodyType::vanishing); // Use helper for consistency
                        float alpha_val;
                        if (should_vanish_now) alpha_val = math::easing::sineEaseInOut(phaseTime, 255.f, -255.f, 1.f);
                        else alpha_val = math::easing::sineEaseInOut(phaseTime, 0.f, 255.f, 1.f);
                        alpha_val = std::max(0.f, std::min(255.f, alpha_val));
                        tiles[i].setFillColor(sf::Color(originalColor.r, originalColor.g, originalColor.b, static_cast<sf::Uint8>(alpha_val)));
                        
                        if (alpha_val <= 10.f && bodies[i].getType() != phys::bodyType::none) { // Vanished
                            bodies[i].setType(phys::bodyType::none); // Mark as none for collision
                            bodies[i].setPosition({-9999.f,-9999.f}); // Move away
                        } else if (alpha_val > 10.f && bodies[i].getType() == phys::bodyType::none) { // Reappearing
                            for(const auto& templ : currentLevelData.platforms) {
                                if(templ.getID() == bodies[i].getID()){
                                    bodies[i].setPosition(templ.getPosition()); 
                                    tiles[i].setPosition(templ.getPosition()); 
                                    bodies[i].setType(phys::bodyType::vanishing); // Restore original type
                                    break;
                                }
                            }
                        }
                    }
                }
                vanishingPlatformCycleTimer += TIME_PER_FIXED_UPDATE;
                if (vanishingPlatformCycleTimer.asSeconds() >= 1.f) {
                    vanishingPlatformCycleTimer -= sf::seconds(1.f); oddEvenVanishing *= -1;
                }


                // --- Player Physics Update ---
                sf::Vector2f pVel = playerBody.getVelocity();
                // ... (Gravity, jump velocity application fine) ...
                pVel.x = horizontalInput * PLAYER_MOVE_SPEED * turboMultiplier;
                if (!playerBody.isOnGround()) {
                    pVel.y += GRAVITY_ACCELERATION * TIME_PER_FIXED_UPDATE.asSeconds();
                    pVel.y = std::min(pVel.y, MAX_FALL_SPEED);
                }
                if (newJumpPressThisFrame) { pVel.y = JUMP_INITIAL_VELOCITY; currentJumpHoldDuration = sf::microseconds(1); }
                else if (jumpIntentThisFrame && currentJumpHoldDuration > sf::Time::Zero && currentJumpHoldDuration < MAX_JUMP_HOLD_TIME) {
                    pVel.y = JUMP_INITIAL_VELOCITY; currentJumpHoldDuration += TIME_PER_FIXED_UPDATE;
                } else if (!jumpIntentThisFrame || currentJumpHoldDuration >= MAX_JUMP_HOLD_TIME) {
                    currentJumpHoldDuration = sf::Time::Zero;
                }
                playerBody.setVelocity(pVel);


                // --- Collision Resolution ---
                phys::CollisionResolutionInfo resolutionResult = phys::CollisionSystem::resolveCollisions(playerBody, bodies, TIME_PER_FIXED_UPDATE.asSeconds());
                // ... (Post-collision velocity adjustments, ground platform effects fine) ...
                 pVel = playerBody.getVelocity(); // Re-fetch after collision response
                if (playerBody.isOnGround()) {
                    currentJumpHoldDuration = sf::Time::Zero;
                    if (playerBody.getGroundPlatform()) {
                        const phys::PlatformBody& pf = *playerBody.getGroundPlatform();
                        if (pf.getType() == phys::bodyType::conveyorBelt) {
                            playerBody.setPosition(playerBody.getPosition() + pf.getSurfaceVelocity() * TIME_PER_FIXED_UPDATE.asSeconds());
                        } else if (pf.getType() == phys::bodyType::moving) {
                             for(const auto& activePlat : activeMovingPlatforms) {
                                if (activePlat.id == pf.getID()) {
                                    phys::PlatformBody* movingPhysBody = nullptr;
                                    for(auto& b_ref : bodies) if(b_ref.getID() == activePlat.id) {movingPhysBody = &b_ref; break;} // Note: b renamed to b_ref
                                    if(movingPhysBody){
                                        sf::Vector2f platformDisp = movingPhysBody->getPosition() - activePlat.lastFramePosition;
                                        playerBody.setPosition(playerBody.getPosition() + platformDisp);
                                    }
                                    break;
                                }
                            }
                        }
                    }
                }
                if (resolutionResult.hitCeiling && pVel.y < 0.f) { pVel.y = 0.f; currentJumpHoldDuration = sf::Time::Zero;}
                playerBody.setVelocity(pVel);


                // --- Trap Check ---
                bool trapHit = false;
                for (const auto& body : bodies) {
                    if (body.getType() == phys::bodyType::trap && playerBody.getAABB().intersects(body.getAABB())) {
                        trapHit = true; break;
                    }
                }
                if (trapHit) {
                    playSfx("death");
                    currentState = GameState::GAME_OVER_LOSE_DEATH;
                    gameMusic.pause(); // Or stop and play menu music later
                    break; // Break from fixed update loop
                }

                // --- Goal & Interactible Check (if E was pressed) ---
                if (interactKeyPressedThisFrame) {
                    // Goal check first (as it's a level end)
                    for (const auto& platform : bodies) {
                        if (platform.getType() == phys::bodyType::goal && playerBody.getAABB().intersects(platform.getAABB())) {
                            playSfx("goal");
                            if (levelManager.hasNextLevel()) {
                                if (levelManager.requestLoadNextLevel(currentLevelData)) {
                                    currentState = GameState::TRANSITIONING;
                                } else { currentState = GameState::MENU; gameMusic.stop(); menuMusic.play(); }
                            } else {
                                currentState = GameState::GAME_OVER_WIN;
                                gameMusic.stop(); menuMusic.play();
                            }
                            goto end_fixed_update_early; // Exit fixed update if state changed
                        }
                    }

                    // Interactible check
                    for (size_t i = 0; i < bodies.size(); ++i) {
                        // Need non-const ref to change type, so access `bodies[i]` directly
                        if (bodies[i].getType() == phys::bodyType::interactible && playerBody.getAABB().intersects(bodies[i].getAABB())) {
                            auto it = activeInteractibles.find(bodies[i].getID());
                            if (it != activeInteractibles.end()) {
                                ActiveInteractiblePlatform& interactState = it->second;

                                if (interactState.currentCooldownTimer > 0.f) continue; // Still on cooldown
                                if (interactState.oneTime && interactState.hasBeenInteractedThisSession) continue; // Used up

                                if (interactState.interactionType == "changeSelf") {
                                    playSfx("click"); // Or a specific interaction sound

                                    bodies[i].setType(interactState.targetBodyTypeEnum);
                                    std::cout << "Platform " << bodies[i].getID() << " interacted, new type: "
                                              << static_cast<int>(interactState.targetBodyTypeEnum) << std::endl;


                                    if (tiles.size() > i) {
                                        if (interactState.hasTargetTileColor) {
                                            tiles[i].setFillColor(interactState.targetTileColor);
                                        } else {
                                            tiles[i].setFillColor(getTileColorForBodyType(interactState.targetBodyTypeEnum));
                                        }
                                    }
                                    
                                    // If changed to 'none', effectively remove it
                                    if (interactState.targetBodyTypeEnum == phys::bodyType::none) {
                                        bodies[i].setPosition({-10000.f, -10000.f}); // Move far away
                                        if (tiles.size() > i) tiles[i].setFillColor(sf::Color::Transparent);
                                    }


                                    if (interactState.oneTime) {
                                        interactState.hasBeenInteractedThisSession = true;
                                    } else {
                                        interactState.currentCooldownTimer = interactState.cooldown;
                                    }
                                    break; // Interact with one platform per key press
                                }
                            }
                        }
                    }
                }
                end_fixed_update_early:; // Label for goto

                // --- Death by Falling ---
                if (playerBody.getPosition().y > PLAYER_DEATH_Y_LIMIT) {
                    playSfx("death");
                    currentState = GameState::GAME_OVER_LOSE_FALL;
                    gameMusic.pause();
                    break; // Break from fixed update loop
                }
            } // End of fixed update loop (while timeSinceLastUpdate >= TIME_PER_FIXED_UPDATE)
        } // End of PLAYING state update
        else if (currentState == GameState::TRANSITIONING) {
            // ... (Existing transition logic is fine) ...
            levelManager.update(frameDeltaTime.asSeconds(), window);
            if (!levelManager.isTransitioning()) {
                setupLevelAssets(currentLevelData, window); // This now sets up interactibles too
                currentState = GameState::PLAYING;
                if (gameMusic.getStatus() != sf::Music::Playing) { // Ensure game music plays
                     menuMusic.stop(); gameMusic.play();
                }
            }
        }

        window.setTitle("Project T");

        // --- DRAW PHASE ---
        window.clear(sf::Color::Black);
        sf::Vector2i currentMousePixelPos = sf::Mouse::getPosition(window);
        sf::Vector2f currentMouseWorldUiPos = window.mapPixelToCoords(currentMousePixelPos, uiView);

        switch(currentState) {
            // ... (MENU, SETTINGS, CREDITS draw logic fine) ...
             case GameState::MENU:
                window.setView(uiView);
                if(menuBgSprite.getTexture()) { window.draw(menuBgSprite); } 
                else { sf::RectangleShape bg(LOGICAL_SIZE); bg.setFillColor(sf::Color(20,20,50)); window.draw(bg); }
                startButtonText.setFillColor(defaultBtnColor); settingsButtonText.setFillColor(defaultBtnColor);
                creditsButtonText.setFillColor(defaultBtnColor); exitButtonText.setFillColor(defaultBtnColor);
                if(startButtonText.getGlobalBounds().contains(currentMouseWorldUiPos)) startButtonText.setFillColor(hoverBtnColor);
                if(settingsButtonText.getGlobalBounds().contains(currentMouseWorldUiPos)) settingsButtonText.setFillColor(hoverBtnColor);
                if(creditsButtonText.getGlobalBounds().contains(currentMouseWorldUiPos)) creditsButtonText.setFillColor(hoverBtnColor);
                if(exitButtonText.getGlobalBounds().contains(currentMouseWorldUiPos)) exitButtonText.setFillColor(exitBtnHoverColor);
                window.draw(menuTitleText); window.draw(startButtonText); window.draw(settingsButtonText);
                window.draw(creditsButtonText); window.draw(exitButtonText);
                break;
            case GameState::SETTINGS:
                window.setView(uiView);
                { sf::RectangleShape bg(LOGICAL_SIZE); bg.setFillColor(sf::Color(20,50,20)); window.draw(bg); }
                settingsBackText.setFillColor(defaultBtnColor);
                if(settingsBackText.getGlobalBounds().contains(currentMouseWorldUiPos)) settingsBackText.setFillColor(hoverBtnColor);
                musicVolDownText.setFillColor(defaultBtnColor); musicVolUpText.setFillColor(defaultBtnColor);
                if(musicVolDownText.getGlobalBounds().contains(currentMouseWorldUiPos)) musicVolDownText.setFillColor(hoverBtnColor);
                if(musicVolUpText.getGlobalBounds().contains(currentMouseWorldUiPos)) musicVolUpText.setFillColor(hoverBtnColor);
                sfxVolDownText.setFillColor(defaultBtnColor); sfxVolUpText.setFillColor(defaultBtnColor);
                if(sfxVolDownText.getGlobalBounds().contains(currentMouseWorldUiPos)) sfxVolDownText.setFillColor(hoverBtnColor);
                if(sfxVolUpText.getGlobalBounds().contains(currentMouseWorldUiPos)) sfxVolUpText.setFillColor(hoverBtnColor);
                resolutionPrevText.setFillColor(defaultBtnColor); resolutionNextText.setFillColor(defaultBtnColor);
                fullscreenToggleText.setFillColor(defaultBtnColor);
                if(resolutionPrevText.getGlobalBounds().contains(currentMouseWorldUiPos) && !isFullscreen) resolutionPrevText.setFillColor(hoverBtnColor);
                if(resolutionNextText.getGlobalBounds().contains(currentMouseWorldUiPos) && !isFullscreen) resolutionNextText.setFillColor(hoverBtnColor);
                if(fullscreenToggleText.getGlobalBounds().contains(currentMouseWorldUiPos)) fullscreenToggleText.setFillColor(hoverBtnColor);
                window.draw(settingsTitleText);
                musicVolValText.setString(std::to_string(static_cast<int>(gameSettings.musicVolume))+"%");
                sfxVolValText.setString(std::to_string(static_cast<int>(gameSettings.sfxVolume))+"%");
                window.draw(musicVolumeLabelText); window.draw(musicVolDownText); window.draw(musicVolValText); window.draw(musicVolUpText);
                window.draw(sfxVolumeLabelText); window.draw(sfxVolDownText); window.draw(sfxVolValText); window.draw(sfxVolUpText);
                window.draw(resolutionLabelText); window.draw(resolutionPrevText); window.draw(resolutionCurrentText); window.draw(resolutionNextText);
                window.draw(fullscreenToggleText);
                window.draw(settingsBackText);
                break;
            case GameState::CREDITS:
                window.setView(uiView);
                { sf::RectangleShape bg(LOGICAL_SIZE); bg.setFillColor(sf::Color(50,20,20)); window.draw(bg); }
                creditsBackText.setFillColor(defaultBtnColor);
                if(creditsBackText.getGlobalBounds().contains(currentMouseWorldUiPos)) creditsBackText.setFillColor(hoverBtnColor);
                window.draw(creditsTitleText); window.draw(creditsNamesText); window.draw(creditsBackText);
                break;
            case GameState::PLAYING:
                // ... (View setup, background, player, tiles drawing fine) ...
                mainView.setCenter( playerBody.getPosition().x + playerBody.getWidth()/2.f, playerBody.getPosition().y + playerBody.getHeight()/2.f - 50.f); // Camera
                window.setView(mainView);
                { // Draw background covering the camera view
                    sf::RectangleShape gameplayAreaBackground(mainView.getSize()); 
                    gameplayAreaBackground.setFillColor(currentLevelData.backgroundColor);
                    gameplayAreaBackground.setPosition(mainView.getCenter() - mainView.getSize() / 2.f); 
                    window.draw(gameplayAreaBackground);
                }
                playerShape.setPosition(playerBody.getPosition());
                for (const auto& t : tiles) { if (t.getFillColor().a > 5) window.draw(t); } // Draw if not fully transparent
                window.draw(playerShape);
                
                window.setView(uiView); // Switch to UI view for debug text
                debugText.setString( "Lvl: " + std::to_string(currentLevelData.levelNumber) +
                                     " Pos: " + std::to_string(static_cast<int>(playerBody.getPosition().x)) + "," + std::to_string(static_cast<int>(playerBody.getPosition().y)) +
                                     " Vel: " + std::to_string(static_cast<int>(playerBody.getVelocity().x)) + "," + std::to_string(static_cast<int>(playerBody.getVelocity().y)) +
                                     " Ground: " + (playerBody.isOnGround() ? "Y" : "N") +
                                     (playerBody.getGroundPlatform() ? (" (ID:" + std::to_string(playerBody.getGroundPlatform()->getID()) +")"):""));
                window.draw(debugText);
                break;
            case GameState::TRANSITIONING:
                // ... (Transition draw logic fine) ...
                window.setView(uiView);
                { sf::RectangleShape transitionBg(LOGICAL_SIZE); transitionBg.setFillColor(sf::Color::Black); transitionBg.setPosition(0,0); window.draw(transitionBg); }
                levelManager.draw(window);
                break;
            // ... (GAME_OVER_WIN, GAME_OVER_LOSE_FALL, GAME_OVER_LOSE_DEATH draw logic fine) ...
            case GameState::GAME_OVER_WIN:
                 window.setView(uiView);
                 { sf::RectangleShape bg(LOGICAL_SIZE); bg.setFillColor(sf::Color(20,60,20)); window.draw(bg); }
                 gameOverStatusText.setString("All Levels Cleared! You Win!");
                 gameOverOption1Text.setString("Play Again (Level 1)");
                 gameOverOption1Text.setFillColor(defaultBtnColor); gameOverOption2Text.setFillColor(defaultBtnColor);
                 if(gameOverOption1Text.getGlobalBounds().contains(currentMouseWorldUiPos)) gameOverOption1Text.setFillColor(hoverBtnColor);
                 if(gameOverOption2Text.getGlobalBounds().contains(currentMouseWorldUiPos)) gameOverOption2Text.setFillColor(hoverBtnColor);
                 window.draw(gameOverStatusText); window.draw(gameOverOption1Text); window.draw(gameOverOption2Text);
                break;
            case GameState::GAME_OVER_LOSE_FALL:
                window.setView(uiView);
                { sf::RectangleShape bg(LOGICAL_SIZE); bg.setFillColor(sf::Color(60,20,20)); window.draw(bg); }
                gameOverStatusText.setString("Game Over! You Fell!");
                gameOverOption1Text.setString("Retry Level");
                gameOverOption1Text.setFillColor(defaultBtnColor); gameOverOption2Text.setFillColor(defaultBtnColor);
                if(gameOverOption1Text.getGlobalBounds().contains(currentMouseWorldUiPos)) gameOverOption1Text.setFillColor(hoverBtnColor);
                if(gameOverOption2Text.getGlobalBounds().contains(currentMouseWorldUiPos)) gameOverOption2Text.setFillColor(hoverBtnColor);
                window.draw(gameOverStatusText); window.draw(gameOverOption1Text); window.draw(gameOverOption2Text);
                break;
            case GameState::GAME_OVER_LOSE_DEATH: // Trap death
                window.setView(uiView);
                { sf::RectangleShape bg(LOGICAL_SIZE); bg.setFillColor(sf::Color(70,10,10)); window.draw(bg); }
                gameOverStatusText.setString("Game Over! Hit a Trap!");
                gameOverOption1Text.setString("Retry Level");
                gameOverOption1Text.setFillColor(defaultBtnColor); gameOverOption2Text.setFillColor(defaultBtnColor);
                if(gameOverOption1Text.getGlobalBounds().contains(currentMouseWorldUiPos)) gameOverOption1Text.setFillColor(hoverBtnColor);
                if(gameOverOption2Text.getGlobalBounds().contains(currentMouseWorldUiPos)) gameOverOption2Text.setFillColor(hoverBtnColor);
                window.draw(gameOverStatusText); window.draw(gameOverOption1Text); window.draw(gameOverOption2Text);
                break;
             default: // Should not happen
                 window.setView(uiView);
                 { sf::RectangleShape bg(LOGICAL_SIZE); bg.setFillColor(sf::Color::Magenta); window.draw(bg); }
                 sf::Text errorText("Unknown Game State!", menuFont, 20); errorText.setOrigin(errorText.getLocalBounds().width/2.f, errorText.getLocalBounds().height/2.f); errorText.setPosition(LOGICAL_SIZE.x/2.f, LOGICAL_SIZE.y/2.f); window.draw(errorText);
                 break;
        }
        window.display();
    }

    menuMusic.stop(); gameMusic.stop();
    return 0;
}