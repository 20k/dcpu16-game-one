#ifndef SHIP_HPP_INCLUDED
#define SHIP_HPP_INCLUDED

#include <vec/vec.hpp>

struct hardware_rocket;

struct ship
{
    vec2f position;
    vec2f velocity;
    float angle = 0;
    float angular_velocity = 0; ///in radians per second
    hardware_rocket* base = nullptr;
};

#endif // SHIP_HPP_INCLUDED
