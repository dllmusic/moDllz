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
		//		ENUMS(RISING_LIGHT,16),
		//		ENUMS(FALLING_LIGHT,16),
		THRESH_LIGHT,
		WAIT_LIGHT,
		NUM_LIGHTS
	};
	float g_out[16] = {0.f};
	float g_in[16] = {0.f};
	//float g_lastin[16] = {0.f};
	float g_sampled[16] = {0.f};
	float g_diff[16] = {0.f};
	float g_glide[16] = {0.f};
	float g_length[16] = {0.f};
	float g_glideIx[16] = {0.f};
	float g_delta[16] = {0.f};
	float g_risegate[16] = {0.f};
	float g_fallgate[16] = {0.f};
	float g_shapeout[16] = {0.f};
	float g_threshdiff[16] = {0.f};
	bool g_newgate[16] = {false};
	int g_waitframe[16] = {0};
	float g_shape[16] = {1.f};
	int g_led[16] = {0};
	int g_threLed[16] = {0};
	float risemode = 0.f;
	float fallmode = 0.f;
	float riseval = 0.f;
	float fallval = 0.f;
	float rise10x = 1.f;
	float fall10x = 1.f;
	float risecvconnected = 0.f;// 0.f or 1.f
	float fallcvconnected = 0.f;// 0.f or 1.f
	float shapecvconnected = 0.f;// 0.f or 1.f
	float cshape = 1.f;
	bool gateinconnected = false;
	bool threshmode;
	float threshold;
	int waitmax = 0;
	int followCh [3] = {0};// follow channel param route
	int followIx = 0;
	float inthru;
	bool binthru = false;
	int numCh = 0;
	float waitlight = 0.f;
	//int prevNumCh = 0;
	
	void process(const ProcessArgs &args) override;
	void onPortChange(const PortChangeEvent& e) override;
	void onRandomize(const RandomizeEvent& e) override {}
	void onReset() override {
		resetGlides(0);
	}
	void resetGlides(int nv){
		for (int i = nv; i < 16; i++){
			g_risegate[i] = 0.f;
			g_fallgate[i] = 0.f;
			g_shapeout[i] = 0.f;
			g_glide[i] = 0.f;
			g_out[i] = 0.f;
			outputs[OUT_OUTPUT].setVoltage(0.f, i);
			g_led[i] = 0.f;
		}
	}
	void setThreshold(float vt){
		threshmode = (vt >= 0.f);
		threshold = std::abs(vt);
	}
	void setWaitMax(float vw){
		waitmax = static_cast<int>(vw * APP->engine->getSampleRate());
		waitlight = 0.2f * vw;
	}
	
	PolyGlider() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(RISE_PARAM,0.f,1.f,0.f,"Rise time","s",0.f,1.f);
		configParam(FALL_PARAM,0.f,1.f,0.f,"Fall time","s",0.f,1.f);
		configParam(RISECV_PARAM,-1.f,1.f,0.f,"Rise CV Amount");
		configParam(FALLCV_PARAM,-1.f,1.f,0.f,"Fall CV Amount");
		configSwitch(RISEMODE_PARAM,0.f,1.f,0.f,"Rise",{"constant Time","constant Rate"});
		configSwitch(FALLMODE_PARAM,0.f,1.f,0.f,"Fall",{"constant Time","constant Rate"});
		configSwitch(RISEX10_PARAM,0.f,1.f,0.f,"Rise Factor",{"1x","10x"});
		configSwitch(FALLX10_PARAM,0.f,1.f,0.f,"Fall Factor",{"1x","10x"});
		configParam(SHAPE_PARAM,-1.f,1.f,0.f,"Slope shape");
		configSwitch(FOLLOW_PARAM,0.f,2.f,0.f,"Glide from",{"Self","ch16","ch1"});
		configParam(WAIT_PARAM,0.f,10.f,0.f,"Wait for Glide","s");
		configParam(THRESH_PARAM,-2.f,2.f,0.f,"Threshold","v");
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
	}
};
///////////////////////////////////////////
/////////////// STEP //////////////////
/////////////////////////////////////////////
void PolyGlider::process(const ProcessArgs &args) {
	numCh = std::max(inputs[IN_INPUT].getChannels(), 0);
	//	if (numCh != prevNumCh) {
	//		PolyGlider::resetGlides(numCh);
	//		prevNumCh = numCh;
	//	}
	if (numCh < 1) return;
	outputs[OUT_OUTPUT].setChannels(numCh);
	outputs[GATERISE_OUTPUT].setChannels(numCh);
	outputs[SHAPE_OUTPUT].setChannels(numCh);
	outputs[GATEFALL_OUTPUT].setChannels(numCh);
	// Detector Lag (Wait)
	if (inputs[WAITCV_INPUT].isConnected()) setWaitMax(params[WAIT_PARAM].getValue() * 0.1f * math::clamp(inputs[WAITCV_INPUT].getVoltage(), 0.f , 10.f));
	lights[WAIT_LIGHT].setBrightness(waitlight);
	/// Threshold
	if (inputs[THRESHCV_INPUT].isConnected()) setThreshold(params[THRESH_PARAM].getValue() * 0.1f * math::clamp(inputs[THRESHCV_INPUT].getVoltage(), 0.f, 10.f));
	lights[THRESH_LIGHT].setBrightness(threshold);
	/// Channels LOOP
	for (int voCh = 0 ; voCh < numCh; voCh++){
		followCh[0] = voCh; //set followCh index[0] to self
		followCh[1] = (voCh + numCh - 1) % numCh; //prev channel (followCh[2] = 0 = ch1)
		g_in[voCh] = inputs[IN_INPUT].getVoltage(voCh);
		/// check Gate Input
		if (gateinconnected){
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
		///
		if (waitmax > 0){
			if(g_waitframe[voCh] < waitmax){
				g_led[voCh] = 1;
				if (binthru && (g_sampled[voCh] != g_in[voCh])){
					g_sampled[voCh] = g_in[voCh];
					g_waitframe[voCh] = 0;
				}
				goto glideNow;
			}
		}
		
		g_led[voCh] = 0;
		if (g_sampled[voCh] != g_in[voCh]) {//new sample
			g_sampled[voCh] = g_in[voCh];
			//g_lastin[voCh] = g_in[voCh];
			g_threshdiff[voCh] = std::abs(g_out[voCh] - g_sampled[voCh]);
			g_glideIx[voCh] = 0.f;
			if ((g_threshdiff[voCh] > threshold) == threshmode){
				g_diff[voCh] = g_out[followCh[followIx]] - g_sampled[voCh];
				g_threLed[voCh] = 0;
				g_waitframe[voCh] = 0;
				if(g_sampled[voCh] > g_out[followCh[followIx]]){
					///// calc delta RISE
					riseval = math::clamp((params[RISE_PARAM].getValue() + risecvconnected * inputs[RISECV_INPUT].getVoltage(voCh) * params[RISECV_PARAM].getValue()) * rise10x, 0.f, 10.f) ;
					///neg diff ... pos delta
					g_delta[voCh] = (-1.f + risemode) * g_diff[voCh] + risemode;
					g_length[voCh] = riseval * args.sampleRate;
				}else if(g_sampled[voCh] < g_out[followCh[followIx]]){
					///// calc delta FALL)
					fallval = math::clamp((params[FALL_PARAM].getValue() + fallcvconnected * inputs[FALLCV_INPUT].getVoltage(voCh) * params[FALLCV_PARAM].getValue()) * fall10x, 0.f, 10.f)  ;
					///pos diff ... neg delta
					g_delta[voCh] = (-1.f + fallmode) * g_diff[voCh] - fallmode;
					g_length[voCh] = fallval * args.sampleRate;
				}
				g_shape[voCh] = math::clamp(cshape + shapecvconnected * inputs[SHAPECV_INPUT].getVoltage(voCh),0.0375f,32.f);
			}else{
				g_threLed[voCh] = 32;
				g_diff[voCh] = 0.f;
				g_length[voCh] = 0.f;
			}
		}
		
		/// glide ///
		
		glideNow :
		g_glide[voCh] = g_diff[voCh] + (g_glideIx[voCh] * g_shape[voCh] * g_delta[voCh]) / ((g_shape[voCh] - 1.f) * g_glideIx[voCh] + g_length[voCh] + 1.f);
		g_shapeout[voCh] = 5.f * g_glide[voCh] / std::abs(g_diff[voCh]);
		////check end glide
		if (g_diff[voCh] < 0.f) {///rising
			g_glideIx[voCh] += 1.0f;///incr Glide Sample Index
			g_risegate[voCh] = 10.f;///Set Gates
			g_fallgate[voCh] = 0.f;
			g_led[voCh] = 2;
			if (g_glide[voCh] > 0.f){///end rising
				g_diff[voCh] = 0.f;///Diff 0 sets Glide to Zero
				g_glide[voCh] = 0.f;///rebound to 0
				g_glideIx[voCh] = 0.f;///Sample Index to 0
				g_risegate[voCh] = 0.f; ///Set Gates
				g_led[voCh] = 0;
			}
		}else if (g_diff[voCh] > 0.f) {///falling)
			g_glideIx[voCh] += 1.0f;///incr Glide Sample Index
			g_risegate[voCh] = 0.f; ///Set Gates
			g_fallgate[voCh] = 10.f;
			g_led[voCh] = 3;
			if (g_glide[voCh] < 0.f) {///end falling
				g_diff[voCh] = 0.f;///Diff 0 sets Glide to Zero
				g_glide[voCh] = 0.f;///rebound to 0
				g_glideIx[voCh] = 0.f;///Sample Index to 0
				g_fallgate[voCh] = 0.f;
				g_led[voCh] = 0;
			}
		}
		g_out[voCh] =  g_in[voCh] * inthru +  g_sampled[voCh] * (1.f - inthru) + g_glide[voCh];
	glideSkip:
		outputs[OUT_OUTPUT].setVoltage(g_out[voCh], voCh);
		outputs[GATERISE_OUTPUT].setVoltage(g_risegate[voCh], voCh);
		outputs[GATEFALL_OUTPUT].setVoltage(g_fallgate[voCh], voCh);
		outputs[SHAPE_OUTPUT].setVoltage(g_shapeout[voCh], voCh);
		if (g_waitframe[voCh] < waitmax) g_waitframe[voCh]++;
	} ///end for loop voCh
}//closing STEP

void PolyGlider::onPortChange(const PortChangeEvent& e) {
	if (e.type == Port::INPUT) {
		switch (e.portId) {
			case RISECV_INPUT :
				risecvconnected = (e.connecting)? 1.f : 0.f;
				break;
			case FALLCV_INPUT :
				fallcvconnected = (e.connecting)? 1.f : 0.f;
				break;
			case SHAPECV_INPUT :
				shapecvconnected = (e.connecting)? 1.f : 0.f;
				break;
			case GATE_INPUT :
				gateinconnected = (e.connecting)? 1.f : 0.f;
				break;
			case IN_INPUT :
				PolyGlider::resetGlides(0);
			case THRESHCV_INPUT :
				if (!e.connecting) setThreshold(params[THRESH_PARAM].getValue());
				break;
			case WAITCV_INPUT :
				if (!e.connecting) setWaitMax(params[WAIT_PARAM].getValue());
				break;
			default:
				break;
		}
	}
	Module::onPortChange(e);
}

struct SWT_x10 : moDllzSwitchLed {
	PolyGlider *module = nullptr;
	int id = 0;
	void onChange(const ChangeEvent& e) override {
		if (!module) return;
		engine::ParamQuantity* pq = getParamQuantity();
		if (pq){
			float pv = 1.f + pq->getValue() * 9.f;
			if (id < 1){//rise x10
				module->rise10x = pv;
				module->paramQuantities[PolyGlider::RISE_PARAM]->displayMultiplier = pv;
			}else{//fall x10
				module->fall10x = pv;
				module->paramQuantities[PolyGlider::FALL_PARAM]->displayMultiplier = pv;
			}
		}
		moDllzSwitchLed::onChange(e);
	}
};

struct SWT_inThru : moDllzSwitchLed {
	PolyGlider *module = nullptr;
	void onChange(const ChangeEvent& e) override {
		if (!module) return;
		engine::ParamQuantity* pq = getParamQuantity();
		if (pq) {
			module->inthru = pq->getValue();
			module->binthru = pq->getValue() > 0.5f;
		}
		moDllzSwitchLed::onChange(e);
	}
};
//rise fall mode
struct SWT_mode : moDllzSwitch {
	PolyGlider *module = nullptr;
	int id = 0;
	void onChange(const ChangeEvent& e) override {
		if (!module) return;
		engine::ParamQuantity* pq = getParamQuantity();
		if (pq) {
			if (id < 1) module->risemode = pq->getValue();
			else module->fallmode = pq->getValue();
		}
		moDllzSwitch::onChange(e);
	}
};
//followIx
struct SWT_follow : moDllzSwitchTH {
	PolyGlider *module = nullptr;
	void onChange(const ChangeEvent& e) override {
		if (!module) return;
		engine::ParamQuantity* pq = getParamQuantity();
		if (pq) {
			module->followIx = static_cast<int>(pq->getValue());
		}
		moDllzSwitchTH::onChange(e);
	}
};
///SHAPE
struct KnobShape : moDllzKnob22  {
	PolyGlider *module = nullptr;
	void onChange(const ChangeEvent& e) override{
		if (!module) return;
		moDllzKnob22::onChange(e);
		engine::ParamQuantity* pq = getParamQuantity();
		if (pq) module->cshape = std::pow(2.f, pq->getSmoothValue() * 4.f);
	}
};
///THRESHOLD
struct KnobThresh : moDllzKnob22  {
	PolyGlider *module = nullptr;
	void onChange(const ChangeEvent& e) override{
		if (!module) return;
		moDllzKnob22::onChange(e);
		engine::ParamQuantity* pq = getParamQuantity();
		if (pq) module->setThreshold(pq->getSmoothValue());
	}
};
///DETECTOR LAG
struct KnobLag : moDllzKnob22  {
	PolyGlider *module = nullptr;
	void onChange(const ChangeEvent& e) override{
		if (!module) return;
		moDllzKnob22::onChange(e);
		engine::ParamQuantity* pq = getParamQuantity();
		if (pq) module->setWaitMax(pq->getSmoothValue());
	}
};

struct GlideLcd : TransparentWidget {
	PolyGlider *module = nullptr;
	int id = 0;
	NVGcolor yellowR = nvgRGB(0xff, 0xdd, 0x00);
	NVGcolor greenT = nvgRGB(0x44, 0xff, 0x66);
	NVGcolor redR = nvgRGB(0xff, 0x00, 0x00);
	float fwaitmax = 0.f;
	float fwait = 0.f;
	float wh = 0.f;
	void drawLayer(const DrawArgs &args, int layer) override {
		if ((!module) || (layer != 1)) return;
		
		switch (module->g_led[id]){
			case 1 :
				fwaitmax = static_cast<float>(module->waitmax);
				fwait = static_cast<float>(module->g_waitframe[id]);
				wh  = -14.f + 14.f * fwait/fwaitmax;
				drawRect(args.vg, 2.f, 14.f, 4.f, wh, redR);
				//drawRect(args.vg, 1.f, 7.f, 5.f, 2.f, redR);
				return;
			case 2 :
				drawTri(args.vg, 1.f, 6.f, 5.f, 4.33f, 1.f ,greenT);
				return;
			case 3 :
				drawTri(args.vg, 1.f, 10.f, 5.f, 4.33f, -1.f, greenT);
				return;
		}
		if (module->g_threLed[id] > 0) {
			module->g_threLed[id] --;
			drawRect(args.vg, 1.f, 7.f, 5.f, 2.f, yellowR);
		}
	}
	
	void drawRect(NVGcontext *nvg, float xa, float ya, float xs, float ys, NVGcolor rc){
		nvgFillColor(nvg, nvgTransRGBA(rc, 0x16));
		for (int i = 1; i<4; i++){
			nvgBeginPath(nvg);
			float fi = static_cast<float>(i);
			nvgRoundedRect(nvg, xa - fi, ya - fi, xs + fi * 2.f, ys + fi * 2.f, fi);
			nvgFill(nvg);
		}
		nvgBeginPath(nvg);
		nvgRect(nvg, xa, ya,xs,ys);
		nvgFillColor(nvg, rc);
		nvgFill(nvg);
	}
	
	void drawTri(NVGcontext *nvg, float xa, float ya, float bb, float hh, float di, NVGcolor tc){
		nvgFillColor(nvg, nvgTransRGBA(tc, 0x16));
		for (int i = 1; i<4; i++){
			float fi = static_cast<float>(i);
			nvgBeginPath(nvg);
			nvgMoveTo(nvg, xa - fi, ya + fi * di);
			nvgLineTo(nvg, xa + bb * .5f, ya - (hh + fi * 2.f) * di);
			nvgLineTo(nvg, xa + bb + fi, ya + fi * di);
			nvgClosePath(nvg);
			nvgFill(nvg);
		}
		nvgBeginPath(nvg);
		nvgMoveTo(nvg, xa, ya);
		nvgLineTo(nvg, xa + bb * .5f, ya - hh * di);
		nvgLineTo(nvg, xa + bb, ya);
		nvgClosePath(nvg);
		nvgFillColor(nvg, tc);
		nvgFill(nvg);
	}
};

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
		float yPos = 21.f;
		for ( int i = 0 ; i < 16; i++){
			float xPos = 10.f + i * 7.f + static_cast<float>(i / 4);
			GlideLcd *gLcd = createWidget<GlideLcd>(Vec(xPos, yPos));
			gLcd->box.size = {7.f, 16.f};
			gLcd->module = module;
			gLcd->id = i;
			addChild(gLcd);
		}
		/// Glide Knobs
		yPos = 40.f;
		addParam(createParam<moDllzKnobM>(Vec(21.f, yPos), module, PolyGlider::RISE_PARAM));
		addParam(createParam<moDllzKnobM>(Vec(70.f, yPos), module, PolyGlider::FALL_PARAM));
		/// CV Knobs
		yPos = 94.f;
		addParam(createParam<moDllzKnob18CV>(Vec(43.5f, yPos), module, PolyGlider::RISECV_PARAM));
		addParam(createParam<moDllzKnob18CV>(Vec(74.5f, yPos), module, PolyGlider::FALLCV_PARAM));
		/// Glides CVs inputs
		yPos = 91.5f;
		addInput(createInput<moDllzPolyI>(Vec(13.5f, yPos),  module, PolyGlider::RISECV_INPUT));
		addInput(createInput<moDllzPolyI>(Vec(99.5f, yPos),  module, PolyGlider::FALLCV_INPUT));
		/// Mode switches & X10 switches
		yPos = 130.f;
		{///RISE 10X
			SWT_x10 *sw = createWidget<SWT_x10>(Vec(21.f,yPos));
			sw->module = module;
			sw->id = 0;
			sw->app::ParamWidget::module = module;
			sw->app::ParamWidget::paramId = PolyGlider::RISEX10_PARAM;
			sw->initParamQuantity();
			addParam(sw);
		}
		{///FALL 10X
			SWT_x10 *sw = createWidget<SWT_x10>(Vec(104.f,yPos));
			sw->module = module;
			sw->id = 1;
			sw->app::ParamWidget::module = module;
			sw->app::ParamWidget::paramId = PolyGlider::FALLX10_PARAM;
			sw->initParamQuantity();
			addParam(sw);
		}
		{///Rise  Mode
			SWT_mode *sw = createWidget<SWT_mode>(Vec(40.f,yPos));
			sw->module = module;
			sw->id = 0;
			sw->app::ParamWidget::module = module;
			sw->app::ParamWidget::paramId = PolyGlider::RISEMODE_PARAM;
			sw->initParamQuantity();
			addParam(sw);
		}
		{///Fall  Mode
			SWT_mode *sw = createWidget<SWT_mode>(Vec(85.f,yPos));
			sw->module = module;
			sw->id = 1;
			sw->app::ParamWidget::module = module;
			sw->app::ParamWidget::paramId = PolyGlider::FALLMODE_PARAM;
			sw->initParamQuantity();
			addParam(sw);
		}
		/// Slope Shape
		yPos = 170.5f;
		addInput(createInput<moDllzPolyI>(Vec(14.f, yPos),  module, PolyGlider::SHAPECV_INPUT));
		///knob shape
		{
			KnobShape *knb = createWidget<KnobShape>(Vec(48.5f, yPos +.5f));
			knb->module = module;
			knb->app::ParamWidget::module = module;
			knb->app::ParamWidget::paramId = PolyGlider::SHAPE_PARAM;
			knb->initParamQuantity();
			addParam(knb);
		}
		yPos = 175.f;
		{///Follow
			SWT_follow *sw = createWidget<SWT_follow>(Vec(91.f,yPos));
			sw->module = module;
			sw->app::ParamWidget::module = module;
			sw->app::ParamWidget::paramId = PolyGlider::FOLLOW_PARAM;
			sw->initParamQuantity();
			addParam(sw);
		}
		yPos = 214.5f;
		/// Rise Fall Gates
		addOutput(createOutput<moDllzPolyO>(Vec(17.f, yPos),  module, PolyGlider::GATERISE_OUTPUT));
		addOutput(createOutput<moDllzPolyO>(Vec(56.f, yPos),  module, PolyGlider::SHAPE_OUTPUT));
		addOutput(createOutput<moDllzPolyO>(Vec(95.f, yPos),  module, PolyGlider::GATEFALL_OUTPUT));
		/// Threshold
		yPos = 255.f;
		addChild(createLight<RedLed3>(Vec(14.f, yPos - 5.f), module, PolyGlider::THRESH_LIGHT));
		addInput(createInput<moDllzPortI>(Vec(20.f, yPos),  module, PolyGlider::THRESHCV_INPUT));
		{///
			KnobThresh *sw = createWidget<KnobThresh>(Vec(82.5f,yPos));
			sw->module = module;
			sw->app::ParamWidget::module = module;
			sw->app::ParamWidget::paramId = PolyGlider::THRESH_PARAM;
			sw->initParamQuantity();
			addParam(sw);
		}
		/// Wait
		yPos = 296.f;
		addChild(createLight<RedLed3>(Vec(14.f, yPos - 5.f), module, PolyGlider::WAIT_LIGHT));
		addInput(createInput<moDllzPortI>(Vec(20.f, yPos),  module, PolyGlider::WAITCV_INPUT));
		{///in thru
			SWT_inThru *sw = createWidget<SWT_inThru>(Vec(101.f,yPos + 1.5f));
			sw->module = module;
			sw->app::ParamWidget::module = module;
			sw->app::ParamWidget::paramId = PolyGlider::THRU_PARAM;
			sw->initParamQuantity();
			addParam(sw);
		}
		{///
			KnobLag *sw = createWidget<KnobLag>(Vec(58.5f, yPos +.5f));
			sw->module = module;
			sw->app::ParamWidget::module = module;
			sw->app::ParamWidget::paramId = PolyGlider::WAIT_PARAM;
			sw->initParamQuantity();
			addParam(sw);
		}
		
		/// IN GATE OUT
		yPos = 326.f;
		addInput(createInput<moDllzPolyI>(Vec(14.f, yPos),  module, PolyGlider::IN_INPUT));
		addInput(createInput<moDllzPolyI>(Vec(52.5f, yPos),  module, PolyGlider::GATE_INPUT));
		addOutput(createOutput<moDllzPolyO>(Vec(100.f, yPos),  module, PolyGlider::OUT_OUTPUT));
		/// TEST  TOP
		//				if (module){
		//					{
		//						ValueTestLCD *MccLCD = createWidget<ValueTestLCD>(Vec(0.f,0.f));
		//						MccLCD->box.size = {60.f, 15.f};
		//						MccLCD->floatVal = &module->threshold;
		//						addChild(MccLCD);
		//					}
		//					{
		//						ValueTestLCD *MccLCD = createWidget<ValueTestLCD>(Vec(60.f,0.f));
		//						MccLCD->box.size = {60.f, 15.f};
		//						MccLCD->intVal = &module->waitmax;
		//						addChild(MccLCD);
		//					}
		//				}
	}
};

Model *modelPolyGlider = createModel<PolyGlider, PolyGliderWidget>("PolyGlider");
