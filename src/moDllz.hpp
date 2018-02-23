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

//struct MIDIMPE8Widget : ModuleWidget {
//    MIDIMPE8Widget();
//    void step() override;
//};
struct MIDIPolyWidget : ModuleWidget {
    MIDIPolyWidget();
    void step() override;
};

struct TwinGliderWidget : ModuleWidget {
    TwinGliderWidget();
    void step() override;
};


///////////////////////
// custom components
///////////////////////

///knob44
struct moDllzKnobM : SVGKnob {
    moDllzKnobM() {
        box.size = Vec(44, 44);
        minAngle = -0.83*M_PI;
        maxAngle = 0.83*M_PI;
        setSVG(SVG::load(assetPlugin(plugin, "res/moDllzKnobM.svg")));
    }
};
///knob32
struct moDllzKnob32 : SVGKnob {
    moDllzKnob32() {
        box.size = Vec(32, 32);
        minAngle = -0.83*M_PI;
        maxAngle = 0.83*M_PI;
        setSVG(SVG::load(assetPlugin(plugin, "res/moDllzKnob32.svg")));
    }
};
///TinyTrim
struct moDllzTTrim : SVGKnob {
  
    moDllzTTrim() {
        box.size = Vec(16, 16);
        minAngle = -0.83*M_PI;
        maxAngle = 0.83*M_PI;
        setSVG(SVG::load(assetPlugin(plugin, "res/moDllzTTrim.svg")));
    }
};


///SnapSelector32
struct moDllzSelector32 : SVGKnob {
    moDllzSelector32() {
        box.size = Vec(32, 32);
        minAngle = -0.85*M_PI;
        maxAngle = 0.85*M_PI;
        snap = true;
        setSVG(SVG::load(assetPlugin(plugin, "res/moDllzSnap32.svg")));
    }
};

