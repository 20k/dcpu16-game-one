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

void format_column(int channel, const std::vector<uint16_t>& values, int offset, int count, const std::vector<int>& highlight)
{
    ImGui::BeginGroup();

    ImGui::Text("C: %i\n", channel);

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

        if(highlight_set.find(i) == highlight_set.end())
            formatted += std::to_string(values[i]) + "\n";
        else
            formatted += std::to_string(values[i]) + " <\n";
    }

    ImGui::Text(formatted.c_str());

    ImGui::EndGroup();
}

int main()
{
    render_settings sett;
    sett.width = 1200;
    sett.height = 800;

    render_window win(sett, "DCPU16-GAME-ONE");

    level_context clevel;
    dcpu::ide::project_instance current_project;

    std::filesystem::create_directory("saves/");

    dcpu::ide::reference_card card;

    while(!win.should_close())
    {
        win.poll();

        card.render();

        ImGui::Begin("Levels", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        std::vector<int> lvl = level::get_available();

        for(int i=0; i < (int)lvl.size(); i++)
        {
            if(ImGui::Button(("LVL: " + std::to_string(lvl[i])).c_str()))
            {
                std::filesystem::create_directory("saves/" + std::to_string(lvl[i]));

                std::string full_filename = "saves/" + std::to_string(lvl[i]) + "/save.dcpu_project";

                std::cout << "DOOT " << current_project.editors.size() << " TWO " << current_project.proj.assembly_data.size() << std::endl;

                if(current_project.proj.project_file.size() > 0)
                    current_project.save();

                current_project = dcpu::ide::project_instance();

                if(file::exists(full_filename))
                {
                    current_project.load(full_filename);
                }
                else
                {
                    current_project.proj.project_file = full_filename;
                    current_project.proj.assembly_files = {"saves/" + std::to_string(lvl[i]) + "/cpu0.d16"};
                    current_project.proj.assembly_data = {""};

                    current_project.editors.emplace_back();
                }

                clevel = level::start(lvl[i]);
            }
        }

        ImGui::End();

        ImGui::Begin("Level", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Text("Description %s", clevel.description.c_str());

        ImGui::Text("Level %i", clevel.level);

        if(ImGui::Button("Validate"))
        {
            stats s = level::validate(clevel, current_project);

            std::cout << "VALID? " << s.success << std::endl;
        }

        ImGui::End();

        ImGui::Begin("Task", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::BeginGroup();

        ImGui::Text("In");

        int start_error_line = clevel.error_locs.size() > 0 ? (clevel.error_locs.front() - 8) : 0;

        for(auto& [channel, vals] : clevel.channel_to_input)
        {
            format_column(channel, vals, start_error_line, 16, {});

            ImGui::SameLine();
        }

        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();

        ImGui::Text("Out");

        for(auto& [channel, vals] : clevel.channel_to_output)
        {
            format_column(channel, vals, start_error_line, 16, {});

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
                format_column(channel, it->second, start_error_line, 16, clevel.error_locs);

                ImGui::SameLine();
            }
        }

        ImGui::EndGroup();

        ImGui::End();

        for(auto& i : current_project.editors)
        {
            i.render();
        }

        sf::sleep(sf::milliseconds(1));

        win.display();
    }

    return 0;
}
