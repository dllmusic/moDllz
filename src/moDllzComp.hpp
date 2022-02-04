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

/////////////////////////////////////////////////////////////////////////////////////////
struct paramLCD : OpaqueWidget {
	paramLCD(){
		font = APP->window->loadFont(mFONT_FILE);
	}
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
