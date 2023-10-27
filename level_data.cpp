#include "level_data.hpp"
#include <filesystem>
#include <toolkit/fs_helpers.hpp>
#include <toml/toml.hpp>
#include <dcpu16-asm/base_asm.hpp>
#include <dcpu16-sim/hardware_clock.hpp>
#include <dcpu16-sim/hardware_lem1802.hpp>
#include "style.hpp"
#include "hardware_rng.hpp"
#include "hardware_inspector.hpp"
#include "hardware_gyro.hpp"
#include "hardware_rocket.hpp"
#include "hardware_rangefinder.hpp"

bool level_data::is_input_channel(int c) const
{
    for(int i : input_channels)
    {
        if(i == c)
            return true;
    }

    ///if its also not specified as an output channel, then th eanswer is whether or not the unspecified trait is set
    for(int i : output_channels)
    {
        ///output channel, cannot be input
        if(i == c)
            return false;
    }

    return unspecified_output_is_read;
}

bool level_data::is_output_channel(int c) const
{
    for(int i : output_channels)
    {
        if(i == c)
            return true;
    }

    for(int i : input_channels)
    {
        ///input channel, cannot be output
        if(i == c)
            return false;
    }

    return unspecified_output_is_write;
}

std::filesystem::path replace_filename(std::filesystem::path in, std::string_view filename)
{
    in.replace_filename(filename);

    return in;
}

toml::value string_to_toml(const std::string& in, const std::string& path)
{
    std::istringstream is(in, std::ios_base::binary | std::ios_base::in);

    return toml::parse(is, path);
}

level_data load_level(const std::filesystem::path& path_to_info)
{
    level_data ret;

    std::string description_toml = file::request::read(path_to_info.string(), file::mode::TEXT).value_or("");

    toml::value parsed;

    try
    {
        parsed = string_to_toml(description_toml, path_to_info.string());
    }
    catch(std::exception& e)
    {
        std::cout << "caught exception in load_level from file " << description_toml << " " << e.what() << std::endl;
        return level_data();
    }

    std::filesystem::path io_path = replace_filename(path_to_info, "io.d16");
    std::filesystem::path validation_path = replace_filename(path_to_info, "validation.d16");

    ret.io_program = file::request::read(io_path.string(), file::mode::TEXT);
    ret.dynamic_validation_program = file::request::read(validation_path.string(), file::mode::TEXT);

    ret.name = toml::get<std::string>(parsed["name"]);
    ret.description = toml::get<std::string>(parsed["description"]);
    ret.short_description = toml::get<std::string>(parsed["short_description"]);
    ret.section = toml::get<std::string>(parsed["section"]);
    ret.hardware_names = toml::get<std::vector<std::string>>(parsed["hardware"]);

    if(parsed.contains("input_channels"))
    {
        ret.input_channels = toml::get<std::vector<int>>(parsed["input_channels"]);
    }
    else
    {
        ret.unspecified_output_is_read = true;
    }

    if(parsed.contains("output_channels"))
    {
        ret.output_channels = toml::get<std::vector<int>>(parsed["output_channels"]);
    }
    else
    {
        ret.unspecified_output_is_write = true;
    }

    if(ret.unspecified_output_is_read && ret.unspecified_output_is_write)
    {
        throw std::runtime_error("Must specify input channels or output channels or both");
    }

    return ret;
}

