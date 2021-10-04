#ifndef LEVEL_DATA_HPP_INCLUDED
#define LEVEL_DATA_HPP_INCLUDED

#include <optional>
#include <string>
#include <map>
#include <dcpu16-sim/base_sim.hpp>
#include <dcpu16-ide/base_ide.hpp>
#include <dcpu16-sim/base_hardware.hpp>
#include <dcpu16-sim/all_hardware.hpp>
#include <deque>
#include "world_state.hpp"
#include "constant_time_exec.hpp"
#include "level_stats.hpp"

struct level_data
{
    int cpus = 1;

    std::string name;
    std::string description;
    std::string short_description;
    std::string section;

    std::optional<std::string> io_program;
    std::optional<std::string> dynamic_validation_program;
    std::vector<std::string> hardware_names;

    std::optional<level_stats::info> my_best_stats;

    std::vector<int> input_channels;
    std::vector<int> output_channels;

    bool unspecified_output_is_write = false;
    bool unspecified_output_is_read = false;

    bool is_input_channel(int c) const;
    bool is_output_channel(int c) const;
};

struct level_runtime_parameters
{
    void build_from(const level_data& data);

    std::map<int, std::vector<uint16_t>> channel_to_input;
    std::map<int, std::vector<uint16_t>> channel_to_output;
    std::map<int, std::map<int, std::vector<int>>> output_to_input_start;

    std::optional<std::shared_ptr<dcpu::sim::CPU>> io_cpu;
    std::optional<std::shared_ptr<dcpu::sim::CPU>> dynamic_validation_cpu;
    std::vector<dcpu::sim::hardware*> hardware;

private:
    void generate_io(const level_data& data);
};

struct assembler_state
{
    bool has_error = false;
};

struct level_runtime_data
{
    void build_from(const level_runtime_parameters& params);

    world_state real_world_state;

    std::map<int, std::deque<uint16_t>> input_queue;

    std::optional<level_stats::info> current_run_stats;
    std::optional<std::shared_ptr<dcpu::sim::CPU>> dynamic_validation_cpu;
    std::vector<dcpu::sim::hardware*> hardware;

    dcpu::sim::fabric fab;
    constant_time_exec exec;

    std::map<int, std::vector<uint16_t>> found_output;
};

struct runtime_errors
{
    std::vector<int> error_locs;
    std::vector<int> error_channels;
};

struct level_instance
{
    level_data data;
    level_runtime_parameters constructed_data;
    level_runtime_data runtime_data;
    assembler_state ass_state;
    runtime_errors execution_state;

    bool displayed_level_over = false;
    bool successful_validation = false;

    void update_assembly_errors(dcpu::ide::project_instance& instance);
};

struct all_level_data
{
    std::vector<level_data> all_levels;

    void load(const std::string& folder = "./levels");

    level_instance make_instance(const level_data& data);
    //void display_level_select();
};

struct level_manager
{
    all_level_data levels;
    std::optional<level_data> selected_level;

    std::optional<level_instance> current_level;
    bool should_return_to_main_menu = false;

    void load(const std::string& folder = "./levels");
    void save_current(dcpu::ide::project_instance& instance);

    void back_to_main_menu();

    void display_level_select(dcpu::ide::project_instance& instance);

    void reset_level(dcpu::ide::project_instance& instance);
    //void start_level(const std::string& level);

    //void setup_validation(dcpu::ide::project_instance& instance);
    void step_validation(dcpu::ide::project_instance& instance);

private:
    void switch_to_level(dcpu::ide::project_instance& instance, const level_data& data);
    void start_level(dcpu::ide::project_instance& instance, const level_data& data);
};

#endif // LEVEL_DATA_HPP_INCLUDED
