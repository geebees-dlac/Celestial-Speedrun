// PhysicsTypes.hpp (or whatever you name it)
#ifndef PHYSICS_TYPES_HPP
#define PHYSICS_TYPES_HPP

namespace phys {
    enum class bodyType {
        none,
        platform,
        conveyorBelt,
        moving,
        jumpthrough,
        falling,
        vanishing,
        spring
        // add other types as needed
    };

    // You could also put other shared physics enums or simple structs here
} // namespace phys

#endif // PHYSICS_TYPES_HPP