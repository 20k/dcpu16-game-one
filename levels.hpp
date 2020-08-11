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
#include "level_stats.hpp"

struct world_context : dcpu::sim::time_state, dcpu::sim::lem1802_screens_state, dcpu::sim::world_base
{

};

struct hardware_data
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
    std::optional<level_stats::info> current_stats;
    bool displayed_level_over = false;

    bool successful_validation = false;
    bool finished = false;
    int cpus = 0;

    std::string level_name;
    std::string description;
    std::string short_description;

    std::map<int, std::vector<uint16_t>> channel_to_input;
    std::map<int, std::vector<uint16_t>> channel_to_output;
    std::map<int, std::map<int, std::vector<int>>> output_to_input_start;

    std::map<int, std::vector<uint16_t>> found_output;
    std::vector<int> error_locs;
    std::vector<int> error_channels;
    bool has_assembly_error = false;

    hardware_data inf;
    world_context real_world_context;

    bool(*extra_validation)(level_context&, dcpu::ide::project_instance& instance) = nullptr;
};

struct run_context
{
    constant_time_exec exec;

    level_context ctx;
};

struct level_selector_state
{
    std::string level_name;
    level_stats::info stats;

    void set_level_name(const std::string& level_name);
};

struct level_over_state
{
    level_stats::info best_stats;
    level_stats::info current_stats;
};

namespace level
{
    //std::vector<std::string> get_available();

    void switch_to_level(run_context& ctx, dcpu::ide::project_instance& instance, const std::string& level_name);

    void display_level_select(level_selector_state& select, run_context& ctx, dcpu::ide::project_instance& instance);

    level_context start(const std::string& name, int answer_rough_count);

    bool setup_validation(level_context& ctx, dcpu::ide::project_instance& instance);
    bool step_validation(level_context& ctx, dcpu::ide::project_instance& instance, int cycles);
}

#endif // LEVELS_HPP_INCLUDED
