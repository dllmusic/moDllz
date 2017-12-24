
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
    int vel = 0;
	MidiValue mod;
	MidiValue afterTouch;
	MidiValue pitchWheel;
	bool gate = false;
    bool pulse = false;

	SchmittTrigger resetTrigger;
    
    PulseGenerator gatePulse;

	MIDIdualCVInterface() : MidiIO(), Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {
		pitchWheel.val = 64;
		pitchWheel.tSmooth.set(0, 0);
	}

	~MIDIdualCVInterface() {
	};

	void step() override;

	//void pressNote(Note_Vel my_note_vel);
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
	pitchWheel.val = 64;
	pitchWheel.tSmooth.set(0, 0);
	afterTouch.val = 0;
	afterTouch.tSmooth.set(0, 0);
    vel=0;
	gate = false;
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
		resetMidi();
		return;
	}

	lights[RESET_LIGHT].value -= lights[RESET_LIGHT].value / 0.55 / engineGetSampleRate(); // fade out light
    
    outputs[PITCH_OUTPUT_Lwr].value = ((noteLwr - 60)) / 12.0;
    outputs[PITCH_OUTPUT_Upr].value = ((noteUpr - 60)) / 12.0;
    
    pulse = gatePulse.process(1.0 / engineGetSampleRate());
    
    outputs[VELOCITY_OUTPUT_Lwr].value = velLwr / 127.0 * 10.0;
    outputs[VELOCITY_OUTPUT_Upr].value = velUpr / 127.0 * 10.0;
    
    retriggModeLwr = params[LWRRETRGGMODE_PARAM].value > 0.5;
    retriggModeUpr = params[UPRRETRGGMODE_PARAM].value > 0.5;
    
    outputs[RETRIGGATE_OUTPUT_Lwr].value = gate && !(retriggLwr && pulse) ? 10.0 : 0.0;
    outputs[RETRIGGATE_OUTPUT_Upr].value = gate && !(retriggUpr && pulse) ? 10.0 : 0.0;
    
    outputs[GATE_OUTPUT].value = gate ? 10.0 : 0.0;
    
    int steps = int(engineGetSampleRate() / 32);

	if (mod.changed) {
		mod.tSmooth.set(outputs[MOD_OUTPUT].value, (mod.val / 127.0 * 10.0), steps);
		mod.changed = false;
	}
	outputs[MOD_OUTPUT].value = mod.tSmooth.next();

	if (pitchWheel.changed) {
		pitchWheel.tSmooth.set(outputs[PITCHWHEEL_OUTPUT].value, (pitchWheel.val - 64) / 64.0 * 10.0, steps);
		pitchWheel.changed = false;
	}
	outputs[PITCHWHEEL_OUTPUT].value = pitchWheel.tSmooth.next();


	/* NOTE: I'll leave out value smoothing for after touch for now. I currently don't
	 * have an after touch capable device around and I assume it would require different
	 * smoothing*/
	outputs[CHANNEL_AFTERTOUCH_OUTPUT].value = afterTouch.val / 127.0 * 10.0;
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
				case 0x40: // sustain
					pedal = (data2 >= 64);
					if (!pedal) {
						releaseNote(-1);
					}
					break;
			}
			break;
		case 0xe: // pitch wheel
			pitchWheel.val = data2;
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
		//Panel *panel = new LightPanel();//
          SVGPanel *panel = new SVGPanel();
          panel->setBackground(SVG::load(assetPlugin(plugin, "res/MIDIdualCV.svg")));
        //
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
    midiChoice->box.pos = Vec(xPos, yPos);
    midiChoice->box.size.x = box.size.x - 26;
    addChild(midiChoice);
    yPos = 47;
    //Midi channel
    ChannelChoice *channelChoice = new ChannelChoice();
    channelChoice->midiModule = dynamic_cast<MidiIO *>(module);
    channelChoice->box.pos = Vec(xPos+14, yPos);
    channelChoice->box.size.x = 40;
    addChild(channelChoice);
    //reset button
    addParam(createParam<LEDButton>(Vec(110, yPos), module, MIDIdualCVInterface::RESET_PARAM, 0.0, 1.0, 0.0));
    addChild(createLight<SmallLight<RedLight>>(Vec(115, yPos + 5), module, MIDIdualCVInterface::RESET_LIGHT));
    yPos = 98;
    
    //Ports PJ3410Port PJ301MPort CL1362Port
    //Lower-Upper Outputs
    for (int i = 0; i < 3; i++)
    {
        addOutput(createOutput<PJ301MPort>(Vec(19, yPos), module, i * 2));
        addOutput(createOutput<PJ301MPort>(Vec(93, yPos), module, i * 2 + 1));
        yPos += 34;
    }
    //Retrig Switches
    addParam(createParam<moDLLzSwitch>(Vec(24, yPos), module, MIDIdualCVInterface::LWRRETRGGMODE_PARAM, 0.0, 1.0, 1.0));
    addParam(createParam<moDLLzSwitch>(Vec(98, yPos), module, MIDIdualCVInterface::UPRRETRGGMODE_PARAM, 0.0, 1.0, 1.0));
    yPos += 34;
    xPos = 88;
    //Common Outputs
    for (int i = 0; i < 4; i++)
    {
        addOutput(createOutput<PJ301MPort>(Vec(xPos, yPos), module, i + 6));
        yPos += 34;
    }
}

void MIDIdualCVWidget::step() {
	ModuleWidget::step();
}
