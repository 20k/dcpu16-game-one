#include "levels.hpp"
#include <dcpu16-asm/base_asm.hpp>
#include <iostream>

dcpu::sim::CPU sim_input(const std::vector<uint16_t>& input)
{
    std::string vals;

    for(uint16_t i : input)
    {
        vals = vals + "SND " + std::to_string(i) + ", 0\n";
    }

    dcpu::sim::CPU inc;

    {
        auto [rinfo_opt, err] = assemble_fwd(vals);

        assert(rinfo_opt.has_value());

        inc.load(rinfo_opt.value().mem, 0);
    }

    return inc;
}

dcpu::sim::CPU sim_output(int len)
{
    std::string out_str;

    uint16_t output_address = 0x8000;

    for(int i=0; i < len; i++)
    {
        uint16_t fin_address = output_address + i;

        out_str += "RCV [" + std::to_string(fin_address) + "], 1\n";
    }

    dcpu::sim::CPU outc;

    {
        auto [rinfo_out, errout] = assemble_fwd(out_str);

        assert(rinfo_out.has_value());

        outc.load(rinfo_out.value().mem, 0);
    }

    return outc;
}

int check_output(const dcpu::sim::CPU& in, const std::vector<uint16_t>& output)
{
    uint16_t output_address = 0x8000;

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
            ctx.cpus.emplace_back();
        }

        return ctx;
    }

    stats validate(const level_context& ctx)
    {
        stats rstat;

        if(ctx.level == 0)
        {
            std::vector<uint16_t> in;

            for(int i=0; i < 1024; i++)
            {
                in.push_back(i);
            }

            std::vector<uint16_t> out;

            for(auto& i : in)
            {
                out.push_back(i);
            }

            auto inc = sim_input(in);
            auto outc = sim_output(in.size());

            std::vector<dcpu::sim::CPU> user;

            for(const dcpu::ide::editor& edit : ctx.cpus)
            {
                dcpu::sim::CPU& next = user.emplace_back();

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

            cpus.push_back(&inc);
            cpus.push_back(&outc);

            for(auto& i : user)
            {
                cpus.push_back(&i);
            }

            dcpu::sim::fabric fab;

            int max_cycles = 1000000;

            for(int i=0; i < max_cycles; i++)
            {
                for(int kk=0; kk < (int)cpus.size(); kk++)
                {
                    cpus[kk]->step(&fab);
                }

                dcpu::sim::resolve_interprocessor_communication(cpus, fab);
            }

            rstat.success = true;

            rstat.success = check_output(outc, out) == -1;
        }

        return rstat;
    }
}
