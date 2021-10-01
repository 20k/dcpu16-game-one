#include <imgui/imgui.h>
#include <toolkit/render_window.hpp>
#include <vec/vec.hpp>
#include <dcpu16-sim/base_sim.hpp>
#include <dcpu16-asm/base_asm_fwd.hpp>
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
//#include <dcpu16-asm/base_asm.hpp>
#include "level_data.hpp"

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

void format_column(int channel, const std::vector<uint16_t>& values, int offset, int count, const std::vector<int>& highlight, const std::vector<int>& errors, bool is_hex, bool use_signed)
{
    ImGui::BeginGroup();

    ImGui::Text("Ch: %i\n", channel);

    std::set<int> highlight_set;
    //std::set<int> error_set;

    for(auto i : highlight)
    {
        highlight_set.insert(i);
    }

    /*for(auto i : errors)
    {
        error_set.insert(i);
    }*/

    if(offset < 0)
        offset = 0;

    for(int i=offset; i < (int)values.size() && i < (offset + count); i++)
    {
        int32_t val = use_signed ? (int32_t)(int16_t)values[i] : (int32_t)values[i];

        std::string base_str = format_hex_or_dec(val, is_hex, use_signed);

        /*bool is_err = error_set.find(i) != error_set.end();

        if(is_err)
        {
            auto pos = ImGui::GetCursorScreenPos();

            pos.x += ImGui::CalcTextSize((base_str + " ").c_str()).x;

            auto dim = ImGui::CalcTextSize("<");

            ImGui::GetCurrentWindow()->DrawList->AddRectFilled(ImVec2(pos.x, pos.y), ImVec2(pos.x + dim.x, pos.y + dim.y), IM_COL32(255, 128, 128, 255));
        }*/

        if(highlight_set.find(i) == highlight_set.end())
            ImGui::TextUnformatted(base_str.c_str());
        else
            ImGui::TextUnformatted((base_str + " <").c_str());
    }

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

int page_round(int in)
{
    return (in/16) * 16;
}

int main()
{
    //symbol_table sym;

    /*std::optional<expression_result> res = parse_expression(sym, "A + 2 + 3 * 4");

    assert(res.has_value());

    assert(res->which_register.has_value());
    assert(res->op.has_value());
    assert(res->word.has_value());

    std::cout << res->which_register.value() << " " << res->op.value() << " " << res->word.value() << std::endl;*/

    render_settings sett;
    sett.width = 1300;
    sett.height = 800;
    sett.viewports = false;

    render_window win(sett, "DCPU16-GAME-ONE");

    ImGui::PushSrgbStyleColor(ImGuiCol_Text, ImVec4(207/255.f, 207/255.f, 207/255.f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_ModalWindowDarkening, ImVec4(0,0,0,0.5));

    ImGui::GetStyle().ItemSpacing.y = 0;

    level_selector_state select;
    level_over_state level_over;

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

    //run_context ctx;
    dcpu::ide::project_instance current_project;

    level_manager levels;
    levels.load();

    std::filesystem::create_directory("saves/");

    dcpu::ide::reference_card card;

    bool is_hex = true;
    bool use_signed = false;

    while(!win.should_close())
    {
        win.poll();

        int level_window_bottom = 20;

        if(levels.should_return_to_main_menu)
        {
            levels.current_level = std::nullopt;
        }

        if(!levels.current_level.has_value())
        {
            levels.display_level_select(current_project);
        }
        else
        {
            std::string name = levels.current_level.value().data.name;
            std::string description = levels.current_level.value().data.description;

            ImGui::Begin("Level", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove);

            level_window_bottom = ImGui::GetWindowSize().y + ImGui::GetWindowPos().y;

            style::start();
            style::push_styles();

            auto screen_size = win.get_window_size();

            ImVec2 dim = ImGui::GetWindowSize();

            ImVec2 offset = {0,0};

            if(sett.viewports)
            {
                offset = {ImGui::GetMainViewport()->Pos.x, ImGui::GetMainViewport()->Pos.y};
            }

            offset.y += ImGui::CalcTextSize("\n").y;

            ImGui::SetWindowPos(ImVec2(screen_size.x()/2 - dim.x/2 + offset.x, offset.y));

            ImGui::Text("Level: %s", name.c_str());

            ImGui::PushTextWrapPos(800);
            ImGui::Text("%s", description.c_str());
            ImGui::PopTextWrapPos();

            if(ImGui::Selectable("> Back"))
            {
                ///saves
                levels.save_current(current_project);
                levels.back_to_main_menu();
            }

            style::pop_styles();
            style::finish();

            ImGui::End();

            level_instance& current_instance = levels.current_level.value();

            {
                uint64_t now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()).time_since_epoch().count();

                bool any_wants_step = false;
                bool any_wants_assemble = false;
                bool any_wants_run = false;
                bool any_wants_pause = false;
                bool any_wants_reset = false;
                bool any_halted = false;

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

                    if(edit.halted)
                        any_halted = true;

                    edit.is_running = (current_instance.runtime_data.exec.max_cycles == (uint64_t)-1);

                    edit.wants_run = false;
                    edit.wants_assemble = false;
                    edit.wants_step = false;
                    edit.wants_pause = false;
                    edit.wants_reset = false;
                }

                for(dcpu::ide::editor& edit : current_project.editors)
                {
                    if(any_wants_run || any_wants_assemble || any_wants_step || any_wants_reset)
                    {
                        edit.halted = false;
                        any_halted = false;
                    }

                    if(any_halted)
                    {
                        edit.is_running = false;
                    }
                }

                if(any_wants_assemble || any_wants_reset)
                {
                    levels.reset_level();
                    levels.save_current(current_project);

                    current_instance.runtime_data.exec.init(0, now_ms);
                }

                if(any_wants_pause)
                {
                    current_instance.runtime_data.exec.init(0, now_ms);
                }

                if(any_wants_step)
                {
                    current_instance.runtime_data.exec.init(1, now_ms);
                }

                if(any_wants_run)
                {
                    current_instance.runtime_data.exec.init(-1, now_ms);
                }

                for(dcpu::ide::editor& edit : current_project.editors)
                {
                    if(edit.dirty_frequency)
                    {
                        edit.dirty_frequency = false;
                        current_instance.runtime_data.exec.cycles_per_s = edit.clock_hz;
                    }
                }

                for(dcpu::ide::editor& edit : current_project.editors)
                {
                    edit.clock_hz = current_instance.runtime_data.exec.cycles_per_s;
                }

                current_instance.runtime_data.exec.exec_until(now_ms, [&](uint64_t cycle_idx, uint64_t time_ms)
                {
                    if(any_halted)
                        return;

                    current_instance.runtime_data.real_world_state.time_ms = time_ms;

                    levels.step_validation(current_project);
                });
            }

            if(!current_instance.displayed_level_over && current_instance.runtime_data.current_run_stats.has_value())
            {
                current_instance.displayed_level_over = true;

                level_over.best_stats = level_stats::load_best(current_instance.data.name).value_or(level_stats::info());
                level_over.current_stats = current_instance.runtime_data.current_run_stats.value();

                for(dcpu::ide::editor& e : current_project.editors)
                {
                    e.wants_pause = true;
                }

                ImGui::OpenPopup("LevelFinish");
            }

            //style::push_resizablewindow_style();
            style::push_styles();

            if(ImGui::BeginPopupModal("LevelFinish", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove))
            {
                style::start();

                ImGui::Text("Success");

                style::text_separator();

                level_stats::info& end_stats = level_over.current_stats;

                ImGui::Text("Result:");
                ImGui::Text("CYCLE COUNT       : %i\n", end_stats.cycles);
                ImGui::Text("INSTRUCTION SIZE  : %i\n", end_stats.assembly_length);

                style::text_separator();

                level_stats::info& best_stats = level_over.best_stats;

                std::string end_valid_str = best_stats.valid ? "VALID" : "INVALID";

                ImGui::Text("Best:");
                ImGui::Text("CYCLE COUNT       : %i\n", best_stats.cycles);
                ImGui::Text("INSTRUCTION SIZE  : %i\n", best_stats.assembly_length);
                //ImGui::Text("VALIDATION        : %s\n", end_valid_str.c_str()); //? is it possible for this to be invalid?

                style::text_separator();

                if(ImGui::Selectable("> Menu"))
                {
                    /*level::switch_to_level(ctx, current_project, ctx.ctx.level_name);
                    ctx.ctx.level_name = "";*/

                    levels.save_current(current_project);
                    levels.back_to_main_menu();

                    ImGui::CloseCurrentPopup();
                }

                if(ImGui::Selectable("> Continue"))
                {
                    current_instance.runtime_data.current_run_stats = std::nullopt;
                    current_instance.displayed_level_over = false;

                    ImGui::CloseCurrentPopup();
                }

                style::finish();

                ImGui::EndPopup();
            }

            style::pop_styles();
            //style::pop_resizablewindow_style();

            //ImGui::SetNextWindowPos(ImVec2(level_window_right + ImGui::CalcTextSize(" ").x, level_window_bottom + ImGui::CalcTextSize(" ").y), ImGuiCond_Always);

            ImGui::Begin("Task", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove);

            int my_width = ImGui::GetWindowSize().x;

            if(my_width < 20)
                my_width = 20;

            ImGui::SetWindowPos(ImVec2(win.get_window_size().x() + ImGui::GetMainViewport()->Pos.x - my_width - 1 - ImGui::CalcTextSize(" ").x * 2, level_window_bottom + ImGui::CalcTextSize(" ").y));

            style::start();
            style::push_styles();

            low_checkbox("Hex", is_hex);
            //ImGui::SameLine();
            low_checkbox("Signed", use_signed);

            bool run_is_finished = true;

            for(auto& [channel, vals] : current_instance.constructed_data.channel_to_output)
            {
                if(current_instance.runtime_data.found_output[channel].size() < current_instance.constructed_data.channel_to_output[channel].size())
                    run_is_finished = false;

                //if(ctx.ctx.inf.output_cpus[channel].regs[PC_REG] < (int)ctx.ctx.inf.output_translation[channel].size())
                //    run_is_finished = false;
            }

            if(current_instance.constructed_data.channel_to_input.size() > 0)
            {
                ImGui::BeginGroup();

                ImGui::Text("In      ");

                for(auto& [channel, vals] : current_instance.constructed_data.channel_to_input)
                {
                    int my_line = (int)vals.size() - (int)current_instance.runtime_data.input_queue[channel].size();

                    my_line = std::max(my_line, 0);

                    int offset = my_line - 8;

                    std::vector<int> to_highlight;

                    if(run_is_finished && current_instance.execution_state.error_locs.size() > 0)
                    {
                        int which_error_line = current_instance.execution_state.error_locs.front();
                        int which_channel = current_instance.execution_state.error_channels.front();

                        if(which_error_line >= 0 && which_error_line < (int)current_instance.constructed_data.output_to_input_start[which_channel][channel].size())
                        {
                            int translated_line = current_instance.constructed_data.output_to_input_start[which_channel][channel][which_error_line];

                            to_highlight.push_back(translated_line);

                            offset = translated_line - 8;
                        }
                    }

                    if(!run_is_finished)
                        to_highlight = {my_line};

                    format_column(channel, vals, page_round(offset), 32, to_highlight, {}, is_hex, use_signed);

                    ImGui::SameLine(0, ImGui::CalcTextSize(" ").x);
                }

                ImGui::EndGroup();

                ImGui::SameLine();

                ImGui::Text(" ");

                ImGui::SameLine();
            }

            ImGui::BeginGroup();

            ImGui::Text("Out     ");

            for(auto& [channel, vals] : current_instance.constructed_data.channel_to_output)
            {
                int my_line = (int)vals.size() - (int)current_instance.runtime_data.found_output[channel].size();

                if(my_line < 0)
                    my_line = 0;

                std::vector<int> to_highlight;

                int offset = my_line - 8;

                if(run_is_finished && current_instance.execution_state.error_locs.size() > 0)
                {
                    int which_error_line = current_instance.execution_state.error_locs.front();
                    int which_channel = current_instance.execution_state.error_channels.front();

                    if(which_channel == channel)
                    {
                        offset = which_error_line - 8;
                    }

                    else if(which_error_line >= 0 && which_error_line < (int)current_instance.constructed_data.output_to_input_start[which_channel][channel].size())
                    {
                        offset = current_instance.constructed_data.output_to_input_start[which_channel][channel][which_error_line] - 8;
                    }
                }

                if(!run_is_finished)
                    to_highlight = {my_line};

                format_column(channel, vals, page_round(offset), 32, to_highlight, {}, is_hex, use_signed);

                ImGui::SameLine(0, ImGui::CalcTextSize(" ").x);
            }

            ImGui::EndGroup();

            ImGui::SameLine();

            ImGui::Text(" ");

            ImGui::SameLine();

            ImGui::BeginGroup();

            ImGui::Text("User    ");

            for(auto& [channel, vals] : current_instance.constructed_data.channel_to_output)
            {
                if(auto it = current_instance.runtime_data.found_output.find(channel); it != current_instance.runtime_data.found_output.end())
                {
                    int my_line = (int)vals.size() - (int)current_instance.runtime_data.found_output[channel].size();

                    if(my_line < 0)
                        my_line = 0;

                    int offset = 0;

                    if(run_is_finished)
                    {
                        if(current_instance.execution_state.error_locs.size() > 0)
                        {
                            offset = current_instance.execution_state.error_locs.front() - 8;
                        }
                        else
                        {
                            offset = my_line - 8;
                        }
                    }
                    else
                    {
                        offset = my_line - 8;
                    }

                    format_column(channel, it->second, page_round(offset), 32, current_instance.execution_state.error_locs, current_instance.execution_state.error_locs, is_hex, use_signed);

                    ImGui::SameLine(0, ImGui::CalcTextSize(" ").x);
                }
            }

            ImGui::EndGroup();

            style::pop_styles();
            style::finish();

            ImGui::End();

            //for(auto& i : current_project.editors)
            for(int i=0; i < (int)current_project.editors.size(); i++)
            {
                std::string root_name = "IDE";

                if(current_project.editors[i].unsaved)
                    root_name += " (unsaved)";

                ImGui::SetNextWindowPos(ImVec2(300, 200), ImGuiCond_Appearing);
                ImGui::SetNextWindowSize(ImVec2(500, 500), ImGuiCond_Appearing);

                style::push_resizablewindow_style();

                ImGui::Begin((root_name + "###IDE" + std::to_string(i)).c_str(), nullptr, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar);

                style::start();

                style::title_separator();

                TextEditor::Palette pal = current_project.editors[i].edit->GetPalette();
                pal[(int)TextEditor::PaletteIndex::Background] = IM_COL32(0,0,0,0);
                current_project.editors[i].edit->SetPalette(pal);

                current_project.editors[i].render_inline(current_project, i);

                style::finish();

                ImGui::End();

                if(current_project.editors[i].is_rendering_mem_editor)
                {
                    ImGui::SetNextWindowSize(ImVec2(400, 400), ImGuiCond_Appearing);

                    ImGui::Begin(("MEMEDIT##" + std::to_string(i)).c_str(), nullptr, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar);

                    style::start();

                    style::title_separator();

                    current_project.editors[i].render_memory_editor_inline(current_project, i);

                    style::finish();

                    ImGui::End();
                }

                style::pop_resizablewindow_style();
            }

            ImGui::SetNextWindowPos(ImVec2(20 + ImGui::GetMainViewport()->Pos.x, ImGui::CalcTextSize("\n").y + ImGui::GetMainViewport()->Pos.y), ImGuiCond_Always);

            ImGui::Begin("Reference", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);

            style::start();
            style::push_styles();

            ImGui::Text("Instructions");

            style::text_separator();

            card.render_inline();

            style::pop_styles();
            style::finish();

            ImGui::End();

            {
                int screen_idx = 0;

                int mult = 2;

                for(auto& buffer : current_instance.runtime_data.real_world_state.memory)
                {
                    ImGui::SetNextWindowSize(ImVec2(128 * mult + ImGui::CalcTextSize(" ").x*2, 96 * mult + ImGui::CalcTextSize(" ").y*2), ImGuiCond_Always);

                    ImGui::Begin((std::string("LEM##") + std::to_string(screen_idx)).c_str(), nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration);
                    style::start();
                    style::push_styles();

                    auto cursor_pos = ImGui::GetCursorScreenPos();

                    for(int y=0; y < 96; y++)
                    {
                        for(int x=0; x < 128; x++)
                        {
                            int linear = y * 128 + x;

                            uint32_t col = buffer.at(linear);

                            auto tl = ImVec2(cursor_pos.x + x * mult, cursor_pos.y + y * mult);
                            auto br = ImVec2(cursor_pos.x + x * mult + mult, cursor_pos.y + y * mult + mult);

                            uint8_t r = col >> 24;
                            uint8_t g = col >> 16;
                            uint8_t b = col >> 8;

                            ImGui::GetCurrentWindow()->DrawList->AddRectFilled(tl, br, IM_COL32(r, g, b, 255));
                        }
                    }

                    style::pop_styles();
                    style::finish();
                    ImGui::End();

                    screen_idx++;

                }
            }
        }

        sf::sleep(sf::milliseconds(1));

        win.display();
    }

    if(current_project.editors.size() > 0)
    {
        current_project.save();
    }

    return 0;
}
