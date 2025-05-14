#ifndef DYNAMICBODY_HPP
#define DYNAMICBODY_HPP
#include <SFML/System/Vector2.hpp>

namespace phys
{
	class dynamicBody
	{
	public:
		dynamicBody();
		~dynamicBody();

	public:
		int m_moveX;
		int m_moveY;
		sf::Vector2f m_lastPosition;
		sf::Vector2f m_position;
		sf::Vector2f m_velocity;
		float m_width;
		float m_height;
	};
}

#endif