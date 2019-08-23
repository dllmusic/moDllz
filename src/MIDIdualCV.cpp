#include "moDllz.hpp"
/*
 * MIDIdualCV converts upper/lower midi note on/off events, velocity , channel aftertouch, pitch wheel,  mod wheel breath cc and expression to CV
 */
struct MidiNoteData {
	uint8_t velocity = 0;
	uint8_t aftertouch = 0;
};

struct MIDIdualCV :  Module {
	enum ParamIds {
		RESETMIDI_PARAM,
		LWRRETRGGMODE_PARAM,
		UPRRETRGGMODE_PARAM,
		SUSTAINHOLD_PARAM,
		PBPOS_UPPER_PARAM,
		PBNEG_UPPER_PARAM,
		PBPOS_LOWER_PARAM,
		PBNEG_LOWER_PARAM,
		SLEW_LOWER_PARAM,
		SLEW_UPPER_PARAM,
		SLEW_LOWER_MODE_PARAM,
		SLEW_UPPER_MODE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		NUM_INPUTS
	};
	enum OutputIds {
		PITCH_OUTPUT_Lwr,
		PITCH_OUTPUT_Upr,
		VELOCITY_OUTPUT_Lwr,
		VELOCITY_OUTPUT_Upr,
		RETRIGGATE_OUTPUT_Lwr,
		RETRIGGATE_OUTPUT_Upr,
		GATE_OUTPUT,
		PBEND_OUTPUT,
		MOD_OUTPUT,
		EXPRESSION_OUTPUT,
		BREATH_OUTPUT,
		SUSTAIN_OUTPUT,
		PRESSURE_OUTPUT,
		PBENDPOS_OUTPUT,
		PBENDNEG_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		RESETMIDI_LIGHT,
		SUSTHOLD_LIGHT,
		NUM_LIGHTS
	};
 
	midi::InputQueue midiInput;
	
	uint8_t mod = 0;
	dsp::ExponentialFilter modFilter;
	uint8_t breath = 0;
	dsp::ExponentialFilter breathFilter;
	uint8_t expression = 0;
	dsp::ExponentialFilter exprFilter;
	uint16_t pitch = 8192;
	dsp::ExponentialFilter pitchFilter;
	uint8_t sustain = 0;
	dsp::ExponentialFilter sustainFilter;
	uint8_t pressure = 0;
	dsp::ExponentialFilter pressureFilter;

	struct NoteData {
		uint8_t velocity = 0;
		uint8_t aftertouch = 0;
	};
	
	NoteData noteData[128];
	
	dsp::SlewLimiter slewlimiterLwr;
	dsp::SlewLimiter slewlimiterUpr;
	
	float slewLwr = 0.f;
	float slewUpr = 0.f;
	
	struct noteLive{
		int note = 0;
		uint8_t vel = 0;
		float volt = 0.f;
	};
	noteLive lowerNote;
	noteLive upperNote;
	bool anynoteGate = false;
	bool sustpedal = false;
	bool sustpedalgate = false;
	bool firstNoGlideLwr = false;
	bool firstNoGlideUpr = false;
	uint8_t lastLwr = 128;
	uint8_t lastUpr = -1;
	
	dsp::PulseGenerator gatePulseLwr;
	dsp::PulseGenerator gatePulseUpr;
	dsp::SchmittTrigger resetMidiTrigger;
	
	bool noteupdated = false;
	int srFrametime = APP->engine->getSampleRate() / 500;
	int processframe = 0;
	float srSampleTime = APP->engine->getSampleTime() * 100.f;
	
