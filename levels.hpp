#ifndef LEVELS_HPP_INCLUDED
#define LEVELS_HPP_INCLUDED

#include <vector>
#include <string>
#include <map>
#include <dcpu16-sim/base_sim.hpp>
#include <dcpu16-ide/base_ide.hpp>
#include <dcpu16-sim/base_hardware.hpp>
#include <dcpu16-sim/all_hardware.hpp>
#include "constant_time_exec.hpp"

struct world_context : dcpu::sim::time_state, dcpu::sim::world_base
{

};

struct validation_info
{
    std::map<int, dcpu::sim::CPU> input_cpus;
    std::map<int, dcpu::sim::CPU> output_cpus;
    std::map<int, stack_vector<uint16_t, MEM_SIZE>> input_translation;
    std::map<int, stack_vector<uint16_t, MEM_SIZE>> output_translation;
    std::vector<dcpu::sim::hardware*> hardware;

    dcpu::sim::fabric fab;

    int cycle = 0;
};

struct level_context
{
    //std::vector<dcpu::ide::editor> cpus;
    bool successful_validation = false;
    bool finished = false;
    int cpus = 0;
    std::string level_name;
    std::string description;

    std::map<int, std::vector<uint16_t>> channel_to_input;
    std::map<int, std::vector<uint16_t>> channel_to_output;

    std::map<int, std::vector<uint16_t>> found_output;
    std::vector<int> error_locs;
    bool has_assembly_error = false;

    validation_info inf;
    world_context real_world_context;

    bool(*extra_validation)(level_context&) = nullptr;
};

struct run_context
{
    constant_time_exec exec;

    level_context ctx;
};

struct stats
{
    int cycles = 0;
    int assembly_length = 0;
    bool success = false;
};

namespace level
{
    std::vector<std::string> get_available();

    level_context start(const std::string& name, int answer_rough_count);

    bool setup_validation(level_context& ctx, dcpu::ide::project_instance& instance);
    bool step_validation(level_context& ctx, dcpu::ide::project_instance& instance, int cycles);

    stats validate(level_context& ctx, dcpu::ide::project_instance& instance);
}

#endif // LEVELS_HPP_INCLUDED
