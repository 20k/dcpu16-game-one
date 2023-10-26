#ifndef HARDWARE_BAD_GYRO_HPP_INCLUDED
#define HARDWARE_BAD_GYRO_HPP_INCLUDED

#include <dcpu16-sim/base_sim.hpp>
#include <dcpu16-sim/base_hardware.hpp>

struct hardware_bad_gyro : dcpu::sim::hardware
{
    std::vector<uint16_t> sequence;
    uint32_t sequence_counter = 0;

    bool on = false;

    uint64_t time_started_ms = 0;
    uint64_t last_tick_time_ms = 0;
    uint16_t tick_divisor = 0;

    uint16_t interrupt_message = 0;

    hardware_bad_gyro(const std::vector<uint16_t>& seq) : sequence(seq)
    {
        manufacturer_id = 0x6E617361;
        hardware_id = 0x7370696E;
        hardware_version = 0;
    }

    virtual void interrupt2(std::span<dcpu::sim::hardware*> all, dcpu::sim::world_base* state, dcpu::sim::CPU& c) override
    {
        dcpu::sim::time_state* tstate = dynamic_cast<dcpu::sim::time_state*>(state);

        if(tstate == nullptr)
            return;

        if(c.regs[A_REG] == 0)
        {
            if(c.regs[B_REG] == 0)
            {
                on = false;
                interrupt_message = 0; ///?
            }
            else
            {
                on = true;
                tick_divisor = c.regs[B_REG];
                time_started_ms = tstate->time_ms;
                last_tick_time_ms = time_started_ms;
            }
        }

        if(!on)
            return;

        if(c.regs[A_REG] == 1)
        {
            ///deliberate bug
            c.regs[C_REG] = sequence[(sequence_counter++) % (uint32_t)sequence.size()];
        }

        if(c.regs[A_REG] == 2)
        {
            interrupt_message = c.regs[B_REG];
        }
    }

    constexpr virtual void step(dcpu::sim::world_base* state, dcpu::sim::CPU& c)
    {
        dcpu::sim::time_state* tstate = dynamic_cast<dcpu::sim::time_state*>(state);

        if(tstate == nullptr)
            return;

        if(!on)
            return;

        if(interrupt_message == 0)
            return;

        uint64_t diff_ms = tstate->time_ms - last_tick_time_ms;

        uint16_t tick_count = ((double)diff_ms) / ((1000. * (uint64_t)tick_divisor) / 60.);

        if(tick_count > 0)
        {
            last_tick_time_ms = (uint64_t)(tick_count * ((1000. * (uint64_t)tick_divisor) / 60.)) + last_tick_time_ms;

            dcpu::interrupt_type type;
            type.is_software = 0;
            type.message = interrupt_message;

            c.add_interrupt(type);
        }
    }

    constexpr virtual void reset() override
    {
        time_started_ms = 0;
        last_tick_time_ms = 0;
        tick_divisor = 0;
        on = false;
        interrupt_message = 0;
        sequence_counter = 0;
    }

    virtual hardware* clone() override
    {
        return new hardware_bad_gyro(*this);
    }
};

#endif // HARDWARE_BAD_GYRO_HPP_INCLUDED
