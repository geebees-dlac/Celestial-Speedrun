// main.cpp

// SFML Includes
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <SFML/Window/Event.hpp>

// Standard Library Includes
#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <limits>
#include <filesystem>
#include <map>

// Project-Specific Includes
#include "CollisionSystem.hpp"
#include "Player.hpp"
#include "PlatformBody.hpp"
#include "Tile.hpp"
#include "PhysicsTypes.hpp"
#include "LevelManager.hpp"
#include "Optimizer.hpp" // Your easing functions

// --- Updated GameState Enum ---
enum class GameState {
    MENU,
    SETTINGS,
    CREDITS,
    PLAYING,
    TRANSITIONING,
    GAME_OVER_WIN,
    GAME_OVER_LOSE_FALL,
    GAME_OVER_LOSE_DEATH
};

// --- Game Settings Struct ---
struct GameSettings {
    float musicVolume = 50.f; // 0-100
    float sfxVolume = 70.f;   // 0-100
};

// --- Global Variables for Window/Resolution Management ---
const sf::Vector2f LOGICAL_SIZE(800.f, 600.f);
std::vector<sf::Vector2u> availableResolutions;
int currentResolutionIndex = 0;
bool isFullscreen = true;

sf::Text resolutionCurrentText;
void updateResolutionDisplayText();


// --- Global Game Objects ---
LevelManager levelManager;
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

struct ActiveInteractiblePlatform {
    unsigned int id;
    std::string interactionType;
    phys::bodyType targetBodyTypeEnum;
    sf::Color targetTileColor;
    bool hasTargetTileColor;
    bool oneTime;
    float cooldown;
    bool hasBeenInteractedThisSession;
    float currentCooldownTimer;
};
std::map<unsigned int, ActiveInteractiblePlatform> activeInteractibles;

sf::Time vanishingPlatformCycleTimer = sf::Time::Zero;
int oddEvenVanishing = 1;

GameSettings gameSettings;

sf::Music menuMusic;
sf::Music gameMusic;
std::map<std::string, sf::SoundBuffer> soundBuffers;
sf::Sound sfxPlayer;

const std::string FONT_PATH = "../assets/fonts/ARIALBD.TTF";
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
const std::string SFX_SPRING = "../assets/audio/sfx_spring.wav"; 

// --- Function to recreate window and update views ---
void applyAndRecreateWindow(sf::RenderWindow& window, sf::View& uiView, sf::View& mainView) {
    sf::VideoMode mode;
    sf::Uint32 style;

    if (isFullscreen) {
        mode = sf::VideoMode::getFullscreenModes()[0];
        style = sf::Style::Fullscreen;
    } else {
        if (currentResolutionIndex >= 0 && currentResolutionIndex < static_cast<int>(availableResolutions.size())) {
            mode = sf::VideoMode(availableResolutions[currentResolutionIndex].x, availableResolutions[currentResolutionIndex].y);
        } else {
            mode = sf::VideoMode(static_cast<unsigned int>(LOGICAL_SIZE.x), static_cast<unsigned int>(LOGICAL_SIZE.y));
            if (availableResolutions.empty()) currentResolutionIndex = -1;
            else {
                 currentResolutionIndex = 0;
                 for(size_t i=0; i < availableResolutions.size(); ++i) {
                     if(availableResolutions[i].x == static_cast<unsigned int>(LOGICAL_SIZE.x) && availableResolutions[i].y == static_cast<unsigned int>(LOGICAL_SIZE.y)) {
                         currentResolutionIndex = static_cast<int>(i);
                         break;
                     }
                 }
                 if(currentResolutionIndex >= 0 && currentResolutionIndex < static_cast<int>(availableResolutions.size())) {
                    mode = sf::VideoMode(availableResolutions[currentResolutionIndex].x, availableResolutions[currentResolutionIndex].y);
                 } else {
                    currentResolutionIndex = 0;
                    if(!availableResolutions.empty()){
                        mode = sf::VideoMode(availableResolutions[0].x, availableResolutions[0].y);
                    } else {
                        mode = sf::VideoMode(static_cast<unsigned int>(LOGICAL_SIZE.x), static_cast<unsigned int>(LOGICAL_SIZE.y));
                    }
                 }
            }
        }
        style = sf::Style::Default;
    }

    window.create(mode, "Project - T", style);
    window.setKeyRepeatEnabled(false);
    window.setVerticalSyncEnabled(true);

    float windowWidth = static_cast<float>(window.getSize().x);
    float windowHeight = static_cast<float>(window.getSize().y);
    float windowAspectRatio = windowWidth / windowHeight;
    float logicalAspectRatio = LOGICAL_SIZE.x / LOGICAL_SIZE.y;

    float viewportX = 0.f;
    float viewportY = 0.f;
    float viewportWidthRatio = 1.f;
    float viewportHeightRatio = 1.f;

    if (windowAspectRatio > logicalAspectRatio) {
        viewportWidthRatio = logicalAspectRatio / windowAspectRatio;
        viewportX = (1.f - viewportWidthRatio) / 2.f;
    } else if (windowAspectRatio < logicalAspectRatio) {
        viewportHeightRatio = windowAspectRatio / logicalAspectRatio;
        viewportY = (1.f - viewportHeightRatio) / 2.f;
    }
    sf::FloatRect viewportRect(viewportX, viewportY, viewportWidthRatio, viewportHeightRatio);

    uiView.setSize(LOGICAL_SIZE);
    uiView.setCenter(LOGICAL_SIZE / 2.f);
    uiView.setViewport(viewportRect);

    mainView.setSize(LOGICAL_SIZE);
    mainView.setViewport(viewportRect);
}


