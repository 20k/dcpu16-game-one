#ifndef STYLE_HPP_INCLUDED
#define STYLE_HPP_INCLUDED

#include <imgui/imgui.h>

namespace style
{
    inline
    void start()
    {
        auto dim = ImGui::GetWindowSize();
        auto pos = ImGui::GetWindowPos();

        auto window_pos = pos;

        std::string text = "";

        auto size = ImGui::CalcTextSize("-");

        pos.x -= size.x;
        //pos.y -= size.y;

        dim.x += size.x * 2;
        //dim.y += size.y * 2;

        int xcount = (dim.x + size.x) / size.x;
        int ycount = (dim.y + size.y) / size.y;
        //ycount++;

        ///find a way to do this with proper box characters
        for(int j=0; j < ycount - 1; j++)
        {
            for(int i=0; i < xcount - 1; i++)
            {
                ///top left
                if(i == 0 && j == 0)
                {
                    text += "\u250C";
                    continue;
                }

                if(i == 0 && j == (int)ycount - 1)
                {
                    text += "\u2514";
                    continue;
                }

                ///top right
                if(i == (int)xcount - 1 && j == 0)
                {
                    text += "\u2556";
                    continue;
                }


                ///bottom right
                if(i == (int)xcount - 1 && j == (int)ycount - 1)
                {
                    text += "\u255D";
                    continue;
                }

                if(j == 0)
                {
                    text += "\u2500";
                    continue;
                }

                if(j == (int)ycount - 1)
                {
                    text += "\u2550";
                    continue;
                }

                if(i == 0)
                {
                    text += "\u2502";
                    continue;
                }

                if(i == (int)xcount - 1)
                {
                    text += "\u2551";
                    continue;
                }

                text += " ";
            }

            text += "\n";
        }

        std::string bottom_line = "\u2514";

        ///render bottom line
        for(int i=1; i < (int)xcount - 1; i++)
        {
            bottom_line += "\u2550";
        }

        std::string right_line = "\u2556\n";

        for(int i=1; i < (int)ycount - 1; i++)
        {
            right_line += "\u2551\n";
        }

        std::string bottom_right = "\u255D";

        //pos.x -= size.x/2;
        pos.x -= 1;
        pos.y -= size.y/2;

        ImGui::GetBackgroundDrawList()->AddText(pos, IM_COL32(0xCF, 0xCF, 0xCF, 0xFF), text.c_str());

        ImVec2 bottom_pos = ImVec2(window_pos.x - size.x - 1, window_pos.y + dim.y);

        ImGui::GetBackgroundDrawList()->AddText(bottom_pos, IM_COL32(0xCF, 0xCF, 0xCF, 0xFF), bottom_line.c_str());

        ImVec2 right_pos = ImVec2(window_pos.x - 1 + dim.x, window_pos.y - size.y/2);

        ImGui::GetBackgroundDrawList()->AddText(right_pos, IM_COL32(0xCF, 0xCF, 0xCF, 0xFF), right_line.c_str());

        ImVec2 bottom_right_pos = ImVec2(window_pos.x - 1 + dim.x, window_pos.y + dim.y);

        ImGui::GetBackgroundDrawList()->AddText(bottom_right_pos, IM_COL32(0xCF, 0xCF, 0xCF,0xFF), bottom_right.c_str());

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);
    }

    inline
    void finish()
    {
        ImGui::PopStyleVar(3);
    }
}

#endif // STYLE_HPP_INCLUDED
