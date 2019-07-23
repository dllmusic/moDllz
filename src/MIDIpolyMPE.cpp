#include "moDllz.hpp"
/*
 * MIDI8MPE Midi to 16ch CV with MPE and regular Polyphonic modes
 */
struct MIDIpolyMPE : Module {
	enum ParamIds {
		//RESETMIDI_PARAM,
		//LCURSOR_PARAM,
		//RCURSOR_PARAM,
		PLUSONE_PARAM,
		MINUSONE_PARAM,
		LEARNCCA_PARAM,
		LEARNCCB_PARAM,
		LEARNCCC_PARAM,
		LEARNCCD_PARAM,
		LEARNCCE_PARAM,
		LEARNCCF_PARAM,
		LEARNCCG_PARAM,
		LEARNCCH_PARAM,
		SUSTHOLD_PARAM,
		DATAKNOB_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		NUM_INPUTS
	};
	enum OutputIds {
		X_OUTPUT,
		Y_OUTPUT,
		Z_OUTPUT,
		VEL_OUTPUT,
		RVEL_OUTPUT,
		GATE_OUTPUT,
		PBEND_OUTPUT,
		MMA_OUTPUT,
		MMB_OUTPUT,
		MMC_OUTPUT,
		MMD_OUTPUT,
		MME_OUTPUT,
		MMF_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		RESETMIDI_LIGHT,
		ENUMS(CH_LIGHT, 16),
		SUSTHOLD_LIGHT,
		NUM_LIGHTS
	};

	midi::InputQueue midiInput;

	enum PolyMode {
		MPE_MODE,
		MPEPLUS_MODE,
		ROTATE_MODE,
		REUSE_MODE,
		RESET_MODE,
		REASSIGN_MODE,
		UNISON_MODE,
		NUM_MODES
	};
	int polyModeIx = ROTATE_MODE;

	bool holdGates = true;
	struct NoteData {
		uint8_t velocity = 0;
		uint8_t aftertouch = 0;
	};

	NoteData noteData[128];
	
	// cachedNotes : UNISON_MODE and REASSIGN_MODE cache all played notes. The other polyModes cache stolen notes (after the 4th one).
	std::vector<uint8_t> cachedNotes;
	
	std::vector<uint8_t> cachedMPE[16];
	
	
	uint8_t notes[16] = {0};
	uint8_t vels[16] = {0};
	uint8_t rvels[16] = {0};
	int16_t mpex[16] = {0};
	uint16_t mpey[16] = {0};
	uint16_t mpez[16] = {0};
	uint8_t mpeyLB[16] = {0};
	uint8_t mpezLB[16] = {0};
	
	uint8_t mpePlusLB[16] = {0};

	bool gates[16] = {false};
	uint8_t Maft = 0;
	// midi cc/at/pb for displays
	int midiCCs[8] = {};
	
	uint8_t midiCCsVal[8] = {128,1,2,7,10,11,12,64};
	uint16_t Mpit = 8192;
	float xpitch[16] = {0.f};
	
	// gates set to TRUE by pedal and current gate. FALSE by pedal.
	bool pedalgates[16] = {false};
	bool pedal = false;
	int rotateIndex = 0;
	int stealIndex = 0;
	int numVo = 8;
	int pbMainDwn = 12;
	int pbMainUp = 12;
	int pbMPE = 96;
	int dTune = 0;
	int mpeYcc = 74; //cc74 (default MPE Y)
	int mpeZcc = 128; //128 = ChannelAfterTouch (default MPE Z)
	//int MPEmode = 0; // Index of different MPE modes...(User and HakenPlus for now)
	int savedMidiCh = -1;//to reset channel from MPE all channels
	int MPEmasterCh = 0;// 0 ~ 15

	int displayYcc = 74;
	int displayZcc = 128;

	int learnIx = 0;
	int cursorIx = 0;
	int selectedmidich = 0;
	float dataKnob = 0.f;
	int frameData = 0;
	int midiActivity = 0;

	dsp::ExponentialFilter MPExFilter[16];
	dsp::ExponentialFilter MPEyFilter[16];
	dsp::ExponentialFilter MPEzFilter[16];
	dsp::ExponentialFilter MCCsFilter[8];
	dsp::ExponentialFilter MpitFilter;
	
	// retrigger for stolen notes (when gates already open)
	dsp::PulseGenerator reTrigger[16];
	//dsp::SchmittTrigger resetMidiTrigger;

