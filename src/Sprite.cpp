#include "Sprite.hpp"
#include <iostream>

namespace sprite {

    Sprite::Sprite(
        std::string texturePath
    ): m_texturePath(texturePath) {
        if (!m_texture.loadFromFile(m_texturePath)){
            std::cerr << "Failed to load texture: " << m_texturePath << std::endl;
            texturePath = defaultTexturePath;
        }
        else {
            std::cout << "Loaded object with texture: " << m_texturePath << std::endl; // debugging only
        }

        setSprite(m_texture);
    }

    void Sprite::setSprite(sf::Texture texture){
        m_sprite.emplace(texture);
    }

    void Sprite::setTexture(std::string path){
        if (!m_texture.loadFromFile(spriteDirectoryRelativePath+path) && !m_texture.loadFromFile(path)){
            std::cerr << "Failed to load texture: " << path << std::endl;
        }
        else {
            std::cout << "Set object texture to: " << path << std::endl;
            m_sprite.emplace(m_texture);
        }


    }

}