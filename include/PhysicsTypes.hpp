// PhysicsTypes.hpp
#ifndef PHYSICS_TYPES_HPP
#define PHYSICS_TYPES_HPP

namespace phys {
    enum class bodyType {
        none, platform, conveyorBelt, moving,
        jumpthrough, falling, vanishing, spring
    };
}
#endif