#include <iostream>
#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath> // For M_PI, sin, pow, std::abs, std::cos

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace math {
namespace interpolate {
    // t: current time
    // b: beginning value
    // c: change in value (ending value - beginning value)
    // d: duration
    float sineEaseIn(float t, float b, float c, float d) {
        if (d == 0.f) return b; // Avoid division by zero, return start value
        return -c * std::cos(t / d * (M_PI / 2.f)) + c + b;
    }

    float quadraticEaseInOut(float t, float b, float c, float d) {
        if (d == 0.f) return b; // Avoid division by zero
        t /= d / 2.f;
        if (t < 1.f) return c / 2.f * t * t + b;
        t--; // t is now effectively in range [0, 1] for the second half easing out
        return -c / 2.f * (t * (t - 2.f) - 1.f) + b;
    }
} // namespace interpolate
} // namespace math

namespace phys {

// Forward declarations (optional here as definitions follow, but good practice)
struct dynamicBody; 
struct platformBody;

enum class bodyType {
    none,
    platform,
    conveyorBelt,
    moving,
    jumpthrough,
    falling,
    vanishing,
    spring 
};

struct platformBody {
    sf::Vector2f m_position = {0.f, 0.f};
    float m_width = 0.f;
    float m_height = 0.f;
    int m_id = -1; // Default to an invalid ID
    bodyType m_type = bodyType::platform;
    float m_surfaceVelocity = 0.f; // For conveyors
    bool m_falling = false;        // State for falling platforms

    platformBody() = default; // Use default constructor
    ~platformBody() = default; // Use default destructor
};

struct dynamicBody {
    sf::Vector2f m_position = {0.f, 0.f};
    sf::Vector2f m_lastPosition = {0.f, 0.f}; // Position in the previous frame
    sf::Vector2f m_velocity = {0.f, 0.f};     // Current velocity
    float m_width = 0.f;
    float m_height = 0.f;
    int m_moveX = 0; // These seem unused in the main game logic but are part of the struct
    int m_moveY = 0;

    dynamicBody() = default;
    ~dynamicBody() = default;
};

struct CollisionInfo {
    bool m_collisionTop = false;
    bool m_collisionBottom = false;
    bool m_collisionLeft = false;
    bool m_collisionRight = false;

    CollisionInfo() = default;
};

class collisionSystem {
public:
    collisionSystem();
    ~collisionSystem();

    // Resolves collisions between player and platforms.
    // Modifies player's position and velocity directly.
    // Sets internal collision flags accessible via getCollisionInfo().
    // Updates getBodyInfo() to reflect the primary platform interacted with.
    void resolveCollisions(dynamicBody* player, platformBody* platforms, int numPlatforms);
    
    // Sets the collision flags. Called by resolveCollisions and potentially by main loop to reset.
    void setCollisionInfo(bool top, bool bottom, bool left, bool right);
    
    const CollisionInfo& getCollisionInfo() const; // Returns flags indicating sides of collision
    const platformBody& getBodyInfo() const;       // Returns info about the platform primarily interacted with

private:
    CollisionInfo m_currentCollisionInfo; // Stores flags for the last resolution pass
    platformBody m_collidedPlatformInfo;  // Stores info about the platform primarily involved
};

// Definitions for collisionSystem methods
collisionSystem::collisionSystem() {
    // m_currentCollisionInfo is default-initialized (all flags false)
    // m_collidedPlatformInfo is default-initialized. Explicitly set type to none.
    m_collidedPlatformInfo.m_type = bodyType::none;
    m_collidedPlatformInfo.m_id = -1;
}

collisionSystem::~collisionSystem() {
    // No dynamic resources to clean up in this basic version
}

void collisionSystem::setCollisionInfo(bool top, bool bottom, bool left, bool right) {
    m_currentCollisionInfo.m_collisionTop = top;
    m_currentCollisionInfo.m_collisionBottom = bottom;
    m_currentCollisionInfo.m_collisionLeft = left;
    m_currentCollisionInfo.m_collisionRight = right;
}

const CollisionInfo& collisionSystem::getCollisionInfo() const {
    return m_currentCollisionInfo;
}

const platformBody& collisionSystem::getBodyInfo() const {
    return m_collidedPlatformInfo;
}

void collisionSystem::resolveCollisions(dynamicBody* player, platformBody* platforms, int numPlatforms) {
    // Accumulated collision flags for this resolution pass
    bool acc_col_top = false;
    bool acc_col_bottom = false;
    bool acc_col_left = false;
    bool acc_col_right = false;
    
    // Reset/initialize the platform info for this pass
    platformBody best_collided_platform_this_pass;
    best_collided_platform_this_pass.m_type = bodyType::none;
    best_collided_platform_this_pass.m_id = -1;

    for (int i = 0; i < numPlatforms; ++i) {
        platformBody* platform = &platforms[i];

        // Skip platforms that are "disabled" (e.g., moved far away)
        if (platform->m_position.x < -9000.f && platform->m_position.y < -9000.f) { // check both x and y for safety
             continue;
        }


        sf::FloatRect playerRect(player->m_position.x, player->m_position.y, player->m_width, player->m_height);
        sf::FloatRect platformRect(platform->m_position.x, platform->m_position.y, platform->m_width, platform->m_height);

        if (playerRect.intersects(platformRect)) {
            // Default to this platform if it's the first one encountered or a bottom collision
            if (best_collided_platform_this_pass.m_type == bodyType::none) {
                best_collided_platform_this_pass = *platform;
            }

            // Calculate penetration depths
            float dx_centers = (player->m_position.x + player->m_width / 2.f) - (platform->m_position.x + platform->m_width / 2.f);
            float dy_centers = (player->m_position.y + player->m_height / 2.f) - (platform->m_position.y + platform->m_height / 2.f);

            float combined_half_widths = (player->m_width + platform->m_width) / 2.f;
            float combined_half_heights = (player->m_height + platform->m_height) / 2.f;

            float overlap_x = combined_half_widths - std::abs(dx_centers);
            float overlap_y = combined_half_heights - std::abs(dy_centers);

            if (overlap_x > 0 && overlap_y > 0) { // Ensure there's an actual overlap on both axes
                // Determine primary axis of collision based on previous non-overlapping state or least penetration
                bool resolve_as_vertical = (overlap_y < overlap_x); // Default: resolve along axis of least penetration

                // Refine axis choice based on previous frame (if not overlapping before)
                sf::FloatRect playerPrevRect(player->m_lastPosition.x, player->m_lastPosition.y, player->m_width, player->m_height);
                if (!playerPrevRect.intersects(platformRect)) {
                    bool prev_x_overlap = (player->m_lastPosition.x + player->m_width > platform->m_position.x && 
                                         player->m_lastPosition.x < platform->m_position.x + platform->m_width);
                    bool prev_y_overlap = (player->m_lastPosition.y + player->m_height > platform->m_position.y &&
                                         player->m_lastPosition.y < platform->m_position.y + platform->m_height);
                    
                    if (prev_x_overlap && !prev_y_overlap) resolve_as_vertical = true;  // Entered vertically
                    else if (!prev_x_overlap && prev_y_overlap) resolve_as_vertical = false; // Entered horizontally
                    // If entered diagonally or from inside, stick to overlap_y < overlap_x
                }


                if (resolve_as_vertical) { // Vertical collision
                    if (dy_centers < 0) { // Player is above platform (center below platform center means player top is lower)
                        bool can_land = true;
                        if (platform->m_type == bodyType::jumpthrough) {
                            // Player's feet (last_pos + height) must have been at or above platform's top (plat_pos_y)
                            // A small tolerance (e.g., 1.0f) helps with floating point comparisons.
                            if (player->m_lastPosition.y + player->m_height > platform->m_position.y + 1.0f) {
                                can_land = false; // Player was below or inside, so pass through
                            }
                        }
                        
                        if (can_land) {
                            player->m_position.y = platform->m_position.y - player->m_height; // Snap to top
                            if (player->m_velocity.y > 0) player->m_velocity.y = 0; // Stop downward movement
                            acc_col_bottom = true;
                            best_collided_platform_this_pass = *platform; // Prioritize platform stood upon
                        }
                    } else { // Player is below platform
                        if (platform->m_type != bodyType::jumpthrough) { // Can't hit head on jump-through from below
                            player->m_position.y = platform->m_position.y + platform->m_height; // Snap to bottom
                            if (player->m_velocity.y < 0) player->m_velocity.y = 0; // Stop upward movement
                            acc_col_top = true;
                             if (best_collided_platform_this_pass.m_type == bodyType::none && !acc_col_bottom) { // If no bottom collision yet
                                best_collided_platform_this_pass = *platform;
                            }
                        }
                    }
                } else { // Horizontal collision
                    if (platform->m_type != bodyType::jumpthrough) { // Jump-through platforms usually don't have side collision
                        if (dx_centers < 0) { // Player is to the left of platform's center
                            player->m_position.x = platform->m_position.x - player->m_width; // Snap to left
                            if (player->m_velocity.x > 0) player->m_velocity.x = 0;
                            acc_col_right = true; // Player's right side hit platform's left
                        } else { // Player is to the right of platform's center
                            player->m_position.x = platform->m_position.x + platform->m_width; // Snap to right
                            if (player->m_velocity.x < 0) player->m_velocity.x = 0;
                            acc_col_left = true; // Player's left side hit platform's right
                        }
                        if (best_collided_platform_this_pass.m_type == bodyType::none && !acc_col_bottom) { // If no bottom collision yet
                            best_collided_platform_this_pass = *platform;
                        }
                    }
                }
            }
        }
    }
    
    // Update the collision system's state based on accumulated results
    this->setCollisionInfo(acc_col_top, acc_col_bottom, acc_col_left, acc_col_right);
    m_collidedPlatformInfo = best_collided_platform_this_pass; // Store the most relevant platform info
}

} // namespace phys

