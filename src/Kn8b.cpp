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
#include <queue>
struct Kn8b :  Module {
	enum ParamIds {
		ENUMS(KNOB_PARAM, 8),
		MAINKNOB_PARAM,
		PAGE_PARAM,
		VCA_PARAM,
		UNIPOLAR_PARAM,
		BIPOLAR_PARAM,
		SUM_PARAM,
		PROD_PARAM,
		TRIMMODE_PARAM,
		NUM_PARAMS
	};
	enum OutputIds {
		MAIN_OUTPUT,
		NUM_OUTPUTS
	};
	enum InputIds {
		MAIN_INPUT,
		CV_INPUT,
		CVTRIM_INPUT,
		NUM_INPUTS
	};
	enum LightIds {
		TRIM_LIGHT,
		NUM_LIGHTS
	};
	
	int chOffset = 0;
	int lcdmode[16] = {0};
	float inV[16] = {0.f};
	float knobVal[16] = {0.f};
	float calcKnob[16] = {0.f};
	float calcVal[16] = {0.f};
	float cvVal[16] = {0.f};
	float outV[16] = {0.f};
	float operation[16] = {0.f};
	float polarity[16] = {0.f};
	float vcaOut[16];
	float vcaMinus[16];
	float trimVal = 0.f;
	float incnnctd = 0.f;
	float cvcnnctd = 0.f;
	float cvtrimcnnctd = 0.f;
	float trimop = 0.f;
	bool incable = false;
	bool outcable = false;
	bool cvcable = false;
	bool cvtrimcable = false;
	int numInCh = 0;
	int numOutCh = 8;
	int prevNumCh = 8;
	const float processMs = 0.02f;//sec
	int PROCESS_RATE = 0;
	int ProcessFrame = 0;
	bool sampleRateWork = true;
	std::string btnunits[2] = {"v","x"};
	bool vca = false;
	
	///////////////////////////////////////////////////////////////////////////////////////
	json_t *dataToJson() override{
		json_t *rootJ = json_object();
		std::string jname;
		int floattoint;
		for (int i = 0; i< 16 ; i++){
			jname = "operation" + std::to_string(i);
			floattoint = static_cast<int>(operation[i]);
			json_object_set_new(rootJ, jname.c_str(), json_integer(floattoint));
			jname = "knobVal" + std::to_string(i);
			floattoint = static_cast<int>(knobVal[i] * 1e8);
			json_object_set_new(rootJ, jname.c_str(), json_integer(floattoint));
			jname = "polarity" + std::to_string(i);
			floattoint = static_cast<int>(polarity[i]);
			json_object_set_new(rootJ, jname.c_str(), json_integer(floattoint));
		}
		jname = "chOffset";
		json_object_set_new(rootJ, jname.c_str(), json_integer(chOffset));
		jname = "numOutCh";
		json_object_set_new(rootJ, jname.c_str(), json_integer(numOutCh));
		return rootJ;
	}
	///////////////////////////////////////////////////////////////////////////////////
	void dataFromJson(json_t *rootJ) override {
		std::string jname;
		json_t *jsaved = nullptr;
		for (int i = 0; i< 16 ; i++){
			jname = "operation" + std::to_string(i);
			jsaved = json_object_get(rootJ, jname.c_str());
			if (jsaved) operation[i] = static_cast<float>(json_integer_value(jsaved));
			jname = "knobVal" + std::to_string(i);
			jsaved = json_object_get(rootJ, jname.c_str());
			if (jsaved) knobVal[i] = static_cast<float>(json_integer_value(jsaved)) * 1e-8;
			jname = "polarity" + std::to_string(i);
			jsaved = json_object_get(rootJ, jname.c_str());
			if (jsaved) polarity[i] = static_cast<float>(json_integer_value(jsaved));
		}
		jname = "chOffset";
		jsaved = json_object_get(rootJ, jname.c_str());
		if (jsaved) chOffset = json_integer_value(jsaved);
		jname = "numOutCh";
		jsaved = json_object_get(rootJ, jname.c_str());
		if (jsaved) numOutCh = json_integer_value(jsaved);
		
		vcaMode((params[VCA_PARAM].getValue() > 0.f));
	}
	/////////////////////////////////////////////////////////////////////////////////////////
	