	MIDIdualCV() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(RESETMIDI_PARAM, 0.0f, 1.0f, 0.0f);
		configParam(PBNEG_LOWER_PARAM, 24.f, -24.0f, 0.f);
		configParam(PBPOS_LOWER_PARAM, -24.f, 24.0f, 0.f);
		configParam(PBNEG_UPPER_PARAM, 24.f, -24.0f, 0.f);
		configParam(PBPOS_UPPER_PARAM, -24.f, 24.0f, 0.f);
		configParam(SLEW_LOWER_PARAM, 0.f, 1.f, 0.f);
		configParam(SLEW_UPPER_PARAM, 0.f, 1.f, 0.f);
		configParam(SLEW_LOWER_MODE_PARAM, 0.f, 1.f, 0.f);
		configParam(SLEW_UPPER_MODE_PARAM, 0.f, 1.f, 0.f);
		configParam(LWRRETRGGMODE_PARAM, 0.0, 1.0, 0.0);
		configParam(UPRRETRGGMODE_PARAM, 0.0, 1.0, 0.0);
		configParam(SUSTAINHOLD_PARAM, 0.0, 1.0, 1.0);
		setLambdas();
	}
	
	void onSampleRateChange() override {
		setLambdas();
		slewLwr = 0.f;//zero to refresh rate
		slewUpr = 0.f;//zero to refresh rate
	}

	void onRandomize() override{}
	
	void setLambdas(){
		srFrametime = APP->engine->getSampleRate() / 500;
		srSampleTime = APP->engine->getSampleTime() * 100.f;
		modFilter.lambda = srSampleTime;
		breathFilter.lambda = srSampleTime;
		exprFilter.lambda = srSampleTime;
		sustainFilter.lambda = srSampleTime;
		pressureFilter.lambda = srSampleTime;
		pitchFilter.lambda = srSampleTime;
	}
	///////////////////         ////           ////          ////         /////////////////////
	/////////////////   ///////////////  /////////  ////////////  //////  ////////////////////
	/////////////////         ////////  /////////       ///////         /////////////////////
	///////////////////////   ///////  /////////  ////////////  ////////////////////////////
	//////////////          ////////  /////////         /////  ////////////////////////////
	
	void process(const ProcessArgs &args) override {
		
		midi::Message msg;
		while (midiInput.shift(&msg)) {
			processMessage(msg);
		}
		
		//pitchFilter.lambda = 100.f * args.sampleTime;
		float pitchwheel;
		float pitchtocvLWR = 0.f;
		float pitchtocvUPR = 0.f;
		if (pitch < 8192){
			pitchwheel = pitchFilter.process(1.f,rescale(pitch, 0, 8192, -5.f, 0.f));
			outputs[PBENDNEG_OUTPUT].setVoltage(pitchwheel * 2.f);
			outputs[PBENDPOS_OUTPUT].setVoltage(0.f);
			pitchtocvLWR = pitchwheel * params[PBNEG_LOWER_PARAM].getValue() / 60.f;
			pitchtocvUPR = pitchwheel * params[PBNEG_UPPER_PARAM].getValue() / 60.f;
		} else {
			pitchwheel = pitchFilter.process(1.f,rescale(pitch, 8192, 16383, 0.f, 5.f));
			outputs[PBENDPOS_OUTPUT].setVoltage(pitchwheel * 2.f);
			outputs[PBENDNEG_OUTPUT].setVoltage(0.f);
			pitchtocvLWR = pitchwheel * params[PBPOS_LOWER_PARAM].getValue() / 60.f;
			pitchtocvUPR = pitchwheel * params[PBPOS_UPPER_PARAM].getValue() / 60.f;
		}
		outputs[PBEND_OUTPUT].setVoltage(pitchwheel);
		
		///////////////////////
		if (processframe++ > srFrametime) {
		
			processframe = 0;
		if (noteupdated){
			anynoteGate = false;
			noteupdated = false;
			///LOWER///
			for (int i = 0; i < 128; i++){
				if (noteData[i].velocity > 0){
					anynoteGate = true;
					/////////
					if (i != lastLwr){
						lastLwr = i;
						if (params[LWRRETRGGMODE_PARAM].getValue() > 0.5f)
							gatePulseLwr.trigger(1e-3);
						else if (i < lowerNote.note)
							gatePulseLwr.trigger(1e-3);
					}
					lowerNote.note = i;
					lowerNote.vel = noteData[i].velocity;
					break;
				}
			}
			if (anynoteGate){
					///UPPER///
					for (int i = 127; i > -1; i--){
						if (noteData[i].velocity > 0){
							if (i != lastUpr){
								lastUpr = i;
								if (params[UPRRETRGGMODE_PARAM].getValue() > 0.5f)
									gatePulseUpr.trigger(1e-3);
								else if (i > upperNote.note)
									gatePulseUpr.trigger(1e-3);
							}
							upperNote.note = i;
							upperNote.vel = noteData[i].velocity;
							break;
						}
					}
					lowerNote.volt = static_cast<float>(lowerNote.note - 60) / 12.0f;
					upperNote.volt =static_cast<float>(upperNote.note - 60) / 12.0f;
					outputs[VELOCITY_OUTPUT_Lwr].setVoltage(static_cast<float>(lowerNote.vel) / 127.0f * 10.0f);
					outputs[VELOCITY_OUTPUT_Upr].setVoltage(static_cast<float>(upperNote.vel) / 127.0 * 10.0);
			}else{
				lowerNote.note = 128;
				upperNote.note = -1;
			}
		}
		}
		if (slewLwr != params[SLEW_LOWER_PARAM].getValue()) {
			slewLwr = params[SLEW_LOWER_PARAM].getValue();
			float slewfloat = 1.0f/(5.0f + slewLwr * args.sampleRate);
			slewlimiterLwr.setRiseFall(slewfloat,slewfloat);
		}

		if (slewLwr > 0.f)
			if (firstNoGlideLwr){
				slewlimiterLwr.setRiseFall(1.f,1.f);
				outputs[PITCH_OUTPUT_Lwr].setVoltage(slewlimiterLwr.process(1.f, lowerNote.volt) + pitchtocvLWR);
				slewLwr = 0.f; // value to retrigger calc next note
			}else{
				outputs[PITCH_OUTPUT_Lwr].setVoltage(slewlimiterLwr.process(1.f, lowerNote.volt) + pitchtocvLWR);
			}
		
		else
			 outputs[PITCH_OUTPUT_Lwr].setVoltage(lowerNote.volt + pitchtocvLWR);
		
		if (slewUpr != params[SLEW_UPPER_PARAM].getValue()) {
			slewUpr = params[SLEW_UPPER_PARAM].getValue();
			float slewfloat = 1.0f/(5.0f + slewUpr * args.sampleRate);
			slewlimiterUpr.setRiseFall(slewfloat,slewfloat);
		}
		if (slewUpr > 0.f)
			if (firstNoGlideUpr){
				slewlimiterUpr.setRiseFall(1.f,1.f);
				outputs[PITCH_OUTPUT_Upr].setVoltage(slewlimiterUpr.process(1.f, upperNote.volt) + pitchtocvUPR);
				slewUpr = 0.f; // value to retrigger calc next note
			}else{
				outputs[PITCH_OUTPUT_Upr].setVoltage(slewlimiterUpr.process(1.f, upperNote.volt) + pitchtocvUPR);
			}
		else
			outputs[PITCH_OUTPUT_Upr].setVoltage(upperNote.volt + pitchtocvUPR);
		
		bool retriggLwr = gatePulseLwr.process(1.f / args.sampleRate);
		bool retriggUpr = gatePulseUpr.process(1.f / args.sampleRate);
		bool gateout = anynoteGate || sustpedalgate;
		
		outputs[RETRIGGATE_OUTPUT_Lwr].setVoltage(gateout && !(retriggLwr)? 10.f : 0.f );
		outputs[RETRIGGATE_OUTPUT_Upr].setVoltage(gateout && !(retriggUpr)? 10.f : 0.f );
		outputs[GATE_OUTPUT].setVoltage(gateout ? 10.f : 0.f );
		
		//modFilter.lambda = 100.f * args.sampleTime;
		outputs[MOD_OUTPUT].setVoltage(modFilter.process(1.f, rescale(mod, 0, 127, 0.f, 10.f)));
		
		//breathFilter.lambda = 100.f * args.sampleTime;
		outputs[BREATH_OUTPUT].setVoltage(breathFilter.process(1.f, rescale(breath, 0, 127, 0.f, 10.f)));
		
		//exprFilter.lambda = 100.f * args.sampleTime;
		outputs[EXPRESSION_OUTPUT].setVoltage(exprFilter.process(1.f, rescale(expression, 0, 127, 0.f, 10.f)));
		
		//sustainFilter.lambda = 100.f * args.sampleTime;
		outputs[SUSTAIN_OUTPUT].setVoltage(sustainFilter.process(1.f, rescale(sustain, 0, 127, 0.f, 10.f)));
		
		//pressureFilter.lambda = 100.f * args.sampleTime;
		outputs[PRESSURE_OUTPUT].setVoltage(pressureFilter.process(1.f, rescale(pressure, 0, 127, 0.f, 10.f)));
	
		///// RESET MIDI LIGHT
		if (resetMidiTrigger.process(params[RESETMIDI_PARAM].getValue())) {
			lights[RESETMIDI_LIGHT].value= 1.0f;
			MidiPanic();
			return;
		}
		if (lights[RESETMIDI_LIGHT].value > 0.0001f){
			lights[RESETMIDI_LIGHT].value -= 0.0001f ; // fade out light
		}
		//}
	}
