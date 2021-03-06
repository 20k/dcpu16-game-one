#ifndef STYLE_HPP_INCLUDED
#define STYLE_HPP_INCLUDED

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

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

        //dim.x += size.x;
        //dim.y += size.y * 2;

        dim.y -= size.y/4;

        int xcount = (dim.x + size.x) / size.x;
        int ycount = (dim.y + size.y) / size.y;

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

                /*if(i == 0 && j == (int)ycount - 1)
                {
                    text += "\u2514";
                    continue;
                }*/

                ///top right
                /*if(i == (int)xcount - 1 && j == 0)
                {
                    text += "\u2556";
                    continue;
                }*/

                ///bottom right
                /*if(i == (int)xcount - 1 && j == (int)ycount - 1)
                {
                    text += "\u255D";
                    continue;
                }*/

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

        auto col = IM_COL32(0xCF, 0xCF, 0xCF, 0xFF);

        float character_y_end = (ycount - 1) * size.y + pos.y;
        float floating_y_end = window_pos.y + dim.y;

        float character_x_end = (xcount - 1) * size.x + pos.x;
        float floating_x_end = window_pos.x + dim.x - 1;

        ImVec2 tl = ImVec2(pos.x, pos.y);
        ImVec2 br = ImVec2(floating_x_end + size.x, floating_y_end + size.y);

        ImGui::PushClipRect(tl, br, false);

        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        draw_list->AddRectFilled(tl, br, IM_COL32(0, 0, 0, 255));

        draw_list->AddText(ImVec2(pos.x, character_y_end), col, "\u2502");
        draw_list->AddText(ImVec2(pos.x, floating_y_end - size.y), col, "\u2502");

        draw_list->AddText(ImVec2(character_x_end, floating_y_end), col, "\u2550");
        draw_list->AddText(ImVec2(floating_x_end - size.x, floating_y_end), col, "\u2550");

        draw_list->AddText(ImVec2(floating_x_end, character_y_end), col, "\u2551");
        draw_list->AddText(ImVec2(floating_x_end, floating_y_end - size.y), col, "\u2551");

        draw_list->AddText(ImVec2(character_x_end, pos.y), col, "\u2500");
        draw_list->AddText(ImVec2(floating_x_end - size.x, pos.y), col, "\u2500");

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

        draw_list->AddText(pos, IM_COL32(0xCF, 0xCF, 0xCF, 0xFF), text.c_str());

        ImVec2 bottom_pos = ImVec2(window_pos.x - size.x - 1, window_pos.y + dim.y);

        draw_list->AddText(bottom_pos, IM_COL32(0xCF, 0xCF, 0xCF, 0xFF), bottom_line.c_str());

        ImVec2 right_pos = ImVec2(window_pos.x - 1 + dim.x, window_pos.y - size.y/2);

        draw_list->AddText(right_pos, IM_COL32(0xCF, 0xCF, 0xCF, 0xFF), right_line.c_str());

        ImVec2 bottom_right_pos = ImVec2(window_pos.x - 1 + dim.x, window_pos.y + dim.y);

        draw_list->AddText(bottom_right_pos, IM_COL32(0xCF, 0xCF, 0xCF,0xFF), bottom_right.c_str());
    }

    inline
    void finish()
    {
        ImGui::PopClipRect();
    }

    inline
    void push_styles()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);
    }

    inline
    void push_resizablewindow_style()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, ImVec4(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0,0,0,1));
        ImGui::PushStyleColor(ImGuiCol_ResizeGrip, ImVec4(0xCF/255.f,0xCF/255.f,0xCF/255.f,1));
        ImGui::PushStyleColor(ImGuiCol_ResizeGripActive, ImVec4(1,1,1,1));
        ImGui::PushStyleColor(ImGuiCol_ResizeGripHovered, ImVec4(1,1,1,1));
    }

    inline
    void pop_styles()
    {
        ImGui::PopStyleVar(3);
    }

    inline
    void pop_resizablewindow_style()
    {
        ImGui::PopStyleColor(9);
        ImGui::PopStyleVar();
    }

    inline
    void title_separator()
    {
        auto window_dim = ImGui::GetWindowSize();

        int char_num = (window_dim.x - ImGui::GetStyle().FramePadding.x*2) / ImGui::CalcTextSize(" ").x;

        char_num -= 1;

        if(char_num <= 0)
            char_num = 0;

        std::string spacer(char_num, '-');

        auto cursor_pos = ImGui::GetCursorScreenPos();

        ImGui::GetCurrentWindow()->DrawList->AddText(ImVec2(cursor_pos.x, cursor_pos.y - ImGui::CalcTextSize(" ").y/2), IM_COL32(0xCF, 0xCF, 0xCF, 0xFF), spacer.c_str());

        ImGui::NewLine();
    }

    inline
    void text_separator()
    {
        auto window_dim = ImGui::GetWindowSize();

        int char_num = (window_dim.x - ImGui::GetStyle().FramePadding.x*2) / ImGui::CalcTextSize(" ").x;

        char_num -= 1;

        if(char_num <= 0)
            char_num = 0;

        std::string spacer(char_num, '-');

        auto cursor_pos = ImGui::GetCursorScreenPos();

        ImGui::GetCurrentWindow()->DrawList->AddText(ImVec2(cursor_pos.x, cursor_pos.y), IM_COL32(0xCF, 0xCF, 0xCF, 0xFF), spacer.c_str());

        ImGui::NewLine();
    }
}

#endif // STYLE_HPP_INCLUDED
