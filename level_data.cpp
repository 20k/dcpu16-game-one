#include "level_data.hpp"
#include <filesystem>
#include <toolkit/fs_helpers.hpp>
#include <toml/toml.hpp>
#include <dcpu16-asm/base_asm.hpp>
#include <dcpu16-sim/hardware_clock.hpp>
#include <dcpu16-sim/hardware_lem1802.hpp>

bool level_data::is_input_channel(int c) const
{
    for(int i : input_channels)
    {
        if(i == c)
            return true;
    }

    return false;
}

bool level_data::is_output_channel(int c) const
{
    for(int i : output_channels)
    {
        if(i == c)
            return true;
    }

    return false;
}

level_data load_level(const std::filesystem::path& path_to_info)
{
    level_data ret;

    std::string description_toml = file::read(path_to_info.string(), file::mode::TEXT);

    toml::value parsed;

    try
    {
        std::istringstream is(description_toml, std::ios_base::binary | std::ios_base::in);

        parsed = toml::parse(is, path_to_info.string());
    }
    catch(std::exception& e)
    {
        std::cout << "caught exception in load_level from file " << description_toml << " " << e.what() << std::endl;
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

    ret.name = toml::get<std::string>(parsed["name"]);
    ret.description = toml::get<std::string>(parsed["description"]);
    ret.short_description = toml::get<std::string>(parsed["short_description"]);
    ret.hardware_names = toml::get<std::vector<std::string>>(parsed["hardware"]);

    ret.input_channels = toml::get<std::vector<int>>(parsed["input_channels"]);
    ret.output_channels = toml::get<std::vector<int>>(parsed["output_channels"]);

    return ret;
}

void all_level_data::load(const std::string& folder)
{
    for(auto& p : std::filesystem::recursive_directory_iterator(folder))
    {
        std::filesystem::path current_file = p.path();

        std::string filename = current_file.filename().string();

        if(filename == "info.toml")
        {
            level_data next = load_level(current_file);

            all_levels.push_back(next);
        }
    }
}

void print_sv(std::string_view in)
{
    for(auto i : in)
    {
        putchar(i);
    }

    printf("\n");
}

void print_err(const error_info& err)
{
    printf("Could not assemble. Err: ");
    print_sv(err.msg);
    printf("Name: ");
    print_sv(err.name_in_source);
    printf("Character: %i", err.character);
    printf("Line %i\n", err.line);
}

char to_lower(char c)
{
    if(c <= 'Z' && c >= 'A')
        return (c - 'Z') + 'z';

    return c;
}

std::string to_lower(std::string in)
{
    for(char& c : in)
    {
        c = to_lower(c);
    }

    return in;
}

void level_runtime_parameters::generate_io(const level_data& data)
{
    if(!io_cpu.has_value())
        return;

    dcpu::sim::CPU& io_cpu_c = io_cpu.value();

    uint64_t max_cycles = 1024 * 1024;

    dcpu::sim::fabric f;

    for(uint64_t current_cycle = 0; current_cycle < max_cycles; current_cycle++)
    {
        io_cpu_c.step(&f);

        if(dcpu::sim::has_write(io_cpu_c))
        {
            auto [value, channel] = dcpu::sim::drain_write(io_cpu_c, f);

            if(data.is_input_channel(channel))
            {
                channel_to_input[channel].push_back(value);
            }

            if(data.is_output_channel(channel))
            {
                channel_to_output[channel].push_back(value);

                for(auto& [key, value] : channel_to_input)
                {
                    output_to_input_start[channel][key].push_back(value.size());
                }
            }
        }
    }
}

void level_runtime_parameters::build_from(const level_data& data)
{
    if(data.io_program.has_value())
    {
        auto [data_opt, err] = assemble(data.io_program.value());

        if(!data_opt.has_value())
        {
            print_err(err);
            return;
        }

        dcpu::sim::CPU cpu;
        cpu.load(data_opt.value().mem, 0);

        io_cpu = cpu;
    }

    if(data.dynamic_validation_program.has_value())
    {
        auto [data_opt, err] = assemble(data.dynamic_validation_program.value());

        if(!data_opt.has_value())
        {
            print_err(err);
            return;
        }

        dcpu::sim::CPU cpu;
        cpu.load(data_opt.value().mem, 0);

        dynamic_validation_cpu = cpu;
    }

    for(const std::string& name : data.hardware_names)
    {
        std::string lower = to_lower(name);

        if(lower == "lem1802")
        {
            dcpu::sim::hardware* hw = new dcpu::sim::LEM1802;

            hardware.push_back(hw);
        }

        if(lower == "clock" || lower == "generic_clock")
        {
            dcpu::sim::hardware* hw = new dcpu::sim::clock;

            hardware.push_back(hw);
        }
    }

    generate_io(data);
}

level_instance all_level_data::make_instance(const level_data& data)
{
    //level_instance
}
