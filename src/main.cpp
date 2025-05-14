#include <iostream>
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
}