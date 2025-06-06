#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <SFML/Window/Event.hpp>
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
#include "CollisionSystem.hpp"
#include "Player.hpp"
#include "PlatformBody.hpp"
#include "Tile.hpp"
#include "PhysicsTypes.hpp"
#include "LevelManager.hpp"
#include "Optimizer.hpp"

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
    float musicVolume = 50.f;
    float sfxVolume = 70.f;
};

// --- Global Variables for Window/Resolution Management ---
const sf::Vector2f LOGICAL_SIZE(800.f, 600.f);
std::vector<sf::VideoMode> availableVideoModes;
int currentResolutionIndex = 0;
bool isFullscreen = true;

sf::Font defaultFont("../assets/fonts/ARIALBD.TTF");
sf::Text resolutionCurrentText(defaultFont);
void updateResolutionDisplayText();

// --- Global Game Objects ---
LevelManager levelManager;
LevelData currentLevelData;
phys::DynamicBody playerBody;
std::vector<phys::PlatformBody> bodies;
std::vector<Tile> tiles;

struct ActiveMovingPlatform {
    unsigned int id;
    sf::Vector2f movementAnchorPosition;
    char axis;
    float distance;
    float cycleTime;
    float cycleDuration;
    int initialDirection;
    sf::Vector2f lastFrameActualPosition;
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
    unsigned int linkedID = 0;
};
std::map<unsigned int, ActiveInteractiblePlatform> activeInteractibles;

sf::Time vanishingPlatformCycleTimer = sf::Time::Zero;
int oddEvenVanishing = 1;

GameSettings gameSettings;

sf::Music menuMusic;
sf::Music gameMusic;
std::map<std::string, sf::SoundBuffer> soundBuffers;
sf::SoundBuffer defaultSoundBuffer("../assets/audio/default.wav");
sf::Sound sfxPlayer(defaultSoundBuffer);

// --- Asset Paths ---
const std::string FONT_PATH = "../assets/fonts/ARIALBD.TTF";
const std::string IMG_MENU_BG = "../assets/images/menu-bg-cropped.png";
const std::string IMG_LOAD_GENERAL = "../assets/images/Loading-screen.png";
const std::string IMG_LOAD_NEXT = "../assets/images/Loading-screen.png";
const std::string IMG_LOAD_RESPAWN = "../assets/images/respawn.png";
const std::string AUDIO_MUSIC_MENU = "../assets/audio/music_menu.ogg";
const std::string AUDIO_MUSIC_GAME = "../assets/audio/music_ingame.ogg";
const std::string SFX_JUMP = "../assets/audio/sfx_jump.wav";
const std::string SFX_DEATH = "../assets/audio/sfx_death.wav";
const std::string SFX_GOAL = "../assets/audio/sfx_goal.wav";
const std::string SFX_CLICK = "../assets/audio/sfx_click.wav";
const std::string SFX_SPRING = "../assets/audio/sfx_spring.wav";
const std::string SFX_PORTAL = "../assets/audio/sfx_portal.wav";

// Sprites
std::map<int, sf::Texture> LevelBackgrounds;
// PLAYER SPRITE LOADING (basic functionality, to be replaced later)
const std::string playerCharacterTexturePath = "../assets/sprites/PlayerChar.png";
sf::Texture playerTexture(playerCharacterTexturePath);

// --- Function to populate available resolutions ---
void populateAvailableResolutions() {
availableVideoModes = sf::VideoMode::getFullscreenModes();
std::sort(availableVideoModes.begin(), availableVideoModes.end(), [](const sf::VideoMode& a, const sf::VideoMode& b) {
if (a.size.x != b.size.x) return a.size.x < b.size.x;
return a.size.y < b.size.y;
});

availableVideoModes.erase(std::unique(availableVideoModes.begin(), availableVideoModes.end(), [](const sf::VideoMode& a, const sf::VideoMode& b){
return a.size.x == b.size.x && a.size.y == b.size.y;
}), availableVideoModes.end());

std::vector<sf::VideoMode> commonWindowed;
    commonWindowed.emplace_back(sf::VideoMode({800, 600}, 32));
    commonWindowed.emplace_back(sf::VideoMode({1024, 768}, 32));
    commonWindowed.emplace_back(sf::VideoMode({1280, 720}, 32));
    commonWindowed.emplace_back(sf::VideoMode({1366, 768}, 32));
    commonWindowed.emplace_back(sf::VideoMode({1600, 900}, 32));
    commonWindowed.emplace_back(sf::VideoMode({1920, 1080}, 32));

for(const auto& mode : commonWindowed) {
    bool found = false;
    for(const auto& existing : availableVideoModes) {
        if(existing.size.x == mode.size.x && existing.size.y == mode.size.y) {
            found = true;
            break;
        }
    }
    if(!found) availableVideoModes.push_back(mode);
}
 std::sort(availableVideoModes.begin(), availableVideoModes.end(), [](const sf::VideoMode& a, const sf::VideoMode& b) {
    if (a.size.x != b.size.x) return a.size.x < b.size.x;
    return a.size.y < b.size.y;
});
availableVideoModes.erase(std::unique(availableVideoModes.begin(), availableVideoModes.end(), [](const sf::VideoMode& a, const sf::VideoMode& b){
    return a.size.x == b.size.x && a.size.y == b.size.y;
}), availableVideoModes.end());

currentResolutionIndex = -1;
if (!availableVideoModes.empty()) {
    for (size_t i = 0; i < availableVideoModes.size(); ++i) {
        if (availableVideoModes[i].size.x == static_cast<unsigned int>(LOGICAL_SIZE.x) &&
            availableVideoModes[i].size.y == static_cast<unsigned int>(LOGICAL_SIZE.y)) {
            currentResolutionIndex = static_cast<int>(i);
            break;
        }
    }
    if (currentResolutionIndex == -1) currentResolutionIndex = 0;
}
}

void applyAndRecreateWindow(sf::RenderWindow& window, sf::View& uiView, sf::View& mainView) {
sf::VideoMode mode;
sf::State windowState;

if (isFullscreen) {
    if (!sf::VideoMode::getFullscreenModes().empty()) {
         mode = sf::VideoMode::getFullscreenModes()[0];
    } else {
        mode = sf::VideoMode({static_cast<unsigned int>(LOGICAL_SIZE.x), static_cast<unsigned int>(LOGICAL_SIZE.y)});
        std::cerr << "Warning: No fullscreen modes available, falling back to windowed." << std::endl;
        isFullscreen = false;
    }
    windowState = sf::State::Fullscreen;
} else {
    if (!availableVideoModes.empty() && currentResolutionIndex >= 0 && currentResolutionIndex < static_cast<int>(availableVideoModes.size())) {
        mode = availableVideoModes[currentResolutionIndex];
    } else {
        mode = sf::VideoMode({static_cast<unsigned int>(LOGICAL_SIZE.x), static_cast<unsigned int>(LOGICAL_SIZE.y)});
        if (availableVideoModes.empty()) currentResolutionIndex = -1;
        else currentResolutionIndex = 0;
    }
    windowState = sf::State::Windowed; // sfml3 migrate: assuming sfml2 default = windowed mode
}

window.create(mode, "Celestial Speedrun", windowState);
window.setKeyRepeatEnabled(false);
window.setVerticalSyncEnabled(true);

    float windowWidth = static_cast<float>(window.getSize().x);
    float windowHeight = static_cast<float>(window.getSize().y);
    float windowAspectRatio = (windowHeight == 0.f) ? 1.f : windowWidth / windowHeight;
    float logicalAspectRatio = LOGICAL_SIZE.x / LOGICAL_SIZE.y;

    float viewportX = 0.f, viewportY = 0.f, viewportWidthRatio = 1.f, viewportHeightRatio = 1.f;

if (windowAspectRatio > logicalAspectRatio) {
    viewportWidthRatio = logicalAspectRatio / windowAspectRatio;
    viewportX = (1.f - viewportWidthRatio) / 2.f;
} else if (windowAspectRatio < logicalAspectRatio) {
    viewportHeightRatio = windowAspectRatio / logicalAspectRatio;
    viewportY = (1.f - viewportHeightRatio) / 2.f;
}
sf::FloatRect viewportRect({viewportX, viewportY}, {viewportWidthRatio, viewportHeightRatio});

    uiView.setSize(LOGICAL_SIZE);
    uiView.setCenter(LOGICAL_SIZE / 2.f);
    uiView.setViewport(viewportRect);

    mainView.setSize(LOGICAL_SIZE);
    mainView.setViewport(viewportRect);
}

void playSfx(const std::string& sfxName) {
    auto it = soundBuffers.find(sfxName);
    if (it != soundBuffers.end()) {
        sfxPlayer.setBuffer(it->second);
        sfxPlayer.setVolume(gameSettings.sfxVolume);
        sfxPlayer.play();
    } else {
        std::cerr << "SFX not loaded/found: " << sfxName << std::endl;
    }
}