	dsp::SchmittTrigger PlusOneTrigger;
	dsp::SchmittTrigger MinusOneTrigger;
	//dsp::SchmittTrigger LcursorTrigger;
	//dsp::SchmittTrigger RcursorTrigger;


	
	MIDIpolyMPE() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		//configParam(RESETMIDI_PARAM, 0.f, 1.f, 0.f);
		configParam(MINUSONE_PARAM, 0.f, 1.f, 0.f);
		configParam(PLUSONE_PARAM, 0.f, 1.f, 0.f);
		//configParam(LCURSOR_PARAM, 0.f, 1.f, 0.f);
		//configParam(RCURSOR_PARAM, 0.f, 1.f, 0.f);
		configParam(SUSTHOLD_PARAM, 0.f, 1.f, 1.f);
		configParam(DATAKNOB_PARAM, -1.f, 1.f, 0.f);
		onReset();
	}

	json_t *dataToJson() override {
		json_t *rootJ = json_object();
		json_object_set_new(rootJ, "midi", midiInput.toJson());
		json_object_set_new(rootJ, "polyModeIx", json_integer(polyModeIx));
		json_object_set_new(rootJ, "pbMainDwn", json_integer(pbMainDwn));
		json_object_set_new(rootJ, "pbMainUp", json_integer(pbMainUp));
		json_object_set_new(rootJ, "pbMPE", json_integer(pbMPE));
		json_object_set_new(rootJ, "numVo", json_integer(numVo));
		json_object_set_new(rootJ, "MPEmasterCh", json_integer(MPEmasterCh));
		json_object_set_new(rootJ, "midiAcc", json_integer(midiCCs[0]));
		json_object_set_new(rootJ, "midiBcc", json_integer(midiCCs[1]));
		json_object_set_new(rootJ, "midiCcc", json_integer(midiCCs[2]));
		json_object_set_new(rootJ, "midiDcc", json_integer(midiCCs[3]));
		json_object_set_new(rootJ, "midiEcc", json_integer(midiCCs[4]));
		json_object_set_new(rootJ, "midiFcc", json_integer(midiCCs[5]));
		json_object_set_new(rootJ, "midiGcc", json_integer(midiCCs[6]));
		json_object_set_new(rootJ, "midiHcc", json_integer(midiCCs[7]));
		json_object_set_new(rootJ, "mpeYcc", json_integer(mpeYcc));
		json_object_set_new(rootJ, "mpeZcc", json_integer(mpeZcc));
		json_object_set_new(rootJ, "dTune", json_integer(dTune));
		return rootJ;
	}

	void dataFromJson(json_t *rootJ) override {
		json_t *midiJ = json_object_get(rootJ, "midi");
		if (midiJ)
			midiInput.fromJson(midiJ);
		json_t *polyModeIxJ = json_object_get(rootJ, "polyModeIx");
		if (polyModeIxJ)
			polyModeIx = json_integer_value(polyModeIxJ);
		json_t *pbMainDwnJ = json_object_get(rootJ, "pbMainDwn");
		if (pbMainDwnJ)
			pbMainDwn = json_integer_value(pbMainDwnJ);
		json_t *pbMainUpJ = json_object_get(rootJ, "pbMainUp");
		if (pbMainUpJ)
			pbMainUp = json_integer_value(pbMainUpJ);
		json_t *pbMPEJ = json_object_get(rootJ, "pbMPE");
		if (pbMPEJ)
			pbMPE = json_integer_value(pbMPEJ);
		json_t *numVoJ = json_object_get(rootJ, "numVo");
		if (numVoJ)
			numVo = json_integer_value(numVoJ);
		json_t *MPEmasterChJ = json_object_get(rootJ, "MPEmasterCh");
		if (MPEmasterChJ)
			MPEmasterCh = json_integer_value(MPEmasterChJ);
		json_t *midiAccJ = json_object_get(rootJ, "midiAcc");
		if (midiAccJ)
			midiCCs[0] = json_integer_value(midiAccJ);
		json_t *midiBccJ = json_object_get(rootJ, "midiBcc");
		if (midiBccJ)
			midiCCs[1] = json_integer_value(midiBccJ);
		json_t *midiCccJ = json_object_get(rootJ, "midiCcc");
		if (midiCccJ)
			midiCCs[2] = json_integer_value(midiCccJ);
		json_t *midiDccJ = json_object_get(rootJ, "midiDcc");
		if (midiDccJ)
			midiCCs[3] = json_integer_value(midiDccJ);
		json_t *midiEccJ = json_object_get(rootJ, "midiEcc");
		if (midiEccJ)
			midiCCs[4] = json_integer_value(midiEccJ);
		json_t *midiFccJ = json_object_get(rootJ, "midiFcc");
		if (midiFccJ)
			midiCCs[5] = json_integer_value(midiFccJ);
		json_t *midiFccG = json_object_get(rootJ, "midiGcc");
		if (midiFccG)
			midiCCs[6] = json_integer_value(midiFccG);
		json_t *midiFccH = json_object_get(rootJ, "midiHcc");
		if (midiFccH)
			midiCCs[7] = json_integer_value(midiFccH);
		json_t *mpeYccJ = json_object_get(rootJ, "mpeYcc");
		if (mpeYccJ)
			mpeYcc = json_integer_value(mpeYccJ);
		json_t *mpeZccJ = json_object_get(rootJ, "mpeZcc");
		if (mpeZccJ)
			mpeZcc = json_integer_value(mpeZccJ);
		json_t *dTuneJ = json_object_get(rootJ, "dTune");
		if (dTuneJ)
			dTune = json_integer_value(dTuneJ);
		onReset();
	}
///////////////////////////ON RESET
	void onReset() override {
		for (int i = 0; i < 16; i++) {
			notes[i] = 60;
			gates[i] = false;
			pedalgates[i] = false;
			mpey[i] = 0.f;
		}
		rotateIndex = -1;
		cachedNotes.clear();
		float lambdaf = 100.f * APP->engine->getSampleTime();
		
		if (polyModeIx < ROTATE_MODE) {
			midiInput.channel = -1;
			for (int i = 0; i < 16; i++) {
				mpex[i] = 0.f;
				mpez[i] = 0.f;
				cachedMPE[i].clear();
				MPExFilter[i].lambda = lambdaf;
				MPEyFilter[i].lambda = lambdaf;
				MPEzFilter[i].lambda = lambdaf;
			}
			if (polyModeIx > 0){// Haken MPE Plus
				displayYcc = 131;
				displayZcc = 132;
			}else{
				displayYcc = mpeYcc;
				displayZcc = mpeZcc;
			}
		} else {
			displayYcc = 130;
			displayZcc = 129;
		}
		learnIx = 0;
		
		for (int i=0; i < 8; i++){
			MCCsFilter[i].lambda = lambdaf;
		}
		MpitFilter.lambda = lambdaf;
	}

