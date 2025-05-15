#ifndef PLATFORMBODY_HPP
#define PLATFORMBODY_HPP

#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Rect.hpp> // For sf::FloatRect
#include "PhysicsTypes.hpp" 
// Make sure your bodyType enum is accessible
// If it's in CollisionSystem.hpp, you might need to include that or move the enum
// to a more general "PhysicsTypes.hpp"
#include "CollisionSystem.hpp" // Or wherever phys::bodyType is defined

namespace phys {

	class PlatformBody {
	public:
		PlatformBody(
            unsigned int id = 0,
            const sf::Vector2f& position = {0.f, 0.f},
            float width = 32.f,
            float height = 32.f,
            bodyType type = bodyType::platform, // Use your enum
            bool initiallyFalling = false,
            const sf::Vector2f& surfaceVelocity = {0.f, 0.f} // Changed to Vector2f
        );

		// ~PlatformBody() = default; // If no custom cleanup

        // Optional: Update method if platforms can move/animate on their own
        // (e.g., moving platforms, falling platforms not handled by main dynamic sim)
        void update(float deltaTime);

		// --- Accessors (Getters) ---
		unsigned int getID() const { return m_id; }
		const sf::Vector2f& getPosition() const { return m_position; }
		float getWidth() const { return m_width; }
		float getHeight() const { return m_height; }
		sf::FloatRect getAABB() const;
		bodyType getType() const { return m_type; }
		bool isFalling() const { return m_falling; } // Renamed for clarity
		const sf::Vector2f& getSurfaceVelocity() const { return m_surfaceVelocity; }

        // --- Mutators (Setters) ---
        // Typically, platform properties are static after creation, but setters can be added if needed.
        void setPosition(const sf::Vector2f& position) { m_position = position; } // If platforms can move
        void setFalling(bool falling) { m_falling = falling; }
        // ... others if necessary

	private:
		unsigned int m_id;
		sf::Vector2f m_position;
        sf::Vector2f m_velocity; // Add if m_falling or type::moving actually moves it
		float m_width;
		float m_height;
		bodyType m_type;
		bool m_falling;
		sf::Vector2f m_surfaceVelocity; // Now a vector for more flexibility

        // If m_falling implies gravity, it might have:
        // static const sf::Vector2f GRAVITY; (defined in .cpp)

        // For type::vanishing or timed behaviors, store remaining time, not a clock:
        // float m_vanishTimer = -1.f; // -1 means inactive/permanent
	};

} // namespace phys

#endif // PLATFORMBODY_HPP