/////////////////////// * * * ///////////////////////////////////////////////// * * *
//					  * * *		 E  N  D	  O  F	 S  T  E  P		  * * *
/////////////////////// * * * ///////////////////////////////////////////////// * * *

	void processMessage(midi::Message msg) {
		switch (msg.getStatus()) {
			case 0x8: { // note off
				uint8_t note = msg.getNote();
					noteData[note].velocity = 0;
					noteData[note].aftertouch = 0;
					noteupdated = true;
				// Remove the note //(vector)
				// auto it = std::find(cachedNotes.begin(), cachedNotes.end(), note);
				// if (it != cachedNotes.end())
				//	cachedNotes.erase(it);
				//noteBuffer.remove(note); //(list)
				}
				break;
			case 0x9: { // note on
				//uint8_t velvalue = msg.getValue();
				uint8_t note = msg.getNote();
					noteData[note].velocity = msg.getValue();
					noteData[note].aftertouch = 0;
					noteupdated = true;
				firstNoGlideLwr = (!anynoteGate && (params[SLEW_LOWER_MODE_PARAM].getValue() > 0.5));
				firstNoGlideUpr = (!anynoteGate  && (params[SLEW_UPPER_MODE_PARAM].getValue() > 0.5));
					sustpedalgate = sustpedal;
				// cachedNotes.push_back(note);
				}
				break;
			case 0xb: // cc
				processCC(msg);
				break;
			case 0xe: // pitch wheel
				pitch = msg.getValue()  * 128 + msg.getNote() ;
				break;
			case 0xd: // channel aftertouch
				pressure = msg.getValue();
				break;
//		  case 0xf: ///realtime clock etc
//			  processSystem(msg);
//			  break;
			default: break;
		}
	}

	void processCC(midi::Message msg) {
		switch (msg.getNote()) {
			case 0x01: // mod
				mod = msg.getValue();
				break;
			case 0x02: // breath
				breath = msg.getValue();
				break;
			case 0x0B: // Expression
				expression = msg.getValue();
				break;
			case 0x40: { // sustain
				sustain = msg.getValue();
				lights[SUSTHOLD_LIGHT].value = (static_cast<float>(sustain)/128.f) * params[SUSTAINHOLD_PARAM].getValue();
				sustpedal = ((params[SUSTAINHOLD_PARAM].getValue() > 0.5) && (msg.getValue() > 63));
				sustpedalgate = anynoteGate && sustpedal;
				}
				break;
			default: break;
		}
	}

	void MidiPanic() {
		pitch = 8192;
		outputs[PBEND_OUTPUT].setVoltage(0.0f);
		mod = 0;
		outputs[MOD_OUTPUT].setVoltage(0.0f);
		breath = 0;
		outputs[BREATH_OUTPUT].setVoltage(0.0f);
		expression = 0;
		outputs[EXPRESSION_OUTPUT].setVoltage(0.0f);
		pressure = 0;
		outputs[PRESSURE_OUTPUT].setVoltage(0.0f);
		sustain = 0;
		outputs[SUSTAIN_OUTPUT].setVoltage(0.0f);
		sustpedal = false;
	}
	
	json_t *dataToJson() override {
		json_t *rootJ = json_object();
		json_object_set_new(rootJ, "midi", midiInput.toJson());
		return rootJ;
	}

	void dataFromJson(json_t *rootJ) override {
		json_t *midiJ = json_object_get(rootJ, "midi");
		midiInput.fromJson(midiJ);
	}
};

