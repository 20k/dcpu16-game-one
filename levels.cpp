#include "levels.hpp"
#include <dcpu16-asm/base_asm_fwd.hpp>
#include <iostream>
#include <cmath>
#include <dcpu16-sim/hardware_clock.hpp>
#include <dcpu16-sim/hardware_lem1802.hpp>
#include <chrono>
#include <filesystem>
#include <toolkit/fs_helpers.hpp>
#include "style.hpp"
#include <imgui/imgui_internal.h>

dcpu::sim::CPU sim_input(const std::vector<uint16_t>& input, int channel, stack_vector<uint16_t, MEM_SIZE>& line_map)
{
    std::string vals;

    for(uint16_t i : input)
    {
        vals = vals + "SND " + std::to_string(i) + ", " + std::to_string(channel) + "\n";
    }

    dcpu::sim::CPU inc;

    {
        auto [rinfo_opt, err] = assemble_fwd(vals);

        assert(rinfo_opt.has_value());

        inc.load(rinfo_opt.value().mem, 0);
        line_map = rinfo_opt.value().pc_to_source_line;
    }

    return inc;
}

dcpu::sim::CPU sim_output(int len, int channel, stack_vector<uint16_t, MEM_SIZE>& line_map)
{
    std::string out_str;

    uint16_t output_address = 0x8000;

    for(int i=0; i < len; i++)
    {
        uint16_t fin_address = output_address + i;

        out_str += "RCV [" + std::to_string(fin_address) + "], " + std::to_string(channel) + "\n";
    }

    dcpu::sim::CPU outc;

    {
        auto [rinfo_out, errout] = assemble_fwd(out_str);

        assert(rinfo_out.has_value());

        outc.load(rinfo_out.value().mem, 0);
        line_map = rinfo_out.value().pc_to_source_line;
    }

    return outc;
}

int check_output(const dcpu::sim::CPU& in, const std::vector<uint16_t>& output, std::vector<uint16_t>& found)
{
    uint16_t output_address = 0x8000;

    for(int i=0; i < (int)output.size(); i++)
    {
        uint16_t fin_address = output_address + i;

        found.push_back(in.mem[fin_address]);
    }

    for(int i=0; i < (int)output.size(); i++)
    {
        uint16_t fin_address = output_address + i;

        if(in.mem[fin_address] != output[i])
            return i;
    }

    return -1;
}

uint16_t lcg(uint64_t& state)
{
    uint64_t a = 2862933555777941757;
    uint64_t b = 3037000493;

    state = a * state + b;

    return state >> (64-16);
}

void level::switch_to_level(run_context& ctx, dcpu::ide::project_instance& instance, const std::string& level_name)
{
    uint64_t now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()).time_since_epoch().count();

    std::filesystem::create_directory("saves/" + level_name);

    std::string full_filename = "saves/" + level_name + "/save.dcpu_project";

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

    ctx.ctx = level::start(level_name, 256);

    level::setup_validation(ctx.ctx, instance);

    ctx.exec.init(0, now_ms);
}

