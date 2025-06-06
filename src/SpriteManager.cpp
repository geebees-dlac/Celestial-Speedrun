#include "SpriteManager.hpp"

sprites::SpriteManager::SpriteManager(){
    if (!defaultTexture.loadFromFile(defaultTexturePath)){
        std::cerr << "Error loading default texture." << std::endl;
    }
}

std::vector<sf::Texture> LoadLevelTextures(std::vector<std::string> texturePaths){
    // for consistency, first element will be default texture
    sf::Texture defaultTexture(DEFAULT_TEXTURE_FILEPATH);

    std::vector<sf::Texture> TextureList;
    TextureList.push_back(defaultTexture);

    for (std::string txPath : texturePaths){
        sf::Texture newTexture(DEFAULT_TEXTURE_FILEPATH);
        if (!newTexture.loadFromFile(TEXTURE_DIRECTORY+txPath) && !newTexture.loadFromFile(txPath)){
            std::cerr << "Erorr loading texture: " << txPath << std::endl;
            newTexture.loadFromFile(DEFAULT_TEXTURE_FILEPATH);
        }
        else {
            TextureList.push_back(newTexture);
        }
    }

    return TextureList;
}

sf::IntRect sprites::SpriteManager::GetPlayerTextureUponMovement(PlayerMoveDirection direction){
    // Returns player texture section to be rendered, depending on current direction of movement
    // direction: -1 = leftward, 0 = stationary, 1 = rightward
    if (direction == LEFT){
        // Return left-facing character
        return sf::IntRect({96,0}, {48,64});
    }
    else if (direction == RIGHT){
        // Return right-facing character
        return sf::IntRect({48,0}, {48,64});
    }
    else {
        // DEFAULT: Return front-facing character
        return sf::IntRect({0,0}, {48,64});
    }
}

bool sprites::SpriteManager::DoorAnimation(Tile& doorSprite){
    sf::Clock animClock;
    float secondsPerFrame = 0.20f;
    sf::Time frameTime = sf::seconds(secondsPerFrame);
    int current = 0;
    int end = 4;
    int width = 455;
    int height = 610;
    std::vector<sf::Vector2f>animFramesTopLeft;
    animFramesTopLeft.push_back(sf::Vector2f(1275, 255));
    animFramesTopLeft.push_back(sf::Vector2f(250, 1275));
    animFramesTopLeft.push_back(sf::Vector2f(1275, 1275));
    animFramesTopLeft.push_back(sf::Vector2f(250, 2305));

    if (current < end){
    if (animClock.getElapsedTime() >= frameTime){
        animClock.restart();

        doorSprite.setTextureRect(sf::IntRect({animFramesTopLeft[current].x, animFramesTopLeft[current].y},
                                    {width, height}));
        
        current++;
    }
}
}
