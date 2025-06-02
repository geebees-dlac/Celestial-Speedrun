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
        if (!newTexture.loadFromFile(txPath)){
            std::cerr << "Erorr loading texture: " << txPath << std::endl;
            newTexture.loadFromFile(DEFAULT_TEXTURE_FILEPATH);
        }
        else {
            TextureList.push_back(newTexture);
        }
    }

    return TextureList;
}