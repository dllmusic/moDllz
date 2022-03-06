/*
 moDllz.hpp
 Copyright (C) 2019 Pablo Delaloza.
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <https:www.gnu.org/licenses/>.
 */
#pragma once
#include <rack.hpp>
#include <iomanip> // setprecision
#include <sstream> // stringstream
#include <list> // std::list
#include <algorithm> // std::find
#include <vector> // std::vector
#include "midiDllz.hpp"

//#define FONT_FILE asset::plugin(pluginInstance, "res/fonts/bold_led_board-7.ttf")
#define mFONT_FILE asset::plugin(pluginInstance, "res/fonts/Gidolinya-Regular.ttf")
//#define mFONT_FILE asset::system("res/fonts/ShareTechMono-Regular.ttf")
using namespace rack;

extern Plugin *pluginInstance;

// Forward-declare each Model, defined in each module source file
extern Model *modelMIDIpolyMPE;
// extern Model *modelMIDIdualCV;
extern Model *modelPolyGlider;
extern Model *modelKn8b;
// extern Model *modelXBender;


