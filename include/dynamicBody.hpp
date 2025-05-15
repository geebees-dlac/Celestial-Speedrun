#ifndef DYNAMICBODY_HPP
#define DYNAMICBODY_HPP

#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Rect.hpp> // For sf::FloatRect
#include "PhysicsTypes.hpp" 
namespace phys {

    // Forward declaration from your CollisionSystem if needed for CollisionResolutionInfo
    // struct CollisionResolutionInfo;

	class DynamicBody {
	public:
		// Constructor with essential initial values
		DynamicBody(
            const sf::Vector2f& initialPosition = {0.f, 0.f},
            float width = 32.f,
            float height = 32.f,
            const sf::Vector2f& initialVelocity = {0.f, 0.f}
        );

		// ~DynamicBody() = default; // If no custom cleanup needed

		// --- Update Logic ---
        // Takes deltaTime and potentially info from collision resolution
		void update(float deltaTime/*, const CollisionResolutionInfo& collisions*/);
        void applyGravity(const sf::Vector2f& gravity, float deltaTime); // Example specific updater
        void handleInput(const sf::Vector2f& inputDirection); // For m_moveX/Y type logic

		// --- Accessors (Getters) ---
		const sf::Vector2f& getPosition() const { return m_position; }
		const sf::Vector2f& getVelocity() const { return m_velocity; }
		const sf::Vector2f& getLastPosition() const { return m_lastPosition; }
		float getWidth() const { return m_width; }
		float getHeight() const { return m_height; }
		sf::FloatRect getAABB() const; // Axis-Aligned Bounding Box for collisions

        // --- Mutators (Setters) - Use with caution, prefer methods that perform actions ---
		void setPosition(const sf::Vector2f& position);
		void setVelocity(const sf::Vector2f& velocity);
        void addVelocity(const sf::Vector2f& deltaVelocity); 
        void setLastPosition(const sf::Vector2f& position);

        // --- Collision State (if DynamicBody itself needs to track its own state) ---
        bool isOnGround() const { return m_onGround; }
        void setOnGround(bool onGround) { m_onGround = onGround; }
        // ... add m_hitCeiling, m_hitWallLeft, m_hitWallRight if needed for its own logic


	private:
		sf::Vector2f m_position;
		sf::Vector2f m_velocity;
		sf::Vector2f m_lastPosition;    // Useful for continuous collision or interpolation
        sf::Vector2f m_inputDirection;  // Replaces m_moveX, m_moveY if they were inputs

		float m_width;
		float m_height;

        // Internal state flags updated after collision resolution
        bool m_onGround = false;
        bool m_hitCeiling = false;
        bool m_hitWallLeft = false;
        bool m_hitWallRight = false;

        // Could have constants like maxSpeed, accelerationRate, etc. will take onto changes later 
        float m_maxSpeed = 200.f;
        float m_acceleration = 500.f;
	};

} // namespace phys

#endif // DYNAMICBODY_HPP