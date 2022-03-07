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

struct Kn8b :  Module {
	enum ParamIds {

		ENUMS(KNOB_PARAM, 8),
		MAINKNOB_PARAM,
		CH1_8_PARAM,
		CH9_16_PARAM,
		UNIPOLAR_PARAM,
		BIPOLAR_PARAM,
		SUM_PARAM,
		PROD_PARAM,
		ROUTE_PARAM,
		NUM_PARAMS
	};
	enum OutputIds {
		MAIN_OUTPUT,
		NUM_OUTPUTS
	};
	enum InputIds {
		MAIN_INPUT,
		CV_INPUT,
		NUM_INPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};
	
	int chOffset = 0;
	int active[16] = {0};
	float inV[16] = {0.f};
	float knobVal[16] = {0.f};
	float calcVal[16] = {0.f};
	float cvVal[16] = {0.f};
	float outV[16] = {0.f};
	float trimVal = 0.f;
	float operation[16] = {0.f};
	float polarity[16] = {0.f};
	unsigned char InOutConnected = 0;
	float incnnctd = 0.f;
	float cvcnnctd = 0.f;
	int numInCh = 0;
	int numOutCh = 8;
	const float processMs = 0.02f;//sec
	int PROCESS_RATE = static_cast<int>(APP->engine->getSampleRate() * processMs);
	int ProcessFrame = 0;
	///////////////////////////////////////////////////////////////////////////////////////
	json_t *dataToJson() override {
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
	///////////////////////////////////////////////////////////////////////////////////////
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
		
	}
	/////////////////////////////////////////////////////////////////////////////////////////
	