// tile class definition
// Derives from sf::RectangleShape to easily draw and manage tile properties.
class tile : public sf::RectangleShape {
public:
    sf::Time m_fallTime = sf::Time::Zero; // Used by falling platforms in main logic
    bool m_falling = false;               // State for falling platforms

    tile() : sf::RectangleShape() {} // Call base class constructor
    
    // sf::Shape (base of sf::RectangleShape) has a virtual destructor.
    // It's good practice for derived classes to also have a virtual destructor.
    // This also solves vtable errors if the destructor is the first non-inline virtual function.
    virtual ~tile() {} 
};

// --- End of definitions for missing symbols ---


int main(void){ // Your existing main function starts here

    const sf::Vector2f tileSize = sf::Vector2f(32, 32);
    const int NUM_PLATFORM_OBJECTS = 24;

    float GRAVITY = 0.f;//variable gravity

	sf::RenderWindow window(sf::VideoMode(800, 600), "Project - T", sf::Style::Default);
	window.setKeyRepeatEnabled(false);
	window.setVerticalSyncEnabled(true);
	window.setFramerateLimit(60);
	sf::Event e;

	//set up the view
	sf::View mainView(sf::FloatRect(0.f, 0.f, 1200.f, 800.f)); // View is larger than window, implies camera can move
	window.setView(mainView);

	//instantiate the collision system object
	phys::collisionSystem collisionSys;

	//set up some platforms - all normal platforms by default
	phys::platformBody bodies[NUM_PLATFORM_OBJECTS];
	for (int i = 0; i < NUM_PLATFORM_OBJECTS;i++){
        bodies[i].m_id = i;//assign each platform an id
		bodies[i].m_width = tileSize.x*4;
        bodies[i].m_height = tileSize.y;
        bodies[i].m_type = phys::bodyType::platform; // Default type
	}

	//place them on the screen somewhere...
	bodies[0].m_position = sf::Vector2f(0.f,window.getSize().y-32.f);
	bodies[1].m_position = sf::Vector2f(0.f, 356.f);
	bodies[2].m_position = sf::Vector2f(480.f, window.getSize().y - 192.f);
	bodies[3].m_position = sf::Vector2f(0.f, 96.f);
	bodies[4].m_position = sf::Vector2f(640.f, 224.f);
	bodies[5].m_position = sf::Vector2f(480.f, window.getSize().y - 32.f);
	bodies[6].m_position = sf::Vector2f(672.f, window.getSize().y - 260.f);
	bodies[7].m_position = sf::Vector2f(96.f, window.getSize().y - 128.f);
	bodies[8].m_position = sf::Vector2f(640.f, window.getSize().y - 160.f); // Corrected: Was 640 without .f
	bodies[9].m_position = sf::Vector2f(640.f, 64.f);
    bodies[10].m_position = sf::Vector2f(292.f, window.getSize().y - 128.f); // Corrected: Was 292 without .f

	//conveyor belt
	bodies[11].m_position = sf::Vector2f(window.getSize().x / 4.f, window.getSize().y / 2.f);
    bodies[11].m_width = tileSize.x*10;
    bodies[11].m_height = tileSize.y;
    bodies[11].m_type = phys::bodyType::conveyorBelt;
    bodies[11].m_surfaceVelocity = 12.f; // Used in main logic for conveyor effect

	//moving platform
	bodies[12].m_height = tileSize.y/8.f;
	bodies[12].m_width = tileSize.x*5;
	bodies[12].m_position = sf::Vector2f(window.getSize().x/4.f,window.getSize().y/4.f);
	bodies[12].m_type = phys::bodyType::moving;
	int platformDir = 1;
	float platformVelocity = 0.f; // This is local to main, for this specific platform

	//jump through platforms
	bodies[13].m_width = tileSize.x;
	bodies[13].m_height = tileSize.y/8.f;
	bodies[13].m_position = sf::Vector2f(160.f,window.getSize().y - 224.f);
	bodies[13].m_type = phys::bodyType::jumpthrough;

	bodies[14].m_width = tileSize.x;
	bodies[14].m_height = tileSize.y/8.f;
	bodies[14].m_position = sf::Vector2f(576.f, window.getSize().y - 288.f); // Corrected: Was 576 without .f
	bodies[14].m_type = phys::bodyType::jumpthrough;

    //falling platforms
	bodies[15].m_width = tileSize.x;
	bodies[15].m_height = tileSize.y;
	bodies[15].m_position = sf::Vector2f(192.f, 64.f); // Corrected: Was 192 without .f
	bodies[15].m_type = phys::bodyType::falling;

	bodies[16].m_width = tileSize.x;
	bodies[16].m_height = tileSize.y;
	bodies[16].m_position = sf::Vector2f(256.f, 64.f);
	bodies[16].m_type = phys::bodyType::falling;

	bodies[17].m_width = tileSize.x;
	bodies[17].m_height = tileSize.y;
	bodies[17].m_position = sf::Vector2f(320.f, 64.f);
	bodies[17].m_type = phys::bodyType::falling;

	bodies[18].m_width = tileSize.x;
	bodies[18].m_height = tileSize.y;
	bodies[18].m_position = sf::Vector2f(384.f, 64.f);
	bodies[18].m_type = phys::bodyType::falling;

	bodies[19].m_width = tileSize.x;
	bodies[19].m_height = tileSize.y;
	bodies[19].m_position = sf::Vector2f(448.f, 64.f);
	bodies[19].m_type = phys::bodyType::falling;

    //vanishing platforms
	bodies[20].m_width = tileSize.x;
	bodies[20].m_height = tileSize.y;
	bodies[20].m_position = sf::Vector2f(192.f, -32.f);
	bodies[20].m_type = phys::bodyType::vanishing;

	bodies[21].m_width = tileSize.x;
	bodies[21].m_height = tileSize.y;
	bodies[21].m_position = sf::Vector2f(256.f, -32.f);
	bodies[21].m_type = phys::bodyType::vanishing;

	bodies[22].m_width = tileSize.x;
	bodies[22].m_height = tileSize.y;
	bodies[22].m_position = sf::Vector2f(320.f, -32.f);
	bodies[22].m_type = phys::bodyType::vanishing;

	bodies[23].m_width = tileSize.x;
	bodies[23].m_height = tileSize.y;
	bodies[23].m_position = sf::Vector2f(384.f, -32.f);
	bodies[23].m_type = phys::bodyType::vanishing;


	tile tiles[NUM_PLATFORM_OBJECTS]; // Array of tile objects (which are sf::RectangleShape)
	for (int i = 0; i < NUM_PLATFORM_OBJECTS; ++i){
		tiles[i].setPosition(bodies[i].m_position);
		tiles[i].setSize(sf::Vector2f(bodies[i].m_width, bodies[i].m_height));
		switch(bodies[i].m_type){
            case phys::bodyType::platform:
                tiles[i].setFillColor(sf::Color(255, 0, 0, 255));
                break;
            case phys::bodyType::conveyorBelt:
                tiles[i].setFillColor(sf::Color(255, 100, 0, 255));
                break;
            case phys::bodyType::moving:
                tiles[i].setFillColor(sf::Color(0, 255, 0, 255));
                break;
            case phys::bodyType::jumpthrough:
                // Make jump-through platforms less opaque to distinguish them or thinner look
                tiles[i].setFillColor(sf::Color(0, 255, 255, 180)); 
                break;
            case phys::bodyType::falling:
                tiles[i].setFillColor(sf::Color(255, 255, 0, 255));
                break;
            case phys::bodyType::vanishing:
                tiles[i].setFillColor(sf::Color(255, 0, 255, 255));
                break;
            default: // phys::bodyType::none or others
                tiles[i].setFillColor(sf::Color(128, 128, 128, 255)); // Grey for unassigned/none
                break;
		}
	}

	//set up for the player
	phys::dynamicBody playerBody;
	playerBody.m_position = sf::Vector2f(window.getSize().x/2.f, window.getSize().y/2.f);
	playerBody.m_width = tileSize.x;
	playerBody.m_height = tileSize.y;
	playerBody.m_velocity = sf::Vector2f(0.f,0.f); // Initialized in struct def, but good to be explicit
	playerBody.m_lastPosition = playerBody.m_position; // Init last_pos to current_pos
	// playerBody.m_moveX = 0; // Unused in this snippet's logic
	// playerBody.m_moveY = 0; // Unused

	sf::RectangleShape player; // Visual representation of the player
	player.setFillColor(sf::Color(255, 255, 255, 255));
	player.setSize(sf::Vector2f(playerBody.m_width, playerBody.m_height));
	player.setPosition(playerBody.m_position);

	//time variables
	sf::Clock tickClock;
	sf::Time timeSinceLastUpdate = sf::Time::Zero;
	sf::Time duration = sf::Time::Zero; // For moving platform cycle
	const sf::Time TimePerFrame = sf::seconds(1.f / 60.f); // Constant for fixed timestep
	sf::Time jumpTime = sf::Time::Zero; // Tracks time since jump button pressed / for jump duration
	sf::Time vanishingTime = sf::Time::Zero; // For vanishing platform cycle
	int oddEven = 1; // For alternating vanishing platforms
	float alpha = 0.f; // For alpha interpolation of vanishing platforms
	// Spring related variables - seem unused in current player logic
	// sf::Time springTime = sf::Time::Zero;
	// bool springPeak = false;
	// bool canSpring = false;
	// bool springSprung = false;

	//get information about the joystick
    if (sf::Joystick::isConnected(0)) {
        sf::Joystick::Identification id = sf::Joystick::getIdentification(0);
        std::cout << "\nVendor ID: " << id.vendorId << "\nProduct ID: " << id.productId << std::endl;
        sf::String controller("Joystick Use: " + id.name);
        window.setTitle(controller);

        unsigned int buttonCount = sf::Joystick::getButtonCount(0);
        bool hasZ = sf::Joystick::hasAxis(0, sf::Joystick::Z);
        std::cout << "Button count: " << buttonCount << std::endl;
        std::cout << "Has a z-axis: " << hasZ << std::endl;
    } else {
        std::cout << "No joystick connected." << std::endl;
    }


	int turbo = 1;// Player speed multiplier

	//for movement
	sf::Vector2f speed = sf::Vector2f(0.f,0.f); // Player input speed

    //collision info from collisionSystem
	// bool intersection = false; // This local 'intersection' seems for specific debug/falling platform logic
	phys::bodyType type = phys::bodyType::none; // Type of platform player is interacting with
	bool collisionTop = false;
	bool collisionBottom = false;
	bool collisionLeft = false;
	bool collisionRight = false;
	bool canJump = false; // Flag: is player allowed to initiate/continue a jump
	bool jumped = false;  // Flag: is jump button currently pressed
	unsigned int jumpCount = 0; // To detect initial press vs. hold for jump

	//for debug info
	int debug = -1; // Toggle for debug output

	bool running = true;
	while (running){
		while (window.pollEvent(e)){
			if (e.type == sf::Event::Closed){
				window.close();
				running = false; // Ensure loop terminates
                break; 
			}

			if (e.type == sf::Event::KeyPressed){
				switch (e.key.code){
				case sf::Keyboard::Escape:
					window.close();
					running = false; // Ensure loop terminates
                    break; 
                case sf::Keyboard::D: // 'D' for Debug
					debug *= -1;
					break;
				default:
					break;
				}
			}
		}
        if (!running) break; // Exit outer loop if window closed

		sf::Time elapsedTime = tickClock.restart();
		timeSinceLastUpdate += elapsedTime;

		while (timeSinceLastUpdate > TimePerFrame)
		{
			timeSinceLastUpdate -= TimePerFrame;
            playerBody.m_lastPosition = playerBody.m_position; // Store position before updates

            // --- Input Handling ---
            speed = sf::Vector2f(0.f, 0.f); // Reset speed from input each frame
            if (sf::Joystick::isConnected(0)) {
                 speed = sf::Vector2f(sf::Joystick::getAxisPosition(0, sf::Joystick::X), 
                                      sf::Joystick::getAxisPosition(0, sf::Joystick::Y)); // Y-axis not used for platformer movement typically
                 
                 // Deadzone for X-axis
                 if (std::abs(speed.x) < 15.f) speed.x = 0.f;


                if (sf::Joystick::isButtonPressed(0, 2)){ // "X" button on XBox 360 for Turbo
                    turbo = 2;
                } else {
                    turbo = 1;
                }

                if(sf::Joystick::isButtonPressed(0,0)){ // "A" button for Jump
                    if (!jumped) jumpCount++; // Count on new press
                    jumped = true;
                } else {
                    jumped = false;
                    // jumpCount reset below based on canJump logic or when grounded
                }

                if(sf::Joystick::isButtonPressed(0,1)){ // "B" button to close
                    window.close();
                    running = false;
                    break; 
                }
            }
            // Keyboard controls (example, can be added)
            // --- Keyboard Controls ---
            // Horizontal Movement (A/D or Left/Right)
            // We check if speed.x is still 0 from the reset or joystick deadzone.
            // If joystick is already providing significant input, we might not want keyboard to override it
            // to zero if no A/D key is pressed.
            // However, the current logic sets speed.x and then joystick might overwrite it.
            // Better: set speed.x based on keyboard first, then joystick can override IF active.
            // For simplicity now: keyboard sets speed.x if it's pressed.

            bool key_left_pressed = sf::Keyboard::isKeyPressed(sf::Keyboard::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Left);
            bool key_right_pressed = sf::Keyboard::isKeyPressed(sf::Keyboard::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Right);

            if (key_left_pressed && !key_right_pressed) {
                speed.x = -100.f; // Set to max negative speed for keyboard
            } else if (key_right_pressed && !key_left_pressed) {
                speed.x = 100.f;  // Set to max positive speed for keyboard
            }
            // If neither or both are pressed, speed.x remains what joystick set it to (or 0.f if joystick inactive/deadzone)

            // Jumping (W or Up Arrow or Space)
            bool key_jump_pressed = sf::Keyboard::isKeyPressed(sf::Keyboard::W) ||
                                    sf::Keyboard::isKeyPressed(sf::Keyboard::Up) ||
                                    sf::Keyboard::isKeyPressed(sf::Keyboard::Space);

            if (key_jump_pressed) {
                if (!jumped) { // If not already considered 'jumped' (e.g., from joystick this frame)
                    jumpCount++;
                }
                jumped = true; // Mark jump button as active
            } else {
                // If NO jump key is pressed AND joystick jump button is also not pressed,
                // then mark 'jumped' as false.
                if ((!sf::Joystick::isConnected(0) || !sf::Joystick::isButtonPressed(0, 0))) {
                    jumped = false;
                }
            }

            // Turbo (Left Shift)
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) {
                turbo = 2;
            } else {
                // If Left Shift is not pressed AND joystick turbo button is also not pressed,
                // then reset turbo.
                if (!sf::Joystick::isConnected(0) || !sf::Joystick::isButtonPressed(0, 2)) {
                    turbo = 1;
                }
            }


            // --- Platform Updates (before player) ---
			if(duration.asSeconds() >= 4.f){ // Moving platform cycle
                platformDir *= -1;
                duration = sf::Time::Zero;
			} else {
                duration += TimePerFrame;
            }

			if(duration.asSeconds() <= 3.f) // Actual movement phase
                platformVelocity = TimePerFrame.asSeconds()*(platformDir * 700.f * math::interpolate::quadraticEaseInOut(duration.asSeconds(), 0.f, 1.f, 3.f));
            else // Pause phase
                platformVelocity = 0.f;
            
            bodies[12].m_position.x += platformVelocity;
            tiles[12].setPosition(bodies[12].m_position);


            for(int i=0; i<NUM_PLATFORM_OBJECTS; ++i){ // Falling platforms logic
                if (bodies[i].m_type == phys::bodyType::falling) {
                    bool player_on_this_falling_platform = 
                        (collisionSys.getCollisionInfo().m_collisionBottom &&
                         collisionSys.getBodyInfo().m_id == bodies[i].m_id &&
                         collisionSys.getBodyInfo().m_type == phys::bodyType::falling);

                    if(player_on_this_falling_platform && !bodies[i].m_falling) { // Player lands on it
                         tiles[i].m_fallTime = sf::Time::Zero; // Reset timer for this specific tile
                         bodies[i].m_falling = true; // Mark body as "triggered to fall"
                         tiles[i].m_falling = false; // Tile itself hasn't started visual fall
                    }

                    if(bodies[i].m_falling) { // If body is triggered
                        tiles[i].m_fallTime += TimePerFrame;
                        if (tiles[i].m_fallTime.asSeconds() > 0.5f) {
                            tiles[i].m_falling = true; // Start visual fall
                        }
                    }
                    
                    if(tiles[i].m_falling){ // If tile is visually falling
                        bodies[i].m_position = sf::Vector2f(-9999.f,-9999.f); // Move physics body away
                        tiles[i].move(0.f, TimePerFrame.asSeconds()*1000.f);
                    }
                }
            }

            // Vanishing platforms
            if(vanishingTime.asSeconds() > 2.f){
                vanishingTime = sf::Time::Zero;
                oddEven *= -1;
            } else {
                vanishingTime += TimePerFrame;
            }

            for(int i=20; i<24; ++i){ // Vanishing platforms hardcoded indices [20,23]
                 bool is_even_platform = (i % 2 == 0);
                 bool should_vanish_now = (oddEven == 1 && is_even_platform) || (oddEven == -1 && !is_even_platform);

                 if (should_vanish_now) { // Platform should be vanishing
                    if(vanishingTime.asSeconds() < 1.f){
                        alpha = math::interpolate::sineEaseIn(vanishingTime.asSeconds(),0.f,255.f, 1.f);
                        tiles[i].setFillColor(sf::Color(255,0,255,255-(unsigned char)alpha));
                    } else {
                        tiles[i].setFillColor(sf::Color(255,0,255,0));
                    }
                    if(tiles[i].getFillColor().a <= 10) // Using small tolerance for alpha comparison
                        bodies[i].m_position = sf::Vector2f(-9999.f,-9999.f);
                 } else { // Platform should be reappearing
                    if(vanishingTime.asSeconds() < 1.f){
                        alpha = math::interpolate::sineEaseIn(vanishingTime.asSeconds(),0.f,255.f, 1.f);
                        tiles[i].setFillColor(sf::Color(255,0,255,(unsigned char)alpha));
                    } else {
                        tiles[i].setFillColor(sf::Color(255,0,255,255));
                    }
                    // Restore position if it was moved away. Original positions are fixed in this setup.
                    // This part is tricky if original positions are not stored. For simplicity, assume they are fixed
                    // and tiles[i].getPosition() from init is okay IF it wasn't moved.
                    // A better way: store original positions. Here, let's assume the tile's position is its target.
                    if (bodies[i].m_position.x < -9000.f) { // If it was "disabled"
                        // Find original position for bodies[i] - this needs careful setup
                        // For now, using the tile's current position, assuming tile wasn't moved.
                        // This is problematic. Need to store original positions.
                        // Quick fix: hardcode original positions again or retrieve from tile if tile wasn't moved.
                        // Example: bodies[20] original pos: sf::Vector2f(192.f, -32.f);
                        // This part of logic needs to be robust if platforms can move AND vanish.
                        // The current code uses tiles[i].getPosition() which is fine if tile IS the reference.
                         bodies[i].m_position = tiles[i].getPosition();
                    }
                 }
            }
            

            // --- Player Logic ---
            // Jump logic
            if (collisionBottom) { // If player is on ground
                jumpCount = 0; // Allow new jump press to be counted
            }
            
            if(jumped && jumpCount > 0 && jumpTime.asSeconds() < 0.4f && collisionBottom) { // Initiate jump from ground
                canJump = true;
                playerBody.m_velocity.y = -10.f; // Initial jump velocity (adjust as needed)
                jumpTime = sf::Time::Zero; // Reset jump timer for this jump's duration/control
                jumpCount = 0; // Consume the jump press count
            } else if (jumped && canJump && jumpTime.asSeconds() < 0.2f) { // Variable jump height control
                 playerBody.m_velocity.y = -10.f; // Maintain upward force
            } else {
                canJump = false; // Stop applying jump force if button released or time limit exceeded
            }
            jumpTime += TimePerFrame;


            // Horizontal movement
            float moveSpeed = 200.f; // Base speed for player
            if ((speed.x > 0.f && !collisionRight) || (speed.x < 0.f && !collisionLeft)){
                 playerBody.m_velocity.x = (speed.x / 100.f) * moveSpeed * turbo;
            } else {
                 playerBody.m_velocity.x = 0.f;
            }
            
            // Apply Gravity
            if (!collisionBottom) {
                if (GRAVITY < 20.f) GRAVITY += 0.8f; // Increase gravity effect (acceleration)
                else GRAVITY = 20.f;          // Terminal velocity for gravity based pull
                playerBody.m_velocity.y += GRAVITY * TimePerFrame.asSeconds() * 20.f; // Scale gravity effect
            } else {
                GRAVITY = 0.f; // Reset gravity effect when on ground
                playerBody.m_velocity.y = 0.f; // Ensure no downward drift if on ground
            }

            // Update position from velocity
            playerBody.m_position.x += playerBody.m_velocity.x * TimePerFrame.asSeconds();
            playerBody.m_position.y += playerBody.m_velocity.y * TimePerFrame.asSeconds();


            // --- Collision Resolution ---
            // The main loop's collision response logic was complex.
            // The phys::collisionSystem::resolveCollisions should handle position correction and flag setting.
            // Then main uses these flags.

            collisionSys.setCollisionInfo(false, false, false, false); // Reset flags for collisionSys before resolving
            for(int k=0; k<3; ++k) { // Multiple iterations can help stabilize collision
                collisionSys.resolveCollisions(&playerBody, bodies, NUM_PLATFORM_OBJECTS);
            }

            // Update local flags from collision system
            collisionTop = collisionSys.getCollisionInfo().m_collisionTop;
            collisionBottom = collisionSys.getCollisionInfo().m_collisionBottom;
            collisionLeft = collisionSys.getCollisionInfo().m_collisionLeft;
            collisionRight = collisionSys.getCollisionInfo().m_collisionRight;
            type = collisionSys.getBodyInfo().m_type; // Platform type player is interacting with

            // --- Post-collision adjustments based on platform type ---
            if (collisionBottom) {
                 playerBody.m_velocity.y = 0; // Stop vertical movement if on ground
                 GRAVITY = 0.f;
                 jumpTime = sf::Time::Zero; // Allow jump timer to reset for canJump logic
                 canJump = false; // Player has landed

                if (type == phys::bodyType::conveyorBelt) {
                    // Conveyor effect: apply surface velocity.
                    // The collision system has already placed the player on the belt.
                    // Add conveyor's surface velocity. Note: bodies[11] is hardcoded.
                    // A more general approach: collisionSys.getBodyInfo().m_surfaceVelocity
                    if (collisionSys.getBodyInfo().m_id == 11) { // Check if it's the specific conveyor
                        playerBody.m_position.x += bodies[11].m_surfaceVelocity * TimePerFrame.asSeconds();
                    }
                } else if (type == phys::bodyType::moving) {
                     // Moving platform effect: player moves with platform
                     // platformVelocity is the visual platform's speed. Player should move with it.
                     // This requires careful handling if player also has input.
                     // The simplest is to add platform's displacement to player.
                     // The playerBody was already adjusted by platformVelocity earlier in input handling.
                     // This line might be redundant or need refinement depending on how platformVelocity is handled.
                     // If platformVelocity is displacement per frame:
                     playerBody.m_position.x += platformVelocity; // platformVelocity is already scaled by TimePerFrame
                }
            }
            
            // Debug output
			if(debug == 1 && (collisionTop || collisionBottom || collisionLeft || collisionRight)){
                std::cout<<"Collision (T/B/L/R): "
                         <<collisionTop<<"/"<<collisionBottom<<"/"<<collisionLeft<<"/"<<collisionRight
                         << " | Type: " << static_cast<int>(type)
                         << " | ID: " << collisionSys.getBodyInfo().m_id
                         << " | Player Y Vel: " << playerBody.m_velocity.y
                         << " | GRAVITY: " << GRAVITY << std::endl;
			}

			player.setPosition(playerBody.m_position); // Sync visual player with physics body
		}
        if (!running) break; 


        // Update view / camera
		mainView.setCenter(player.getPosition()); // Center view on player

		// --- Drawing ---
		window.clear(sf::Color::Black); // Clear with a color
		window.setView(mainView);

		for (const auto& tile_obj : tiles){ // Iterate over const references
            if (tile_obj.getFillColor().a > 0) // Only draw visible tiles
			    window.draw(tile_obj);
		}
		window.draw(player);
		window.display();
	}

	return 0;
}