////////////////////////////////////////////////////
	int getPolyIndex(int nowIndex) {
		for (int i = 0; i < numVo; i++) {
			nowIndex++;
			if (nowIndex > (numVo - 1))
				nowIndex = 0;
			if (!(gates[nowIndex] || pedalgates[nowIndex])) {
				stealIndex = nowIndex;
				return nowIndex;
			}
		}
		// All taken = steal (stealIndex always rotates)
		stealIndex++;
		if (stealIndex > (numVo - 1))
			stealIndex = 0;
		///if ((polyMode > MPE_MODE) && (polyMode < REASSIGN_MODE) && (gates[stealIndex]))
		/// cannot reach here if polyMode == MPE mode ...no need to check
		if ((polyModeIx < REASSIGN_MODE) && (gates[stealIndex]))
			cachedNotes.push_back(notes[stealIndex]);
		return stealIndex;
	}

	void pressNote(uint8_t channel, uint8_t note, uint8_t vel) {
		
		// Set notes and gates
		switch (polyModeIx) {
			case MPE_MODE:
			case MPEPLUS_MODE:{
				//////if gate push note to mpe_buffer for legato/////
				if (channel == MPEmasterCh) return;
				//rotateIndex = channel;
				//if ((rotateIndex < 0) || (rotateIndex > 7)) return;
				if (gates[channel]) cachedMPE[channel].push_back(notes[channel]);
			} break;
			case ROTATE_MODE: {
				rotateIndex = getPolyIndex(rotateIndex);
			} break;

			case REUSE_MODE: {
				bool reuse = false;
				for (int i = 0; i < numVo; i++) {
					if (notes[i] == note) {
						rotateIndex = i;
						reuse = true;
						break;
					}
				}
				if (!reuse)
					rotateIndex = getPolyIndex(rotateIndex);
			} break;

			case RESET_MODE: {
				rotateIndex = getPolyIndex(-1);
			} break;

			case REASSIGN_MODE: {
				cachedNotes.push_back(note);
				rotateIndex = getPolyIndex(-1);
			} break;

			case UNISON_MODE: {
				cachedNotes.push_back(note);
				for (int i = 0; i < numVo; i++) {
					notes[i] = note;
					vels[i] = vel;
					gates[i] = true;
					pedalgates[i] = pedal;
					reTrigger[i].trigger(1e-3);
				}
				return;
			} break;

			default: break;
		}
		// Set notes and gates
		if (gates[rotateIndex] || pedalgates[rotateIndex])
			reTrigger[rotateIndex].trigger(1e-3);
		notes[rotateIndex] = note;
		vels[rotateIndex] = vel;
		gates[rotateIndex] = true;
		pedalgates[rotateIndex] = pedal;
	}

	void releaseNote(uint8_t channel, uint8_t note, uint8_t vel) {
		
		if (polyModeIx > MPEPLUS_MODE) {
		// Remove the note
		auto it = std::find(cachedNotes.begin(), cachedNotes.end(), note);
		if (it != cachedNotes.end())
			cachedNotes.erase(it);
		}else{
			if (channel == MPEmasterCh) return;
			auto it = std::find(cachedMPE[channel].begin(), cachedMPE[channel].end(), note);
			if (it != cachedMPE[channel].end())
				cachedMPE[channel].erase(it);
		}

		switch (polyModeIx) {
			case MPE_MODE:
			case MPEPLUS_MODE:{
				if (note == notes[channel]) {
					if (pedalgates[channel]) {
						gates[channel] = false;
					}
					/// check for cachednotes on MPE buffers [8]...
					else if (!cachedMPE[channel].empty()) {
						notes[channel] = cachedMPE[channel].back();
						cachedMPE[channel].pop_back();
					}
					else {
						gates[channel] = false;
					}
					rvels[channel] = vel;
				}
			} break;

			case REASSIGN_MODE: {
				for (int i = 0; i < numVo; i++) {
					if (i < (int) cachedNotes.size()) {
						if (!pedalgates[i])
							notes[i] = cachedNotes[i];
						pedalgates[i] = pedal;
					}
					else {
						gates[i] = false;
						rvels[i] = vel;
					}
				}
			} break;

			case UNISON_MODE: {
				if (vel > 128) vel = 64;
				if (!cachedNotes.empty()) {
					uint8_t backnote = cachedNotes.back();
					for (int i = 0; i < numVo; i++) {
						notes[i] = backnote;
						gates[i] = true;
						rvels[i] = vel;
					}
				}
				else {
					for (int i = 0; i < numVo; i++) {
						gates[i] = false;
						rvels[i] = vel;
					}
				}
				
			} break;

			// default ROTATE_MODE REUSE_MODE RESET_MODE
			default: {
				for (int i = 0; i < numVo; i++) {
					if (notes[i] == note) {
						if (pedalgates[i]) {
							gates[i] = false;
						}
						else if (!cachedNotes.empty()) {
							notes[i] = cachedNotes.back();
							cachedNotes.pop_back();
						}
						else {
							gates[i] = false;
						}
						rvels[i] = vel;
					}
				}
			} break;
		}
	}

	void pressPedal() {
		pedal = true;
		lights[SUSTHOLD_LIGHT].value = params[SUSTHOLD_PARAM].getValue();
		if (polyModeIx == MPE_MODE) {
			for (int i = 0; i < 8; i++) {
				pedalgates[i] = gates[i];
			}
		}else {
			for (int i = 0; i < numVo; i++) {
				pedalgates[i] = gates[i];
			}
		}
	}

	void releasePedal() {
		pedal = false;
		lights[SUSTHOLD_LIGHT].value = 0.f;
		// When pedal is off, recover notes for pressed keys (if any) after they were already being "cycled" out by pedal-sustained notes.
		if (polyModeIx < ROTATE_MODE) {
			for (int i = 0; i < 16; i++) {
				pedalgates[i] = false;
				if (!cachedMPE[i].empty()) {
						notes[i] = cachedMPE[i].back();
						cachedMPE[i].pop_back();
						gates[i] = true;
				}
			}
		}else{
			for (int i = 0; i < numVo; i++) {
				pedalgates[i] = false;
				if (!cachedNotes.empty()) {
					if  (polyModeIx < REASSIGN_MODE){
						notes[i] = cachedNotes.back();
						cachedNotes.pop_back();
						gates[i] = true;
					}
				}
			}
			if (polyModeIx == REASSIGN_MODE) {
				for (int i = 0; i < numVo; i++) {
					if (i < (int) cachedNotes.size()) {
						notes[i] = cachedNotes[i];
						gates[i] = true;
					}
					else {
						gates[i] = false;
					}
				}
			}
		}
	}
	
	void onSampleRateChange() override {
		onReset();
	}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////
