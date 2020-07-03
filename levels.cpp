#include "levels.hpp"
#include <dcpu16-asm/base_asm.hpp>
#include <iostream>

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

            std::string vals;

            for(auto& i : in)
            {
                vals = vals + "SND " + std::to_string(i) + ", 0\n";
            }

            std::string out_str;

            uint16_t output_address = 0x8000;

            for(int i=0; i < (int)in.size(); i++)
            {
                uint16_t fin_address = output_address + i;

                out_str += "RCV [" + std::to_string(fin_address) + "], 1\n";
            }

            dcpu::sim::CPU inc;
            dcpu::sim::CPU outc;

            {
                auto [rinfo_opt, err] = assemble_fwd(vals);

                assert(rinfo_opt.has_value());

                inc.load(rinfo_opt.value().mem, 0);
            }

            {
                auto [rinfo_out, errout] = assemble_fwd(out_str);

                assert(rinfo_out.has_value());

                outc.load(rinfo_out.value().mem, 0);
            }

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

            for(int i=0; i < (int)in.size(); i++)
            {
                uint16_t offset = output_address + i;

                uint16_t v = outc.fetch_location(dcpu::sim::location::memory{offset});

                //std::cout << "FOUND " << v << " AT " << offset << std::endl;

                if(v != out[i])
                {
                    rstat.success = false;
                }
            }
        }

        return rstat;
    }
}
