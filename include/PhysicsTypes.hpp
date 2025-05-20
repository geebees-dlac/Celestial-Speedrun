// PhysicsTypes.hpp
#ifndef PHYSICS_TYPES_HPP
#define PHYSICS_TYPES_HPP

namespace phys {
    enum class bodyType {
        none = 0,
        platform = 1,       
        conveyorBelt = 2,
        moving = 3,
        // gian the id 4 is not missing, i have a plan for it later, remind me later...
        falling = 5,
        vanishing = 6,
        spring = 7,    
        trap = 8,      //spikes ni siya wait or anything that kills the player
        solid = 9,     
        goal = 10      // Added for level exit
    };
}
#endif