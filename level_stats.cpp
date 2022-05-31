#include "level_stats.hpp"
#include <toml/toml.hpp>
#include <filesystem>
#include <toolkit/fs_helpers.hpp>

std::optional<level_stats::info> level_stats::load_best(const std::string& level_name)
{
    file::mkdir("stats");
    file::mkdir("stats/" + level_name);

    std::string data_file = "stats/" + level_name + "/stats.toml";

    std::string data = file::read(data_file, file::mode::TEXT);;

    std::istringstream is(data, std::ios_base::binary | std::ios_base::in);

    try
    {
        toml::value val = toml::parse(is, data_file);

        level_stats::info ret;
        ret.assembly_length = toml::get<int>(val["assembly_length"]);
        ret.cycles = toml::get<int>(val["cycles"]);
        ret.valid = toml::get<bool>(val["valid"]);

        return ret;
    }
    catch(...)
    {

    }

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
}
