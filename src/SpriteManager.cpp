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

sf::IntRect sprites::SpriteManager::GetPlayerTextureUponMovement(int direction){
    // Returns player texture section to be rendered, depending on current direction of movement
    // direction: -1 = leftward, 0 = stationary, 1 = rightward
    if (direction == -1){
        // Return left-facing character
        return sf::IntRect({96,0}, {48,64});
    }
    else if (direction == 0){
        // Return front-facing character
        return sf::IntRect({0,0}, {48,64});
    }
    else if (direction == 1){
        // Return right-facing character
        return sf::IntRect({48,0}, {48,64});
    }
}
