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
struct moDllzKnobM : SVGKnob {
    moDllzKnobM() {
        box.size = Vec(44, 44);
        minAngle = -0.83*M_PI;
        maxAngle = 0.83*M_PI;
        setSVG(SVG::load(assetPlugin(plugin, "res/moDllzKnobM.svg")));
    }
};
///switch
struct moDllzSwitch : SVGSwitch, ToggleSwitch {
    moDllzSwitch() {
        box.size = Vec(10, 20);
        addFrame(SVG::load(assetPlugin(plugin, "res/moDllzSwitch_0.svg")));
        addFrame(SVG::load(assetPlugin(plugin, "res/moDllzSwitch_1.svg")));
    }
};
///Horizontal switch
struct moDllzSwitchH : SVGSwitch, ToggleSwitch {
    moDllzSwitchH() {
        box.size = Vec(20, 10);
        addFrame(SVG::load(assetPlugin(plugin, "res/moDllzSwitchH_0.svg")));
        addFrame(SVG::load(assetPlugin(plugin, "res/moDllzSwitchH_1.svg")));
    }
};
///switch TriState
struct moDllzSwitchT : SVGSwitch, ToggleSwitch {
    moDllzSwitchT() {
        box.size = Vec(10, 30);
        addFrame(SVG::load(assetPlugin(plugin, "res/moDllzSwitchT_0.svg")));
        addFrame(SVG::load(assetPlugin(plugin, "res/moDllzSwitchT_1.svg")));
        addFrame(SVG::load(assetPlugin(plugin, "res/moDllzSwitchT_2.svg")));
    }
};
///Toggle Button
//struct moDllzToggle : SVGSwitch, ToggleSwitch {
 //   moDllzToggle() {
 //       box.size = Vec(36, 12);
 //       addFrame(SVG::load(assetPlugin(plugin, "res/moDllzToggle_0.svg")));
 //       addFrame(SVG::load(assetPlugin(plugin, "res/moDllzToggle_1.svg")));
 //   }
//};
///Jack
struct moDllzPort : SVGPort {
        moDllzPort() {
            background->svg = SVG::load(assetPlugin(plugin, "res/moDllzPort.svg"));
            background->wrap();
            box.size = background->box.size;
        }
};
