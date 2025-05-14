#ifndef TILE_HPP
#define TILE_HPP

#include <SFML/Graphics.hpp>

class tile : public sf::RectangleShape
{
    public:
        tile();
        ~tile();

    public:
        bool m_falling;
        sf::Time m_fallTime;
};

#endif // TILE_HPP