	Kn8b() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configSwitch(PAGE_PARAM, 0.f, 1.f, 0.f,"Channels Page");
		configSwitch(VCA_PARAM, 0.f, 1.f, 0.f,"VCA mode");
		configSwitch(UNIPOLAR_PARAM, 0.f, 1.f, 0.f,"Set Knobs to Unipolar");
		configSwitch(BIPOLAR_PARAM, 0.f, 1.f, 0.f,"Set Knobs to Bipolar");
		configSwitch(SUM_PARAM, 0.f, 1.f, 0.f,"Set to Input + Knobs");
		configSwitch(PROD_PARAM, 0.f, 1.f, 0.f,"Set to Input x Knobs");
		configSwitch(TRIMMODE_PARAM, 0.f, 1.f, 0.f,"Main Knob mode",{"Trim","Scale"});
		configInput(MAIN_INPUT, "to Knobs");
		configInput(CV_INPUT, "Knobs CV");
		configOutput(MAIN_OUTPUT, "Knobs");
		configBypass(MAIN_INPUT , MAIN_OUTPUT);
		configParam(MAINKNOB_PARAM,-1.f, 1.f, 0.f,"Main Knob","x(-5/5|0/10)v");
		for (int i = 0 ; i < 8; i++){
			configParam(KNOB_PARAM + i, -1.f, 1.f, 0.f,"Knob "+std::to_string(i + 1),"v",0.f,5.f);
		}
	}
	
	void initKnobs(){
		for (int i = 0; i<16 ; i++){
			chOffset = 0;
			lcdmode[i] = 0;
			inV[i] = 0.f;
			knobVal[i] = 0.f;
			calcKnob[i] = 0.f;
			calcVal[i] = 0.f;
			cvVal[i] = 0.f;
			outV[i] = 0.f;
			operation[i] = 0.f;
			polarity[i] = 0.f;
			vcaOut[i] = 0.f;
			vcaMinus[i] = 0.f;
			trimVal = 0.f;
		}
		knobsPage();
		paramQuantities[MAINKNOB_PARAM]->setSmoothValue(0.f);
	}
	
	void onReset() override {
		initKnobs();
	}
	
	void onRandomize(const RandomizeEvent& e) override {}

	void onSampleRateChange() override {
		setProcessRate(sampleRateWork);
	}
	
	void setProcessRate(bool srw){
		sampleRateWork = srw;
		if (srw) PROCESS_RATE = 0;
		else PROCESS_RATE = static_cast<int>(APP->engine->getSampleRate() * processMs);
	}
	
	void process(const ProcessArgs &args) override{
		if (vca) {
			numInCh = std::max(inputs[MAIN_INPUT].getChannels(), 1);
			numOutCh = numInCh;
			outputs[MAIN_OUTPUT].setChannels(numOutCh);
			float cvtrim = inputs[CVTRIM_INPUT].getVoltage() * 0.1f;
			float calcTrim = cvtrimcnnctd * (trimVal + 1.f) * trimop * cvtrim + cvtrimcnnctd * (trimVal + 1.f) + (1.f - trimop) * cvtrim + (1.f - cvtrimcnnctd) * (trimVal + 1.f);
			lights[TRIM_LIGHT].setBrightness(std::abs(calcTrim));
			for (int i = 0; i< numInCh; i++){
				lcdmode[i] = 1;
				cvVal[i] = inputs[CV_INPUT].getVoltage(i);
				inV[i] = inputs[MAIN_INPUT].getVoltage(i);
				calcVal[i] = cvVal[i] * .1f * calcTrim * (knobVal[i] + 1.f);
				outV[i] = inV[i] * calcVal[i];
				outputs[MAIN_OUTPUT].setVoltage(math::clamp(outV[i], -5.f,5.f), i);
				float vo = std::abs(outV[i]) * 12.f;
				if (vo > vcaOut[i]){
					vcaOut[i] = vo;
				}
			}
			return;
		}
		if (ProcessFrame++ < PROCESS_RATE) return;
		ProcessFrame = 0;
		if (outcable) {
			outputs[MAIN_OUTPUT].setChannels(numOutCh);
			if (cvcable || cvtrimcable) updateCalcValCv();
			if (incable) {
				numInCh = std::max(inputs[MAIN_INPUT].getChannels(), 1);
				int io = 0;
				for (int i = 0 ; i < std::min(numInCh,numOutCh) ; i++){
					float clmin = -5.f + (5.f * polarity[i]);
					float clmax = clmin + 10.f;
					inV[i] = inputs[MAIN_INPUT].getVoltage(i);
					outV[i] = inThruOp(i);
					lcdmode[i] = 4;
					outputs[MAIN_OUTPUT].setVoltage(math::clamp(outV[i],clmin,clmax), i);
					io++;
				}
				for (int i = io ; i < numOutCh; i++){
					float clmin = -5.f + (5.f * polarity[i]);
					float clmax = clmin + 10.f;
					outV[i] = calcVal[i];
					lcdmode[i] = 3;
					outputs[MAIN_OUTPUT].setVoltage(math::clamp(outV[i],clmin,clmax), i);
				}
			}else{
				for (int i = 0 ; i < numOutCh ; i++){
					float clmin = -5.f + (5.f * polarity[i]);
					float clmax = clmin + 10.f;
					outV[i] = calcVal[i];
					lcdmode[i] = 3;
					outputs[MAIN_OUTPUT].setVoltage(math::clamp(outV[i],clmin,clmax), i);
				}
			}
		}else{/// no outputs. Display only
			if (incable) {
				if (cvcable || cvtrimcable) updateCalcValCv();
				numInCh = std::max(inputs[MAIN_INPUT].getChannels(), 1);
				for (int i = 0 ; i < std::min(numInCh,numOutCh) ; i++){
					inV[i] = inputs[MAIN_INPUT].getVoltage(i);
					outV[i] = inThruOp(i);
					lcdmode[i] = 2;
				}
			}else{
				for (int i = 0 ; i < numOutCh ; i++){
					lcdmode[i] = 1;
				}
			}
		}
		if (prevNumCh != numOutCh) {
			lcdClearMode(numOutCh - 1);
			prevNumCh = numOutCh;
		}
	}
	
	void onPortChange(const PortChangeEvent& e) override{
		if (e.type == Port::INPUT) {
			switch (e.portId) {
				case (MAIN_INPUT) :{
					for (int i = 0; i < 8 ; i++){
						knobsInfo(i);
					}
					incable = e.connecting;
					if (incable){
						incnnctd = 1.f;
					}else{
						paramQuantities[VCA_PARAM]->setValue(0.f);
						incnnctd = 0.f;
						for (int i = 0; i< 16 ; i++){
							inV[i] = 0.f;
						}
					}
				}break;
				case (CV_INPUT) :{
					cvcable = e.connecting;
					if (cvcable){
						cvcnnctd =  1.f;
						//configMainKnob(1.f,0.f,"v+");
					}else{
						paramQuantities[VCA_PARAM]->setValue(0.f);
						cvcnnctd =  0.f;
						//configMainKnob(5.f,0.f,"v+");
					}
				}break;
				case (CVTRIM_INPUT) :{
					cvtrimcable = e.connecting;
					cvtrimcnnctd = (cvtrimcable)? 1.f : 0.f;
				}break;
			}
		}else{
			outcable = e.connecting;
		}
		Module::onPortChange(e);
	}
	
	void updateCalcValCv(){
		float cvtrim = inputs[CVTRIM_INPUT].getVoltage() * 0.1f;
		float calcTrim = cvtrimcnnctd * trimVal * cvtrim + (1.f - cvtrimcnnctd) * trimVal;
		int numCvIn = std::max(inputs[CV_INPUT].getChannels(), 1);
		for (int i = 0; i< numOutCh; i++){
			cvVal[i] = inputs[CV_INPUT].getVoltage(i % numCvIn);
			float calcvalknob = calcKnob[i] + cvVal[i] * .1f;
			calcVal[i] = trimop * calcvalknob * calcTrim + (1.f - trimop) * (calcvalknob + calcTrim * 5.f);
		}
		lights[TRIM_LIGHT].setBrightness(std::abs(calcTrim));
	}
	
	void updateKnobVal(int i){
		float opin = operation[i] * incnnctd;// + (1.f - incnnctd);// 0+ or 1x if connected 0 disc
		float op = (1.f - opin) * 5.f + opin; //5 + or 1 x
		calcKnob[i] = op * (knobVal[i] + polarity[i]);
		calcVal[i] = trimop * calcKnob[i] * trimVal + (1.f - trimop) * (calcKnob[i] + trimVal * 5.f);;
	}
	
	float inThruOp(int i){
		return  (1.f - operation[i]) * (inV[i] + calcVal[i]) + operation[i] * inV[i] * calcVal[i];
	}
	
	void knobsPage(){
		for (int i = 0; i < 8 ; i++){
			knobsInfo(i);
			paramQuantities[KNOB_PARAM + i]->name= "Ch:" + std::to_string(i + 1 + chOffset);
			paramQuantities[KNOB_PARAM + i]->setSmoothValue(knobVal[i + chOffset]);
		}
	}
	void knobsInfo(int i){
		int pid = KNOB_PARAM + i;
		float op =	operation[i+chOffset];
		float dm = (1.f - op) * 5.f + op;
		paramQuantities[pid]->displayMultiplier = dm;
		paramQuantities[pid]->displayOffset = polarity[i+chOffset] * dm;
		paramQuantities[pid]->unit= btnunits[static_cast<int>(op)];
		paramQuantities[pid]->defaultValue= -polarity[i+chOffset];;
	}
	
	void lcdClearMode(int ifrom){
		for (int i = ifrom; i< 16; i++){
			lcdmode[i] = 0;
		}//clean up Lcd status
	}
	
	
	void knobsUniBipolar(float ub , int ifrom, int ito){
		for (int i = ifrom; i< ito; i++){
			polarity[i] = ub;
			knobsInfo(i);
			updateKnobVal(i);
		}
	}
	void knobsSumProd(float op , int ifrom, int ito){
		for (int i = ifrom; i< ito; i++){
			operation[i] = op;
			knobsInfo(i);
			updateKnobVal(i);
		}
	}
	
	void vcaMode(bool onoff){
		vca = onoff;
		if (vca){
			//numInCh = std::max(inputs[MAIN_INPUT].getChannels(), 1);
			numOutCh = numInCh;
			for (int i = numOutCh; i < 16; i++){
				lcdmode[i] = 0;
			}
			for (int i = 0; i< std::min(numOutCh,8) ; i++){
				int pid = KNOB_PARAM + i;
				paramQuantities[pid]->displayMultiplier = 1.f;
				paramQuantities[pid]->displayOffset = 1.f;
				paramQuantities[pid]->unit= "x CV";
				paramQuantities[pid]->setSmoothValue(0.f);
			}
			configMainKnob(2);//vca
			paramQuantities[MAINKNOB_PARAM]->setSmoothValue(0.f);
		}else{
			for (int i = 0; i< 8; i++){
				knobsInfo(i);
			}
			configMainKnob(trimop);
		}
	}
	void configMainKnob(int mode){
		switch (mode){
			case 0 : {
				paramQuantities[MAINKNOB_PARAM]->displayMultiplier = 5.f;
				paramQuantities[MAINKNOB_PARAM]->displayOffset = 0.f;
				paramQuantities[MAINKNOB_PARAM]->unit= "v+";
			}break;
			case 1 : {
				paramQuantities[MAINKNOB_PARAM]->displayMultiplier = 1.f;
				paramQuantities[MAINKNOB_PARAM]->displayOffset = 0.f;
				paramQuantities[MAINKNOB_PARAM]->unit= "x";
			}break;
			case 2 : {//vca
				paramQuantities[MAINKNOB_PARAM]->displayMultiplier = 1.f;
				paramQuantities[MAINKNOB_PARAM]->displayOffset = 1.f;
				paramQuantities[MAINKNOB_PARAM]->unit= "vca";
			}break;
				
		}
	}
};

