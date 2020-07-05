#include "levels.hpp"
#include <dcpu16-asm/base_asm.hpp>
#include <iostream>

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

namespace level
{
    std::vector<int> get_available()
    {
        return {0};
    }

    level_context start(int level)
    {
        level_context ctx;
        ctx.level = level;

        if(ctx.level == 0)
        {
            ctx.description = "Intro to the DCPU-16";
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

        return ctx;
    }

    ///TODO: Step by step validation, dummy example and
    ///then more comprehensive tests
    stats validate(level_context& ctx, dcpu::ide::project_instance& instance)
    {
        ctx.found_output.clear();
        ctx.error_locs.clear();

        stats rstat;

        if(ctx.level == 0)
        {
            /*std::vector<uint16_t> in;

            for(int i=0; i < 256; i++)
            {
                in.push_back(i);
            }

            std::vector<uint16_t> out;

            for(auto& i : in)
            {
                out.push_back(i);
            }

            auto inc = sim_input(in);
            auto outc = sim_output(in.size());*/

            std::map<int, dcpu::sim::CPU> inputs;

            for(auto& [channel, vec] : ctx.channel_to_input)
            {
                inputs[channel] = sim_input(vec, channel);
            }

            std::map<int, dcpu::sim::CPU> outputs;

            for(auto& [channel, vec] : ctx.channel_to_output)
            {
                outputs[channel] = sim_output(vec.size(), channel);
            }

            std::vector<dcpu::sim::CPU*> user;

            for(dcpu::ide::editor& edit : instance.editors)
            {
                edit.c = dcpu::sim::CPU();

                dcpu::sim::CPU& next = edit.c;

                user.push_back(&next);

                //dcpu::sim::CPU& next = user.emplace_back();

                auto [rinfo_opt2, err2] = assemble_fwd(edit.get_text());

                if(!rinfo_opt2.has_value())
                {
                    printf("Error %s\n", err2.data());
                    rstat.success = false;

                    return rstat;
                }

                next.load(rinfo_opt2.value().mem, 0);
            }

            stack_vector<dcpu::sim::CPU*, 64> cpus;

            /*cpus.push_back(&inc);
            cpus.push_back(&outc);*/

            for(auto& [channel, sim] : inputs)
            {
                cpus.push_back(&sim);
            }

            for(auto& [channel, sim] : outputs)
            {
                cpus.push_back(&sim);
            }

            for(auto& i : user)
            {
                cpus.push_back(i);
            }

            dcpu::sim::fabric fab;

            int max_cycles = 1000000;

            for(int i=0; i < max_cycles; i++)
            {
                for(int kk=0; kk < (int)cpus.size(); kk++)
                {
                    cpus[kk]->cycle_step(&fab);
                }

                dcpu::sim::resolve_interprocessor_communication(cpus, fab);
            }

            rstat.success = true;

            for(auto& [channel, sim] : outputs)
            {
                const std::vector<uint16_t>& output_val = ctx.channel_to_output[channel];

                //ctx.found_output[channel] = output_val;

                std::vector<uint16_t> found;

                int err_pos = check_output(sim, output_val, found);

                if(err_pos != -1)
                    rstat.success = false;

                for(int i=0; i < (int)found.size() && i < (int)output_val.size(); i++)
                {
                    if(found[i] != output_val[i])
                        ctx.error_locs.push_back(i);
                }

                //ctx.error_loc = err_pos;
                ctx.found_output[channel] = found;
            }

            //rstat.success = check_output(outc, out) == -1;
        }

        return rstat;
    }
}
