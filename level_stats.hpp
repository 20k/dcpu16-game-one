#ifndef LEVEL_STATS_HPP_INCLUDED
#define LEVEL_STATS_HPP_INCLUDED

#include <optional>
#include <string>

namespace level_stats
{
    struct info
    {
        int cycles = 0;
        int assembly_length = 0;
    };

    std::optional<level_stats::info> load_best(const std::string& level_name);
    void save_best(const std::string& level_name, const level_stats::info& inf);
}

#endif // LEVEL_STATS_HPP_INCLUDED