//////   STEP START
///////////////////////
	
	
	void process(const ProcessArgs &args) override {

		midi::Message msg;
		while (midiInput.shift(&msg)) {
			processMessage(msg);
		}

		outputs[X_OUTPUT].setChannels(numVo);
		outputs[Y_OUTPUT].setChannels(numVo);
		outputs[Z_OUTPUT].setChannels(numVo);
		outputs[VEL_OUTPUT].setChannels(numVo);
		outputs[RVEL_OUTPUT].setChannels(numVo);
		outputs[GATE_OUTPUT].setChannels(numVo);

		float pbVo = 0.f, pbVoice = 0.f;
		
		if (Mpit < 8192){
			pbVo = MpitFilter.process(1.f ,rescale(Mpit, 0, 8192, -5.f, 0.f));
			pbVoice = -1.f * pbVo * static_cast<float>(pbMainDwn) / 60.f;
		} else {
			pbVo = MpitFilter.process(1.f ,rescale(Mpit, 8192, 16383, 0.f, 5.f));
			pbVoice = pbVo * static_cast<float>(pbMainUp) / 60.f;
		}
		outputs[PBEND_OUTPUT].setVoltage(pbVo);
		bool sustainHold = (params[SUSTHOLD_PARAM].getValue() > .5 );

		if (polyModeIx > MPEPLUS_MODE){
			for (int i = 0; i < numVo; i++) {
				float lastGate = ((gates[i] || (sustainHold && pedalgates[i])) && (!(reTrigger[i].process(args.sampleTime))))? 10.f : 0.f;
				outputs[GATE_OUTPUT].setVoltage(lastGate, i);
				outputs[X_OUTPUT].setVoltage(((notes[i] - 60) / 12.f) + pbVoice, i);
				outputs[VEL_OUTPUT].setVoltage(rescale(vels[i], 0, 127, 0.f, 10.f), i);
				outputs[RVEL_OUTPUT].setVoltage(rescale(rvels[i], 0, 127, 0.f, 10.f), i);
				//outputs[Y_OUTPUT].setVoltage(rescale(mpey[i], 0, 16383, 0.f, 10.f), i);
				outputs[Y_OUTPUT].setVoltage(rescale(mpey[i], 0, 16383, 0.f, 10.f), i);
				outputs[Z_OUTPUT].setVoltage(rescale(noteData[notes[i]].aftertouch, 0, 127, 0.f, 10.f), i);
				lights[CH_LIGHT+ i].value = ((i == rotateIndex)? 0.2f : 0.f) + (lastGate * .08f);
			}
		} else {/// MPE MODE!!!
			for (int i = 0; i < 16; i++) {
				float lastGate = ((gates[i] || (sustainHold && pedalgates[i])) && (!(reTrigger[i].process(args.sampleTime)))) ? 10.f : 0.f;
				outputs[GATE_OUTPUT].setVoltage(lastGate, i);
				if ( mpex[i] < 0){
					xpitch[i] = (MPExFilter[i].process(1.f ,rescale(mpex[i], -8192 , 0, -5.f, 0.f))) * pbMPE / 60.f;
				} else {
					xpitch[i] = (MPExFilter[i].process(1.f ,rescale(mpex[i], 0, 8191, 0.f, 5.f))) * pbMPE / 60.f;
				}
				outputs[X_OUTPUT].setVoltage(xpitch[i] + ((notes[i] - 60) / 12.f) + pbVoice, i);
				outputs[VEL_OUTPUT].setVoltage(rescale(vels[i], 0, 127, 0.f, 10.f));
				outputs[RVEL_OUTPUT].setVoltage(rescale(rvels[i], 0, 127, 0.f, 10.f), i);
				outputs[Y_OUTPUT].setVoltage(MPEyFilter[i].process(1.f ,rescale(mpey[i], 0, 16383, 0.f, 10.f)), i);
				outputs[Z_OUTPUT].setVoltage(MPEzFilter[i].process(1.f ,rescale(mpez[i], 0, 16383, 0.f, 10.f)), i);
				lights[CH_LIGHT + i].value = ((i == rotateIndex)? 0.2f : 0.f) + (lastGate * .08f);
			}
		}
		for (int i = 0; i < 8; i++){
			if (midiCCs[i] == 128)
	//			outputs[MMA_OUTPUT + i].setVoltage(pbVo);
	//		else if (midiCCs[i + 5] == 129)
				outputs[MMA_OUTPUT + i].setVoltage(MCCsFilter[i].process(1.f ,rescale(Maft, 0, 127, 0.f, 10.f)));
			else
				outputs[MMA_OUTPUT + i].setVoltage(MCCsFilter[i].process(1.f ,rescale(midiCCsVal[i], 0, 127, 0.f, 10.f)));
		}

		//// PANEL KNOB AND BUTTONS
		dataKnob = params[DATAKNOB_PARAM].getValue();
		
		if ( dataKnob > 0.07f){
			int knobInterval = static_cast<int>(0.05 * static_cast<float>(args.sampleRate) / dataKnob);
			if (frameData ++ > knobInterval){
				frameData = 0;
				
				dataPlus();
			}
		}else if(dataKnob < -0.07f){
			int knobInterval = static_cast<int>(0.05 * static_cast<float>(args.sampleRate) / -dataKnob);
			if (frameData ++ > knobInterval){
				frameData = 0;
				dataMinus();
			}
		}
		
		if (PlusOneTrigger.process(params[PLUSONE_PARAM].getValue())) {
			dataPlus();
			return;
		}
		if (MinusOneTrigger.process(params[MINUSONE_PARAM].getValue())) {
			dataMinus();
			return;
		}
//		if (LcursorTrigger.process(params[LCURSOR_PARAM].getValue())) {
//			if ((polyModeIx == 1) && (cursorIx == 6)){
//				cursorIx = 3;
//			}else{
//				if (cursorIx > 0) cursorIx --;
//				else cursorIx = 15;
//			}
//			learnIx = 0;
//			return;
//		}
//		if (RcursorTrigger.process(params[RCURSOR_PARAM].getValue())) {
//			if ((polyModeIx == 1) && (cursorIx == 3)){
//				cursorIx = 6;
//			}else{
//				if (cursorIx < 15) cursorIx ++;
//				else cursorIx = 0;
//			}
//			learnIx = 0;
//			return;
//		}
		
		///// RESET MIDI
//		if (resetMidiTrigger.process(params[RESETMIDI_PARAM].getValue())) {
//			lights[RESETMIDI_LIGHT].value= 1.0f;
//			onReset();
//			return;
//		}
//		if (lights[RESETMIDI_LIGHT].value > 0.0001f){
//			lights[RESETMIDI_LIGHT].value -= 0.0001f ; // fade out light
//		}
	}
