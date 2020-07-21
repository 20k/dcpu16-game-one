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
#include <imgui/misc/freetype/imgui_freetype.h>
#include "style.hpp"
#include <imguicolortextedit/texteditor.h>

/*:start

RCV X, 0
SND X, 1

SET PC, start
*/

///TODO:
///Need the ability to handle inputs and outputs of different respective sizes
///Need to highlight next value to read, and write. This may fix the above

///Need the dummy test data + real data separation

static std::string format_hex(uint16_t val, bool is_sign)
{
    std::stringstream str;

    if(!is_sign)
        str << std::uppercase << std::hex << val;
    else
        str << std::uppercase << std::hex << (int16_t)val;

    std::string rval = str.str();

    for(int i=(int)rval.size(); i < 4; i++)
    {
        rval = "0" + rval;
    }

    return "0x" + rval;
}

///formatted to same width as format_hex
static std::string format_dec(uint16_t val, bool is_sign)
{
    std::string out;

    if(!is_sign)
        out = std::to_string(val);
    else
        out = std::to_string((int16_t)val);

    for(int i=(int)out.size(); i < 6; i++)
    {
        out = out + " ";
    }

    return out;
}

static std::string format_hex_or_dec(uint16_t val, bool hex, bool is_sign)
{
    return hex ? format_hex(val, is_sign) : format_dec(val, is_sign);
}

void low_checkbox(const std::string& str, bool& val)
{
    std::string display = val ? "[x] " + str : "[ ] " + str;

    if(ImGui::Selectable(display.c_str()))
        val = !val;
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
            formatted += format_hex_or_dec(val, is_hex, use_signed) + "\n";
        else
            formatted += format_hex_or_dec(val, is_hex, use_signed) + " <\n";
    }

    ImGui::Text(formatted.c_str());

    ImGui::EndGroup();
}

void main_menu(level_selector_state& select, run_context& ctx, dcpu::ide::project_instance& current_project)
{
    level::display_level_select(select, ctx, current_project);
}

/*void squarify()
{
    auto dim = ImGui::GetWindowSize();
    auto pos = ImGui::GetWindowPos();

    ImGui::GetForegroundDrawList()->AddText(pos, 0xFFFFFFFF, "----");
}*/

