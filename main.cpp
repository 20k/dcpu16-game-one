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

    level_context clevel;
    dcpu::ide::project_instance current_project;

    std::filesystem::create_directory("saves/");

    dcpu::ide::reference_card card;

    int step_amount = 0;

    bool is_hex = true;
    bool use_signed = false;

    while(!win.should_close())
    {
        win.poll();

        card.render();

        ImGui::Begin("Levels", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        std::vector<std::string> lvl = level::get_available();

        for(int i=0; i < (int)lvl.size(); i++)
        {
            if(ImGui::Button(("LVL: " + lvl[i]).c_str()))
            {
                std::filesystem::create_directory("saves/" + lvl[i]);

                std::string full_filename = "saves/" + lvl[i] + "/save.dcpu_project";

                std::cout << "DOOT " << current_project.editors.size() << " TWO " << current_project.proj.assembly_data.size() << std::endl;

                if(current_project.proj.project_file.size() > 0)
                {
                    current_project.save();
                }

                current_project = dcpu::ide::project_instance();

                if(file::exists(full_filename))
                {
                    current_project.load(full_filename);
                }
                else
                {
                    current_project.proj.project_file = full_filename;
                    current_project.proj.assembly_files = {"saves/" + lvl[i] + "/cpu0.d16"};
                    current_project.proj.assembly_data = {""};

                    current_project.editors.emplace_back();
                }

                clevel = level::start(lvl[i], 12);

                level::setup_validation(clevel, current_project);
            }
        }

        ImGui::End();

        ImGui::Begin("Level", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Text("%s", clevel.description.c_str());

        ImGui::Text("Level %s", clevel.level_name.c_str());

        ImGui::Text("Validation Status:");

        ImGui::SameLine();

        if(!clevel.successful_validation)
        {
            ImGui::TextColored(ImVec4(255, 0, 0, 255), "Invalid");
        }
        else
        {
            ImGui::TextColored(ImVec4(0, 255, 0, 255), "Valid");
        }

        if(ImGui::Button("Validate"))
        {
            stats s = level::validate(clevel, current_project);

            std::cout << "VALID? " << s.success << std::endl;
        }

        if(ImGui::Button("Reset"))
        {
            clevel = level::start(clevel.level_name, 12);

            level::setup_validation(clevel, current_project);
        }

        if(ImGui::Button("Step Puzzle"))
        {
            level::step_validation(clevel, current_project, step_amount);
        }

        ImGui::InputInt("Step Amount", &step_amount);

        if(step_amount < 1)
            step_amount = 1;

        ImGui::End();

        ImGui::Begin("Task", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Checkbox("Hex", &is_hex);
        ImGui::SameLine();
        ImGui::Checkbox("Signed", &use_signed);

        ImGui::BeginGroup();

        ImGui::Text("In");

        int start_error_line = clevel.error_locs.size() > 0 ? (clevel.error_locs.front() - 8) : 0;

        if(!clevel.finished)
            start_error_line = 0;

        for(auto& [channel, vals] : clevel.channel_to_input)
        {
            int my_line = start_error_line;

            ///the -1 requires some explaining
            ///basically, the blocking multiprocessor instructions
            ///block on the *next* instruction, not the current one
            ///which makes sense logically, but results in bad debuggability
            ///this is a hack
            if(!clevel.finished)
                my_line = clevel.inf.input_translation[channel][clevel.inf.input_cpus[channel].regs[PC_REG]] - 1;

            if(my_line < 0)
                my_line = 0;

            std::vector<int> to_highlight;

            if(!clevel.finished)
                to_highlight.push_back(my_line);

            format_column(channel, vals, start_error_line, 16, to_highlight, is_hex, use_signed);

            ImGui::SameLine();
        }

        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();

        ImGui::Text("Out");

        for(auto& [channel, vals] : clevel.channel_to_output)
        {
            format_column(channel, vals, start_error_line, 16, {}, is_hex, use_signed);

            ImGui::SameLine();
        }

        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();

        ImGui::Text("User");

        for(auto& [channel, vals] : clevel.channel_to_output)
        {
            if(auto it = clevel.found_output.find(channel); it != clevel.found_output.end())
            {
                format_column(channel, it->second, start_error_line, 16, clevel.error_locs, is_hex, use_signed);

                ImGui::SameLine();
            }
        }

        ImGui::EndGroup();

        ImGui::End();

        for(auto& i : current_project.editors)
        {
            i.render(current_project);
        }

        sf::sleep(sf::milliseconds(1));

        win.display();
    }

    return 0;
}
