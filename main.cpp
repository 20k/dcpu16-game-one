#include <imgui/imgui.h>
#include <toolkit/render_window.hpp>
#include <vec/vec.hpp>
#include <dcpu16-sim/base_sim.hpp>
#include <dcpu16-asm/base_asm.hpp>
#include <dcpu16-ide/base_ide.hpp>
#include <SFML/System.hpp>
#include <toolkit/fs_helpers.hpp>
#include "levels.hpp"
#include <iostream>
#include <filesystem>
#include <set>
#include <chrono>
#include "constant_time_exec.hpp"

/*:start

RCV X, 0
SND X, 1

SET PC, start
*/

///TODO:
///Need the ability to handle inputs and outputs of different respective sizes
///Need to highlight next value to read, and write. This may fix the above

///Need the dummy test data + real data separation

std::string to_hex(uint16_t val)
{
    std::stringstream str;

    str << std::hex << val;

    std::string rval = str.str();

    for(int i=(int)rval.size(); i < 4; i++)
    {
        rval = "0" + rval;
    }

    return "0x" + rval;
}

std::string format(int val, bool hex)
{
    return hex ? to_hex(val) : std::to_string(val);
}

void format_column(int channel, const std::vector<uint16_t>& values, int offset, int count, const std::vector<int>& highlight, bool is_hex, bool use_signed)
{
    ImGui::BeginGroup();

    ImGui::Text("Ch: %i\n", channel);

    std::set<int> highlight_set;

    for(auto i : highlight)
    {
        highlight_set.insert(i);
    }

    std::string formatted;

    /*for(auto v : values)
    {
        formatted += std::to_string(v) + "\n";
    }*/

    if(offset < 0)
        offset = 0;

    for(int i=offset; i < (int)values.size() && i < (offset + count); i++)
    {
        /*if(i != highlight)
            ImGui::Text("%i", values[i]);
        else
            ImGui::TextColored(ImVec4(255, 0, 0, 255), "%i", values[i]);*/

        int32_t val = use_signed ? (int32_t)(int16_t)values[i] : (int32_t)values[i];

        if(highlight_set.find(i) == highlight_set.end())
            formatted += format(val, is_hex) + "\n";
        else
            formatted += format(val, is_hex) + " <\n";
    }

    ImGui::Text(formatted.c_str());

    ImGui::EndGroup();
}

int main()
{
    render_settings sett;
    sett.width = 1200;
    sett.height = 800;
    sett.viewports = true;

    render_window win(sett, "DCPU16-GAME-ONE");

    run_context ctx;
    dcpu::ide::project_instance current_project;

    std::filesystem::create_directory("saves/");

    dcpu::ide::reference_card card;

    bool is_hex = true;
    bool use_signed = false;

    while(!win.should_close())
    {
        win.poll();

        card.render();

        level::display_level_select(ctx, current_project);

        ImGui::Begin("Level", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Text("%s", ctx.ctx.description.c_str());

        ImGui::Text("Level %s", ctx.ctx.level_name.c_str());

        ImGui::Text("Validation Status:");

        ImGui::SameLine();

        if(!ctx.ctx.successful_validation)
        {
            ImGui::TextColored(ImVec4(255, 0, 0, 255), "Invalid");
        }
        else
        {
            ImGui::TextColored(ImVec4(0, 255, 0, 255), "Valid");
        }

        ImGui::End();

        {
            uint64_t now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()).time_since_epoch().count();

            bool any_wants_step = false;
            bool any_wants_assemble = false;
            bool any_wants_run = false;
            bool any_wants_pause = false;
            bool any_wants_reset = false;

            for(dcpu::ide::editor& edit : current_project.editors)
            {
                if(edit.wants_step)
                    any_wants_step = true;

                if(edit.wants_assemble)
                    any_wants_assemble = true;

                if(edit.wants_run)
                    any_wants_run = true;

                if(edit.wants_pause)
                    any_wants_pause = true;

                if(edit.wants_reset)
                    any_wants_reset = true;

                edit.is_running = (ctx.exec.max_cycles == (uint64_t)-1);

                edit.wants_run = false;
                edit.wants_assemble = false;
                edit.wants_step = false;
                edit.wants_pause = false;
                edit.wants_reset = false;
            }

            if(any_wants_assemble || any_wants_reset)
            {
                ctx.ctx = level::start(ctx.ctx.level_name, 256);

                level::setup_validation(ctx.ctx, current_project);

                ctx.exec.init(0, now_ms);
            }

            if(any_wants_pause)
            {
                ctx.exec.init(0, now_ms);
            }

            if(any_wants_step)
            {
                ctx.exec.init(1, now_ms);
            }

            if(any_wants_run)
            {
                ctx.exec.init(-1, now_ms);
            }

            for(dcpu::ide::editor& edit : current_project.editors)
            {
                if(edit.dirty_frequency)
                {
                    edit.dirty_frequency = false;
                    ctx.exec.cycles_per_s = edit.clock_hz;
                }
            }

            for(dcpu::ide::editor& edit : current_project.editors)
            {
                edit.clock_hz = ctx.exec.cycles_per_s;
            }

            ctx.exec.exec_until(now_ms, [&](uint64_t cycle_idx, uint64_t time_ms)
            {
                ctx.ctx.real_world_context.time_ms = time_ms;
                level::step_validation(ctx.ctx, current_project, 1);
            });
        }

        ImGui::Begin("Task", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Checkbox("Hex", &is_hex);
        ImGui::SameLine();
        ImGui::Checkbox("Signed", &use_signed);

        ImGui::BeginGroup();

        ImGui::Text("In");

        int start_error_line = ctx.ctx.error_locs.size() > 0 ? (ctx.ctx.error_locs.front() - 8) : 0;

        if(!ctx.ctx.finished)
            start_error_line = 0;

        for(auto& [channel, vals] : ctx.ctx.channel_to_input)
        {
            int my_line = start_error_line;

            ///the -1 requires some explaining
            ///basically, the blocking multiprocessor instructions
            ///block on the *next* instruction, not the current one
            ///which makes sense logically, but results in bad debuggability
            ///this is a hack
            if(!ctx.ctx.finished)
                my_line = ctx.ctx.inf.input_translation[channel][ctx.ctx.inf.input_cpus[channel].regs[PC_REG]] - 1;

            if(my_line < 0)
                my_line = 0;

            std::vector<int> to_highlight;

            if(!ctx.ctx.finished)
                to_highlight.push_back(my_line);

            format_column(channel, vals, start_error_line, 16, to_highlight, is_hex, use_signed);

            ImGui::SameLine();
        }

        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();

        ImGui::Text("Out");

        for(auto& [channel, vals] : ctx.ctx.channel_to_output)
        {
            format_column(channel, vals, start_error_line, 16, {}, is_hex, use_signed);

            ImGui::SameLine();
        }

        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();

        ImGui::Text("User");

        for(auto& [channel, vals] : ctx.ctx.channel_to_output)
        {
            if(auto it = ctx.ctx.found_output.find(channel); it != ctx.ctx.found_output.end())
            {
                format_column(channel, it->second, start_error_line, 16, ctx.ctx.error_locs, is_hex, use_signed);

                ImGui::SameLine();
            }
        }

        ImGui::EndGroup();

        ImGui::End();

        //for(auto& i : current_project.editors)
        for(int i=0; i < (int)current_project.editors.size(); i++)
        {
            current_project.editors[i].render(current_project, i);
        }

        sf::sleep(sf::milliseconds(1));

        win.display();
    }

    return 0;
}
