// TextureManager.hpp
#ifndef TEXTURE_MANAGER_HPP
#define TEXTURE_MANAGER_HPP

#include <SFML/Graphics/Texture.hpp>
#include <string>
#include <map>
#include <stdexcept>


class TextureManager {
public:
    TextureManager() = default;

    sf::Texture& getTexture(const std::string& texturePath) {
        auto it = m_textures.find(texturePath);
        if (it == m_textures.end()) {
            sf::Texture texture;
            if (!texture.loadFromFile(texturePath)) {
                throw std::runtime_error("Nah brother your texture aint here: " + texturePath);
            }
            
            texture.setRepeated(true); 
            texture.setSmooth(false); 
            m_textures[texturePath] = texture;
            return m_textures[texturePath];
        }
        return it->second;
    }

private:
    std::map<std::string, sf::Texture> m_textures;
};

#endif 