#include "moDllz.hpp"
/*
 * XBender
 */
struct XBender : Module {
	enum ParamIds {
		XBEND_PARAM,
		XBENDCVTRIM_PARAM,
		XBENDRANGE_PARAM,
		BEND_PARAM,
		BENDCVTRIM_PARAM,
		AXISXFADE_PARAM,
		AXISSLEW_PARAM,
		AXISTRNSUP_PARAM,
		AXISTRNSDWN_PARAM,
		AXISMOD_PARAM,
		AXISSELECT_PARAM = 10,
		AXISSELECTCV_PARAM = 18,
		SNAPAXIS_PARAM,
		YCENTER_PARAM,
		YZOOM_PARAM,
		AUTOZOOM_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		IN_INPUT,
		XBENDCV_INPUT = 8,
		XBENDRANGE_INPUT,
		BENDCV_INPUT,
		AXISSELECT_INPUT,
		AXISEXT_INPUT,
		AXISXFADE_INPUT,
		AXISMOD_INPUT,
		AXISSLEW_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUT_OUTPUT,
		AXIS_OUTPUT = 8,
		NUM_OUTPUTS
	};
	enum LightIds {
		AXIS_LIGHT,
		AUTOZOOM_LIGHT = 8,
		SNAPAXIS_LIGHT,
		NUM_LIGHTS
	};
	float inAxis = 0.f;
	float axisSlew = 0.f;
	int axisTransParam = 0;
	float finalAxis = 0.f;
	float AxisShift =  0.f;
	float axisXfade;
	float selectedAxisF = 0.f;
	int selectedAxisI = 0;
	
	float dZoom = 1.f;
	float dCenter = 0.5f;
	int frameAutoZoom = 0;
	
	float slewchanged = 0.f;
	float XBenderKnobVal = 0.f;
	float XBenderKnobCV = 0.f;
	float xbend = 0.f;
	
	float testVal = 0.f;
	
	bool newZoom = false;
	
	dsp::SchmittTrigger axisTransUpTrigger;
	dsp::SchmittTrigger axisTransDwnTrigger;
	dsp::SchmittTrigger axisSelectTrigger[8];
	
	struct ioXBended {
		float inx = 0.0f;
		float xout = 0.0f;
		bool iactive = false;
	};
	
	ioXBended ioxbended[8];
	
	dsp::SlewLimiter slewlimiter;
	
	XBender() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(YCENTER_PARAM, -120.f, 120.f, 0.f);
		configParam(YZOOM_PARAM, 1.f, 12.f, 1.f);
		configParam(AUTOZOOM_PARAM, 0.f, 1.f, 1.f);
		for (int i = 0; i < 8; i++){
			configParam(AXISSELECT_PARAM + i, 0.f, 1.f, 0.f);
		}
		configParam(SNAPAXIS_PARAM, 0.f, 1.f, 0.f);
		configParam(AXISXFADE_PARAM, -1.f, 1.f, 1.f);
		configParam(AXISSLEW_PARAM, 0.f, 1.f, 0.f);
		configParam(AXISMOD_PARAM, 0.f, 1.f, 0.f);
		configParam(AXISTRNSUP_PARAM, 0.f, 1.f, 0.f);
		configParam(AXISTRNSDWN_PARAM, 0.f, 1.f, 0.f);
		configParam(XBEND_PARAM, -1.f, 1.f, 0.f);
		configParam(XBENDCVTRIM_PARAM, 0.f, 24.f, 24.f);
		configParam(XBENDRANGE_PARAM, 1.f, 5.f, 1.f);
		configParam(BEND_PARAM, -1.f, 1.f, 0.f);
		configParam(BENDCVTRIM_PARAM, 0.f, 60.f, 12.f);
	}
	void process(const ProcessArgs &args) override;
	void onReset() override {
		for (int ix = 0; ix < 8 ; ix++){
		outputs[OUT_OUTPUT + ix].setVoltage(inputs[IN_INPUT + ix].getVoltage());
		}
	}
	void onRandomize() override{};
	json_t *dataToJson() override {
		json_t *rootJ = json_object();
		json_object_set_new(rootJ, "selectedAxisI", json_integer(selectedAxisI));
		json_object_set_new(rootJ, "axisTransParam", json_integer(axisTransParam));
		return rootJ;
	}
	void dataFromJson(json_t *rootJ) override {
		json_t *selectedAxisIJ = json_object_get(rootJ,("selectedAxisI"));
		selectedAxisI = json_integer_value(selectedAxisIJ);
		json_t *axisTransParamJ = json_object_get(rootJ,("axisTransParam"));
		axisTransParam = json_integer_value(axisTransParamJ);
	}
};