void playSfx(const std::string& sfxName) {
    if (soundBuffers.count(sfxName)) {
        sfxPlayer.setBuffer(soundBuffers[sfxName]);
        sfxPlayer.setVolume(gameSettings.sfxVolume);
        sfxPlayer.play();
    } else {
        std::cerr << "SFX not loaded/found: " << sfxName << std::endl;
    }
}

void loadAudio() {
    if (!menuMusic.openFromFile(AUDIO_MUSIC_MENU))
        std::cerr << "Error loading menu music: " << AUDIO_MUSIC_MENU << std::endl;
    else menuMusic.setLoop(true);

    if (!gameMusic.openFromFile(AUDIO_MUSIC_GAME))
        std::cerr << "Error loading game music: " << AUDIO_MUSIC_GAME << std::endl;
    else gameMusic.setLoop(true);

    sf::SoundBuffer buffer;
    if (buffer.loadFromFile(SFX_JUMP)) soundBuffers["jump"] = buffer;
    else std::cerr << "Error loading SFX: " << SFX_JUMP << std::endl;
    if (buffer.loadFromFile(SFX_DEATH)) soundBuffers["death"] = buffer;
    else std::cerr << "Error loading SFX: " << SFX_DEATH << std::endl;
    if (buffer.loadFromFile(SFX_GOAL)) soundBuffers["goal"] = buffer;
    else std::cerr << "Error loading SFX: " << SFX_GOAL << std::endl;
    if (buffer.loadFromFile(SFX_CLICK)) soundBuffers["click"] = buffer;
    else std::cerr << "Error loading SFX: " << SFX_CLICK << std::endl;
    if (buffer.loadFromFile(SFX_SPRING)) soundBuffers["spring"] = buffer; // Load spring SFX
    else std::cerr << "Error loading SFX: " << SFX_SPRING << std::endl;
}

sf::Color getTileColorForBodyType(phys::bodyType type, const sf::Color& defaultColorIfUnknown = sf::Color::Magenta) {
    switch (type) {
        case phys::bodyType::solid:        return sf::Color(100, 100, 100, 255);
        case phys::bodyType::platform:     return sf::Color(70, 150, 200, 180);
        case phys::bodyType::conveyorBelt: return sf::Color(255, 150, 50, 255);
        case phys::bodyType::moving:       return sf::Color(70, 200, 70, 255);
        case phys::bodyType::falling:      return sf::Color(200, 200, 70, 255);
        case phys::bodyType::vanishing:    return sf::Color(200, 70, 200, 255);
        case phys::bodyType::spring:       return sf::Color(255, 255, 0, 255);
        case phys::bodyType::trap:         return sf::Color(255, 20, 20, 255); 
        case phys::bodyType::goal:         return sf::Color(20, 255, 20, 128);
        case phys::bodyType::interactible: return sf::Color(180, 180, 220, 200);
        case phys::bodyType::none:         return sf::Color(0, 0, 0, 0);
        default:                           return defaultColorIfUnknown;
    }
}