struct Knob26x : moDllzKnob26  {
	Knob26x() {
		maxAngle = M_PI * 2.f/3.f;
		minAngle = - maxAngle;
	}
	Kn8b *module = nullptr;
	int id = 0;
	void onChange(const ChangeEvent& e) override{
		if (!module) return;
		moDllzKnob26::onChange(e);
		engine::ParamQuantity* pq = getParamQuantity();
		if (pq){
			int ix = id + module->chOffset;
			module->knobVal[ix] = pq->getSmoothValue();
			if(!module->vca) module->updateKnobVal(ix);
		}
	}
};

struct Knob26G : Knob26x  {
	void onChange(const ChangeEvent& e) override{
		if (!module) return;
		moDllzKnob26::onChange(e);
		engine::ParamQuantity* pq = getParamQuantity();
		if (pq){
			module->trimVal = pq->getSmoothValue();
			if (module->vca || module->cvcable) return;
			for (int i = 0; i< module->numOutCh; i++){
				module->updateKnobVal(i);
			}
		}
	}
};

struct btn_page : SvgSwitch {
	btn_page() {
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/buttons/btn_page.svg")));
		shadow->opacity = 0.f;
	}
	Kn8b *module = nullptr;
	void onChange(const ChangeEvent& e) override {
		if (!module) return;
		engine::ParamQuantity* pq = getParamQuantity();
		if (pq) {
			module->chOffset = static_cast<int>(pq->getValue()) * 8;
			module->knobsPage();
		}
		SvgSwitch::onChange(e);
	}
};
struct btn_vca : SvgSwitch {
	btn_vca() {
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/buttons/btn_vca_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/buttons/btn_vca_1.svg")));
		shadow->opacity = 0.f;
	}
	std::shared_ptr<Font> font;
	Kn8b *module = nullptr;
	void drawLayer(const DrawArgs &args, int layer) override {
		if ((!module) || (!module->vca) || (layer != 1)) return;
		nvgFillColor(args.vg, nvgRGBA(0xff, 0x00, 0x00, 0x16));
		for (int i = 0 ; i<6 ;i++){
			float fi = static_cast<float>(i);
			float xfi = 1.7f - fi;
			float yfi = 2.2f - fi;
			float xbi = 12.7f + fi * 2.f;
			float ybi = 7.6f + fi * 2.f;
			nvgBeginPath(args.vg);
			nvgRoundedRect(args.vg, xfi, yfi, xbi, ybi ,2.f + fi * 0.5f) ;
			nvgFill(args.vg);
		}
		font = APP->window->loadFont(dFONT_FILE);
		if (!(font && font->handle >= 0)) return;
		nvgFontFaceId(args.vg, font->handle);
		nvgFontSize(args.vg, 10.f);
		nvgFillColor(args.vg, nvgRGBA(0xff, 0, 0, 0x66));
		nvgText(args.vg,1.75f,9.75f,"VCA",NULL);
	}
	void onChange(const ChangeEvent& e) override {
		if (!module) return;
		if ((module->incnnctd + module->cvcnnctd) < 2.f) {
			this->resetAction();
			return;
		}
		engine::ParamQuantity* pq = getParamQuantity();
		if (pq) module->vcaMode(pq->getValue()>0);
		SvgSwitch::onChange(e);
	}
};
struct btn_unipolar : SvgSwitch {
	btn_unipolar() {
		momentary = true;
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/buttons/btn_unipolar.svg")));
		shadow->opacity = 0.f;
	}
	Kn8b *module = nullptr;
	void onDragEnd(const DragEndEvent& e) override {
		if (!module) return;
		module->knobsUniBipolar(1.f, module->chOffset,  module->chOffset + 8 );
		SvgSwitch::onDragEnd(e);
	}
};
struct btn_bipolar : SvgSwitch {
	btn_bipolar() {
		momentary = true; //using DragEnd instead of change so it doesn't trigger on load
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/buttons/btn_bipolar.svg")));
		shadow->opacity = 0.f;
	}
	Kn8b *module = nullptr;
	void onDragEnd(const DragEndEvent& e) override {
		if (!module) return;
		module->knobsUniBipolar(0.f, module->chOffset,  module->chOffset + 8 );
		SvgSwitch::onDragEnd(e);
	}
};
struct btn_sum : SvgSwitch {
	btn_sum() {
		momentary = true;
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/buttons/btn_sum.svg")));
		shadow->opacity = 0.f;
	}
	Kn8b *module = nullptr;
	void onDragEnd(const DragEndEvent& e) override {
		if (!module) return;
		module->knobsSumProd(0.f, module->chOffset,  module->chOffset + 8 );
		SvgSwitch::onDragEnd(e);
	}
};
struct btn_prod : SvgSwitch {
	btn_prod() {
		momentary = true;
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/buttons/btn_prod.svg")));
		shadow->opacity = 0.f;
	}
	Kn8b *module = nullptr;
	void onDragEnd(const DragEndEvent& e) override {
		if (!module) return;
		module->knobsSumProd(1.f, module->chOffset,  module->chOffset + 8 );
		SvgSwitch::onDragEnd(e);
	}
};
//trim mode
struct Trim_mode : moDllzSwitch {
	Kn8b *module = nullptr;

