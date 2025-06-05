#ifndef PLATFORM_DRAWABLE_HPP
#define PLATFORM_DRAWABLE_HPP

#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include "PlatformBody.hpp" 

struct PlatformDrawable {
    sf::Sprite sprite;
    unsigned int bodyId; 

    PlatformDrawable(unsigned int id = 0) : bodyId(id) {}

    void initialize(sf::Texture& texture, const phys::PlatformBody& body) {
        bodyId = body.getID(); 

        sprite.setTexture(texture);
        sprite.setPosition(body.getPosition());

        sprite.setTextureRect(sf::IntRect(
            0, 0, 
            static_cast<int>(body.getWidth()),
            static_cast<int>(body.getHeight())
        ));
    }


    void updatePosition(const sf::Vector2f& newPosition) {
        sprite.setPosition(newPosition);
    }

    void reinitializeVisuals(sf::Texture& newTexture, const phys::PlatformBody& body) {
        sprite.setTexture(newTexture); 
        sprite.setPosition(body.getPosition());
        sprite.setTextureRect(sf::IntRect(
            0, 0,
            static_cast<int>(body.getWidth()),
            static_cast<int>(body.getHeight())
        ));
    }

    void draw(sf::RenderWindow& window) const {
        window.draw(sprite);
    }
};

#endif