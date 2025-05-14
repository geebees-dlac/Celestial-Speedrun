#ifndef PLATFORMBODY_HPP
#define PLATFORMBODY_HPP
#include <SFML/System/Vector2.hpp>
#include <SFML/System/Clock.hpp>

/**
* phys namespace to encapsulate all physics-related code.
*/
namespace phys
{
	class platformBody
	{
	public:

		////////////////////////////////////////////////////////////
		/// \brief Constructor
		///
		/// Creates a platform body, defaults to a static body (m_type = 0).
		/// Width and height are set to 32 by default, positional
		/// attributes are initialized at sf::Vector2f(0,0).
		///
		////////////////////////////////////////////////////////////
		platformBody();

		////////////////////////////////////////////////////////////
		/// \brief Default destructor
		///
		////////////////////////////////////////////////////////////

		~platformBody();

	public:

		////////////////////////////////////////////////////////////
		/// \brief Member data
		////////////////////////////////////////////////////////////

        unsigned int m_id;///< unique identifier for the platform
		sf::Vector2f m_position;///< Coordinates describing the position of the platform body
		float m_width;///< Width of the platform body
		float m_height;///< Height of the platform body
		int m_type;///< The type of platform body
		bool m_collisionTop;///< Collision information - top
		bool m_collisionBottom;///< Collision information - bottom
		bool m_collisionLeft;///< Collision information - left
		bool m_collisionRight;///< Collision information - right
		bool m_falling;///< Flag for a falling platform
		float m_surfaceVelocity;///< The velocity interacting with objects in contact with the surface
	};
}

#endif //PLATFORMBODY_HPP