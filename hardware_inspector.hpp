#ifndef HARDWARE_INSPECTOR_HPP_INCLUDED
#define HARDWARE_INSPECTOR_HPP_INCLUDED

#include <dcpu16-sim/base_sim.hpp>
#include <dcpu16-sim/base_hardware.hpp>
#include <dcpu16-sim/hardware_clock.hpp>
#include <dcpu16-sim/hardware_lem1802.hpp>

struct hardware_inspector : dcpu::sim::hardware
{
    uint32_t hardware_id = 0x21436587;
    uint16_t hardware_version = 0;
    uint32_t manufacturer_id = 0x20000;

    constexpr virtual void interrupt2(std::span<hardware*> all_hardware, dcpu::sim::world_base* state, dcpu::sim::CPU& c) override
    {
        uint16_t hwidx = c.regs[A_REG];

        if(hwidx >= all_hardware.size())
            return;

        hardware* to_check = all_hardware[hwidx];

        if(dcpu::sim::clock* clk = dynamic_cast<dcpu::sim::clock*>(to_check); clk != nullptr)
        {
            c.regs[B_REG] = clk->on;
        }

        if(dcpu::sim::LEM1802* lem = dynamic_cast<dcpu::sim::LEM1802*>(to_check); lem != nullptr)
        {
            c.regs[B_REG] = lem->vram_map;
        }
    }

    constexpr virtual hardware* clone() override {return new hardware_inspector(*this);}
};

#endif // HARDWARE_INSPECTOR_HPP_INCLUDED
