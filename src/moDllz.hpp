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