///////////////////////////////////////////
///////////////STEP //////////////////
/////////////////////////////////////////////

void XBender::process(const ProcessArgs &args) {
	
	if(inputs[AXISSELECT_INPUT].isConnected()) {
		if (params[SNAPAXIS_PARAM].getValue() > 0.5f){
			selectedAxisI = (clamp(static_cast<int>(inputs[AXISSELECT_INPUT].getVoltage() * 0.8f), 0, 7));
			selectedAxisF = static_cast<float>(selectedAxisI);
			inAxis = inputs[selectedAxisI].getVoltage();
		} else {
			selectedAxisF = clamp(inputs[AXISSELECT_INPUT].getVoltage() * 0.7, 0.f, 7.f);
			selectedAxisI = static_cast<int>(selectedAxisF);
			float ax_float = selectedAxisF - static_cast<float>(selectedAxisI);
			inAxis = (inputs[selectedAxisI].getVoltage() * (1.f - ax_float) + (inputs[selectedAxisI + 1].getVoltage() * ax_float));
		}
	}else{
		 inAxis = inputs[selectedAxisI].getVoltage();
	}
	
	lights[SNAPAXIS_LIGHT].value = (params[SNAPAXIS_PARAM].getValue() > 0.5f)? 1.f : 0.f;
	axisXfade = clamp((params[AXISXFADE_PARAM].getValue() + inputs[AXISXFADE_INPUT].getVoltage()/ 10.f), 0.f, 1.f);

	if (inputs[AXISEXT_INPUT].isConnected()){
		float axisext = inputs[AXISEXT_INPUT].getVoltage();
		axisSlew = crossfade(axisext, inAxis, axisXfade);
	} else axisSlew = inAxis;

	float slewsum = clamp((params[AXISSLEW_PARAM].getValue() + inputs[AXISSLEW_INPUT].getVoltage() * .1f), 0.f , 1.f);
	if (slewchanged != slewsum) {
		slewchanged = slewsum;
		float slewfloat = 1.0f/(5.0f + slewsum * args.sampleRate);
		slewlimiter.setRiseFall(slewfloat,slewfloat);
	}
	
	finalAxis = slewlimiter.process(1.f,axisSlew) + (inputs[AXISMOD_INPUT].getVoltage() * params[AXISMOD_PARAM].getValue());
	AxisShift = (static_cast<float>(axisTransParam)/12.f);
	finalAxis += clamp(AxisShift, -12.f,12.f);
	
	float range = clamp((params[XBENDRANGE_PARAM].getValue() + inputs[XBENDRANGE_INPUT].getVoltage()/2.f), 1.f,5.f);
	XBenderKnobCV = (params[XBENDCVTRIM_PARAM].getValue() / 24.f) * (inputs[XBENDCV_INPUT].getVoltage() / 5.f);
	XBenderKnobVal = params[XBEND_PARAM].getValue();
	xbend = clamp((XBenderKnobVal + XBenderKnobCV),-1.f,1.f);
	
	float bend = clamp((params[BEND_PARAM].getValue() + (inputs[BENDCV_INPUT].getVoltage() /5.f) * (params[BENDCVTRIM_PARAM].getValue() /60.f)),-1.f, 1.f);
	
	for (int i = 0; i < 8; i++){
		if (inputs[IN_INPUT + i].isConnected()) {
			if (axisSelectTrigger[i].process(params[AXISSELECT_PARAM + i].getValue())) {
				selectedAxisI = i;
				selectedAxisF = static_cast<float>(i); // float for display
			}
			ioxbended[i].iactive= true;
			lights[AXIS_LIGHT + i].value = (static_cast<int>(selectedAxisF + 0.5f) == i)? 1.f : 0.01f;
			ioxbended[i].inx = inputs[IN_INPUT + i].getVoltage();
			float diff = (finalAxis - ioxbended[i].inx) * xbend * range;
			ioxbended[i].xout = clamp((ioxbended[i].inx + diff + bend * 6.f),-12.f,12.f);
			outputs[OUT_OUTPUT + i].setVoltage(ioxbended[i].xout);
		}else{
			lights[AXIS_LIGHT + i].value = 0;
			ioxbended[i].iactive=false;
		}
	} //for loop i
  
	outputs[AXIS_OUTPUT].setVoltage(finalAxis);
	
	if (axisTransUpTrigger.process(params[AXISTRNSUP_PARAM].getValue()))
			if (axisTransParam < 48) axisTransParam ++;
	if (axisTransDwnTrigger.process(params[AXISTRNSDWN_PARAM].getValue()))
			if (axisTransParam > -48) axisTransParam --;

	bool autoZoom = (params[AUTOZOOM_PARAM].getValue() > 0.f);
	if (autoZoom){
		frameAutoZoom ++;
		if (frameAutoZoom > 128) {
			frameAutoZoom = 0;
		float autoZoomMin = 12.f , autoZoomMax = -12.f;
			int active = 0;
		for (int i = 0; i < 8; i++){
			if (inputs[IN_INPUT + i].isConnected()) {
			active ++;
			if (ioxbended[i].inx < autoZoomMin)
				autoZoomMin = ioxbended[i].inx;
			if (ioxbended[i].xout < autoZoomMin)
				autoZoomMin = ioxbended[i].xout;
			if (ioxbended[i].inx > autoZoomMax)
				autoZoomMax = ioxbended[i].inx;
			if (ioxbended[i].xout > autoZoomMax)
				autoZoomMax = ioxbended[i].xout;
			}
		}
			if (finalAxis < autoZoomMin)
				autoZoomMin = finalAxis;
			if (finalAxis > autoZoomMax)
				autoZoomMax = finalAxis;
			float autoZ = 22.f / clamp((autoZoomMax - autoZoomMin),1.f,24.f);
			float autoCenter = 10.f * (autoZoomMin + (autoZoomMax - autoZoomMin) / 2.f);
			dZoom = clamp(autoZ, 1.f, 15.f);
			dCenter = clamp(autoCenter, -120.f, 120.f);
		}
		lights[AUTOZOOM_LIGHT].value = 10.f;
	}
	else {
		dCenter = params[XBender::YCENTER_PARAM].getValue();
		dZoom = params[XBender::YZOOM_PARAM].getValue();
		lights[AUTOZOOM_LIGHT].value = 0.f;
	}
}/////////////////////   closing STEP   ////////////////////////////////////////////////

