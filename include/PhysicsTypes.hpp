// PhysicsTypes.hpp
#ifndef PHYSICS_TYPES_HPP
#define PHYSICS_TYPES_HPP

namespace phys {
    enum class bodyType {
        none = 0,
        platform = 1,       
        conveyorBelt = 2,
        moving = 3,
        interactible = 4,
        falling = 5,
        vanishing = 6,
        spring = 7,    
        trap = 8,   
        solid = 9,     
        goal = 10      
    };
}
#endif