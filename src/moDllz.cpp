/*
moDllz.cpp
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

#include "moDllz.hpp"
// The pluginInstance-wide instance of the Plugin class
Plugin *pluginInstance;

void init(rack::Plugin *p) {
	pluginInstance = p;
	p->addModel(modelMIDIpolyMPE);
	p->addModel(modelMIDIpolyMPE64);
	p->addModel(modelMIDIdualCV);
	p->addModel(modelMIDIpoly16);
	p->addModel(modelXBender);
	p->addModel(modelTwinGlider);
	p->addModel(modelMIDI8MPE);
	//p->addModel(modelPolyTune);
}
