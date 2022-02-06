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
/////////////////////////////////////////////////////////////////////////////////////////
struct paramLCD : OpaqueWidget {
	paramLCD(){}
	int intptr = 0;
	bool boolptr = true;
	int buttonId = 0;
	int *cursorIx = &intptr;
	int *param_val = &intptr;
	int *onFocus = &intptr;
	int *learnId = &intptr;
	bool *updateKnob = &boolptr;
	bool *canedit = &boolptr;
	bool canlearn = false;

	unsigned char tcol = 0xdd; //base color x x x
	unsigned char redSel = 0xcc; // sel red.
	int flashFocus = 0;

	float mdfontSize = 13.f;
	std::shared_ptr<Font> font;
	std::string sDisplay = "";

	void drawLayer(const DrawArgs &args, int layer) override;
	void onButton(const event::Button &e) override;
};

//// MIDIcc ParamLCD Button
struct MIDIccLCD : paramLCD{
	MIDIccLCD() {
		for (int i = 0 ; i < 128; i++){
			sNames[i].assign("cc" + std::to_string(i));
		}
		sNames[1].assign("Mod");
		sNames[2].assign("BrC");
		sNames[7].assign("Vol");
		sNames[10].assign("Pan");
		sNames[11].assign("Expr");
		sNames[64].assign("Sust");
		sNames[128].assign("chAT");
		sNames[129].assign("nteAT");
		sNames[131].assign("cc74+");
		sNames[132].assign("chAT+");
	}
	std::string sNames[133];

	void drawLayer(const DrawArgs &args, int layer) override;
};
////MIDIccs ParamLCD
struct PTR_paramLCD : MIDIccLCD{
	PTR_paramLCD(){}
	int **ptr_param_val = &param_val;
	void drawLayer(const DrawArgs &args, int layer) override;
};
/// mpeY / Detune  ParamLCD Button
struct MPEYdetuneLCD : MIDIccLCD{
	MPEYdetuneLCD() {}
	int **ptr_param_val = &param_val;
	int *detune_val = &intptr;
	void drawLayer(const DrawArgs &args, int layer) override;
};
//// MPE RelVel / PBend ParamLCD Button
struct RelVelLCD : paramLCD{
	RelVelLCD() {}
	int **ptr_param_val = &param_val;
	std::string pNames[2] = {"RelVel","chPB"};
	void drawLayer(const DrawArgs &args, int layer) override;
};
/// + - value  ParamLCD Button
struct PlusMinusValLCD : paramLCD{
	PlusMinusValLCD() {}
	void drawLayer(const DrawArgs &args, int layer) override;
};
///Value Test LCD
struct ValueTestLCD : TransparentWidget{
	ValueTestLCD() {
		font = APP->window->loadFont(mFONT_FILE);
	}
	std::shared_ptr<Font> font;
	float floatptr = 0;
	float *testval = &floatptr;
	float mdfontSize = 14.f;
	void drawLayer(const DrawArgs &args, int layer) override;
};
///Data KNOB
struct DataEntryKnob : RoundKnob {
	DataEntryKnob() {
		minAngle = -0.75*M_PI;
		maxAngle = 0.75*M_PI;
		snap = true;
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/knobs/dataKnob_fg.svg")));
		bg->setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/knobs/dataKnob_bg.svg")));
		shadow->opacity = 0.f;
	}
	void onEnter(const EnterEvent& e) override {
		//// no tooltip
	};
	void onLeave(const LeaveEvent& e) override {
		//// no tooltip destroy
	}
};
///Data Knob LED
struct DataEntyOnLed : TransparentWidget {
	DataEntyOnLed(){
		box.size.x = 107.f;
		box.size.y = 42.f;
	}
	int dummyInit = 0;
	int *cursorIx = &dummyInit;

	void drawLayer(const DrawArgs &args, int layer) override;
	void onButton(const event::Button &e) override;
};

struct transparentMidiButton : MidiButton {
	transparentMidiButton();
	float mdfontSize = 12.f;
	//	float xcenter = 0.f;
	//	char drawframe = 0;
	std::shared_ptr<Font> font;
	NVGcolor okColor = nvgRGB(0x88,0x88,0x88);
	NVGcolor unsetColor = nvgRGB(0xff,0,0);
	
	void drawLayer(const DrawArgs &args, int layer) override;
	void onButton(const event::Button &e) override;
};

///////////////////////
// custom components
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
/////knob32
//struct moDllzKnob32 : SvgKnob {
//	moDllzKnob32() {
//		// box.size = Vec(32, 32);
//		minAngle = -0.83*M_PI;
//		maxAngle = 0.83*M_PI;
//		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/moDllzKnob32.svg")));
//		shadow->opacity = 0.f;
//	}
//	//void randomize() override{
//	//}
//};
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
		//  box.size = Vec(32, 32);
		minAngle = -0.83*M_PI;
		maxAngle = 0.83*M_PI;
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/knobs/moDllzKnob22_fg.svg")));
		bg->setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/knobs/moDllzKnob22_bg.svg")));
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


