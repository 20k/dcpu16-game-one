#include "levels.hpp"
#include <dcpu16-asm/base_asm.hpp>
#include <iostream>
#include <cmath>

dcpu::sim::CPU sim_input(const std::vector<uint16_t>& input, int channel)
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
    }

    return inc;
}

dcpu::sim::CPU sim_output(int len, int channel)
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

namespace level
{
    std::vector<std::string> get_available()
    {
        return {"INTRO", "AMPLIFY", "POWR"};
    }

    level_context start(const std::string& level_name)
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
                              "Use RCV X, 0 to receive input on Ch 0, and SND X, 1 to send output on Ch 1\n"
                              "Remember to make your program loop by using SET PC, 0";
            ctx.cpus = 1;

            std::vector<uint16_t> input;
            std::vector<uint16_t> output;

            for(int i=0; i < 256; i++)
            {
                input.push_back(i);
                output.push_back(i);
            }

            ctx.channel_to_input[0] = input;
            ctx.channel_to_output[1] = output;
        }

        if(ctx.level_name == "AMPLIFY")
        {
            ctx.description = "Amplify the input - Multiply by 16";
            ctx.cpus = 1;

            std::vector<uint16_t> input;
            std::vector<uint16_t> output;

            for(uint64_t i=0; i < 256; i++)
            {
                uint16_t in = lcg(seed) % (65536 / 32);
                uint16_t out = in * 16;

                input.push_back(in);
                output.push_back(out);
            }

            ctx.channel_to_input[0] = input;
            ctx.channel_to_output[1] = output;
        }

        if(ctx.level_name == "POWR")
        {
            ctx.description = "Power - raise input 1 to the power of input 2\na^b is the same as a * a * a ... * a, done b times";
            ctx.cpus = 1;

            std::vector<uint16_t> input1{15};
            std::vector<uint16_t> input2{1};
            std::vector<uint16_t> output{15};

            for(uint64_t i=0; i < 256; i++)
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
        }

        return ctx;
    }

    void setup_validation(level_context& ctx, dcpu::ide::project_instance& instance)
    {
        ctx.found_output.clear();
        ctx.error_locs.clear();

        ctx.inf.fab = dcpu::sim::fabric();

        ctx.inf.input_cpus.clear();
        ctx.inf.output_cpus.clear();

        ctx.inf.cycle = 0;

        for(auto& [channel, vec] : ctx.channel_to_input)
        {
            ctx.inf.input_cpus[channel] = sim_input(vec, channel);
        }

        for(auto& [channel, vec] : ctx.channel_to_output)
        {
            ctx.inf.output_cpus[channel] = sim_output(vec.size(), channel);
        }

        for(dcpu::ide::editor& edit : instance.editors)
        {
            edit.c = dcpu::sim::CPU();
        }
    }

    void step_validation(level_context& ctx, dcpu::ide::project_instance& instance, int cycles)
    {
        ctx.found_output.clear();
        ctx.error_locs.clear();

        std::vector<dcpu::sim::CPU*> user;

        for(dcpu::ide::editor& edit : instance.editors)
        {
            dcpu::sim::CPU& next = edit.c;

            user.push_back(&next);

            auto [rinfo_opt2, err2] = assemble_fwd(edit.get_text());

            if(!rinfo_opt2.has_value())
            {
                printf("Error %s\n", err2.data());

                return;
            }

            next.load(rinfo_opt2.value().mem, 0);

            edit.translation_map = rinfo_opt2.value().translation_map;
        }

        stack_vector<dcpu::sim::CPU*, 64> cpus;

        for(auto& [channel, sim] : ctx.inf.input_cpus)
        {
            cpus.push_back(&sim);
        }

        for(auto& [channel, sim] : ctx.inf.output_cpus)
        {
            cpus.push_back(&sim);
        }

        for(auto& i : user)
        {
            cpus.push_back(i);
        }

        int max_cycles = cycles;

        for(int i=0; i < max_cycles; i++)
        {
            for(int kk=0; kk < (int)cpus.size(); kk++)
            {
                cpus[kk]->cycle_step(&ctx.inf.fab);
            }

            dcpu::sim::resolve_interprocessor_communication(cpus, ctx.inf.fab);
        }

        for(auto& [channel, sim] : ctx.inf.output_cpus)
        {
            const std::vector<uint16_t>& output_val = ctx.channel_to_output[channel];

            std::vector<uint16_t> found;

            check_output(sim, output_val, found);

            for(int i=0; i < (int)found.size() && i < (int)output_val.size(); i++)
            {
                if(found[i] != output_val[i])
                    ctx.error_locs.push_back(i);
            }

            ctx.found_output[channel] = found;
        }
    }

    ///TODO: Step by step validation, dummy example and
    ///then more comprehensive tests
    stats validate(level_context& ctx, dcpu::ide::project_instance& instance)
    {
        ctx.found_output.clear();
        ctx.error_locs.clear();

        stats rstat;

        setup_validation(ctx, instance);

        step_validation(ctx, instance, 1000000);

        rstat.success = ctx.error_locs.size() == 0;

        return rstat;
    }
}
