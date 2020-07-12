#ifndef CONSTANT_TIME_EXEC_HPP_INCLUDED
#define CONSTANT_TIME_EXEC_HPP_INCLUDED

#include <stdint.h>

struct constant_time_exec
{
    uint64_t last_time_ms = 0;

    uint64_t max_cycles = 0;
    uint64_t current_cycles = 0;

    uint64_t cycles_per_s = 1000;

    void init(uint64_t _max_cycles, uint64_t current_time_ms)
    {
        current_cycles = 0;
        max_cycles = _max_cycles;
        last_time_ms = current_time_ms;
    }

    ///executes a function repeatedly for however many cycles have elapsed
    template<typename T>
    void exec_until(uint64_t current_time_ms, T&& func)
    {
        if(max_cycles == current_cycles)
            return;

        uint64_t diff_ms = current_time_ms - last_time_ms;

        uint64_t cycles_to_exec = diff_ms * (double)cycles_per_s / 1000.;

        uint64_t simulated_next_time_ms = last_time_ms + cycles_to_exec * (double)cycles_per_s / 1000.;

        uint64_t start_time_ms = last_time_ms;

        last_time_ms = simulated_next_time_ms;

        if((cycles_to_exec + current_cycles) > max_cycles)
        {
            cycles_to_exec = max_cycles - current_cycles;
        }

        current_cycles += cycles_to_exec;

        assert(current_cycles <= max_cycles);

        for(uint64_t i = 0; i < cycles_to_exec; i++)
        {
            double exec_frac = (double)i / cycles_to_exec;

            double exec_diff_ms = (last_time_ms - start_time_ms) * exec_frac;

            uint64_t real_time_ms = start_time_ms + exec_diff_ms;

            func(i, real_time_ms);
        }

        //return cycles_to_exec;
    }
};

#endif // CONSTANT_TIME_EXEC_HPP_INCLUDED
