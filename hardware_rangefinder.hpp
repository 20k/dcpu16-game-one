#ifndef HARDWARE_RANGEFINDER_HPP_INCLUDED
#define HARDWARE_RANGEFINDER_HPP_INCLUDED

#include <dcpu16-sim/base_sim.hpp>
#include <dcpu16-sim/base_hardware.hpp>

struct hardware_rangefinder : dcpu::sim::hardware
{
    hardware_rangefinder()
    {
        manufacturer_id = 0x6E617361;
        hardware_id = 0x46494E44;
    }

    virtual void interrupt2(std::span<dcpu::sim::hardware*> all, dcpu::sim::world_base* state, dcpu::sim::CPU& c) override
    {
        world_state* st = dynamic_cast<world_state*>(state);

        assert(st);

        if(c.regs[A_REG] == 0)
        {
            double distance_in_m = st->player.position.length();

            double out = 0;

            ///distance in meters
            if(c.regs[B_REG] == 0)
                out = distance_in_m;

            if(c.regs[B_REG] == 1)
                out = distance_in_m / 1000.;

            if(c.regs[B_REG] == 2)
                out = distance_in_m / 1000. / 1000.;

            if(c.regs[B_REG] == 3)
                out = distance_in_m / 1000. / 1000. / 1000.;

            c.regs[C_REG] = clamp(out, 0., 65535.);
        }
    }

    virtual hardware* clone() override
    {
        return new hardware_rangefinder(*this);
    }
};

#endif // HARDWARE_RANGEFINDER_HPP_INCLUDED