///Bend Realtime Display
struct BenderDisplay : TransparentWidget {
	XBender *module;
	BenderDisplay() {
	}
	
	void draw(const DrawArgs &args) override
	{
		if (module) {
			const float dispHeight = 228.f;
			const float dispCenter = dispHeight / 2.f;
			float yZoom = module->dZoom;
			float yCenter = module->dCenter * yZoom + dispCenter;
			float AxisIx = module->selectedAxisF;
			float Axis = module->finalAxis;
			float AxisXfade = module->axisXfade;
			float keyw = 10.f * yZoom /12.f;
			nvgScissor(args.vg, 0.f, 0.f, 152.f, dispHeight);// crop drawing to display
			///// BACKGROUND
			nvgBeginPath(args.vg);
			nvgFillColor(args.vg, nvgRGB(0x2a, 0x2a, 0x2a));
			nvgRect(args.vg, 20.f, yCenter - 120.f * yZoom, 110.f, 20.f * yZoom);
			nvgRect(args.vg, 20.f, yCenter + 100.f * yZoom, 110.f, 20.f * yZoom);
			nvgFill(args.vg);
			
			nvgBeginPath(args.vg);
			if (yZoom > 2.5f) {
				nvgFillColor(args.vg, nvgRGB(0x0, 0x0, 0x0));
				nvgRect(args.vg, 0.f, 0.f, 20.f, dispHeight);
				nvgFill(args.vg);
				nvgBeginPath(args.vg);
				nvgFillColor(args.vg, nvgRGB(0x2f, 0x2f, 0x2f));
			} else {
				nvgFillColor(args.vg, nvgRGB(0x1a, 0x1a, 0x1a));
			}
			nvgRect(args.vg, 20.f, yCenter - 50.f * yZoom, 110.f, 100.f * yZoom );
			nvgFill(args.vg);
			for (int i = 0; i < 11; i++){
				if (yZoom < 2.5f){
					// 1V lines
					nvgBeginPath(args.vg);
					nvgStrokeColor(args.vg, nvgRGB(0x2d,0x2d,0x2d));
					nvgMoveTo(args.vg, 20.f, yCenter - 10.f * yZoom * i);
					nvgLineTo(args.vg, 130.f,yCenter - 10.f * yZoom * i);
					nvgMoveTo(args.vg, 20.f, yCenter + 10.f * yZoom * i);
					nvgLineTo(args.vg, 130.f,yCenter + 10.f * yZoom * i);
					nvgStroke(args.vg);
				}else if (i < 5){
					// keyboard
					float keyPos = yCenter + 10.f * yZoom * i;
					// C's highlight
					nvgBeginPath(args.vg);
					nvgFillColor(args.vg, nvgRGB(0x44, 0x44, 0x44));
					nvgRect(args.vg, 20.f, yCenter - 10.f * yZoom * i - keyw * 0.5f, 110.f, keyw);
					nvgFill(args.vg);
					/// over center
					nvgBeginPath(args.vg);
					nvgFillColor(args.vg, nvgRGB(0x0, 0x0, 0x0));
					nvgRect(args.vg, 20.f, keyPos + keyw * 1.5f, 110.f, keyw);
					nvgRect(args.vg, 20.f, keyPos + keyw * 3.5f , 110.f, keyw);
					nvgRect(args.vg, 20.f, keyPos + keyw * 5.5f, 110.f, keyw);
					nvgRect(args.vg, 20.f, keyPos + keyw * 8.5f, 110.f, keyw);
					nvgRect(args.vg, 20.f, keyPos + keyw * 10.5f, 110.f, keyw);
					nvgFill(args.vg);
					/// C's highlight
					nvgBeginPath(args.vg);
					nvgFillColor(args.vg, nvgRGB(0x44, 0x44, 0x44));
					nvgRect(args.vg, 20.f, yCenter + 10.f * yZoom * i - keyw * 0.5f, 110.f, keyw);
					nvgFill(args.vg);
					/// under center
					keyPos = yCenter - 10.f * yZoom * i;
					nvgBeginPath(args.vg);
					nvgFillColor(args.vg, nvgRGB(0x0, 0x0, 0x0));
					nvgRect(args.vg, 20.f, keyPos - keyw * 1.5f, 110.f, keyw);
					nvgRect(args.vg, 20.f, keyPos - keyw * 3.5f , 110.f, keyw);
					nvgRect(args.vg, 20.f, keyPos - keyw * 6.5f, 110.f, keyw);
					nvgRect(args.vg, 20.f, keyPos - keyw * 8.5f, 110.f, keyw);
					nvgRect(args.vg, 20.f, keyPos - keyw * 10.5f, 110.f, keyw);
					nvgFill(args.vg);
				}
			}
			// center 0v...
			nvgBeginPath(args.vg);
			if (yZoom < 2.5f){
				nvgStrokeColor(args.vg,nvgRGBA(0x80, 0x00, 0x00 ,0x77));
				nvgStrokeWidth(args.vg,1.f);
				nvgMoveTo(args.vg, 20.f, yCenter);
				nvgLineTo(args.vg, 130.f, yCenter);
				nvgStroke(args.vg);
			}//... center C
			else {
				nvgFillColor(args.vg, nvgRGBA(0x80, 0x00, 0x00 ,0x77));
				nvgRect(args.vg, 20.f, yCenter - keyw * 0.5f, 110.f, keyw);
				nvgFill(args.vg);
			}
			// Bend Lines
			const float yfirst = 10.5f;
			const float ystep = 26.f;
			for (int i = 0; i < 8 ; i++){
				if (module->ioxbended[i].iactive) {
				float yport = yfirst + i * ystep;
					float yi =  yZoom * module->ioxbended[i].inx * -10.f + yCenter ;
					float yo =  yZoom * module->ioxbended[i].xout * -10.f + yCenter ;
					nvgBeginPath(args.vg);
					nvgStrokeWidth(args.vg,1.f);
					nvgStrokeColor(args.vg,nvgRGBA(0xff, 0xff, 0xff,0x80));
					nvgMoveTo(args.vg, 0.f, yport);
					nvgLineTo(args.vg, 20.f, yi);
					nvgLineTo(args.vg, 130.f, yo);
					nvgLineTo(args.vg, 150.f, yport);
					nvgStroke(args.vg);
					if (yZoom > 2.5f){
						nvgBeginPath(args.vg);
						nvgFillColor(args.vg, nvgRGBA(0xff, 0xff, 0xff,0x60));
						nvgRoundedRect(args.vg, 15.f, yi - keyw * 0.5f, 10.f, keyw, yZoom / 3.f);
						nvgFill(args.vg);
						nvgBeginPath(args.vg);
						nvgRoundedRect(args.vg, 125.f, yo - keyw * 0.5f, 10.f, keyw, yZoom / 3.f);
						nvgFill(args.vg);
					}
				}
			}
			// Axis Line
			NVGcolor extColor = nvgRGBA(0xee, 0xee, 0x00, (1.f - AxisXfade) * 0xff);
			NVGcolor intColor = nvgRGBA(0xee, 0x00, 0x00, AxisXfade * 0xff);
			NVGcolor axisColor = nvgRGB(0xee, (1.f - AxisXfade) * 0xee, 0x00);
			Axis = yZoom * Axis * -10.f + yCenter;
			nvgStrokeWidth(args.vg,1.f);
			//ext
			nvgBeginPath(args.vg);
			nvgStrokeColor(args.vg,extColor);
			nvgMoveTo(args.vg, 0.f, 228.f);
			nvgLineTo(args.vg, 20.f, Axis);
			nvgStroke(args.vg);
			// int
			nvgBeginPath(args.vg);
			nvgStrokeColor(args.vg,intColor);
			nvgMoveTo(args.vg, 0.f, yfirst + AxisIx * ystep);
			nvgLineTo(args.vg, 20.f, Axis);
			nvgStroke(args.vg);
			// axis
			nvgBeginPath(args.vg);
			nvgStrokeColor(args.vg,axisColor);
			nvgMoveTo(args.vg, 20.f, Axis);
			nvgLineTo(args.vg, 130.f, Axis);
			nvgLineTo(args.vg, 150.f, 222.f);
			nvgStroke(args.vg);
		}else{///PREVIEW
			std::shared_ptr<Font> font;
			std::string s1 = "8 Channel";
			std::string s2 = "Axis X-fader";
			font = APP->window->loadFont(mFONT_FILE);
			nvgFontSize(args.vg, 24.f);
			nvgFontFaceId(args.vg, font->handle);
			nvgTextAlign(args.vg, NVG_ALIGN_CENTER);
			nvgFillColor(args.vg, nvgRGB(0xDD,0xDD,0xDD));
			nvgTextBox(args.vg, 0.f, 80.0f,box.size.x, s1.c_str(), NULL);
			nvgTextBox(args.vg, 0.f, 112.0f,box.size.x, s2.c_str(), NULL);
		}
	}
};

