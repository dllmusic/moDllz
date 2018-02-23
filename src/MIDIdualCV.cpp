
#include <list>
#include <algorithm>
#include "rtmidi/RtMidi.h"
#include "core.hpp"
#include "MidiIO.hpp"
#include "dsp/digital.hpp"
#include "moDllz.hpp"
/*
 * MIDIdualCVInterface converts midi note on/off events, velocity , channel aftertouch, pitch wheel and mod wheel to CV

 */
struct MidiValue {
	int val = 0; // Controller value
	TransitionSmoother tSmooth;
	bool changed = false; // Value has been changed by midi message (only if it is in sync!)
};

struct MIDIdualCVInterface : MidiIO, Module {
	enum ParamIds {
		RESET_PARAM,
        LWRRETRGGMODE_PARAM,
        UPRRETRGGMODE_PARAM,
        PBDUALPOLARITY_PARAM,
        SUSTAINHOLD_PARAM,
        NUM_PARAMS
	};
	enum InputIds {
		NUM_INPUTS
	};
	enum OutputIds {
		PITCH_OUTPUT_Lwr = 0,
        PITCH_OUTPUT_Upr,
		VELOCITY_OUTPUT_Lwr,
        VELOCITY_OUTPUT_Upr,
        RETRIGGATE_OUTPUT_Lwr,
        RETRIGGATE_OUTPUT_Upr,
        GATE_OUTPUT,
        PITCHWHEEL_OUTPUT,
        MOD_OUTPUT,
        EXPRESSION_OUTPUT,
        BREATH_OUTPUT,
        SUSTAIN_OUTPUT,
        CHANNEL_AFTERTOUCH_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		RESET_LIGHT,
		NUM_LIGHTS
	};
 
    
    std::list<int> notes;
    bool pedal = false;
	int noteUpr = 60; // C4, most modules should use 261.626 Hz
	int noteLwr = 60; // C4, most modules should use 261.626 Hz
    int nowUpr = -1;
    int nowLwr = 128;
    int velUpr = 0;
    int velLwr = 0;
    bool retriggLwr = false;
    bool retriggUpr = false;
    bool retriggModeLwr = false;
    bool retriggModeUpr = false;
    bool sustainhold = true;
    int vel = 0;
	MidiValue mod;
	MidiValue afterTouch;
	MidiValue pitchWheel;
    MidiValue expression;
    MidiValue breath;
    MidiValue sustain;
	bool gate = false;
    bool pulse = false;

	SchmittTrigger resetTrigger;
    
    PulseGenerator gatePulse;

	MIDIdualCVInterface() : MidiIO(), Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {

    pitchWheel.val = 8192;
	pitchWheel.tSmooth.set(outputs[PITCHWHEEL_OUTPUT].value + 5.0 * (1- params[PBDUALPOLARITY_PARAM].value), pitchWheel.val / 16384.0 * 10.0 );
	}

	~MIDIdualCVInterface() {
	};

	void step() override;

    void pressNote(int note, int vel);

	void releaseNote(int note);

	void processMidi(std::vector<unsigned char> msg);

	json_t *toJson() override {
		json_t *rootJ = json_object();
		addBaseJson(rootJ);
		return rootJ;
	}

	void fromJson(json_t *rootJ) override {
		baseFromJson(rootJ);
	}

	void reset() override {
		resetMidi();
	}

	void resetMidi() override;
};

void MIDIdualCVInterface::resetMidi() {
	mod.val = 0;
	mod.tSmooth.set(0, 0);
	pitchWheel.val = 8192;
    pitchWheel.tSmooth.set(outputs[PITCHWHEEL_OUTPUT].value + 5.0 * (1- params[PBDUALPOLARITY_PARAM].value), pitchWheel.val / 16384.0 * 10.0 );
	afterTouch.val = 0;
	afterTouch.tSmooth.set(0, 0);
    expression.val = 0;
    expression.tSmooth.set(0, 0);
    breath.val = 0;
    breath.tSmooth.set(0, 0);
    sustain.val = 0;
    sustain.tSmooth.set(0, 0);
    pedal = false;
    vel = 0;
    velUpr = 0;
    velLwr = 0;
	gate = false;
    nowUpr = -1;
    nowLwr = 128;
	notes.clear();
}