/*#include <iostream>
#include <SFML/Graphics.hpp>
#include "../include/CollisionSystem.hpp"
#include "../include/dynamicBody.hpp"
#include "../include/platformBody.hpp"
#include "../include/tile.hpp"
#include "../include/interpolate.hpp"
#include <vector>

int main(void){

    const sf::Vector2f tileSize = sf::Vector2f(32, 32);
    const int NUM_PLATFORM_OBJECTS = 24;

    float GRAVITY = 0.f;//variable gravity

	sf::RenderWindow window(sf::VideoMode(800, 600), "Collision Detection", sf::Style::Default);
	window.setKeyRepeatEnabled(false);
	window.setVerticalSyncEnabled(true);
	window.setFramerateLimit(60);
	sf::Event e;

	//set up the view
	sf::View mainView(sf::FloatRect(0.f, 0.f, 1200.f, 800.f));
	window.setView(mainView);

	//instantiate the collision system object
	phys::collisionSystem collisionSys;

	//set up some platforms - all normal platforms by default
	phys::platformBody bodies[NUM_PLATFORM_OBJECTS];
	for (int i = 0; i < NUM_PLATFORM_OBJECTS;i++){
        bodies[i].m_id = i;//assign each platform an id
		bodies[i].m_width = tileSize.x*4;
        bodies[i].m_height = tileSize.y;
        bodies[i].m_type = phys::bodyType::platform;
	}

	//place them on the screen somewhere...
	bodies[0].m_position = sf::Vector2f(0.f,window.getSize().y-32.f);
	bodies[1].m_position = sf::Vector2f(0.f, 356.f);
	bodies[2].m_position = sf::Vector2f(480.f, window.getSize().y - 192.f);
	bodies[3].m_position = sf::Vector2f(0.f, 96.f);
	bodies[4].m_position = sf::Vector2f(640.f, 224.f);
	bodies[5].m_position = sf::Vector2f(480.f, window.getSize().y - 32.f);
	bodies[6].m_position = sf::Vector2f(672.f, window.getSize().y - 260.f);
	bodies[7].m_position = sf::Vector2f(96.f, window.getSize().y - 128.f);
	bodies[8].m_position = sf::Vector2f(640, window.getSize().y - 160.f);
	bodies[9].m_position = sf::Vector2f(640.f, 64.f);
    bodies[10].m_position = sf::Vector2f(292, window.getSize().y - 128.f);

	//conveyor belt
	bodies[11].m_position = sf::Vector2f(window.getSize().x / 4.f, window.getSize().y / 2.f);
    bodies[11].m_width = tileSize.x*10;
    bodies[11].m_height = tileSize.y;
    bodies[11].m_type = phys::bodyType::conveyorBelt;
    bodies[11].m_surfaceVelocity = 12.f;

	//moving platform
	bodies[12].m_height = tileSize.y/8;
	bodies[12].m_width = tileSize.x*5;
	bodies[12].m_position = sf::Vector2f(window.getSize().x/4,window.getSize().y/4);
	bodies[12].m_type = phys::bodyType::moving;
	int platformDir = 1;
	float platformVelocity = 0.f;

	//jump through platforms
	bodies[13].m_width = tileSize.x;
	bodies[13].m_height = tileSize.y/8.f;
	bodies[13].m_position = sf::Vector2f(160.f,window.getSize().y - 224.f);
	bodies[13].m_type = phys::bodyType::jumpthrough;

	bodies[14].m_width = tileSize.x;
	bodies[14].m_height = tileSize.y/8.f;
	bodies[14].m_position = sf::Vector2f(576, window.getSize().y - 288.f);
	bodies[14].m_type = phys::bodyType::jumpthrough;

    //falling platforms
	bodies[15].m_width = tileSize.x;
	bodies[15].m_height = tileSize.y;
	bodies[15].m_position = sf::Vector2f(192, 64.f);
	bodies[15].m_type = phys::bodyType::falling;

	bodies[16].m_width = tileSize.x;
	bodies[16].m_height = tileSize.y;
	bodies[16].m_position = sf::Vector2f(256.f, 64.f);
	bodies[16].m_type = phys::bodyType::falling;

	bodies[17].m_width = tileSize.x;
	bodies[17].m_height = tileSize.y;
	bodies[17].m_position = sf::Vector2f(320.f, 64.f);
	bodies[17].m_type = phys::bodyType::falling;

	bodies[18].m_width = tileSize.x;
	bodies[18].m_height = tileSize.y;
	bodies[18].m_position = sf::Vector2f(384.f, 64.f);
	bodies[18].m_type = phys::bodyType::falling;

	bodies[19].m_width = tileSize.x;
	bodies[19].m_height = tileSize.y;
	bodies[19].m_position = sf::Vector2f(448.f, 64.f);
	bodies[19].m_type = phys::bodyType::falling;

    //vanishing platforms
	bodies[20].m_width = tileSize.x;
	bodies[20].m_height = tileSize.y;
	bodies[20].m_position = sf::Vector2f(192.f, -32.f);
	bodies[20].m_type = phys::bodyType::vanishing;

	bodies[21].m_width = tileSize.x;
	bodies[21].m_height = tileSize.y;
	bodies[21].m_position = sf::Vector2f(256.f, -32.f);
	bodies[21].m_type = phys::bodyType::vanishing;

	bodies[22].m_width = tileSize.x;
	bodies[22].m_height = tileSize.y;
	bodies[22].m_position = sf::Vector2f(320.f, -32.f);
	bodies[22].m_type = phys::bodyType::vanishing;

	bodies[23].m_width = tileSize.x;
	bodies[23].m_height = tileSize.y;
	bodies[23].m_position = sf::Vector2f(384.f, -32.f);
	bodies[23].m_type = phys::bodyType::vanishing;


	tile tiles[NUM_PLATFORM_OBJECTS];
	for (int i = 0; i < NUM_PLATFORM_OBJECTS; ++i){
		tiles[i].setPosition(bodies[i].m_position);
		tiles[i].setSize(sf::Vector2f(bodies[i].m_width, bodies[i].m_height));
		switch(bodies[i].m_type){
            case phys::bodyType::platform:
            tiles[i].setFillColor(sf::Color(255, 0, 0, 255));
            break;
            case phys::bodyType::conveyorBelt:
            tiles[i].setFillColor(sf::Color(255, 100, 0, 255));
            break;
            case phys::bodyType::moving:
            tiles[i].setFillColor(sf::Color(0, 255, 0, 255));
            break;
            case phys::bodyType::jumpthrough:
            tiles[i].setFillColor(sf::Color(0, 255, 255, 255));
            break;
            case phys::bodyType::falling:
            tiles[i].setFillColor(sf::Color(255, 255, 0, 255));
            break;
            case phys::bodyType::vanishing:
            tiles[i].setFillColor(sf::Color(255, 0, 255, 255));
            break;

            default:
                break;
		}
	}

	//set up for the player
	phys::dynamicBody playerBody;
	playerBody.m_position = sf::Vector2f(window.getSize().x/2.f, window.getSize().y/2.f);
	playerBody.m_width = tileSize.x;
	playerBody.m_height = tileSize.y;
	playerBody.m_velocity = sf::Vector2f(0,0);
	playerBody.m_lastPosition = sf::Vector2f(window.getSize().x/2.f, window.getSize().y/2.f);
	playerBody.m_moveX = 0;
	playerBody.m_moveY = 0;

	sf::RectangleShape player;
	player.setFillColor(sf::Color(255, 255, 255, 255));
	player.setSize(sf::Vector2f(tileSize.x, tileSize.y));
	player.setPosition(playerBody.m_position);

	//time variables
	sf::Clock tickClock;
	sf::Time timeSinceLastUpdate = sf::Time::Zero;
	sf::Time duration = sf::Time::Zero;
	sf::Time TimePerFrame = sf::seconds(1.f / 60.f);
	sf::Time jumpTime = sf::Time::Zero;
	sf::Time vanishingTime = sf::Time::Zero;
	int oddEven = 1;
	float alpha = 0.f;
	sf::Time springTime = sf::Time::Zero;
	bool springPeak = false;//peak is 80.f on the y-axis
	bool canSpring = false;
	bool springSprung = false;//indicator that the spring was used

	//get information about the joystick (available in SFML 2.2 - comment out if using older version)
	sf::Joystick::Identification id = sf::Joystick::getIdentification(0);
	std::cout << "\nVendor ID: " << id.vendorId << "\nProduct ID: " << id.productId << std::endl;
	sf::String controller("Joystick Use: " + id.name);
	window.setTitle(controller);//easily tells us what controller is connected

	//query joystick for settings if it's plugged in...
	if (sf::Joystick::isConnected(0)){
		// check how many buttons joystick number 0 has
		unsigned int buttonCount = sf::Joystick::getButtonCount(0);

		// check if joystick number 0 has a Z axis
		bool hasZ = sf::Joystick::hasAxis(0, sf::Joystick::Z);

		std::cout << "Button count: " << buttonCount << std::endl;
        std::cout << "Has a z-axis: " << hasZ << std::endl;
	}

	int turbo = 1;//indicate whether player is using button for turbo speed ;)

	//for movement
	sf::Vector2f speed = sf::Vector2f(0.f,0.f);

    //collision info for insight on handling player movement
	bool intersection = false;
	unsigned int type = phys::bodyType::none;
	bool collisionTop = false;
	bool collisionBottom = false;
	bool collisionLeft = false;
	bool collisionRight = false;
	bool canJump = false;
	bool jumped = false;
	unsigned int jumpCount = 0;

	//for debug info
	int debug = -1;

	bool running = true;
	while (running){
		while (window.pollEvent(e)){
			if (e.type == sf::Event::Closed){
				window.close();
				return 0;
			}

			if (e.type == sf::Event::KeyPressed){
				switch (e.key.code){
				case sf::Keyboard::Escape:
				{
					window.close();
					return 0;
				}
					break;
                case sf::Keyboard::D:
				{
					debug *= -1;
				}
					break;
				default:
					break;
				}
			}
		}

		sf::Time elapsedTime = tickClock.restart();
		timeSinceLastUpdate += elapsedTime;
		while (timeSinceLastUpdate > TimePerFrame)
		{
			timeSinceLastUpdate -= TimePerFrame;

            //////////////////////////////////////
            //get joystick input inside fixed
            //time step - pollEvent() seems to be
            //missing joystick events for some
            //reason (sure it's my fault!)
            //////////////////////////////////////

            //check state of joystick
            speed = sf::Vector2f(sf::Joystick::getAxisPosition(0, sf::Joystick::X), sf::Joystick::getAxisPosition(0, sf::Joystick::Y));

			if (sf::Joystick::isButtonPressed(0, 2)){//"X" button on the XBox 360 controller
				turbo = 2;
			}

			if (!sf::Joystick::isButtonPressed(0, 2)){
				turbo = 1;
			}

			if(sf::Joystick::isButtonPressed(0,0)){//"A" button on the XBox 360 controller
                jumpCount++;
                jumped = true;
			}

			if(!sf::Joystick::isButtonPressed(0,0) && jumpCount > 1){
                jumped = false;
                jumpCount = 0;
			}

			if(sf::Joystick::isButtonPressed(0,1)){//"B" button on the XBox 360 controller
				window.close();
				return 0;
			}

			//accumulator for moving platform
			if(duration.asSeconds() >= 4.f){
                platformDir *= -1;
                duration = sf::Time::Zero;
			}
            else
                duration += TimePerFrame;

            //moving platform updates
			if(duration.asSeconds() <= 3.f)
                platformVelocity = TimePerFrame.asSeconds()*(platformDir * 700.f * math::interpolate::quadraticEaseInOut(duration.asSeconds(), 0.f, 1.f, 3.f));
            else
                platformVelocity = 0.f;

            //update the position of the moving platforms (bodies and geometry)
            //this needs to occur before player updates
			bodies[12].m_position.x += platformVelocity;

            tiles[12].setPosition(bodies[12].m_position);

            //find bodies that are falling and handle them
            //while simultaneously setting the fall flag for sprite
            for(int i=0; i<NUM_PLATFORM_OBJECTS; ++i){
                //establish what a valid collision is for falling platform
                bool collided = bodies[i].m_falling;

                //conditions for falling platforms to fall
                //based on a half second delay

                if(collided && tiles[i].m_fallTime.asSeconds() > .5f){
                    tiles[i].m_fallTime = sf::Time::Zero;
                }else{
                    tiles[i].m_fallTime += TimePerFrame;
                }

                if(collided && tiles[i].m_fallTime.asSeconds() > .5f){
                    tiles[i].m_falling = true;
                }
            }

            //update the position of the falling platform sprite and move the bodies off-screen
            for(int i=0; i<NUM_PLATFORM_OBJECTS; ++i){
                if(tiles[i].m_falling && bodies[i].m_falling){
                    bodies[i].m_position = sf::Vector2f(-9999,0);
                    tiles[i].move(0.f, TimePerFrame.asSeconds()*1000.f);
                }
            }

            //compute time and resets for vanishing platforms
            if(vanishingTime.asSeconds() > 2.f){
                vanishingTime = sf::Time::Zero;
                oddEven *= -1;//occilate between odd and even platforms
                //std::cout<<"oddEven: "<<oddEven<<std::endl;
            }else{
                vanishingTime += TimePerFrame;
            }

            //make platforms vanish and reappear
            //this applies to platforms in the index [20,23]
            if(oddEven == 1){
                for(int i=20; i<24; ++i){
                    if(i%2 == 0){//even numbered platforms vanish...
                        if(vanishingTime.asSeconds() < 1.f){
                            alpha = math::interpolate::sineEaseIn(vanishingTime.asSeconds(),0.f,255.f, 1.f);
                            tiles[i].setFillColor(sf::Color(255,0,255,255-(unsigned int)alpha));
                        }
                        else{
                            tiles[i].setFillColor(sf::Color(255,0,255,0));
                        }
                        if(tiles[i].getFillColor().a <= 0)//friendlier to the player to wait until the platform completely disappears before moving the physics body off-screen
                            bodies[i].m_position = sf::Vector2f(-9999,0);
                    }else{//odd numbered platforms reappear...
                        if(vanishingTime.asSeconds() < 1.f){
                            alpha = math::interpolate::sineEaseIn(vanishingTime.asSeconds(),0.f,255.f, 1.f);
                            tiles[i].setFillColor(sf::Color(255,0,255,(unsigned int)alpha));
                        }
                        else{
                            tiles[i].setFillColor(sf::Color(255,0,255,255));
                        }
                        bodies[i].m_position = tiles[i].getPosition();
                    }
                }
            }else{
                for(int i=20; i<24; ++i){
                    if(i%2 == 0){//even numbered platforms reappear...
                        if(vanishingTime.asSeconds() < 1.f){
                            alpha = math::interpolate::sineEaseIn(vanishingTime.asSeconds(),0.f,255.f, 1.f);
                            tiles[i].setFillColor(sf::Color(255,0,255,(unsigned int)alpha));
                        }
                        else{
                            tiles[i].setFillColor(sf::Color(255,0,255,255));
                        }
                        bodies[i].m_position = tiles[i].getPosition();
                    }else{//odd numbered platforms vanish...
                        if(vanishingTime.asSeconds() < 1.f){
                            alpha = math::interpolate::sineEaseIn(vanishingTime.asSeconds(),0.f,255.f, 1.f);
                            tiles[i].setFillColor(sf::Color(255,0,255,255-(unsigned int)alpha));
                        }
                        else{
                            tiles[i].setFillColor(sf::Color(255,0,255,0));
                        }
                        if(tiles[i].getFillColor().a <= 0)//friendlier to the player to wait until the platform completely disappears before moving the physics body off-screen
                            bodies[i].m_position = sf::Vector2f(-9999,0);
                    }
                }
            }

            //check for springboard activity and handle the motion of the springboard


            //do "can jump" routine
            //when the jump count is greater than 1
            //the timer resets
            if(jumpCount > 1){
                jumpTime = sf::Time::Zero;
            }
            else
                jumpTime += TimePerFrame;

            //if jump is pressed
            //and the timer is less than .5 seconds
            //then the player can jump
            canJump = jumped && jumpTime.asSeconds() < 0.4f ? true : false;

            //do updates

            //get intersection information
            intersection = playerBody.m_position.y + playerBody.m_height >= collisionSys.getBodyInfo().m_position.y - 2.f
				&& playerBody.m_position.y + playerBody.m_height <= collisionSys.getBodyInfo().m_position.y + collisionSys.getBodyInfo().m_height
				&& playerBody.m_position.x + playerBody.m_width >= collisionSys.getBodyInfo().m_position.x
				&& playerBody.m_position.x + playerBody.m_width <= collisionSys.getBodyInfo().m_position.x + collisionSys.getBodyInfo().m_width + playerBody.m_width;

            //get platform type information
            type = collisionSys.getBodyInfo().m_type;

            //debug info
			if(intersection && debug == 1){
                std::cout<<"Top Collision: "<<collisionSys.getCollisionInfo().m_collisionTop<<std::endl;
                std::cout<<"Bottom Collision: "<<collisionSys.getCollisionInfo().m_collisionBottom<<std::endl;
                std::cout<<"Left Collision: "<<collisionSys.getCollisionInfo().m_collisionLeft<<std::endl;
                std::cout<<"Right Collision: "<<collisionSys.getCollisionInfo().m_collisionRight<<std::endl;
                std::cout<<"type: "<<collisionSys.getBodyInfo().m_type<<std::endl;
                std::cout<<"id: "<<collisionSys.getBodyInfo().m_id<<std::endl;
                std::cout<<"Gravity: "<<GRAVITY<<std::endl;
			}

            //get collision information
            collisionTop = collisionSys.getCollisionInfo().m_collisionTop;
            collisionBottom = collisionSys.getCollisionInfo().m_collisionBottom;
            collisionLeft = collisionSys.getCollisionInfo().m_collisionLeft;
            collisionRight = collisionSys.getCollisionInfo().m_collisionRight;

            //update the position of the square according to input from joystick
            //CHECK DEAD ZONES - OTHERWISE INPUT WILL RESULT IN JITTERY MOVEMENTS WHEN NO INPUT IS PROVIDED
            //INPUT RANGES FROM -100 TO 100
            if ((speed.x > 15.f && !collisionLeft) || (speed.x < -15.f && !collisionRight)){
                playerBody.m_position.x += turbo*speed.x*TimePerFrame.asSeconds();
            }
            else
                speed.x = 0.f;

            if(canJump){
                if(jumpTime.asSeconds() < .2f && !collisionTop){
                    playerBody.m_position.y += -800.f*TimePerFrame.asSeconds() + GRAVITY;
                }

                else if(jumpTime.asSeconds() > .2f && !collisionTop){
                    playerBody.m_position.y += GRAVITY - 13.9;//black magic...makes jumps parabolic instead of triangular
                }
            }

            if(collisionBottom){//reduces stickiness
                playerBody.m_position.y = playerBody.m_lastPosition.y;
                playerBody.m_position.x += turbo*speed.x*TimePerFrame.asSeconds();
            }

            //check for contact with static platforms
            if(type == phys::bodyType::platform && collisionBottom){
                GRAVITY = 0.f;
                playerBody.m_position.x += turbo*speed.x*TimePerFrame.asSeconds();
            }

            //sticky ceilings problem solved
            else if(collisionTop && (!collisionLeft || !collisionRight)){
                playerBody.m_position.y = playerBody.m_lastPosition.y + .5f;
                playerBody.m_position.x += turbo*speed.x*TimePerFrame.asSeconds();
            }

            //sticky walls problem solved
            else if(!collisionBottom && (collisionLeft || collisionRight)){
                playerBody.m_position.y += GRAVITY;
                playerBody.m_position.x = playerBody.m_lastPosition.x;
            }

            //check for contact with the jump-through platforms
			else if(type == phys::bodyType::jumpthrough && collisionBottom){
                GRAVITY = 0.f;
                playerBody.m_position.x += turbo*speed.x*TimePerFrame.asSeconds();
			}

            //check for contact with the conveyor belt
			else if (type == phys::bodyType::conveyorBelt && collisionBottom){
                GRAVITY = 0.f;
                playerBody.m_position.x += turbo*speed.x*TimePerFrame.asSeconds() - bodies[11].m_surfaceVelocity;
			}

			//check for contact with the moving platform
			else if(type == phys::bodyType::moving  && collisionBottom){
                    GRAVITY = 0.f;
                    playerBody.m_position.x += turbo*speed.x*TimePerFrame.asSeconds() + (2.f*platformVelocity);
			}

			//check for contact with the falling platform
			else if(collisionBottom && intersection && type == phys::bodyType::falling){
                    bodies[collisionSys.getBodyInfo().m_id].m_falling = true;
                    GRAVITY = 0.f;
                    playerBody.m_position.x += turbo*speed.x*TimePerFrame.asSeconds();
			}

			//check for contact with the vanishing platforms
			else if(type == phys::bodyType::vanishing && collisionBottom){
                    GRAVITY = 0.f;
                    playerBody.m_position.x += turbo*speed.x*TimePerFrame.asSeconds();
			}

            else{
                if(GRAVITY < 20.f)
                    GRAVITY += .4f;
                else
                    GRAVITY = 20.f;
                playerBody.m_position.y += GRAVITY;
                playerBody.m_position.x += turbo*speed.x*TimePerFrame.asSeconds();
            }

            collisionSys.setCollisionInfo(false, false, false, false);//reset collision info every frame

            //handle collisions - 1 call works, but 3 iterations seems to work well and reduce jittering
            for(int i=0; i<3; ++i)
                collisionSys.resolveCollisions(&playerBody, bodies, NUM_PLATFORM_OBJECTS);

            playerBody.m_lastPosition = playerBody.m_position;

			player.setPosition(playerBody.m_position);//attach player geometry to player body
		}

        //good enough for now - but will make you dizzy!!!
		mainView.setCenter(player.getPosition());

		window.clear();

		window.setView(mainView);

		for (auto i : tiles){
			window.draw(i);
		}

		window.draw(player);

		window.display();
	}

	return 0;
}*/