// Transp Display
struct AxisTranspDisplay : TransparentWidget {
	XBender *module;
	std::shared_ptr<Font> font;
	std::string s;
	float mdfontSize = 11.f;
	AxisTranspDisplay(){
		font = APP->window->loadFont(FONT_FILE);
	}
	void draw(const DrawArgs &args) override {
		int AxisTransP = module ? module->axisTransParam : 0;
		s = std::to_string(AxisTransP);
		nvgFontSize(args.vg, mdfontSize);
		nvgFontFaceId(args.vg, font->handle);
		nvgTextAlign(args.vg, NVG_ALIGN_CENTER);
		nvgFillColor(args.vg, nvgRGB(0xFF,0xFF,0xFF));
		nvgTextBox(args.vg, 0.f, 14.0f,box.size.x, s.c_str(), NULL);
	}
};

struct RangeSelector: moDllzSmSelector{
	RangeSelector(){
		minAngle = -0.4*M_PI;
		maxAngle = 0.4*M_PI;
	}
};

struct xbendKnob : SvgKnob {
	xbendKnob() {
		minAngle = -0.83*M_PI;
		maxAngle = 0.83*M_PI;
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/xbendKnob.svg")));
		shadow->opacity = 0.f;
	}
};

struct zTTrim : SvgKnob {
	zTTrim() {
		minAngle = 0;
		maxAngle = 1.75*M_PI;
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/zTTrim.svg")));
		shadow->opacity = 0.f;
	}
};