void MIDIdualCVInterface::step() {
	if (isPortOpen()) {
		std::vector<unsigned char> message;

		// midiIn->getMessage returns empty vector if there are no messages in the queue
		getMessage(&message);
		while (message.size() > 0) {
			processMidi(message);
			getMessage(&message);
		}
	}

    if (resetTrigger.process(params[RESET_PARAM].value)) {
        lights[RESET_LIGHT].value= 1.0;
        resetMidi();
        
		return;
	}
    
    if (lights[RESET_LIGHT].value > 0.0001){
	lights[RESET_LIGHT].value -= 0.0001 ; // fade out light
    }
    outputs[PITCH_OUTPUT_Lwr].value = ((noteLwr - 60)) / 12.0;
    outputs[PITCH_OUTPUT_Upr].value = ((noteUpr - 60)) / 12.0;
    
    pulse = gatePulse.process(1.0 / engineGetSampleRate());
    
    outputs[VELOCITY_OUTPUT_Lwr].value = velLwr / 127.0 * 10.0;
    outputs[VELOCITY_OUTPUT_Upr].value = velUpr / 127.0 * 10.0;
    
    retriggModeLwr = params[LWRRETRGGMODE_PARAM].value > 0.5;
    retriggModeUpr = params[UPRRETRGGMODE_PARAM].value < 0.5;
    
    sustainhold = params[SUSTAINHOLD_PARAM].value > 0.5;
    
    outputs[RETRIGGATE_OUTPUT_Lwr].value = gate && !(retriggLwr && pulse) ? 10.0 : 0.0;
    outputs[RETRIGGATE_OUTPUT_Upr].value = gate && !(retriggUpr && pulse) ? 10.0 : 0.0;
    
    outputs[GATE_OUTPUT].value = gate ? 10.0 : 0.0;
    
    int steps = int(engineGetSampleRate() / 200);///SMOOTHING TO 50 mS
    ///PITCH WHEEL
    if (pitchWheel.changed) {
        //pitchWheel.tSmooth.set(outputs[PITCHWHEEL_OUTPUT].value, (pitchWheel.val - 64) / 64.0 * 10.0, steps); (ORIGINAL PITCH BEND)
        
        //    min = 0     >>-5v or 0v
        // center = 8192  >> 0v or 5v ---- (default)
        //    max = 16383 >> 5v or 10v
        pitchWheel.tSmooth.set(outputs[PITCHWHEEL_OUTPUT].value + 5.0 * (1- params[PBDUALPOLARITY_PARAM].value), pitchWheel.val / 16384.0 * 10.0 , steps);
        pitchWheel.changed = false;
    }
    outputs[PITCHWHEEL_OUTPUT].value = pitchWheel.tSmooth.next() - 5.0 * (1- params[PBDUALPOLARITY_PARAM].value);
    
    ///MODULATION
	if (mod.changed) {
		mod.tSmooth.set(outputs[MOD_OUTPUT].value, (mod.val / 127.0 * 10.0), steps);
		mod.changed = false;
	}
	outputs[MOD_OUTPUT].value = mod.tSmooth.next();

    ///EXPRESSION
    if (expression.changed) {
        expression.tSmooth.set(outputs[EXPRESSION_OUTPUT].value, (expression.val / 127.0 * 10.0), steps);
        expression.changed = false;
    }
    outputs[EXPRESSION_OUTPUT].value = expression.tSmooth.next();
    
    ///BREATH
    if (breath.changed) {
        breath.tSmooth.set(outputs[BREATH_OUTPUT].value, (breath.val / 127.0 * 10.0), steps);
        breath.changed = false;
    }
    outputs[BREATH_OUTPUT].value = breath.tSmooth.next();
    
    ///SUSTAIN (no smoothing)
    outputs[SUSTAIN_OUTPUT].value = sustain.val / 127.0 * 10.0;
    
    ///AFTERTOUCH
    if (afterTouch.changed) {
        afterTouch.tSmooth.set(outputs[CHANNEL_AFTERTOUCH_OUTPUT].value, (afterTouch.val / 127.0 * 10.0), steps);
        afterTouch.changed = false;
    }
    outputs[CHANNEL_AFTERTOUCH_OUTPUT].value = afterTouch.tSmooth.next();
}