void loadAudio() {
if (!menuMusic.openFromFile(AUDIO_MUSIC_MENU))
std::cerr << "Error loading menu music: " << AUDIO_MUSIC_MENU << std::endl;
else menuMusic.setLooping(true);

if (!gameMusic.openFromFile(AUDIO_MUSIC_GAME))
    std::cerr << "Error loading game music: " << AUDIO_MUSIC_GAME << std::endl;
else gameMusic.setLooping(true);

    auto loadSfxBuffer = [&](const std::string& name, const std::string& path) {
        sf::SoundBuffer buffer;
        if (buffer.loadFromFile(path)) {
            soundBuffers[name] = buffer;
        } else {
            std::cerr << "Error loading SFX: " << path << std::endl;
        }
    };

    loadSfxBuffer("jump", SFX_JUMP);
    loadSfxBuffer("death", SFX_DEATH);
    loadSfxBuffer("goal", SFX_GOAL);
    loadSfxBuffer("click", SFX_CLICK);
    loadSfxBuffer("spring", SFX_SPRING);
    loadSfxBuffer("portal", SFX_PORTAL);
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
        case phys::bodyType::portal:       return sf::Color(147, 112, 219, 200);
        case phys::bodyType::none:         return sf::Color::Transparent;
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
    playerBody.setGroundPlatform(nullptr);
    playerBody.setLastPosition(data.playerStartPosition);

    std::cout << "Level " << data.levelNumber << " - TexturesList contains keys: ";
    for (const auto& [id, tex] : data.TexturesList) {
        std::cout << id << " ";
    }
    std::cout << std::endl;


    // Load custom background, if existing
    std::cout << "CHECKING FOR LEVEL BACKGROUND: " << data.levelNumber << std::endl;
    if (data.TexturesList.find(LEVEL_BG_ID) != data.TexturesList.end()){
        // Level has custom background
        std::cout << "FOUND LEVEL " << data.levelNumber << " BACKGROUND!" << std::endl;
        if (LevelBackgrounds.find(data.levelNumber) == LevelBackgrounds.end()){
            sf::Texture levelbg = data.TexturesList.find(LEVEL_BG_ID)->second;
            LevelBackgrounds.emplace(data.levelNumber, std::move(levelbg));
            std::cout << "LOADED LEVEL BACKGROUND: " << data.levelNumber << std::endl;
        }
    }

    bodies.reserve(data.platforms.size());
    for (const auto& p_body_template : data.platforms) {
        bodies.push_back(p_body_template);
        phys::PlatformBody& new_body_ref = bodies.back();

        if (new_body_ref.getType() == phys::bodyType::moving) {
            bool foundDetail = false;
            for(const auto& detail : data.movingPlatformDetails){
                if(detail.id == new_body_ref.getID()){
                    sf::Vector2f movementAnchor = detail.startPosition;
                    float t0_offset = 0.f;
                    if (detail.cycleDuration > 0.f && detail.cycleDuration / 2.0f > 1e-5f) {
                        t0_offset = math::easing::sineEaseInOut(
                            0.f, 0.f,
                            static_cast<float>(detail.initialDirection) * detail.distance,
                            detail.cycleDuration / 2.0f
                        );
                    }
                    sf::Vector2f calculatedInitialPos = movementAnchor;
                    if(detail.axis == 'x') calculatedInitialPos.x += t0_offset;
                    else if(detail.axis == 'y') calculatedInitialPos.y += t0_offset;

                    if (std::abs(new_body_ref.getPosition().x - calculatedInitialPos.x) > 0.1f ||
                        std::abs(new_body_ref.getPosition().y - calculatedInitialPos.y) > 0.1f) {
                         new_body_ref.setPosition(calculatedInitialPos);
                    }

                    activeMovingPlatforms.push_back({
                        detail.id, movementAnchor, detail.axis, detail.distance,
                        0.0f,
                        detail.cycleDuration, detail.initialDirection,
                        new_body_ref.getPosition()
                    });
                    foundDetail = true;
                    break;
                }
            }
            if(!foundDetail){
                std::cerr << "Warning: Moving platform ID " << p_body_template.getID()
                          << " (type 'moving' in JSON) missing movement details in LevelData. Will be static." << std::endl;
            }
        }
        else if (new_body_ref.getType() == phys::bodyType::interactible) {
            bool foundDetail = false;
            for (const auto& detail : data.interactiblePlatformDetails) {
                if (detail.id == new_body_ref.getID()) {
                    activeInteractibles[detail.id] = {
                        detail.id, detail.interactionType,
                        levelManager.stringToBodyType(detail.targetBodyTypeStr),
                        detail.targetTileColor, detail.hasTargetTileColor,
                        detail.oneTime, detail.cooldown,
                        false,
                        0.f,
                        detail.linkedID
                    };
                    foundDetail = true;
                    break;
                }
            }
            if (!foundDetail) {
                std::cerr << "Warning: Interactible platform ID " << p_body_template.getID()
                          << " (type 'interactible' in JSON) missing interaction details in LevelData. Will be static or unresponsive." << std::endl;
            }
        }
    }

tiles.reserve(bodies.size());
for (const auto& body : bodies) {
    // get body texture first
    std::string bodyTexturePath = body.getTexturePath();
    std::cout << bodyTexturePath << std::endl;
    // find texture in loaded textures


    // then pass into newtile declaration
    Tile newTile(sf::Vector2f(body.getWidth(), body.getHeight()));
    newTile.setPosition(body.getPosition());
    //newTile.setFillColor(getTileColorForBodyType(body.getType()));



    //find texture in loaded textures
    if (!bodyTexturePath.empty()){
        auto textureLiIt = currentLevelData.TexturesList.find(bodyTexturePath);
        if (textureLiIt != currentLevelData.TexturesList.end()){
            // texture is loaded in list
            newTile.setTexture(&(textureLiIt->second));
            std::cout << "Loaded object of texture: " << textureLiIt->first << std::endl;

            // adjust object dimensions
            auto textureDiIt = currentLevelData.TexturesDimensions.find(body.getID());
            if (textureDiIt != currentLevelData.TexturesDimensions.end()){
                // object has custom dimensions
                newTile.setTextureRect(textureDiIt->second);
                std::cout << "Adjusted obj " << body.getID() << " dimensions to: ["
                << textureDiIt->second.position.x << "," << textureDiIt->second.position.y << "] x ["
                << (textureDiIt->second.size.x + textureDiIt->second.position.x) << ","
                << (textureDiIt->second.size.y + textureDiIt->second.position.y) << "]" << std::endl;
            }
        }
        else {
            // texture not found; load default texture instead
            sf::Texture d(DEFAULT_TEXTURE_FILEPATH);
            newTile.setTexture(&d);
            std::cout << "Failed to load texture " << textureLiIt -> first << std::endl;
        }
    }

    // Determine if tile is special block
    if (body.getType() == phys::bodyType::goal) newTile.setSpecialTile(Tile::SpecialTile::GOAL);

    tiles.push_back(newTile);
}

    vanishingPlatformCycleTimer = sf::Time::Zero;
    oddEvenVanishing = 1;
}