struct cTTrim : SvgKnob {
	cTTrim() {
		minAngle = -0.83*M_PI;
		maxAngle = 0.83*M_PI;
		snap = true;
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/cTTrim.svg")));
		shadow->opacity = 0.f;
	}
};

struct autoZoom : SvgSwitch {
	autoZoom() {
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/autoButton.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/autoButton.svg")));
	}
};

struct snapAxisButton : SvgSwitch {
  snapAxisButton() {
	  addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/snapButton.svg")));
	  addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/snapButton.svg")));
  }
};

struct XBenderWidget : ModuleWidget {
	XBenderWidget(XBender *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/XBender.svg")));
		float xPos;
		float yPos;
		float xyStep = 26.f ;
		//Screws
		addChild(createWidget<ScrewBlack>(Vec(0, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 15, 0)));
		addChild(createWidget<ScrewBlack>(Vec(0, 365)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 15, 365)));
		xPos = 73.f;
		yPos = 22.f;
		{
			BenderDisplay *benderDisplay = new BenderDisplay();
			benderDisplay->box.pos = Vec(xPos, yPos);
			benderDisplay->box.size = {152.f, 228.f};
			benderDisplay->module = module ?  module : NULL;
		 	addChild(benderDisplay);
		}
		yPos = 252.f;
		/// View Center Zoom
		xPos = 170.f;
		addParam(createParam<cTTrim>(Vec(xPos,yPos), module, XBender::YCENTER_PARAM));
		xPos = 203;
		addParam(createParam<zTTrim>(Vec(xPos,yPos), module, XBender::YZOOM_PARAM));

