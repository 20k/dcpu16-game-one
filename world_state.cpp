#include "world_state.hpp"
#include "hardware_rocket.hpp"

void step_world(world_state& st)
{
    float step_ds = 16.f/1000.f;

    if(st.player.base)
    {
        hardware_rocket* rocket = st.player.base;

        if(rocket->on)
        {
            vec2f dir = {cos(st.player.angle), sin(st.player.angle)};

            if(rocket->propellant > 0)
            {
                double prev = rocket->propellant;

                rocket->propellant = std::max(rocket->propellant - rocket->propellant_ejected_ps * step_ds, 0.);

                double diff = prev - rocket->propellant;
                double frac = diff / rocket->propellant_ejected_ps * step_ds;

                float linear_accel_ms = 1 * frac;
                vec2f accel = dir * linear_accel_ms * step_ds;

                st.player.velocity += accel;
            }
        }
    }

    st.player.position += st.player.velocity * step_ds;
    st.player.angle += st.player.angular_velocity * step_ds;
}
