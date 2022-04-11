/*
 moDllzComp.hpp components for MIDIpolyMPE
 
 Copyright (C) 2022 Pablo Delaloza.
 
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

#ifndef moDllzComp_hpp
#define moDllzComp_hpp

#include "moDllz.hpp"
#endif /* moDllzComp_hpp */
using namespace rack;

///Value Test LCD
struct ValueTestLCD : TransparentWidget{
	ValueTestLCD() {}
	std::shared_ptr<Font> font;
	int *intVal = nullptr;
	float *floatVal = nullptr;
	float mdfontSize = 14.f;
	void drawLayer(const DrawArgs &args, int layer) override;
};

/////
//ui::Tooltip* tooltip = new ui::Tooltip;
//tooltip->text = text;
//return tooltip;
//}
//
//void onEnter(const EnterEvent& e) override {
//setTooltip(createTooltip());
//}

///////////////////////
// gen components
///////////////////////

///knob44
struct moDllzKnobM : RoundKnob {
	moDllzKnobM() {
		// box.size = Vec(44, 44);
		minAngle = -0.83*M_PI;
		maxAngle = 0.83*M_PI;
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/knobs/moDllzKnobM_fg.svg")));
		bg->setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/knobs/moDllzKnobM_bg.svg")));
		shadow->opacity = 0.f;
	}
};
//
struct moDllzKnob26 : RoundKnob {
	moDllzKnob26() {
		minAngle = -0.83*M_PI;
		maxAngle = 0.83*M_PI;
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/knobs/moDllzKnob26_fg.svg")));
		bg->setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/knobs/moDllzKnob26_bg.svg")));
		shadow->opacity = 0.f;
	}
};
//
struct moDllzKnob22 : RoundKnob {
	moDllzKnob22() {
		minAngle = -0.83*M_PI;
		maxAngle = 0.83*M_PI;
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/knobs/moDllzKnob22_fg.svg")));
		bg->setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/knobs/moDllzKnob22_bg.svg")));
		shadow->opacity = 0.f;
	}
};

struct moDllzKnob18 : RoundKnob {
	moDllzKnob18() {
		minAngle = -0.83*M_PI;
		maxAngle = 0.83*M_PI;
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/knobs/moDllzKnob18_fg.svg")));
		bg->setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/knobs/moDllzKnob18_bg.svg")));
		shadow->opacity = 0.f;
	}
};

struct moDllzKnob18CV : RoundKnob {//18 CV Trim
	moDllzKnob18CV() {
		minAngle = -0.83*M_PI;
		maxAngle = 0.83*M_PI;
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/knobs/moDllzKnob18CV_fg.svg")));
		bg->setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/knobs/moDllzKnob18_bg.svg")));
		shadow->opacity = 0.f;
	}
};
//
/////TinyTrim
struct moDllzTTrim : SvgKnob {
	moDllzTTrim() {
		minAngle = -0.83*M_PI;
		maxAngle = 0.83*M_PI;
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/knobs/moDllzTTrim.svg")));
		shadow->opacity = 0.f;
	}
};
struct TTrimSnap : moDllzTTrim{
	TTrimSnap(){
		snap = true;
	}
};

/////switch
struct moDllzSwitch : SvgSwitch {
	moDllzSwitch() {
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/switches/moDllzSwitch_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/switches/moDllzSwitch_1.svg")));
		shadow->opacity = 0.f;
	}
};

/////Horizontal switch
struct moDllzSwitchH : SvgSwitch {
	moDllzSwitchH() {
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/switches/moDllzSwitchH_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/switches/moDllzSwitchH_1.svg")));
		shadow->opacity = 0.f;
	}
};
/////Switch with Led
struct moDllzSwitchLed : SvgSwitch {
	moDllzSwitchLed() {
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/switches/moDllzSwitchLed_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/switches/moDllzSwitchLed_1.svg")));
		shadow->opacity = 0.f;
	}
};

