#include <string>
#include <iostream>
#include <SFML/Graphics.hpp>

#ifndef SPRITE_MANAGER
#define SPRITE_MANAGER

namespace sprites {

    class SpriteManager{
        #define DEFAULT_TEXTURE_FILEPATH "../assets/sprites/default.png"

        public:
            SpriteManager();

            const sf::Texture& getDefaultTexture() const {return defaultTexture;}

            static std::vector<sf::Texture> LoadLevelTextures(std::vector<std::string>);

        private:
            std::string textureDirectoryRelative = "../assets/sprites/";
            std::string defaultTexturePath = textureDirectoryRelative + "default.png";

            sf::Texture defaultTexture;


    };
}



#endif