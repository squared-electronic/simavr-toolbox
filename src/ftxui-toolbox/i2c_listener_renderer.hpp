#pragma once

#include <ftxui-toolbox/ftxui_simulated_avr.hpp>
#include <ftxui/component/component_base.hpp>
#include <simavr-toolbox/sim_i2c_listener.hpp>

ftxui::Component I2CListenerRenderer(avr_t* avr);