void level::display_level_select(level_selector_state& select, run_context& ctx, dcpu::ide::project_instance& instance)
{
    std::vector<std::string> intro_levels = {"INTRO", "AMPLIFY", "DIVISIONS", "SPACESHIP_OPERATOR", "CHECKSUM"};
    std::vector<std::string> software_levels = {"POWR"};
    std::vector<std::string> hardware_levels = {"HWENUMERATE", "CONSOLE", "SANDBOX"};

    std::vector<std::pair<std::string, std::vector<std::string>>> all_levels =
    {
        {"TUTORIAL", intro_levels},
        {"SOFTWARE", software_levels},
        {"HARDWARE", hardware_levels},
    };

    if(select.level_name.size() == 0)
    {
        select.level_name = "INTRO";
    }

    ImGui::SetNextWindowSize(ImVec2(300, 0));

    ImGui::Begin("Levels", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove);

    auto screen_dim = ImGui::GetIO().DisplaySize;
    auto screen_pos = ImGui::GetMainViewport()->Pos;
    auto window_dim = ImGui::GetWindowSize();

    ImVec2 centre_pos = ImVec2(screen_pos.x + screen_dim.x/2 - window_dim.x/2, screen_pos.y + screen_dim.y/2 - window_dim.y/2);

    ImGui::SetWindowPos(ImVec2(centre_pos.x, centre_pos.y));

    style::start();

    for(int i=0; i < (int)all_levels.size(); i++)
    {
        //if(ImGui::TreeNode(all_levels[i].first.c_str()))

        ImGui::Selectable(all_levels[i].first.c_str());
        {
            for(int j = 0; j < (int)all_levels[i].second.size(); j++)
            {
                const std::string& level_name = all_levels[i].second[j];

                std::string level_str = (level_name == select.level_name) ? ("[" + level_name + "]") : (" " + level_name + " ");

                //ImGui::Spacing();

                ImGui::Text(" ");

                ImGui::SameLine(0,0);

                if(ImGui::Selectable(level_str.c_str()))
                {
                    //switch_to_level(ctx, instance, level_name);
                    select.level_name = level_name;
                }
            }

            //ImGui::TreePop();
        }
    }

    //ImGui::NewLine();

    if(select.level_name.size() > 0)
    {
        //int width = ImGui::GetContentRegionAvail().x;

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

        //ImGui::Text("NAME              : %s", select.level_name.c_str());
        ImGui::Text("CYCLE COUNT       : 0");
        ImGui::Text("INSTRUCTION COUNT : 0");
        ImGui::Text("VALIDATION        : INVALID");

        ImGui::NewLine();

        std::string description = level::start(select.level_name, 256).short_description;


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

        //ImGui::Text(spacer.c_str());

        //ImGui::NewLine();

        if(ImGui::Selectable("START"))
        {
            switch_to_level(ctx, instance, select.level_name);
        }
    }

    style::finish();

    ImGui::End();
}

namespace level
{
    /*std::vector<std::string> get_available()
    {
        return {"INTRO", "AMPLIFY", "DIVISIONS", "SPACESHIP_OPERATOR", "CHECKSUM", "POWR", "HWENUMERATE"};
    }*/

    void simple_page_mapping(level_context& ctx)
    {
        for(auto& [channel, vec] : ctx.channel_to_output)
        {
            for(auto& [chan2, _] : ctx.channel_to_input)
            {
                for(int i=0; i < (int)vec.size(); i++)
                {
                    ctx.output_to_input_start[channel][chan2].push_back(i);
                }
            }

            for(auto& [chan2, _] : ctx.channel_to_output)
            {
                if(chan2 == channel)
                    continue;

                for(int i=0; i < (int)vec.size(); i++)
                {
                    ctx.output_to_input_start[channel][chan2].push_back(i);
                }
            }
        }
    }