	void onChange(const ChangeEvent& e) override {
		if (!module) return;
		engine::ParamQuantity* pq = getParamQuantity();
		if (pq) {
			module->trimop = pq->getValue();
			for (int i = 0; i< module->numOutCh; i++){
				module->updateKnobVal(i);
			}
			if(!module->vca) module->configMainKnob(module->trimop);
		}
		moDllzSwitch::onChange(e);
	}
};

////LCD
struct Kn8bLCD : TransparentWidget{
	Kn8bLCD() {}
	Kn8b *module = nullptr;
	std::shared_ptr<Font> font;
	int id = 0;
	std::string modetxt[4] = {"+","","x","x"};
	float mdfontSize = 14.f;
	float qvVal = 0.f;
	float minusVal = 0.f;
	NVGcolor colorBtn = nvgRGB(0xdd, 0xdd, 0xdd);
	NVGcolor colorVca = nvgRGB(0xff, 0x88, 0x00);
	NVGcolor colorVcaS = nvgRGB(0xee, 0xee, 0xee);
	NVGcolor colorOut = nvgRGB(0xee, 0xee, 0xee);
	NVGcolor colorIn = nvgRGB(0x44, 0xdd, 0xff);
	NVGcolor colorCv = nvgRGB(0xdd, 0xdd, 0x00);
	NVGcolor colorNoConn = nvgRGB(0x88, 0x88, 0x88);
	NVGcolor opColor[2] = {nvgRGB(0xff, 0x99, 0x66), nvgRGB(0xff, 0x44, 0x44)};
	
