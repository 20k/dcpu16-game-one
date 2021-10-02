#ifndef HARDWARE_RNG_HPP_INCLUDED
#define HARDWARE_RNG_HPP_INCLUDED

#include <dcpu16-sim/base_sim.hpp>
#include <dcpu16-sim/base_hardware.hpp>

struct hardware_rng : dcpu::sim::hardware
{
    uint64_t internal_state = 1;

    uint32_t hardware_id = 0x12345678;
    uint16_t hardware_version = 0;
    uint32_t manufacturer_id = 0x20000;

    constexpr static
    uint16_t next(uint64_t& state)
    {
        uint64_t a = 2862933555777941757;
        uint64_t b = 3037000493;

        state = a * state + b;

        return state >> (64-16);
    }

    constexpr virtual void interrupt(dcpu::sim::world_base* state, dcpu::sim::CPU& c) override
    {
        c.set_location(dcpu::sim::location::reg{X_REG}, next(internal_state));
    }

    constexpr virtual void reset() override
    {
        internal_state = 1;
    }

    constexpr virtual hardware* clone(){return new hardware_rng(*this);}
};

#endif // HARDWARE_RNG_HPP_INCLUDED
