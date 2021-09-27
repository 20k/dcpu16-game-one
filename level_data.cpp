#include "level_data.hpp"
#include <filesystem>
#include <toolkit/fs_helpers.hpp>
#include <nlohmann/json.hpp>

level_data load_level(const std::filesystem::path& path_to_info)
{
    level_data ret;

    std::string description_json = file::read(path_to_info.string(), file::mode::TEXT);

    nlohmann::json parsed;

    try
    {
        parsed = nlohmann::json::parse(description_json);
    }
    catch(std::exception& e)
    {
        std::cout << "caught exception in load_level from file " << description_json << " " << e.what() << std::endl;
        return level_data();
    }

    std::string io_cpu = "io.d16";
    std::string validation_cpu = "validation.d16";

    std::filesystem::path io_path = path_to_info;
    io_path.replace_filename(io_cpu);

    std::filesystem::path validation_path = path_to_info;
    validation_path.replace_filename(validation_cpu);

    if(file::exists(io_path.string()))
    {
        ret.io_program = file::read(io_path.string(), file::mode::TEXT);
    }

    if(file::exists(validation_path.string()))
    {
        ret.dynamic_validation_program = file::read(validation_path.string(), file::mode::TEXT);
    }

    ret.name = parsed["name"];
    ret.description = parsed["description"];
    ret.short_description = parsed["short_description"];
    ret.hardware_names = parsed["hardware"].get<std::vector<std::string>>();

    return ret;
}

void all_level_data::load(const std::string& folder)
{
    for(auto& p : std::filesystem::recursive_directory_iterator(folder))
    {
        std::filesystem::path current_file = p.path();

        std::string filename = current_file.filename().string();

        if(filename == "info.json")
        {
            level_data next = load_level(current_file);

        }
    }
}
