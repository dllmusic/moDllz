#include "moDllz.hpp"


// The plugin-wide instance of the Plugin class
Plugin *plugin;

void init(rack::Plugin *p) {
    plugin = p;
    // This is the unique identifier for your plugin
	p->slug = TOSTRING(SLUG);
//#ifdef VERSION
	p->version = TOSTRING(VERSION);
//#endif

    p->addModel(modelMIDIPoly);
    p->addModel(modelTwinGlider);
    p->addModel(modelMIDIdualCV);
    p->addModel(modelXBender);
}