void setupLevelAssets(const LevelData& data, sf::RenderWindow& window) {
    bodies.clear();
    tiles.clear();
    activeMovingPlatforms.clear();
    activeInteractibles.clear();

    playerBody.setPosition(data.playerStartPosition);
    playerBody.setVelocity({0.f, 0.f});
    playerBody.setOnGround(false);
    playerBody.setGroundPlatform(nullptr); // Explicitly nullify here
    playerBody.setLastPosition(data.playerStartPosition);

    std::cout << "Setting up assets for level: " << data.levelName << " (Number: " << data.levelNumber << ")" << std::endl;
    std::cout << "  Player Start: (" << data.playerStartPosition.x << ", " << data.playerStartPosition.y << ")" << std::endl;
    std::cout << "  Platform Count: " << data.platforms.size() << std::endl;

    bodies.reserve(data.platforms.size());
    for (const auto& p_body_template : data.platforms) {
        bodies.push_back(p_body_template);

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
        else if (p_body_template.getType() == phys::bodyType::interactible) {
            bool foundDetail = false;
            for (const auto& detail : data.interactiblePlatformDetails) {
                if (detail.id == p_body_template.getID()) {
                    ActiveInteractiblePlatform activeDetail;
                    activeDetail.id = detail.id;
                    activeDetail.interactionType = detail.interactionType;
                    activeDetail.targetBodyTypeEnum = levelManager.stringToBodyType(detail.targetBodyTypeStr);
                    activeDetail.targetTileColor = detail.targetTileColor;
                    activeDetail.hasTargetTileColor = detail.hasTargetTileColor;
                    activeDetail.oneTime = detail.oneTime;
                    activeDetail.cooldown = detail.cooldown;
                    activeDetail.hasBeenInteractedThisSession = false;
                    activeDetail.currentCooldownTimer = 0.f;

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
        newTile.setFillColor(getTileColorForBodyType(body.getType())); // ensures correct initial color
        tiles.push_back(newTile);
    }

    vanishingPlatformCycleTimer = sf::Time::Zero;
    oddEvenVanishing = 1;
}

void updateResolutionDisplayText() {
    if (isFullscreen) {
        resolutionCurrentText.setString("Fullscreen");
    } else {
        if (currentResolutionIndex >= 0 && currentResolutionIndex < static_cast<int>(availableResolutions.size())) {
            resolutionCurrentText.setString(
                std::to_string(availableResolutions[currentResolutionIndex].x) + "x" +
                std::to_string(availableResolutions[currentResolutionIndex].y)
            );
        } else {
            resolutionCurrentText.setString(
                std::to_string(static_cast<int>(LOGICAL_SIZE.x)) + "x" +
                std::to_string(static_cast<int>(LOGICAL_SIZE.y)) + " (Default)"
            );
        }
    }
    sf::FloatRect bounds = resolutionCurrentText.getLocalBounds();
    resolutionCurrentText.setOrigin(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
    resolutionCurrentText.setPosition(LOGICAL_SIZE.x / 2.f, 320);
}


int main(void) {
    sf::RenderWindow window;
    sf::View uiView;
    sf::View mainView;

    sf::Clock gameClock;
    sf::Time timeSinceLastFixedUpdate = sf::Time::Zero;
    const sf::Time TIME_PER_FIXED_UPDATE = sf::seconds(1.f / 60.f);

    bool running = true;
    bool interactKeyPressedThisFrame = false;

    sf::Time currentJumpHoldDuration = sf::Time::Zero;
    int turboMultiplier = 1;

    sf::Font menuFont;
    sf::Text menuTitleText, startButtonText, settingsButtonText, creditsButtonText, exitButtonText;
    sf::Texture menuBgTexture; sf::Sprite menuBgSprite;
    sf::Text settingsTitleText, musicVolumeLabelText, musicVolValText, sfxVolumeLabelText, sfxVolValText, settingsBackText;
    sf::Text musicVolDownText, musicVolUpText, sfxVolDownText, sfxVolUpText;
    sf::Text resolutionLabelText, resolutionPrevText, resolutionNextText, fullscreenToggleText;
    sf::Text creditsTitleText, creditsNamesText, creditsBackText;
    sf::Text gameOverStatusText, gameOverOption1Text, gameOverOption2Text;
    sf::Text debugText;
    sf::RectangleShape playerShape;

    // --- Game Constants ---
    const float PLAYER_MOVE_SPEED = 200.f;
    const float JUMP_INITIAL_VELOCITY = -450.f;
    const float GRAVITY_ACCELERATION = 1200.f;
    const float MAX_FALL_SPEED = 700.f;
    const sf::Time MAX_JUMP_HOLD_TIME = sf::seconds(0.18f);
    const float PLAYER_DEATH_Y_LIMIT = 2000.f;
    const float SPRING_BOUNCE_VELOCITY = -600.f; // New constant for spring


    availableResolutions.push_back({800, 600});
    availableResolutions.push_back({1024, 768});
    availableResolutions.push_back({1280, 720});
    availableResolutions.push_back({1366, 768});
    availableResolutions.push_back({1600, 900});
    availableResolutions.push_back({1920, 1080});

    bool foundInitialRes = false;
    for(size_t i=0; i < availableResolutions.size(); ++i) {
        if (availableResolutions[i].x == static_cast<unsigned int>(LOGICAL_SIZE.x) && availableResolutions[i].y == static_cast<unsigned int>(LOGICAL_SIZE.y)) {
            currentResolutionIndex = static_cast<int>(i);
            foundInitialRes = true;
            break;
        }
    }
    if (!foundInitialRes && !availableResolutions.empty()) {
        currentResolutionIndex = 0;
    } else if (availableResolutions.empty()){
        currentResolutionIndex = -1;
    }

    const sf::Vector2f tileSize(32.f, 32.f); // Used for player body init

    applyAndRecreateWindow(window, uiView, mainView);

    GameState currentState = GameState::MENU;
    levelManager.setMaxLevels(5);
    levelManager.setLevelBasePath("../assets/levels/");
    levelManager.setTransitionProperties(0.75f);
    levelManager.setGeneralLoadingScreenImage(IMG_LOAD_GENERAL);
    levelManager.setNextLevelLoadingScreenImage(IMG_LOAD_NEXT);
    levelManager.setRespawnLoadingScreenImage(IMG_LOAD_RESPAWN);

    loadAudio();

    playerBody = phys::DynamicBody({0.f, 0.f}, tileSize.x, tileSize.y, {0.f, 0.f});

    if (!menuFont.loadFromFile(FONT_PATH)) {
        std::cerr << "FATAL: Failed to load font: " << FONT_PATH << ". Trying fallback." << std::endl;
        #ifdef _WIN32
        if (!menuFont.loadFromFile("C:/Windows/Fonts/arialbd.ttf")) { std::cerr << "Windows fallback font failed.\n"; return -1; }
        #else        
        if (!menuFont.loadFromFile("/System/Library/Fonts/Supplemental/Arial Bold.ttf") &&
            !menuFont.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf")) {
            std::cerr << "Linux/macOS fallback font failed.\n"; return -1;
            }
        #endif
        if (menuFont.getInfo().family.empty()) {std::cerr << "All font loading attempts failed.\n"; return -1;}
        std::cout << "Loaded a fallback font." << std::endl;
    }

    auto setupTextUI = [&](sf::Text& text, const sf::String& str, float yPos, unsigned int charSize = 30, float xOffset = 0.f) {
        text.setFont(menuFont);
        text.setString(str);
        text.setCharacterSize(charSize);
        text.setFillColor(sf::Color::White);
        sf::FloatRect bounds = text.getLocalBounds();
        text.setOrigin(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
        text.setPosition(LOGICAL_SIZE.x / 2.f + xOffset, yPos);
    };

    if (menuBgTexture.loadFromFile(IMG_MENU_BG)) {
        menuBgSprite.setTexture(menuBgTexture);
        menuBgSprite.setScale(LOGICAL_SIZE.x / menuBgTexture.getSize().x,
                              LOGICAL_SIZE.y / menuBgTexture.getSize().y);
        menuBgSprite.setPosition(0.f,0.f);
    }
    else {
        std::cerr << "Warning: Menu BG image not found: " << IMG_MENU_BG << std::endl;
    }

    setupTextUI(menuTitleText, "Project - T", 100, 48);
    setupTextUI(startButtonText, "Start Game", 250);
    setupTextUI(settingsButtonText, "Settings", 300);
    setupTextUI(creditsButtonText, "Credits", 350);
    setupTextUI(exitButtonText, "Exit", 400);

    setupTextUI(settingsTitleText, "Settings", 70, 40);
    setupTextUI(musicVolumeLabelText, "Music Volume:", 150, 24, -100);
    setupTextUI(musicVolDownText, "<", 150, 24, 20);
    setupTextUI(musicVolValText, "", 150, 24, 80);
    setupTextUI(musicVolUpText, ">", 150, 24, 140);
    setupTextUI(sfxVolumeLabelText, "SFX Volume:", 200, 24, -100);
    setupTextUI(sfxVolDownText, "<", 200, 24, 20);
    setupTextUI(sfxVolValText, "", 200, 24, 80);
    setupTextUI(sfxVolUpText, ">", 200, 24, 140);
    setupTextUI(resolutionLabelText, "Resolution:", 270, 24, -100);
    setupTextUI(resolutionPrevText, "<", 320, 24, -30);
    resolutionCurrentText.setFont(menuFont);
    resolutionCurrentText.setCharacterSize(24);
    resolutionCurrentText.setFillColor(sf::Color::White);
    updateResolutionDisplayText();
    setupTextUI(resolutionNextText, ">", 320, 24, 30);
    setupTextUI(fullscreenToggleText, "Toggle Fullscreen", 370, 24);
    setupTextUI(settingsBackText, "Back to Menu", 450);

    setupTextUI(creditsTitleText, "Credits", 100, 40);
    setupTextUI(creditsNamesText, "Jan\nZean\nJecer\nGian", 250, 28);
    setupTextUI(creditsBackText, "Back to Menu", 450);

    setupTextUI(gameOverStatusText, "", 150, 36);
    setupTextUI(gameOverOption1Text, "", 280);
    setupTextUI(gameOverOption2Text, "Main Menu", 330);

    sf::Color defaultBtnColor = sf::Color::White;
    sf::Color hoverBtnColor = sf::Color::Yellow;
    sf::Color exitBtnHoverColor = sf::Color::Red;

    playerShape.setFillColor(sf::Color(220, 220, 250, 255));
    playerShape.setSize(sf::Vector2f(playerBody.getWidth(), playerBody.getHeight()));

    debugText.setFont(menuFont);
    debugText.setCharacterSize(14);
    debugText.setFillColor(sf::Color::White);
    debugText.setPosition(10.f, 10.f);

    menuMusic.setVolume(gameSettings.musicVolume);
    gameMusic.setVolume(gameSettings.musicVolume);
    menuMusic.play();

    // --- MAIN GAME LOOP ---
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
                                currentResolutionIndex--; if (currentResolutionIndex < 0) currentResolutionIndex = static_cast<int>(availableResolutions.size()) - 1;
                                applyAndRecreateWindow(window, uiView, mainView); updateResolutionDisplayText();
                            }
                        } else if (resolutionNextText.getGlobalBounds().contains(worldPosUi)) {
                             if (!isFullscreen && !availableResolutions.empty()) {
                                currentResolutionIndex++; if (currentResolutionIndex >= static_cast<int>(availableResolutions.size())) currentResolutionIndex = 0;
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
                                    currentState = GameState::TRANSITIONING; gameMusic.play();
                                } else { currentState = GameState::MENU; gameMusic.stop(); menuMusic.play(); levelManager.setCurrentLevelNumber(0); }
                            } else if (currentState == GameState::GAME_OVER_WIN) {
                                levelManager.setCurrentLevelNumber(0);
                                if (levelManager.requestLoadNextLevel(currentLevelData)) {
                                    currentState = GameState::TRANSITIONING; menuMusic.stop(); gameMusic.play();
                                } else { currentState = GameState::MENU; menuMusic.play(); }
                            }
                        } else if (gameOverOption2Text.getGlobalBounds().contains(worldPosUi)) {
                            currentState = GameState::MENU; gameMusic.stop(); menuMusic.play();
                            levelManager.setCurrentLevelNumber(0);
                        }
                    }
                     if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
                        currentState = GameState::MENU; gameMusic.stop(); menuMusic.play(); levelManager.setCurrentLevelNumber(0);
                     }
                    break;
                case GameState::TRANSITIONING:
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
            playerShape.setSize(sf::Vector2f(playerBody.getWidth(), playerBody.getHeight()));

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
                if (newJumpPressThisFrame && !playerBody.getGroundPlatformTemporarilyIgnored()) { // Don't play jump sound if trying to drop through
                    const phys::PlatformBody* groundPlat = playerBody.getGroundPlatform();
                    // Add defensive check before dereferencing groundPlat for its type
                    bool safeToAccessGroundPlat = false;
                    if (groundPlat) {
                        for (const auto& body_ref : bodies) {
                            if (&body_ref == groundPlat) {
                                safeToAccessGroundPlat = true;
                                break;
                            }
                        }
                    }
                    if (!safeToAccessGroundPlat || groundPlat->getType() != phys::bodyType::spring) { // No jump sound if on spring, spring has its own
                         playSfx("jump");
                    }
                }
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

                // Update Interactible Cooldowns
                for (auto& pair : activeInteractibles) {
                    ActiveInteractiblePlatform& interactible = pair.second;
                    if (interactible.currentCooldownTimer > 0.f) {
                        interactible.currentCooldownTimer -= TIME_PER_FIXED_UPDATE.asSeconds();
                        if (interactible.currentCooldownTimer < 0.f) {
                            interactible.currentCooldownTimer = 0.f;
                        }
                    }
                }

                // Update Tiles (Falling, Vanishing)
                for (size_t i = 0; i < bodies.size(); ++i) {
                    if (tiles.size() <= i) continue; // Boundary check for tiles vector

                    // Only update tile if the corresponding body is not 'none' OR if it's a vanishing tile that might reappear
                    if (bodies[i].getType() != phys::bodyType::none ||
                       (tiles[i].getFillColor() != getTileColorForBodyType(phys::bodyType::vanishing) && bodies[i].getType() == phys::bodyType::vanishing) ) { // Extra check for vanishing tile reappearance
                        tiles[i].update(TIME_PER_FIXED_UPDATE);
                    }


                    if (bodies[i].getType() == phys::bodyType::falling) {
                        bool playerOnThis = playerBody.isOnGround() && playerBody.getGroundPlatform() && playerBody.getGroundPlatform()->getID() == bodies[i].getID();
                        if (playerOnThis && !tiles[i].isFalling() && !tiles[i].hasFallen()) {
                            tiles[i].startFalling(sf::seconds(0.25f));
                        }
                        if (tiles[i].isFalling() && !bodies[i].isFalling()) { // Visuals started falling
                            bodies[i].setFalling(true); // Sync physics body if not already
                        }
                        if (tiles[i].hasFallen() && bodies[i].getType() != phys::bodyType::none) { // Tile visually "gone"
                            // Check if player is on this platform before making it 'none'
                            if (playerBody.getGroundPlatform() == &bodies[i]) {
                                playerBody.setOnGround(false);
                                playerBody.setGroundPlatform(nullptr);
                            }
                            bodies[i].setPosition({-9999.f, -9999.f}); // Move physics body out of play
                            bodies[i].setType(phys::bodyType::none);    // Make it non-collidable
                        }
                    } else if (bodies[i].getType() == phys::bodyType::vanishing) {
                        // Determine original template position for respawn
                        sf::Vector2f originalPos = {-9999.f, -9999.f};
                        for(const auto& templ : currentLevelData.platforms) {
                            if(templ.getID() == bodies[i].getID()){
                                originalPos = templ.getPosition();
                                break;
                            }
                        }

                        bool is_even_id = (bodies[i].getID() % 2 == 0);
                        bool should_be_fading_out_now = (oddEvenVanishing == 1 && is_even_id) || (oddEvenVanishing == -1 && !is_even_id);
                        float phaseTime = std::fmod(vanishingPlatformCycleTimer.asSeconds(), 1.0f); // Cycle is 1 second total
                        sf::Color originalColor = getTileColorForBodyType(phys::bodyType::vanishing);
                        float alpha_val;

                        if (should_be_fading_out_now) { // Fading out
                            alpha_val = math::easing::sineEaseInOut(phaseTime, 255.f, -255.f, 1.f);
                        } else { // Fading in
                            alpha_val = math::easing::sineEaseInOut(phaseTime, 0.f, 255.f, 1.f);
                        }
                        alpha_val = std::max(0.f, std::min(255.f, alpha_val));
                        tiles[i].setFillColor(sf::Color(originalColor.r, originalColor.g, originalColor.b, static_cast<sf::Uint8>(alpha_val)));

                        if (alpha_val <= 10.f) { // Visually gone or nearly gone
                            if (bodies[i].getType() != phys::bodyType::none) { // If it's not already 'none'
                                // Check if player is on this platform before making it 'none'
                                if (playerBody.getGroundPlatform() == &bodies[i]) { // If player was on it
                                    playerBody.setOnGround(false);
                                    playerBody.setGroundPlatform(nullptr);
                                }
                                bodies[i].setPosition({-9999.f, -9999.f}); // Move physics body
                                bodies[i].setType(phys::bodyType::none);    // Make non-collidable
                            }
                        } else { // Should be visible
                            if (bodies[i].getType() == phys::bodyType::none) { // If it was 'none', bring it back
                                 if(originalPos.x > -9998.f) { // Check if originalPos was found
                                    bodies[i].setPosition(originalPos);
                                    tiles[i].setPosition(originalPos); // Sync tile visual position
                                }
                                bodies[i].setType(phys::bodyType::vanishing); // Restore original type
                            }
                        }
                    }
                }
                vanishingPlatformCycleTimer += TIME_PER_FIXED_UPDATE;
                if (vanishingPlatformCycleTimer.asSeconds() >= 1.0f) { // Vanishing cycle duration (1s fade out, 1s fade in for each set)
                    vanishingPlatformCycleTimer -= sf::seconds(1.0f);
                    oddEvenVanishing *= -1; // Switch which set is vanishing/appearing
                }


                sf::Vector2f pVel = playerBody.getVelocity();
                pVel.x = horizontalInput * PLAYER_MOVE_SPEED * turboMultiplier;
                if (!playerBody.isOnGround()) {
                    pVel.y += GRAVITY_ACCELERATION * TIME_PER_FIXED_UPDATE.asSeconds();
                    pVel.y = std::min(pVel.y, MAX_FALL_SPEED);
                }
                if (newJumpPressThisFrame) {
                    pVel.y = JUMP_INITIAL_VELOCITY; currentJumpHoldDuration = sf::microseconds(1);
                } else if (jumpIntentThisFrame && currentJumpHoldDuration > sf::Time::Zero && currentJumpHoldDuration < MAX_JUMP_HOLD_TIME) {
                    // Allow extending jump if already in air from a regular jump (not spring)
                    // Add defensive check for ground platform here too
                    const phys::PlatformBody* groundPlatForJumpExtend = playerBody.getGroundPlatform();
                    bool safeToAccessGroundPlatForJumpExtend = false;
                    if(groundPlatForJumpExtend){
                        for(const auto& body_ref : bodies){
                            if(&body_ref == groundPlatForJumpExtend){
                                safeToAccessGroundPlatForJumpExtend = true;
                                break;
                            }
                        }
                    }
                    if (playerBody.getVelocity().y < 0 && (!safeToAccessGroundPlatForJumpExtend || groundPlatForJumpExtend->getType() != phys::bodyType::spring)) {
                         pVel.y = JUMP_INITIAL_VELOCITY; // Maintain upward velocity for a bit
                    }
                    currentJumpHoldDuration += TIME_PER_FIXED_UPDATE;
                } else if (!jumpIntentThisFrame || currentJumpHoldDuration >= MAX_JUMP_HOLD_TIME) {
                    currentJumpHoldDuration = sf::Time::Zero;
                }
                playerBody.setVelocity(pVel);

                phys::CollisionResolutionInfo resolutionResult = phys::CollisionSystem::resolveCollisions(playerBody, bodies, TIME_PER_FIXED_UPDATE.asSeconds());
                 pVel = playerBody.getVelocity(); // Get velocity after collision response

                if (playerBody.isOnGround()) {
                    currentJumpHoldDuration = sf::Time::Zero; // Reset jump hold if landed
                    if (playerBody.getGroundPlatform()) {
                        // Defensive check for playerBody.getGroundPlatform() before dereferencing
                        const phys::PlatformBody* currentGroundPlatform = playerBody.getGroundPlatform();
                        bool safeToAccessCurrentGroundPlatform = false;
                        if (currentGroundPlatform) {
                            for (const auto& body_ref : bodies) {
                                if (&body_ref == currentGroundPlatform) {
                                    safeToAccessCurrentGroundPlatform = true;
                                    break;
                                }
                            }
                        }

                        if (safeToAccessCurrentGroundPlatform) { // Only proceed if pointer is likely valid
                            const phys::PlatformBody& pf = *currentGroundPlatform; // Dereference now safer
                            if (pf.getType() == phys::bodyType::conveyorBelt) {
                                playerBody.setPosition(playerBody.getPosition() + pf.getSurfaceVelocity() * TIME_PER_FIXED_UPDATE.asSeconds());
                            } else if (pf.getType() == phys::bodyType::moving) {
                                 for(const auto& activePlat : activeMovingPlatforms) {
                                    if (activePlat.id == pf.getID()) {
                                        phys::PlatformBody* movingPhysBody = nullptr;
                                        for(auto& b_ref : bodies) if(b_ref.getID() == activePlat.id) {movingPhysBody = &b_ref; break;}
                                        if(movingPhysBody){
                                            sf::Vector2f platformDisp = movingPhysBody->getPosition() - activePlat.lastFramePosition;
                                            playerBody.setPosition(playerBody.getPosition() + platformDisp);
                                        }
                                        break;
                                    }
                                }
                            } else if (pf.getType() == phys::bodyType::spring) {
                                pVel.y = SPRING_BOUNCE_VELOCITY;
                                playerBody.setOnGround(false); // Player is launched, no longer on ground immediately
                                playerBody.setGroundPlatform(nullptr);
                                playSfx("spring");
                            }
                        } else {
                            // Ground platform pointer was non-null but became invalid,
                            // might need to force player off ground if this state is reached.
                            // This case implies an issue elsewhere if `resolveCollisions` didn't already handle it.
                             playerBody.setOnGround(false);
                             playerBody.setGroundPlatform(nullptr);
                        }
                    }
                }
                if (resolutionResult.hitCeiling && pVel.y < 0.f) {
                    pVel.y = 0.f; currentJumpHoldDuration = MAX_JUMP_HOLD_TIME; // Stop jump if ceiling hit
                }
                playerBody.setVelocity(pVel);

                bool trapHit = false;
                for (const auto& body : bodies) { // Iterate over `bodies` not `currentLevelData.platforms`
                    if (body.getType() == phys::bodyType::trap && body.getAABB().intersects(playerBody.getAABB())) {
                        trapHit = true;
                        break;
                    }
                }
                if (trapHit) {
                    playSfx("death");
                    currentState = GameState::GAME_OVER_LOSE_DEATH;
                    gameMusic.pause();
                    break;
                }

                if (interactKeyPressedThisFrame) {
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
                            goto end_fixed_update_interact;
                        }
                    }

                    for (size_t i = 0; i < bodies.size(); ++i) {
                        if (bodies[i].getType() == phys::bodyType::interactible && playerBody.getAABB().intersects(bodies[i].getAABB())) {
                            auto it = activeInteractibles.find(bodies[i].getID());
                            if (it != activeInteractibles.end()) {
                                ActiveInteractiblePlatform& interactState = it->second;
                                if (interactState.currentCooldownTimer > 0.f) continue;
                                if (interactState.oneTime && interactState.hasBeenInteractedThisSession) continue;

                                if (interactState.interactionType == "changeSelf") {
                                    playSfx("click");
                                    phys::bodyType oldType = bodies[i].getType();
                                    bodies[i].setType(interactState.targetBodyTypeEnum);
                                    std::cout << "Platform " << bodies[i].getID() << " interacted, old type: " << static_cast<int>(oldType)
                                              << ", new type: " << static_cast<int>(interactState.targetBodyTypeEnum) << std::endl;

                                    if (tiles.size() > i) {
                                        if (interactState.hasTargetTileColor) {
                                            tiles[i].setFillColor(interactState.targetTileColor);
                                        } else {
                                            tiles[i].setFillColor(getTileColorForBodyType(interactState.targetBodyTypeEnum));
                                        }
                                    }

                                    if (interactState.targetBodyTypeEnum == phys::bodyType::none) {
                                        // Check if player is on this platform before making it 'none'
                                        if (playerBody.getGroundPlatform() == &bodies[i]) {
                                            playerBody.setOnGround(false);
                                            playerBody.setGroundPlatform(nullptr);
                                        }
                                        bodies[i].setPosition({-10000.f, -10000.f});
                                        if (tiles.size() > i) tiles[i].setFillColor(sf::Color::Transparent); // Make tile invisible
                                    }

                                    if (interactState.oneTime) {
                                        interactState.hasBeenInteractedThisSession = true;
                                    } else {
                                        interactState.currentCooldownTimer = interactState.cooldown;
                                    }
                                    break;
                                }
                            }
                        }
                    }
                }
                end_fixed_update_interact:;

                if (playerBody.getPosition().y > PLAYER_DEATH_Y_LIMIT) {
                    playSfx("death");
                    currentState = GameState::GAME_OVER_LOSE_FALL;
                    gameMusic.pause();
                    break;
                }
            } // end of fixed update loop
        } // end of PLAYING state
        else if (currentState == GameState::TRANSITIONING) {
            levelManager.update(frameDeltaTime.asSeconds(), window);
            if (!levelManager.isTransitioning()) {
                setupLevelAssets(currentLevelData, window);
                currentState = GameState::PLAYING;
                if (gameMusic.getStatus() != sf::Music::Playing) {
                     menuMusic.stop(); gameMusic.play();
                }
            }
        }

        window.setTitle("Project T");

        window.clear(sf::Color::Black);
        sf::Vector2i currentMousePixelPos = sf::Mouse::getPosition(window);
        sf::Vector2f currentMouseWorldUiPos = window.mapPixelToCoords(currentMousePixelPos, uiView);

        switch(currentState) {
             case GameState::MENU:
                window.setView(uiView);
                if(menuBgSprite.getTexture()) { window.draw(menuBgSprite); }
                else { sf::RectangleShape bg(LOGICAL_SIZE); bg.setFillColor(sf::Color(20,20,50)); bg.setPosition(0,0); window.draw(bg); }
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
                { sf::RectangleShape bg(LOGICAL_SIZE); bg.setFillColor(sf::Color(20,50,20)); bg.setPosition(0,0); window.draw(bg); }
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
                { sf::RectangleShape bg(LOGICAL_SIZE); bg.setFillColor(sf::Color(50,20,20)); bg.setPosition(0,0); window.draw(bg); }
                creditsBackText.setFillColor(defaultBtnColor);
                if(creditsBackText.getGlobalBounds().contains(currentMouseWorldUiPos)) creditsBackText.setFillColor(hoverBtnColor);
                window.draw(creditsTitleText); window.draw(creditsNamesText); window.draw(creditsBackText);
                break;
            case GameState::PLAYING:
                mainView.setCenter( playerBody.getPosition().x + playerBody.getWidth()/2.f, playerBody.getPosition().y + playerBody.getHeight()/2.f - 50.f);
                window.setView(mainView);
                {
                    sf::RectangleShape gameplayAreaBackground(mainView.getSize());
                    gameplayAreaBackground.setFillColor(currentLevelData.backgroundColor);
                    gameplayAreaBackground.setPosition(mainView.getCenter() - mainView.getSize() / 2.f);
                    window.draw(gameplayAreaBackground);
                }
                playerShape.setPosition(playerBody.getPosition());
                for (const auto& t : tiles) { // Draw tiles based on their current state
                    if (t.getFillColor().a > 5 && !t.hasFallen()) { // Only draw if visible and not fully fallen
                         window.draw(t);
                    }
                }
                window.draw(playerShape);
                
                // --- START OF MODIFIED DEBUG TEXT SECTION ---
                window.setView(uiView);
                { // Scope for debug string construction
                    std::string debugString = "Lvl: " + std::to_string(currentLevelData.levelNumber) +
                                             " Pos: " + std::to_string(static_cast<int>(playerBody.getPosition().x)) + "," + std::to_string(static_cast<int>(playerBody.getPosition().y)) +
                                             " Vel: " + std::to_string(static_cast<int>(playerBody.getVelocity().x)) + "," + std::to_string(static_cast<int>(playerBody.getVelocity().y)) +
                                             " Ground: " + (playerBody.isOnGround() ? "Y" : "N");

                    const phys::PlatformBody* groundPlat = playerBody.getGroundPlatform();
                    if (groundPlat) {
                        bool platformStillExistsAndMatches = false;
                        for (const auto& body_ref : bodies) { 
                            if (&body_ref == groundPlat) { 
                                platformStillExistsAndMatches = true;
                                break;
                            }
                        }

                        if (platformStillExistsAndMatches) {
                            debugString += " (ID:" + std::to_string(groundPlat->getID()) +
                                           (groundPlat->getType() == phys::bodyType::none ? " TYPE_NONE" : "") + ")";
                        } else {
                            debugString += " (GroundRef: INVALID)"; // Simplified "INVALID"
                            // Consider if we should nullify player's ground platform if it's invalid.
                            // This implies that resolveCollisions or other logic didn't catch it.
                            // playerBody.setOnGround(false); // This might be too aggressive if resolveCollisions handles it next frame
                            // playerBody.setGroundPlatform(nullptr); 
                        }
                    }
                    // No "else" needed, if groundPlat is nullptr, string is already complete.
                    debugText.setString(debugString);
                }
                window.draw(debugText);
                break;
            case GameState::TRANSITIONING:
                window.setView(uiView);
                {
                    sf::RectangleShape transitionBg(LOGICAL_SIZE);
                    transitionBg.setFillColor(sf::Color::Black);
                    transitionBg.setPosition(0,0);
                    window.draw(transitionBg);
                }
                levelManager.draw(window);
                break;
            case GameState::GAME_OVER_WIN:
                 window.setView(uiView);
                 { sf::RectangleShape bg(LOGICAL_SIZE); bg.setFillColor(sf::Color(20,60,20)); bg.setPosition(0,0); window.draw(bg); }
                 gameOverStatusText.setString("All Levels Cleared! You Win!");
                 gameOverOption1Text.setString("Play Again (Level 1)");
                 gameOverOption1Text.setFillColor(defaultBtnColor); 
                 gameOverOption2Text.setFillColor(defaultBtnColor); 

                 if(gameOverOption1Text.getGlobalBounds().contains(currentMouseWorldUiPos)) {
                     gameOverOption1Text.setFillColor(hoverBtnColor);
                 }
                 if(gameOverOption2Text.getGlobalBounds().contains(currentMouseWorldUiPos)) {
                     gameOverOption2Text.setFillColor(hoverBtnColor); 
                 }
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
                gameOverOption2Text.setFillColor(defaultBtnColor); // Ensure option 2 also gets default color

                if(gameOverOption1Text.getGlobalBounds().contains(currentMouseWorldUiPos)) {
                    gameOverOption1Text.setFillColor(hoverBtnColor);
                }
                if(gameOverOption2Text.getGlobalBounds().contains(currentMouseWorldUiPos)) {
                    gameOverOption2Text.setFillColor(hoverBtnColor); // CORRECTED: hoverBtnColor
                }
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