	void drawLayer(const DrawArgs &args, int layer) override {
		if ((!module) || (layer != 1)) return;
		font = APP->window->loadFont(mFONT_FILE);
		if (!(font && font->handle >= 0)) return;
		int ix = id + module->chOffset;
		std::string strout;
		strout= std::to_string(id + 1 + module->chOffset);
		nvgScissor(args.vg, 0.f, 0.f, 74.f, 33.f);
		nvgFontFaceId(args.vg, font->handle);
		nvgFontSize(args.vg, mdfontSize - 2.f);
		nvgTextAlign(args.vg, NVG_ALIGN_CENTER);
		if (module->lcdmode[ix] < 1){
			nvgFillColor(args.vg, colorNoConn);
			nvgTextBox(args.vg, 0.f, 10.f,12.f, strout.c_str(), NULL);
			return;
		}
		if (module->vca){
			nvgBeginPath(args.vg);
			nvgFillColor(args.vg, colorVca);
			nvgTextBox(args.vg, 0.f, 10.f, 12.f, strout.c_str(), NULL);
			float knbval = module->knobVal[ix] + module->trimVal;
			nvgTextAlign(args.vg, NVG_ALIGN_RIGHT);
			text2screen(args.vg, opColor[1], 11.f, fstring(knbval));
			text2screen(args.vg, colorVca, 22.f, fstring(module->calcVal[ix]));
			float xval = module->calcVal[ix] * 60.f;
			nvgRect(args.vg, 0.f, 24.f , xval, 6.f);
			nvgFill(args.vg);
			nvgBeginPath(args.vg);
			nvgFillColor(args.vg, colorVcaS);
			float ov = std::abs(module->outV[ix]) * 14.f;//5v max * 14 = 70 pix
			if(ov > qvVal) {
				qvVal = ov;
				minusVal = 0.01f;
			}
			nvgRect(args.vg, 0.f, 30.f, qvVal, 2.f);
			nvgFill(args.vg);
			if (qvVal > 0.f){
				qvVal -= minusVal;
				minusVal *= 2.f;
			}
			return;
		}
		//nvgGlobalAlpha(args.vg, 1.f);
		nvgFillColor(args.vg, colorBtn);
		nvgTextBox(args.vg, 0.f, 10.f,12.f, strout.c_str(), NULL);
		unsigned char dim = static_cast<unsigned char>(module->incnnctd) * 0x66 + 0x77;
		nvgFillColor(args.vg, nvgRGB(dim, dim, dim));
		nvgStrokeColor(args.vg, nvgRGB(dim, dim, dim));
		int operation = module->operation[ix];
		nvgBeginPath(args.vg);
		nvgStrokeWidth(args.vg, 1.f);
		if (operation > 0){/////OPERATION/// prod
			nvgMoveTo(args.vg, 3.25f, 14.25f);
			nvgLineTo(args.vg, 8.75f, 19.75f);
			nvgMoveTo(args.vg, 8.75f, 14.25f);
			nvgLineTo(args.vg, 3.25f, 19.75f);
		}else{// sum
			nvgMoveTo(args.vg, 6.f, 13.5f);
			nvgLineTo(args.vg, 6.f, 20.5f);
			nvgMoveTo(args.vg, 2.5f, 17.f);
			nvgLineTo(args.vg, 9.5f, 17.f);
		}
		nvgStroke(args.vg);
		nvgBeginPath(args.vg);////////////////POLARITY/////////////////////////////////////
		nvgStrokeColor(args.vg, colorBtn);
		nvgFillColor(args.vg, colorBtn);
		if(module->polarity[ix] > 0){///uni
			nvgRect(args.vg, 3.5f, 23.f, 5.f, 8.f);
			nvgFill(args.vg);
			nvgBeginPath(args.vg);
			nvgMoveTo(args.vg, 1.5f, 30.5f);
			nvgLineTo(args.vg, 10.5f, 30.5f);
			nvgStroke(args.vg);
		}else{///bi
			nvgRect(args.vg, 3.5f, 23.f, 5.f, 4.f);
			nvgFill(args.vg);
			nvgBeginPath(args.vg);
			nvgRect(args.vg, 4.f, 27.f, 4.f, 3.5f);
			nvgMoveTo(args.vg, 1.5f, 27.f);
			nvgLineTo(args.vg, 10.5f, 27.f);
			nvgStroke(args.vg);
		}
		nvgFontSize(args.vg, mdfontSize + 1.f);
		nvgTextAlign(args.vg, NVG_ALIGN_RIGHT);
		float ypos = 19.f;
		switch (module->lcdmode[ix]) {
			case 1: {////////////// NOT CONNECTED
				if (module->cvcnnctd > 0.f){
					text2screen(args.vg, colorCv, 12.f, fstring(module->cvVal[ix]) + "v");
					ypos = 26.f;
				}
				text2screen(args.vg, colorNoConn, ypos, fstring(module->calcVal[ix]) + "v");
			} break;
			case 2: {//input -> Lcd  /////IN V
				text2screen(args.vg, colorIn, 12.f, fstring(module->inV[ix]) + "v");
				strout = modetxt[operation * 2 + static_cast<int>(module->calcVal[ix]< 0.f)] + fstring(module->calcVal[ix]);
				text2screen(args.vg, opColor[operation] , 26.f, strout);
			} break;
			case 3: {//////////////OUT V
				ypos = 19.f;
				if (module->cvcnnctd > 0.f){
					text2screen(args.vg, colorCv, 12.f, fstring(module->cvVal[ix]) + "v");
					ypos = 26.f;
				}
				text2screen(args.vg, colorOut, ypos, fstring(module->outV[ix]) + "v");
			} break;
			case 4: {//thru knobs /////IN V
				nvgFontSize(args.vg, mdfontSize - 2.f);
				text2screen(args.vg, colorIn, 9.f, fstring(module->inV[ix]) + "v");
				strout = modetxt[operation * 2 + static_cast<int>(module->calcVal[ix]< 0.f)] + fstring(module->calcVal[ix]);
				text2screen(args.vg, opColor[operation] , 18.5f, strout);
				nvgFontSize(args.vg, mdfontSize);
				text2screen(args.vg, colorOut, 31.f, fstring(module->outV[ix]) + "v");
			} break;
		}
	}
	void text2screen(NVGcontext *ctx, NVGcolor tcol, float liney, std::string sto){
		nvgFillColor(ctx, tcol);
		nvgTextBox(ctx, 0.f, liney, box.size.x-2.f, sto.c_str(), NULL);
	}
	std::string fstring(float v){
		std::stringstream stream;
		stream << std::fixed << std::setprecision(3) << v;
		return stream.str();
	}
	void onButton(const event::Button &e) override {
		if ((e.button == GLFW_MOUSE_BUTTON_LEFT) && (e.action == GLFW_PRESS)){
			if (e.pos.x > 20.f) return;
			int line = static_cast<int>(e.pos.y / 11.f);
			int ix = id + module->chOffset;
			switch (line) {
				case 0 : {
					module->numOutCh = ix + 1;
				}
					break;
				case 1 : {
					float op = (module->operation[ix] > 0.f)? 0.f : 1.f;
					module->knobsSumProd(op, ix, ix + 1);
				}
					break;
				case 2 : {
					float ub = (module->polarity[ix] > 0.f)? 0.f : 1.f;
					module->knobsUniBipolar(ub, ix, ix + 1);
				}
					break;
			}
		}
	}
};