	Kn8b() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configSwitch(CH1_8_PARAM, 0.f, 1.f, 0.f,"Channels 1 to 8");
		configSwitch(CH9_16_PARAM, 0.f, 1.f, 0.f,"Channels 9 to 16");
		configSwitch(UNIPOLAR_PARAM, 0.f, 1.f, 0.f,"Set Knobs to Unipolar");
		configSwitch(BIPOLAR_PARAM, 0.f, 1.f, 0.f,"Set Knobs to Bipolar");
		configSwitch(SUM_PARAM, 0.f, 1.f, 0.f,"Set to Input + Knobs");
		configSwitch(PROD_PARAM, 0.f, 1.f, 0.f,"Set to Input x Knobs");
		configInput(MAIN_INPUT, "to Knobs");
		configInput(CV_INPUT, "Knobs CV");
		configOutput(MAIN_OUTPUT, "Knobs");
		configParam(MAINKNOB_PARAM,-1.f, 1.f, 0.f,"Trim","x(-5/5|0/10)v");
		configBypass(MAIN_INPUT , MAIN_OUTPUT);
		for (int i = 0 ; i < 8; i++){
			configParam(KNOB_PARAM + i, -1.f, 1.f, 0.f,"Knob "+std::to_string(i + 1),"v",0.f,5.f);
		}
		knobsPage();
	}
	
	void onSampleRateChange() override {
		PROCESS_RATE = static_cast<int>(APP->engine->getSampleRate() * processMs);
	}
	
	void process(const ProcessArgs &args) override {
//		if (ProcessFrame++ < PROCESS_RATE) return;
//		ProcessFrame = 0;
		int numCvIn = std::max(inputs[CV_INPUT].getChannels(), static_cast<int>(cvcnnctd));
		for (int i = 0; i< numCvIn; i++){
			cvVal[i] = inputs[CV_INPUT].getVoltage(i);
			updateCalcVal(i);
		}
		switch (InOutConnected) {
			case 0: {//Knob->Lcd
					for (int i = 0 ; i < numOutCh ; i++){
						outV[i] = math::clamp(calcVal[i], -5.f + (5.f * polarity[i]), 5.f + (5.f * polarity[i]));
						active[i] = 1;
					}
					for (int i = numOutCh ; i < 16; i++){
						active[i] = 0;
					}
				
			}break;
			case 1: {//input->Lcd
				numInCh = std::max(inputs[MAIN_INPUT].getChannels(), 1);
				int io = 0;
					for (int i = 0 ; i < std::min(numInCh,numOutCh) ; i++){
						inV[i] = inputs[MAIN_INPUT].getVoltage(i);
						outV[i] = math::clamp(inThruOp(i), -5.f + (5.f * polarity[i]), 5.f + (5.f * polarity[i]));
						active[i] = 2;
						io++;
					}
					for (int i = io ; i < 16; i++){
						active[i] = 0;
					}
				
			}break;
			case 2: {//knobs->output
				outputs[MAIN_OUTPUT].setChannels(numOutCh);
				for (int i = 0 ; i < numOutCh ; i++){
					outV[i] = math::clamp(calcVal[i], -5.f + (5.f * polarity[i]), 5.f + (5.f * polarity[i]));
					active[i] = 3;
					outputs[MAIN_OUTPUT].setVoltage(outV[i], i);
				}
				for (int i = numOutCh ; i < 16; i++){
					active[i] = 0;
				}
				
			}break;
			case 3: {//input+*Knobs->output
				outputs[MAIN_OUTPUT].setChannels(numOutCh);
				numInCh = std::max(inputs[MAIN_INPUT].getChannels(), 1);
				int io = 0;
				for (int i = 0 ; i < std::min(numInCh,numOutCh) ; i++){
					inV[i] = inputs[MAIN_INPUT].getVoltage(i);
					outV[i] = math::clamp(inThruOp(i), -5.f + (5.f * polarity[i]), 5.f + (5.f * polarity[i]));
					active[i] = 4;
					outputs[MAIN_OUTPUT].setVoltage(outV[i], i);
					io++;
				}
				for (int i = io ; i < numOutCh; i++){
					outV[i] = math::clamp(calcVal[i], -5.f + (5.f * polarity[i]), 5.f + (5.f * polarity[i]));
					active[i] = 3;
					outputs[MAIN_OUTPUT].setVoltage(outV[i], i);
				}
				for (int i = numOutCh ; i < 16; i++){
					active[i] = 0;
				}
			}break;
		}
	}
	
	void knobsPage(){
		for (int i = 0; i < 8 ; i++){
			knobsInfo(i);
			paramQuantities[KNOB_PARAM + i]->name= "Ch:" + std::to_string(i + 1 + chOffset);
			paramQuantities[KNOB_PARAM + i]->setValue(knobVal[i + chOffset]);
		}
	}
	
	void knobsInfo(int i){
			int pid = KNOB_PARAM + i;
			if (operation[i+chOffset] < 0.5f){
				paramQuantities[pid]->displayMultiplier = 5.f;
				paramQuantities[pid]->unit= "v";
				paramQuantities[pid]->defaultValue= 0.f;
			}else{
				paramQuantities[pid]->displayMultiplier = 1.f;
				paramQuantities[pid]->unit= "x";
				paramQuantities[pid]->defaultValue= 1.f;
			}
	}
	void knobsUniBipolar(float ub , int ifrom, int ito){
		for (int i = ifrom; i< ito; i++){
			polarity[i] = ub;
			updateCalcVal(i);
		}
	}
	void knobsSumProd(float op , int ifrom, int ito){
		for (int i = ifrom; i< ito; i++){
			operation[i] = op;
			updateCalcVal(i);
		}
	}
	
	void updateCalcVal(int i){
		float opin = operation[i] * incnnctd;// + (1.f - incnnctd);// 0+ or 1x if connected 0 disc
		float op = (1.f - opin) * 5.f + opin; //5 + or 1 x
		float cv = (1.f - cvcnnctd) * trimVal * 5.f + cvcnnctd * cvVal[i] * trimVal;
		calcVal[i] = op * (knobVal[i] + polarity[i]) + cv;
	}

	float inThruOp(int i){
		return  (1.f - operation[i]) * (calcVal[i] + inV[i]) + inV[i] * calcVal[i] * operation[i];
	}
	
	void onPortChange(const PortChangeEvent& e) override {
		//		/** True if connecting, false if disconnecting. */
		//		bool connecting;
		//		/** Port::INPUT or Port::OUTPUT */
		//		Port::Type type;
		//		int portId;
		if (e.type == Port::INPUT) {
			if (e.portId == MAIN_INPUT){
				for (int i = 0; i < 8 ; i++){
					knobsInfo(i);
				}
				if (e.connecting){
					incnnctd = 1.f;
					InOutConnected += 1;
				}else{
					incnnctd = 0.f;
					InOutConnected -= 1;
					for (int i = 0; i< 16 ; i++){
						inV[i] = 0.f;
					}
				}
			}else{//CV input
				if (e.connecting){
					cvcnnctd =  1.f;
					//paramQuantities[MAINKNOB_PARAM]->displayMultiplier = 1.f;
					paramQuantities[MAINKNOB_PARAM]->unit= "x CV";
				}else{
					cvcnnctd =  0.f;
					//paramQuantities[MAINKNOB_PARAM]->displayMultiplier = 5.f;
					paramQuantities[MAINKNOB_PARAM]->unit= "x(5|10)v";
				}
			}
		}else{
			InOutConnected += (e.connecting)? 2 : -2;
		}
		Module::onPortChange(e);
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
			module->updateCalcVal(ix);
		}
	}
};

