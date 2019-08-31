#include "moDllz.hpp"
// The pluginInstance-wide instance of the Plugin class
Plugin *pluginInstance;

void init(rack::Plugin *p) {
	pluginInstance = p;
	p->addModel(modelMIDIpolyMPE);
	p->addModel(modelMIDIdualCV);
	p->addModel(modelMIDIpoly16);
	p->addModel(modelXBender);
	p->addModel(modelTwinGlider);
	p->addModel(modelMIDI8MPE);
}