struct moDllzSmSelector : SVGKnob{
    moDllzSmSelector() {
        box.size = Vec(36, 36);
        minAngle = -0.5*M_PI;
        maxAngle = 0.5*M_PI;
        snap = true;
        setSVG(SVG::load(assetPlugin(plugin, "res/moDllzSmSelector.svg")));
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
///Switch with Led
struct moDllzSwitchLed : SVGSwitch, ToggleSwitch {
    moDllzSwitchLed() {
        box.size = Vec(10, 18);
        addFrame(SVG::load(assetPlugin(plugin, "res/moDllzSwitchLed_0.svg")));
        addFrame(SVG::load(assetPlugin(plugin, "res/moDllzSwitchLed_1.svg")));
    }
};
///Horizontal switch with Led
struct moDllzSwitchLedH : SVGSwitch, ToggleSwitch {
    moDllzSwitchLedH() {
        box.size = Vec(18, 10);
        addFrame(SVG::load(assetPlugin(plugin, "res/moDllzSwitchLedH_0.svg")));
        addFrame(SVG::load(assetPlugin(plugin, "res/moDllzSwitchLedH_1.svg")));
    }
};
///Horizontal switch Triple with Led
struct moDllzSwitchLedHT : SVGSwitch, ToggleSwitch {
    moDllzSwitchLedHT() {
        box.size = Vec(24, 10);
        addFrame(SVG::load(assetPlugin(plugin, "res/moDllzSwitchLedHT_0.svg")));
        addFrame(SVG::load(assetPlugin(plugin, "res/moDllzSwitchLedHT_1.svg")));
        addFrame(SVG::load(assetPlugin(plugin, "res/moDllzSwitchLedHT_2.svg")));
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

///switch TriState Horizontal
struct moDllzSwitchTH : SVGSwitch, ToggleSwitch {
    moDllzSwitchTH() {
        box.size = Vec(30, 10);
        addFrame(SVG::load(assetPlugin(plugin, "res/moDllzSwitchTH_0.svg")));
        addFrame(SVG::load(assetPlugin(plugin, "res/moDllzSwitchTH_1.svg")));
        addFrame(SVG::load(assetPlugin(plugin, "res/moDllzSwitchTH_2.svg")));
    }
};

///Momentary Button
struct moDllzMoButton : SVGSwitch, MomentarySwitch {
    moDllzMoButton() {
        box.size = Vec(48, 27);
        addFrame(SVG::load(assetPlugin(plugin, "res/moDllzMoButton_0.svg")));
        addFrame(SVG::load(assetPlugin(plugin, "res/moDllzMoButton_1.svg")));
    }
};

///Momentary Label Clear Button
struct moDllzClearButton : SVGSwitch, MomentarySwitch {
    moDllzClearButton() {
        box.size = Vec(38, 13);
        addFrame(SVG::load(assetPlugin(plugin, "res/moDllzClearButton.svg")));
    }
};

///Momentary Round Button
struct moDllzRoundButton : SVGSwitch, MomentarySwitch {
    moDllzRoundButton() {
        box.size = Vec(14, 14);
        addFrame(SVG::load(assetPlugin(plugin, "res/moDllzRoundButton.svg")));
    }
};
///Led Switch
struct moDllzLedSwitch : SVGSwitch, ToggleSwitch {
    moDllzLedSwitch() {
        box.size = Vec(5, 5);
        addFrame(SVG::load(assetPlugin(plugin, "res/moDllzLedSwitch_0.svg")));
        addFrame(SVG::load(assetPlugin(plugin, "res/moDllzLedSwitch_1.svg")));
    }
};
///Led Switch
struct moDllzLedButton : SVGSwitch, ToggleSwitch {
    moDllzLedButton() {
        box.size = Vec(24, 14);
        addFrame(SVG::load(assetPlugin(plugin, "res/moDllzLedButton_0.svg")));
        addFrame(SVG::load(assetPlugin(plugin, "res/moDllzLedButton_1.svg")));
    }
};

///Momentary PulseUp
struct moDllzPulseUp : SVGSwitch, MomentarySwitch {
    moDllzPulseUp() {
        box.size = Vec(12, 12);
        addFrame(SVG::load(assetPlugin(plugin, "res/moDllzPulse2Up.svg")));
    }
};
///Momentary PulseDown
struct moDllzPulseDwn : SVGSwitch, MomentarySwitch {
    moDllzPulseDwn() {
        box.size = Vec(12, 12);
        addFrame(SVG::load(assetPlugin(plugin, "res/moDllzPulse2Dwn.svg")));
    }
};

///MuteGate
struct moDllzMuteG : SVGSwitch, MomentarySwitch {
    moDllzMuteG() {
        box.size = Vec(67, 12);
        addFrame(SVG::load(assetPlugin(plugin, "res/moDllzMuteG.svg")));
    }
};
///MuteGateP
struct moDllzMuteGP : SVGSwitch, MomentarySwitch {
    moDllzMuteGP() {
        box.size = Vec(26, 11);
        addFrame(SVG::load(assetPlugin(plugin, "res/moDllzMuteGP.svg")));
    }
};



///Jacks
struct moDllzPort : SVGPort {
        moDllzPort() {
            background->svg = SVG::load(assetPlugin(plugin, "res/moDllzPort.svg"));
            background->wrap();
            box.size = background->box.size;
        }
};
struct moDllzPortDark : SVGPort {
    moDllzPortDark() {
        background->svg = SVG::load(assetPlugin(plugin, "res/moDllzPortDark.svg"));
        background->wrap();
        box.size = background->box.size;
    }
};
//struct moDllzPortClear : SVGPort {
//    moDllzPortClear() {
//        background->svg = SVG::load(assetPlugin(plugin, "res/moDllzPortClear.svg"));
//        background->wrap();
//        box.size = background->box.size;
//    }
//};

//
//struct CenteredLabel : Widget {
//    std::shared_ptr<Font> font;
//    std::string text;
//    int fontSize;
//
//    CenteredLabel(int _fontSize = 12) {
//        //font = Font::load(FONT_FILE);
//        fontSize = _fontSize;
//        //box.size.y = BND_WIDGET_HEIGHT;
//        //
//    }
//    void draw(NVGcontext *vg) override {
//        //nvgFillColor(vg, nvgRGBA(0x33, 0x33, 0x33, 0x88));
//        //nvgBeginPath(vg);
//        //nvgRoundedRect(vg, 0.f, 0.f, box.size.x, box.size.y, 2.5f);
//        //nvgFill(vg);
//        nvgTextAlign(vg, NVG_ALIGN_CENTER);
//        nvgFillColor(vg, nvgRGB(0xdd, 0xdd, 0xdd));
//        nvgFontFaceId(vg, font->handle);
//        nvgTextLetterSpacing(vg, 0.0);
//        nvgFontSize(vg, fontSize);
//        //nvgText(vg, box.pos.x, box.pos.y, text.c_str(), NULL);
//        nvgText(vg, box.size.x * .5, 18.0, text.c_str(), NULL);
//        //Vec textPos = Vec(47 - 12 * text.length(), 18.0);
//        //nvgText(vg, textPos.x, textPos.y, text.c_str(), NULL);
//    }
//};

//struct LabeledKnob : moDllzKnobM {
//    LabeledKnob() {
//        //  setSVG(SVG::load(assetPlugin(plugin, "res/SmallWhiteKnob.svg")));
//    }
//    CenteredLabel* linkedLabel = nullptr;
//
//    void connectLabel(CenteredLabel* label) {
//        linkedLabel = label;
//        if (linkedLabel) {
//            linkedLabel->text = formatCurrentValue();
//        }
//    }
//
//    void onChange(EventChange &e) override {
//        moDllzKnobM::onChange(e);
//        if (linkedLabel) {
//            linkedLabel->text = formatCurrentValue();
//        }
//    }
//
//    virtual std::string formatCurrentValue() {
//        return std::to_string(static_cast<unsigned int>(value));
//    }
//};
//struct BPMKnob : LabeledKnob {
//    BPMKnob(){}
//    std::string formatCurrentValue() {
//        return std::to_string(static_cast<unsigned int>(powf(2.0, value)*60.0)) + " BPM";
//    }
//};

