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
    std::map<int, stack_vector<uint16_t, MEM_SIZE>> input_translation;
    std::map<int, stack_vector<uint16_t, MEM_SIZE>> output_translation;

    dcpu::sim::fabric fab;

    int cycle = 0;
};

struct level_context
{
    //std::vector<dcpu::ide::editor> cpus;
    bool finished = false;
    int cpus = 0;
    std::string level_name;
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
    std::vector<std::string> get_available();

    level_context start(const std::string& name, int answer_rough_count);

    void setup_validation(level_context& ctx, dcpu::ide::project_instance& instance);
    void step_validation(level_context& ctx, dcpu::ide::project_instance& instance, int cycles);

    stats validate(level_context& ctx, dcpu::ide::project_instance& instance);
}

#endif // LEVELS_HPP_INCLUDED