///////////////////////
//////   STEP END
///////////////////////

	void processMessage(midi::Message msg) {
		switch (msg.getStatus()) {
			// note off
			case 0x8: {
				if ((polyModeIx < ROTATE_MODE) && (msg.getChannel() == MPEmasterCh)) return;
				releaseNote(msg.getChannel(), msg.getNote(), msg.getValue());
			} break;
			// note on
			case 0x9: {
				if ((polyModeIx < ROTATE_MODE) && (msg.getChannel() == MPEmasterCh)) return;
				if (msg.getValue() > 0) {
					//noteData[msg.getNote()].velocity = msg.getValue();
					pressNote(msg.getChannel(), msg.getNote(), msg.getValue());
				}
				else {
					releaseNote(msg.getChannel(), msg.getNote(), 64);//64 is the default rel velocity.
				}
				midiActivity = 128;
			} break;
			// note (poly) aftertouch
			case 0xa: {
				if (polyModeIx < ROTATE_MODE) return;
				noteData[msg.getNote()].aftertouch = msg.getValue();
			} break;
				
			// channel aftertouch
			case 0xd: {
				if (learnIx > 0) {// learn enabled ???
					midiCCs[learnIx - 1] = 129;
					learnIx = 0;
					return;
				}////////////////////////////////////////
				else if (polyModeIx < ROTATE_MODE){
					if (msg.getChannel() == MPEmasterCh){
						Maft = msg.getNote();
					}else if (polyModeIx > 0){
						mpez[msg.getChannel()] =  msg.getNote() * 128 + mpePlusLB[msg.getChannel()];
						mpePlusLB[msg.getChannel()] = 0;
					}else {
						if (mpeZcc == 128)
							mpez[msg.getChannel()] = msg.getNote() * 128;
						if (mpeYcc == 128)
							mpey[msg.getChannel()] = msg.getNote() * 128;
						}
				}else{
					Maft = msg.getNote();
				}
			} break;
				// pitch Bend
			case 0xe:{
				if (learnIx > 0) {// learn enabled ???
					midiCCs[learnIx - 1] = 128;
					learnIx = 0;
					return;
				}////////////////////////////////////////
				else if (polyModeIx < ROTATE_MODE){
					if (msg.getChannel() == MPEmasterCh){
						Mpit = msg.getValue() * 128 + msg.getNote();
					}else{
						mpex[msg.getChannel()] = msg.getValue() * 128 + msg.getNote() - 8192;
					}
				}else{
					Mpit = msg.getValue() * 128 + msg.getNote(); //14bit Pitch Bend
				}
			} break;
			// cc
			case 0xb: {
				///////// LEARN CC   ???
				if (learnIx > 0) {
					midiCCs[learnIx - 1] = msg.getNote();
					learnIx = 0;
					return;
				}else if (polyModeIx < ROTATE_MODE){
					if (msg.getChannel() == MPEmasterCh){
						processCC(msg);
					}else if (polyModeIx == MPEPLUS_MODE){ //Continuum
						if (msg.getNote() == 87){
							mpePlusLB[msg.getChannel()] = msg.getValue();
						}else if (msg.getNote() == 74){
							mpey[msg.getChannel()] =  msg.getValue() * 128 + mpePlusLB[msg.getChannel()];
							mpePlusLB[msg.getChannel()] = 0;
						}

					}else if (msg.getNote() == mpeYcc){
					//cc74 0x4a default
						mpey[msg.getChannel()] = msg.getValue() * 128;
					}else if (msg.getNote() == mpeZcc){
						mpez[msg.getChannel()] = msg.getValue() * 128;
					}
				}else{
					processCC(msg);
				}
			} break;
			default: break;
		}
	}

	void processCC(midi::Message msg) {
		if (msg.getNote() ==  0x40) { //internal sust pedal
			if (msg.getValue() >= 64)
				pressPedal();
			else
				releasePedal();
		}
		for (int i = 0; i < 6; i++){
			if (midiCCs[i] == msg.getNote()){
				midiCCsVal[i] = msg.getValue();
				return;
			}
		}
	}
	void MidiPanic() {
		onReset();
		pedal = false;
		lights[SUSTHOLD_LIGHT].value = 0.f;
		for (int i = 0; i < 8; i++){
			notes[i] = 0;
			vels[i] = 0;
			mpex[i] = 0;
			mpey[i] = 0;
			mpez[i] = 0;
			gates[i] = false;
			xpitch[i] = {0.f};
		}
		for (int i = 0; i < NUM_OUTPUTS; i++) {
			outputs[i].setVoltage(0.f);
		}
	}

	void dataPlus(){
		switch (cursorIx){
			case 0: {
				
			}break;
			case 1: {
				if (polyModeIx < UNISON_MODE) polyModeIx ++;
				else polyModeIx = MPE_MODE;
				onReset();
			}break;
			case 2: {
				if (polyModeIx < ROTATE_MODE) {
					if (pbMPE < 96) pbMPE ++;
				} else {
					if (numVo < 16) numVo ++;
					onReset();
				}
			}break;
			case 3: {
				if (dTune < 99) dTune ++;
			}break;
			case 4: {
				if (mpeYcc <128)
					mpeYcc ++;
				//else
				//	mpeYcc = 0;
				displayYcc = mpeYcc;
			}break;
			case 5: {
				if (mpeZcc <128)
					mpeZcc ++;
				//else
				//	mpeZcc = 0;
				displayZcc = mpeZcc;
			}break;
			case 6: {
				if (pbMainDwn < 96) pbMainDwn ++;
			}break;
			case 7: {
				if (pbMainUp < 96) pbMainUp ++;
			}break;
			default: {
				if (midiCCs[cursorIx - 8] < 128)
					midiCCs[cursorIx - 8]  ++;
				//else
				//	midiCCs[cursorIx - 8] = 0;
			}break;
		}
		learnIx = 0;;
		return;
	}
	void dataMinus(){
		switch (cursorIx){
			case 0: {
				
			}break;
			case 1: {
				if (polyModeIx > MPE_MODE) polyModeIx --;
				else polyModeIx = UNISON_MODE;
				onReset();
			}break;
			case 2: {
				if (polyModeIx < ROTATE_MODE) {
					if (pbMPE > 0) pbMPE --;
				} else {
					if (numVo > 1) numVo --;
					onReset();
				}
			}break;
			case 3: {
				if (dTune > 0) dTune --;
			}break;
			case 4: {
				if (mpeYcc > 0)
					mpeYcc --;
				//else
				//	mpeYcc = 128;
				displayYcc = mpeYcc;
			}break;
			case 5: {
				if (mpeZcc > 0)
					mpeZcc --;
				//else
				//	mpeZcc = 128;
				displayZcc = mpeZcc;
			}break;
			case 6: {
				if (pbMainDwn > -96) pbMainDwn --;
			}break;
			case 7: {
				if (pbMainUp > -96) pbMainUp --;
			}break;
			default: {
				if (midiCCs[cursorIx - 8] > 0)
					midiCCs[cursorIx - 8] --;
				//else
				//	midiCCs[cursorIx - 8] = 128;
			}break;
		}
		learnIx = 0;
		return;
	}
};
// Main Display
struct PolyModeDisplay : TransparentWidget {
	PolyModeDisplay(){
		// font = Font::load(mFONT_FILE);
		font = APP->window->loadFont(mFONT_FILE);
	}
	