		yPos += 1.5f;
		xPos = 125.f;
		addParam(createParam<autoZoom>(Vec(xPos, yPos ), module, XBender::AUTOZOOM_PARAM));
		addChild(createLight<TinyLight<RedLight>>(Vec(xPos + 17.f, yPos + 4.f), module, XBender::AUTOZOOM_LIGHT));

		/// IN  - Axis select  - OUTS
		xPos = 12.f;
		yPos = 22.f;
		for (int i = 0; i < 8; i++){
			// IN leds  OUT
			addInput(createInput<moDllzPort>(Vec(xPos, yPos),  module, XBender::IN_INPUT + i));
			addParam(createParam<moDllzRoundButton>(Vec(xPos + 37.f, yPos + 5.f), module, XBender::AXISSELECT_PARAM + i));
			addChild(createLight<TinyLight<RedLight>>(Vec(xPos + 42.5f, yPos + 10.5f), module, XBender::AXIS_LIGHT + i));
			addOutput(createOutput<moDllzPort>(Vec(234.8f, yPos),  module, XBender::OUT_OUTPUT + i));
			yPos += xyStep;
		}
		yPos = 248.f;
		//// AXIS out >>>> on the Right
		addOutput(createOutput<moDllzPort>(Vec(234.8f, yPos),  module, XBender::AXIS_OUTPUT));
		
