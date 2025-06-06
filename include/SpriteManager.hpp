#include <string>
#include <iostream>
#include <SFML/Graphics.hpp>
#include "Tile.hpp"

#ifndef SPRITE_MANAGER
#define SPRITE_MANAGER

namespace sprites {

    class SpriteManager{
        #define DEFAULT_TEXTURE_FILEPATH "../assets/sprites/default.png"
        #define TEXTURE_DIRECTORY "../assets/sprites/"
        #define IMAGE_DIRECTORY "../assets/images/"
        #define LEVEL_BG_ID "LEVEL_BG"

        public:    
            typedef enum {
                NONE = 0,
                LEFT = 1,
                RIGHT = 2
            } PlayerMoveDirection;
            
            SpriteManager();

            const sf::Texture& getDefaultTexture() const {return defaultTexture;}

            static std::vector<sf::Texture> LoadLevelTextures(std::vector<std::string>);

            static sf::IntRect GetPlayerTextureUponMovement(PlayerMoveDirection direction);

            static bool DoorAnimation(Tile& doorSprite);
        private:
            std::string textureDirectoryRelative = "../assets/sprites/";
            std::string defaultTexturePath = textureDirectoryRelative + "default.png";

            sf::Texture defaultTexture;

    };
}



#endif