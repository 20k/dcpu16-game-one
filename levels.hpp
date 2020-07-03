#ifndef LEVELS_HPP_INCLUDED
#define LEVELS_HPP_INCLUDED

#include <vector>
#include <string>
#include <dcpu16-sim/base_sim.hpp>
#include <dcpu16-ide/base_ide.hpp>

struct level_context
{
    std::vector<dcpu::ide::editor> cpus;
    int level = 0;
    std::string description;
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

    stats validate(const level_context& ctx);
}

#endif // LEVELS_HPP_INCLUDED