void all_level_data::load(const std::string& folder)
{
    std::string base_info = file::request::read(folder + "/info.toml", file::mode::TEXT).value_or("");

    toml::value base_toml;

    try
    {
        base_toml = string_to_toml(base_info, folder + "/info.toml");

   }
    catch(std::exception& e)
    {
        std::cout << "caught exception in load_level from root file " << base_info << " " << e.what() << std::endl;
        assert(false);
    }

    std::vector<std::string> levels = toml::get<std::vector<std::string>>(base_toml["levels"]);

    for(const std::string& level_dir : levels)
    {
        std::filesystem::path current_file = std::filesystem::path(folder) / level_dir;

        //if(std::filesystem::is_directory(current_file))
        {
            //if(file::memfs::exists((current_file / ".ignore").string()))
            //    continue;

            all_levels.push_back(load_level(current_file / "info.toml"));
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

void level_runtime_parameters::generate_io(std::span<dcpu::sim::hardware*> hws, const level_data& data)
{
    if(!io_cpu.has_value())
        return;

    hardware_rng hwrng;
    hardware_inspector hwinspec;

    stack_vector<dcpu::sim::hardware*, 65536> hw;

    for(auto& i : hws)
    {
        hw.push_back(i);
    }

    hw.push_back(&hwrng);
    hw.push_back(&hwinspec);

    dcpu::sim::CPU& io_cpu_c = *io_cpu.value();

    uint64_t max_cycles = 1024 * 1024;

    dcpu::sim::fabric f;

    for(uint64_t current_cycle = 0; current_cycle < max_cycles; current_cycle++)
    {
        if(io_cpu_c.step(&f, hw.as_span()))
            current_cycle = max_cycles;

        if(dcpu::sim::has_any_write(io_cpu_c))
        {
            auto [value, channel] = dcpu::sim::drain_any_write(io_cpu_c, f);

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

/*constexpr void test()
{
    constexpr std::string_view view = R"(SET Y, 256

SND X, 0
SND X, 1

ADD X, 1

IFN X, Y
SET PC, 0

:hold
SET PC, hold)";

    constexpr auto val = assemble(view);

    static_assert(val.first.has_value());
}*/

void level_runtime_parameters::build_from(const level_data& data)
{
    if(data.io_program.has_value())
    {
        auto [data_opt, err] = assemble_fwd(data.io_program.value());

        if(!data_opt.has_value())
        {
            std::cout << "Bad io program " << data.io_program.value() << std::endl;
            print_err(err);
            return;
        }

        std::shared_ptr<dcpu::sim::CPU> ptr = std::make_shared<dcpu::sim::CPU>();
        ptr->load(data_opt.value().mem, 0);

        io_cpu = std::move(ptr);
    }

    if(data.dynamic_validation_program.has_value())
    {
        auto [data_opt, err] = assemble(data.dynamic_validation_program.value());

        if(!data_opt.has_value())
        {
            printf("Dyn err\n");
            print_err(err);
            return;
        }

        std::shared_ptr<dcpu::sim::CPU> ptr = std::make_shared<dcpu::sim::CPU>();
        ptr->load(data_opt.value().mem, 0);

        dynamic_validation_cpu = std::move(ptr);
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

        if(lower == "dummy")
        {
            dcpu::sim::hardware* dummy = new dcpu::sim::hardware;
            dummy->manufacturer_id = 0xDEADBEEF;

            hardware.push_back(dummy);
        }

        if(lower == "bad_gyroscope")
        {
            dcpu::sim::hardware* hw = new hardware_bad_gyro;

            hardware.push_back(hw);
        }

        if(lower == "gyroscope")
        {
            dcpu::sim::hardware* hw = new hardware_gyro;

            hardware.push_back(hw);
        }

        if(lower == "rocket")
        {
            dcpu::sim::hardware* hw = new hardware_rocket;

            hardware.push_back(hw);
        }
    }

    generate_io(hardware, data);
}

void level_runtime_data::build_from(const level_runtime_parameters& params)
{
    if(params.dynamic_validation_cpu.has_value())
    {
        std::shared_ptr<dcpu::sim::CPU> ptr = std::make_shared<dcpu::sim::CPU>();
        *ptr = *params.dynamic_validation_cpu.value();

        dynamic_validation_cpu = std::move(ptr);
    }

    for(dcpu::sim::hardware* hw : params.hardware)
    {
        hardware.push_back(hw->clone());
    }

    for(auto& [channel, vals] : params.channel_to_input)
    {
        for(auto& j : vals)
        {
            input_queue[channel].push_back(j);
        }
    }

    for(dcpu::sim::hardware* hw : hardware)
    {
        if(auto rocket = dynamic_cast<hardware_rocket*>(hw); rocket != nullptr)
        {
            real_world_state.player.base = rocket;
        }
    }

    //real_world_state = params.real_world_state;
}

void level_instance::update_assembly_errors(dcpu::ide::project_instance<dcpu::ide::editor>& instance)
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

void level_manager::start_level(dcpu::ide::project_instance<dcpu::ide::editor>& instance, const level_data& data)
{
    current_level = levels.make_instance(data);

    bool has_error = false;

    for(dcpu::ide::editor& edit : instance.editors)
    {
        if(edit.assemble())
            has_error = true;
    }

    current_level.value().ass_state.has_error = has_error;
}

void level_manager::switch_to_level(dcpu::ide::project_instance<dcpu::ide::editor>& instance, const level_data& data)
{
    uint64_t now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()).time_since_epoch().count();

    ///TODO: SECURITY
    file::mkdir("saves/" + data.name);

    std::string full_filename = "saves/" + data.name + "/save.dcpu_project";

    if(instance.proj.project_file.size() > 0)
    {
        instance.save();
    }

    instance = dcpu::ide::project_instance<dcpu::ide::editor>();

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

    start_level(instance, data);

    current_level.value().update_assembly_errors(instance);
    current_level.value().runtime_data.exec.init(0, now_ms);
}

void level_manager::save_current(dcpu::ide::project_instance<dcpu::ide::editor>& instance)
{
    if(instance.proj.project_file.size() > 0)
    {
        instance.save();
    }
}

void level_manager::back_to_main_menu()
{
    should_return_to_main_menu = true;
}

void level_manager::reset_level(dcpu::ide::project_instance<dcpu::ide::editor>& instance)
{
    level_instance current = current_level.value();

    start_level(instance, current.data);
}

void level_manager::step_validation(dcpu::ide::project_instance<dcpu::ide::editor>& instance)
{
    level_instance& my_level = current_level.value();

    my_level.execution_state = runtime_errors();

    std::vector<dcpu::sim::CPU*> user;

    for(dcpu::ide::editor& edit : instance.editors)
    {
        dcpu::sim::CPU& next = edit.c;

        user.push_back(&next);
    }

    stack_vector<dcpu::sim::CPU*, 64> cpus;

    for(auto& i : user)
    {
        cpus.push_back(i);
    }

    stack_vector<dcpu::sim::hardware*, 65536> all_hardware;

    for(auto& i : my_level.runtime_data.hardware)
    {
        all_hardware.push_back(i);
    }

    int max_cycles = 1;

    for(int i=0; i < max_cycles; i++)
    {
        for(int kk=0; kk < (int)cpus.size(); kk++)
        {
            instance.editors[kk].halted = instance.editors[kk].halted || cpus[kk]->cycle_step(&my_level.runtime_data.fab, all_hardware.as_span(), &my_level.runtime_data.real_world_state);
        }

        dcpu::sim::resolve_interprocessor_communication(cpus, my_level.runtime_data.fab);
    }

    for(int kk=0; kk < (int)cpus.size(); kk++)
    {
        dcpu::sim::CPU& c = *cpus[kk];

        if(dcpu::sim::has_any_write(c))
        {
            auto [value, channel] = dcpu::sim::drain_any_write(c, my_level.runtime_data.fab);

            my_level.runtime_data.found_output[channel].push_back(value);
        }

        for(auto& [channel, values] : my_level.runtime_data.input_queue)
        {
            if(values.size() == 0)
                continue;

            if(dcpu::sim::has_read(c, channel))
            {
                uint16_t front = values.front();
                values.pop_front();

                dcpu::sim::fulfill_read(c, my_level.runtime_data.fab, front, channel);
            }
        }
    }

    {
        assert(user.size() >= 1);

        int screen_idx = 0;

        for(int i=0; i < (int)my_level.runtime_data.hardware.size(); i++)
        {
            dcpu::sim::hardware* hw = my_level.runtime_data.hardware[i];

            bool is_lem = hw->hardware_id == 0x7349f615;

            if(!is_lem)
                continue;

            dcpu::sim::LEM1802* as_lem = dynamic_cast<dcpu::sim::LEM1802*>(hw);

            assert(as_lem);

            as_lem->render(&my_level.runtime_data.real_world_state, *user.front());

            screen_idx++;
        }
    }

    for(auto& [channel, values] : my_level.runtime_data.found_output)
    {
        std::vector<uint16_t> current_output = values;

        const std::vector<uint16_t>& output_vals = my_level.constructed_data.channel_to_output[channel];

        current_output.resize(output_vals.size());

        for(int i=0; i < (int)current_output.size() && i < (int)output_vals.size(); i++)
        {
            if(current_output[i] != output_vals[i])
            {
                my_level.execution_state.error_locs.push_back(i);
                my_level.execution_state.error_channels.push_back(channel);
            }
        }
    }

    bool dynamic_validation_success = true;

    if(my_level.runtime_data.dynamic_validation_cpu.has_value())
    {
        dynamic_validation_success = false;

        hardware_rng hwrng;
        hardware_inspector hwinspec;

        stack_vector<dcpu::sim::hardware*, 65536> hw;

        for(auto& i : my_level.runtime_data.hardware)
        {
            hw.push_back(i);
        }

        cpu_proxy hwcpu;
        hwcpu.c = cpus[0];

        hw.push_back(&hwcpu);
        hw.push_back(&hwrng);
        hw.push_back(&hwinspec);

        dcpu::sim::CPU dynamic_cpu_c = *my_level.runtime_data.dynamic_validation_cpu.value();

        ///on the validation program to execute responsibly
        uint64_t max_cycles = 1024 * 1024;

        dcpu::sim::fabric f;

        for(uint64_t current_cycle = 0; current_cycle < max_cycles; current_cycle++)
        {
            if(dynamic_cpu_c.step(&f, hw.as_span(), &my_level.runtime_data.real_world_state))
                current_cycle = max_cycles;

            if(dcpu::sim::has_any_write(dynamic_cpu_c))
            {
                dynamic_validation_success = true;
            }
        }
    }

    bool any_errors_at_all = my_level.execution_state.error_locs.size() > 0 || my_level.ass_state.has_error || !dynamic_validation_success;

    ///just succeeded
    if(!my_level.successful_validation && !any_errors_at_all)
    {
        int total_cycles = 0;
        int total_assembly = 0;

        for(dcpu::ide::editor& edit : instance.editors)
        {
            dcpu::sim::CPU& next = edit.c;

            total_cycles += next.cycle_count;
            total_assembly += edit.translation_map.size();
        }

        level_stats::info rstat;
        rstat.cycles = total_cycles;
        rstat.assembly_length = total_assembly;
        rstat.valid = true;

        {
            auto best_stats_opt = level_stats::load_best(my_level.data.name);

            level_stats::info best;
            best.assembly_length = INT_MAX;
            best.cycles = INT_MAX;

            if(best_stats_opt.has_value())
                best = best_stats_opt.value();

            best.cycles = std::min(best.cycles, rstat.cycles);
            best.assembly_length = std::min(best.assembly_length, rstat.assembly_length);
            best.valid = true;

            level_stats::save_best(my_level.data.name, best);
        }

        my_level.runtime_data.current_run_stats = rstat;
    }

    my_level.successful_validation = !any_errors_at_all;
}

void level_manager::display_level_select(dcpu::ide::project_instance<dcpu::ide::editor>& instance)
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

    std::stable_sort(levels.all_levels.begin(), levels.all_levels.end(), [&](auto& p1, auto& p2)
    {
        int rank1 = section_rank(p1.section);
        int rank2 = section_rank(p2.section);

        return rank1 < rank2;
    });

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

        std::optional<level_stats::info> best_stats = level_stats::load_best(selected.name);

        if(best_stats.has_value())
        {
            std::string validation_string = best_stats.value().valid ? "VALID" : "INVALID";

            ImGui::Text("CYCLE COUNT       : %i", best_stats.value().cycles);
            ImGui::Text("INSTRUCTION SIZE  : %i", best_stats.value().assembly_length);
            ImGui::Text("VALIDATION        : %s", validation_string.c_str());
        }
        else
        {
            ImGui::Text("CYCLE COUNT       : 0");
            ImGui::Text("INSTRUCTION SIZE  : 0");
            ImGui::Text("VALIDATION        : INVALID");
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