//struct springDataKnobB : RoundKnob {
//	springDataKnobB() {
//		minAngle = -0.75*M_PI;
//		maxAngle = 0.75*M_PI;
//		snap = true;
//		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/knobs/dataKnob_fg.svg")));
//		bg->setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/knobs/dataKnob_bg.svg")));
//		shadow->opacity = 0.f;
//	}
//	void onDragEnd(const DragEndEvent &e) override {
//		if (e.button != GLFW_MOUSE_BUTTON_LEFT)
//			return;
//		this->resetAction();
//		SvgKnob::onDragEnd(e);
//	}
//	void onEnter(const EnterEvent& e) override {
//		//// no tooltip
//	};
//	void onLeave(const LeaveEvent& e) override {
//		//// no tooltip destroy
//	}
//};
//
/////SnapSelector32
//struct moDllzSelector32 : SvgKnob {
//	moDllzSelector32() {
//		// box.size = Vec(32, 32);
//		minAngle = -0.85*M_PI;
//		maxAngle = 0.85*M_PI;
//		snap = true;
//		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/moDllzSnap32.svg")));
//		shadow->opacity = 0.f;
//	}
//	//void randomize() override{
//	//}
//};
//
//struct moDllzSmSelector : SvgKnob{
//	moDllzSmSelector() {
//	// box.size = Vec(36, 36);
//		minAngle = -0.5*M_PI;
//		maxAngle = 0.5*M_PI;
//		snap = true;
//		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/moDllzSmSelector.svg")));
//		shadow->opacity = 0.f;
//	}
//	//void randomize() override{
//	//}
//};
//
/////switch
struct moDllzSwitch : SvgSwitch {
	moDllzSwitch() {
		// box.size = Vec(10, 20);
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/switches/moDllzSwitch_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/switches/moDllzSwitch_1.svg")));
		shadow->opacity = 0.f;
	}
	//void randomize() override{
	//}
};
/////Horizontal switch
struct moDllzSwitchH : SvgSwitch {
	moDllzSwitchH() {
		// box.size = Vec(20, 10);
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/switches/moDllzSwitchH_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/switches/moDllzSwitchH_1.svg")));
		shadow->opacity = 0.f;
	}
	//void randomize() override{
	//}
};
/////Switch with Led
struct moDllzSwitchLed : SvgSwitch {
	moDllzSwitchLed() {
		// box.size = Vec(10, 18);
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/switches/moDllzSwitchLed_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/switches/moDllzSwitchLed_1.svg")));
		shadow->opacity = 0.f;
	}
};
///Horizontal switch with Led
struct moDllzSwitchLedH : SvgSwitch {
	moDllzSwitchLedH() {
		// box.size = Vec(18, 10);
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/switches/moDllzSwitchLedH_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/switches/moDllzSwitchLedH_1.svg")));
		shadow->opacity = 0.f;
	}
	//void randomize() override{
	//}
};
/////Horizontal switch Triple with Led
//struct moDllzSwitchLedHT : SvgSwitch {
//	moDllzSwitchLedHT() {
//		// box.size = Vec(24, 10);
//		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/moDllzSwitchLedHT_0.svg")));
//		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/moDllzSwitchLedHT_1.svg")));
//		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/moDllzSwitchLedHT_2.svg")));
//		shadow->opacity = 0.f;
//	}
//	//void randomize() override{
//	//}
//};
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

//struct moDllzSwitchWithLed : moDllzSwitchLed {
//	app::ModuleLightWidget* light;
//	moDllzSwitchWithLed() {
//
//		light = new RedLight;;
//		this->addChild(light);
//		light.setBrightness(10.f);
//	}
//		app::ModuleLightWidget* getLight() {
//			return light;
//		}
//	void step() override {
//		moDllzSwitchLed::step();
//		redLed->box.pos = this->box.pos
//		.plus(this->box.size.div(2))
//		.plus(this->getParamQuantity()->getSmoothValue() * this->box.size.x);
//	}
//};
//template <typename TBase, typename TLightBase = RedLight>
//struct LightSlider : TBase {
//	app::ModuleLightWidget* light;
//
//	LightSlider() {
//		light = new TLightBase;
//		this->addChild(light);
//	}
//
//	app::ModuleLightWidget* getLight() {
//		return light;
//	}
//
//	void step() override {
//		TBase::step();
//		// Move center of light to center of handle
//		light->box.pos = this->handle->box.pos
//			.plus(this->handle->box.size.div(2))
//			.minus(light->box.size.div(2));
//	}
//};

