/*
 MIDIdualCV converts upper/lower midi note to dual CV
 
 Copyright (C) 2019 Pablo Delaloza.
 
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
#include "moDllzComp.hpp"
struct Xpand  :  Module {
	enum ParamIds {
		NUM_PARAMS
	};
	enum OutputIds {
		XP_OUTPUT,
		XG_OUTPUT,
		XV_OUTPUT,
		XD_OUTPUT,
		XT_OUTPUT,
		XR_OUTPUT,
		NUM_OUTPUTS
	};
	enum InputIds {
		NUM_INPUTS
	};
	enum LightIds {
		ENUMS(CH_LIGHT, 16),
		NUM_LIGHTS
	};
	
	int numChPre = 0;
	int ProcessFrame = 0;
	int PROCESS_RATE = static_cast<int>(APP->engine->getSampleRate() * 0.0005); //.5ms
	sharedXpander *sharedXp = new sharedXpander();
	int xpanderId = 0;
	Xpand() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configOutput(XP_OUTPUT, "XP Out");
		sharedXpander::xpanders++;
		sharedXpander::xpandnum[0]++;//default to A
	}
	~Xpand() {
		sharedXpander::xpandnum[xpanderId]--;
		sharedXpander::xpanders--;
		delete sharedXp;
	}
	json_t *dataToJson() override {
		json_t *rootJ = json_object();
		json_object_set_new(rootJ, "xpanderId", json_integer(xpanderId));
		return rootJ;
	}
	void dataFromJson(json_t *rootJ) override {
		json_t *xpanderIdJ = json_object_get(rootJ, "xpanderId");
		if (xpanderIdJ) {
			sharedXpander::xpandnum[0]--;
			xpanderId = json_integer_value(xpanderIdJ);
			sharedXpander::xpandnum[xpanderId]++;
		}
	}
	void onSampleRateChange() override {
		PROCESS_RATE = static_cast<int>(APP->engine->getSampleRate() * 0.0005); //.5ms
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////   P  R  O  C  E  S  S   ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void process(const ProcessArgs &args) override{
		if (ProcessFrame++ < PROCESS_RATE) return;/////// R E T U R N ////////////
		ProcessFrame = 0;
		int nVoCh = sharedXpander::xpandch[xpanderId];
		outputs[XP_OUTPUT].setChannels(nVoCh);
		outputs[XG_OUTPUT].setChannels(nVoCh);
		outputs[XV_OUTPUT].setChannels(nVoCh);
		outputs[XT_OUTPUT].setChannels(nVoCh);
		outputs[XR_OUTPUT].setChannels(nVoCh);
		outputs[XD_OUTPUT].setChannels(nVoCh);
		for (int i = 0 ; i < nVoCh ; i++){
			outputs[XP_OUTPUT].setVoltage(sharedXpander::xpPitch[xpanderId][i],i);
			outputs[XG_OUTPUT].setVoltage(sharedXpander::xpGate[xpanderId][i],i);
			outputs[XV_OUTPUT].setVoltage(sharedXpander::xpVel[xpanderId][i],i);
			outputs[XR_OUTPUT].setVoltage(sharedXpander::xpRvel[xpanderId][i],i);
			outputs[XT_OUTPUT].setVoltage(sharedXpander::xpNafT[xpanderId][i],i);
			outputs[XD_OUTPUT].setVoltage(sharedXpander::xpDetP[xpanderId][i],i);
			lights[CH_LIGHT+ i].setBrightness(sharedXpander::xpLed[xpanderId][i]);
		}
		if (numChPre != nVoCh){
			for (int i = 0 ; i < 16 ; i++){
				lights[CH_LIGHT+ i].setBrightness(0.f);
			}
			numChPre = nVoCh;
		}
	}
};
struct XpanderLCD : OpaqueWidget {
	Xpand *module = nullptr;
	std::shared_ptr<Font> font;
	std::string strId[4]= {"A","B","C","D"};
	void drawLayer(const DrawArgs &args, int layer) override {
		if (layer != 1) return;/////// R E T U R N ////////////
		font = APP->window->loadFont(mFONT_FILE);
		if (!(font && font->handle >= 0)) return;/////// R E T U R N ////////////
		nvgFontFaceId(args.vg, font->handle);
		nvgFontSize(args.vg, 14.f);
		nvgTextAlign(args.vg, NVG_ALIGN_CENTER);
		nvgFillColor(args.vg, nvgRGBA(0xff, 0xff, 0xff, 0x66));
		for (int i = 0; i < 4 ; i++){
			nvgTextBox(args.vg,1.f + 12.f * static_cast<float>(i),15.f,12.f,(strId[i]).c_str(),NULL);
		}
		float xsel= 12.f * static_cast<float>(module->xpanderId);
		if (sharedXpander::xpandch[module->xpanderId]>0){
		nvgFillColor(args.vg, nvgRGBA(0x00, 0xff, 0x00, 0x16));
		for (int i = 0 ; i<6 ;i++){
			float fi = static_cast<float>(i);
			float xfi = xsel + 1.f - fi;
			float yfi = 2.f - fi;
			float xbi = 12.f + fi * 2.f;
			float ybi = 13.f + fi * 2.f;
			nvgBeginPath(args.vg);
			nvgRoundedRect(args.vg, xfi, yfi, xbi, ybi ,3.f + fi) ;
			nvgFill(args.vg);
		}
			nvgFillColor(args.vg, nvgRGB(0x00, 0xff, 0x00));
		}else{
			nvgFillColor(args.vg, nvgRGB(0xee, 0xee, 0xee));
		}
		nvgTextBox(args.vg,1.f +xsel ,15.f ,12.f ,(strId[module->xpanderId]).c_str(),NULL);
	}
	void onButton(const event::Button &e) override {
		if ((e.button != GLFW_MOUSE_BUTTON_LEFT) || (e.action != GLFW_PRESS)) return;/////// R E T
		sharedXpander::xpandnum[module->xpanderId]--;
		module->xpanderId = static_cast<int>(e.pos.x / 12.5f);
		sharedXpander::xpandnum[module->xpanderId]++;
	}
};
//////////////////////////////////////////////////////////////////////////////////////
///// MODULE WIDGET
/////////////////////////////////////////////////////////////////////////////////////
struct XpandWidget : ModuleWidget {
		XpandWidget(Xpand *module){
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/modules/Xpand.svg")));
		addChild(createWidget<ScrewBlack>(Vec(0.f, 0.f)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 15.f, 0.f)));
		addChild(createWidget<ScrewBlack>(Vec(0.f, 365.f)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 15.f, 365.f)));
		float yPos, xPos;
		yPos = 42.f;
		xPos = 14.f;
		for (int i = 0; i < 4; i++){
			for (int j = 0; j < 4; j++){
				
			addChild(createLight<VoiceChGreenLed>(Vec(xPos + j * 10.f, yPos + i * 6.f), module, Xpand::CH_LIGHT + i * 4.f + j));
			}
		}
		yPos = 104.5;
		for (int i = 0; i < 6; i++){
			addOutput(createOutput<moDllzPolyO>(Vec(18.5f, yPos),  module, Xpand::XP_OUTPUT + i));
			yPos+= 45.f;
		}
		if (module){
		
				XpanderLCD *xpLCD = createWidget<XpanderLCD>(Vec(5.f,17.f));
				xpLCD->box.size = {50.f, 50.f};
				xpLCD->module = module;
				addChild(xpLCD);
			for (int i = 0; i<4; i++){
				ValueTestLCD *MccLCD = createWidget<ValueTestLCD>(Vec(15.f * i,0.f));
				MccLCD->box.size = {15.f, 15.f};
				MccLCD->intVal = &sharedXpander::xpandnum[i];
				addChild(MccLCD);
			}
			
		}
			
	}
};
Model *modelXpand = createModel<Xpand, XpandWidget>("Xpand");
