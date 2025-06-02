
#ifndef SPRITE_HPP
#define SPRITE_HPP

#include <string>
#include <SFML/Graphics.hpp>

namespace sprite {
    class Sprite {
        public:
            Sprite(
                std::string texturePath = "../assets/sprites/default.png"
            );

            std::string getTexturePath() const {return m_texturePath;}
            sf::Texture getTexture() const {return m_texture;}
            void setTexture(std::string path);
            void setSprite(sf::Texture texture);
        private:
            std::string m_texturePath;
            std::string defaultTexturePath = "../assets/sprites/default.png";
            std::string spriteDirectoryRelativePath = "../assets/sprites/";

            sf::Texture m_texture;

            std::optional<sf::Sprite> m_sprite;
    };
}


#endif