#include "rack.hpp"


using namespace rack;


extern Plugin *plugin;

////////////////////
// module widgets
////////////////////

struct MIDIdualCVWidget : ModuleWidget {
    MIDIdualCVWidget();
    void step() override;
};

struct TwinGliderWidget : ModuleWidget {
    TwinGliderWidget();
    void step() override;
};


///////////////////////
// custom components
///////////////////////

///knob
struct moDLLzKnobM : SVGKnob {
    moDLLzKnobM() {
        box.size = Vec(40, 40);
        minAngle = -0.85*M_PI;
        maxAngle = 0.85*M_PI;
        setSVG(SVG::load(assetPlugin(plugin, "res/moDLLzKnobM.svg")));
    }
};
///switch
struct moDLLzSwitch : SVGSwitch, ToggleSwitch {
    moDLLzSwitch() {
        box.size = Vec(15, 24);
        addFrame(SVG::load(assetPlugin(plugin, "res/moDLLzSwitch_0.svg")));
        addFrame(SVG::load(assetPlugin(plugin, "res/moDLLzSwitch_1.svg")));
    }
};
///Jack
struct moDLLzPort : SVGPort {
        moDLLzPort() {
            background->svg = SVG::load(assetPlugin(plugin, "res/moDLLzPort.svg"));
            background->wrap();
            box.size = background->box.size;
        }
};