	MIDIpolyMPE *module;

	float mdfontSize = 12.f;
	std::string sMode ="";
	std::string sVo ="";
	std::string sPBM ="";
	//std::string sMPEmidiCh = "";
	std::string yyDisplay = "";
	std::string zzDisplay = "";
	std::shared_ptr<Font> font;
	std::string polyModeStr[7] = {
		"M. P. E.",
		"M. P. E. Plus",
		"C Y C L E",
		"R E U S E",
		"R E S E T",
		"R E A S S I G N",
		"U N I S O N",
	};
	int drawFrame = 0;
	int cursorIxI = 0;
	int flashFocus = 0;
	
	void draw(const DrawArgs &args) override {
			int p_polyMode = module->polyModeIx;
			int p_numVo = module->numVo;
			int p_pbMPE = module->pbMPE;
			int p_cursorIx = module->cursorIx;
		
		if (drawFrame ++ > 5){
			drawFrame = 0;
					sMode = polyModeStr[p_polyMode];
					if (p_polyMode < 2){
						sVo =  "channel PBend:" + std::to_string(p_pbMPE);
					}else{
						sVo = "Polyphony "+ std::to_string(p_numVo);
					}
				if (cursorIxI != p_cursorIx){
					cursorIxI = p_cursorIx;
					flashFocus = 64;
				}
			}
			nvgFontSize(args.vg, mdfontSize);
			nvgFontFaceId(args.vg, font->handle);
			nvgFillColor(args.vg, nvgRGB(0xcc, 0xcc, 0xcc));//Text
			//nvgGlobalCompositeOperation(args.vg, NVG_SOURCE_OUT);
			nvgTextAlign(args.vg, NVG_ALIGN_CENTER);
			nvgTextBox(args.vg, 4.f, 11.0f,124.f, sMode.c_str(), NULL);
			nvgTextBox(args.vg, 4.f, 24.f,124.f, sVo.c_str(), NULL);
			nvgGlobalCompositeBlendFunc(args.vg,  NVG_ONE , NVG_ONE);
			nvgBeginPath(args.vg);
			switch (cursorIxI){
				case 0:{
				}break;
				case 1:{ // PolyMode
					nvgRoundedRect(args.vg, 1.f, 1.f, 130.f, 12.f, 3.f);
				}break;
				case 2:{ //numVoices/PB MPE
					nvgRoundedRect(args.vg, 1.f, 14.f, 130.f, 12.f, 3.f);
				}break;
			}
			if (flashFocus > 0)
				flashFocus -= 2;
			int rgbint = 0x55 + flashFocus;
			nvgFillColor(args.vg, nvgRGB(rgbint,rgbint,rgbint)); //SELECTED
			nvgFill(args.vg);
	}
	void onButton(const event::Button &e) override {
		if ((e.button == GLFW_MOUSE_BUTTON_LEFT) && (e.action == GLFW_PRESS)){
			int i = static_cast<int>(e.pos.y / 13.f) + 1;
			if (module->cursorIx == i) module->cursorIx = 0;
			else module->cursorIx = i;
		}
	}
};