///Horizontal switch with Led
struct moDllzSwitchLedH : SvgSwitch {
	moDllzSwitchLedH() {
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/switches/moDllzSwitchLedH_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/switches/moDllzSwitchLedH_1.svg")));
		shadow->opacity = 0.f;
	}
};
/////Horizontal switch - Triple
struct moDllzSwitchTH : SvgSwitch {
	moDllzSwitchTH() {
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/switches/moDllzSwitchTH_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/switches/moDllzSwitchTH_1.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/switches/moDllzSwitchTH_2.svg")));
		shadow->opacity = 0.f;
	}
};
//
/// Transp Light over led switches
struct TranspOffRedLight : ModuleLightWidget {
	TranspOffRedLight() {
		box.size = Vec(10.f,10.f);
		addBaseColor(nvgRGBA(0xff, 0x00, 0x00, 0x88));//borderColor = nvgRGBA(0, 0, 0, 0x60);
	}
};

struct VoiceChRedLed : ModuleLightWidget {
	VoiceChRedLed() {
		box.size = Vec(2.f,2.f);
		addBaseColor(nvgRGB(0xff, 0x00, 0x00));//borderColor = nvgRGBA(0, 0, 0, 0x60);
	}
};
struct VoiceChBlueLed : ModuleLightWidget {
	VoiceChBlueLed() {
		box.size = Vec(2.f,2.f);
		addBaseColor(nvgRGB(0x00, 0x88, 0xff));//borderColor = nvgRGBA(0, 0, 0, 0x60);
	}
};

struct VoiceChGreenLed : ModuleLightWidget {
	VoiceChGreenLed() {
		box.size = Vec(2.f,2.f);
		addBaseColor(nvgRGB(0x00, 0xff, 0x00));//borderColor = nvgRGBA(0, 0, 0, 0x60);
	}
};

struct RedLed3 : ModuleLightWidget {
	RedLed3() {
		box.size = Vec(3.f,3.f);
		addBaseColor(nvgRGB(0xff, 0x00, 0x00));
		this->borderColor = nvgRGBA(0, 0, 0, 0x60);
		this->bgColor = nvgRGB(0x33, 0x33, 0x33);
	}
};

struct RedLed2 : ModuleLightWidget {
	RedLed2() {
		box.size = Vec(2.f,2.f);
		addBaseColor(nvgRGB(0xff, 0x00, 0x00));
		this->borderColor = nvgRGBA(0, 0, 0, 0x60);
		this->bgColor = nvgRGB(0x33, 0x33, 0x33);
	}
};

/////Round Push
struct roundPush15 : SvgSwitch {
	roundPush15() {
		momentary = true;
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/buttons/roundPush.svg")));
		shadow->opacity = 0.f;
	}
};

///switch TriState
struct moDllzSwitchT : SvgSwitch {
	moDllzSwitchT() {
		//   box.size = Vec(10, 30);
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/switches/moDllzSwitchT_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/switches/moDllzSwitchT_1.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/switches/moDllzSwitchT_2.svg")));
		shadow->opacity = 0.f;
	}
};
//

struct minusButtonB : SvgSwitch {
	minusButtonB() {
		momentary = true;
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/buttons/SqrMinus_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/buttons/SqrMinus_1.svg")));
		shadow->opacity = 0.f;
	}
	void onEnter(const EnterEvent& e) override {
		//// no tooltip
	};
	void onLeave(const LeaveEvent& e) override {
		//// no tooltip destroy
	}
};
struct plusButtonB : SvgSwitch {
	plusButtonB() {
		momentary = true;
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/buttons/SqrPlus_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/buttons/SqrPlus_1.svg")));
		shadow->opacity = 0.f;
	}
	void onEnter(const EnterEvent& e) override {
		//// no tooltip
	};
	void onLeave(const LeaveEvent& e) override {
		//// no tooltip destroy
	}
};
/////Jacks
struct moDllzPortI : SvgPort {
	moDllzPortI() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ports/moDllzPortI.svg")));
		shadow->opacity = 0.f;
	}
};

struct moDllzPortO : SvgPort {
	moDllzPortO() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ports/moDllzPortO.svg")));
		shadow->opacity = 0.f;
	}
};

struct moDllzPolyI : SvgPort {
	moDllzPolyI() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ports/moDllzPolyI.svg")));
		shadow->opacity = 0.f;
	}
};

struct moDllzPolyO : SvgPort {
	moDllzPolyO() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ports/moDllzPolyO.svg")));
		shadow->opacity = 0.f;
	}
};

struct SvgBuffered : widget::Widget {
	widget::FramebufferWidget* fb;
	widget::SvgWidget* sw;

	SvgBuffered();
	void setSvg(std::shared_ptr<window::Svg> svg);
};
