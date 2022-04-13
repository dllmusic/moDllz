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
		LAPSE_PARAM,
		LAPSEMODE_PARAM,
		THRU_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		RISECV_INPUT,
		FALLCV_INPUT,
		SHAPECV_INPUT,
		THRESHCV_INPUT,
		LAPSECV_INPUT,
		GATE_INPUT,
		EXT_INPUT,
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
		THRESH_LIGHT,
		LAPSE_LIGHT,
		THRU_LIGHT,
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
	float g_threshdiff[16] = {0.f};
	float g_shape[16] = {1.f};
	int g_cvstate[16] = {0};
	bool g_newgate[16] = {false};
	int g_lapseframe[16] = {0};
	int g_led[16] = {0};
	int g_ledlapse[16] = {0};
	int g_threLed[16] = {0};
	float riseknob = 0.f;
	float fallknob = 0.f;
	float riseknobcv = 0.f;
	float fallknobcv = 0.f;
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
	int lapsemax = 0;
	float follow[3] = {0.f};
	int followIx = 0;
	bool lapsemode = false;
	int lapsemodeint = 0;
	float lapselight = 0.f;
	float inthrufloat = 0.f;
	bool inthru = false;
	int numCh = 0;
	int prevNumCh = 0;
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
			g_led[i] = 0;
			g_ledlapse[i] = 0;
			g_threLed[i] = 0;
			g_lapseframe[i] = 0;
		}
	}
	void setThreshold(float vt){
		threshmode = (vt >= 0.f);
		threshold = std::abs(vt);
	}
	void setLapseMax(float vw){
		lapsemax = static_cast<int>(vw * APP->engine->getSampleRate());
		lapselight = 0.2f * vw;
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
		configParam(THRESH_PARAM,-2.f,2.f,0.f,"Threshold","v");
		configParam(LAPSE_PARAM,0.f,10.f,0.f,"Glide Timeframe","s");
		configSwitch(LAPSEMODE_PARAM,0.f,1.f,0.f,"Timeframe Mode",{"TimeOut","TimeIn"});
		configSwitch(THRU_PARAM,0.f,1.f,0.f,"Timeframe Through");
		configOutput(GATERISE_OUTPUT,"10V Gate @ Rising");
		configOutput(SHAPE_OUTPUT,"+5-5V Shape Rising/Falling");
		configOutput(GATEFALL_OUTPUT,"10V Gate @ Falling");
		configOutput(OUT_OUTPUT,"Signal Output");
		configInput(RISECV_INPUT,"Rise time CV");
		configInput(FALLCV_INPUT,"Fall time CV");
		configInput(THRESHCV_INPUT,"Threshold CV");
		configInput(LAPSECV_INPUT,"Timeframe CV");
		configInput(GATE_INPUT,"Legato Glide Gate");
		configInput(EXT_INPUT,"Follow (Glide from)");
		configInput(IN_INPUT,"Signal Input");
		configBypass(IN_INPUT,OUT_OUTPUT);
	}
};
///////////////////////////////////////////
/////////////// STEP //////////////////
/////////////////////////////////////////////
void PolyGlider::process(const ProcessArgs &args) {
	numCh = inputs[IN_INPUT].getChannels();
	if (numCh < 1) return;
	outputs[OUT_OUTPUT].setChannels(numCh);
	outputs[GATERISE_OUTPUT].setChannels(numCh);
	outputs[SHAPE_OUTPUT].setChannels(numCh);
	outputs[GATEFALL_OUTPUT].setChannels(numCh);
	// Detector Lag (Lapse)
	if (inputs[LAPSECV_INPUT].isConnected()) setLapseMax(params[LAPSE_PARAM].getValue() * 0.1f * math::clamp(inputs[LAPSECV_INPUT].getVoltage(), 0.f , 10.f));
	lights[LAPSE_LIGHT].setBrightness(lapselight);
	/// Threshold
	if (inputs[THRESHCV_INPUT].isConnected()) setThreshold(params[THRESH_PARAM].getValue() * 0.1f * math::clamp(inputs[THRESHCV_INPUT].getVoltage(), 0.f, 10.f));
	lights[THRESH_LIGHT].setBrightness(threshold);
	/// Channels LOOP
	for (int voCh = 0 ; voCh < numCh; voCh++){
		follow[0] = g_out[voCh];
		follow[1] = g_out[(voCh + numCh - 1) % numCh];
		follow[2] = inputs[EXT_INPUT].getVoltage(voCh);
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
		if (lapsemax > 0){
			if((g_lapseframe[voCh] < lapsemax) != lapsemode){
				g_ledlapse[voCh] = 1 - lapsemodeint;
				if (inthru && (g_sampled[voCh] != g_in[voCh])){
					g_sampled[voCh] = g_in[voCh];
					g_lapseframe[voCh] = 0;
				}
				goto glideNow;
			}
		}
		
		g_ledlapse[voCh] = lapsemodeint;
		if (g_sampled[voCh] != g_in[voCh]) {//new sample
			g_sampled[voCh] = g_in[voCh];
			g_threshdiff[voCh] = std::abs(g_out[voCh] - g_sampled[voCh]);
			g_glideIx[voCh] = 0.f;
			if ((g_threshdiff[voCh] > threshold) == threshmode){
				g_diff[voCh] = follow[followIx] - g_sampled[voCh];
				g_threLed[voCh] = 0;
				g_lapseframe[voCh] = 0;
				if(g_sampled[voCh] > follow[followIx]){
					///neg diff ... pos delta
					g_delta[voCh] = (-1.f + risemode) * g_diff[voCh] + risemode;
					g_cvstate[voCh] = 1;
				}else if(g_sampled[voCh] < follow[followIx]) {
					///pos diff ... neg delta
					g_delta[voCh] = (-1.f + fallmode) * g_diff[voCh] - fallmode;
					g_cvstate[voCh] = 2;
				}
				g_shape[voCh] = math::clamp(cshape + shapecvconnected * inputs[SHAPECV_INPUT].getVoltage(voCh),0.0375f,32.f);
			}else{
				g_threLed[voCh] = 32;
				g_diff[voCh] = 0.f;
				g_cvstate[voCh] = 0;
				g_length[voCh] = 0.f;
			}
		}
		///get live cvs
		switch (g_cvstate[voCh]){
			case 0: break;
			case 1:{
				riseval = std::max((riseknob + risecvconnected * riseknobcv * inputs[RISECV_INPUT].getVoltage(voCh)) * rise10x, 0.f);
				g_length[voCh] = riseval * args.sampleRate;
			}
				break;
			case 2: {
				fallval = std::max((fallknob + fallcvconnected * fallknobcv * inputs[FALLCV_INPUT].getVoltage(voCh)) * fall10x, 0.f);
				g_length[voCh] = fallval * args.sampleRate;
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
		g_out[voCh] =  g_in[voCh] * inthrufloat +  g_sampled[voCh] * (1.f - inthrufloat) + g_glide[voCh];
	glideSkip:
		outputs[OUT_OUTPUT].setVoltage(g_out[voCh], voCh);
		outputs[GATERISE_OUTPUT].setVoltage(g_risegate[voCh], voCh);
		outputs[GATEFALL_OUTPUT].setVoltage(g_fallgate[voCh], voCh);
		outputs[SHAPE_OUTPUT].setVoltage(g_shapeout[voCh], voCh);
		if (g_lapseframe[voCh] < lapsemax) g_lapseframe[voCh]++;
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
				//PolyGlider::resetGlides(0);
			case THRESHCV_INPUT :
				if (!e.connecting) setThreshold(params[THRESH_PARAM].getValue());
				break;
			case LAPSECV_INPUT :
				if (!e.connecting) setLapseMax(params[LAPSE_PARAM].getValue());
				break;
			default:
				break;
		}
	}
	Module::onPortChange(e);
}
//X10 Switch
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
//Thru Switch
struct SWT_inThru : roundPush15 {
	PolyGlider *module = nullptr;
	void onButton(const event::Button &e) override {
		if ((!module)||(module->lapsemode) || (e.button != GLFW_MOUSE_BUTTON_LEFT) || (e.action != GLFW_PRESS)) return;
		module->inthru = !module->inthru;
		module->inthrufloat = (module->inthru)? 1.f : 0.f;
		module->lights[PolyGlider::THRU_LIGHT].setBrightness(module->inthrufloat);
	}
};

//Timeframe mode
struct SWT_lapse : moDllzSwitch {
	PolyGlider *module = nullptr;
	void onChange(const ChangeEvent& e) override {
		if (!module) return;
		engine::ParamQuantity* pq = getParamQuantity();
		if (pq) {
			if (pq->getValue() > 0.5f){
				module->lapsemodeint = 1;
				module->lapsemode = true;
				module->inthrufloat = 1.f;
				module->inthru = true;
			}else{
				module->lapsemodeint = 0;
				module->lapsemode = false;
				module->inthrufloat = 0.f;
				module->inthru = false;
			}
			module->lights[PolyGlider::THRU_LIGHT].setBrightness(module->inthrufloat);
		}
		moDllzSwitch::onChange(e);
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
///RISE
struct KnobRise : moDllzKnobM  {
	PolyGlider *module = nullptr;
	void onChange(const ChangeEvent& e) override{
		if (!module) return;
		moDllzKnobM::onChange(e);
		engine::ParamQuantity* pq = getParamQuantity();
		if (pq) module->riseknob = pq->getSmoothValue();
	}
};
///FALL
struct KnobFall : moDllzKnobM  {
	PolyGlider *module = nullptr;
	void onChange(const ChangeEvent& e) override{
		if (!module) return;
		moDllzKnobM::onChange(e);
		engine::ParamQuantity* pq = getParamQuantity();
		if (pq) module->fallknob = pq->getSmoothValue();
	}
};
///RISECV
struct KnobRiseCv : moDllzKnob18CV  {
	PolyGlider *module = nullptr;
	void onChange(const ChangeEvent& e) override{
		if (!module) return;
		moDllzKnob18CV::onChange(e);
		engine::ParamQuantity* pq = getParamQuantity();
		if (pq) module->riseknobcv = pq->getSmoothValue();
	}
};
///FALLCV
struct KnobFallCv : moDllzKnob18CV  {
	PolyGlider *module = nullptr;
	void onChange(const ChangeEvent& e) override{
		if (!module) return;
		moDllzKnob18CV::onChange(e);
		engine::ParamQuantity* pq = getParamQuantity();
		if (pq) module->fallknobcv = pq->getSmoothValue();
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
		if (pq) module->setLapseMax(pq->getSmoothValue());
	}
};

struct GlideLcd : TransparentWidget {
	PolyGlider *module = nullptr;
	int id = 0;
	NVGcolor yellowR = nvgRGB(0xff, 0xdd, 0x00);
	NVGcolor greenT = nvgRGB(0x44, 0xff, 0x66);
	NVGcolor redR = nvgRGB(0xff, 0x00, 0x00);
	NVGcolor greenTb = nvgRGBA(0x44, 0xff, 0x66,0xaa);
	NVGcolor redRb = nvgRGBA(0xff, 0x00, 0x00,0xbb);
	NVGcolor lapseCol[2] = {redRb,greenTb};
	void drawLayer(const DrawArgs &args, int layer) override {
		if ((!module) || (layer != 1) || !(id < module->numCh)) return;
		
		switch (module->g_ledlapse[id]){
			case 0 :
				nvgBeginPath(args.vg);
				nvgRect(args.vg, 2.f, 7.f , 3.f , 2.f);
				nvgFillColor(args.vg, lapseCol[1 - module->lapsemodeint]);
				nvgFill(args.vg);
				break;
			case 1 :
				int wh  = -14.f + 14.f * (1 + module->g_lapseframe[id])/(1 +module->lapsemax);
				nvgBeginPath(args.vg);
				nvgRect(args.vg, 2.5f, 15.f , 2.f , static_cast<float>(wh));
				nvgFillColor(args.vg, lapseCol[module->lapsemodeint]);
				nvgFill(args.vg);
				break;
		}
		switch (module->g_led[id]){
			case 0 :
				break;
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
		{
			KnobRise *knb = createWidget<KnobRise>(Vec(21.f, yPos));
			knb->module = module;
			knb->app::ParamWidget::module = module;
			knb->app::ParamWidget::paramId = PolyGlider::RISE_PARAM;
			knb->initParamQuantity();
			addParam(knb);
		}
		{
			KnobFall *knb = createWidget<KnobFall>(Vec(70.f, yPos));
			knb->module = module;
			knb->app::ParamWidget::module = module;
			knb->app::ParamWidget::paramId = PolyGlider::FALL_PARAM;
			knb->initParamQuantity();
			addParam(knb);
		}
		/// CV Knobs
		yPos = 94.f;
		{
			KnobRiseCv *knb = createWidget<KnobRiseCv>(Vec(43.5f, yPos));
			knb->module = module;
			knb->app::ParamWidget::module = module;
			knb->app::ParamWidget::paramId = PolyGlider::RISECV_PARAM;
			knb->initParamQuantity();
			addParam(knb);
		}
		{
			KnobFallCv *knb = createWidget<KnobFallCv>(Vec(74.5f, yPos));
			knb->module = module;
			knb->app::ParamWidget::module = module;
			knb->app::ParamWidget::paramId = PolyGlider::FALLCV_PARAM;
			knb->initParamQuantity();
			addParam(knb);
		}
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
		addInput(createInput<moDllzPortI>(Vec(20.f, yPos),  module, PolyGlider::THRESHCV_INPUT));
		{///
			KnobThresh *sw = createWidget<KnobThresh>(Vec(82.5f,yPos));
			sw->module = module;
			sw->app::ParamWidget::module = module;
			sw->app::ParamWidget::paramId = PolyGlider::THRESH_PARAM;
			sw->initParamQuantity();
			addParam(sw);
		}
		addChild(createLight<RedLed2>(Vec(13.f, yPos - 7.f), module, PolyGlider::THRESH_LIGHT));
		/// Lapse
		yPos = 296.f;
		addInput(createInput<moDllzPortI>(Vec(14.f, yPos),  module, PolyGlider::LAPSECV_INPUT));
		{///
			KnobLag *sw = createWidget<KnobLag>(Vec(49.5f, yPos + .5f));
			sw->module = module;
			sw->app::ParamWidget::module = module;
			sw->app::ParamWidget::paramId = PolyGlider::LAPSE_PARAM;
			sw->initParamQuantity();
			addParam(sw);
		}
		{///Timeframe
			SWT_lapse *sw = createWidget<SWT_lapse>(Vec(82.f,yPos + 1.5f));
			sw->module = module;
			sw->app::ParamWidget::module = module;
			sw->app::ParamWidget::paramId = PolyGlider::LAPSEMODE_PARAM;
			sw->initParamQuantity();
			addParam(sw);
		}
		{///in thru
			SWT_inThru *sw = createWidget<SWT_inThru>(Vec(105.f, yPos + 4.f));
			sw->module = module;
			sw->app::ParamWidget::module = module;
			sw->app::ParamWidget::paramId = PolyGlider::THRU_PARAM;
			sw->initParamQuantity();
			sw->addChild(createLight<RedLed2>(Vec(6.5f, 6.5f), module, PolyGlider::THRU_LIGHT));
			addParam(sw);
		}
		addChild(createLight<RedLed2>(Vec(13.f, yPos - 7.f), module, PolyGlider::LAPSE_LIGHT));
		
		/// IN GATE OUT
		yPos = 327.f;
		addInput(createInput<moDllzPolyI>(Vec(13.f, yPos),  module, PolyGlider::IN_INPUT));
		addInput(createInput<moDllzPolyI>(Vec(42.5f, yPos),  module, PolyGlider::GATE_INPUT));
		addInput(createInput<moDllzPolyI>(Vec(70.5f, yPos),  module, PolyGlider::EXT_INPUT));
		addOutput(createOutput<moDllzPolyO>(Vec(101.f, yPos),  module, PolyGlider::OUT_OUTPUT));
	}
};

Model *modelPolyGlider = createModel<PolyGlider, PolyGliderWidget>("PolyGlider");