struct Knob26G : Knob26x  {
	Knob26G() {}
	void onChange(const ChangeEvent& e) override{
		if (!module) return;
		moDllzKnob26::onChange(e);
		engine::ParamQuantity* pq = getParamQuantity();
		if (pq){
			module->trimVal = pq->getSmoothValue();
			for (int i = 0; i< module->numOutCh; i++){
				module->updateCalcVal(i);
			}
		}
	}
};

struct btn_ch1_8 : SvgSwitch {
	btn_ch1_8() {
		momentary = true;
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/buttons/btn_ch1_8.svg")));
	}
	Kn8b *module = nullptr;
	void onDragEnd(const DragEndEvent& e) override {
		if (!module) return;
		module->chOffset = 0;
		module->knobsPage();
		SvgSwitch::onDragEnd(e);
	}
};
struct btn_ch9_16 : SvgSwitch {
	btn_ch9_16() {
		momentary = true;
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/buttons/btn_ch9_16.svg")));
	}
	Kn8b *module = nullptr;
	void onDragEnd(const DragEndEvent& e) override {
		if (!module) return;
		module->chOffset = 8;
		module->knobsPage();
		SvgSwitch::onDragEnd(e);
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
		momentary = true;
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
////LCD
struct Kn8bLCD : TransparentWidget{
	Kn8bLCD() {
	}
	Kn8b *module = nullptr;
	std::shared_ptr<Font> font;
	int id = 0;
	//int frameDisplay = 0;
	std::string modetxt[4] = {"+","","x","x"};
	float mdfontSize = 14.f;
	
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
		nvgFontFaceId(args.vg, font->handle);
		nvgFontSize(args.vg, mdfontSize - 2.f);
		nvgTextAlign(args.vg, NVG_ALIGN_CENTER);
		if (module->active[ix] < 1){
			nvgFillColor(args.vg, nvgRGB(0x88, 0x88, 0x88));
			nvgTextBox(args.vg, 0.f, 10.f,12.f, strout.c_str(), NULL);
			return;
		}
		nvgFillColor(args.vg, nvgRGB(0xdd, 0xdd, 0xdd));
		unsigned char dim = static_cast<unsigned char>(module->incnnctd) * 0x88 + 0x55;
		nvgStrokeColor(args.vg, nvgRGB(dim, dim, dim));
		nvgTextBox(args.vg, 0.f, 10.f,12.f, strout.c_str(), NULL);
		int operation = module->operation[ix];
		nvgBeginPath(args.vg);
		nvgStrokeWidth(args.vg, 1.f);
		if (operation > 0){// prod
			//nvgStrokeColor(args.vg, nvgRGBA(0xdd, 0x11, 0x11,alph));
			nvgMoveTo(args.vg, 3.25f, 14.25f);
			nvgLineTo(args.vg, 8.75f, 19.75f);
			nvgMoveTo(args.vg, 8.75f, 14.25f);
			nvgLineTo(args.vg, 3.25f, 19.75f);
			//nvgFillColor(args.vg, nvgRGBA(0xdd, 0x11, 0x11,alph * 0.2f));
		}else{// sum
			//nvgStrokeColor(args.vg, nvgRGBA(0xdd, 0x88, 0x00,alph));
			nvgMoveTo(args.vg, 6.f, 13.5f);
			nvgLineTo(args.vg, 6.f, 20.5f);
			nvgMoveTo(args.vg, 2.5f, 17.f);
			nvgLineTo(args.vg, 9.5f, 17.f);
			//nvgFillColor(args.vg, nvgRGBA(0xdd, 0x88, 0x00,alph * 0.2f));
		}
		nvgStroke(args.vg);
//		nvgBeginPath(args.vg);
//		nvgCircle(args.vg, -20.f, 14.5f, 15.5f);
//		nvgFill(args.vg);
		
		nvgBeginPath(args.vg);
//		nvgFillColor(args.vg, nvgRGB(0xdd, 0xdd, 0xdd));
		nvgStrokeColor(args.vg, nvgRGB(0xdd, 0xdd, 0xdd));
		if(module->polarity[ix] > 0){
			nvgRect(args.vg, 3.5f, 23.f, 5.f, 8.f);
			nvgFill(args.vg);
			nvgBeginPath(args.vg);
			nvgMoveTo(args.vg, 1.5f, 30.5f);
			nvgLineTo(args.vg, 10.5f, 30.5f);
			nvgStroke(args.vg);
		}else{
			nvgRect(args.vg, 3.5f, 23.f, 5.f, 4.f);
			nvgFill(args.vg);
			nvgBeginPath(args.vg);
			nvgRect(args.vg, 4.f, 27.f, 4.f, 3.5f);
			nvgMoveTo(args.vg, 1.5f, 27.f);
			nvgLineTo(args.vg, 10.5f, 27.f);
			nvgStroke(args.vg);
		}
		nvgFontSize(args.vg, mdfontSize);
		nvgTextAlign(args.vg, NVG_ALIGN_RIGHT);
		std::stringstream stream;
		float lineW = box.size.x-1.f;
		float ypos = 19.f;
		switch (module->active[ix]) {
			case 1: {////////////// NOT CONNECTED
				if (module->cvcnnctd > 0.f){
					stream << std::fixed << std::setprecision(3) << module->cvVal[ix];
					strout = stream.str() + "v";
					nvgFontSize(args.vg, mdfontSize + 1.f);
					nvgFillColor(args.vg, colorCv);
					nvgTextBox(args.vg, 0.f, 12.f, lineW, strout.c_str(), NULL);
					ypos = 26.f;
				}
				stream.str(std::string());
				stream << std::fixed << std::setprecision(3) << module->outV[ix];
				strout = stream.str() + "v";
				nvgFontSize(args.vg, mdfontSize + 1.f);
				nvgFillColor(args.vg, colorNoConn);
				nvgTextBox(args.vg, 0.f, ypos, lineW, strout.c_str(), NULL);
			} break;
			case 2: {//input -> Lcd  /////IN V
				stream << std::fixed << std::setprecision(3) << module->inV[ix];
				strout = stream.str() + "v";
				nvgFontSize(args.vg, mdfontSize + 1.f);
				nvgFillColor(args.vg, colorIn);
				nvgTextBox(args.vg, 0.f, 12.f, lineW, strout.c_str(), NULL);
				stream.str(std::string());///// Knob
				stream << std::fixed << std::setprecision(3) << module->calcVal[ix];
				strout = modetxt[operation * 2 + static_cast<int>(module->calcVal[ix] < 0.f)] + stream.str();
				nvgFontSize(args.vg, mdfontSize);
				nvgFillColor(args.vg, nvgRGB(0xff, 0xaa * (1-operation), 0x77));
				nvgTextBox(args.vg, 0.f, 26.f, lineW, strout.c_str(), NULL);
			} break;
			case 3: {//////////////OUT V
				if (module->cvcnnctd > 0.f){
					stream << std::fixed << std::setprecision(3) << module->cvVal[ix];
					strout = stream.str() + "v";
					nvgFontSize(args.vg, mdfontSize + 1.f);
					nvgFillColor(args.vg, colorCv);
					nvgTextBox(args.vg, 0.f, 12.f, lineW, strout.c_str(), NULL);
					ypos = 26.f;
				}
				stream.str(std::string());
				stream << std::fixed << std::setprecision(3) << module->outV[ix];
				strout = stream.str() + "v";
				nvgFontSize(args.vg, mdfontSize + 1.f);
				nvgFillColor(args.vg, colorOut);
				nvgTextBox(args.vg, 0.f, ypos, lineW, strout.c_str(), NULL);
			} break;
			case 4: {//thru knobs /////IN V
				nvgFontSize(args.vg, mdfontSize - 2.f);
				stream << std::fixed << std::setprecision(3) << module->inV[ix];
				strout = stream.str() + "v";
				nvgFillColor(args.vg, colorIn);
				nvgTextBox(args.vg, 0.f, 9.f, lineW, strout.c_str(), NULL);
				stream.str(std::string());/////////////// Knob
				stream << std::fixed << std::setprecision(3) << module->calcVal[ix];
				strout = modetxt[operation * 2 + static_cast<int>(module->calcVal[ix]< 0.f)] + stream.str();
				nvgFillColor(args.vg, opColor[operation]);
				nvgTextBox(args.vg, 0.f, 18.5f, lineW, strout.c_str(), NULL);
				stream.str(std::string());///////////OUT V
				stream << std::fixed << std::setprecision(3) << module->outV[ix];
				strout = stream.str() + "v";
				nvgFontSize(args.vg, mdfontSize);
				nvgFillColor(args.vg, colorOut);
				nvgTextBox(args.vg, 0.f, 31.f, lineW, strout.c_str(), NULL);
			} break;
		}
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
		if (!module){
			app::SvgScrew *prepanel = new app::SvgScrew;
			prepanel->setSvg(Svg::load(asset::plugin(pluginInstance, "res/modules/Kn8b_pre.svg")));
			addChild(prepanel);
			return;
		}
		addChild(createWidget<ScrewBlack>(Vec(0, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 15, 0)));
		addChild(createWidget<ScrewBlack>(Vec(0, 365)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 15, 365)));
		float yPos = 18.5f;

			for (int i = 0 ; i < 8; i++){
				Kn8bLCD *MccLCD = createWidget<Kn8bLCD>(Vec(40.f,yPos));
				MccLCD->box.size = {60.f, 33.f};
				MccLCD->module = module;
				MccLCD->id = i;
				addChild(MccLCD);

				Knob26x * knb = new Knob26x;
				knb->box.pos = Vec(7.f, yPos + 3.5f);
				knb->module = module;
				knb->id = i;
				knb->app::ParamWidget::module = module;
				knb->app::ParamWidget::paramId = Kn8b::KNOB_PARAM + i;
				knb->initParamQuantity();
				addParam(knb);
				yPos += 35.f;
			 }
		yPos = 301.f;

		{
			btn_ch1_8 *bt = createWidget<btn_ch1_8>(Vec(41.f,yPos));
			bt->module = module;
			bt->app::ParamWidget::module = module;
			bt->app::ParamWidget::paramId = Kn8b::CH1_8_PARAM;
			bt->initParamQuantity();
			addParam(bt);
		}
		{
			btn_ch9_16 *bt = createWidget<btn_ch9_16>(Vec(41.f,yPos + 14.f));
			bt->module = module;
			bt->app::ParamWidget::module = module;
			bt->app::ParamWidget::paramId = Kn8b::CH9_16_PARAM;
			bt->initParamQuantity();
			addParam(bt);
		}
		{
			btn_unipolar *bt = createWidget<btn_unipolar>(Vec(61.f,yPos));
			bt->module = module;
			bt->app::ParamWidget::module = module;
			bt->app::ParamWidget::paramId = Kn8b::UNIPOLAR_PARAM;
			bt->initParamQuantity();
			addParam(bt);
		}
		{
			btn_bipolar *bt = createWidget<btn_bipolar>(Vec(61.f,yPos + 14.f));
			bt->module = module;
			bt->app::ParamWidget::module = module;
			bt->app::ParamWidget::paramId = Kn8b::BIPOLAR_PARAM;
			bt->initParamQuantity();
			addParam(bt);
		}
		{
			btn_sum *bt = createWidget<btn_sum>(Vec(81.f,yPos));
			bt->module = module;
			bt->app::ParamWidget::module = module;
			bt->app::ParamWidget::paramId = Kn8b::SUM_PARAM;
			bt->initParamQuantity();
			addParam(bt);
		}
		{
			btn_prod *bt = createWidget<btn_prod>(Vec(81.f,yPos + 14.f));
			bt->module = module;
			bt->app::ParamWidget::module = module;
			bt->app::ParamWidget::paramId = Kn8b::PROD_PARAM;
			bt->initParamQuantity();
			addParam(bt);
		}
		yPos = 300.f;
		addInput(createInput<moDllzPolyI>(Vec(8.5f, yPos),  module, Kn8b::CV_INPUT));
		
		yPos = 333.5f;
		{
			Knob26G* knb = new Knob26G;
			knb->box.pos = Vec(39.5f, yPos+ 1.5f);
			knb->module = module;
			knb->app::ParamWidget::module = module;
			knb->app::ParamWidget::paramId = Kn8b::MAINKNOB_PARAM;
			knb->initParamQuantity();
			addParam(knb);
		}
		addInput(createInput<moDllzPolyI>(Vec(8.5f, yPos),  module, Kn8b::MAIN_INPUT));
		addOutput(createOutput<moDllzPolyO>(Vec(73.5f, yPos),  module, Kn8b::MAIN_OUTPUT));
////			{
////				ValueTestLCD *MccLCD = createWidget<ValueTestLCD>(Vec(0.f,0.f));
////				MccLCD->box.size = {60.f, 15.f};
////				MccLCD->intVal = &module->InOutConnected;
////				addChild(MccLCD);
////			}
	}
};

Model *modelKn8b = createModel<Kn8b, Kn8bWidget>("Kn8b");
