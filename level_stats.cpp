#include "level_stats.hpp"
#include <toml/toml.hpp>
#include <filesystem>
#include <toolkit/fs_helpers.hpp>
#include <map>
#include <iostream>

std::map<std::string, std::optional<level_stats::info>>& get_cache()
{
    static thread_local std::map<std::string, std::optional<level_stats::info>> cache;

    return cache;
}

std::optional<level_stats::info> level_stats::load_best(const std::string& level_name)
{
    auto& cache = get_cache();

    if(auto it = cache.find(level_name); it != cache.end())
        return it->second;

    file::mkdir("stats");
    file::mkdir("stats/" + level_name);

    std::string data_file = "stats/" + level_name + "/stats.toml";

    std::string data = file::read(data_file, file::mode::TEXT);;

    std::istringstream is(data, std::ios_base::in);

    try
    {
        toml::value val = toml::parse(is, data_file);

        level_stats::info ret;

        ret.assembly_length = toml::get<int>(val["assembly_length"]);
        ret.cycles = toml::get<int>(val["cycles"]);
        ret.valid = toml::get<bool>(val["valid"]);

        cache[level_name] = ret;

        return ret;
    }
    catch(std::exception& err)
    {
        //std::cout << "No load best in " << level_name << " What " << err.what() << std::endl;
    }

    cache[level_name] = std::nullopt;

    return std::nullopt;
}

void level_stats::save_best(const std::string& level_name, const level_stats::info& inf)
{
    file::mkdir("stats");
    file::mkdir("stats/" + level_name);
    std::string data_file = "stats/" + level_name + "/stats.toml";

    toml::value val;
    val["assembly_length"] = inf.assembly_length;
    val["cycles"] = inf.cycles;
    val["valid"] = inf.valid;

    std::string as_str = toml::format(val);

    file::write_atomic(data_file, as_str, file::mode::TEXT);

    get_cache()[level_name] = inf;
}
