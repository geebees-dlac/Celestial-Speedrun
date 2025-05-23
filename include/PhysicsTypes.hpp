// PhysicsTypes.hpp
#ifndef PHYSICS_TYPES_HPP
#define PHYSICS_TYPES_HPP

namespace phys {
    enum class bodyType {
        none = 0, //empty space
        platform = 1, //phase through when approached by anywhere but the top
        conveyorBelt = 2, // moving block should make adjustable speeds later
        moving = 3, //moving platform
        interactible = 4, //triggers block to phase in or out (a key of sorts for lvl 4)
        falling = 5, //a block that falls when touched with 0 cooldown
        vanishing = 6, //a block that phases in or out
        spring = 7,  //block that gives 2x the jump  
        trap = 8,   //block that kills the player
        solid = 9,   //standard issue block  
        goal = 10, //block that ends the level
        portal = 11 //block that phases the player to a block with the same id of and body type
    };
}
#endif