#include "moDllz.hpp"
/*
 * MIDI8MPE Midi to 16ch CV with MPE and regular Polyphonic modes
 */
struct MIDIpolyMPE : Module {
	enum ParamIds {
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
	// midi cc/at/pb
	int midiCCs[8] = {};
	
	uint8_t midiCCsVal[8] = {128,1,2,7,10,11,12,64};
	int16_t Mpit = 0;
	float xpitch[16] = {0.f};
	float drift[16] = {0.f};
	// gates set to TRUE by pedal and current gate. FALSE by pedal.
	bool pedalgates[16] = {false};
	bool pedal = false;
	int rotateIndex = 0;
	int stealIndex = 0;
	int numVo = 8;
	int pbMainDwn = -12;
	int pbMainUp = 2;
	int pbMPE = 96;
	bool mpePbOut = true;
	int dTune = 0;
	int noteMin = 0;
	int noteMax = 127;
	int velMin = 1;
	int velMax = 127;
	int mpeYcc = 74; //cc74 (default MPE Y)
	int mpeZcc = 128; //128 = ChannelAfterTouch (default MPE Z)
	int savedMidiCh = -1;//to reset channel from MPE all channels
	int MPEmasterCh = 0;// 0 ~ 15
	int displayYcc = 74;
	int displayZcc = 128;
	int learnIx = 0;
	int cursorIx = 0;
	int const numPolycur = 6;
	int selectedmidich = 0;
	float dataKnob = 0.f;
	int frameData = 0;
	int midiActivity = 0;
	
	dsp::ExponentialFilter MPExFilter[16];
	dsp::ExponentialFilter MPEyFilter[16];
	dsp::ExponentialFilter MPEzFilter[16];
	dsp::ExponentialFilter MCCsFilter[8];
	dsp::ExponentialFilter MpitFilter;
	dsp::PulseGenerator reTrigger[16];	// retrigger for stolen notes
	dsp::SchmittTrigger PlusOneTrigger;
	dsp::SchmittTrigger MinusOneTrigger;