////////

struct MIDIdualCVWidget : ModuleWidget {
	
	MIDIdualCVWidget(MIDIdualCV *module){
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/MIDIdualCV.svg")));
		//Screws
		addChild(createWidget<ScrewBlack>(Vec(0, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 15, 0)));
		addChild(createWidget<ScrewBlack>(Vec(0, 365)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 15, 365)));
		
///MIDI
		float xPos = 8.0f;
		float yPos = 19.0f;

		
		MidiWidget *midiWidget = createWidget<MidiWidget>(Vec(xPos,yPos));
		midiWidget->box.size = Vec(119,36);
		midiWidget->setMidiPort(module ? &module->midiInput : NULL);

		midiWidget->driverChoice->box.size.y = 12.f;
		midiWidget->deviceChoice->box.size.y = 12.f;
		midiWidget->channelChoice->box.size.y = 12.f;

		midiWidget->driverChoice->box.pos = Vec(0.f, 0.f);
		midiWidget->deviceChoice->box.pos = Vec(0.f, 12.f);
		midiWidget->channelChoice->box.pos = Vec(0.f, 24.f);

		midiWidget->driverSeparator->box.pos = Vec(0.f, 12.f);
		midiWidget->deviceSeparator->box.pos = Vec(0.f, 24.f);

