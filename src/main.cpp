#include <iostream>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp> // For sf::Music and sf::Sound
#include <vector>
#include <cmath>
#include <string>
#include <cstdlib> // For std::fmod with floats if needed, though cmath should cover it
#include <ctime>   // Not strictly used here but often good for games
#include <algorithm> // For std::min/max
#include <limits>    // For numeric_limits
#include <filesystem> // For checking font path if needed, currently not actively used for dynamic checks
#include <map>        // For SFX map

#include "CollisionSystem.hpp"
#include "Player.hpp"
#include "PlatformBody.hpp"
#include "Tile.hpp"
#include "PhysicsTypes.hpp"
#include "LevelManager.hpp"
#include "Optimizer.hpp" // CHANGED FROM Math_Easing.hpp


// --- Updated GameState Enum ---
enum class GameState {
    MENU,
    SETTINGS,
    CREDITS,
    PLAYING,
    TRANSITIONING,
    GAME_OVER_WIN,
    GAME_OVER_LOSE_FALL, // Specific to falling out
    GAME_OVER_LOSE_DEATH // Specific to trap death
};

// --- Game Settings Struct ---
struct GameSettings {
    float musicVolume = 100.f; // 0-100
    float sfxVolume = 100.f;   // 0-100
};

// --- Global Game Objects ---
LevelManager levelManager;
LevelData currentLevelData; // Holds data for the *currently active or loading* level
phys::DynamicBody playerBody;
std::vector<phys::PlatformBody> bodies; // Physics bodies for collision
std::vector<Tile> tiles;                // Visual tiles

struct ActiveMovingPlatform {
    unsigned int id;
    sf::Vector2f startPosition; // The absolute starting point of the movement path
    char axis;
    float distance;
    float cycleTime;
    float cycleDuration;
    int direction; // Current direction of movement for path segment
    sf::Vector2f lastFramePosition; // Platform's position in the previous frame
};
std::vector<ActiveMovingPlatform> activeMovingPlatforms;

sf::Time vanishingPlatformCycleTimer = sf::Time::Zero;
int oddEvenVanishing = 1;

GameSettings gameSettings;

// --- Audio Objects ---
sf::Music menuMusic;
sf::Music gameMusic;
std::map<std::string, sf::SoundBuffer> soundBuffers;
sf::Sound sfxPlayer;

// --- Asset Paths ---
const std::string FONT_PATH = "../assets/fonts/ARIALBD.TTF"; // Make sure this path is correct
const std::string IMG_MENU_BG = "../assets/images/mainmenu_bg.png";
const std::string IMG_LOAD_GENERAL = "../assets/images/loading.png";
const std::string IMG_LOAD_NEXT = "../assets/images/menuload.png";
const std::string IMG_LOAD_RESPAWN = "../assets/images/respawn.png";

const std::string AUDIO_MUSIC_MENU = "../assets/audio/music_menu.ogg";
const std::string AUDIO_MUSIC_GAME = "../assets/audio/music_ingame.ogg";
const std::string SFX_JUMP = "../assets/audio/sfx_jump.wav";
const std::string SFX_DEATH = "../assets/audio/sfx_death.wav";
const std::string SFX_GOAL = "../assets/audio/sfx_goal.wav";
const std::string SFX_CLICK = "../assets/audio/sfx_click.wav";


// --- Helper to play SFX ---
void playSfx(const std::string& sfxName) {
    if (soundBuffers.count(sfxName)) {
        sfxPlayer.setBuffer(soundBuffers[sfxName]);
        sfxPlayer.setVolume(gameSettings.sfxVolume);
        sfxPlayer.play();
    } else {
        std::cerr << "SFX not loaded/found: " << sfxName << std::endl;
    }
}

