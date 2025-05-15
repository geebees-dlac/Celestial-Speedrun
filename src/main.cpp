#include <iostream>
#include <SFML/Graphics.hpp>
#include <vector>     // Recommended over C-style arrays
#include <cmath>      // For std::abs, std::cos, std::pow
#include <string>     // For sf::String, std::string
#include <cstdlib>   // For std::abs, std::rand, std::srand
#include <ctime>     // For std::time
#include <algorithm> // For std::max, std::min
#include <limits>    // For std::numeric_limits
#include <cassert>   // For assert
#include <functional> // For std::function
#include <memory>    // For std::unique_ptr, std::shared_ptr
#include <array>     // For std::array
#include <utility>   // For std::pair, std::make_pair
#include <type_traits> // For std::is_same, std::enable_if
#include <cstddef>   // For std::size_t
#include <cstdint>   // For std::int32_t, std::uint32_t
#include <cfloat>    // For FLT_MAX, FLT_MIN
#include <filesystem>
#include <SFML/System.hpp> // For sf::Time, sf::seconds
#include <SFML/Window.hpp> // For sf::Event
#include <SFML/Graphics/RenderWindow.hpp> // For sf::RenderWindow
#include <SFML/Graphics/Font.hpp> // For sf::Font
#include <SFML/Graphics/Text.hpp> // For sf::Text
#include <SFML/Graphics/RectangleShape.hpp> // For sf::RectangleShape
#include <SFML/Graphics/RenderStates.hpp> // For sf::RenderStates
#include <SFML/Graphics/RenderTarget.hpp> // For sf::RenderTarget
#include <SFML/Graphics/View.hpp> // For sf::View
#include <SFML/Graphics/Transformable.hpp> // For sf::Transformable
#include <SFML/Graphics/Drawable.hpp> // For sf::Drawable
#include <SFML/Graphics/Color.hpp> // For sf::Color



// Use C++20 <numbers> if available, otherwise define M_PI precisely
#if __cplusplus >= 202002L && __has_include(<numbers>)
#include <numbers>
#define M_PI std::numbers::pi_v<float>
#else
#ifndef M_PI
#define M_PI 3.14159265358979323846f // Add 'f' for float
#endif
#endif

#include <CollisionSystem.hpp>
#include <dynamicBody.hpp>
#include <platformBody.hpp>
#include <tile.hpp>
#include <interpolate.hpp> 
#include <PhysicsTypes.hpp>


enum class GameState {
    MENU,
    PLAYING,
    // PAUSED // Future state
};

