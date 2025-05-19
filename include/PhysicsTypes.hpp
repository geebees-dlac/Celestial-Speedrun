// PhysicsTypes.hpp
#ifndef PHYSICS_TYPES_HPP
#define PHYSICS_TYPES_HPP
#include <string>
#include <stdexcept>
#include <iostream>

namespace phys {
    enum class bodyType {
        none = 0,
        platform = 1,       
        conveyorBelt = 2,
        moving = 3,
        falling = 5,
        vanishing = 6,
        spring = 7,    
        trap = 8,      //spikes ni siya wait
        solid = 9      //gian the id 4 is not missing, i have a plan for it later, remind me later...
    };
}

phys::bodyType stringToBodyType(const std::string& typeStr);
phys::bodyType stringToBodyType(const std::string& typeStr) {
    if (typeStr == "solid") return phys::bodyType::solid;
    if (typeStr == "platform") return phys::bodyType::platform;
    if (typeStr == "conveyorBelt") return phys::bodyType::conveyorBelt;
    if (typeStr == "moving") return phys::bodyType::moving;
    if (typeStr == "falling") return phys::bodyType::falling;
    if (typeStr == "vanishing") return phys::bodyType::vanishing;
    if (typeStr == "none") return phys::bodyType::none;
    if (typeStr == "spring") return phys::bodyType::spring;
    if (typeStr == "trap") return phys::bodyType::trap;
    throw std::runtime_error("Unknown platform type string: " + typeStr);
    std::cerr << "Warning: Unknown platform type string: " << typeStr << std::endl;
    return phys::bodyType::none;
}
#endif