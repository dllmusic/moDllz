#include "moDllz.hpp"
// The pluginInstance-wide instance of the Plugin class
Plugin *pluginInstance;

void init(rack::Plugin *p) {
	pluginInstance = p;
	p->addModel(modelTwinGlider);
	p->addModel(modelXBender);
	p->addModel(modelMIDIdualCV);
	p->addModel(modelMIDI8MPE);
	p->addModel(modelMIDIpoly16);
}
