#ifndef LEVEL_HARDWARE_HPP_INCLUDED
#define LEVEL_HARDWARE_HPP_INCLUDED

#include <dcpu16-sim/base_sim.hpp>
#include <dcpu16-ide/base_ide.hpp>
#include <dcpu16-sim/base_hardware.hpp>
#include <dcpu16-sim/all_hardware.hpp>

struct level_hardware
{
    std::map<int, dcpu::sim::CPU> input_cpus;
    std::map<int, dcpu::sim::CPU> output_cpus;
    std::map<int, stack_vector<uint16_t, MEM_SIZE>> input_translation;
    std::map<int, stack_vector<uint16_t, MEM_SIZE>> output_translation;
    std::vector<dcpu::sim::hardware*> hardware;

    dcpu::sim::fabric fab;

    int cycle = 0;
};

#endif // LEVEL_HARDWARE_HPP_INCLUDED
