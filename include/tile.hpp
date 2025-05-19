#ifndef TILE_HPP
#define TILE_HPP

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/Transformable.hpp> // Added for completeness

class Tile : public sf::Drawable, public sf::Transformable {
public:
    Tile(const sf::Vector2f& size = {32.f, 32.f},
         const sf::Color& color = sf::Color::White);



    void startFalling(sf::Time delay);/* {
        if (!m_isFalling && !m_hasFallen) {
            m_fallDelayTimer = delay;
            if (m_fallDelayTimer <= sf::Time::Zero) { // Corrected: check m_fallDelayTimer
                m_isFalling = true;
            }
        }
    }*/


    void update(sf::Time deltaTime);/* { // Single definition here
        if (m_hasFallen) return;

        if (!m_isFalling && m_fallDelayTimer > sf::Time::Zero) {
            m_fallDelayTimer -= deltaTime;
            if (m_fallDelayTimer <= sf::Time::Zero) {
                m_isFalling = true;
            }
        }

        if (m_isFalling) {
            // 'this->' is optional for member function calls or sf::Transformable methods
            move(0.f, m_fallSpeed * deltaTime.asSeconds()); // Visual movement
            if (getPosition().y > 800.f) { // Some arbitrary off-screen limit (adjust as needed)
                m_hasFallen = true; // Mark as visually "done"
                // m_isFalling = false; // Optionally stop processing fall once hasFallen
            }
        }
    }*/

    bool isFalling() const { return m_isFalling; }
    bool hasFallen() const { return m_hasFallen; } // Added a getter for m_hasFallen

    void setFillColor(const sf::Color& color) { m_shape.setFillColor(color); }
    const sf::Color& getFillColor() const { return m_shape.getFillColor(); }
    void setTexture(const sf::Texture* texture, bool resetRect = false) { m_shape.setTexture(texture, resetRect); }

    sf::FloatRect getGlobalBounds() const;
    sf::FloatRect getLocalBounds() const;


private:
    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    sf::RectangleShape m_shape;
    bool m_isFalling;
    sf::Time m_fallDelayTimer;
    bool m_hasFallen;
    float m_fallSpeed;
    sf::Time m_visualFallTimerHelper;
};

#endif // TILE_HPP