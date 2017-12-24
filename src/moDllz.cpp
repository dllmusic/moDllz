#include "moDllz.hpp"


// The plugin-wide instance of the Plugin class
Plugin *plugin;

void init(rack::Plugin *p) {
    plugin = p;
    // This is the unique identifier for your plugin
	p->slug = "moDllz";
#ifdef VERSION
	p->version = TOSTRING(VERSION);
#endif
	p->website = "https://github.com/dllmusic/VCV_moDllz";
    // For each module, specify the ModuleWidget subclass, manufacturer slug (for saving in patches), manufacturer human-readable name, module slug, and module name
	p->addModel(createModel<MIDIdualCVWidget>  ("moDllz", "MIDIdualCV",   "MIDI-to-dualCV interface", MIDI_TAG, DUAL_TAG));
    p->addModel(createModel<TwinGliderWidget>  ("moDllz", "TwinGlider",   "TwinGlider", SLEW_LIMITER_TAG,  DUAL_TAG, ENVELOPE_FOLLOWER_TAG, UTILITY_TAG));
}
