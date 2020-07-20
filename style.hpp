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

        std::string text = "";

        auto size = ImGui::CalcTextSize("-");

        pos.x -= size.x;
        //pos.y -= size.y;

        dim.x += size.x * 2;
        //dim.y += size.y * 2;

        int xcount = (dim.x + size.x) / size.x;
        int ycount = (dim.y + size.y) / size.y;

        ///find a way to do this with proper box characters
        for(int j=0; j < ycount; j++)
        {
            for(int i=0; i < xcount; i++)
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

        //pos.x -= size.x/2;
        pos.x -= 1;
        pos.y -= size.y/2;

        ImGui::GetBackgroundDrawList()->AddText(pos, IM_COL32(0xCF, 0xCF, 0xCF, 0xFF), text.c_str());

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