		midiWidget->driverChoice->textOffset = Vec(2.f,10.f);
		midiWidget->deviceChoice->textOffset = Vec(2.f,10.f);
		midiWidget->channelChoice->textOffset = Vec(2.f,10.f);

		midiWidget->driverChoice->color = nvgRGB(0xdd, 0xdd, 0xdd);
		midiWidget->deviceChoice->color = nvgRGB(0xdd, 0xdd, 0xdd);
		midiWidget->channelChoice->color = nvgRGB(0xdd, 0xdd, 0xdd);
		addChild(midiWidget);
		
//	//reset button
		xPos = 89.f;
		yPos = 43.f;
		addParam(createParam<moDllzMidiPanic>(Vec(xPos, yPos), module, MIDIdualCV::RESETMIDI_PARAM));
		addChild(createLight<SmallLight<RedLight>>(Vec(xPos + 4.f, yPos + 3.5f), module, MIDIdualCV::RESETMIDI_LIGHT));
	
	//Lower-Upper Mods
		
	yPos = 83.f;
		//PitchBend Direct
		addParam(createParam<TTrimSnap>(Vec(11.f,yPos), module, MIDIdualCV::PBNEG_LOWER_PARAM));
		addParam(createParam<TTrimSnap>(Vec(33.f,yPos), module, MIDIdualCV::PBPOS_LOWER_PARAM));
		addParam(createParam<TTrimSnap>(Vec(88.f,yPos), module, MIDIdualCV::PBNEG_UPPER_PARAM));
		addParam(createParam<TTrimSnap>(Vec(110.f,yPos), module, MIDIdualCV::PBPOS_UPPER_PARAM));
	yPos = 108.f;
		//Glide
		addParam(createParam<moDllzKnob22>(Vec(18.f,yPos), module, MIDIdualCV::SLEW_LOWER_PARAM));
		addParam(createParam<moDllzKnob22>(Vec(95.f,yPos), module, MIDIdualCV::SLEW_UPPER_PARAM));
	yPos = 135.f;
		addParam(createParam<moDllzSwitchLedH>(Vec(20.f,yPos), module, MIDIdualCV::SLEW_LOWER_MODE_PARAM));
		addParam(createParam<moDllzSwitchLedH>(Vec(97.f,yPos), module, MIDIdualCV::SLEW_UPPER_MODE_PARAM));

