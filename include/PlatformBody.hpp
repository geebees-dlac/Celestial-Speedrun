#ifndef PLATFORMBODY_HPP
#define PLATFORMBODY_HPP

#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Rect.hpp>
#include "Sprite.hpp"
#include "PhysicsTypes.hpp"

namespace phys {

    extern const sprite::Sprite DEFAULT_SPRITE;

    class PlatformBody {
    public:
        PlatformBody(
            unsigned int id = 0,
            const sf::Vector2f& position = {0.f, 0.f},
            float width = 32.f,
            float height = 32.f,
            bodyType type = bodyType::platform,
            bool initiallyFalling = false,
            const sf::Vector2f& surfaceVelocity = {0.f, 0.f},
            sprite::Sprite bodySprite = DEFAULT_SPRITE
        );

        void setPortalID(unsigned int id) { m_portalID = id; }
        unsigned int getPortalID() const { return m_portalID; }
        void setTeleportOffset(const sf::Vector2f& offset) { m_teleportOffset = offset; }
        const sf::Vector2f& getTeleportOffset() const { return m_teleportOffset; }
        void update(float deltaTime);

        unsigned int getID() const { return m_id; }
        const sf::Vector2f& getPosition() const { return m_position; }
        float getWidth() const { return m_width; }
        float getHeight() const { return m_height; }
        sf::FloatRect getAABB() const;
        bodyType getType() const { return m_type; }
        sprite::Sprite getSprite() const { return m_bodySprite;}
        sf::Texture getSpriteTexture() const { return m_bodySprite.getTexture();}
        bool isFalling() const { return m_falling; }
        const sf::Vector2f& getSurfaceVelocity() const { return m_surfaceVelocity; }

        void setPosition(const sf::Vector2f& position) { m_position = position; }
        void setFalling(bool falling) { m_falling = falling; }
        void setType(bodyType newType) { m_type = newType; }

    private:
        unsigned int m_id;
        sf::Vector2f m_position;
        sf::Vector2f m_velocity;
        float m_width;
        float m_height;
        bodyType m_type;
        bool m_falling;
        sf::Vector2f m_surfaceVelocity;
        unsigned int m_portalID = 0;
        sf::Vector2f m_teleportOffset = {10.f, 0.f};
        sprite::Sprite m_bodySprite;
    };

}

#endif
