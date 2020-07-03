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

/*:start

RCV X, 0
SND X, 1

SET PC, start
*/

int main()
{
    render_settings sett;
    sett.width = 1200;
    sett.height = 800;

    render_window win(sett, "DCPU16-GAME-ONE");

    level_context clevel;

    while(!win.should_close())
    {
        win.poll();

        //card.render();

        ImGui::Begin("Levels", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        std::vector<int> lvl = level::get_available();

        for(int i=0; i < (int)lvl.size(); i++)
        {
            if(ImGui::Button(("LVL: " + std::to_string(lvl[i])).c_str()))
            {
                clevel = level::start(lvl[i]);
            }
        }

        ImGui::End();

        ImGui::Begin("Level", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Text("Description %s", clevel.description.c_str());

        ImGui::Text("Level %i", clevel.level);

        if(ImGui::Button("Validate"))
        {
            stats s = level::validate(clevel);

            std::cout << "VALID? " << s.success << std::endl;
        }

        ImGui::End();

        for(auto& i : clevel.cpus)
        {
            i.render();
        }

        sf::sleep(sf::milliseconds(1));

        win.display();
    }

    return 0;
}