int main()
{
    render_settings sett;
    sett.width = 1200;
    sett.height = 800;
    sett.viewports = true;

    render_window win(sett, "DCPU16-GAME-ONE");

    ImGui::PushSrgbStyleColor(ImGuiCol_Text, ImVec4(207/255.f, 207/255.f, 207/255.f, 255));

    ImGui::GetStyle().ItemSpacing.y = 0;

    level_selector_state select;

    //ImGui::GetStyle().ScaleAllSizes(2);

    {
        ImFontAtlas* atlas = ImGui::GetIO().Fonts;

        ImFontConfig font_cfg;
        font_cfg.GlyphExtraSpacing = ImVec2(0, 0);

        ImGuiIO& io = ImGui::GetIO();

        io.Fonts->Clear();
        //io.Fonts->ClearFonts();

        static const ImWchar range[] =
        {
            0x0020, 0x00FF, // Basic Latin + Latin Supplement
            0x2500, 0x25EF, // Some extension characters for pipes etc
            0,
        };

        #ifndef __EMSCRIPTEN__
        ///BASE
        io.Fonts->AddFontFromFileTTF("DosFont.ttf", 16, &font_cfg, &range[0]);
        #endif // __EMSCRIPTEN__
        ///TEXT_EDITOR
        //io.Fonts->AddFontFromFileTTF("VeraMono.ttf", editor_font_size, &font_cfg);
        ///DEFAULT
        //io.Fonts->AddFontDefault();

        #ifdef __EMSCRIPTEN__
        io.Fonts->AddFontDefault(); ///kinda hacky
        #endif // __EMSCRIPTEN__

        ImGuiFreeType::BuildFontAtlas(atlas, 0, 1);
    }

    run_context ctx;
    dcpu::ide::project_instance current_project;

    std::filesystem::create_directory("saves/");

    dcpu::ide::reference_card card;

    bool is_hex = true;
    bool use_signed = false;

    while(!win.should_close())
    {
        win.poll();

        if(ctx.ctx.level_name.size() == 0)
        {
            main_menu(select, ctx, current_project);
        }
        else
        {
            int level_window_bottom = 20;

            ImGui::Begin("Level", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove);

            level_window_bottom = ImGui::GetWindowSize().y + ImGui::GetWindowPos().y;

            style::start();

            auto screen_size = win.get_window_size();

            ImVec2 dim = ImGui::GetWindowSize();

            ImVec2 offset = {0,0};

            if(sett.viewports)
            {
                offset = {ImGui::GetMainViewport()->Pos.x, ImGui::GetMainViewport()->Pos.y};
            }

            offset.y += ImGui::CalcTextSize("\n").y;

            ImGui::SetWindowPos(ImVec2(screen_size.x()/2 - dim.x/2 + offset.x, offset.y));


            ImGui::Text("Level: %s", ctx.ctx.level_name.c_str());

            ImGui::PushTextWrapPos(800);
            ImGui::Text("%s", ctx.ctx.description.c_str());
            ImGui::PopTextWrapPos();

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

            if(ImGui::Selectable("> Back"))
            {
                ///saves
                level::switch_to_level(ctx, current_project, ctx.ctx.level_name);
                ctx.ctx.level_name = "";
            }

            style::finish();

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

            //ImGui::SetNextWindowPos(ImVec2(level_window_right + ImGui::CalcTextSize(" ").x, level_window_bottom + ImGui::CalcTextSize(" ").y), ImGuiCond_Always);

            ImGui::Begin("Task", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove);

            int my_width = ImGui::GetWindowSize().x;

            if(my_width < 20)
                my_width = 20;

            ImGui::SetWindowPos(ImVec2(win.get_window_size().x() + ImGui::GetMainViewport()->Pos.x - my_width - 1 - ImGui::CalcTextSize(" ").x * 2, level_window_bottom + ImGui::CalcTextSize(" ").y));

            style::start();

            low_checkbox("Hex", is_hex);
            //ImGui::SameLine();
            low_checkbox("Signed", use_signed);

            int start_error_line = ctx.ctx.error_locs.size() > 0 ? (ctx.ctx.error_locs.front() - 8) : 0;

            if(!ctx.ctx.finished)
                start_error_line = 0;

            if(ctx.ctx.channel_to_input.size() > 0)
            {
                ImGui::BeginGroup();

                ImGui::Text("In");

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

                    ImGui::SameLine(0, ImGui::CalcTextSize(" ").x);
                }

                ImGui::EndGroup();

                ImGui::SameLine();

                ImGui::Text(" ");

                ImGui::SameLine();
            }

            ImGui::BeginGroup();

            ImGui::Text("Out");

            for(auto& [channel, vals] : ctx.ctx.channel_to_output)
            {
                format_column(channel, vals, start_error_line, 16, {}, is_hex, use_signed);

                ImGui::SameLine(0, ImGui::CalcTextSize(" ").x);
            }

            ImGui::EndGroup();

            ImGui::SameLine();

            ImGui::Text(" ");

            ImGui::SameLine();

            ImGui::BeginGroup();

            ImGui::Text("User");

            for(auto& [channel, vals] : ctx.ctx.channel_to_output)
            {
                if(auto it = ctx.ctx.found_output.find(channel); it != ctx.ctx.found_output.end())
                {
                    format_column(channel, it->second, start_error_line, 16, ctx.ctx.error_locs, is_hex, use_signed);

                    ImGui::SameLine(0, ImGui::CalcTextSize(" ").x);
                }
            }

            ImGui::EndGroup();

            style::finish();

            ImGui::End();

            //for(auto& i : current_project.editors)
            for(int i=0; i < (int)current_project.editors.size(); i++)
            {
                std::string root_name = "IDE";

                if(current_project.editors[i].unsaved)
                    root_name += " (unsaved)";

                ImGui::SetNextWindowSize(ImVec2(500, 500), ImGuiCond_Appearing);

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

                ImGui::Begin((root_name + "###IDE" + std::to_string(i)).c_str(), nullptr, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar);

                style::start();

                ImGui::PopStyleVar(3);

                style::separator();

                TextEditor::Palette pal = current_project.editors[i].edit->GetPalette();
                pal[(int)TextEditor::PaletteIndex::Background] = IM_COL32(0,0,0,0);
                current_project.editors[i].edit->SetPalette(pal);

                current_project.editors[i].render_inline(current_project, i);

                //style::finish();

                ImGui::End();

                ImGui::PopStyleColor(9);
                ImGui::PopStyleVar();

                //current_project.editors[i].render(current_project, i);
            }

            ImGui::SetNextWindowPos(ImVec2(20 + ImGui::GetMainViewport()->Pos.x, ImGui::CalcTextSize("\n").y + ImGui::GetMainViewport()->Pos.y), ImGuiCond_Always);

            ImGui::Begin("Reference", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);

            style::start();

            card.render_inline();

            style::finish();

            ImGui::End();
        }

        sf::sleep(sf::milliseconds(1));

        win.display();
    }

    return 0;
}
