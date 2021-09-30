#include "level_data.hpp"
#include <filesystem>
#include <toolkit/fs_helpers.hpp>
#include <toml/toml.hpp>
#include <dcpu16-asm/base_asm.hpp>
#include <dcpu16-sim/hardware_clock.hpp>
#include <dcpu16-sim/hardware_lem1802.hpp>
#include "style.hpp"

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
    ret.section = toml::get<std::string>(parsed["section"]);
    ret.hardware_names = toml::get<std::vector<std::string>>(parsed["hardware"]);

    ret.input_channels = toml::get<std::vector<int>>(parsed["input_channels"]);
    ret.output_channels = toml::get<std::vector<int>>(parsed["output_channels"]);

    ret.my_best_stats = level_stats::load_best(ret.name);

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

void level_runtime_data::build_from(const level_runtime_parameters& params)
{
    dynamic_validation_cpu = params.dynamic_validation_cpu;

    for(dcpu::sim::hardware* hw : params.hardware)
    {
        hardware.push_back(hw->clone());
    }
}

void level_instance::update_assembly_errors(dcpu::ide::project_instance& instance)
{
    bool has_error = false;

    for(dcpu::ide::editor& edit : instance.editors)
    {
        if(edit.assemble())
            has_error = true;
    }

    ass_state.has_error = has_error;
}

level_instance all_level_data::make_instance(const level_data& data)
{
    level_instance inst;
    inst.data = data;
    inst.constructed_data.build_from(inst.data);
    inst.runtime_data.build_from(inst.constructed_data);

    return inst;
}

void level_manager::load(const std::string& folder)
{
    levels.load(folder);
}

void level_manager::start_level(const level_data& data)
{
    current_level = levels.make_instance(data);
}

void level_manager::switch_to_level(dcpu::ide::project_instance& instance, const level_data& data)
{
    uint64_t now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()).time_since_epoch().count();

    ///TODO: SECURITY
    std::filesystem::create_directory("saves/" + data.name);

    std::string full_filename = "saves/" + data.name + "/save.dcpu_project";

    if(instance.proj.project_file.size() > 0)
    {
        instance.save();
    }

    instance = dcpu::ide::project_instance();

    if(file::exists(full_filename))
    {
        instance.load(full_filename);
    }
    else
    {
        instance.proj.project_file = full_filename;
        instance.proj.assembly_files = {"cpu0.d16"};
        instance.proj.assembly_data = {""};

        instance.editors.emplace_back();
    }

    start_level(data);

    current_level.value().update_assembly_errors(instance);
    current_level.value().runtime_data.exec.init(0, now_ms);
}

void level_manager::reset_level()
{
    level_instance current = current_level.value();

    start_level(current.data);
}

void level_manager::step_validation(dcpu::ide::project_instance& instance)
{
    level_instance& my_level = current_level.value();

    my_level.execution_state = runtime_errors();
}

void level_manager::display_level_select(dcpu::ide::project_instance& instance)
{
    auto section_rank = [](const std::string& in)
    {
        if(in == "TUTORIAL")
            return 0;

        if(in == "SOFTWARE")
            return 1;

        if(in == "HARDWARE")
            return 2;

        return 3;
    };

    std::sort(levels.all_levels.begin(), levels.all_levels.end(), [&](auto& p1, auto& p2){return section_rank(p1.section) < section_rank(p2.section);});

    ImGui::SetNextWindowSize(ImVec2(300, 0));

    ImGui::Begin("Levels", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove);

    auto screen_dim = ImGui::GetIO().DisplaySize;
    auto screen_pos = ImGui::GetMainViewport()->Pos;
    auto window_dim = ImGui::GetWindowSize();

    ImVec2 centre_pos = ImVec2(screen_pos.x + screen_dim.x/2 - window_dim.x/2, screen_pos.y + screen_dim.y/2 - window_dim.y/2);

    ImGui::SetWindowPos(ImVec2(centre_pos.x, centre_pos.y));

    style::start();

    std::string last_section = "";

    for(int i=0; i < (int)levels.all_levels.size(); i++)
    {
        level_data& this_level = levels.all_levels[i];

        if(last_section != this_level.section)
            ImGui::Selectable(this_level.section.c_str());

        last_section = this_level.section;

        const std::string& level_name = this_level.name;

        std::string level_str = " " + level_name + " ";

        if(selected_level.has_value())
        {
            level_data& selected = selected_level.value();

            if(selected.section == this_level.section && selected.name == this_level.name)
            {
                level_str = "[" + level_name + "]";
            }
        }

        ImGui::Text(" ");

        ImGui::SameLine(0,0);

        if(ImGui::Selectable(level_str.c_str()))
        {
            selected_level = this_level;
        }
    }

    //if(select.level_name.size() > 0)

    if(selected_level.has_value())
    {
        level_data& selected = selected_level.value();

        int width = ImGui::GetWindowSize().x - ImGui::GetStyle().FramePadding.x - ImGui::GetStyle().ItemSpacing.x - 10;

        int char_num = width /  ImGui::CalcTextSize("-").x;

        if(char_num < 0)
            char_num = 0;

        std::string spacer(char_num, '-');

        //ImGui::Text(spacer.c_str());

        auto cursor_pos = ImGui::GetCursorScreenPos();

        ImGui::GetCurrentWindow()->DrawList->AddText(ImVec2(cursor_pos.x, cursor_pos.y), IM_COL32(0xCF, 0xCF, 0xCF, 0xFF), spacer.c_str());

        ImGui::NewLine();

        ImGui::Text("INFO");

        ImGui::Indent();

        if(selected.my_best_stats.has_value())
        {
            std::string validation_string = selected.my_best_stats.value().valid ? "VALID" : "INVALID";

            ImGui::Text("CYCLE COUNT       : %i", selected.my_best_stats.value().cycles);
            ImGui::Text("INSTRUCTION SIZE  : %i", selected.my_best_stats.value().assembly_length);
            ImGui::Text("VALIDATION        : %s", validation_string.c_str());
        }

        ImGui::NewLine();

        std::string description = selected.short_description;

        float old_pos = ImGui::GetCursorScreenPos().y;

        ImGui::TextWrapped("%s\n", description.c_str());

        float new_pos = ImGui::GetCursorScreenPos().y;

        float height = new_pos - old_pos;

        int columns = ceilf(height / ImGui::CalcTextSize("\n").y);

        for(int i=columns; i < 4; i++)
        {
            ImGui::NewLine();
        }

        ImGui::Unindent();

        ImGui::GetCurrentWindow()->DrawList->AddText(ImVec2(cursor_pos.x, cursor_pos.y), IM_COL32(0xCF, 0xCF, 0xCF, 0xFF), spacer.c_str());
        ImGui::NewLine();

        if(ImGui::Selectable("START"))
        {
            switch_to_level(instance, selected);
        }
    }

    style::finish();

    ImGui::End();
}