    level_context start(const std::string& level_name, int answer_rough_count)
    {
        level_context ctx;
        ctx.level_name = level_name;

        uint64_t seed = std::hash<std::string>{}(level_name) * 2 + 1;

        for(int i=0; i < 128; i++)
        {
            lcg(seed);
        }

        if(ctx.level_name == "INTRO")
        {
            ctx.description = "Intro to the DCPU-16. Pass the input to the output\n"
                              "Use RCV X, 0 to receive input on Ch:0, and SND X, 1 to send output on Ch:1\n"
                              "Remember to make your program loop by using SET PC, 0";

            ctx.short_description = "PASS THE INPUT TO THE OUTPUT";

            ctx.cpus = 1;

            std::vector<uint16_t> input;
            std::vector<uint16_t> output;

            for(int i=0; i < answer_rough_count; i++)
            {
                input.push_back(i);
                output.push_back(i);
            }

            ctx.channel_to_input[0] = input;
            ctx.channel_to_output[1] = output;

            simple_page_mapping(ctx);
        }

        if(ctx.level_name == "AMPLIFY")
        {
            ctx.description = "Amplify the input - Multiply by 16";
            ctx.short_description = "MULTIPLY THE INPUT BY 16";
            ctx.cpus = 1;

            std::vector<uint16_t> input;
            std::vector<uint16_t> output;

            for(int i=0; i < answer_rough_count; i++)
            {
                uint16_t in = lcg(seed) % (65536 / 32);
                uint16_t out = in * 16;

                input.push_back(in);
                output.push_back(out);
            }

            ctx.channel_to_input[0] = input;
            ctx.channel_to_output[1] = output;

            simple_page_mapping(ctx);
        }

        ///can't figure out any way this puzzle can be different
        ///fundamental constraint: Needs 2 outputs, 1+ inputs
        ///with a reusable component
        /*if(ctx.level_name == "DIFFS")
        {
            ctx.description = "Calculate Ch:0 - Ch:1 and write it to Ch:2\nCalculate Ch:1 - Ch:0 and write it to Ch:3";
            ctx.cpus = 1;

            std::vector<uint16_t> input1;
            std::vector<uint16_t> input2;
            std::vector<uint16_t> output1;
            std::vector<uint16_t> output2;

            for(int i=0; i < 256; i++)
            {
                uint16_t in1 = lcg(seed);
                uint16_t in2 = lcg(seed);

                input1.push_back(in1);
                input2.push_back(in2);

                output1.push_back(in1 - in2);
                output2.push_back(in2 - in1);
            }

            ctx.channel_to_input[0] = input1;
            ctx.channel_to_input[1] = input2;
            ctx.channel_to_output[2] = output1;
            ctx.channel_to_output[3] = output2;
        }*/

        if(ctx.level_name == "DIVISIONS")
        {
            ctx.description = "Calculate Ch:0 / 4 and write it to Ch:1\nCalculate Ch:0 / 32 and write it to Ch:2";
            ctx.short_description = "PERFORM DIVISIONS IN SERIES";
            ctx.cpus = 1;

            std::vector<uint16_t> input1;
            std::vector<uint16_t> output1;
            std::vector<uint16_t> output2;

            for(int i=0; i < answer_rough_count; i++)
            {
                uint16_t in1 = lcg(seed);

                input1.push_back(in1);

                output1.push_back(in1 / 4);
                output2.push_back(in1 / 32);
            }

            ctx.channel_to_input[0] = input1;
            ctx.channel_to_output[1] = output1;
            ctx.channel_to_output[2] = output2;

            simple_page_mapping(ctx);
        }

        ///next up, conditionals
        ///then conditional chaining?
        ///then powr

        if(ctx.level_name == "SPACESHIP_OPERATOR")
        {
            ctx.description = "If Ch:0 < 0, write -1 to Ch:1\nIf Ch:0 == 0, write 0 to Ch:1\nIf Ch:0 > 0, write 1 to Ch:1\nMany instructions have signed equivalents for operating on negative values";
            ctx.short_description = "IMPLEMENT THE <=> OPERATOR";
            ctx.cpus = 1;

            std::vector<uint16_t> input1{1, 0, 0xffff};
            std::vector<uint16_t> output1{1, 0, 0xffff};

            for(int i=0; i < answer_rough_count; i++)
            {
                uint16_t in1 = lcg(seed);
                int16_t as_int = (int16_t)in1;

                int16_t res = 0;

                if(as_int < 0)
                    res = -1;

                if(as_int > 0)
                    res = 1;

                input1.push_back(in1);
                output1.push_back(res);
            }

            ctx.channel_to_input[0] = input1;
            ctx.channel_to_output[1] = output1;

            simple_page_mapping(ctx);
        }

        /*if(ctx.level_name == "SORT")
        {
            ctx.description = "Given Ch:0 and Ch:1, write the smaller (unsigned) value to Ch:2, and the larger (unsigned) to Ch:3";
            ctx.cpus = 1;

            std::vector<uint16_t> input1{1};
            std::vector<uint16_t> input2{0};
            std::vector<uint16_t> output1{0};
            std::vector<uint16_t> output2{1};

            for(int i=0; i < 256; i++)
            {
                uint16_t in1 = lcg(seed);
                uint16_t in2 = lcg(seed);

                uint16_t out1 = in1 < in2 ? in1 : in2;
                uint16_t out2 = in1 < in2 ? in2 : in1;

                output1.push_back(out1);
                output2.push_back(out2);
            }
        }*/

        if(ctx.level_name == "CHECKSUM")
        {
            ctx.description = "Read Ch:1 number of values from Ch:0, add them together, and write them to Ch:2\n"
                              "Eg: Ch:0 = [7, 6, 5, 4, 9] and Ch:1 = [2, 3], Ch:2 <- (7 + 6), Ch:2 <- (5 + 4 + 9)\n\n"
                              "Use :some_name to create a new label, and SET PC, some_name to jump to it";
            ctx.short_description = "IMPLEMENT THE CHECKSUM ALGORITHM";
            ctx.cpus = 1;

            std::vector<uint16_t> input1{1, 1};
            std::vector<uint16_t> input2{2};
            std::vector<uint16_t> output1{2};

            //ctx.output_to_input_start[0].push_back(0);
            //ctx.output_to_input_start[0].push_back(0);

            //ctx.output_to_input_start[1].push_back(0);
            ctx.output_to_input_start[2][0].push_back(0);
            ctx.output_to_input_start[2][1].push_back(0);

            int current_page = 1;

            for(int i=0; i < answer_rough_count; i++)
            {
                uint16_t in2 = lcg(seed) % 64;

                uint16_t accum = 0;

                int start1 = input1.size();
                int start2 = input2.size();

                for(int kk=0; kk < in2; kk++)
                {
                    uint16_t val = lcg(seed);

                    input1.push_back(val);

                    //ctx.channel_to_line_to_page[0].push_back(current_page);

                    accum += val;
                }

                input2.push_back(in2);
                output1.push_back(accum);

                //ctx.channel_to_line_to_page[1].push_back(current_page);
                //ctx.channel_to_line_to_page[2].push_back(current_page);

                ctx.output_to_input_start[2][0].push_back(start1);
                ctx.output_to_input_start[2][1].push_back(start2);

                current_page++;

                if((int)input1.size() > answer_rough_count)
                    break;
            }

            ctx.channel_to_input[0] = input1;
            ctx.channel_to_input[1] = input2;
            ctx.channel_to_output[2] = output1;
        }

        if(ctx.level_name == "POWR")
        {
            ctx.description = "Power - raise Ch:0 to the power of Ch:1\na^b is the same as a * a * a ... * a, done b times";
            ctx.short_description = "IMPLEMENT THE POWER ALGORITHM";
            ctx.cpus = 1;

            std::vector<uint16_t> input1{15, 15};
            std::vector<uint16_t> input2{1, 2};
            std::vector<uint16_t> output{15, 225};

            for(int i=0; i < answer_rough_count; i++)
            {
                uint16_t in1 = lcg(seed) % 128;
                uint16_t in2 = lcg(seed) % 16;
                //uint16_t out = std::pow(in1, in2);

                uint16_t res = 1;

                for(int i=0; i < in2; i++)
                {
                    res = res * in1;
                }

                //for(int i=1; i < in2)

                input1.push_back(in1);
                input2.push_back(in2);
                output.push_back(res);
            }

            input1.push_back(0);
            input2.push_back(0);
            output.push_back(1);

            ctx.channel_to_input[0] = input1;
            ctx.channel_to_input[1] = input2;
            ctx.channel_to_output[2] = output;

            simple_page_mapping(ctx);
        }

        if(ctx.level_name == "HWENUMERATE")
        {
            ctx.description = "Hardware - Write the number of connected devices (HWN) to Ch:0\nSearch for the clock with hardware id 0x12d0b402\n"
                              "Initialise the clock by sending an interrupt with HWI, with [A=0, B>0]\nConsult the manual for detailed specifications\n"
                              "See https://github.com/20k/dcpu16-specs/blob/master/clock.md for the full specification (this will be in the manual later)";
            ctx.short_description = "ENUMERATE HARDWARE DEVICES AND INITIALISE THE CLOCK";

            ctx.cpus = 1;

            std::vector<uint16_t> output{2};

            dcpu::sim::hardware* dummy = new dcpu::sim::hardware;
            dummy->manufacturer_id = 0xDEADBEEF;
            ctx.inf.hardware.push_back(dummy);
            ctx.inf.hardware.push_back(new dcpu::sim::clock);

            ctx.extra_validation = [](level_context& ctx, dcpu::ide::project_instance& instance)
            {
                assert(ctx.inf.hardware.size() == 2);

                dcpu::sim::hardware* my_clock = ctx.inf.hardware[1];

                dcpu::sim::clock* as_clock = dynamic_cast<dcpu::sim::clock*>(my_clock);

                assert(as_clock);

                if(as_clock->on == false)
                    return true;

                return false;
            };

            ctx.channel_to_output[0] = output;

            simple_page_mapping(ctx);
        }

        if(ctx.level_name == "CONSOLE")
        {
            ctx.description = "Search for the LEM1802 display with hardware id 0x7349f615\n"
                              "Initialise the console by sending an interrupt with HWI, with [A=0, B=0x8000]\n"
                              "Write the exact string \"Hello World!\", in ascii, in any colour other than black\n"
                              "Consult https://github.com/20k/dcpu16-specs/blob/master/lem1802.md for more details";
            ctx.short_description = "INITIALISE THE CONSOLE AND DISPLAY HELLO WORLD";

            ctx.cpus = 1;

            dcpu::sim::hardware* LEM = new dcpu::sim::LEM1802;
            ctx.inf.hardware.push_back(LEM);
            ctx.real_world_context.memory.push_back({});

            ctx.extra_validation = [](level_context& ctx, dcpu::ide::project_instance& instance)
            {
                if(instance.editors.size() == 0)
                    return true;

                dcpu::sim::hardware* my_screen = ctx.inf.hardware[0];

                dcpu::sim::LEM1802* as_lem = dynamic_cast<dcpu::sim::LEM1802*>(my_screen);

                assert(as_lem);

                if(as_lem->vram_map == 0)
                    return true;

                bool correctly_set = true;

                std::string target = "Hello World!";

                uint16_t len = target.size();

                for(uint16_t idx = 0; idx < len; idx++)
                {
                    uint16_t addr = idx + as_lem->vram_map;

                    uint8_t c = instance.editors[0].c.mem[addr] & 0b1111111;

                    if(c != target[idx])
                        correctly_set = false;
                }

                return !correctly_set;
            };

            simple_page_mapping(ctx);
        }

        if(ctx.level_name == "SANDBOX")
        {
            ctx.description = "You've got a clock and a LEM1802 to play about with";
            ctx.short_description = "CLOCK AND LEM SANDBOX";

            ctx.cpus = 1;

            dcpu::sim::hardware* LEM = new dcpu::sim::LEM1802;
            ctx.inf.hardware.push_back(LEM);
            dcpu::sim::hardware* clock = new dcpu::sim::clock;
            ctx.inf.hardware.push_back(clock);

            ctx.real_world_context.memory.push_back({});

            simple_page_mapping(ctx);
        }

        return ctx;
    }

