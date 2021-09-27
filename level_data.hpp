#ifndef LEVEL_DATA_HPP_INCLUDED
#define LEVEL_DATA_HPP_INCLUDED

#include <optional>
#include <string>
#include <map>
#include <dcpu16-sim/base_sim.hpp>
#include <dcpu16-ide/base_ide.hpp>
#include <dcpu16-sim/base_hardware.hpp>
#include <dcpu16-sim/all_hardware.hpp>
#include "world_state.hpp"
#include "constant_time_exec.hpp"

struct level_data
{
    int cpus = 1;

    std::string name;
    std::string description;
    std::string short_description;

    std::optional<std::string> io_program;
    std::optional<std::string> dynamic_validation_program;
    std::vector<std::string> hardware_names;
};

struct level_runtime_parameters
{
    std::map<int, std::vector<uint16_t>> channel_to_input;
    std::map<int, std::vector<uint16_t>> channel_to_output;
    std::map<int, std::map<int, std::vector<int>>> output_to_input_start;

    std::optional<dcpu::sim::CPU> io_cpu;
    std::optional<dcpu::sim::CPU> dynamic_validation_cpu;
    std::vector<dcpu::sim::hardware*> hardware;
};

struct assembler_state
{
    std::vector<int> error_locs;
    std::vector<int> error_channels;
    bool has_error = false;
};

struct level_runtime_data
{
    world_state real_world_state;

    std::optional<dcpu::sim::CPU> dynamic_validation_cpu;
    std::vector<dcpu::sim::hardware*> hardware;

    dcpu::sim::fabric fab;
    constant_time_exec exec;

    std::map<int, std::vector<uint16_t>> found_output;
};

struct all_level_data
{
    void load(const std::string& folder = "./levels");
};

#endif // LEVEL_DATA_HPP_INCLUDED