int main(void) {

    const sf::Vector2f tileSize(32.f, 32.f); // Use constructor initialization
    const int NUM_PLATFORM_OBJECTS = 24;

    // Player physics constants (tune these to match desired feel)
    const float PLAYER_MOVE_SPEED = 200.f;
    const float JUMP_INITIAL_VELOCITY = -400.f; // Negative for upward velocity
    const float GRAVITY_ACCELERATION = 980.f;   // Pixels/second^2
    const float MAX_FALL_SPEED = 700.f;       // Max downward speed due to gravity
    const sf::Time MAX_JUMP_HOLD_TIME = sf::seconds(0.15f); // How long holding jump affects ascent

    sf::RenderWindow window(sf::VideoMode(800, 600), "Project - T", sf::Style::Default);
    window.setKeyRepeatEnabled(false);
    window.setVerticalSyncEnabled(true);
    window.setFramerateLimit(60);
    sf::Event e;

    GameState currentState = GameState::MENU; // Default to MENU state

    // --- Menu Resources ---
    sf::Font menuFont;
    std::string fontPath = "assets/ARIALBD.TTF"; // Ensure this path is correct
    if (!menuFont.loadFromFile(fontPath)) {
        std::cerr << "Failed to load font from '" << fontPath << "'" << std::endl;
        return -1;
    }
    // (Menu text and button setup - assuming this is largely unchanged from your previous single-file version)
    // ... your menu setup code ... (Start, Settings, Exit buttons)
    sf::Text startButtonText, settingsButtonText, exitButtonText;
    // ... (use your setupButton lambda or similar) ...
    // Example minimal setup if not copying from previous snippet:
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


    //set up the view for PLAYING state
    sf::View mainView(sf::FloatRect(0.f, 0.f, 800.f, 600.f)); // Start with view matching window
    // mainView.setCenter(...) will be updated based on player in PLAYING state.


    phys::CollisionSystem collisionSys;

    // Use std::vector for more flexibility if NUM_PLATFORM_OBJECTS might change
    std::vector<phys::PlatformBody> bodies(NUM_PLATFORM_OBJECTS);
    for (int i = 0; i < NUM_PLATFORM_OBJECTS; i++) {
        bodies[i].m_id = i;
        bodies[i].m_width = tileSize.x * 4;
        bodies[i].m_height = tileSize.y;
        bodies[i].m_type = phys::bodyType::platform; // Default
    }

    // --- Platform Setup (same as your code, positions and types) ---
    bodies[0].m_position = sf::Vector2f(0.f, window.getSize().y - 32.f);
    bodies[1].m_position = sf::Vector2f(0.f, 356.f);
    bodies[2].m_position = sf::Vector2f(480.f, window.getSize().y - 192.f);
    bodies[3].m_position = sf::Vector2f(0.f, 96.f);
    bodies[4].m_position = sf::Vector2f(640.f, 224.f);
    bodies[5].m_position = sf::Vector2f(480.f, window.getSize().y - 32.f);
    bodies[6].m_position = sf::Vector2f(672.f, window.getSize().y - 260.f);
    bodies[7].m_position = sf::Vector2f(96.f, window.getSize().y - 128.f);
    bodies[8].m_position = sf::Vector2f(640.f, window.getSize().y - 160.f); // .f for consistency
    bodies[9].m_position = sf::Vector2f(640.f, 64.f);
    bodies[10].m_position = sf::Vector2f(292.f, window.getSize().y - 128.f); // .f for consistency

    bodies[11].m_position = sf::Vector2f(window.getSize().x / 4.f, window.getSize().y / 2.f);
    bodies[11].m_width = tileSize.x * 10;
    bodies[11].m_height = tileSize.y;
    bodies[11].m_type = phys::bodyType::conveyorBelt;
    bodies[11].m_surfaceVelocity = 70.f; // Adjusted for per-second velocity

    bodies[12].m_height = tileSize.y / 8.f; // .f
    bodies[12].m_width = tileSize.x * 5;
    bodies[12].m_position = sf::Vector2f(window.getSize().x / 4.f, window.getSize().y / 4.f); // .f
    bodies[12].m_type = phys::bodyType::moving;
    int platformDir = 1;
    float platformFrameVelocity = 0.f; // Renamed to clarify it's delta per frame

    bodies[13].m_width = tileSize.x;
    bodies[13].m_height = tileSize.y / 8.f; // .f
    bodies[13].m_position = sf::Vector2f(160.f, window.getSize().y - 224.f);
    bodies[13].m_type = phys::bodyType::jumpthrough;

    bodies[14].m_width = tileSize.x;
    bodies[14].m_height = tileSize.y / 8.f; // .f
    bodies[14].m_position = sf::Vector2f(576.f, window.getSize().y - 288.f); // .f
    bodies[14].m_type = phys::bodyType::jumpthrough;

    // --- Falling and Vanishing Platforms (setup same as your code) ---
    // bodies[15] to bodies[19] for falling
    for(int i=15; i<=19; ++i) {
        bodies[i].m_width = tileSize.x; bodies[i].m_height = tileSize.y;
        bodies[i].m_position = sf::Vector2f(192.f + (i-15)*tileSize.x, 64.f);
        bodies[i].m_type = phys::bodyType::falling;
    }
    // bodies[20] to bodies[23] for vanishing
    for(int i=20; i<=23; ++i) {
        bodies[i].m_width = tileSize.x; bodies[i].m_height = tileSize.y;
        bodies[i].m_position = sf::Vector2f(192.f + (i-20)*tileSize.x, -32.f);
        bodies[i].m_type = phys::bodyType::vanishing;
    }


    std::vector<Tile> tiles(NUM_PLATFORM_OBJECTS); // Using tile class
    for (int i = 0; i < NUM_PLATFORM_OBJECTS; ++i) {
        tiles[i].setPosition(bodies[i].m_position);
        tiles[i].setSize(sf::Vector2f(bodies[i].m_width, bodies[i].m_height));
        switch (bodies[i].m_type) {
            case phys::bodyType::platform:     tiles[i].setFillColor(sf::Color(200, 70, 70, 255)); break;
            case phys::bodyType::conveyorBelt: tiles[i].setFillColor(sf::Color(255, 150, 50, 255)); break;
            case phys::bodyType::moving:       tiles[i].setFillColor(sf::Color(70, 200, 70, 255)); break;
            case phys::bodyType::jumpthrough:  tiles[i].setFillColor(sf::Color(70, 150, 200, 180)); break; // Semi-transparent
            case phys::bodyType::falling:      tiles[i].setFillColor(sf::Color(200, 200, 70, 255)); break;
            case phys::bodyType::vanishing:    tiles[i].setFillColor(sf::Color(200, 70, 200, 255)); break;
            default:                           tiles[i].setFillColor(sf::Color(128, 128, 128, 255)); break;
        }
    }

    phys::DynamicBody playerBody;
    playerBody.m_position = sf::Vector2f(window.getSize().x / 2.f, window.getSize().y / 2.f - 100.f); // Start a bit higher
    playerBody.m_width = tileSize.x;
    playerBody.m_height = tileSize.y;
    playerBody.m_velocity = sf::Vector2f(0.f, 0.f);
    playerBody.m_lastPosition = playerBody.m_position;

    sf::RectangleShape playerShape; // Visual representation
    playerShape.setFillColor(sf::Color(220, 220, 250, 255));
    playerShape.setSize(sf::Vector2f(playerBody.m_width, playerBody.m_height));
    playerShape.setPosition(playerBody.m_position);

    sf::Clock tickClock;
    sf::Time timeSinceLastUpdate = sf::Time::Zero;
    const sf::Time TIME_PER_FRAME = sf::seconds(1.f / 60.f); // Fixed timestep

    // Platform behavior timers
    sf::Time movingPlatformCycleTimer = sf::Time::Zero;
    sf::Time vanishingPlatformCycleTimer = sf::Time::Zero;
    int oddEvenVanishing = 1;

    // Player state variables for input and physics logic
    bool jumpKeyHeld = false;         // Is the jump key/button currently down?
    sf::Time currentJumpHoldDuration = sf::Time::Zero; // How long has jump been held in current ascent
    bool playerIsGrounded_lastFrame = false; // Grounded state from PREVIOUS fixed update for input logic


    // Joystick info
    if (sf::Joystick::isConnected(0)) {
        sf::Joystick::Identification id = sf::Joystick::getIdentification(0);
        std::cout << "\nJoystick Connected: " << id.name.toAnsiString()
                  << " (Vendor ID: " << id.vendorId << ", Product ID: " << id.productId << ")" << std::endl;
        unsigned int buttonCount = sf::Joystick::getButtonCount(0);
        std::cout << "Button count: " << buttonCount << std::endl;
    } else {
        std::cout << "No joystick connected." << std::endl;
    }
    int turboMultiplier = 1;
    int debugMode = -1; // -1 for off, 1 for on

    // Main Game Loop
    bool running = true;
    while (running) {

        // --- MENU STATE ---
        if (currentState == GameState::MENU) {
            window.setTitle("Project - T (Menu)");
            sf::Event menuEvent;
            while (window.pollEvent(menuEvent)) {
                if (menuEvent.type == sf::Event::Closed) { running = false; window.close(); }
                if (menuEvent.type == sf::Event::KeyPressed && menuEvent.key.code == sf::Keyboard::Escape) { running = false; window.close(); }

                sf::Vector2f mousePosView = window.mapPixelToCoords(sf::Mouse::getPosition(window), window.getDefaultView());
                // Reset button colors
                startButtonText.setFillColor(sf::Color::White);
                settingsButtonText.setFillColor(sf::Color::White);
                exitButtonText.setFillColor(sf::Color::White);

                bool mouseLButtonPressed = sf::Mouse::isButtonPressed(sf::Mouse::Left);

                if (startButtonText.getGlobalBounds().contains(mousePosView)) {
                    startButtonText.setFillColor(mouseLButtonPressed ? sf::Color::Green : sf::Color::Yellow);
                    if (menuEvent.type == sf::Event::MouseButtonReleased && menuEvent.mouseButton.button == sf::Mouse::Left) {
                        currentState = GameState::PLAYING;
                        tickClock.restart(); timeSinceLastUpdate = sf::Time::Zero; // Reset timer for play state
                    }
                } else if (settingsButtonText.getGlobalBounds().contains(mousePosView)) {
                    settingsButtonText.setFillColor(mouseLButtonPressed ? sf::Color::Green : sf::Color::Yellow);
                     // if (menuEvent.type == sf::Event::MouseButtonReleased && menuEvent.mouseButton.button == sf::Mouse::Left) { /* NYI */ }
                } else if (exitButtonText.getGlobalBounds().contains(mousePosView)) {
                    exitButtonText.setFillColor(mouseLButtonPressed ? sf::Color::Red : sf::Color::Yellow);
                    if (menuEvent.type == sf::Event::MouseButtonReleased && menuEvent.mouseButton.button == sf::Mouse::Left) { running = false; window.close(); }
                }
            }
            window.clear(sf::Color(30, 30, 70));
            window.setView(window.getDefaultView());
            window.draw(startButtonText); window.draw(settingsButtonText); window.draw(exitButtonText);
            window.display();
            if (!running) break;
            continue; // Skip PLAYING state if still in MENU
        }


        // --- PLAYING STATE ---
        window.setTitle("Project - T"); // Or joystick name if you prefer
        // Standard event polling for PLAYING state
        while (window.pollEvent(e)) {
            if (e.type == sf::Event::Closed) { running = false; window.close(); }
            if (e.type == sf::Event::KeyPressed) {
                if (e.key.code == sf::Keyboard::Escape) { currentState = GameState::MENU; } // Go back to menu
                if (e.key.code == sf::Keyboard::P) { debugMode *= -1; } // Toggle debug with 'P'
            }
        }
        if (!running || currentState == GameState::MENU) { // If state changed or window closed
            if (!running) break;
            continue;
        }

        sf::Time elapsedTime = tickClock.restart();
        timeSinceLastUpdate += elapsedTime;

        // Fixed Timestep Loop
        while (timeSinceLastUpdate >= TIME_PER_FRAME) {
            timeSinceLastUpdate -= TIME_PER_FRAME;
            playerBody.m_lastPosition = playerBody.m_position; // Store position at start of fixed step

            // --- 1. INPUT PROCESSING ---
            float horizontalInput = 0.f;
            bool jumpKeyPressedThisFrame = false; // For detecting a new jump command
            jumpKeyHeld = false;                  // Reset status for current frame
            turboMultiplier = 1;

            // Joystick (adapt button numbers as needed)
            if (sf::Joystick::isConnected(0)) {
                float joyX = sf::Joystick::getAxisPosition(0, sf::Joystick::X);
                if (std::abs(joyX) > 20.f) horizontalInput = joyX / 100.f; // Deadzone 20%
                if (sf::Joystick::isButtonPressed(0, 0)) jumpKeyHeld = true; // Button 0 (e.g., A on Xbox)
                if (sf::Joystick::isButtonPressed(0, 2)) turboMultiplier = 2; // Button 2 (e.g., X on Xbox)
            }


            if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) horizontalInput = -1.f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) horizontalInput = 1.f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Up) || sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) jumpKeyHeld = true;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift)) turboMultiplier = 2;

            if (jumpKeyHeld && playerIsGrounded_lastFrame && currentJumpHoldDuration == sf::Time::Zero) { // New jump press condition
                jumpKeyPressedThisFrame = true;
            }


            // --- PLATFORM BEHAVIORS ---
            // Moving platform (bodies[12])
            movingPlatformCycleTimer += TIME_PER_FRAME;
            if (movingPlatformCycleTimer.asSeconds() >= 4.f) {
                platformDir *= -1;
                movingPlatformCycleTimer = sf::Time::Zero;
            }
            float moveCycleProgress = movingPlatformCycleTimer.asSeconds();
            if (moveCycleProgress <= 3.f) { // Only move for first 3 seconds of a 4-sec half-cycle
                float easeFactor = math::interpolate::quadraticEaseInOut(moveCycleProgress, 0.f, 1.f, 3.f);
                platformFrameVelocity = platformDir * 200.f * easeFactor * TIME_PER_FRAME.asSeconds(); // 200 is target speed
            } else {
                platformFrameVelocity = 0.f;
            }
            bodies[12].m_position.x += platformFrameVelocity;
            tiles[12].setPosition(bodies[12].m_position);

            // Falling platforms (bodies[15-19])
            for (int i = 15; i <= 19; ++i) {
                if (bodies[i].m_type == phys::bodyType::falling) {
                    bool player_on_this_platform = (playerIsGrounded_lastFrame && // Must be grounded
                                                    collisionSys.getBodyInfo().m_id == bodies[i].m_id &&
                                                    collisionSys.getBodyInfo().m_type == phys::bodyType::falling);
                    
                    if (player_on_this_platform && !bodies[i].m_falling && tiles[i].m_fallTime == sf::Time::Zero) { // If player lands and not already triggered
                        tiles[i].m_fallTime = sf::microseconds(1); // Start the timer (use small value to indicate 'active')
                        tiles[i].setFillColor(sf::Color::Yellow); // Visual cue: unstable
                    }

                    if (tiles[i].m_fallTime > sf::Time::Zero && !bodies[i].m_falling) { // Timer active
                        tiles[i].m_fallTime += TIME_PER_FRAME;
                        if (tiles[i].m_fallTime.asSeconds() > 0.5f) { // After 0.5s delay
                            bodies[i].m_falling = true; // Platform physics state
                            tiles[i].m_falling = true;  // Tile visual/animation state
                            tiles[i].setFillColor(sf::Color(255,100,0)); // Visual cue: falling
                        }
                    }
                    if (bodies[i].m_falling) { // If platform physics is falling
                        tiles[i].move(0.f, 400.f * TIME_PER_FRAME.asSeconds()); // Visual fall speed
                        if (tiles[i].getPosition().y > window.getSize().y + 50.f) { // Fallen off screen
                           bodies[i].m_position = sf::Vector2f(-9999.f, -9999.f); // Move physics body away
                           // Optionally reset: bodies[i].m_falling = false; tiles[i].m_falling = false; tiles[i].m_fallTime = sf::Time::Zero; tiles[i].setPosition(originalPos);
                        }
                    }
                }
            }
            
            // Vanishing platforms (bodies[20-23])
            vanishingPlatformCycleTimer += TIME_PER_FRAME;
            if (vanishingPlatformCycleTimer.asSeconds() > 2.f) { // 2 second phase
                vanishingPlatformCycleTimer = sf::Time::Zero;
                oddEvenVanishing *= -1;
            }
            for (int i = 20; i <= 23; ++i) {
                bool is_even_platform = (i % 2 == 0);
                bool should_be_vanishing_phase = (oddEvenVanishing == 1 && is_even_platform) || (oddEvenVanishing == -1 && !is_even_platform);
                float t = vanishingPlatformCycleTimer.asSeconds(); // Time into current 1s fade in/out

                if (should_be_vanishing_phase) { // Vanishing
                    float alpha_val = math::interpolate::sineEaseIn(t, 0.f, 255.f, 1.f); // Fade out
                    tiles[i].setFillColor(sf::Color(200, 70, 200, 255 - static_cast<sf::Uint8>(alpha_val)));
                    if (tiles[i].getFillColor().a <= 10) bodies[i].m_position = sf::Vector2f(-9999.f, -9999.f); // Make non-collidable
                } else { // Reappearing
                    float alpha_val = math::interpolate::sineEaseIn(t, 0.f, 255.f, 1.f); // Fade in
                    tiles[i].setFillColor(sf::Color(200, 70, 200, static_cast<sf::Uint8>(alpha_val)));
                    if (tiles[i].getFillColor().a > 10 && bodies[i].m_position.x < -9000.f) { // Restore if transparent before
                        // Find original position for body[i] (this needs to be stored if you want them to reappear in same spot)
                        // For now, using tile's current position. Be careful if tile moves while vanished.
                         bodies[i].m_position = tiles[i].getPosition(); // Simplistic restore
                    }
                }
            }

            // --- 2. PLAYER PHYSICS ---
            // Horizontal movement
            playerBody.m_velocity.x = horizontalInput * PLAYER_MOVE_SPEED * turboMultiplier;

            // Vertical movement - Gravity
            if (!playerIsGrounded_lastFrame) { // Apply gravity if not on ground in the previous frame
                playerBody.m_velocity.y += GRAVITY_ACCELERATION * TIME_PER_FRAME.asSeconds();
                if (playerBody.m_velocity.y > MAX_FALL_SPEED) {
                    playerBody.m_velocity.y = MAX_FALL_SPEED;
                }
            }

            // Vertical movement - Jump
            if (jumpKeyPressedThisFrame && playerIsGrounded_lastFrame) {
                playerBody.m_velocity.y = JUMP_INITIAL_VELOCITY;
                currentJumpHoldDuration = sf::microseconds(1); // Start the hold timer slightly > 0
            } else if (jumpKeyHeld && currentJumpHoldDuration > sf::Time::Zero && currentJumpHoldDuration < MAX_JUMP_HOLD_TIME) {
                // Maintain upward velocity influence if jump key held (variable jump height)
                // This can be a constant upward push or counteracting some gravity.
                // For a simple variable jump, often re-applying or maintaining JUMP_INITIAL_VELOCITY is too strong.
                // A common method is to reduce gravity's effect or apply a smaller upward force.
                // Let's make it maintain some upward thrust to counteract gravity more.
                playerBody.m_velocity.y = JUMP_INITIAL_VELOCITY * 0.8f; // Example: slightly less than initial impulse
                currentJumpHoldDuration += TIME_PER_FRAME;
            } else if (!jumpKeyHeld || currentJumpHoldDuration >= MAX_JUMP_HOLD_TIME) {
                currentJumpHoldDuration = sf::Time::Zero; // Reset hold timer if key released or max time reached
            }


            // --- 3. APPLY VELOCITY TO POSITION (Candidate Position) ---
            playerBody.m_position += playerBody.m_velocity * TIME_PER_FRAME.asSeconds();


            // --- 4. COLLISION DETECTION AND RESOLUTION ---
            collisionSys.setCollisionInfo(false, false, false, false); // Reset for this pass
            for (int k = 0; k < 3; ++k) { // Iterative resolution
                collisionSys.resolveCollisions(&playerBody, bodies.data(), bodies.size());
            }

            // --- 5. POST-COLLISION ADJUSTMENTS & STATE UPDATE ---
            // Get fresh collision info for this frame
            const phys::CollisionInfo& currentCollisions = collisionSys.getCollisionInfo();
            const phys::platformBody&  collidedPlatform = collisionSys.getBodyInfo();

            playerIsGrounded_lastFrame = currentCollisions.m_collisionBottom; // Update for NEXT frame's input logic

            if (currentCollisions.m_collisionBottom) {
                if (playerBody.m_velocity.y > 0) playerBody.m_velocity.y = 0.f; // Nullify downward velocity
                currentJumpHoldDuration = sf::Time::Zero; // Reset jump hold when landing

                // Handle special platform surface interactions
                if (collidedPlatform.m_type == phys::bodyType::conveyorBelt) {
                    playerBody.m_position.x += collidedPlatform.m_surfaceVelocity * TIME_PER_FRAME.asSeconds();
                } else if (collidedPlatform.m_type == phys::bodyType::moving && collidedPlatform.m_id == bodies[12].m_id) {
                    playerBody.m_position.x += platformFrameVelocity; // platformFrameVelocity is delta per frame
                }
            }

            if (currentCollisions.m_collisionTop) {
                if (playerBody.m_velocity.y < 0) playerBody.m_velocity.y = 0.f; // Nullify upward velocity
                currentJumpHoldDuration = sf::Time::Zero; // Stop jump hold on head bonk
            }

            // `resolveCollisions` should have zeroed m_velocity.x if a horizontal collision occurred and was resolved.
            // if ((currentCollisions.m_collisionLeft && playerBody.m_velocity.x < 0) || (currentCollisions.m_collisionRight && playerBody.m_velocity.x > 0)) {
            // playerBody.m_velocity.x = 0.f; // This should be handled by resolveCollisions ideally
            // }

            playerShape.setPosition(playerBody.m_position);


            if (debugMode == 1) {
                std::cout << "Pos: (" << playerBody.m_position.x << "," << playerBody.m_position.y << ") "
                          << "Vel: (" << playerBody.m_velocity.x << "," << playerBody.m_velocity.y << ") "
                          << "Grounded: " << playerIsGrounded_lastFrame << " "
                          << "JumpKeyHeld: " << jumpKeyHeld << " JHoldT: " << currentJumpHoldDuration.asSeconds()
                          << " Col(T/B/L/R): " << currentCollisions.m_collisionTop << "/" << currentCollisions.m_collisionBottom << "/" << currentCollisions.m_collisionLeft << "/" << currentCollisions.m_collisionRight
                          << " PlatformID: " << collidedPlatform.m_id
                          << " Type: " << static_cast<int>(collidedPlatform.m_type)
                          << std::endl;
            }

        } // End fixed timestep loop


        // --- Camera Update ---
        // Smooth camera (optional, simple direct follow for now)
        // Keep player somewhat centered but don't go beyond "level" bounds if you have them
        float viewX = playerBody.m_position.x;
        float viewY = playerBody.m_position.y - 50; // A bit above player center
        // Clamp viewX and viewY to your world boundaries if necessary
        mainView.setCenter(viewX, viewY);

        // --- Drawing ---
        window.clear(sf::Color(20, 20, 40)); // Dark blue background
        window.setView(mainView);
        for (const auto& t : tiles) { // Use const ref
            if (t.getFillColor().a > 5) { // Draw if not fully transparent
                 window.draw(t);
            }
        }
        window.draw(playerShape);
        window.setView(window.getDefaultView()); // For any UI elements drawn over game
        // Draw debug text or scores here if needed
        window.display();

    } // End main game loop (while running)

    return 0;
}