    bool setup_validation(level_context& ctx, dcpu::ide::project_instance& instance)
    {
        for(auto& i : ctx.inf.hardware)
        {
            i->reset();
        }

        //ctx.real_world_context = world_context();

        ctx.successful_validation = false;
        ctx.finished = false;
        ctx.found_output.clear();
        ctx.error_locs.clear();
        ctx.error_channels.clear();

        ctx.inf.fab = dcpu::sim::fabric();

        ctx.inf.input_cpus.clear();
        ctx.inf.output_cpus.clear();

        ctx.inf.input_translation.clear();
        ctx.inf.output_translation.clear();

        ctx.inf.cycle = 0;

        for(auto& [channel, vec] : ctx.channel_to_input)
        {
            ctx.inf.input_cpus[channel] = sim_input(vec, channel, ctx.inf.input_translation[channel]);
        }

        for(auto& [channel, vec] : ctx.channel_to_output)
        {
            ctx.inf.output_cpus[channel] = sim_output(vec.size(), channel, ctx.inf.output_translation[channel]);
        }

        bool has_error = false;

        for(dcpu::ide::editor& edit : instance.editors)
        {
            if(edit.assemble())
                has_error = true;
        }

        ctx.has_assembly_error = has_error;

        return has_error;
    }

    ///TODO: Need to handle world time for clock
    bool step_validation(level_context& ctx, dcpu::ide::project_instance& instance, int cycles)
    {
        ctx.found_output.clear();
        ctx.error_locs.clear();
        ctx.error_channels.clear();

        std::vector<dcpu::sim::CPU*> user;

        for(dcpu::ide::editor& edit : instance.editors)
        {
            dcpu::sim::CPU& next = edit.c;

            user.push_back(&next);
        }

        stack_vector<dcpu::sim::CPU*, 64> cpus;

        ///this must come first
        for(auto& i : user)
        {
            cpus.push_back(i);
        }

        for(auto& [channel, sim] : ctx.inf.input_cpus)
        {
            cpus.push_back(&sim);
        }

        for(auto& [channel, sim] : ctx.inf.output_cpus)
        {
            cpus.push_back(&sim);
        }

        stack_vector<dcpu::sim::hardware*, 65536> all_hardware;

        for(auto& i : ctx.inf.hardware)
        {
            all_hardware.push_back(i);
        }

        int max_cycles = cycles;

        for(int i=0; i < max_cycles; i++)
        {
            for(int kk=0; kk < (int)cpus.size(); kk++)
            {
                if(kk < (int)user.size())
                {
                    instance.editors[kk].halted = instance.editors[kk].halted || cpus[kk]->cycle_step(&ctx.inf.fab, &all_hardware, &ctx.real_world_context);
                }
                else
                {
                    cpus[kk]->cycle_step(&ctx.inf.fab, &all_hardware, &ctx.real_world_context);
                }

            }

            dcpu::sim::resolve_interprocessor_communication(cpus, ctx.inf.fab);
        }

        {
            assert(user.size() >= 1);

            int screen_idx = 0;

            for(int i=0; i < (int)ctx.inf.hardware.size(); i++)
            {
                dcpu::sim::hardware* hw = ctx.inf.hardware[i];

                bool is_lem = hw->hardware_id == 0x7349f615;

                if(!is_lem)
                    continue;

                auto& rendering = ctx.real_world_context.memory.at(screen_idx);

                dcpu::sim::LEM1802* as_lem = dynamic_cast<dcpu::sim::LEM1802*>(hw);

                assert(as_lem);

                as_lem->render(&ctx.real_world_context, *user.front(), rendering);

                screen_idx++;
            }
        }

        for(auto& [channel, sim] : ctx.inf.output_cpus)
        {
            const std::vector<uint16_t>& output_val = ctx.channel_to_output[channel];

            std::vector<uint16_t> found;

            check_output(sim, output_val, found);

            for(int i=0; i < (int)found.size() && i < (int)output_val.size(); i++)
            {
                if(found[i] != output_val[i])
                {
                    ctx.error_locs.push_back(i);
                    ctx.error_channels.push_back(channel);
                }
            }

            ctx.found_output[channel] = found;
        }

        bool hardware_errors = false;

        if(ctx.extra_validation != nullptr)
        {
            ///todo: DEBUGGING
            if(ctx.extra_validation(ctx, instance))
            {
                hardware_errors = true;
            }
        }

        bool any_errors_at_all = hardware_errors || ctx.error_locs.size() > 0 || ctx.has_assembly_error;

        ctx.successful_validation = !any_errors_at_all;

        return any_errors_at_all;
    }

    stats validate(level_context& ctx, dcpu::ide::project_instance& instance)
    {
        ctx.found_output.clear();
        ctx.error_locs.clear();
        ctx.error_channels.clear();

        std::string name = ctx.level_name;

        ctx = level::start(name, 256);

        stats rstat;

        bool err1 = setup_validation(ctx, instance);

        bool err2 = step_validation(ctx, instance, 1000000);

        ctx.finished = true;

        rstat.success = ctx.error_locs.size() == 0 && !err2 && !err1;
        ctx.successful_validation = rstat.success;

        return rstat;
    }
}