// --- Function to load audio assets ---
void loadAudio() {
    if (!menuMusic.openFromFile(AUDIO_MUSIC_MENU))
        std::cerr << "Error loading menu music: " << AUDIO_MUSIC_MENU << std::endl;
    else menuMusic.setLoop(true);

    if (!gameMusic.openFromFile(AUDIO_MUSIC_GAME))
        std::cerr << "Error loading game music: " << AUDIO_MUSIC_GAME << std::endl;
    else gameMusic.setLoop(true);

    sf::SoundBuffer buffer; // Reusable buffer for loading
    if (buffer.loadFromFile(SFX_JUMP)) soundBuffers["jump"] = buffer;
    else std::cerr << "Error loading SFX: " << SFX_JUMP << std::endl;
    if (buffer.loadFromFile(SFX_DEATH)) soundBuffers["death"] = buffer;
    else std::cerr << "Error loading SFX: " << SFX_DEATH << std::endl;
    if (buffer.loadFromFile(SFX_GOAL)) soundBuffers["goal"] = buffer;
    else std::cerr << "Error loading SFX: " << SFX_GOAL << std::endl;
    if (buffer.loadFromFile(SFX_CLICK)) soundBuffers["click"] = buffer;
    else std::cerr << "Error loading SFX: " << SFX_CLICK << std::endl;
}


// --- Function to Setup Level Assets (Visuals, Physics instances from currentLevelData) ---
void setupLevelAssets(const LevelData& data, sf::RenderWindow& window) {
    bodies.clear();
    tiles.clear();
    activeMovingPlatforms.clear();

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
        bodies.push_back(p_body_template); // Add physics body

        // Link moving platform details
        if (p_body_template.getType() == phys::bodyType::moving) {
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
            case phys::bodyType::trap:         newTile.setFillColor(sf::Color(255, 20, 20, 255)); break;
            case phys::bodyType::goal:         newTile.setFillColor(sf::Color(20, 255, 20, 128)); break;
            case phys::bodyType::interactible: newTile.setFillColor(sf::Color(180, 180, 220, 200)); break;
            case phys::bodyType::none:         newTile.setFillColor(sf::Color(0, 0, 0, 0)); break;
            default:                           newTile.setFillColor(sf::Color(128, 128, 128, 255)); break;
        }
        tiles.push_back(newTile);
    }

    vanishingPlatformCycleTimer = sf::Time::Zero;
    oddEvenVanishing = 1;
}