		yPos = 249.f;
		/// AXIS select in
		addInput(createInput<moDllzPort>(Vec(xPos, yPos),  module, XBender::AXISSELECT_INPUT));
		addParam(createParam<snapAxisButton>(Vec(xPos + 22.5f, yPos + 2.f ), module, XBender::SNAPAXIS_PARAM));
		addChild(createLight<TinyLight<RedLight>>(Vec(xPos + 40.f, yPos + 4.f), module, XBender::SNAPAXIS_LIGHT));

		/// AXIS EXT - XFADE
		yPos += 25.f;
		addInput(createInput<moDllzPort>(Vec(xPos, yPos),  module, XBender::AXISXFADE_INPUT));
		addParam(createParam<moDllzKnob26>(Vec(35.f,280.f), module, XBender::AXISXFADE_PARAM));
	
		yPos += 25.f;
		addInput(createInput<moDllzPort>(Vec(xPos, yPos),  module, XBender::AXISEXT_INPUT));
			
		/// AXIS Slew
		yPos += 25.f;
		addInput(createInput<moDllzPort>(Vec(xPos, yPos),  module, XBender::AXISSLEW_INPUT));
		addParam(createParam<moDllzKnob26>(Vec(34.f, 324.f), module, XBender::AXISSLEW_PARAM));
 
		//AXIS Mod Shift
		addInput(createInput<moDllzPort>(Vec(xPos + 53.f,yPos),  module, XBender::AXISMOD_INPUT));
		addParam(createParam<moDllzKnob26>(Vec(87.f, 324.f), module, XBender::AXISMOD_PARAM));

		//AXIS Transp
		xPos = 73.5f;
		yPos = 286.5f;
		addParam(createParam<moDllzPulseUp>(Vec(xPos + 31.f,yPos - 1.f), module, XBender::AXISTRNSUP_PARAM));
		addParam(createParam<moDllzPulseDwn>(Vec(xPos + 31.f ,yPos + 10.f), module, XBender::AXISTRNSDWN_PARAM));
		
		 {
			AxisTranspDisplay *axisTranspDisplay = new AxisTranspDisplay();
			axisTranspDisplay->box.pos = Vec(xPos, yPos);
			axisTranspDisplay->box.size = {30.f, 20};
			axisTranspDisplay->module = module;
			 addChild(axisTranspDisplay);
		}
		//XBEND
		xPos = 124.f;
		yPos = 272.f;
		
		addParam(createParam<xbendKnob>(Vec(xPos, yPos), module, XBender::XBEND_PARAM));

		xPos = 127.5f;
		yPos = 328.f;
		addInput(createInput<moDllzPort>(Vec(xPos,yPos),  module, XBender::XBENDCV_INPUT));
		addParam(createParam<TTrimSnap>(Vec(xPos + 26.5f,yPos + 7.f), module, XBender::XBENDCVTRIM_PARAM));
		//XBEND RANGE
		xPos = 181.f;
		yPos = 288.f;
		addParam(createParam<RangeSelector>(Vec(xPos, yPos), module, XBender::XBENDRANGE_PARAM));

		xPos = 187.5f;
		yPos = 328.f;
		addInput(createInput<moDllzPort>(Vec(xPos,yPos),  module, XBender::XBENDRANGE_INPUT));
		//BEND
		xPos = 219.f;
		yPos = 288.f;
		addParam(createParam<moDllzKnobM>(Vec(xPos, yPos), module, XBender::BEND_PARAM));

		xPos = 218.5f;
		yPos = 328.f;
		addInput(createInput<moDllzPort>(Vec(xPos,yPos),  module, XBender::BENDCV_INPUT));
		addParam(createParam<TTrimSnap>(Vec(xPos + 26.5f,yPos + 7.f), module, XBender::BENDCVTRIM_PARAM));
	}
};

Model *modelXBender = createModel<XBender, XBenderWidget>("XBender");