//////////////////////////////////////////////////////////////////////////////////////
///// MODULE WIDGET
///////////////////
struct Kn8bWidget : ModuleWidget {
	Kn8bWidget(Kn8b *module){
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/modules/Kn8b.svg")));
//		if (!module){
//			app::SvgScrew *prepanel = new app::SvgScrew;
//			prepanel->setSvg(Svg::load(asset::plugin(pluginInstance, "res/modules/Kn8b_pre.svg")));
//			addChild(prepanel);
//		}
		addChild(createWidget<ScrewBlack>(Vec(0, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 15, 0)));
		addChild(createWidget<ScrewBlack>(Vec(0, 365)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 15, 365)));
		float yPos = 18.5f;
		
		for (int i = 0 ; i < 8; i++){
			Kn8bLCD *MccLCD = createWidget<Kn8bLCD>(Vec(40.f,yPos));
			MccLCD->box.size = {74.f, 33.f};
			MccLCD->module = module;
			MccLCD->id = i;
			addChild(MccLCD);
			
			Knob26x * knb = new Knob26x;
			knb->box.pos = Vec(8.f, yPos + 3.5f);
			knb->module = module;
			knb->id = i;
			knb->app::ParamWidget::module = module;
			knb->app::ParamWidget::paramId = Kn8b::KNOB_PARAM + i;
			knb->initParamQuantity();
			addParam(knb);
			yPos += 35.f;
		}
		