struct MidiccDisplay : OpaqueWidget {
	MidiccDisplay(){
	font = APP->window->loadFont(mFONT_FILE);
	}
	MIDIpolyMPE *module;
	float mdfontSize = 12.f;
	std::string sDisplay = "";
	int displayID = 0;
	int ccNumber = -1;
	int pbDwn = -1;
	int pbUp = -1;
	int dTune = -1;
	bool learnOn = false;
	int mymode = 0;
	bool focusOn = false;
	int flashFocus = 0;
	bool canlearn = true;
	bool canedit = true;
	std::shared_ptr<Font> font;
	void draw(const DrawArgs &args) override{
		switch (displayID){
			case 1:{
				if (dTune != module->dTune){
					dTune = module->dTune;
					sDisplay = "dt " + std::to_string(dTune);
				};
				canlearn = false;
			}break;
			case 2:{
				if (ccNumber != module->displayYcc) {
					ccNumber = module->displayYcc;
					mymode = 0;
					displayedCC();
				}
				canedit = (module->polyModeIx == MIDIpolyMPE::MPE_MODE);
				canlearn = false;
			}break;
			case 3:{
				if (ccNumber != module->displayZcc) {
					ccNumber = module->displayZcc;
					mymode = 0;
					displayedCC();
				}
				canedit = (module->polyModeIx == MIDIpolyMPE::MPE_MODE);
				canlearn = false;
			}break;
			case 4:{
				if (pbDwn != module->pbMainDwn){
					pbDwn = module->pbMainDwn;
					sDisplay = std::to_string(pbDwn);
				};
				canlearn = false;
			}break;
			case 5:{
				if (pbUp != module->pbMainUp){
					pbUp = module->pbMainUp;
					sDisplay = std::to_string(pbUp);
				}
				canlearn = false;
			}break;
			default:{
				if (ccNumber !=  module->midiCCs[displayID - 6]) {
					ccNumber =  module->midiCCs[displayID - 6];
					displayedCC();
				}
				canlearn = true;
			}break;
		}
		if (focusOn && (displayID != (module->cursorIx - 2))){
			focusOn = false;
			mymode = 0;
			if (mymode == 2) {
				displayedCC();
				module->learnIx = 0;
			}
		}
		nvgFontSize(args.vg, mdfontSize);
		nvgFontFaceId(args.vg, font->handle);
		nvgTextAlign(args.vg, NVG_ALIGN_CENTER);
		switch(mymode){
			case 0:
				nvgFillColor(args.vg, nvgRGB(0xcc, 0xcc, 0xcc));
				break;
			case 1:{
				//nvgGlobalCompositeBlendFunc(args.vg,  NVG_ONE , NVG_ONE);
				nvgBeginPath(args.vg);
				nvgRoundedRect(args.vg, 0.f, 0.f, box.size.x, box.size.y,3.f);
				nvgStrokeColor(args.vg, nvgRGB(0x66, 0x66, 0x66));
				nvgStroke(args.vg);
				if (flashFocus > 0) flashFocus -= 2;
				nvgGlobalCompositeBlendFunc(args.vg,  NVG_ONE , NVG_ONE);
				int rgbint = 0x55 + flashFocus;
				nvgFillColor(args.vg, nvgRGB(rgbint,rgbint,rgbint)); //SELECTED
				nvgFill(args.vg);
				nvgFillColor(args.vg, nvgRGB(0xcc, 0xcc, 0xcc));
			}break;
			case 2:{
				nvgBeginPath(args.vg);
				nvgRoundedRect(args.vg, 0.f, 0.f, box.size.x, box.size.y,3.f);
				nvgStrokeColor(args.vg, nvgRGB(0xdd, 0x0, 0x0));
				nvgStroke(args.vg);
				nvgFillColor(args.vg, nvgRGBA(0xcc, 0x0, 0x0,0x64));
				nvgFill(args.vg);
				nvgFillColor(args.vg, nvgRGB(0xff, 0x00, 0x00));//LEARN
			}break;
		}
		nvgTextBox(args.vg, 0.f, 10.f,box.size.x, sDisplay.c_str(), NULL);
	}
	void mymodeAction(){
		switch (mymode){
			case 0:{
				focusOn = false;
				if (canlearn) displayedCC();
				module->cursorIx = 0;
				module->learnIx = 0;
			}break;
			case 1:{
				module->cursorIx = displayID + 2;
				//module->learnIx = 0;
				flashFocus = 64;
				focusOn = true;
			}break;
			case 2:{
				sDisplay = "LRN";
				module->learnIx = displayID;
			}break;
		}
	}
	void displayedCC(){
		switch (ccNumber) {
			case 1 :{
				sDisplay = "Mod";
			}break;
			case 2 :{
				sDisplay = "BrC";
			}break;
			case 7 :{
				sDisplay = "Vol";
			}break;
			case 10 :{
				sDisplay = "Pan";
			}break;
			case 11 :{
				sDisplay = "Expr";
			}break;
			case 64 :{
				sDisplay = "Sust";
			}break;
			case 128 :{
				sDisplay = "chAT";
			}break;
			case 129 :{
				sDisplay = "noteAT";
			}break;
			case 130 :{
				sDisplay = "noDet";
			}break;
			case 131 :{//HiRes MPE Y
				sDisplay = "cc74+";
			}break;
			case 132 :{//HiRes MPE Z
				sDisplay = "chAT+";
			}break;
			default :{
				sDisplay = "cc" + std::to_string(ccNumber);
			}
		}
	}
	void onButton(const event::Button &e) override {
		if ((e.button == GLFW_MOUSE_BUTTON_LEFT) && (canedit)){
			//int dispLine = static_cast<int>((e.pos.y - 2.f)/ 13.f) ;
			//bool valpress = (e.pos.x >  box.size.x / 2.f);
			if (e.action == GLFW_RELEASE){
				mymode ++;
				if (mymode > 2) mymode = 0;
				else if ((mymode > 1) && (!canlearn)) mymode = 0;
				mymodeAction();
			};
		}
	}
};

struct springDataKnob : SvgKnob {
	springDataKnob() {
		minAngle = -0.75*M_PI;
		maxAngle = 0.75*M_PI;
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/dataKnobB.svg")));
		shadow->opacity = 0.f;
	}
	void onButton(const event::Button &e) override{
		math::Vec c = box.size.div(2);
		float dist = e.pos.minus(c).norm();
		if (dist <= c.x) {
			ParamWidget::onButton(e);
		}
		if (e.button == GLFW_MOUSE_BUTTON_LEFT && e.action == GLFW_RELEASE) this->resetAction();
	}
};

