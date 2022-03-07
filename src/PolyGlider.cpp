/*
 PolyGlider: Poly slew limiter
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

struct PolyGlider : Module {
	enum ParamIds {
		RISE_PARAM,
		FALL_PARAM,
		RISECV_PARAM,
		FALLCV_PARAM,
		RISEX10_PARAM,
		FALLX10_PARAM,
		RISEMODE_PARAM,
		FALLMODE_PARAM,
		SHAPE_PARAM,
		FOLLOW_PARAM,
		THRESH_PARAM,
		THRESHMODE_PARAM,
		WAIT_PARAM,
		THRU_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		RISECV_INPUT,
		FALLCV_INPUT,
		SHAPECV_INPUT,
		THRESHCV_INPUT,
		WAITCV_INPUT,
		GATE_INPUT,
		IN_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		GATERISE_OUTPUT,
		GATEFALL_OUTPUT,
		SHAPE_OUTPUT,
		OUT_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(RISING_LIGHT,16),
		ENUMS(FALLING_LIGHT,16),
		ENUMS(NOTREADY_LIGHT,16),
		NOTHRESH_LIGHT,
		NUM_LIGHTS
	};
	float g_out[16] = {0.f};
	float g_in[16] = {0.f};
	float g_sampled[16] = {0.f};
	float g_diff[16] = {0.f};
	float g_glide[16] = {0.f};
	float g_length[16] = {0.f};
	float g_glideIx[16] = {0.f};
	float g_delta[16] = {0.f};
	float g_risegate[16] = {0.f};
	float g_fallgate[16] = {0.f};
	float g_shapeout[16] = {0.f};
	bool g_newgate[16] = {false};
	int g_waitframe[16] = {0};
	float g_shape[16] = {1.f};
	float risemode = 0.f;
	float fallmode = 0.f;
	float riseval = 0.f;
	float fallval = 0.f;
	float risecvconnected;// 0.f or 1.f
	float fallcvconnected;// 0.f or 1.f
	float shapecvconnected;// 0.f or 1.f
	float cshape = 1.f;
	bool threshmode;
	float threshold;
	int waitmax = 0;
	int followCh [3];// follow channel param route
	int followIx = 0;
	float inthru;
	int numCh = 0;
	int prevNumCh = 0;

	void process(const ProcessArgs &args) override;
	void onRandomize() override{};
	void onReset() override {
		resetGlides(0);
	}
	void resetGlides(int nv){
		for (int i = nv; i < 16; i++){
			//reset unused
			g_risegate[i] = 0.f;
			g_fallgate[i] = 0.f;
			g_shapeout[i] = 0.f;
			g_glide[i] = 0.f;
			g_out[i] = 0.f;
			outputs[OUT_OUTPUT].setVoltage(0.f, i);
			lights[RISING_LIGHT + i].value = 0.f;
			lights[FALLING_LIGHT + i].value = 0.f;
			lights[NOTREADY_LIGHT + i].value = 0.f;
		}
	}
	PolyGlider() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(RISE_PARAM,0.f,1.f,0.f,"Rise time","s",0.f,1.f);
		configParam(FALL_PARAM,0.f,1.f,0.f,"Fall time","s",0.f,1.f);
		configParam(RISECV_PARAM,-1.f,1.f,0.f,"Rise CV Amount");
		configParam(FALLCV_PARAM,-1.f,1.f,0.f,"Fall CV Amount");
		configSwitch(RISEMODE_PARAM,0.f,1.f,1.f,"Rise",{"constant Time","constant Rate"});
		configSwitch(FALLMODE_PARAM,0.f,1.f,1.f,"Fall",{"constant Time","constant Rate"});
		configSwitch(RISEX10_PARAM,0.f,1.f,0.f,"Rise Factor",{"1x","10x"});
		configSwitch(FALLX10_PARAM,0.f,1.f,0.f,"Fall Factor",{"1x","10x"});
		configParam(SHAPE_PARAM,-1.f,1.f,0.f,"Slope shape");
		configSwitch(FOLLOW_PARAM,0.f,2.f,2.f,"Glide from",{"Self","ch16","ch1"});
		configParam(WAIT_PARAM,0.f,1.f,0.f,"Wait for Glide","s",0.f,1.f);
		configParam(THRESH_PARAM,0.f,1.f,0.f,"Threshold","v");
		configSwitch(THRESHMODE_PARAM,0.f,1.f,0.f,"Glide",{"Below","Above"});
		configSwitch(THRU_PARAM,0.f,1.f,0.f,"During Lag",{"block","pass through"});
		configOutput(GATERISE_OUTPUT,"10V Gate @ Rising");
		configOutput(SHAPE_OUTPUT,"10V Gate @ Rising/Falling");
		configOutput(GATEFALL_OUTPUT,"10V Gate @ Falling");
		configOutput(OUT_OUTPUT,"Glide Output");
		configInput(RISECV_INPUT,"Rise time CV");
		configInput(FALLCV_INPUT,"Fall time CV");
		configInput(THRESHCV_INPUT,"Threshold CV");
		configInput(WAITCV_INPUT,"Wait CV");
		configInput(GATE_INPUT,"Gate Glide");
		configInput(IN_INPUT,"Signal Input");
		configBypass(IN_INPUT,OUT_OUTPUT);
		followCh[1] = 0;
		followCh[2] = 15;
	}
};
///////////////////////////////////////////
/////////////// STEP //////////////////
/////////////////////////////////////////////
void PolyGlider::process(const ProcessArgs &args) {
	if (!inputs[IN_INPUT].isConnected()) {
		if (prevNumCh != 0) {
			PolyGlider::resetGlides(0);
			prevNumCh = 0;
		}
		return;
	}
	numCh = std::max(inputs[IN_INPUT].getChannels(), 1);
	if (numCh != prevNumCh) {
		PolyGlider::resetGlides(numCh);
		prevNumCh = numCh;
	}
	outputs[OUT_OUTPUT].setChannels(numCh);
	outputs[GATERISE_OUTPUT].setChannels(numCh);
	outputs[SHAPE_OUTPUT].setChannels(numCh);
	outputs[GATEFALL_OUTPUT].setChannels(numCh);
	// follow self ch1 or ch16
	followIx = static_cast<int>(params[FOLLOW_PARAM].getValue());
	// rise fall cvs & params
	risemode = params[RISEMODE_PARAM].getValue();
	fallmode = params[FALLMODE_PARAM].getValue();
	risecvconnected = (inputs[RISECV_INPUT].isConnected()) ? 1.f : 0.f;
	fallcvconnected = (inputs[FALLCV_INPUT].isConnected()) ? 1.f : 0.f;
	// Detector Lag
	if (inputs[WAITCV_INPUT].isConnected()) waitmax = static_cast<int>(math::clamp(params[WAIT_PARAM].getValue() * args.sampleRate + inputs[WAITCV_INPUT].getVoltage(), 0.f , 10.f * args.sampleRate ));
	else waitmax = static_cast<int>(params[WAIT_PARAM].getValue() * args.sampleRate);
	/// Change Threshold
	threshmode = static_cast<bool>(params[THRESHMODE_PARAM].getValue());
	if (inputs[THRESHCV_INPUT].isConnected()) threshold = math::clamp(params[THRESH_PARAM].getValue() + inputs[THRESHCV_INPUT].getVoltage(), 0.f , 5.f);
	else threshold = params[THRESH_PARAM].getValue();
	/// thru in->out outside of Lag or thresh
	inthru = params[THRU_PARAM].getValue();
	/// Curve Shape
	shapecvconnected = (inputs[SHAPECV_INPUT].isConnected()) ? 1.f : 0.f;
	cshape = std::pow(2.f, params[SHAPE_PARAM].getValue() * 4.f);
	/// Channels LOOP
	for (int voCh = 0 ; voCh < numCh; voCh++){

		followCh[0] = voCh; //set followCh index[0] to self
		g_in[voCh] = inputs[IN_INPUT].getVoltage(voCh);
		/// check Gate Input
		if (inputs[GATE_INPUT].isConnected()){
			if (inputs[GATE_INPUT].getVoltage(voCh) > 5.f) {
				if (!g_newgate[voCh]){
					g_newgate[voCh] = true;
					g_out[voCh] = g_in[voCh];
					g_risegate[voCh] = 0.f;
					g_fallgate[voCh] = 0.f;
					g_shapeout[voCh] = 0.f;
					goto glideSkip;
				}
			}else{
				g_newgate[voCh] = false;
			}
		}
		if (g_waitframe[voCh] > waitmax -1){
			lights[NOTREADY_LIGHT + voCh].setBrightness(0.f);
			//if ((std::abs(g_in[voCh] - g_out[voCh]) > threshold) == threshmode){
				if (g_sampled[voCh] != g_in[voCh]) {//new sample
					g_sampled[voCh] = g_in[voCh];
					g_diff[voCh] = g_out[followCh[followIx]] - g_sampled[voCh];
					g_glideIx[voCh] = 0.f;
					if ((std::abs(g_diff[voCh]) > threshold) == threshmode){
						g_waitframe[voCh] = 0;
						lights[NOTHRESH_LIGHT].setBrightness(0.f);
						if(g_sampled[voCh] > g_out[followCh[followIx]]){
							///// calc delta RISE
							riseval = math::clamp((params[RISE_PARAM].getValue() + risecvconnected * inputs[RISECV_INPUT].getVoltage(voCh) * params[RISECV_PARAM].getValue()) * (1.f + params[RISEX10_PARAM].getValue() * 9.f), 0.f, 10.f) ;
							///neg diff ... pos delta
							g_delta[voCh] = (-1.f + risemode) * g_diff[voCh] + risemode;
							g_length[voCh] = riseval * args.sampleRate;
						}else if(g_sampled[voCh] < g_out[followCh[followIx]]){
							///// calc delta FALL)
							fallval = math::clamp((params[FALL_PARAM].getValue() + fallcvconnected * inputs[FALLCV_INPUT].getVoltage(voCh) * params[FALLCV_PARAM].getValue()) * (1.f + params[FALLX10_PARAM].getValue() * 9.f), 0.f, 10.f)  ;
							///pos diff ... neg delta
							g_delta[voCh] = (-1.f + fallmode) * g_diff[voCh] - fallmode;
							g_length[voCh] = fallval * args.sampleRate;
						}
						g_shape[voCh] = math::clamp(cshape + shapecvconnected * inputs[SHAPECV_INPUT].getVoltage(voCh),0.0375f,32.f);
					}else{
						lights[NOTHRESH_LIGHT].setBrightness(10.f);
						g_diff[voCh] = 0.f;
						g_length[voCh] = 0.f;
					}
			}
		}else{
			lights[NOTREADY_LIGHT + voCh].setBrightness(10.f);
		}
		/// glide ///
		g_glide[voCh] = g_diff[voCh] + (g_glideIx[voCh] * g_shape[voCh] * g_delta[voCh]) / ((g_shape[voCh] - 1.f) * g_glideIx[voCh] + g_length[voCh] + 1.f);
		g_shapeout[voCh] = 5.f * g_glide[voCh] / std::abs(g_diff[voCh]);
		
		////check end glide
		if (g_diff[voCh] < 0.f) {///rising
			g_glideIx[voCh] += 1.0f;///incr Glide Sample Index
			g_risegate[voCh] = 10.f;///Set Gates
			g_fallgate[voCh] = 0.f;
			if (g_glide[voCh] > 0.f){///end rising
				g_diff[voCh] = 0.f;///Diff 0 sets Glide to Zero
				g_glide[voCh] = 0.f;///rebound to 0
				g_glideIx[voCh] = 0.f;///Sample Index to 0
				g_risegate[voCh] = 0.f; ///Set Gates
				//g_shapeout[voCh] = 0.f;
			}
		}else if (g_diff[voCh] > 0.f) {///falling)
			g_glideIx[voCh] += 1.0f;///incr Glide Sample Index
			g_risegate[voCh] = 0.f; ///Set Gates
			g_fallgate[voCh] = 10.f;
			if (g_glide[voCh] < 0.f) {///end falling
				g_diff[voCh] = 0.f;///Diff 0 sets Glide to Zero
				g_glide[voCh] = 0.f;///rebound to 0
				g_glideIx[voCh] = 0.f;///Sample Index to 0
				g_fallgate[voCh] = 0.f;
			}
		}
		g_out[voCh] =  g_in[voCh] * inthru +  g_sampled[voCh] * (1.f - inthru) + g_glide[voCh];
	glideSkip:
		outputs[OUT_OUTPUT].setVoltage(g_out[voCh], voCh);
		outputs[GATERISE_OUTPUT].setVoltage(g_risegate[voCh], voCh);
		outputs[GATEFALL_OUTPUT].setVoltage(g_fallgate[voCh], voCh);
		outputs[SHAPE_OUTPUT].setVoltage(g_shapeout[voCh], voCh);
		lights[RISING_LIGHT + voCh].setBrightness(g_risegate[voCh]);
		lights[FALLING_LIGHT + voCh].setBrightness(g_fallgate[voCh]);
		if (g_waitframe[voCh] < waitmax) g_waitframe[voCh]++;
	} ///end for loop voCh
}//closing STEP

struct PolyGliderWidget : ModuleWidget {
	PolyGliderWidget(PolyGlider *module){
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/modules/Polyglider.svg")));
		/// Screws
		addChild(createWidget<ScrewBlack>(Vec(0, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 15, 0)));
		addChild(createWidget<ScrewBlack>(Vec(0, 365)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 15, 365)));
		/// Gliding LEDs
		float Ypos = 20.f;
		for ( int i = 0 ; i < 16; i++){
			float Xpos = 17.f + i * 6.f + static_cast<float>(i / 4) * 3.f;
			addChild(createLight<VoiceChGreenLed>(Vec(Xpos, Ypos), module, PolyGlider::RISING_LIGHT + i));
			addChild(createLight<VoiceChGreenLed>(Vec(Xpos, Ypos + 4.f), module, PolyGlider::FALLING_LIGHT + i));
			addChild(createLight<VoiceChRedLed>(Vec(Xpos, Ypos + 2.f), module, PolyGlider::NOTREADY_LIGHT + i));
		}
		/// Glide Knobs
		Ypos = 39.f;
		addParam(createParam<moDllzKnobM>(Vec(21.f, Ypos), module, PolyGlider::RISE_PARAM));
		addParam(createParam<moDllzKnobM>(Vec(70.f, Ypos), module, PolyGlider::FALL_PARAM));
		/// CV Knobs
		Ypos = 94.f;
		addParam(createParam<moDllzKnob18CV>(Vec(43.5f, Ypos), module, PolyGlider::RISECV_PARAM));
		addParam(createParam<moDllzKnob18CV>(Vec(74.5f, Ypos), module, PolyGlider::FALLCV_PARAM));
		/// Glides CVs inputs
		Ypos = 91.5f;
		addInput(createInput<moDllzPolyI>(Vec(13.5f, Ypos),  module, PolyGlider::RISECV_INPUT));
		addInput(createInput<moDllzPolyI>(Vec(99.5f, Ypos),  module, PolyGlider::FALLCV_INPUT));
		/// Mode switches & X10 switches
		Ypos = 130.f;
		addParam(createParam<moDllzSwitchLed>(Vec(21.f, Ypos), module, PolyGlider::RISEX10_PARAM));
		addParam(createParam<moDllzSwitch>(Vec(40.f, Ypos), module, PolyGlider::RISEMODE_PARAM));
		addParam(createParam<moDllzSwitch>(Vec(85.f, Ypos), module, PolyGlider::FALLMODE_PARAM));
		addParam(createParam<moDllzSwitchLed>(Vec(104.f, Ypos), module, PolyGlider::FALLX10_PARAM));
		/// Slope Shape
		Ypos = 170.5f;
		addInput(createInput<moDllzPolyI>(Vec(14.f, Ypos),  module, PolyGlider::SHAPECV_INPUT));
		addParam(createParam<moDllzKnob22>(Vec(50.5f, Ypos +.5f), module, PolyGlider::SHAPE_PARAM));
		/// Follow
		Ypos = 175.f;
		addParam(createParam<moDllzSwitchTH>(Vec(92.f, Ypos), module, PolyGlider::FOLLOW_PARAM));
		Ypos = 214.5f;
		/// Rise Fall Gates
		addOutput(createOutput<moDllzPolyO>(Vec(17.f, Ypos),  module, PolyGlider::GATERISE_OUTPUT));
		addOutput(createOutput<moDllzPolyO>(Vec(56.f, Ypos),  module, PolyGlider::SHAPE_OUTPUT));
		addOutput(createOutput<moDllzPolyO>(Vec(95.f, Ypos),  module, PolyGlider::GATEFALL_OUTPUT));
		/// Threshold
		Ypos = 255.f;
		addInput(createInput<moDllzPortI>(Vec(20.f, Ypos),  module, PolyGlider::THRESHCV_INPUT));
		addParam(createParam<moDllzKnob22>(Vec(58.5f, Ypos +.5f), module, PolyGlider::THRESH_PARAM));
		addParam(createParam<moDllzSwitch>(Vec(101.f, Ypos +1.5f), module, PolyGlider::THRESHMODE_PARAM));
		addChild(createLight<VoiceChRedLed>(Vec(105.f+ 8.f, Ypos + 10.f), module, PolyGlider::NOTHRESH_LIGHT));
		/// Wait
		Ypos = 296.f;
		addInput(createInput<moDllzPortI>(Vec(20.f, Ypos),  module, PolyGlider::WAITCV_INPUT));
		addParam(createParam<moDllzKnob22>(Vec(58.5f, Ypos +.5f), module, PolyGlider::WAIT_PARAM));
		addParam(createParam<moDllzSwitchLed>(Vec(101.f, Ypos + 1.5f), module, PolyGlider::THRU_PARAM));
		/// IN GATE OUT
		Ypos = 326.f;
		addInput(createInput<moDllzPolyI>(Vec(14.f, Ypos),  module, PolyGlider::IN_INPUT));
		addInput(createInput<moDllzPolyI>(Vec(52.5f, Ypos),  module, PolyGlider::GATE_INPUT));
		addOutput(createOutput<moDllzPolyO>(Vec(100.f, Ypos),  module, PolyGlider::OUT_OUTPUT));
		/// TEST VALUE TOP
//		if (module){
//			{
//				ValueTestLCD *MccLCD = createWidget<ValueTestLCD>(Vec(0.f,0.f));
//				MccLCD->box.size = {60.f, 15.f};
//				MccLCD->floatVal = &module->g_glide[0];
//				addChild(MccLCD);
//			}
//			{
//				ValueTestLCD *MccLCD = createWidget<ValueTestLCD>(Vec(60.f,0.f));
//				MccLCD->box.size = {60.f, 15.f};
//				MccLCD->floatVal = &module->g_diff[0];
//				addChild(MccLCD);
//			}
//		}
	}
};

Model *modelPolyGlider = createModel<PolyGlider, PolyGliderWidget>("PolyGlider");