//void MIDIdualCVInterface::pressNote( Note_Vel my_note_vel) {
void MIDIdualCVInterface::pressNote(int note, int vel) {
    gate = true;
    
    this->retriggUpr = false ;
    this->retriggLwr = false ;
    
    // Remove existing similar note
    
    
    auto it = std::find(notes.begin(), notes.end(), note);
    if (it != notes.end())
    notes.erase(it);
	
    // Push note
	notes.push_back(note);
    
    if (notes.size() == 1)
    {
        this->noteUpr = note ;
        this->noteLwr = note ;
        this->velUpr = vel ;
        this->velLwr = vel ;
        this->nowUpr = note ;
        this->nowLwr = note ;
        this->retriggUpr = true ;
        this->retriggLwr = true ;
    }
    else
    {
        gatePulse.trigger(1e-3);
        notes.sort();
        ///
        auto it2 = notes.end();
        it2--;
        this->noteUpr = *it2 ;
        if (noteUpr > nowUpr)
        {
            this->nowUpr = noteUpr ;
            this->retriggUpr = true ;
            this->velUpr = vel ;
        }

        auto it3 = notes.begin() ;
        this->noteLwr = *it3 ;
        if (noteLwr < nowLwr)
        {
            this->nowLwr = noteLwr ;
            this->retriggLwr = true ;
            this->velLwr = vel ;
        }
    }
}

void MIDIdualCVInterface::releaseNote(int note) {
    gatePulse.trigger(1e-3);
    this->retriggUpr = false ;
    this->retriggLwr = false ;
        // Remove the note
	auto it = std::find(notes.begin(), notes.end(), note);
	if (it != notes.end())
		notes.erase(it);
	if (pedal) {
		// Don't release if pedal is held
		gate = true;
	} else if (!notes.empty()) {
		auto it2 = notes.end();
		it2--;
        this->noteUpr = *it2 ;
        this->retriggUpr = (noteUpr < nowUpr) && retriggModeUpr ;
        this->nowUpr = noteUpr ;
        auto it3 = notes.begin();
		this->noteLwr = *it3 ;
        this->retriggLwr = (noteLwr > nowLwr) && retriggModeLwr ;
        this->nowLwr = noteLwr ;
		gate = true;
	} else {
		gate = false;
        this->nowUpr = -1;
        this->nowLwr = 128;
        this->velUpr = 0;
        this->velLwr = 0;
	}
}

void MIDIdualCVInterface::processMidi(std::vector<unsigned char> msg) {
	int channel = msg[0] & 0xf;
	int status = (msg[0] >> 4) & 0xf;
	int data1 = msg[1];
	int data2 = msg[2];

	// fprintf(stderr, "channel %d status %d data1 %d data2 %d\n", channel, status, data1, data2);

	// Filter channels
	if (this->channel >= 0 && this->channel != channel)
		return;

	switch (status) {
		// note off
		case 0x8: {
			releaseNote(data1);
		}
			break;
		case 0x9: // note on
			if (data2 > 0) {
                pressNote(data1, data2);
				//this->vel = data2;
			} else {
				// For some reason, some keyboards send a "note on" event with a velocity of 0 to signal that the key has been released.
				releaseNote(data1);
			}
			break;
		case 0xb: // cc
			switch (data1) {
				case 0x01: // mod
					mod.val = data2;
					mod.changed = true;
					break;
                case 0x02: // breath
                    breath.val = data2;
                    breath.changed = true;
                    break;
                case 0x0B: // Expression
                    expression.val = data2;
                    expression.changed = true;
                    break;
                    
				case 0x40: // sustain
                    sustain.val = data2;
                    sustain.changed = true;
                    if (sustainhold){
                    pedal = (data2 >= 64);
                    }else pedal = 0;
                    
                    if (!pedal) releaseNote(-1);
                    
					break;
			}
			break;
		case 0xe: // pitch wheel /// data2 * 128 + data1 -- center 8192 0x2000
			pitchWheel.val = (data2 << 7) + data1;
			pitchWheel.changed = true;
			break;
		case 0xd: // channel aftertouch
			afterTouch.val = data1;
			afterTouch.changed = true;
			break;
	}
}