	MIDIpolyMPE() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(MINUSONE_PARAM, 0.f, 1.f, 0.f);
		configParam(PLUSONE_PARAM, 0.f, 1.f, 0.f);
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
		json_object_set_new(rootJ, "mpePbOut", json_integer(mpePbOut ? 1 : 0));
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
		json_object_set_new(rootJ, "noteMin", json_integer(noteMin));
		json_object_set_new(rootJ, "noteMax", json_integer(noteMax));
		json_object_set_new(rootJ, "velMin", json_integer(velMin));
		json_object_set_new(rootJ, "velMax", json_integer(velMax));
		return rootJ;
	}
	int mdriverJx = 0 , mchannelJx = 0;
	std::string mdeviceJx = "-";
	void dataFromJson(json_t *rootJ) override {
		json_t *midiJ = json_object_get(rootJ, "midi");
		if (midiJ)	{
			json_t* driverJ = json_object_get(midiJ, "driver");
			if (driverJ) mdriverJx = json_integer_value(driverJ);
			json_t* deviceNameJ = json_object_get(midiJ, "deviceName");
			if (deviceNameJ) mdeviceJx = json_string_value(deviceNameJ);
			json_t* channelJ = json_object_get(midiJ, "channel");
			if (channelJ) mchannelJx = json_integer_value(channelJ);
			midiInput.fromJson(midiJ);
		}
		json_t *polyModeIxJ = json_object_get(rootJ, "polyModeIx");
		if (polyModeIxJ) polyModeIx = json_integer_value(polyModeIxJ);
		json_t *pbMainDwnJ = json_object_get(rootJ, "pbMainDwn");
		if (pbMainDwnJ) pbMainDwn = json_integer_value(pbMainDwnJ);
		json_t *pbMainUpJ = json_object_get(rootJ, "pbMainUp");
		if (pbMainUpJ) pbMainUp = json_integer_value(pbMainUpJ);
		json_t *pbMPEJ = json_object_get(rootJ, "pbMPE");
		if (pbMPEJ) pbMPE = json_integer_value(pbMPEJ);
		json_t *mpePbOutJ = json_object_get(rootJ, "mpePbOut");
		if (mpePbOutJ) mpePbOut = static_cast<bool>(json_integer_value(mpePbOutJ));
		json_t *numVoJ = json_object_get(rootJ, "numVo");
		if (numVoJ) numVo = json_integer_value(numVoJ);
		json_t *MPEmasterChJ = json_object_get(rootJ, "MPEmasterCh");
		if (MPEmasterChJ) MPEmasterCh = json_integer_value(MPEmasterChJ);
		json_t *midiAccJ = json_object_get(rootJ, "midiAcc");
		if (midiAccJ) midiCCs[0] = json_integer_value(midiAccJ);
		json_t *midiBccJ = json_object_get(rootJ, "midiBcc");
		if (midiBccJ) midiCCs[1] = json_integer_value(midiBccJ);
		json_t *midiCccJ = json_object_get(rootJ, "midiCcc");
		if (midiCccJ) midiCCs[2] = json_integer_value(midiCccJ);
		json_t *midiDccJ = json_object_get(rootJ, "midiDcc");
		if (midiDccJ) midiCCs[3] = json_integer_value(midiDccJ);
		json_t *midiEccJ = json_object_get(rootJ, "midiEcc");
		if (midiEccJ) midiCCs[4] = json_integer_value(midiEccJ);
		json_t *midiFccJ = json_object_get(rootJ, "midiFcc");
		if (midiFccJ) midiCCs[5] = json_integer_value(midiFccJ);
		json_t *midiFccG = json_object_get(rootJ, "midiGcc");
		if (midiFccG) midiCCs[6] = json_integer_value(midiFccG);
		json_t *midiFccH = json_object_get(rootJ, "midiHcc");
		if (midiFccH) midiCCs[7] = json_integer_value(midiFccH);
		json_t *mpeYccJ = json_object_get(rootJ, "mpeYcc");
		if (mpeYccJ) mpeYcc = json_integer_value(mpeYccJ);
		json_t *mpeZccJ = json_object_get(rootJ, "mpeZcc");
		if (mpeZccJ) mpeZcc = json_integer_value(mpeZccJ);
		json_t *dTuneJ = json_object_get(rootJ, "dTune");
		if (dTuneJ) dTune = json_integer_value(dTuneJ);
		json_t *noteMinJ = json_object_get(rootJ, "noteMin");
		if (noteMinJ) noteMin = json_integer_value(noteMinJ);
		json_t *noteMaxJ = json_object_get(rootJ, "noteMax");
		if (noteMaxJ) noteMax = json_integer_value(noteMaxJ);
		json_t *velMinJ = json_object_get(rootJ, "velMin");
		if (velMinJ) velMin = json_integer_value(velMinJ);
		json_t *velMaxJ = json_object_get(rootJ, "velMax");
		if (velMaxJ) velMax = json_integer_value(velMaxJ);
		onReset();
	}

///////////////////////////ON RESET
	void onReset() override {
		float lambdaf = 100.f * APP->engine->getSampleTime();
		pedal = false;
		lights[SUSTHOLD_LIGHT].value = 0.f;
		midiActivity = 64;
		for (int i = 0; i < 16; i++) {
			notes[i] = 60;
			gates[i] = false;
			pedalgates[i] = false;
			mpey[i] = 0;
			vels[i] = 0;
			rvels[i] = 0;
			xpitch[i] = {0.f};
			mpex[i] = 0;
			mpez[i] = 0;
			cachedMPE[i].clear();
			MPExFilter[i].lambda = lambdaf;
			MPEyFilter[i].lambda = lambdaf;
			MPEzFilter[i].lambda = lambdaf;
			mpeyLB[i] = 0;
			mpezLB[i] = 0;
			mpePlusLB[i] = 0;
		}
		rotateIndex = -1;
		cachedNotes.clear();
		if (polyModeIx < ROTATE_MODE) {
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
	
	void onRandomize() override{};
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
		// All taken = steal (rotates)
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
		if (note < noteMin) return;
		if (note > noteMax) return;
		if (vel < velMin) return;
		if (vel > velMax) return;
		// Set notes and gates
		switch (polyModeIx) {
			case MPE_MODE:
			case MPEPLUS_MODE:{
				if (channel == MPEmasterCh) return;
				rotateIndex = channel;
				//////if gate push note to mpe_buffer for legato/////
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
					drift[i] = static_cast<float>((rand() % dTune  - dTune/2)) / 1200.f;
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
		drift[rotateIndex] = static_cast<float>((rand() % 100  - 50) * dTune) * 0.00004f;
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
					/// check for cachednotes on MPE buffers...
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
		if (Mpit < 0){
			pbVo = MpitFilter.process(1.f ,rescale(Mpit, -8192, 0, -5.f, 0.f));
			pbVoice = -1.f * pbVo * pbMainDwn / 60.f;
		} else {
			pbVo = MpitFilter.process(1.f ,rescale(Mpit, 0, 8191, 0.f, 5.f));
			pbVoice = pbVo * pbMainUp / 60.f;
		}
		outputs[PBEND_OUTPUT].setVoltage(pbVo);
		bool sustainHold = (params[SUSTHOLD_PARAM].getValue() > .5 );

		if (polyModeIx > MPEPLUS_MODE){
			for (int i = 0; i < numVo; i++) {
				float lastGate = ((gates[i] || (sustainHold && pedalgates[i])) && (!(reTrigger[i].process(args.sampleTime))))? 10.f : 0.f;
				outputs[GATE_OUTPUT].setVoltage(lastGate, i);
				float thispitch = ((notes[i] - 60) / 12.f) + pbVoice;
				outputs[X_OUTPUT].setVoltage(thispitch, i);
				outputs[Y_OUTPUT].setVoltage(thispitch + drift[i] , i);	//drifted out
				outputs[VEL_OUTPUT].setVoltage(rescale(vels[i], 0, 127, 0.f, 10.f), i);
				outputs[RVEL_OUTPUT].setVoltage(rescale(rvels[i], 0, 127, 0.f, 10.f), i);
				outputs[Z_OUTPUT].setVoltage(rescale(noteData[notes[i]].aftertouch, 0, 127, 0.f, 10.f), i);
				lights[CH_LIGHT+ i].value = ((i == rotateIndex)? 0.2f : 0.f) + (lastGate * .08f);
			}
		} else {/// MPE MODE!!!
			for (int i = 0; i < 16; i++) {
				float lastGate = ((gates[i] || (sustainHold && pedalgates[i])) && (!(reTrigger[i].process(args.sampleTime))))? 10.f : 0.f;
				outputs[GATE_OUTPUT].setVoltage(lastGate, i);
				if (mpex[i] < 0) xpitch[i] = (MPExFilter[i].process(1.f ,rescale(mpex[i], -8192, 0, -5.f, 0.f)));
				else xpitch[i] = (MPExFilter[i].process(1.f ,rescale(mpex[i], 0, 8191, 0.f, 5.f)));
				outputs[X_OUTPUT].setVoltage(xpitch[i]  * pbMPE / 60.f + ((notes[i] - 60) / 12.f) + pbVoice, i);
				outputs[VEL_OUTPUT].setVoltage(rescale(vels[i], 0, 127, 0.f, 10.f), i);
				if (mpePbOut || (polyModeIx > MPE_MODE)) outputs[RVEL_OUTPUT].setVoltage(xpitch[i], i);
				else outputs[RVEL_OUTPUT].setVoltage(rescale(rvels[i], 0, 127, 0.f, 10.f), i);
				outputs[Y_OUTPUT].setVoltage(MPEyFilter[i].process(1.f ,rescale(mpey[i], 0, 16383, 0.f, 10.f)), i);
				outputs[Z_OUTPUT].setVoltage(MPEzFilter[i].process(1.f ,rescale(mpez[i], 0, 16383, 0.f, 10.f)), i);
				lights[CH_LIGHT + i].value = ((i == rotateIndex)? 0.2f : 0.f) + (lastGate * .08f);
			}
		}
		for (int i = 0; i < 8; i++){
			if (midiCCs[i] == 128)
				outputs[MMA_OUTPUT + i].setVoltage(MCCsFilter[i].process(1.f ,rescale(Maft, 0, 127, 0.f, 10.f)));
			else
				outputs[MMA_OUTPUT + i].setVoltage(MCCsFilter[i].process(1.f ,rescale(midiCCsVal[i], 0, 127, 0.f, 10.f)));
		}
		//// PANEL KNOB AND BUTTONS
		dataKnob = params[DATAKNOB_PARAM].getValue();
		if ( dataKnob > 0.07f){
			int knobInterval = static_cast<int>(0.05 * args.sampleRate / dataKnob);
			if (frameData ++ > knobInterval){
				frameData = 0;
				dataPlus();
			}
		}else if(dataKnob < -0.07f){
			int knobInterval = static_cast<int>(0.05 * args.sampleRate / -dataKnob);
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
		if (midiActivity < 0) onReset();
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
						Mpit = msg.getValue() * 128 + msg.getNote()  - 8192;
					}else{
						mpex[msg.getChannel()] = msg.getValue() * 128 + msg.getNote()  - 8192;
					}
				}else{
					Mpit = msg.getValue() * 128 + msg.getNote() - 8192; //14bit Pitch Bend
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
				if (noteMin < noteMax) noteMin ++;
			}break;
			case 4: {
				if (noteMax < 127) noteMax ++;
			}break;
			case 5: {
				if (velMin < velMax) velMin ++;
			}break;
			case 6: {
				if (velMax < 127) velMax ++;
			}break;
			case 7: {
				if (polyModeIx == MPE_MODE) {
					if (mpeYcc <128) mpeYcc ++;
					displayYcc = mpeYcc;
				}else if (dTune < 99) dTune ++;
			}break;
			case 8: {
				if (mpeZcc <128) mpeZcc ++;
				displayZcc = mpeZcc;
			}break;
			case 9: {
				mpePbOut = !mpePbOut ;
			}break;
			case 10: {
				if (pbMainDwn < 96) pbMainDwn ++;
			}break;
			case 11: {
				if (pbMainUp < 96) pbMainUp ++;
			}break;
			default: {
				if (midiCCs[cursorIx - numPolycur - 5] < 128)
					midiCCs[cursorIx - numPolycur - 5]  ++;
			}break;
		}
		learnIx = 0;
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
				if (noteMin > 0) noteMin --;
			}break;
			case 4: {
				if (noteMax > noteMin) noteMax --;
			}break;
			case 5: {
				if (velMin > 1) velMin --;
			}break;
			case 6: {
				if (velMax > velMin) velMax --;
			}break;
			case 7: {
				if (polyModeIx == MPE_MODE) {
					if (mpeYcc > 0) mpeYcc --;
					displayYcc = mpeYcc;
				}else if (dTune > 0) dTune --;
			}break;
			case 8: {
				if (mpeZcc > 0) mpeZcc --;
				displayZcc = mpeZcc;
			}break;
			case 9: {
				mpePbOut = !mpePbOut ;
			}break;
			case 10: {
				if (pbMainDwn > -96) pbMainDwn --;
			}break;
			case 11: {
				if (pbMainUp > -96) pbMainUp --;
			}break;
			default: {
				if (midiCCs[cursorIx - numPolycur - 5] > 0)
					midiCCs[cursorIx - numPolycur - 5] --;
			}break;
		}
		learnIx = 0;
		return;
	}
};
// Main Display
struct PolyModeDisplay : TransparentWidget {
	PolyModeDisplay(){
		font = APP->window->loadFont(mFONT_FILE);
	}
	MIDIpolyMPE *module;
	float mdfontSize = 12.f;
	std::string sMode = "";
	std::string sVo = "";
	std::string sPBM = "";
	std::string snoteMin = "";
	std::string snoteMax = "";
	std::string svelMin = "";
	std::string svelMax = "";
	std::string notes = "N:";
	std::string vels = "V:";
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
	std::string noteName[12] = {
		"C","C#","D","Eb","E","F","F#","G","Ab","A","Bb","B"
	};
	int drawFrame = 0;
	int cursorIxI = 0;
	int flashFocus = 0;

	
	void draw(const DrawArgs &args) override {
		int *p_cursorIx = &module->cursorIx;
		if (drawFrame ++ > 5){
			drawFrame = 0;
			sMode = polyModeStr[module->polyModeIx];
			if (module->polyModeIx < 2){
				sVo =  "channel PBend:" + std::to_string(module->pbMPE);
			}else{
				sVo = "Polyphony "+ std::to_string(module->numVo);
			}
			snoteMin = noteName[module->noteMin % 12] + std::to_string((module->noteMin / 12) - 2);
			snoteMax = noteName[module->noteMax % 12] + std::to_string((module->noteMax / 12) - 2);
			svelMin = std::to_string(module->velMin);
			svelMax = std::to_string(module->velMax);
			if (cursorIxI != *p_cursorIx){
				cursorIxI = *p_cursorIx;
				flashFocus = 64;
			}
		}
		nvgFontSize(args.vg, mdfontSize);
		nvgFontFaceId(args.vg, font->handle);
		nvgFillColor(args.vg, nvgRGB(0xcc, 0xcc, 0xcc));//Text
		nvgTextAlign(args.vg, NVG_ALIGN_CENTER);
		nvgTextBox(args.vg, 1.f, 11.0f,134.f, sMode.c_str(), NULL);
		nvgTextBox(args.vg, 1.f, 24.f,134.f, sVo.c_str(), NULL);
		nvgTextBox(args.vg, 2.f, 37.f,12.f, "N:", NULL);
		nvgTextBox(args.vg, 14.f, 37.f,30.f, snoteMin.c_str(), NULL);
		nvgTextBox(args.vg, 44.f, 37.f,30.f, snoteMax.c_str(), NULL);
		nvgTextBox(args.vg, 78.f, 37.f,12.f, "V:", NULL);
		nvgTextBox(args.vg, 90.f, 37.f,22.f, svelMin.c_str(), NULL);
		nvgTextBox(args.vg, 112.f, 37.f,22.f, svelMax.c_str(), NULL);
		nvgGlobalCompositeBlendFunc(args.vg,  NVG_ONE , NVG_ONE);
		nvgBeginPath(args.vg);
		switch (cursorIxI){
			case 0:{
			}break;
			case 1:{ // PolyMode
				nvgRoundedRect(args.vg, 1.f, 1.f, 134.f, 12.f, 3.f);
			}break;
			case 2:{ //numVoices/PB MPE
				nvgRoundedRect(args.vg, 1.f, 14.f, 134.f, 12.f, 3.f);
			}break;
			case 3:{ //minNote
				nvgRoundedRect(args.vg, 15.f, 28.f, 28.f, 12.f, 3.f);
			}break;
			case 4:{ //maxNote
				nvgRoundedRect(args.vg, 45.f, 28.f, 28.f, 12.f, 3.f);
			}break;
			case 5:{ //minVel
				nvgRoundedRect(args.vg, 91.f, 28.f, 20.f, 12.f, 3.f);
			}break;
			case 6:{ //maxVel
				nvgRoundedRect(args.vg, 113.f, 28.f, 20.f, 12.f, 3.f);
			}break;
		}
		if (flashFocus > 0)	flashFocus -= 2;
		int rgbint = 0x55 + flashFocus;
		nvgFillColor(args.vg, nvgRGB(rgbint,rgbint,rgbint)); //SELECTED
		nvgFill(args.vg);
	}
	void onButton(const event::Button &e) override {
		if ((e.button == GLFW_MOUSE_BUTTON_LEFT) && (e.action == GLFW_PRESS)){
			int i = static_cast<int>(e.pos.y / 13.f) + 1;
			if (i > 2) i += static_cast<int>(e.pos.x /34.f);
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
	bool refreshccy = true;
	int polychanged = -1;
	std::shared_ptr<Font> font;
	void draw(const DrawArgs &args) override{
		switch (displayID){
			case 1:{
				if (module->polyModeIx < MIDIpolyMPE::ROTATE_MODE) {
					if ((ccNumber != module->displayYcc) || (polychanged != module->polyModeIx)) {
						ccNumber = module->displayYcc;
						//mymode = 0;
						polychanged = module->polyModeIx;
						displayedCC();
					}
					canedit = (module->polyModeIx == MIDIpolyMPE::MPE_MODE);
				}else{
					if ((dTune != module->dTune) || (polychanged != module->polyModeIx)) {
						dTune = module->dTune;
						polychanged = module->polyModeIx;
						sDisplay = "dft " + std::to_string(module->dTune);
					}
					canedit = true;
				}
				canlearn = false;
			}break;
			case 2:{
				if (ccNumber != module->displayZcc) {
					ccNumber = module->displayZcc;
					//mymode = 0;
					displayedCC();
				}
				canedit = (module->polyModeIx == MIDIpolyMPE::MPE_MODE);
				canlearn = false;
			}break;
			case 3:{
				switch (module->polyModeIx) {
					case MIDIpolyMPE::MPE_MODE: {
						if (module->mpePbOut) sDisplay = "chnPB";
						else sDisplay = "RlVel";
						canedit = true;
					}break;
					case MIDIpolyMPE::MPEPLUS_MODE: {
						sDisplay = "chnPB";
						canedit = false;
					}break;
					default: {
						sDisplay = "RlVel";
						canedit = false;
					}break;
				}
				canlearn = false;
			}break;
			case 4:{
				if (pbDwn != module->pbMainDwn){
					pbDwn = module->pbMainDwn;
					if (pbDwn > -1)
						sDisplay = "+" + std::to_string(pbDwn);
					else
						sDisplay = std::to_string(pbDwn);
				};
				canlearn = false;
			}break;
			case 5:{
				if (pbUp != module->pbMainUp){
					pbUp = module->pbMainUp;
					if (pbUp > -1)
						sDisplay = "+" + std::to_string(pbUp);
					else
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
				if ((mymode==2) && (module->learnIx == 0)) {
					mymode = 0;
					displayedCC();
				}
			}break;
		}
		if (focusOn && (displayID != (module->cursorIx - module->numPolycur))){
			focusOn = false;
			if (mymode == 2) {
				displayedCC();
				module->learnIx = 0;
			}
			mymode = 0;
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
		//sDisplay = std::to_string(displayID);
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
				module->cursorIx = displayID + module->numPolycur;
				//module->learnIx = 0;
				flashFocus = 64;
				focusOn = true;
			}break;
			case 2:{
				sDisplay = "LRN";
				module->learnIx = displayID - 5;
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
				sDisplay = "Dtn";
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

struct springDataKnobB : SvgKnob {
	springDataKnobB() {
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
		
		float xPos = 7.f;
		float yPos = 19.f;
		int dispID = 1;
		if (module) {
			//MIDI
				MIDIscreen *dDisplay = createWidget<MIDIscreen>(Vec(xPos,yPos));
				dDisplay->box.size = {136.f, 40.f};
				dDisplay->setMidiPort (&module->midiInput, &module->polyModeIx, &module->MPEmasterCh, &module->midiActivity, &module->mdriverJx, &module->mdeviceJx, &module->mchannelJx);
				addChild(dDisplay);
			//PolyModes LCD
			xPos = 7.f;
			yPos = 61.f;
				PolyModeDisplay *polyModeDisplay = createWidget<PolyModeDisplay>(Vec(xPos,yPos));
				polyModeDisplay->box.size = {136.f, 40.f};
				polyModeDisplay->module = module;
				addChild(polyModeDisplay);
			//  Y Z LCDs
			xPos = 55.f;
			yPos = 156.f;
			for ( int i = 0; i < 2; i++){
				MidiccDisplay *MccDisplay = createWidget<MidiccDisplay>(Vec(xPos,yPos));
				MccDisplay->box.size = {40.f, 13.f};
				MccDisplay->displayID =  dispID ++;;
				MccDisplay->module = (module ? module : NULL);
				addChild(MccDisplay);
				xPos += 44.f;
			}
			// RelVel / chPbend LCD
			xPos = 19.f;
			yPos = 201.f;
			MidiccDisplay *MccDisplay = createWidget<MidiccDisplay>(Vec(xPos,yPos));
			MccDisplay->box.size = {34.f, 13.f};
			MccDisplay->displayID =  dispID ++;;
			MccDisplay->module = module;
			addChild(MccDisplay);
			// Pitch Bend LCD
			xPos = 10.5f;
			yPos = 256.5f;
			for ( int i = 0; i < 2; i++){
				MidiccDisplay *MccDisplay = createWidget<MidiccDisplay>(Vec(xPos,yPos));
				MccDisplay->box.size = {30.f, 13.f};
				MccDisplay->displayID =  dispID ++;;
				MccDisplay->module = module;
				addChild(MccDisplay);
				xPos += 33.f;
			}
		}
		yPos = 150.5f;
		xPos = 13.f;
		// ch Leds x 16
		for (int i = 0; i < 16; i++){
			addChild(createLight<TinyLight<RedLight>>(Vec(xPos + i * 8.f, yPos), module, MIDIpolyMPE::CH_LIGHT + i));
		}
		xPos = 57.f;
		yPos = 107.f;
		////DATA KNOB + -
		addParam(createParam<springDataKnobB>(Vec(xPos, yPos), module, MIDIpolyMPE::DATAKNOB_PARAM));
		yPos = 113.5f;
		xPos = 24.f;
		addParam(createParam<minusButtonB>(Vec(xPos, yPos), module, MIDIpolyMPE::MINUSONE_PARAM));
		xPos = 103.f;
		addParam(createParam<plusButtonB>(Vec(xPos, yPos), module, MIDIpolyMPE::PLUSONE_PARAM));
		// X Y Z Outs
		float xOffset = 44.f;
		yPos = 172.f;
		xPos = 19.5f;
			addOutput(createOutput<moDllzPortPoly>(Vec(xPos, yPos),  module, MIDIpolyMPE::X_OUTPUT));
			xPos += xOffset;
			addOutput(createOutput<moDllzPortPoly>(Vec(xPos, yPos),  module, MIDIpolyMPE::Y_OUTPUT));
			xPos += xOffset;
			addOutput(createOutput<moDllzPortPoly>(Vec(xPos, yPos),  module, MIDIpolyMPE::Z_OUTPUT));
		yPos = 217.f;
		xPos = 24.5f;
		xOffset = 35.f;
		//Vel RVel Gate Outs
		addOutput(createOutput<moDllzPortPoly>(Vec(xPos, yPos),  module, MIDIpolyMPE::RVEL_OUTPUT));
		xPos += xOffset;
		addOutput(createOutput<moDllzPortPoly>(Vec(xPos, yPos),  module, MIDIpolyMPE::VEL_OUTPUT));
		xPos += xOffset;
		addOutput(createOutput<moDllzPortPoly>(Vec(xPos, yPos),  module, MIDIpolyMPE::GATE_OUTPUT));
		xPos += xOffset;
		///Sustain hold notes switch
		addParam(createParam<moDllzSwitchLed>(Vec(xPos - 3.f, yPos + 4.f), module, MIDIpolyMPE::SUSTHOLD_PARAM));
		addChild(createLight<TranspOffRedLight>(Vec(xPos - 3.f, yPos + 4.f), module, MIDIpolyMPE::SUSTHOLD_LIGHT));
		// PBend Out
		yPos = 251.5f;
		xPos = 117.5f;
		addOutput(createOutput<moDllzPort>(Vec(xPos, yPos),  module, MIDIpolyMPE::PBEND_OUTPUT));
		// CC's x 8
		yPos = 282.f;
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
				addOutput(createOutput<moDllzPort>(Vec(xPos + 3.5f, yPos + 13.f),  module, MIDIpolyMPE::MMA_OUTPUT + i + r * 4));
				xPos += 33.f;
			}
			yPos += 40.f;
		}
	}
};

Model *modelMIDIpolyMPE = createModel<MIDIpolyMPE, MIDIpolyMPEWidget>("MIDIpolyMPE");
