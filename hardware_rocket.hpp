#ifndef HARDWARE_ROCKET_HPP_INCLUDED
#define HARDWARE_ROCKET_HPP_INCLUDED

#include <dcpu16-sim/base_sim.hpp>
#include <dcpu16-sim/base_hardware.hpp>

struct hardware_rocket : dcpu::sim::hardware
{
    bool on = false;
    double propellant = 0;
    double propellant_ejected_ps = 0;

    hardware_rocket()
    {
        hardware_id = 0x66697265;
        manufacturer_id = 0x6E617361;
    }

    virtual void interrupt2(std::span<dcpu::sim::hardware*> all, dcpu::sim::world_base* state, dcpu::sim::CPU& c) override
    {
        if(c.regs[A_REG] == 0)
        {
            if(c.regs[B_REG] == 0)
            {
                on = false;
            }
            else
            {
                on = true;
            }
        }
    }

    virtual void reset() override
    {
        on = false;
        propellant = 0;
        propellant_ejected_ps = 0;
    }

    virtual hardware* clone() override
    {
        return new hardware_rocket(*this);
    }
};

#endif // HARDWARE_ROCKET_HPP_INCLUDED