		yPos = 301.5;
		addInput(createInput<moDllzPolyI>(Vec(9.5f, yPos),  module, Kn8b::CV_INPUT));
		addInput(createInput<moDllzPortI>(Vec(36.f, yPos),  module, Kn8b::CVTRIM_INPUT));
		
		{
			Knob26G* knb = new Knob26G;
			knb->box.pos = Vec(66.5f, yPos + 1.5f);
			knb->module = module;
			knb->app::ParamWidget::module = module;
			knb->app::ParamWidget::paramId = Kn8b::MAINKNOB_PARAM;
			knb->initParamQuantity();
			knb->addChild(createLight<RedLed2>(Vec(-4.f, 13.f), module, Kn8b::TRIM_LIGHT));
			addParam(knb);
		}
		
		{///Trim mode
			Trim_mode *sw = createWidget<Trim_mode>(Vec(105.f, yPos + 2.5f));
			sw->module = module;
			sw->app::ParamWidget::module = module;
			sw->app::ParamWidget::paramId = Kn8b::TRIMMODE_PARAM;
			sw->initParamQuantity();
			addParam(sw);
		}
		
		yPos = 334.f;
		float xPos = 34.f;
		{
			btn_unipolar *bt = createWidget<btn_unipolar>(Vec(xPos,yPos));
			bt->module = module;
			bt->app::ParamWidget::module = module;
			bt->app::ParamWidget::paramId = Kn8b::UNIPOLAR_PARAM;
			bt->initParamQuantity();
			addParam(bt);
		}
		{
			btn_bipolar *bt = createWidget<btn_bipolar>(Vec(xPos,yPos + 14.f));
			bt->module = module;
			bt->app::ParamWidget::module = module;
			bt->app::ParamWidget::paramId = Kn8b::BIPOLAR_PARAM;
			bt->initParamQuantity();
			addParam(bt);
		}
		{
			btn_sum *bt = createWidget<btn_sum>(Vec(xPos + 18.f,yPos));
			bt->module = module;
			bt->app::ParamWidget::module = module;
			bt->app::ParamWidget::paramId = Kn8b::SUM_PARAM;
			bt->initParamQuantity();
			addParam(bt);
		}
		{
			btn_prod *bt = createWidget<btn_prod>(Vec(xPos + 18.f,yPos + 14.f));
			bt->module = module;
			bt->app::ParamWidget::module = module;
			bt->app::ParamWidget::paramId = Kn8b::PROD_PARAM;
			bt->initParamQuantity();
			addParam(bt);
		}
		{
			btn_page *bt = createWidget<btn_page>(Vec(xPos + 36.f,yPos));
			bt->module = module;
			bt->app::ParamWidget::module = module;
			bt->app::ParamWidget::paramId = Kn8b::PAGE_PARAM;
			bt->initParamQuantity();
			addParam(bt);
		}
		{
			btn_vca *bt = createWidget<btn_vca>(Vec(xPos + 36.f,yPos + 14.f));
			bt->module = module;
			bt->app::ParamWidget::module = module;
			bt->app::ParamWidget::paramId = Kn8b::VCA_PARAM;
			bt->initParamQuantity();
			addParam(bt);
		}
		