///switch TriState
struct moDllzSwitchT : SvgSwitch {
	moDllzSwitchT() {
		//   box.size = Vec(10, 30);
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/switches/moDllzSwitchT_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/switches/moDllzSwitchT_1.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/switches/moDllzSwitchT_2.svg")));
		shadow->opacity = 0.f;
	}
	//void randomize() override{
	//}
};
//
/////switch TriState Horizontal
//struct moDllzSwitchTH : SvgSwitch {
//	moDllzSwitchTH() {
//		// box.size = Vec(30, 10);
//		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/moDllzSwitchTH_0.svg")));
//		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/moDllzSwitchTH_1.svg")));
//		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/moDllzSwitchTH_2.svg")));
//		shadow->opacity = 0.f;
//	}
//	//void randomize() override{
//	//}
//};
//
struct RouteOut : SvgSwitch {
	RouteOut() {
		// box.size = Vec(26, 12);
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/switches/chn1_8.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/switches/chn9_16.svg")));
		shadow->opacity = 0.f;
	}
};
/////Momentary Button
//struct moDllzMoButton : SvgSwitch {
//	moDllzMoButton() {
//		momentary = true;
//		// box.size = Vec(48, 27);
//		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/moDllzMoButton_0.svg")));
//		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/moDllzMoButton_1.svg")));
//		shadow->opacity = 0.f;
//	}
//	//void randomize() override{
//	//}
//};
//
/////Momentary Label Clear Button
//struct moDllzClearButton : SvgSwitch {
//	moDllzClearButton() {
//		momentary = true;
//		// box.size = Vec(38, 13);
//		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/moDllzClearButton.svg")));
//		shadow->opacity = 0.f;
//	}
//	//void randomize() override{
//	//}
//};
//
/////Momentary Round Button
//struct moDllzRoundButton : SvgSwitch {
//	moDllzRoundButton() {
//		momentary = true;
//		// box.size = Vec(14, 14);
//		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/moDllzRoundButton.svg")));
//		shadow->opacity = 0.f;
//	}
//	//void randomize() override{
//	//}
//};
//
/////Momentary PulseUp
//struct moDllzPulseUp : SvgSwitch {
//	moDllzPulseUp() {
//		momentary = true;
//		// box.size = Vec(12, 12);
//		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/moDllzPulse2Up.svg")));
//		shadow->opacity = 0.f;
//	}
//	//void randomize() override{
//	//}
//};
/////Momentary PulseDown
//struct moDllzPulseDwn : SvgSwitch {
//	moDllzPulseDwn() {
//		momentary = true;
//		// box.size = Vec(12, 12);
//		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/moDllzPulse2Dwn.svg")));
//		shadow->opacity = 0.f;
//	}
//	//void randomize() override{
//	//}
//};
//
/////MuteGate
//struct moDllzMuteG : SvgSwitch {
//	moDllzMuteG() {
//		momentary = true;
//		// box.size = Vec(67, 12);
//		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/moDllzMuteG.svg")));
//		shadow->opacity = 0.f;
//	}
//	//void randomize() override{
//	//}
//};
/////MuteGateP
//struct moDllzMuteGP : SvgSwitch {
//	moDllzMuteGP() {
//		momentary = true;
//		// box.size = Vec(26, 11);
//		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/moDllzMuteGP.svg")));
//		shadow->opacity = 0.f;
//	}
//	//void randomize() override{
//	//}
//};
/////MIDIPanic
//struct moDllzMidiPanic : SvgSwitch {
//	moDllzMidiPanic() {
//		momentary = true;
//		// box.size = Vec(38, 12);
//		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/midireset3812.svg")));
//		shadow->opacity = 0.f;
//	}
//	//void randomize() override{
//	//}
//};
//
///// Cursor  Buttons L R
//struct moDllzcursorL : SvgSwitch {
//	moDllzcursorL() {
//		momentary = true;
//		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/cursorL_0.svg")));
//		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/cursorL_1.svg")));
//		shadow->opacity = 0.f;
//	}
//	//void randomize() override{
//	//}
//};
//struct moDllzcursorR : SvgSwitch {
//	moDllzcursorR() {
//		momentary = true;
//		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/cursorR_0.svg")));
//		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/cursorR_1.svg")));
//		shadow->opacity = 0.f;
//	}
//	//void randomize() override{
//	//}
//};
///// UpDown + - Buttons
//struct minusButton : SvgSwitch {
//	minusButton() {
//		momentary = true;
//		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/minusButton_0.svg")));
//		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/minusButton_1.svg")));
//		shadow->opacity = 0.f;
//	}
//	//void randomize() override{
//	//}
//};
//struct plusButton : SvgSwitch {
//	plusButton() {
//		momentary = true;
//		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/plusButton_0.svg")));
//		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/plusButton_1.svg")));
//		shadow->opacity = 0.f;
//	}
//	//void randomize() override{
//	//}
//};
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
