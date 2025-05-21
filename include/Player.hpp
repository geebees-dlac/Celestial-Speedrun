#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Rect.hpp> 
#include "PhysicsTypes.hpp"

namespace phys {

    class PlatformBody;
	class DynamicBody {
	public:
		DynamicBody(
            const sf::Vector2f& initialPosition = {0.f, 0.f},
            float width = 32.f,
            float height = 32.f,
            const sf::Vector2f& initialVelocity = {0.f, 0.f}
        );

		const sf::Vector2f& getPosition() const { return m_position; }
		const sf::Vector2f& getVelocity() const { return m_velocity; }
		const sf::Vector2f& getLastPosition() const { return m_lastPosition; }
        sf::FloatRect getLastAABB() const; // platform logic zzzz
		float getWidth() const { return m_width; }
		float getHeight() const { return m_height; }
		sf::FloatRect getAABB() const;

		void setPosition(const sf::Vector2f& position);
		void setVelocity(const sf::Vector2f& velocity);
        void addVelocity(const sf::Vector2f& deltaVelocity);
        void setLastPosition(const sf::Vector2f& position); // Typically called once per physics step start

        // Collision State is managed by DynamicBody, and informed by CollisionSystem yes i am documenting this now not ai
        bool isOnGround() const { return m_onGround; }
        void setOnGround(bool onGround) { m_onGround = onGround; } // Set by main loop after collision

        // --- Specific Platform Interaction Logic ---
        void setGroundPlatform(const PlatformBody* platform);
        const PlatformBody* getGroundPlatform() const;

        void setTryingToDrop(bool trying); // Called from input
        bool isTryingToDropFromPlatform() const;

        void setGroundPlatformTemporarilyIgnored(const PlatformBody* platform);
        const PlatformBody* getGroundPlatformTemporarilyIgnored() const;


	private:
		sf::Vector2f m_position;
		sf::Vector2f m_velocity;
		sf::Vector2f m_lastPosition;

		float m_width;
		float m_height;

        bool m_onGround = false;

        // --- State for specific platform interactions ---
        const PlatformBody* m_groundPlatform = nullptr;
        bool m_isTryingToDrop = false;
        const PlatformBody* m_tempIgnoredPlatform = nullptr;

        // m_maxSpeed, m_acceleration (not directly used by collision system, but good to keep)
        float m_maxSpeed = 200.f;
        float m_acceleration = 500.f;
	};

}

#endif 