struct MIDIpolyMPEWidget : ModuleWidget {
	MIDIpolyMPEWidget(MIDIpolyMPE *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance,"res/MIDIpolyMPE.svg")));
		//Screws
		addChild(createWidget<ScrewBlack>(Vec(0, 0)));
		addChild(createWidget<ScrewBlack>(Vec(135, 0)));
		addChild(createWidget<ScrewBlack>(Vec(0, 365)));
		addChild(createWidget<ScrewBlack>(Vec(135, 365)));
		
		float xPos = 9.f;//61;
		float yPos = 20.f;
		int dispID = 1;
		if (module) {
				MIDIscreen *dDisplay = createWidget<MIDIscreen>(Vec(xPos,yPos));
				dDisplay->box.size = {132.f, 40.f};
				dDisplay->setMidiPort (&module->midiInput,&module->polyModeIx,&module->MPEmasterCh,&module->midiActivity);
				addChild(dDisplay);
			xPos = 9.f;
			yPos = 63.f;
			
				PolyModeDisplay *polyModeDisplay = createWidget<PolyModeDisplay>(Vec(xPos,yPos));
				polyModeDisplay->box.size = {132.f, 32.f};
				polyModeDisplay->module = module;
				addChild(polyModeDisplay);
			xPos = 14.f;
			yPos = 150.f;
			
			for ( int i = 0; i < 3; i++){
				MidiccDisplay *MccDisplay = createWidget<MidiccDisplay>(Vec(xPos,yPos));
				MccDisplay->box.size = {36.f, 13.f};
				MccDisplay->displayID =  dispID ++;;
				MccDisplay->module = (module ? module : NULL);
				addChild(MccDisplay);
				xPos += 43.f;
			}
			xPos = 25.5f;
			yPos = 250.f;
			for ( int i = 0; i < 2; i++){
				MidiccDisplay *MccDisplay = createWidget<MidiccDisplay>(Vec(xPos,yPos));
				MccDisplay->box.size = {24.f, 13.f};
				MccDisplay->displayID =  dispID ++;;
				MccDisplay->module = module;
				addChild(MccDisplay);
				xPos += 24.f;
			}
		}

		yPos = 94.f;
		xPos = 13.f;
		for (int i = 0; i < 16; i++){
			addChild(createLight<TinyLight<RedLight>>(Vec(xPos + i * 8.f, yPos), module, MIDIpolyMPE::CH_LIGHT + i));
		}
		xPos = 57.f;
		yPos = 102.f;
		////DATA KNOB
		addParam(createParam<springDataKnob>(Vec(xPos, yPos), module, MIDIpolyMPE::DATAKNOB_PARAM));
		
		///Buttons
		yPos = 108.5f;
		xPos = 24.f;
		addParam(createParam<minusButtonB>(Vec(xPos, yPos), module, MIDIpolyMPE::MINUSONE_PARAM));
		xPos = 103.f;
		addParam(createParam<plusButtonB>(Vec(xPos, yPos), module, MIDIpolyMPE::PLUSONE_PARAM));


//		yPos = 108.f;
//		xPos = 10.f;
//		addParam(createParam<moDllzcursorL>(Vec(xPos, yPos), module, MIDIpolyMPE::LCURSOR_PARAM));
//		xPos = 122.f;
//		addParam(createParam<moDllzcursorR>(Vec(xPos, yPos), module, MIDIpolyMPE::RCURSOR_PARAM));

		yPos = 165.f;
		float xOffset = 42.5f;

		xPos = 21.f;
			addOutput(createOutput<moDllzPortPoly>(Vec(xPos, yPos),  module, MIDIpolyMPE::X_OUTPUT));
			xPos += xOffset;
			addOutput(createOutput<moDllzPortPoly>(Vec(xPos, yPos),  module, MIDIpolyMPE::Y_OUTPUT));
			xPos += xOffset;
			addOutput(createOutput<moDllzPortPoly>(Vec(xPos, yPos),  module, MIDIpolyMPE::Z_OUTPUT));
		
		yPos = 210.f;
		xPos = 27.f;
		xOffset = 33.f;
		
			addOutput(createOutput<moDllzPortPoly>(Vec(xPos, yPos),  module, MIDIpolyMPE::VEL_OUTPUT));
			xPos += xOffset;
			addOutput(createOutput<moDllzPortPoly>(Vec(xPos, yPos),  module, MIDIpolyMPE::RVEL_OUTPUT));
			xPos += xOffset;
			addOutput(createOutput<moDllzPortPoly>(Vec(xPos, yPos),  module, MIDIpolyMPE::GATE_OUTPUT));
			xPos += xOffset;
		///Sustain hold notes
		addParam(createParam<moDllzSwitchLed>(Vec(xPos, yPos + 4.f), module, MIDIpolyMPE::SUSTHOLD_PARAM));
		addChild(createLight<TranspOffRedLight>(Vec(xPos, yPos + 4.f), module, MIDIpolyMPE::SUSTHOLD_LIGHT));
		//}
		yPos = 245.f;
		xPos = 113.f;
		addOutput(createOutput<moDllzPort>(Vec(xPos, yPos),  module, MIDIpolyMPE::PBEND_OUTPUT));
		
		
		yPos = 278.f;
		for ( int r = 0; r < 2; r++){
			xPos = 10.5f;
			for ( int i = 0; i < 4; i++){
				if (module){
					MidiccDisplay *MccDisplay = createWidget<MidiccDisplay>(Vec(xPos,yPos));
					MccDisplay->box.size = {30.f, 13.f};
					MccDisplay->displayID = dispID ++;
					MccDisplay->module = module;
					addChild(MccDisplay);
				}
				addOutput(createOutput<moDllzPort>(Vec(xPos + 1.5f, yPos + 13.f),  module, MIDIpolyMPE::MMA_OUTPUT + i + r * 4));
				xPos += 33.f;
			}
			yPos += 44.f;
		}
	}
};

Model *modelMIDIpolyMPE = createModel<MIDIpolyMPE, MIDIpolyMPEWidget>("MIDIpolyMPE");
