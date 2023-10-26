#ifndef WORLD_STATE_HPP_INCLUDED
#define WORLD_STATE_HPP_INCLUDED

#include "ship.hpp"

struct world_state : dcpu::sim::time_state, dcpu::sim::world_base
{
    ship player;
};

#endif // WORLD_STATE_HPP_INCLUDED