int main(void) {
    const sf::Vector2f LOGICAL_SIZE(800.f, 600.f); // Design resolution
    const sf::Vector2f tileSize(32.f, 32.f);
    const float PLAYER_MOVE_SPEED = 200.f;
    const float JUMP_INITIAL_VELOCITY = -450.f;
    const float GRAVITY_ACCELERATION = 1200.f;
    const float MAX_FALL_SPEED = 700.f;
    const sf::Time MAX_JUMP_HOLD_TIME = sf::seconds(0.18f);
    const float PLAYER_DEATH_Y_LIMIT = 2000.f; // Relative to game world coordinates

    sf::VideoMode desktopMode = sf::VideoMode::getFullscreenModes()[0];
    sf::RenderWindow window(desktopMode, "Project - T", sf::Style::Fullscreen);
    window.setKeyRepeatEnabled(false);
    window.setVerticalSyncEnabled(true);

    sf::View uiView;     // For menus, HUD, transitions - fixed to LOGICAL_SIZE
    sf::View mainView;   // For gameplay - follows player, same LOGICAL_SIZE dimensions

    // Calculate viewport for letterboxing/pillarboxing
    float windowWidth = static_cast<float>(desktopMode.width);
    float windowHeight = static_cast<float>(desktopMode.height);
    float windowAspectRatio = windowWidth / windowHeight;
    float logicalAspectRatio = LOGICAL_SIZE.x / LOGICAL_SIZE.y;

    float viewportX = 0.f;
    float viewportY = 0.f;
    float viewportWidthRatio = 1.f;
    float viewportHeightRatio = 1.f;

    if (windowAspectRatio > logicalAspectRatio) { // Window is wider than logical (letterbox top/bottom bars, or pillarbox if math wrong -> pillarbox for wider)
        viewportWidthRatio = logicalAspectRatio / windowAspectRatio; // Viewport width is scaled down
        viewportX = (1.f - viewportWidthRatio) / 2.f;             // Center it
    } else if (windowAspectRatio < logicalAspectRatio) { // Window is narrower than logical (pillarbox left/right bars, or letterbox if math wrong -> letterbox for taller)
        viewportHeightRatio = windowAspectRatio / logicalAspectRatio; // Viewport height is scaled down
        viewportY = (1.f - viewportHeightRatio) / 2.f;              // Center it
    }
    sf::FloatRect viewportRect(viewportX, viewportY, viewportWidthRatio, viewportHeightRatio);

    uiView.setSize(LOGICAL_SIZE);
    uiView.setCenter(LOGICAL_SIZE / 2.f); // Centered on the logical 800x600 area
    uiView.setViewport(viewportRect);     // Maps this logical area to the calculated part of the window

    mainView.setSize(LOGICAL_SIZE);     // Game world view also uses logical dimensions
    mainView.setViewport(viewportRect); // And same screen viewport for aspect ratio
    // mainView.setCenter will be updated dynamically based on player position for scrolling


    GameState currentState = GameState::MENU;
    levelManager.setMaxLevels(5);
    levelManager.setLevelBasePath("../assets/levels/");
    levelManager.setTransitionProperties(0.75f); 
    levelManager.setGeneralLoadingScreenImage(IMG_LOAD_GENERAL);
    levelManager.setNextLevelLoadingScreenImage(IMG_LOAD_NEXT);
    levelManager.setRespawnLoadingScreenImage(IMG_LOAD_RESPAWN);

    loadAudio();

    playerBody = phys::DynamicBody({0.f, 0.f}, tileSize.x, tileSize.y, {0.f, 0.f});

    sf::Font menuFont;
    if (!menuFont.loadFromFile(FONT_PATH)) {
        std::cerr << "FATAL: Failed to load font: " << FONT_PATH << ". Exiting." << std::endl;
        #ifdef _WIN32
        if (!menuFont.loadFromFile("C:/Windows/Fonts/arialbd.ttf")) return -1;
        #else
        // Fallback for other systems if needed
        #endif
        if (menuFont.getInfo().family.empty()) {std::cerr << "Fallback font failed.\n"; return -1;}
        std::cout << "Loaded fallback font for debug." << std::endl;
    }

    auto setupTextUI = [&](sf::Text& text, const sf::String& str, float yPos, unsigned int charSize = 30, float xOffset = 0.f) {
        text.setFont(menuFont);
        text.setString(str);
        text.setCharacterSize(charSize);
        text.setFillColor(sf::Color::White);
        sf::FloatRect bounds = text.getLocalBounds();
        text.setOrigin(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
        // Position relative to LOGICAL_SIZE for UI elements
        text.setPosition(LOGICAL_SIZE.x / 2.f + xOffset, yPos);
    };

    sf::Text menuTitleText, startButtonText, settingsButtonText, creditsButtonText, exitButtonText;
    sf::Texture menuBgTexture; sf::Sprite menuBgSprite;
    if (menuBgTexture.loadFromFile(IMG_MENU_BG)) {
        menuBgSprite.setTexture(menuBgTexture);
        // Scale sprite to fill the LOGICAL_SIZE view
        menuBgSprite.setScale(LOGICAL_SIZE.x / menuBgTexture.getSize().x,
                              LOGICAL_SIZE.y / menuBgTexture.getSize().y);
        menuBgSprite.setPosition(0.f, 0.f); // Position at top-left of the LOGICAL_SIZE view
    }
    else {
        std::cerr << "Warning: Menu BG image not found: " << IMG_MENU_BG << std::endl;
    }


    setupTextUI(menuTitleText, "Project - T", 100, 48);
    setupTextUI(startButtonText, "Start Game", 250);
    setupTextUI(settingsButtonText, "Settings", 300);
    setupTextUI(creditsButtonText, "Credits", 350);
    setupTextUI(exitButtonText, "Exit", 400);

    sf::Text settingsTitleText, musicVolumeText, musicVolValText, sfxVolumeText, sfxVolValText, settingsBackText;
    setupTextUI(settingsTitleText, "Settings", 100, 40);
    setupTextUI(musicVolumeText, "Music Volume:", 200, 24, -100);
    setupTextUI(musicVolValText, "", 200, 24, 50);
    setupTextUI(sfxVolumeText, "SFX Volume:", 250, 24, -100);
    setupTextUI(sfxVolValText, "", 250, 24, 50);
    setupTextUI(settingsBackText, "Back to Menu", 450);

    sf::Text creditsTitleText, creditsNamesText, creditsBackText;
    setupTextUI(creditsTitleText, "Credits", 100, 40);
    setupTextUI(creditsNamesText, "Jan\nZean\nJecer\nGian", 250, 28);
    setupTextUI(creditsBackText, "Back to Menu", 450);

    sf::Text gameOverStatusText, gameOverOption1Text, gameOverOption2Text;
    setupTextUI(gameOverStatusText, "", 150, 36);
    setupTextUI(gameOverOption1Text, "", 280);
    setupTextUI(gameOverOption2Text, "Main Menu", 330);

    sf::Color defaultBtnColor = sf::Color::White;
    sf::Color hoverBtnColor = sf::Color::Yellow;
    sf::Color exitBtnHoverColor = sf::Color::Red;

    sf::RectangleShape playerShape;
    playerShape.setFillColor(sf::Color(220, 220, 250, 255));
    playerShape.setSize(sf::Vector2f(playerBody.getWidth(), playerBody.getHeight()));

    sf::Clock gameClock;
    sf::Time timeSinceLastFixedUpdate = sf::Time::Zero;
    const sf::Time TIME_PER_FIXED_UPDATE = sf::seconds(1.f / 60.f);

    sf::Time currentJumpHoldDuration = sf::Time::Zero;
    int turboMultiplier = 1;
    bool interactKeyPressedThisFrame = false;

    sf::Text debugText;
    debugText.setFont(menuFont);
    debugText.setCharacterSize(14);
    debugText.setFillColor(sf::Color::White);
    debugText.setPosition(10.f, 10.f); // Positioned relative to top-left of uiView (LOGICAL_SIZE)

    menuMusic.setVolume(gameSettings.musicVolume);
    menuMusic.play();

    bool running = true;
    while (running) {
        interactKeyPressedThisFrame = false;
        sf::Time frameDeltaTime = gameClock.restart();

        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                running = false; window.close();
            }
             if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::F4) {
                if(currentState == GameState::PLAYING && levelManager.requestLoadNextLevel(currentLevelData)){
                     currentState = GameState::TRANSITIONING; playSfx("goal");
                } else if (currentState == GameState::PLAYING) {
                    currentState = GameState::GAME_OVER_WIN; gameMusic.stop(); menuMusic.play();
                }
            }

            // Convert mouse coordinates using uiView for all UI interactions
            sf::Vector2i pixelPos = sf::Mouse::getPosition(window);
            sf::Vector2f worldPosUi = window.mapPixelToCoords(pixelPos, uiView);


            switch(currentState) {
                case GameState::MENU:
                    if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
                        playSfx("click");
                        if (startButtonText.getGlobalBounds().contains(worldPosUi)) {
                            levelManager.setCurrentLevelNumber(0);
                            if (levelManager.requestLoadNextLevel(currentLevelData)) {
                                currentState = GameState::TRANSITIONING;
                                menuMusic.stop(); gameMusic.setVolume(gameSettings.musicVolume); gameMusic.play();
                                window.setTitle("Project T - Loading...");
                            } else { std::cerr << "MENU: Failed request to load level 1." << std::endl; }
                        } else if (settingsButtonText.getGlobalBounds().contains(worldPosUi)) {
                            currentState = GameState::SETTINGS;
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
                            gameMusic.pause(); menuMusic.setVolume(gameSettings.musicVolume); menuMusic.play();
                            window.setTitle("Project T (Menu - Paused)");
                        } else if (event.key.code == sf::Keyboard::R) {
                            playSfx("click");
                            if (levelManager.requestRespawnCurrentLevel(currentLevelData)) {
                                currentState = GameState::TRANSITIONING;
                                window.setTitle("Project T - Respawning...");
                            } else {std::cerr << "PLAYING: Failed respawn request.\n";}
                        } else if (event.key.code == sf::Keyboard::E) {
                            interactKeyPressedThisFrame = true;
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
                                if (levelManager.requestRespawnCurrentLevel(currentLevelData)) {
                                    currentState = GameState::TRANSITIONING;
                                    gameMusic.setVolume(gameSettings.musicVolume); gameMusic.play();
                                } else {
                                    std::cerr << "GAME_OVER_LOSE: Failed respawn. Returning to MENU.\n";
                                    currentState = GameState::MENU; gameMusic.stop(); menuMusic.play(); levelManager.setCurrentLevelNumber(0);
                                }
                            } else if (currentState == GameState::GAME_OVER_WIN) {
                                levelManager.setCurrentLevelNumber(0);
                                if (levelManager.requestLoadNextLevel(currentLevelData)) {
                                    currentState = GameState::TRANSITIONING;
                                    menuMusic.stop(); gameMusic.setVolume(gameSettings.musicVolume); gameMusic.play();
                                } else {
                                    std::cerr << "GAME_OVER_WIN: Failed Play Again. Returning to MENU.\n";
                                     currentState = GameState::MENU; menuMusic.play();
                                }
                            }
                        } else if (gameOverOption2Text.getGlobalBounds().contains(worldPosUi)) { // Back to Menu
                            currentState = GameState::MENU;
                            gameMusic.stop(); menuMusic.setVolume(gameSettings.musicVolume); menuMusic.play();
                            levelManager.setCurrentLevelNumber(0);
                        }
                    }
                     if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
                        currentState = GameState::MENU; gameMusic.stop(); menuMusic.play(); levelManager.setCurrentLevelNumber(0);
                     }
                    break;
                case GameState::TRANSITIONING:
                    // No input handling during transitions, usually
                    break;
                default: 
                    std::cerr << "Warning: Unhandled GameState in event loop: " << static_cast<int>(currentState) << std::endl;
                    currentState = GameState::MENU; 
                    break;
            }
        }

        if (!running) break;

        // --- UPDATE LOGIC ---
        timeSinceLastFixedUpdate += frameDeltaTime;

        if (currentState == GameState::PLAYING) {
            window.setTitle("Project T - Level " + currentLevelData.levelName + " (" + std::to_string(currentLevelData.levelNumber) +")");
            while (timeSinceLastFixedUpdate >= TIME_PER_FIXED_UPDATE) {
                timeSinceLastFixedUpdate -= TIME_PER_FIXED_UPDATE;
                playerBody.setLastPosition(playerBody.getPosition());

                float horizontalInput = 0.f;
                turboMultiplier = (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift)) ? 2 : 1;
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) horizontalInput = -1.f;
                else if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) horizontalInput = 1.f;

                bool jumpIntentThisFrame = (sf::Keyboard::isKeyPressed(sf::Keyboard::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Up) || sf::Keyboard::isKeyPressed(sf::Keyboard::Space));
                bool dropIntentThisFrame = (sf::Keyboard::isKeyPressed(sf::Keyboard::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Down));
                bool newJumpPressThisFrame = (jumpIntentThisFrame && playerBody.isOnGround() && currentJumpHoldDuration == sf::Time::Zero);
                if (newJumpPressThisFrame) playSfx("jump");
                playerBody.setTryingToDrop(dropIntentThisFrame && playerBody.isOnGround());

                for(auto& activePlat : activeMovingPlatforms) {
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

                 for (size_t i = 0; i < bodies.size(); ++i) {
                    if (tiles.size() <= i || (bodies[i].getType() == phys::bodyType::none && tiles[i].getFillColor().a == 0)) continue;
                    tiles[i].update(TIME_PER_FIXED_UPDATE);
                    if (bodies[i].getType() == phys::bodyType::falling) {
                        bool playerOnThis = playerBody.isOnGround() && playerBody.getGroundPlatform() && playerBody.getGroundPlatform()->getID() == bodies[i].getID();
                        if (playerOnThis && !tiles[i].isFalling() && !tiles[i].hasFallen()) tiles[i].startFalling(sf::seconds(0.25f));
                        if (tiles[i].isFalling() && !bodies[i].isFalling()) bodies[i].setFalling(true);
                        if (tiles[i].hasFallen() && bodies[i].getPosition().x > -9000.f) bodies[i].setPosition({-9999.f, -9999.f});
                    } else if (bodies[i].getType() == phys::bodyType::vanishing) {
                        bool is_even_id = (bodies[i].getID() % 2 == 0);
                        bool should_vanish_now = (oddEvenVanishing == 1 && is_even_id) || (oddEvenVanishing == -1 && !is_even_id);
                        float phaseTime = std::fmod(vanishingPlatformCycleTimer.asSeconds(), 1.0f);
                        sf::Color originalColor(200, 70, 200, 255); float alpha_val;
                        if (should_vanish_now) alpha_val = math::easing::sineEaseInOut(phaseTime, 255.f, -255.f, 1.f);
                        else alpha_val = math::easing::sineEaseInOut(phaseTime, 0.f, 255.f, 1.f);
                        alpha_val = std::max(0.f, std::min(255.f, alpha_val));
                        tiles[i].setFillColor(sf::Color(originalColor.r, originalColor.g, originalColor.b, static_cast<sf::Uint8>(alpha_val)));
                        if (alpha_val <= 10.f && bodies[i].getPosition().x > -9000.f) bodies[i].setPosition({-9999.f, -9999.f});
                        else if (alpha_val > 10.f && bodies[i].getPosition().x < -9000.f) {
                            for(const auto& templ : currentLevelData.platforms) if(templ.getID() == bodies[i].getID()){
                                bodies[i].setPosition(templ.getPosition()); tiles[i].setPosition(templ.getPosition()); break;
                            }
                        }
                    }
                }
                vanishingPlatformCycleTimer += TIME_PER_FIXED_UPDATE;
                if (vanishingPlatformCycleTimer.asSeconds() >= 1.f) {
                    vanishingPlatformCycleTimer -= sf::seconds(1.f); oddEvenVanishing *= -1;
                }

                sf::Vector2f pVel = playerBody.getVelocity();
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

                phys::CollisionResolutionInfo resolutionResult = phys::CollisionSystem::resolveCollisions(playerBody, bodies, TIME_PER_FIXED_UPDATE.asSeconds());
                pVel = playerBody.getVelocity();
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
                                    for(auto& b : bodies) if(b.getID() == activePlat.id) {movingPhysBody = &b; break;}
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

                bool trapHit = false;
                for (const auto& body : bodies) {
                    if (body.getType() == phys::bodyType::trap && playerBody.getAABB().intersects(body.getAABB())) {
                        trapHit = true; break;
                    }
                }
                if (trapHit) {
                    playSfx("death");
                    currentState = GameState::GAME_OVER_LOSE_DEATH;
                    gameMusic.pause();
                    window.setTitle("Project T - Ouch! Game Over");
                    break; 
                }

                if (interactKeyPressedThisFrame) {
                    for (const auto& platform : bodies) {
                        if (platform.getType() == phys::bodyType::goal && playerBody.getAABB().intersects(platform.getAABB())) {
                            playSfx("goal");
                            if (levelManager.hasNextLevel()) {
                                if (levelManager.requestLoadNextLevel(currentLevelData)) {
                                    currentState = GameState::TRANSITIONING;
                                    window.setTitle("Project T - Next Level...");
                                } else { currentState = GameState::MENU; gameMusic.stop(); menuMusic.play(); }
                            } else {
                                currentState = GameState::GAME_OVER_WIN;
                                gameMusic.stop(); menuMusic.play();
                                window.setTitle("Project T - You Win!");
                            }
                            if (currentState != GameState::PLAYING) break;
                        }
                    }
                    if (currentState != GameState::PLAYING) break; 
                }

                if (playerBody.getPosition().y > PLAYER_DEATH_Y_LIMIT) {
                    playSfx("death");
                    currentState = GameState::GAME_OVER_LOSE_FALL;
                    gameMusic.pause();
                    window.setTitle("Project T - Game Over (Fell)");
                    break; 
                }
            } // End fixed update loop
        }
        else if (currentState == GameState::TRANSITIONING) {
            levelManager.update(frameDeltaTime.asSeconds(), window);
            if (!levelManager.isTransitioning()) {
                setupLevelAssets(currentLevelData, window);
                currentState = GameState::PLAYING;
                if (gameMusic.getStatus() != sf::Music::Playing) {
                     menuMusic.stop(); gameMusic.setVolume(gameSettings.musicVolume); gameMusic.play();
                }
            }
        }


        // --- DRAW PHASE ---
        window.clear(sf::Color::Black); // Clear entire window to black for letter/pillar boxing

        // For mouse hover updates, get current mouse position relative to uiView
        sf::Vector2i pixelPos = sf::Mouse::getPosition(window);
        sf::Vector2f worldPosUi = window.mapPixelToCoords(pixelPos, uiView);


        switch(currentState) {
            case GameState::MENU:
                window.setView(uiView);
                if(menuBgSprite.getTexture()) {
                    window.draw(menuBgSprite);
                } else { // Fallback if texture not loaded
                    sf::RectangleShape bg(LOGICAL_SIZE); 
                    bg.setFillColor(sf::Color(20,20,50));
                    bg.setPosition(0,0); // Covers uiView
                    window.draw(bg);
                }
                startButtonText.setFillColor(defaultBtnColor); settingsButtonText.setFillColor(defaultBtnColor);
                creditsButtonText.setFillColor(defaultBtnColor); exitButtonText.setFillColor(defaultBtnColor);
                if(startButtonText.getGlobalBounds().contains(worldPosUi)) startButtonText.setFillColor(hoverBtnColor);
                if(settingsButtonText.getGlobalBounds().contains(worldPosUi)) settingsButtonText.setFillColor(hoverBtnColor);
                if(creditsButtonText.getGlobalBounds().contains(worldPosUi)) creditsButtonText.setFillColor(hoverBtnColor);
                if(exitButtonText.getGlobalBounds().contains(worldPosUi)) exitButtonText.setFillColor(exitBtnHoverColor);
                window.draw(menuTitleText); window.draw(startButtonText); window.draw(settingsButtonText);
                window.draw(creditsButtonText); window.draw(exitButtonText);
                break;
            case GameState::SETTINGS:
                window.setView(uiView);
                { sf::RectangleShape bg(LOGICAL_SIZE); bg.setFillColor(sf::Color(20,50,20)); bg.setPosition(0,0); window.draw(bg); }
                settingsBackText.setFillColor(defaultBtnColor);
                if(settingsBackText.getGlobalBounds().contains(worldPosUi)) settingsBackText.setFillColor(hoverBtnColor);
                window.draw(settingsTitleText);
                musicVolValText.setString(std::to_string(static_cast<int>(gameSettings.musicVolume))+"%");
                sfxVolValText.setString(std::to_string(static_cast<int>(gameSettings.sfxVolume))+"%");
                window.draw(musicVolumeText); window.draw(musicVolValText);
                window.draw(sfxVolumeText); window.draw(sfxVolValText);
                window.draw(settingsBackText);
                break;
            case GameState::CREDITS:
                window.setView(uiView);
                { sf::RectangleShape bg(LOGICAL_SIZE); bg.setFillColor(sf::Color(50,20,20)); bg.setPosition(0,0); window.draw(bg); }
                creditsBackText.setFillColor(defaultBtnColor);
                if(creditsBackText.getGlobalBounds().contains(worldPosUi)) creditsBackText.setFillColor(hoverBtnColor);
                window.draw(creditsTitleText); window.draw(creditsNamesText); window.draw(creditsBackText);
                break;
            case GameState::PLAYING:
                mainView.setCenter( playerBody.getPosition().x + playerBody.getWidth()/2.f, playerBody.getPosition().y + playerBody.getHeight()/2.f - 50.f);
                window.setView(mainView);
                
                // Draw background for the gameplay area within mainView
                {
                    sf::RectangleShape gameplayAreaBackground(LOGICAL_SIZE); 
                    gameplayAreaBackground.setFillColor(currentLevelData.backgroundColor);
                    // Position at the top-left of the current view (mainView)
                    gameplayAreaBackground.setPosition(mainView.getCenter() - mainView.getSize() / 2.f); 
                    window.draw(gameplayAreaBackground);
                }

                playerShape.setPosition(playerBody.getPosition());
                for (const auto& t : tiles) { if (t.getFillColor().a > 5) window.draw(t); }
                window.draw(playerShape);
                
                // Switch to uiView for debug text
                window.setView(uiView); 
                debugText.setString( "Lvl: " + std::to_string(currentLevelData.levelNumber) +
                                     " Pos: " + std::to_string(static_cast<int>(playerBody.getPosition().x)) + "," + std::to_string(static_cast<int>(playerBody.getPosition().y)) +
                                     " Vel: " + std::to_string(static_cast<int>(playerBody.getVelocity().x)) + "," + std::to_string(static_cast<int>(playerBody.getVelocity().y)) +
                                     " Ground: " + (playerBody.isOnGround() ? "Y" : "N") +
                                     (playerBody.getGroundPlatform() ? (" (ID:" + std::to_string(playerBody.getGroundPlatform()->getID()) +")"):""));
                window.draw(debugText);
                break;
            case GameState::TRANSITIONING:
                window.setView(uiView);
                // LevelManager will draw its overlay. If its internal images aren't scaled to LOGICAL_SIZE, they might appear incorrectly.
                // A solid background could be drawn here first if LevelManager's overlay has transparency
                // { sf::RectangleShape bg(LOGICAL_SIZE); bg.setFillColor(sf::Color::Black); bg.setPosition(0,0); window.draw(bg); }
                levelManager.draw(window); 
                break;
            case GameState::GAME_OVER_WIN:
                 window.setView(uiView);
                 { sf::RectangleShape bg(LOGICAL_SIZE); bg.setFillColor(sf::Color(20,60,20)); bg.setPosition(0,0); window.draw(bg); }
                 gameOverStatusText.setString("All Levels Cleared! You Win!");
                 gameOverOption1Text.setString("Play Again?");
                 gameOverOption1Text.setFillColor(defaultBtnColor);
                 gameOverOption2Text.setFillColor(defaultBtnColor);
                 if(gameOverOption1Text.getGlobalBounds().contains(worldPosUi)) gameOverOption1Text.setFillColor(hoverBtnColor);
                 if(gameOverOption2Text.getGlobalBounds().contains(worldPosUi)) gameOverOption2Text.setFillColor(hoverBtnColor);
                 window.draw(gameOverStatusText);
                 window.draw(gameOverOption1Text);
                 window.draw(gameOverOption2Text);
                break;
            case GameState::GAME_OVER_LOSE_FALL:
                window.setView(uiView);
                { sf::RectangleShape bg(LOGICAL_SIZE); bg.setFillColor(sf::Color(60,20,20)); bg.setPosition(0,0); window.draw(bg); }
                gameOverStatusText.setString("Game Over! You Fell!");
                gameOverOption1Text.setString("Retry Level");
                gameOverOption1Text.setFillColor(defaultBtnColor);
                gameOverOption2Text.setFillColor(defaultBtnColor);
                if(gameOverOption1Text.getGlobalBounds().contains(worldPosUi)) gameOverOption1Text.setFillColor(hoverBtnColor);
                if(gameOverOption2Text.getGlobalBounds().contains(worldPosUi)) gameOverOption2Text.setFillColor(hoverBtnColor);
                window.draw(gameOverStatusText);
                window.draw(gameOverOption1Text);
                window.draw(gameOverOption2Text);
                break;
            case GameState::GAME_OVER_LOSE_DEATH:
                window.setView(uiView);
                { sf::RectangleShape bg(LOGICAL_SIZE); bg.setFillColor(sf::Color(70,10,10)); bg.setPosition(0,0); window.draw(bg); }
                gameOverStatusText.setString("Game Over! Hit a Trap!");
                gameOverOption1Text.setString("Retry Level");
                gameOverOption1Text.setFillColor(defaultBtnColor);
                gameOverOption2Text.setFillColor(defaultBtnColor);
                if(gameOverOption1Text.getGlobalBounds().contains(worldPosUi)) gameOverOption1Text.setFillColor(hoverBtnColor);
                if(gameOverOption2Text.getGlobalBounds().contains(worldPosUi)) gameOverOption2Text.setFillColor(hoverBtnColor);
                window.draw(gameOverStatusText);
                window.draw(gameOverOption1Text);
                window.draw(gameOverOption2Text);
                break;
             default:
                 window.setView(uiView);
                 { sf::RectangleShape bg(LOGICAL_SIZE); bg.setFillColor(sf::Color::Magenta); bg.setPosition(0,0); window.draw(bg); }
                 sf::Text errorText("Unknown Game State!", menuFont, 20);
                 errorText.setOrigin(errorText.getLocalBounds().width/2.f, errorText.getLocalBounds().height/2.f); 
                 errorText.setPosition(LOGICAL_SIZE.x/2.f, LOGICAL_SIZE.y/2.f);
                 window.draw(errorText);
                 break;
        }
        window.display();
    }

    menuMusic.stop(); gameMusic.stop();
    return 0;
}