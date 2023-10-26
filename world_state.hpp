#ifndef WORLD_STATE_HPP_INCLUDED
#define WORLD_STATE_HPP_INCLUDED

#include "ship.hpp"
#include <dcpu16-sim/base_hardware.hpp>

struct world_state : dcpu::sim::time_state, dcpu::sim::world_base
{
    ship player;
};

void step_world(world_state& st);

#endif // WORLD_STATE_HPP_INCLUDED