	//Lower-Upper Outputs
	yPos = 150.0f;
		addOutput(createOutput<moDllzPort>(Vec(17.5f, yPos),  module, MIDIdualCV::PITCH_OUTPUT_Lwr));
		addOutput(createOutput<moDllzPort>(Vec(94.5f, yPos),  module, MIDIdualCV::PITCH_OUTPUT_Upr));
	yPos = 177.f;
		addOutput(createOutput<moDllzPort>(Vec(17.5f, yPos),  module, MIDIdualCV::VELOCITY_OUTPUT_Lwr));
		addOutput(createOutput<moDllzPort>(Vec(94.5f, yPos),  module, MIDIdualCV::VELOCITY_OUTPUT_Upr));
	yPos = 204.f;
		addOutput(createOutput<moDllzPort>(Vec(17.5f, yPos),  module, MIDIdualCV::RETRIGGATE_OUTPUT_Lwr));
		addOutput(createOutput<moDllzPort>(Vec(94.5f, yPos),  module, MIDIdualCV::RETRIGGATE_OUTPUT_Upr));
		
	//Retrig Switches
		
	yPos = 243.f;
	addParam(createParam<moDllzSwitchH>(Vec(19.f, yPos), module, MIDIdualCV::LWRRETRGGMODE_PARAM));
	addParam(createParam<moDllzSwitchH>(Vec(96.f, yPos), module, MIDIdualCV::UPRRETRGGMODE_PARAM));
	
	yPos = 240.f;
	//Common Outputs
		addOutput(createOutput<moDllzPort>(Vec(56.f, yPos),  module, MIDIdualCV::GATE_OUTPUT));
	yPos = 286.f;
		addOutput(createOutput<moDllzPort>(Vec(17.f, yPos),  module, MIDIdualCV::PBEND_OUTPUT));
		addOutput(createOutput<moDllzPort>(Vec(44.f, yPos),  module, MIDIdualCV::MOD_OUTPUT));
		addOutput(createOutput<moDllzPort>(Vec(71.f, yPos),  module, MIDIdualCV::BREATH_OUTPUT));
		addOutput(createOutput<moDllzPort>(Vec(98.f, yPos),  module, MIDIdualCV::PRESSURE_OUTPUT));
		
		addOutput(createOutput<moDllzPort>(Vec(17.f, 310.f),  module, MIDIdualCV::PBENDPOS_OUTPUT));
	yPos = 334.f;
		addOutput(createOutput<moDllzPort>(Vec(17.f, yPos),  module, MIDIdualCV::PBENDNEG_OUTPUT));
		addOutput(createOutput<moDllzPort>(Vec(44.f, yPos),  module, MIDIdualCV::EXPRESSION_OUTPUT));
		addOutput(createOutput<moDllzPort>(Vec(71.f, yPos),  module, MIDIdualCV::SUSTAIN_OUTPUT));
	///Sustain hold notes
		addParam(createParam<moDllzSwitchLed>(Vec(104.5f, yPos+4.f), module, MIDIdualCV::SUSTAINHOLD_PARAM));
		addChild(createLight<TranspOffRedLight>(Vec(104.5f, yPos+4.f), module, MIDIdualCV::SUSTHOLD_LIGHT));
	}
};

Model *modelMIDIdualCV = createModel<MIDIdualCV, MIDIdualCVWidget>("MIDIdualCV");