		yPos = 331.5f;

		addInput(createInput<moDllzPolyI>(Vec(6.f, yPos),  module, Kn8b::MAIN_INPUT));
		addOutput(createOutput<moDllzPolyO>(Vec(91.f, yPos),  module, Kn8b::MAIN_OUTPUT));
		//			{
		//				ValueTestLCD *MccLCD = createWidget<ValueTestLCD>(Vec(0.f,0.f));
		//				MccLCD->box.size = {60.f, 15.f};
		//				MccLCD->intVal = &module->InOutConnected;
		//				addChild(MccLCD);
		//			}
	}
	
	void appendContextMenu(Menu *menu) override {
		Kn8b *module = dynamic_cast<Kn8b*>(this->module);
		assert(module);
		
		menu->addChild(new MenuSeparator());
		
		if (module->vca){
			menu->addChild(createMenuLabel("Processing rate (VCA)"));
		}else{
			MenuItem *pr = createSubmenuItem("Processing rate", "", [=](Menu* menu) {
				menu->addChild(createCheckMenuItem("sample", "",
												   [=]() {return module->sampleRateWork;},
												[=]() {module->setProcessRate(true);}
												));
				menu->addChild(createCheckMenuItem("1 ms", "",
												   [=]() {return !module->sampleRateWork;},
												[=]() {module->setProcessRate(false);}
												));
			});
			menu->addChild(pr);
		}
	}
};

Model *modelKn8b = createModel<Kn8b, Kn8bWidget>("Kn8b");
