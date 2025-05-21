#ifndef TILE_HPP
#define TILE_HPP

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/Transformable.hpp>

class Tile : public sf::Drawable, public sf::Transformable {
public:
    Tile(const sf::Vector2f& size = {32.f, 32.f},
         const sf::Color& color = sf::Color::White);

    void startFalling(sf::Time delay);
    void update(sf::Time deltaTime);

    bool isFalling() const { return m_isFalling; }
    bool hasFallen() const { return m_hasFallen; }

    void setFillColor(const sf::Color& color) { m_shape.setFillColor(color); }
    sf::Color getFillColor() const {return m_shape.getFillColor();}
    //const sf::Color& getFillColor() const { return m_shape.getFillColor(); }
    void setTexture(const sf::Texture* texture, bool resetRect = false) { m_shape.setTexture(texture, resetRect); }

    sf::FloatRect getGlobalBounds() const;
    sf::FloatRect getLocalBounds() const;

    // Optional: If you reuse Tile objects, add a reset method
    // void resetState(const sf::Vector2f& newPosition, const sf::Color& newColor = sf::Color::White);

private:
    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    sf::RectangleShape m_shape;
    bool m_isFalling;
    sf::Time m_fallDelayTimer;
    bool m_hasFallen;
    float m_fallSpeed;
};

#endif