MIDIdualCVWidget::MIDIdualCVWidget()
{
	MIDIdualCVInterface *module = new MIDIdualCVInterface();
	setModule(module);
	box.size = Vec(15 * 9, 380);
	{
        SVGPanel *panel = new SVGPanel();
        panel->setBackground(SVG::load(assetPlugin(plugin, "res/MIDIdualCV.svg")));
        panel->box.size = box.size;
		addChild(panel);
	}

	float yPos = 21;
    float xPos = 16;
    //Screws
	addChild(createScrew<ScrewBlack>(Vec(0, 0)));
	addChild(createScrew<ScrewBlack>(Vec(box.size.x - 15, 0)));
	addChild(createScrew<ScrewBlack>(Vec(0, 365)));
	addChild(createScrew<ScrewBlack>(Vec(box.size.x - 15, 365)));
    
    
    //Midi Port
    MidiChoice *midiChoice = new MidiChoice();
    midiChoice->midiModule = dynamic_cast<MidiIO *>(module);
    midiChoice->box.pos = Vec(12, 21);
    midiChoice->box.size.x = 118;
    addChild(midiChoice);
    yPos = 44;
    //Midi channel
    ChannelChoice *channelChoice = new ChannelChoice();
    channelChoice->midiModule = dynamic_cast<MidiIO *>(module);
    channelChoice->box.pos = Vec(30, yPos);
    channelChoice->box.size.x = 40;
    addChild(channelChoice);
    //reset button
    addParam(createParam<LEDButton>(Vec(108, yPos), module, MIDIdualCVInterface::RESET_PARAM, 0.0, 1.0, 0.0));
    addChild(createLight<SmallLight<RedLight>>(Vec(114, yPos + 6), module, MIDIdualCVInterface::RESET_LIGHT));
    
    yPos = 89;
    //Ports PJ3410Port PJ301MPort CL1362Port
    //Lower-Upper Outputs
    for (int i = 0; i < 3; i++)
    {
        addOutput(createOutput<moDllzPort>(Vec(19, yPos), module, i * 2));
        addOutput(createOutput<moDllzPort>(Vec(93, yPos), module, i * 2 + 1));
        yPos += 24;
    }
    //Retrig Switches
    addParam(createParam<moDllzSwitchH>(Vec(26, 170), module, MIDIdualCVInterface::LWRRETRGGMODE_PARAM, 0.0, 1.0, 0.0));
    addParam(createParam<moDllzSwitchH>(Vec(90, 170), module, MIDIdualCVInterface::UPRRETRGGMODE_PARAM, 0.0, 1.0, 1.0));
    yPos = 200;
    xPos = 72;
    //Common Outputs
    for (int i = 6; i < MIDIdualCVInterface::NUM_OUTPUTS; i++)
    {
        addOutput(createOutput<moDllzPort>(Vec(xPos, yPos), module, i));
        yPos += 23;
    }
    ///PitchWheel +- or +
    addParam(createParam<moDllzSwitch>(Vec(105, 225), module, MIDIdualCVInterface::PBDUALPOLARITY_PARAM, 0.0, 1.0, 0.0));
    ///Sustain hold notes
    addParam(createParam<moDllzSwitchLed>(Vec(105, 318), module, MIDIdualCVInterface::SUSTAINHOLD_PARAM, 0.0, 1.0, 1.0));
}



void MIDIdualCVWidget::step() {
	ModuleWidget::step();
}
