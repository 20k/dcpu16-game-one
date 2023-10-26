#ifndef HARDWARE_INSPECTOR_HPP_INCLUDED
#define HARDWARE_INSPECTOR_HPP_INCLUDED

#include <dcpu16-sim/base_sim.hpp>
#include <dcpu16-sim/base_hardware.hpp>
#include <dcpu16-sim/hardware_clock.hpp>
#include <dcpu16-sim/hardware_lem1802.hpp>
#include "hardware_gyro.hpp"

#include "world_state.hpp"

struct cpu_proxy : dcpu::sim::hardware
{
    dcpu::sim::CPU* c = nullptr;

    virtual hardware* clone() override {return nullptr;}
};

struct hardware_inspector : dcpu::sim::hardware
{
    hardware_inspector()
    {
        hardware_id = 0x21436587;
        hardware_version = 0;
        manufacturer_id = 0x20000;
    }

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
            if(c.regs[B_REG] == 0)
            {
                c.regs[C_REG] = lem->vram_map;
            }
        }

        if(cpu_proxy* prox = dynamic_cast<cpu_proxy*>(to_check); prox != nullptr)
        {
            c.regs[C_REG] = prox->c->mem[c.regs[B_REG]];
        }

        if(hardware_bad_gyro* gyro = dynamic_cast<hardware_bad_gyro*>(to_check); gyro != nullptr)
        {
            if(c.regs[B_REG] == 0)
                gyro->sequence.push_back(c.regs[C_REG]);
        }
    }

    virtual hardware* clone() override {return new hardware_inspector(*this);}
};

#endif // HARDWARE_INSPECTOR_HPP_INCLUDED