void updateResolutionDisplayText() {
if (isFullscreen) {
resolutionCurrentText.setString("Fullscreen");
} else {
if (!availableVideoModes.empty() && currentResolutionIndex >= 0 && currentResolutionIndex < static_cast<int>(availableVideoModes.size())) {
const auto& mode = availableVideoModes[currentResolutionIndex];
resolutionCurrentText.setString(std::to_string(mode.size.x) + "x" + std::to_string(mode.size.y));
} else {
resolutionCurrentText.setString(
std::to_string(static_cast<int>(LOGICAL_SIZE.x)) + "x" +
std::to_string(static_cast<int>(LOGICAL_SIZE.y)) + " (Default)"
);
}
}
sf::FloatRect bounds = resolutionCurrentText.getLocalBounds();
resolutionCurrentText.setOrigin({bounds.position.x + bounds.size.x / 2.f, bounds.position.y + bounds.size.y / 2.f});
resolutionCurrentText.setPosition({LOGICAL_SIZE.x / 2.f, 320.f});
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
sf::Text menuTitleText(menuFont), startButtonText(menuFont), settingsButtonText(menuFont), creditsButtonText(menuFont), exitButtonText(menuFont);
sf::Texture menuBgTexture(IMG_MENU_BG); sf::Sprite menuBgSprite(menuBgTexture);
sf::Text settingsTitleText(menuFont), musicVolumeLabelText(menuFont), musicVolValText(menuFont), sfxVolumeLabelText(menuFont), sfxVolValText(menuFont), settingsBackText(menuFont);
sf::Text musicVolDownText(menuFont), musicVolUpText(menuFont), sfxVolDownText(menuFont), sfxVolUpText(menuFont);
sf::Text resolutionLabelText(menuFont), resolutionPrevText(menuFont), resolutionNextText(menuFont), fullscreenToggleText(menuFont);
sf::Text creditsTitleText(menuFont), creditsNamesText(menuFont), creditsBackText(menuFont);
sf::Text gameOverStatusText(menuFont), gameOverOption1Text(menuFont), gameOverOption2Text(menuFont);
sf::Text debugText(menuFont);
sf::RectangleShape playerShape;

std::optional<sf::Sprite> levelBgSprite;

bool menuBgSpriteLoaded = false;

bool goalReached = false;
bool doorAnimationOngoing = false;
std::vector<sf::Vector2f>doorAnimFramesTopLeft;
    doorAnimFramesTopLeft.push_back(sf::Vector2f(1275, 255));
    doorAnimFramesTopLeft.push_back(sf::Vector2f(250, 1275));
    doorAnimFramesTopLeft.push_back(sf::Vector2f(1275, 1275));
    doorAnimFramesTopLeft.push_back(sf::Vector2f(250, 2305));
    doorAnimFramesTopLeft.push_back(sf::Vector2f(1275,255));
int doorWidth = 455;
int doorHeight = 610;
int doorCurrentFrame = 0;
sf::Clock animClock;
float secondsPerFrame_door = 0.20f;
Tile* animatedDoorTile = nullptr;
sf::Time frameTime_door = sf::seconds(secondsPerFrame_door);

    // --- Game Constants ---
    const float PLAYER_MOVE_SPEED = 200.f;
    const float JUMP_INITIAL_VELOCITY = -450.f;
    const float GRAVITY_ACCELERATION = 1200.f;
    const float MAX_FALL_SPEED = 700.f;
    const sf::Time MAX_JUMP_HOLD_TIME = sf::seconds(0.18f);
    const float PLAYER_DEATH_Y_LIMIT = 2000.f;
    const float SPRING_BOUNCE_VELOCITY = 3.0f * JUMP_INITIAL_VELOCITY;
    const float creditsScrollSpeed = 40.f;

    // --- Initialization ---
    populateAvailableResolutions();
    const sf::Vector2f tileSize(32.f, 32.f);
    applyAndRecreateWindow(window, uiView, mainView);

    GameState currentState = GameState::MENU;
    levelManager.setMaxLevels(5);
    levelManager.setLevelBasePath("../assets/levels/");
    levelManager.setTransitionProperties(0.75f);
    levelManager.setGeneralLoadingScreenImage(IMG_LOAD_GENERAL);
    levelManager.setNextLevelLoadingScreenImage(IMG_LOAD_NEXT);
    levelManager.setRespawnLoadingScreenImage(IMG_LOAD_RESPAWN);

    loadAudio();

    playerBody = phys::DynamicBody({0,0}, tileSize.x+16, tileSize.y*2);

if (!menuFont.openFromFile(FONT_PATH)) {
    std::cerr << "FATAL: Failed to load font: " << FONT_PATH << ". Trying fallback." << std::endl;
    #if defined(_WIN32)
    if (!menuFont.openFromFile("C:/Windows/Fonts/arialbd.ttf")) { std::cerr << "Windows fallback font failed.\n"; return -1; }
    #elif defined(__APPLE__)
    if (!menuFont.openFromFile("/System/Library/Fonts/Supplemental/Arial Bold.ttf")) { if(!menuFont.openFromFile("/Library/Fonts/Arial Bold.ttf")) {std::cerr << "macOS fallback font failed.\n"; return -1; }}
    #else // Linux
    if (!menuFont.openFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf")) {
        std::cerr << "Linux fallback font failed.\n"; return -1;
        }
        #endif
        if (menuFont.getInfo().family.empty()) {std::cerr << "All font loading attempts failed.\n"; return -1;}
        std::cout << "Loaded a fallback font: " << menuFont.getInfo().family << std::endl;
    }

auto setupTextUI = [&](sf::Text& text, const sf::String& str, float yPos, unsigned int charSize = 30, float xOffset = 0.f) {
    text.setFont(menuFont);
    text.setString(str);
    text.setCharacterSize(charSize);
    text.setFillColor(sf::Color::White);
    sf::FloatRect text_bounds = text.getLocalBounds();
    text.setOrigin({text_bounds.position.x + text_bounds.size.x / 2.f, text_bounds.position.y + text_bounds.size.y / 2.f});
    text.setPosition({LOGICAL_SIZE.x / 2.f + xOffset, yPos});
};

if (menuBgTexture.loadFromFile(IMG_MENU_BG)) {
    menuBgSprite.setTexture(menuBgTexture);
    menuBgSpriteLoaded = true;
    if (menuBgTexture.getSize().x > 0 && menuBgTexture.getSize().y > 0) {
        menuBgSprite.setScale({LOGICAL_SIZE.x / (menuBgTexture.getSize().x),
                              LOGICAL_SIZE.y / (menuBgTexture.getSize().y)});
    }
    menuBgSprite.setPosition({0.f,0.f});
    std::cout << "Loaded " << IMG_MENU_BG << std::endl;
}
else {
    std::cerr << "Warning: Menu BG image not found: " << IMG_MENU_BG << std::endl;
}

 // 1. MAIN MENU (Unchanged from previous version)
    setupTextUI(menuTitleText, "Celestial Speedrun", 120.f, 64);
    setupTextUI(startButtonText, "[ START ]", 280.f, 40);
    setupTextUI(settingsButtonText, "[ Settings ]", 350.f, 28);
    setupTextUI(creditsButtonText, "[ Credits ]", 390.f, 28);
    setupTextUI(exitButtonText, "[ Exit ]", 550.f, 24);


    // 2. SETTINGS SCREEN (Completely redesigned for robustness)
    setupTextUI(settingsTitleText, "S E T T I N G S", 80.f, 40);

    // -- Volume Controls Section --
    const float labelXOffset = -180.f;
    const float controlGroupXOffset = 120.f;
    const float arrowSpacing = 80.f;

    setupTextUI(musicVolumeLabelText, "Music Volume", 180.f, 24, labelXOffset);
    setupTextUI(musicVolDownText, "<", 180.f, 28, controlGroupXOffset - arrowSpacing);
    setupTextUI(musicVolValText, "", 180.f, 24, controlGroupXOffset);
    setupTextUI(musicVolUpText, ">", 180.f, 28, controlGroupXOffset + arrowSpacing);

    setupTextUI(sfxVolumeLabelText, "SFX Volume", 240.f, 24, labelXOffset);
    setupTextUI(sfxVolDownText, "<", 240.f, 28, controlGroupXOffset - arrowSpacing);
    setupTextUI(sfxVolValText, "", 240.f, 24, controlGroupXOffset);
    setupTextUI(sfxVolUpText, ">", 240.f, 28, controlGroupXOffset + arrowSpacing);

    // -- Display Controls Section (THE CORE FIX) --
    // We now build the layout around the fixed Y-position (320.f) from updateResolutionDisplayText().
    setupTextUI(resolutionLabelText, "--- Display ---", 290.f, 24); // A clean section divider

    // Vertically align the arrows with the resolution text at Y = 320.f
    resolutionCurrentText.setFont(menuFont);
    resolutionCurrentText.setCharacterSize(24);
    resolutionCurrentText.setFillColor(sf::Color::White);
    updateResolutionDisplayText(); // This hardcodes the text value's Y position to 320.f
    setupTextUI(resolutionPrevText, "<", 320.f, 28, -160.f); // Y=320, with wide horizontal offset
    setupTextUI(resolutionNextText, ">", 320.f, 28, 160.f); // Y=320, with wide horizontal offset

    setupTextUI(fullscreenToggleText, "[ Toggle Fullscreen ]", 370.f, 24); // Positioned neatly below the resolution line

    setupTextUI(settingsBackText, "[ Back to Menu ]", 540.f, 26); // At the very bottom


    // 3. CREDITS SCREEN (Unchanged from previous version)
    setupTextUI(creditsTitleText, "Celestial Speedrun", 80.f, 48);
    setupTextUI(creditsNamesText,
        "Congratulations on finishing the game!\n\n\n\n\n\n\n\n\n\n"

        "A tribute to the sleepless nights...\n\n\n"
        "PROGRAMMING & ENGINEERING\n"
        "Jecer Egagamao & Gian De la Cruz\n"
        "   - for creating the spaghetti code that somehow runs\n\n"
        "LEVEL & GAME DESIGN\n"
        "Zeann Tahimic\n"
        "   - for designing the masterfully rage-inducing difficulty\n\n"
        "ART & VISUALS\n"
        "Jan Abuyo\n"
        "   - for the wonderful sprites and visual design\n\n\n"
        "SPECIAL THANKS\n"
        "Sound effects from online marketplaces\n"
        "Inspiration from Mark Richards & various gamedev tutorials\n"
        "Made with the powerful SFML Multimedia Library",
        325.f, 22);
    setupTextUI(creditsBackText, "--- Back to Menu ---", 560.f, 24);


    // 4. GAME OVER SCREEN (Original layout is fine, remains unchanged)
    setupTextUI(gameOverStatusText, "", 150.f, 36, -180);
    setupTextUI(gameOverOption1Text, "", 280.f, 30, -82.5);
    setupTextUI(gameOverOption2Text, "Main Menu", 330.f);

    sf::Color defaultBtnColor = sf::Color::White;
    sf::Color hoverBtnColor = sf::Color::Yellow;
    sf::Color exitBtnHoverColor = sf::Color::Red;

    //playerShape.setFillColor(sf::Color(220, 220, 250, 255));
    // PLAYER TEXTURE LOADING (to be moved/replaced later)
    /*if (!playerTexture.loadFromFile("../assets/sprites/Mc1_left_side.png")){
        std::cerr << "Cannot load player texture." << std::endl;
        playerTexture.loadFromFile(DEFAULT_TEXTURE_FILEPATH);
    }*/
    playerShape.setTexture(&playerTexture);
    sf::IntRect playerDimensions = sprites::SpriteManager::GetPlayerTextureUponMovement(sprites::SpriteManager::NONE);
    playerShape.setTextureRect(playerDimensions);

    playerShape.setSize(sf::Vector2f(playerBody.getWidth(), playerBody.getHeight()));

debugText.setFont(menuFont);
debugText.setCharacterSize(14);
debugText.setFillColor(sf::Color::White);
debugText.setPosition({10.f, 10.f});

menuMusic.setVolume(gameSettings.musicVolume);
gameMusic.setVolume(gameSettings.musicVolume);
if (menuMusic.getStatus() != sf::Music::Status::Playing && menuMusic.openFromFile(AUDIO_MUSIC_MENU)) {
    menuMusic.play();
}

    // --- MAIN GAME LOOP ---
    while (running) {
        interactKeyPressedThisFrame = false;
        sf::Time frameDeltaTime = gameClock.restart();

    //sf::Event event;
    while (const std::optional event = window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            running = false; window.close();
        }
        if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()){
            if (keyPressed->scancode == sf::Keyboard::Scancode::P) {
                if(currentState == GameState::PLAYING && levelManager.requestLoadNextLevel(currentLevelData)){
                        currentState = GameState::TRANSITIONING; playSfx("goal");
                } else if (currentState == GameState::PLAYING && !levelManager.hasNextLevel()) {
                    currentState = GameState::CREDITS; // Go to credits instead of win screen
                    creditsNamesText.setPosition({LOGICAL_SIZE.x / 2.f, LOGICAL_SIZE.y + creditsNamesText.getLocalBounds().size.y / 2.f}); // Reset the scroll
                    if(gameMusic.getStatus() == sf::Music::Status::Playing) gameMusic.stop();
                    if(menuMusic.getStatus() != sf::Music::Status::Playing && menuMusic.openFromFile(AUDIO_MUSIC_MENU)) menuMusic.play();
                }
            }
        }

            sf::Vector2i pixelPos = sf::Mouse::getPosition(window);
            sf::Vector2f worldPosUi = window.mapPixelToCoords(pixelPos, uiView);

        switch(currentState) {
            case GameState::MENU:
                if (const auto* mouseButtonReleased = event->getIf<sf::Event::MouseButtonReleased>()){
                    if (mouseButtonReleased->button == sf::Mouse::Button::Left) {
                        playSfx("click");
                        if (startButtonText.getGlobalBounds().contains(worldPosUi)) {
                            levelManager.setCurrentLevelNumber(0);
                            if (levelManager.requestLoadNextLevel(currentLevelData)) {
                                currentState = GameState::TRANSITIONING;
                                if(menuMusic.getStatus() == sf::Music::Status::Playing) menuMusic.stop();
                                if(gameMusic.getStatus() != sf::Music::Status::Playing && gameMusic.openFromFile(AUDIO_MUSIC_GAME)) gameMusic.play();
                            } else { std::cerr << "MENU: Failed request to load initial level." << std::endl; }
                        } else if (settingsButtonText.getGlobalBounds().contains(worldPosUi)) {
                            currentState = GameState::SETTINGS;
                            updateResolutionDisplayText();
                        } else if (creditsButtonText.getGlobalBounds().contains(worldPosUi)) {
                            currentState = GameState::CREDITS;
                            creditsNamesText.setPosition({LOGICAL_SIZE.x / 2.f, LOGICAL_SIZE.y + creditsNamesText.getLocalBounds().size.y / 2.f});
                        } else if (exitButtonText.getGlobalBounds().contains(worldPosUi)) {
                            running = false; window.close();
                        }
                    }
                }
                if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()){
                    if (keyPressed->scancode == sf::Keyboard::Scancode::Escape){
                        running = false; 
                        window.close(); 
                    }
                }
                break;
            case GameState::SETTINGS:
                if (const auto* mouseButtonReleased = event->getIf<sf::Event::MouseButtonReleased>()){
                    if (mouseButtonReleased->button == sf::Mouse::Button::Left) {
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
                            if (!isFullscreen && !availableVideoModes.empty()) {
                                currentResolutionIndex--; if (currentResolutionIndex < 0) currentResolutionIndex = static_cast<int>(availableVideoModes.size()) - 1;
                                applyAndRecreateWindow(window, uiView, mainView); updateResolutionDisplayText();
                            }
                        } else if (resolutionNextText.getGlobalBounds().contains(worldPosUi)) {
                                if (!isFullscreen && !availableVideoModes.empty()) {
                                currentResolutionIndex++; if (currentResolutionIndex >= static_cast<int>(availableVideoModes.size())) currentResolutionIndex = 0;
                                applyAndRecreateWindow(window, uiView, mainView); updateResolutionDisplayText();
                            }
                        } else if (fullscreenToggleText.getGlobalBounds().contains(worldPosUi)) {
                            isFullscreen = !isFullscreen;
                            applyAndRecreateWindow(window, uiView, mainView); updateResolutionDisplayText();
                        }
                    }
                }
                if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()){
                    if (keyPressed->scancode == sf::Keyboard::Scancode::Escape){
                        currentState = GameState::MENU;
                    }
                }
                break;
            case GameState::CREDITS:
                if (const auto* mouseButtonReleased = event->getIf<sf::Event::MouseButtonReleased>()){
                    if (mouseButtonReleased->button == sf::Mouse::Button::Left){
                        playSfx("click");
                        if (creditsBackText.getGlobalBounds().contains(worldPosUi)) currentState = GameState::MENU;
                    }
                }
                if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()){
                    if (keyPressed->scancode == sf::Keyboard::Scancode::Escape) currentState = GameState::MENU;
                }
                break;
            case GameState::PLAYING:
                if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                    if (keyPressed->scancode == sf::Keyboard::Scancode::Escape) {
                        currentState = GameState::MENU;
                        if(gameMusic.getStatus() == sf::Music::Status::Playing) gameMusic.pause();
                        if(menuMusic.getStatus() != sf::Music::Status::Playing && menuMusic.openFromFile(AUDIO_MUSIC_MENU)) menuMusic.play();
                    } else if (keyPressed->scancode== sf::Keyboard::Scancode::R) {
                        playSfx("click");
                        if (levelManager.requestRespawnCurrentLevel(currentLevelData)) {
                            currentState = GameState::TRANSITIONING;
                        } else {std::cerr << "PLAYING: Failed respawn request.\n";}
                    } else if (keyPressed->scancode == sf::Keyboard::Scancode::E) {
                        interactKeyPressedThisFrame = true;
                    }
                }
                break;
             case GameState::GAME_OVER_WIN:
             case GameState::GAME_OVER_LOSE_FALL:
             case GameState::GAME_OVER_LOSE_DEATH:
                if (const auto* mouseButtonReleased = event->getIf<sf::Event::MouseButtonReleased>()){
                    if (mouseButtonReleased->button == sf::Mouse::Button::Left) {
                        playSfx("click");
                        if (gameOverOption1Text.getGlobalBounds().contains(worldPosUi)) {
                            if (currentState == GameState::GAME_OVER_LOSE_FALL || currentState == GameState::GAME_OVER_LOSE_DEATH) { // Retry
                                if (levelManager.requestRespawnCurrentLevel(currentLevelData)) {
                                    currentState = GameState::TRANSITIONING;
                                    if(menuMusic.getStatus() == sf::Music::Status::Playing) menuMusic.stop();
                                    if(gameMusic.getStatus() != sf::Music::Status::Playing && gameMusic.openFromFile(AUDIO_MUSIC_GAME)) gameMusic.play();
                                } else { currentState = GameState::MENU; if(gameMusic.getStatus() == sf::Music::Status::Playing) gameMusic.stop(); if(menuMusic.getStatus() != sf::Music::Status::Playing && menuMusic.openFromFile(AUDIO_MUSIC_MENU)) menuMusic.play(); levelManager.setCurrentLevelNumber(0); }
                            } else if (currentState == GameState::GAME_OVER_WIN) { // Play Again (Level 1)
                                levelManager.setCurrentLevelNumber(0);
                                if (levelManager.requestLoadNextLevel(currentLevelData)) {
                                    currentState = GameState::TRANSITIONING;
                                    if(menuMusic.getStatus() == sf::Music::Status::Playing) menuMusic.stop();
                                    if(gameMusic.getStatus() != sf::Music::Status::Playing && gameMusic.openFromFile(AUDIO_MUSIC_GAME)) gameMusic.play();
                                } else { currentState = GameState::MENU; if(menuMusic.getStatus() != sf::Music::Status::Playing && menuMusic.openFromFile(AUDIO_MUSIC_MENU)) menuMusic.play(); }
                            }
                        } else if (gameOverOption2Text.getGlobalBounds().contains(worldPosUi)) { // Main Menu
                            currentState = GameState::MENU;
                            if(gameMusic.getStatus() == sf::Music::Status::Playing) gameMusic.stop();
                            if(menuMusic.getStatus() != sf::Music::Status::Playing && menuMusic.openFromFile(AUDIO_MUSIC_MENU)) menuMusic.play();
                            levelManager.setCurrentLevelNumber(0);
                        }
                    }
                }
                if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()){
                    if (keyPressed->scancode == sf::Keyboard::Scancode::Escape) { // Go to menu on Esc
                        currentState = GameState::MENU;
                        if(gameMusic.getStatus() == sf::Music::Status::Playing) gameMusic.stop();
                        if(menuMusic.getStatus() != sf::Music::Status::Playing && menuMusic.openFromFile(AUDIO_MUSIC_MENU)) menuMusic.play();
                        levelManager.setCurrentLevelNumber(0);
                    }
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

        // --- Game Logic Update ---
        if (currentState == GameState::PLAYING && !goalReached) {
            playerShape.setSize(sf::Vector2f(playerBody.getWidth(), playerBody.getHeight()));

            while (timeSinceLastFixedUpdate >= TIME_PER_FIXED_UPDATE) {
                timeSinceLastFixedUpdate -= TIME_PER_FIXED_UPDATE;
                const float fixed_dt_seconds = TIME_PER_FIXED_UPDATE.asSeconds();

                playerBody.setLastPosition(playerBody.getPosition());

            // --- Handle Input for Player ---
            float horizontalInput = 0.f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::RShift)) turboMultiplier = 2;
            else turboMultiplier = 1;

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Left)) {
                horizontalInput = -1.f;
                playerShape.setTextureRect(sprites::SpriteManager::GetPlayerTextureUponMovement(sprites::SpriteManager::LEFT));
            }
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Right)) {
                horizontalInput = 1.f;
                playerShape.setTextureRect(sprites::SpriteManager::GetPlayerTextureUponMovement(sprites::SpriteManager::RIGHT));
            }
            else playerShape.setTextureRect(sprites::SpriteManager::GetPlayerTextureUponMovement(sprites::SpriteManager::NONE));

            bool jumpIntentThisFrame = (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Up) || sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Space));
            bool dropIntentThisFrame = (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Down));
            bool newJumpPressThisFrame = (jumpIntentThisFrame && playerBody.isOnGround() && currentJumpHoldDuration == sf::Time::Zero);

                if (newJumpPressThisFrame && !playerBody.getGroundPlatformTemporarilyIgnored()) {
                    const phys::PlatformBody* groundPlat = playerBody.getGroundPlatform();
                    bool safeToAccessGroundPlat = false;
                    if (groundPlat) {
                        for (const auto& body_ref : bodies) { if (&body_ref == groundPlat) { safeToAccessGroundPlat = true; break; } }
                    }
                    if (!safeToAccessGroundPlat || groundPlat->getType() != phys::bodyType::spring) {
                         playSfx("jump");
                    }
                }
                playerBody.setTryingToDrop(dropIntentThisFrame && playerBody.isOnGround());

                // --- Update Moving Platforms ---
                for(auto& activePlat : activeMovingPlatforms) {
                    phys::PlatformBody* movingBodyPtr = nullptr; size_t tileIdx = (size_t)-1;
                    for(size_t i_plat=0; i_plat < bodies.size(); ++i_plat) {
                        if(bodies[i_plat].getID() == activePlat.id && bodies[i_plat].getType() == phys::bodyType::moving) {
                            movingBodyPtr = &bodies[i_plat];
                            tileIdx = i_plat;
                            break;
                        }
                    }

                    if (movingBodyPtr) {
                        activePlat.lastFrameActualPosition = movingBodyPtr->getPosition();
                        activePlat.cycleTime += fixed_dt_seconds;
                        float effectiveCycleDur = activePlat.cycleDuration > 1e-5f ? activePlat.cycleDuration : 1.f;
                        activePlat.cycleTime = std::fmod(activePlat.cycleTime, effectiveCycleDur);

                        float singleMovePhaseDur = effectiveCycleDur / 2.0f;
                        float offset = 0.f;
                        if (singleMovePhaseDur > 1e-5f) {
                            float currentPhaseTime = activePlat.cycleTime;
                            if (currentPhaseTime < singleMovePhaseDur) {
                                offset = math::easing::sineEaseInOut(currentPhaseTime, 0.f, activePlat.initialDirection * activePlat.distance, singleMovePhaseDur);
                            } else {
                                currentPhaseTime -= singleMovePhaseDur;
                                offset = math::easing::sineEaseInOut(currentPhaseTime, activePlat.initialDirection * activePlat.distance, -(activePlat.initialDirection * activePlat.distance), singleMovePhaseDur);
                            }
                        }
                        sf::Vector2f newPos = activePlat.movementAnchorPosition;
                        if(activePlat.axis == 'x') newPos.x += offset;
                        else if(activePlat.axis == 'y') newPos.y += offset;

                        movingBodyPtr->setPosition(newPos);
                        if (tileIdx < tiles.size()) {
                            tiles[tileIdx].setPosition(newPos);
                        }
                    }
                }

                // --- Update Interactible Cooldowns ---
                for (auto& pair : activeInteractibles) {
                    ActiveInteractiblePlatform& interactible = pair.second;
                    if (interactible.currentCooldownTimer > 0.f) {
                        interactible.currentCooldownTimer -= fixed_dt_seconds;
                        if (interactible.currentCooldownTimer < 0.f) interactible.currentCooldownTimer = 0.f;
                    }
                }

                // --- Update Platform States (Falling, Vanishing) ---
                for (size_t i_body = 0; i_body < bodies.size(); ++i_body) {
                    if (tiles.size() <= i_body) continue;

                    phys::PlatformBody& current_body = bodies[i_body];
                    Tile& current_tile = tiles[i_body];

                    const phys::PlatformBody* template_body_ptr = nullptr;
                    sf::Vector2f originalPos = {-9999.f, -9999.f};

                    for(const auto& templ : currentLevelData.platforms) {
                        if(templ.getID() == current_body.getID()){
                            template_body_ptr = &templ;
                            originalPos = templ.getPosition();
                            break;
                        }
                    }

                    if (!template_body_ptr) {
                        continue;
                    }

                    if (template_body_ptr->getType() == phys::bodyType::falling) {
                        if (!current_body.isFalling()) {
                              bool playerOnThis = playerBody.isOnGround() && playerBody.getGroundPlatform() == &current_body;
                              if (playerOnThis && !current_tile.isFalling() && !current_tile.hasFallen()) {
                                  current_tile.startFalling(sf::seconds(0.5f));
                              }
                        }

                        current_tile.update(TIME_PER_FIXED_UPDATE);

                        if (current_tile.isFalling() && !current_body.isFalling()) {
                            current_body.setFalling(true);
                        }
                        if (current_tile.isFalling() && current_body.isFalling()) {
                            current_body.setPosition(current_tile.getPosition());
                        }

                        if (current_tile.hasFallen() && current_body.getType() != phys::bodyType::none) {
                            if (playerBody.getGroundPlatform() == &current_body) {
                                playerBody.setOnGround(false);
                                playerBody.setGroundPlatform(nullptr);
                            }
                            current_body.setPosition({-9999.f, -9999.f});
                            current_body.setType(phys::bodyType::none);
                            current_tile.setFillColor(sf::Color::Transparent);
                        }
                    }
                    else if (template_body_ptr->getType() == phys::bodyType::vanishing) {
                        bool is_even_id = (current_body.getID() % 2 == 0);
                        bool should_be_fading_out_now = (oddEvenVanishing == 1 && is_even_id) || (oddEvenVanishing == -1 && !is_even_id);

                        float phaseTime = std::fmod(vanishingPlatformCycleTimer.asSeconds(), 1.0f);
                        sf::Color baseVanishingColor = getTileColorForBodyType(phys::bodyType::vanishing);
                        float alpha_val;

                    if (should_be_fading_out_now) { // Fading out (target: 0 alpha / non-interactive)
                        alpha_val = math::easing::sineEaseInOut(phaseTime, 255.f, -255.f, 1.f);
                    } else { // Fading in (target: 255 alpha / interactive)
                        alpha_val = math::easing::sineEaseInOut(phaseTime, 0.f, 255.f, 1.f);
                    }
                    alpha_val = std::max(0.f, std::min(255.f, alpha_val)); // Clamp alpha
                    uint8_t finalAlphaByte = static_cast<uint8_t>(alpha_val);

                        if (alpha_val <= 10.f) {
                            if (current_body.getType() != phys::bodyType::none) {
                                if (playerBody.getGroundPlatform() == &current_body) {
                                    playerBody.setOnGround(false);
                                    playerBody.setGroundPlatform(nullptr);
                                }
                                current_body.setType(phys::bodyType::none);
                            }
                            if (current_body.getPosition() != sf::Vector2f(-9999.f, -9999.f)) current_body.setPosition({-9999.f, -9999.f});
                            if (current_tile.getPosition() != sf::Vector2f(-9999.f, -9999.f)) current_tile.setPosition({-9999.f, -9999.f});
                            finalAlphaByte = 0;
                        } else {
                            if (current_body.getType() == phys::bodyType::none) {
                                current_body.setType(phys::bodyType::vanishing);
                            }
                            if (originalPos.x > -9998.f) {
                                if (current_body.getPosition() != originalPos) current_body.setPosition(originalPos);
                                if (current_tile.getPosition() != originalPos) current_tile.setPosition(originalPos);
                            } else {
                                if (current_body.getType() != phys::bodyType::none) current_body.setType(phys::bodyType::none);
                                if(current_body.getPosition() != sf::Vector2f(-9999.f, -9999.f)) current_body.setPosition({-9999.f, -9999.f});
                                if(current_tile.getPosition() != sf::Vector2f(-9999.f, -9999.f)) current_tile.setPosition({-9999.f, -9999.f});
                                finalAlphaByte = 0;
                            }
                        }
                        // REMOVED DEPENDENCE ON BLOCK TYPE COLOR - WILL NOW RENDER AS SPRITE
                        current_tile.setFillColor(sf::Color(255, 255, 255, finalAlphaByte));
                    }
                }

                vanishingPlatformCycleTimer += TIME_PER_FIXED_UPDATE;
                if (vanishingPlatformCycleTimer.asSeconds() >= 1.0f) {
                    vanishingPlatformCycleTimer -= sf::seconds(1.0f);
                    oddEvenVanishing *= -1;
                }

                // --- Player Velocity Update ---
                sf::Vector2f pVel = playerBody.getVelocity();
                pVel.x = horizontalInput * PLAYER_MOVE_SPEED * static_cast<float>(turboMultiplier);

                if (!playerBody.isOnGround()) {
                    pVel.y += GRAVITY_ACCELERATION * fixed_dt_seconds;
                    pVel.y = std::min(pVel.y, MAX_FALL_SPEED);
                }

                if (newJumpPressThisFrame) {
                    pVel.y = JUMP_INITIAL_VELOCITY;
                    currentJumpHoldDuration = sf::microseconds(1);
                } else if (jumpIntentThisFrame && currentJumpHoldDuration > sf::Time::Zero && currentJumpHoldDuration < MAX_JUMP_HOLD_TIME) {
                    const phys::PlatformBody* groundPlatForJumpExtend = playerBody.getGroundPlatform();
                    bool safeToAccessGroundPlatForJumpExtend = false;
                    if(groundPlatForJumpExtend){
                        for(const auto& body_ref : bodies){ if(&body_ref == groundPlatForJumpExtend){ safeToAccessGroundPlatForJumpExtend = true; break; } }
                    }
                    if (playerBody.getVelocity().y < 0.f && (!safeToAccessGroundPlatForJumpExtend || groundPlatForJumpExtend->getType() != phys::bodyType::spring) ) {
                         pVel.y = JUMP_INITIAL_VELOCITY;
                    }
                    currentJumpHoldDuration += TIME_PER_FIXED_UPDATE;
                } else {
                    currentJumpHoldDuration = sf::Time::Zero;
                }
                playerBody.setVelocity(pVel);

                // --- Collision Resolution ---
                phys::CollisionResolutionInfo resolutionResult = phys::CollisionSystem::resolveCollisions(playerBody, bodies, fixed_dt_seconds);
                pVel = playerBody.getVelocity();

                // --- Post-Collision Player Logic ---
                if (playerBody.isOnGround()) {
                    currentJumpHoldDuration = sf::Time::Zero;
                    const phys::PlatformBody* currentGroundPlatform = playerBody.getGroundPlatform();

                    if (currentGroundPlatform) {
                        bool safeToAccessCurrentGroundPlatform = false;
                        for (const auto& body_ref : bodies) { if (&body_ref == currentGroundPlatform) { safeToAccessCurrentGroundPlatform = true; break; } }

                        if (safeToAccessCurrentGroundPlatform) {
                            const phys::PlatformBody& pf = *currentGroundPlatform;
                            if (pf.getType() == phys::bodyType::conveyorBelt) {
                                playerBody.setPosition(playerBody.getPosition() + pf.getSurfaceVelocity() * fixed_dt_seconds);
                            } else if (pf.getType() == phys::bodyType::moving) {
                                 for(const auto& activePlat : activeMovingPlatforms) {
                                    if (activePlat.id == pf.getID()) {
                                        phys::PlatformBody* movingPhysBody = nullptr;
                                        for(auto& b_ref : bodies) if(b_ref.getID() == activePlat.id && b_ref.getType() == phys::bodyType::moving) {movingPhysBody = &b_ref; break;}

                                        if(movingPhysBody){
                                            sf::Vector2f platformFrameDisplacement = movingPhysBody->getPosition() - activePlat.lastFrameActualPosition;
                                            playerBody.setPosition(playerBody.getPosition() + platformFrameDisplacement);
                                        }
                                        break;
                                    }
                                }
                            } else if (pf.getType() == phys::bodyType::spring) {
                                pVel.y = SPRING_BOUNCE_VELOCITY;
                                playerBody.setOnGround(false);
                                playerBody.setGroundPlatform(nullptr);
                                playSfx("spring");
                            }
                        } else {
                             playerBody.setOnGround(false);
                             playerBody.setGroundPlatform(nullptr);
                        }
                    }
                }

                if (resolutionResult.hitCeiling && pVel.y < 0.f) {
                    pVel.y = 0.f;
                    currentJumpHoldDuration = MAX_JUMP_HOLD_TIME;
                }
                playerBody.setVelocity(pVel);

            // --- Trap Check ---
            bool trapHit = false;
            for (const auto& body_check_trap : bodies) {
                if (body_check_trap.getType() == phys::bodyType::trap && body_check_trap.getAABB().findIntersection(playerBody.getAABB())) {
                    trapHit = true;
                    break;
                }
            }
            if (trapHit) {
                playSfx("death");
                currentState = GameState::GAME_OVER_LOSE_DEATH;
                if(gameMusic.getStatus() == sf::Music::Status::Playing) gameMusic.pause();
                if(menuMusic.getStatus() != sf::Music::Status::Playing && menuMusic.openFromFile(AUDIO_MUSIC_MENU)) menuMusic.play();
                break;
            }

            //Interaction (Goal, Interactibles, Portals)
            if (interactKeyPressedThisFrame) { 
                bool interaction_occurred_this_frame = false; 

                // --- Portal Interaction ---
                for (const auto& entered_portal_body : bodies) {
                    if (entered_portal_body.getType() == phys::bodyType::portal &&
                        playerBody.getAABB().findIntersection(entered_portal_body.getAABB())) {
                        
                        playSfx("portal"); 
                        
                        unsigned int targetPortalPlatformID = entered_portal_body.getPortalID(); 
                        sf::Vector2f teleportOffset = entered_portal_body.getTeleportOffset();   

                        if (targetPortalPlatformID == 0) { 
                            std::cerr << "Player entered unlinked portal (ID: " << entered_portal_body.getID() 
              << ", targetPortalPlatformID is 0)." << std::endl;
                            interaction_occurred_this_frame = true;
                            break; 
                        }

                        phys::PlatformBody* destination_portal_ptr = nullptr;
                        for (auto& potential_target_body : bodies) {
                            if (potential_target_body.getID() == targetPortalPlatformID) {
                                if (potential_target_body.getType() == phys::bodyType::portal) {
                                    destination_portal_ptr = &potential_target_body;
                                } else {
                                    std::cerr << "Error: Portal ID " << entered_portal_body.getID()
                                              << " links to ID " << targetPortalPlatformID
                                              << ", but the target entity is not a portal (actual type: "
                                              << static_cast<int>(potential_target_body.getType()) << ").\n";
                                }
                                break; 
                            }
                        }

                        if (destination_portal_ptr) {
                            sf::Vector2f final_pos = destination_portal_ptr->getPosition() + teleportOffset;
                            
                            playerBody.setPosition(final_pos); 
                            playerBody.setVelocity({0.f, 0.f});
                            
                            std::cout << "Player teleported from portal " << entered_portal_body.getID() 
                                      << " to portal " << destination_portal_ptr->getID() 
                                      << " at (" << final_pos.x << ", " << final_pos.y << ")\n";

                        } else {
                            // Destination portal was not found (or targetPortalPlatformID was invalid)
                            std::cerr << "Error: Portal ID " << entered_portal_body.getID()
                                      << " attempted to link to non-existent/invalid portal ID: " 
                                      << targetPortalPlatformID << std::endl;
                        }
                        interaction_occurred_this_frame = true; 
                        break;
                    }
                }
                if(interaction_occurred_this_frame) {
                    goto end_fixed_update_for_interaction;
                }


                // --- Goal Interaction ---
 for (const auto& platform_body_check_goal : bodies) {
                    if (platform_body_check_goal.getType() == phys::bodyType::goal && playerBody.getAABB().findIntersection(platform_body_check_goal.getAABB())) {
                        for (auto tile : tiles){
                            if (tile.getSpecialTile() == Tile::SpecialTile::GOAL){
                                // run door animation
                                std::cout<<"DOOR ANIMATION!"<<std::endl;
                                animatedDoorTile = &tile;
                                doorAnimationOngoing = true;
                                goalReached = true;

                                for (Tile& t : tiles){
                                    if (t.getSpecialTile() == Tile::SpecialTile::GOAL){
                                        animatedDoorTile = &t;
                                        break;
                                    }
                                }

                                animClock.restart();
                                break;
                            }
                        }
                        playSfx("goal");
                        interaction_occurred_this_frame = true;
                        goto end_fixed_update_for_interaction;
                    }
                }
                if(interaction_occurred_this_frame) { 
                    goto end_fixed_update_for_interaction;
                }


                // --- Interactible Platform Interaction ---
                for (size_t k = 0; k < bodies.size(); ++k) {
                    phys::PlatformBody& interact_body_ref = bodies[k];
                    if (interact_body_ref.getType() == phys::bodyType::interactible && playerBody.getAABB().findIntersection(interact_body_ref.getAABB())) {
                        auto it = activeInteractibles.find(interact_body_ref.getID());
                        if (it != activeInteractibles.end()) {
                            ActiveInteractiblePlatform& interactState = it->second;
                            if (interactState.currentCooldownTimer > 0.f || (interactState.oneTime && interactState.hasBeenInteractedThisSession)) {
                                continue; 
                            }

                                if (interactState.interactionType == "changeSelf") {
                                    playSfx("click");
                                    interact_body_ref.setType(interactState.targetBodyTypeEnum);

                                    if (tiles.size() > k) {
                                        if (interactState.hasTargetTileColor) {
                                            tiles[k].setFillColor(interactState.targetTileColor);
                                        } else {
                                            tiles[k].setFillColor(getTileColorForBodyType(interactState.targetBodyTypeEnum));
                                        }
                                    }

                                    if (interactState.targetBodyTypeEnum == phys::bodyType::none) {
                                        if (playerBody.getGroundPlatform() == &interact_body_ref) {
                                            playerBody.setOnGround(false);
                                            playerBody.setGroundPlatform(nullptr);
                                        }
                                        interact_body_ref.setPosition({-10000.f, -10000.f}); 
                                        if (tiles.size() > k) tiles[k].setFillColor(sf::Color::Transparent);
                                    }

                                    if (interactState.linkedID != 0) {
                                        for (size_t linked_idx = 0; linked_idx < bodies.size(); ++linked_idx) {
                                            if (bodies[linked_idx].getID() == interactState.linkedID) {
                                                phys::PlatformBody& linked_body_ref = bodies[linked_idx];
                                                
                                                Tile* linked_tile_ref_ptr = nullptr;
                                                if (linked_idx < tiles.size()) {
                                                    linked_tile_ref_ptr = &tiles[linked_idx];
                                                }


                                                if (linked_body_ref.getType() == phys::bodyType::solid || linked_body_ref.getType() == phys::bodyType::platform ) {
                                                    if (playerBody.getGroundPlatform() == &linked_body_ref) {
                                                        playerBody.setOnGround(false);
                                                        playerBody.setGroundPlatform(nullptr);
                                                    }
                                                    linked_body_ref.setType(phys::bodyType::none); 
                                                    linked_body_ref.setPosition({-10000.f, -10000.f});
                                                    if(linked_tile_ref_ptr) {
                                                        linked_tile_ref_ptr->setFillColor(sf::Color::Transparent);
                                                        linked_tile_ref_ptr->setPosition({-10000.f, -10000.f});
                                                    }

                                                } else if (linked_body_ref.getType() == phys::bodyType::none) {
                                                    sf::Vector2f originalLinkedPos = {-9999.f, -9999.f};
                                                    phys::bodyType originalLinkedType = phys::bodyType::solid; 
                                                    
                                                    for(const auto& templ : currentLevelData.platforms){
                                                        if(templ.getID() == linked_body_ref.getID()){
                                                            originalLinkedPos = templ.getPosition();
                                                            originalLinkedType = templ.getType(); 
                                                            break;
                                                        }
                                                    }

                                                    if(originalLinkedPos.x > -9998.f){ 
                                                       linked_body_ref.setPosition(originalLinkedPos);
                                                       linked_body_ref.setType(originalLinkedType);
                                                       if(linked_tile_ref_ptr){
                                                           linked_tile_ref_ptr->setPosition(originalLinkedPos);
                                                           linked_tile_ref_ptr->setFillColor(getTileColorForBodyType(originalLinkedType));
                                                       }
                                                    }
                                                } 
                                                break; // Found and processed linked body
                                            }
                                        }
                                    }

                                    if (interactState.oneTime) interactState.hasBeenInteractedThisSession = true;
                                    else interactState.currentCooldownTimer = interactState.cooldown;
                                    
                                    interaction_occurred_this_frame = true; 
                                    goto end_fixed_update_for_interaction;
                                }
                        }
                    }
                }
                end_fixed_update_for_interaction:;
            }

            // --- Death by Falling ---
            if (playerBody.getPosition().y > PLAYER_DEATH_Y_LIMIT) {
                playSfx("death");
                currentState = GameState::GAME_OVER_LOSE_FALL;
                if(gameMusic.getStatus() == sf::Music::Status::Playing) gameMusic.pause();
                if(menuMusic.getStatus() != sf::Music::Status::Playing && menuMusic.openFromFile(AUDIO_MUSIC_MENU)) menuMusic.play();
                break;
            }

        } // End of fixed update (while) loop
    }
    else if (currentState == GameState::CREDITS) {
        // FIX 1: move() now takes a single sf::Vector2f argument
        creditsNamesText.move({0.f, -creditsScrollSpeed * frameDeltaTime.asSeconds()});

        // FIX 2: Changed .height to .size.y
        if (creditsNamesText.getPosition().y < -(creditsNamesText.getLocalBounds().size.y / 2.f)) {
            // FIX 3: Changed .height to .size.y
            creditsNamesText.setPosition({LOGICAL_SIZE.x / 2.f, LOGICAL_SIZE.y + creditsNamesText.getLocalBounds().size.y / 2.f});
        }
    }
    else if (currentState == GameState::TRANSITIONING) {
        levelManager.update(frameDeltaTime.asSeconds(), window, isFullscreen);
        if (!levelManager.isTransitioning()) {
            setupLevelAssets(currentLevelData, window);

            // Check for custom background
            if (LevelBackgrounds.find(currentLevelData.levelNumber) != LevelBackgrounds.end()){
                // Has custom background
                sf::Texture& levelBgTexture = LevelBackgrounds.find(currentLevelData.levelNumber)->second;
                // Resizing background to fit screen
                float scaleX = 1.0f;
                float scaleY = 1.0f;
                if (isFullscreen){
                    // Fullscreen logic
                    scaleX = LOGICAL_SIZE.x / static_cast<float>(levelBgTexture.getSize().x);
                    scaleY = LOGICAL_SIZE.y / static_cast<float>(levelBgTexture.getSize().y);
                }
                else {
                    scaleX = static_cast<float>(window.getSize().x) / static_cast<float>(levelBgTexture.getSize().x);
                    scaleY = static_cast<float>(window.getSize().y) / static_cast<float>(levelBgTexture.getSize().y);
                }
                levelBgSprite = sf::Sprite(levelBgTexture);
                levelBgSprite->setScale({scaleX, scaleY});
                
            } else levelBgSprite = std::nullopt;

            currentState = GameState::PLAYING;
            if(menuMusic.getStatus() == sf::Music::Status::Playing) menuMusic.stop();
            if(gameMusic.getStatus() != sf::Music::Status::Playing && gameMusic.openFromFile(AUDIO_MUSIC_GAME)) {
                gameMusic.setVolume(gameSettings.musicVolume);
                gameMusic.play();
            }
        }
    }

        // --- Drawing ---
        window.setTitle("Celestial Speedrun");
        window.clear( (currentState == GameState::PLAYING ||
                        currentState == GameState::TRANSITIONING ||
                        currentState == GameState::GAME_OVER_LOSE_DEATH ||
                        currentState == GameState::GAME_OVER_LOSE_FALL ||
                        currentState == GameState::GAME_OVER_WIN)
                       && currentLevelData.platforms.size() > 0
                       ? currentLevelData.backgroundColor
                       : sf::Color::Black);

        if (levelBgSprite.has_value()){
            window.draw(levelBgSprite.value());
        }

        sf::Vector2i currentMousePixelPos = sf::Mouse::getPosition(window);
        sf::Vector2f currentMouseWorldUiPos = window.mapPixelToCoords(currentMousePixelPos, uiView);

    switch(currentState) {
         case GameState::MENU:
            window.setView(uiView);
            if(menuBgSpriteLoaded) window.draw(menuBgSprite);
            else {sf::RectangleShape bg(LOGICAL_SIZE); bg.setFillColor(sf::Color(20,20,50)); window.draw(bg);}

                startButtonText.setFillColor(startButtonText.getGlobalBounds().contains(currentMouseWorldUiPos) ? hoverBtnColor : defaultBtnColor);
                settingsButtonText.setFillColor(settingsButtonText.getGlobalBounds().contains(currentMouseWorldUiPos) ? hoverBtnColor : defaultBtnColor);
                creditsButtonText.setFillColor(creditsButtonText.getGlobalBounds().contains(currentMouseWorldUiPos) ? hoverBtnColor : defaultBtnColor);
                exitButtonText.setFillColor(exitButtonText.getGlobalBounds().contains(currentMouseWorldUiPos) ? exitBtnHoverColor : defaultBtnColor);

                window.draw(menuTitleText); window.draw(startButtonText); window.draw(settingsButtonText);
                window.draw(creditsButtonText); window.draw(exitButtonText);
                break;
            case GameState::SETTINGS:
                window.setView(uiView);
                { sf::RectangleShape bg(LOGICAL_SIZE); bg.setFillColor(sf::Color(20,50,20)); window.draw(bg); }
                settingsBackText.setFillColor(settingsBackText.getGlobalBounds().contains(currentMouseWorldUiPos) ? hoverBtnColor : defaultBtnColor);
                musicVolDownText.setFillColor(musicVolDownText.getGlobalBounds().contains(currentMouseWorldUiPos) ? hoverBtnColor : defaultBtnColor);
                musicVolUpText.setFillColor(musicVolUpText.getGlobalBounds().contains(currentMouseWorldUiPos) ? hoverBtnColor : defaultBtnColor);
                sfxVolDownText.setFillColor(sfxVolDownText.getGlobalBounds().contains(currentMouseWorldUiPos) ? hoverBtnColor : defaultBtnColor);
                sfxVolUpText.setFillColor(sfxVolUpText.getGlobalBounds().contains(currentMouseWorldUiPos) ? hoverBtnColor : defaultBtnColor);
                resolutionPrevText.setFillColor(resolutionPrevText.getGlobalBounds().contains(currentMouseWorldUiPos) && !isFullscreen ? hoverBtnColor : defaultBtnColor);
                resolutionNextText.setFillColor(resolutionNextText.getGlobalBounds().contains(currentMouseWorldUiPos) && !isFullscreen ? hoverBtnColor : defaultBtnColor);
                fullscreenToggleText.setFillColor(fullscreenToggleText.getGlobalBounds().contains(currentMouseWorldUiPos) ? hoverBtnColor : defaultBtnColor);

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
              { sf::RectangleShape bg(LOGICAL_SIZE); sf::Texture bgTxtr(IMG_MENU_BG); bg.setTexture(&bgTxtr); 
                /*bg.setFillColor(sf::Color(20,60,20));*/ window.draw(bg); }
            creditsBackText.setFillColor(creditsBackText.getGlobalBounds().contains(currentMouseWorldUiPos) ? hoverBtnColor : defaultBtnColor);
            window.draw(creditsTitleText); window.draw(creditsNamesText); window.draw(creditsBackText);
            break;
        case GameState::PLAYING:
            mainView.setCenter(playerBody.getPosition() + sf::Vector2f(playerBody.getWidth() / 2.f, playerBody.getHeight() / 2.f - 50.f));
            window.setView(mainView);
            // Background cleared globally


                playerShape.setPosition(playerBody.getPosition());
                for (Tile& t : tiles) {
                    if (t.getFillColor().a > 0 && !t.hasFallen()) {
                        if (doorAnimationOngoing){
                            if (animatedDoorTile != &t){
                                continue;
                            }
                        }
                        window.draw(t);
                        
                    }
                }
                window.draw(playerShape);

                window.setView(uiView);
                {
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
                                           (groundPlat->getType() == phys::bodyType::none ? " TYPE_NONE" : (" Type:" + std::to_string(static_cast<int>(groundPlat->getType())))) + ")";
                            if (groundPlat->getType() == phys::bodyType::portal) {
                                debugString += " LinkID:" + std::to_string(groundPlat->getPortalID());
                            }
                        } else {
                            debugString += " (GroundRef: INVALID)";
                        }
                    }
                    debugText.setString(debugString);
                }
                window.draw(debugText);

                if (goalReached && doorAnimationOngoing){
                    if (animClock.getElapsedTime() >= frameTime_door){
                        animClock.restart();
                        std::cout << "ANIMATED DOOR! Frame "<< doorCurrentFrame 
                        << " [" << doorAnimFramesTopLeft[doorCurrentFrame].x << ","
                        << doorAnimFramesTopLeft[doorCurrentFrame].y << "]" << std::endl;
                        animatedDoorTile->setTextureRect(sf::IntRect({ doorAnimFramesTopLeft[doorCurrentFrame].x,
                                                                    doorAnimFramesTopLeft[doorCurrentFrame].y},
                                                                    {doorWidth, doorHeight}));
                        doorCurrentFrame++;
                    }
                    window.draw(*animatedDoorTile);
                    if (doorCurrentFrame == 5){
                        goalReached = false;
                        doorAnimationOngoing = false;
                        doorCurrentFrame = 0;
                        if (levelManager.hasNextLevel()) {
                            if (levelManager.requestLoadNextLevel(currentLevelData)) {
                                currentState = GameState::TRANSITIONING;
                            } else { 
                                currentState = GameState::MENU; 
                                if(gameMusic.getStatus() == sf::Music::Status::Playing) gameMusic.stop(); 
                                if(menuMusic.getStatus() != sf::Music::Status::Playing && menuMusic.openFromFile(AUDIO_MUSIC_MENU)) menuMusic.play(); 
                            }
                        } else {
                            currentState = GameState::GAME_OVER_WIN;
                            if(gameMusic.getStatus() == sf::Music::Status::Playing) gameMusic.stop();
                            if(menuMusic.getStatus() != sf::Music::Status::Playing && menuMusic.openFromFile(AUDIO_MUSIC_MENU)) menuMusic.play();
                        }
                    }
                }
                break;
            case GameState::TRANSITIONING:
                window.setView(uiView);
                levelManager.draw(window);
                break;
            case GameState::GAME_OVER_WIN:
                 window.setView(uiView);
                 { sf::RectangleShape bg(LOGICAL_SIZE); bg.setFillColor(sf::Color(20,60,20)); window.draw(bg); }
                 gameOverStatusText.setString("All Levels Cleared! You Win!");
                 gameOverOption1Text.setString("Play Again (Level 1)");

                 gameOverOption1Text.setFillColor(gameOverOption1Text.getGlobalBounds().contains(currentMouseWorldUiPos) ? hoverBtnColor : defaultBtnColor);
                 gameOverOption2Text.setFillColor(gameOverOption2Text.getGlobalBounds().contains(currentMouseWorldUiPos) ? hoverBtnColor : defaultBtnColor);

                 window.draw(gameOverStatusText);
                 window.draw(gameOverOption1Text);
                 window.draw(gameOverOption2Text);
                break;
             case GameState::GAME_OVER_LOSE_FALL:
                 window.setView(uiView);
                 { sf::RectangleShape bg(LOGICAL_SIZE); bg.setFillColor(sf::Color(60,20,20)); window.draw(bg); }
                 gameOverStatusText.setString("Game Over! You Fell!");
                 gameOverOption1Text.setString("Retry Level");

                 gameOverOption1Text.setFillColor(gameOverOption1Text.getGlobalBounds().contains(currentMouseWorldUiPos) ? hoverBtnColor : defaultBtnColor);
                 gameOverOption2Text.setFillColor(gameOverOption2Text.getGlobalBounds().contains(currentMouseWorldUiPos) ? hoverBtnColor : defaultBtnColor);

                 window.draw(gameOverStatusText);
                 window.draw(gameOverOption1Text);
                 window.draw(gameOverOption2Text);
                break;
            case GameState::GAME_OVER_LOSE_DEATH:
                window.setView(uiView);
                { sf::RectangleShape bg(LOGICAL_SIZE); bg.setFillColor(sf::Color(70,10,10)); window.draw(bg); }
                gameOverStatusText.setString("Game Over! Hit a Trap!");
                gameOverOption1Text.setString("Retry Level");

                gameOverOption1Text.setFillColor(gameOverOption1Text.getGlobalBounds().contains(currentMouseWorldUiPos) ? hoverBtnColor : defaultBtnColor);
                gameOverOption2Text.setFillColor(gameOverOption2Text.getGlobalBounds().contains(currentMouseWorldUiPos) ? hoverBtnColor : defaultBtnColor);

            window.draw(gameOverStatusText);
            window.draw(gameOverOption1Text);
            window.draw(gameOverOption2Text);
            break;
         default:
             window.setView(uiView);
             { sf::RectangleShape bg(LOGICAL_SIZE); bg.setFillColor(sf::Color::Magenta); window.draw(bg); }
             sf::Text errorText(menuFont);
             errorText.setString("Unknown game state!");
             errorText.setCharacterSize(30);
             errorText.setOrigin({errorText.getLocalBounds().size.x/2.f, errorText.getLocalBounds().size.y/2.f});
             errorText.setPosition({LOGICAL_SIZE.x/2.f, LOGICAL_SIZE.y/2.f});
             window.draw(errorText);
             break;
    }
    window.display();
}

if (menuMusic.getStatus() == sf::Music::Status::Playing) menuMusic.stop();
if (gameMusic.getStatus() == sf::Music::Status::Playing) gameMusic.stop();
return 0;
}