#ifndef LEVELS_HPP_INCLUDED
#define LEVELS_HPP_INCLUDED

#include <vector>
#include <string>
#include <map>
#include <dcpu16-sim/base_sim.hpp>
#include <dcpu16-ide/base_ide.hpp>

struct validation_info
{
    std::map<int, dcpu::sim::CPU> input_cpus;
    std::map<int, dcpu::sim::CPU> output_cpus;

    int cycle = 0;
};

struct level_context
{
    //std::vector<dcpu::ide::editor> cpus;
    int cpus = 0;
    int level = 0;
    std::string description;

    std::map<int, std::vector<uint16_t>> channel_to_input;
    std::map<int, std::vector<uint16_t>> channel_to_output;

    std::map<int, std::vector<uint16_t>> found_output;
    std::vector<int> error_locs;

    validation_info inf;
};

struct stats
{
    int cycles = 0;
    int assembly_length = 0;
    bool success = false;
};

namespace level
{
    std::vector<int> get_available();

    level_context start(int level);

    void setup_validation(level_context& ctx, dcpu::ide::project_instance& instance);
    void step_validation(level_context& ctx, dcpu::ide::project_instance& instance, int cycles);

    stats validate(level_context& ctx, dcpu::ide::project_instance& instance);
}

#endif // LEVELS_HPP_INCLUDED
