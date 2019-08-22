#include "moDllz.hpp"
/*
 * MIDI8MPE Midi to 8ch CV with MPE and regular Polyphonic modes
 */
struct MIDI8MPE : Module {
	enum ParamIds {
		RESETMIDI_PARAM,
		LCURSOR_PARAM,
		RCURSOR_PARAM,
		PLUSONE_PARAM,
		MINUSONE_PARAM,
		LEARNCCA_PARAM,
		LEARNCCB_PARAM,
		LEARNCCC_PARAM,
		LEARNCCD_PARAM,
		LEARNCCE_PARAM,
		LEARNCCF_PARAM,
		SUSTHOLD_PARAM,
		DATAKNOB_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(X_OUTPUT, 8),
		ENUMS(Y_OUTPUT, 8),
		ENUMS(Z_OUTPUT, 8),
		ENUMS(VEL_OUTPUT, 8),
		ENUMS(GATE_OUTPUT, 8),
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
		ENUMS(CH_LIGHT, 8),
		SUSTHOLD_LIGHT,
		NUM_LIGHTS
	};

	midi::InputQueue midiInput;

	enum PolyMode {
		MPE_MODE,
		ROTATE_MODE,
		REUSE_MODE,
		RESET_MODE,
		REASSIGN_MODE,
		UNISON_MODE,
		NUM_MODES
	};
	PolyMode polyMode = ROTATE_MODE;

	bool holdGates = true;
	struct NoteData {
		uint8_t velocity = 0;
		uint8_t aftertouch = 0;
	};

	NoteData noteData[128];
	
	// cachedNotes : UNISON_MODE and REASSIGN_MODE cache all played notes. The other polyModes cache stolen notes (after the 4th one).
	std::vector<uint8_t> cachedNotes;
	
	std::vector<uint8_t> cachedMPE[8];
	
	
	uint8_t notes[8] = {0};
	uint8_t vels[8] = {0};
	int16_t mpex[8] = {0};
	uint16_t mpey[8] = {0};
	uint16_t mpez[8] = {0};
	uint8_t mpeyLB[8] = {0};
	uint8_t mpezLB[8] = {0};
	
	uint8_t mpePlusLB[8] = {0};

	bool gates[8] = {false};
	uint8_t Maft = 0;
	int midiCCs[6] = {128,1,129,11,7,64};
	uint8_t midiCCsVal[6] = {0};
	uint16_t Mpit = 8192;
	float xpitch[8] = {0.f};
	
	// gates set to TRUE by pedal and current gate. FALSE by pedal.
	bool pedalgates[8] = {false};
	bool pedal = false;
	int rotateIndex = 0;
	int stealIndex = 0;
	int numVo = 8;
	int polyModeIx = 1;
	int pbMain = 12;
	int pbMPE = 96;
	int mpeYcc = 74; //cc74 (default MPE Y)
	int mpeZcc = 128; //128 = ChannelAfterTouch (default MPE Z)
	int MPEmode = 0; // Index of different MPE modes...(User and HakenPlus for now)
	int savedMidiCh = -1;//to reset channel from MPE all channels
	int MPEmasterCh = 0;// 0 ~ 15
	int MPEfirstCh = 1;// 0 ~ 15

	int displayYcc = 74;
	int displayZcc = 128;

	int learnIx = 0;

	int cursorIx = 0;
	int cursorI = 0;
	int selectedmidich = 0;
	int cursorPoly[9] = {0,1,3,7,8,9,10,11,12};
	int cursorMPE[12] = {0,2,3,4,5,6,7,8,9,10,11,12};
	int cursorMPEsub[10] = {0,2,3,4,7,8,9,10,11,12};
	//float dummy = 0.f;
	float dataKnob = 0.f;
	int frameData = 0;
	

	dsp::ExponentialFilter MPExFilter[8];
	dsp::ExponentialFilter MPEyFilter[8];
	dsp::ExponentialFilter MPEzFilter[8];
	dsp::ExponentialFilter MCCsFilter[6];
	dsp::ExponentialFilter MpitFilter;
	
	// retrigger for stolen notes (when gates already open)
	dsp::PulseGenerator reTrigger[8];
	dsp::SchmittTrigger resetMidiTrigger;

	dsp::SchmittTrigger PlusOneTrigger;
	dsp::SchmittTrigger MinusOneTrigger;
	dsp::SchmittTrigger LcursorTrigger;
	dsp::SchmittTrigger RcursorTrigger;

	dsp::SchmittTrigger learnCCsTrigger[6];

	
	MIDI8MPE() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(RESETMIDI_PARAM, 0.f, 1.f, 0.f);
		configParam(MINUSONE_PARAM, 0.f, 1.f, 0.f);
		configParam(PLUSONE_PARAM, 0.f, 1.f, 0.f);
		configParam(LCURSOR_PARAM, 0.f, 1.f, 0.f);
		configParam(RCURSOR_PARAM, 0.f, 1.f, 0.f);
		configParam(LEARNCCA_PARAM, 0.f, 1.f, 0.f);
		configParam(LEARNCCB_PARAM, 0.f, 1.f, 0.f);
		configParam(LEARNCCC_PARAM, 0.f, 1.f, 0.f);
		configParam(LEARNCCD_PARAM, 0.f, 1.f, 0.f);
		configParam(LEARNCCE_PARAM, 0.f, 1.f, 0.f);
		configParam(LEARNCCF_PARAM, 0.f, 1.f, 0.f);
		configParam(SUSTHOLD_PARAM, 0.f, 1.f, 1.f);
		configParam(DATAKNOB_PARAM, -1.f, 1.f, 0.f);
		onReset();
	}

	json_t *dataToJson() override {
		json_t *rootJ = json_object();
		json_object_set_new(rootJ, "midi", midiInput.toJson());
		json_object_set_new(rootJ, "polyMode", json_integer(polyMode));
		json_object_set_new(rootJ, "pbMain", json_integer(pbMain));
		json_object_set_new(rootJ, "pbMPE", json_integer(pbMPE));
		json_object_set_new(rootJ, "numVo", json_integer(numVo));
		json_object_set_new(rootJ, "MPEmasterCh", json_integer(MPEmasterCh));
		json_object_set_new(rootJ, "MPEfirstCh", json_integer(MPEfirstCh));
		json_object_set_new(rootJ, "midiAcc", json_integer(midiCCs[0]));
		json_object_set_new(rootJ, "midiBcc", json_integer(midiCCs[1]));
		json_object_set_new(rootJ, "midiCcc", json_integer(midiCCs[2]));
		json_object_set_new(rootJ, "midiDcc", json_integer(midiCCs[3]));
		json_object_set_new(rootJ, "midiEcc", json_integer(midiCCs[4]));
		json_object_set_new(rootJ, "midiFcc", json_integer(midiCCs[5]));
		json_object_set_new(rootJ, "mpeYcc", json_integer(mpeYcc));
		json_object_set_new(rootJ, "mpeZcc", json_integer(mpeZcc));
		json_object_set_new(rootJ, "MPEmode", json_integer(MPEmode));
		return rootJ;
	}

	void dataFromJson(json_t *rootJ) override {
		json_t *midiJ = json_object_get(rootJ, "midi");
		if (midiJ)
			midiInput.fromJson(midiJ);
		json_t *polyModeJ = json_object_get(rootJ, "polyMode");
		if (polyModeJ)
			polyMode = (PolyMode) json_integer_value(polyModeJ);
		polyModeIx = polyMode;
		json_t *pbMainJ = json_object_get(rootJ, "pbMain");
		if (pbMainJ)
			pbMain = json_integer_value(pbMainJ);
		json_t *pbMPEJ = json_object_get(rootJ, "pbMPE");
		if (pbMPEJ)
			pbMPE = json_integer_value(pbMPEJ);
		json_t *numVoJ = json_object_get(rootJ, "numVo");
		if (numVoJ)
			numVo = json_integer_value(numVoJ);
		json_t *MPEmasterChJ = json_object_get(rootJ, "MPEmasterCh");
		if (MPEmasterChJ)
			MPEmasterCh = json_integer_value(MPEmasterChJ);
		json_t *MPEfirstChJ = json_object_get(rootJ, "MPEfirstCh");
		if (MPEfirstChJ)
			MPEfirstCh = json_integer_value(MPEfirstChJ);
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
		json_t *mpeYccJ = json_object_get(rootJ, "mpeYcc");
		if (mpeYccJ)
			mpeYcc = json_integer_value(mpeYccJ);
		json_t *mpeZccJ = json_object_get(rootJ, "mpeZcc");
		if (mpeZccJ)
			mpeZcc = json_integer_value(mpeZccJ);
		json_t *MPEmodeJ = json_object_get(rootJ, "MPEmode");
		if (MPEmodeJ)
			MPEmode = json_integer_value(MPEmodeJ);
		
		if (polyModeIx > 0){
			displayYcc = 129;
			displayZcc = 130;
		}else if (MPEmode > 0){
			displayYcc = 131;
			displayZcc = 132;
		}else {
			displayYcc = mpeYcc;
			displayZcc = mpeZcc;
		}
	}
///////////////////////////ON RESET
	void onReset() override {
		for (int i = 0; i < 8; i++) {
			notes[i] = 60;
			gates[i] = false;
			pedalgates[i] = false;
			mpey[i] = 0.f;
		}
		rotateIndex = -1;
		cachedNotes.clear();
		float lambdaf = 100.f * APP->engine->getSampleTime();
		
		if (polyMode == MPE_MODE) {
			midiInput.channel = -1;
			for (int i = 0; i < 8; i++) {
				mpex[i] = 0.f;
				mpez[i] = 0.f;
				cachedMPE[i].clear();
				MPExFilter[i].lambda = lambdaf;
				MPEyFilter[i].lambda = lambdaf;
				MPEzFilter[i].lambda = lambdaf;
			}
			if (MPEmode > 0){// Haken MPE Plus
				displayYcc = 131;
				displayZcc = 132;
			}else{
				displayYcc = mpeYcc;
				displayZcc = mpeZcc;
			}
		} else {
			displayYcc = 129;
			displayZcc = 130;
		}
		learnIx = 0;
		
		MpitFilter.lambda = lambdaf;
		for (int i=0; i < 6; i++){
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
		// All taken = steal (stealIndex always rotates)
		stealIndex++;
		if (stealIndex > (numVo - 1))
			stealIndex = 0;
		///if ((polyMode > MPE_MODE) && (polyMode < REASSIGN_MODE) && (gates[stealIndex]))
		/// cannot reach here if polyMode == MPE mode ...no need to check
		if ((polyMode < REASSIGN_MODE) && (gates[stealIndex]))
			cachedNotes.push_back(notes[stealIndex]);
		return stealIndex;
	}

	void pressNote(uint8_t channel, uint8_t note, uint8_t vel) {
		
		// Set notes and gates
		switch (polyMode) {
			case MPE_MODE: {
				//////if gate push note to mpe_buffer for legato/////
				rotateIndex = channel - MPEfirstCh;
				if ((rotateIndex < 0) || (rotateIndex > 7)) return;
				if (gates[rotateIndex]) cachedMPE[rotateIndex].push_back(notes[rotateIndex]);
				
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
		
		if (polyMode > MPE_MODE) {
		// Remove the note
		auto it = std::find(cachedNotes.begin(), cachedNotes.end(), note);
		if (it != cachedNotes.end())
			cachedNotes.erase(it);
		}else{
			int i = channel - MPEfirstCh;
			if ((i < 0) || (i > 7)) return;
			auto it = std::find(cachedMPE[i].begin(), cachedMPE[i].end(), note);
			if (it != cachedMPE[i].end())
				cachedMPE[i].erase(it);
		}

		switch (polyMode) {
			case MPE_MODE: {
				int i = channel - MPEfirstCh;
				if (note == notes[i]) {
					if (pedalgates[i]) {
						gates[i] = false;
					}
					/// check for cachednotes on MPE buffers [8]...
					else if (!cachedMPE[i].empty()) {
						notes[i] = cachedMPE[i].back();
						cachedMPE[i].pop_back();
					}
					else {
						gates[i] = false;
					}
					if (vel < 128) // 128 = from NoteOn ZeroVel
						vels[i] = vel;///Rel Vel
				}
			} break;

			case REASSIGN_MODE: {
				if (vel > 128) vel = 64;
				for (int i = 0; i < numVo; i++) {
					if (i < (int) cachedNotes.size()) {
						if (!pedalgates[i])
							notes[i] = cachedNotes[i];
						pedalgates[i] = pedal;
					}
					else {
						gates[i] = false;
						mpey[i] = vel * 128;
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
						mpey[i] = vel * 128;
					}
				}
				else {
					for (int i = 0; i < numVo; i++) {
						gates[i] = false;
						mpey[i] = vel * 128;
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
						if (vel < 128) // 128 = from NoteOn ZeroVel
							mpey[i] = vel * 128;
						else//Fixed RelVel
							mpey[i] = 8192;
					}
				}
			} break;
		}
	}

	void pressPedal() {
		pedal = true;
		lights[SUSTHOLD_LIGHT].value = params[SUSTHOLD_PARAM].getValue();
		if (polyMode == MPE_MODE) {
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
		if (polyMode == MPE_MODE) {
			for (int i = 0; i < 8; i++) {
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
					if  (polyMode < REASSIGN_MODE){
						notes[i] = cachedNotes.back();
						cachedNotes.pop_back();
						gates[i] = true;
					}
				}
			}
			if (polyMode == REASSIGN_MODE) {
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

		float pbVo = 0.f;
		if (Mpit < 8192){
			pbVo = MpitFilter.process(1.f ,rescale(Mpit, 0, 8192, -5.f, 0.f));
		} else {
			pbVo = MpitFilter.process(1.f ,rescale(Mpit, 8192, 16383, 0.f, 5.f));
		}
//		outputs[MMA_OUTPUT].setVoltage(pbVo);
		bool sustainHold = (params[SUSTHOLD_PARAM].getValue() > .5 );

		if (polyMode > PolyMode::MPE_MODE){
			for (int i = 0; i < numVo; i++) {
				float lastGate = ((gates[i] || (sustainHold && pedalgates[i])) && (!(reTrigger[i].process(args.sampleTime))))? 10.f : 0.f;
				outputs[GATE_OUTPUT + i].setVoltage(lastGate);
				outputs[X_OUTPUT + i].setVoltage(((notes[i] - 60) / 12.f) + (pbVo * static_cast<float>(pbMain) / 60.f));
				outputs[VEL_OUTPUT + i].setVoltage(rescale(vels[i], 0, 127, 0.f, 10.f));
				outputs[Y_OUTPUT + i].setVoltage(rescale(mpey[i], 0, 16383, 0.f, 10.f));
				outputs[Z_OUTPUT + i].setVoltage(rescale(noteData[notes[i]].aftertouch, 0, 127, 0.f, 10.f));
				lights[CH_LIGHT + i].value = ((i == rotateIndex)? 0.2f : 0.f) + (lastGate * .08f);
			}
		} else {/// MPE MODE!!!
			for (int i = 0; i < 8; i++) {
				float lastGate = ((gates[i] || (sustainHold && pedalgates[i])) && (!(reTrigger[i].process(args.sampleTime)))) ? 10.f : 0.f;
				outputs[GATE_OUTPUT + i].setVoltage(lastGate );
				if ( mpex[i] < 0){
					xpitch[i] = (MPExFilter[i].process(1.f ,rescale(mpex[i], -8192 , 0, -5.f, 0.f))) * pbMPE / 60.f;
				} else {
					xpitch[i] = (MPExFilter[i].process(1.f ,rescale(mpex[i], 0, 8191, 0.f, 5.f))) * pbMPE / 60.f;
				}
				outputs[X_OUTPUT + i].setVoltage(xpitch[i] + ((notes[i] - 60) / 12.f) + (pbVo * static_cast<float>(pbMain) / 60.f));
				outputs[VEL_OUTPUT + i].setVoltage(rescale(vels[i], 0, 127, 0.f, 10.f));
				outputs[Y_OUTPUT + i].setVoltage(MPEyFilter[i].process(1.f ,rescale(mpey[i], 0, 16383, 0.f, 10.f)));
				outputs[Z_OUTPUT + i].setVoltage(MPEzFilter[i].process(1.f ,rescale(mpez[i], 0, 16383, 0.f, 10.f)));
				lights[CH_LIGHT + i].value = ((i == rotateIndex)? 0.2f : 0.f) + (lastGate * .08f);
			}
		}
		for (int i = 0; i < 6; i++){
			if (midiCCs[i] == 128)
				outputs[MMA_OUTPUT + i].setVoltage(pbVo);
			else if (midiCCs[i] == 129)
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
		if (LcursorTrigger.process(params[LCURSOR_PARAM].getValue())) {
			if (polyMode == MPE_MODE){
				if (MPEmode > 0){
					if (cursorI > 0) cursorI --;
					else cursorI = 9;
					cursorIx = cursorMPEsub[cursorI];
				}else{
					if (cursorI > 0) cursorI --;
					else cursorI = 11;
					cursorIx = cursorMPE[cursorI];
				}
			}else{
				if (cursorI > 0) cursorI --;
				else cursorI = 8;
				cursorIx = cursorPoly[cursorI];
			}
			learnIx = 0;
			return;
		}
		if (RcursorTrigger.process(params[RCURSOR_PARAM].getValue())) {
			if (polyMode == MPE_MODE){
				if (MPEmode > 0){
					if (cursorI < 9) cursorI ++;
					else cursorI = 0;
					cursorIx = cursorMPEsub[cursorI];
				}else{
					if (cursorI < 11) cursorI ++;
					else cursorI = 0;
					cursorIx = cursorMPE[cursorI];
				}
			}else{
				if (cursorI < 8) cursorI ++;
				else cursorI = 0;
				cursorIx = cursorPoly[cursorI];
			}
			learnIx = 0;
			return;
		}
		for (int i = 0; i < 6; i++){
			if (learnCCsTrigger[i].process(params[LEARNCCA_PARAM + i].getValue())) {
				if (learnIx == i + 1)
					learnIx = 0;
				else{
					learnIx = i + 1;
					//cursorIx = i + 7;
				}
				return;
			}
		}
		
		///// RESET MIDI
		if (resetMidiTrigger.process(params[RESETMIDI_PARAM].getValue())) {
			lights[RESETMIDI_LIGHT].value= 1.0f;
			onReset();
			return;
		}
		if (lights[RESETMIDI_LIGHT].value > 0.0001f){
			lights[RESETMIDI_LIGHT].value -= 0.0001f ; // fade out light
		}
	}
///////////////////////
//////   STEP END
///////////////////////

	void processMessage(midi::Message msg) {


		switch (msg.getStatus()) {
			// note off
			case 0x8: {
				if ((polyMode == MPE_MODE) && (msg.getChannel() == MPEmasterCh)) return;
				releaseNote(msg.getChannel(), msg.getNote(), msg.getValue());
			} break;
			// note on
			case 0x9: {
				if ((polyMode == MPE_MODE) && (msg.getChannel() == MPEmasterCh)) return;
				if (msg.getValue() > 0) {
					//noteData[msg.getNote()].velocity = msg.getValue();
					pressNote(msg.getChannel(), msg.getNote(), msg.getValue());
				}
				else {
					releaseNote(msg.getChannel(), msg.getNote(), 128);//128 to bypass Release vel on Vel Outputs
				}
			} break;
			// note (poly) aftertouch
			case 0xa: {
				if (polyMode == MPE_MODE) return;
				noteData[msg.getNote()].aftertouch = msg.getValue();
			} break;
				
			// channel aftertouch
			case 0xd: {
				if (learnIx > 0) {// learn enabled ???
					midiCCs[learnIx - 1] = 129;
					learnIx = 0;
					return;
				}////////////////////////////////////////
				else if (polyMode == MPE_MODE){
					if (msg.getChannel() == MPEmasterCh){
						Maft = msg.getNote();
					}else if (MPEmode == 1){
						mpez[msg.getChannel() - MPEfirstCh] =  msg.getNote() * 128 + mpePlusLB[msg.getChannel() - MPEfirstCh];
						mpePlusLB[msg.getChannel() - MPEfirstCh] = 0;
					}else {
						if (mpeZcc == 128)
							mpez[msg.getChannel() - MPEfirstCh] = msg.getNote() * 128;
						if (mpeYcc == 128)
							mpey[msg.getChannel() - MPEfirstCh] = msg.getNote() * 128;
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
				else if (polyMode == MPE_MODE){
					if (msg.getChannel() == MPEmasterCh){
						Mpit = msg.getValue() * 128 + msg.getNote();
					}else{
						mpex[msg.getChannel() - MPEfirstCh] = msg.getValue() * 128 + msg.getNote() - 8192;
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
				}else if (polyMode == MPE_MODE){
					if (msg.getChannel() == MPEmasterCh){
						processCC(msg);
					}else if (MPEmode == 1){ //Continuum
						if (msg.getNote() == 87){
							mpePlusLB[msg.getChannel() - MPEfirstCh] = msg.getValue();
						}else if (msg.getNote() == 74){
							mpey[msg.getChannel() - MPEfirstCh] =  msg.getValue() * 128 + mpePlusLB[msg.getChannel() - MPEfirstCh];
							mpePlusLB[msg.getChannel() - MPEfirstCh] = 0;
						}

					}else if (msg.getNote() == mpeYcc){
					//cc74 0x4a default
						mpey[msg.getChannel() - MPEfirstCh] = msg.getValue() * 128;
					}else if (msg.getNote() == mpeZcc){
						mpez[msg.getChannel() - MPEfirstCh] = msg.getValue() * 128;
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
				if (polyMode == MPE_MODE){
					if (MPEmode < 1){
						MPEmode ++;
						onReset();
					}else{//last MPE submode... go to Poly
						polyMode = (PolyMode) (1);
						onReset();
						midiInput.channel = savedMidiCh;//restore channel
					}
				}else if (polyMode < UNISON_MODE) {
					polyMode = (PolyMode) (polyMode + 1);
					onReset();
				}else {
					polyMode = MPE_MODE;
					MPEmode = 0; // no MPE submode...
					savedMidiCh = midiInput.channel;// save Poly MIDI channel
					onReset();
				}
				polyModeIx = polyMode;
				
			}break;
			case 1: {
				if (numVo < 8) numVo ++;
				//else numVo = 2;
				onReset();
			}break;
			case 2: {
				if (MPEfirstCh < 8){
					MPEfirstCh ++;
					MPEmasterCh = MPEfirstCh - 1;
				}else{
					MPEfirstCh = 0;
					MPEmasterCh = 15;
				}
				onReset();
			}break;
			case 3: {
				if (pbMain < 96) pbMain ++;
			}break;
			case 4: {
				if (pbMPE < 96) pbMPE ++;
			}break;
			case 5: {
				if (mpeYcc <128)
					mpeYcc ++;
				else
					mpeYcc = 0;
				displayYcc = mpeYcc;
			}break;
			case 6: {
				if (mpeZcc <128)
					mpeZcc ++;
				else
					mpeZcc = 0;
				displayZcc = mpeZcc;
			}break;
			default: {
				if (midiCCs[cursorIx - 7] < 129)
					midiCCs[cursorIx - 7]  ++;
				else
					midiCCs[cursorIx - 7] = 0;
			}break;
		}
		learnIx = 0;;
		return;
	}
	void dataMinus(){
		switch (cursorIx){
			case 0: {
				if (polyMode > MPE_MODE) {
					polyMode = (PolyMode) (polyMode - 1);
					MPEmode = 1;
					savedMidiCh = midiInput.channel;
					onReset();
				}else if (MPEmode > 0){
					MPEmode --;
					onReset();
				}else {//last MPE submode... go to Poly
					polyMode = UNISON_MODE;
					onReset();
					midiInput.channel = savedMidiCh;//restore channel
				}
				polyModeIx = polyMode;
			}break;
			case 1: {
				if (numVo > 2) numVo --;
				//else numVo = 8;
				onReset();
			}break;
			case 2:{
				if (MPEfirstCh > 1){
					MPEfirstCh -- ;
					MPEmasterCh = MPEfirstCh - 1;
				}else if (MPEfirstCh == 1){
					MPEfirstCh = 0;
					MPEmasterCh = 15;
				}else {
					MPEfirstCh = 8;
					MPEmasterCh = 7;
				}
				onReset();
			}break;
			case 3: {
				if (pbMain > 0) pbMain --;
			}break;
			case 4: {
				if (pbMPE > 0) pbMPE --;
			}break;
			case 5: {
				if (mpeYcc > 0)
					mpeYcc --;
				else
					mpeYcc = 128;
				displayYcc = mpeYcc;
			}break;
			case 6: {
				if (mpeZcc > 0)
					mpeZcc --;
				else
					mpeZcc = 128;
				displayZcc = mpeZcc;
			}break;
			default: {
				if (midiCCs[cursorIx - 7] > 0)
					midiCCs[cursorIx - 7] --;
				else
					midiCCs[cursorIx - 7] = 129;
			}break;
		}
		learnIx = 0;
		return;
	}
};
// Main Display
struct PolyModeDisplayB : TransparentWidget {
	PolyModeDisplayB(){
		// font = Font::load(mFONT_FILE);
		font = APP->window->loadFont(mFONT_FILE);
	}
	
	MIDI8MPE *module;
	
	int pointerinit = 0;
	float mdfontSize = 12.f;
	std::string sMode ="";
	std::string sVo ="";
	std::string sPBM ="";
	std::string sPBMPE ="";
	std::string sMPEmidiCh = "";
	std::string yyDisplay = "";
	std::string zzDisplay = "";
	std::shared_ptr<Font> font;
	std::string polyModeStr[6] = {
		"M. P. E.",
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
		if (module) {
			int p_polyMode = module->polyModeIx;
			int p_MPEmode = module->MPEmode;
			int p_numVo = module->numVo;
			int p_pbMain = module->pbMain;
			int p_pbMPE = module->pbMPE;
			int p_MPEmasterCh = module->MPEmasterCh;
			int p_MPEfirstCh = module->MPEfirstCh;
			int p_YccNumber = module->displayYcc;
			int p_ZccNumber = module->displayZcc;
			int p_cursorIx = module->cursorIx;
		
		if (drawFrame ++ > 5){
			drawFrame = 0;
					if (p_polyMode < 1) {
						if (p_MPEmode == 1) sMode = "M. P. E. Plus";/// Continuum Hi Res YZ
						else if (p_MPEmode > 1) sMode = "M. P. E. w RelVel";
						else sMode = polyModeStr[p_polyMode];
					}else{
						sMode = polyModeStr[p_polyMode];
					}
					sVo = "Poly "+ std::to_string(p_numVo) +" Vo outs";
					sPBM = "PBend:" + std::to_string(p_pbMain);
					sPBMPE = " CH PBend:" + std::to_string(p_pbMPE);
					sMPEmidiCh = "channels M:" + std::to_string(p_MPEmasterCh + 1) + " Vo:" + std::to_string(p_MPEfirstCh + 1) + "++";

					switch (p_YccNumber) {
						case 129 :{//(locked)  Rel Vel
							yyDisplay = "rVel";
						}break;
						case 131 :{//HiRes MPE Y
							yyDisplay = "cc74+";
						}break;
						default :{
							yyDisplay = "cc" + std::to_string(p_YccNumber);
						}
					}
					switch (p_ZccNumber) {
						case 128 :{
							zzDisplay = "chnAT";
						}break;
						case 130 :{//(locked)  note AfterT
							zzDisplay = "nteAT";
						}break;
						case 132 :{//HiRes MPE Z
							zzDisplay = "chAT+";
						}break;
						default :{
							zzDisplay = "cc" + std::to_string(p_ZccNumber);
						}
					}
				//}
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
			nvgTextBox(args.vg, 50.f, 52.f, 31.f, yyDisplay.c_str(), NULL);// YY
			nvgTextBox(args.vg, 82.f, 52.f, 31.f, zzDisplay.c_str(), NULL);// ZZ
			
			if (p_polyMode < 1){
				nvgTextBox(args.vg, 4.f, 24.f,124.f, sMPEmidiCh.c_str(), NULL);// MPE Channels
				nvgTextAlign(args.vg, NVG_ALIGN_LEFT);
				nvgTextBox(args.vg, 58.f, 37.f,66.f, sPBMPE.c_str(), NULL);//MPE PitchBend
			} else {
				nvgTextBox(args.vg, 4.f, 24.f,124.f, sVo.c_str(), NULL);
			}
			
			nvgTextAlign(args.vg, NVG_ALIGN_LEFT);
			nvgTextBox(args.vg, 4.f, 37.0f, 50.f, sPBM.c_str(), NULL);
			nvgGlobalCompositeBlendFunc(args.vg,  NVG_ONE , NVG_ONE);
			nvgBeginPath(args.vg);
			switch (cursorIxI){
				case 0:{ // PolyMode
					nvgRoundedRect(args.vg, 1.f, 1.f, 130.f, 12.f, 3.f);
				}break;
				case 1:{ //numVoices Poly
					nvgRoundedRect(args.vg, 1.f, 14.f, 130.f, 12.f, 3.f);
				}break;
				case 2:{ //MPE channels
					nvgRoundedRect(args.vg, 1.f, 14.f, 130.f, 12.f, 3.f);
				}break;
				case 3:{//mainPB
					nvgRoundedRect(args.vg, 1.f, 27.f, 52.f, 12.f, 3.f);
				}break;
				case 4:{//mpePB
					nvgRoundedRect(args.vg, 54.f, 27.f, 77.f, 12.f, 3.f);
				}break;
				case 5:{//YY
					nvgRoundedRect(args.vg, 50.f, 42.f, 31, 13.f, 3.f);
				}break;
				case 6:{//ZZ
					nvgRoundedRect(args.vg, 82.f, 42.f, 31, 13.f, 3.f);
				}break;
			}
			if (flashFocus > 0)
				flashFocus -= 2;
			int rgbint = 0x55 + flashFocus;
			nvgFillColor(args.vg, nvgRGB(rgbint,rgbint,rgbint)); //SELECTED
			nvgFill(args.vg);
		} else{///PREVIEW
			std::shared_ptr<Font> mfont;
			std::string s1 = "MIDI to 8ch";
			std::string s2 = "CV with MPE";
			font = APP->window->loadFont(mFONT_FILE);
			nvgFontSize(args.vg, 20.f);
			nvgFontFaceId(args.vg, font->handle);
			nvgTextAlign(args.vg, NVG_ALIGN_CENTER);
			nvgFillColor(args.vg, nvgRGB(0xDD,0xDD,0xDD));
			nvgTextBox(args.vg, 0.f, 16.f,box.size.x, s1.c_str(), NULL);
			nvgTextBox(args.vg, 0.f, 36.f,box.size.x, s2.c_str(), NULL);
		}
	}
};

struct MidiccDisplayB : TransparentWidget {
	MidiccDisplayB(){
	font = APP->window->loadFont(mFONT_FILE);
}
	MIDI8MPE *module;
	float mdfontSize = 12.f;
	std::string sDisplay = "";
	int pointerinit = 0;
	int p_cursor = 0;
	int cursorI = -1;
	int displayID = 0;//set on each instance
	int ccNumber = -1;
	bool learnOn = false;
	bool learnChanged = false;

	int flashFocus = 0;
	int displayFrames = 0;
	std::shared_ptr<Font> font;
	void draw(const DrawArgs &args) override{

		if ((module) && (displayFrames ++ > 5)){
			displayFrames = 0;
			int p_ccNumber = module->midiCCs[displayID];
			p_cursor = module->cursorIx - 7;
			//int p_learnIx = ;
			learnOn = (displayID + 1 == module->learnIx);
			if (learnOn){
				learnChanged = true;
				sDisplay = "LRN";
			}else if ((ccNumber != p_ccNumber) || (learnChanged)){
				learnChanged = false;
				ccNumber = p_ccNumber;
				switch (ccNumber) {
					case 128 :{
						sDisplay = "PBnd";
					}break;
					case 129 :{
						sDisplay = "chAT";
					}break;
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
					default :{
						sDisplay = "c" + std::to_string(ccNumber);
					}
				}
			}
		}
		if (learnOn) {
			nvgBeginPath(args.vg);
			nvgRoundedRect(args.vg, 0.f, 0.f, box.size.x, box.size.y,3.f);
			nvgStrokeColor(args.vg, nvgRGB(0xdd, 0x0, 0x0));
			nvgStroke(args.vg);
			nvgRoundedRect(args.vg, 0.f, 0.f, box.size.x, box.size.y,3.f);

			nvgFillColor(args.vg, nvgRGBA(0xcc, 0x0, 0x0,0x64));
			nvgFill(args.vg);
			///text color
			nvgFillColor(args.vg, nvgRGB(0xff, 0x00, 0x00));//LEARN
		}else{
			///text color
			nvgFillColor(args.vg, nvgRGB(0xcc, 0xcc, 0xcc));
		}
		nvgFontSize(args.vg, mdfontSize);
		nvgFontFaceId(args.vg, font->handle);
		nvgTextAlign(args.vg, NVG_ALIGN_CENTER);
		nvgTextBox(args.vg, 0.f, 10.f,box.size.x, sDisplay.c_str(), NULL);

		if (cursorI != p_cursor){
			cursorI = p_cursor;
			if (p_cursor == displayID)
				flashFocus = 64;
		}
		if ((displayID == cursorI) && (!learnOn)){
			nvgGlobalCompositeBlendFunc(args.vg,  NVG_ONE , NVG_ONE);
			nvgBeginPath(args.vg);
			nvgRoundedRect(args.vg, 0.f, 0.f, box.size.x, box.size.y,3.f);
//			nvgStrokeColor(args.vg, nvgRGB(0x66, 0x66, 0x66));
//			nvgStroke(args.vg);
			if (flashFocus > 0)
				flashFocus -= 2;
			int rgbint = 0x55 + flashFocus;
			nvgFillColor(args.vg, nvgRGB(rgbint,rgbint,rgbint)); //SELECTED
			nvgFill(args.vg);
		}
	}
};

struct BlockChannel : OpaqueWidget {
	MIDI8MPE *module;
	void draw(const DrawArgs &args) override {
		if (module) {
		int p_polyMode = module->polyModeIx;
		if ( p_polyMode > 0) {
				box.size = Vec(0.f,0.f);
			}else{
				box.size = Vec(94.f,13.f);
				NVGcolor ledColor = nvgRGBA(0x00, 0x00, 0x00,0xaa);
				nvgBeginPath(args.vg);
				nvgRoundedRect(args.vg, 0.f, 0.f, 94.f, 13.f,3.f);
				nvgFillColor(args.vg, ledColor);
				nvgFill(args.vg);
			}
		}
	}
};

///MIDIlearnMCC
struct learnMccButton : SvgSwitch {
	learnMccButton() {
		momentary = true;
		box.size = Vec(26, 13);
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/learnMcc_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/learnMcc_1.svg")));
	}
};

struct springDataKnob : SvgKnob {
	springDataKnob() {
		minAngle = -0.75*M_PI;
		maxAngle = 0.75*M_PI;
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/dataKnob.svg")));
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

struct MIDI8MPEWidget : ModuleWidget {
	MIDI8MPEWidget(MIDI8MPE *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance,"res/MIDI8MPE.svg")));
		//Screws
		addChild(createWidget<ScrewBlack>(Vec(0, 0)));
		addChild(createWidget<ScrewBlack>(Vec(180, 0)));
		addChild(createWidget<ScrewBlack>(Vec(0, 365)));
		addChild(createWidget<ScrewBlack>(Vec(180, 365)));
		
		float xPos = 8.f;//61;
		float yPos = 18.f;
		{
			MidiWidget *midiWidget = createWidget<MidiWidget>(Vec(xPos,yPos));
			midiWidget->box.size = Vec(132.f,41.f);
			midiWidget->setMidiPort(module ? &module->midiInput : NULL);
			
			midiWidget->driverChoice->box.size.y = 12.f;
			midiWidget->deviceChoice->box.size.y = 12.f;
			midiWidget->channelChoice->box.size.y = 12.f;

			midiWidget->driverChoice->box.pos = Vec(0.f, 2.f);
			midiWidget->deviceChoice->box.pos = Vec(0.f, 15.f);
			midiWidget->channelChoice->box.pos = Vec(0.f, 28.f);

			midiWidget->driverSeparator->box.pos = Vec(0.f, 15.f);
			midiWidget->deviceSeparator->box.pos = Vec(0.f, 28.f);

			midiWidget->driverChoice->textOffset = Vec(2.f,10.f);
			midiWidget->deviceChoice->textOffset = Vec(2.f,10.f);
			midiWidget->channelChoice->textOffset = Vec(2.f,10.f);
			
			midiWidget->driverChoice->color = nvgRGB(0xcc, 0xcc, 0xcc);
			midiWidget->deviceChoice->color = nvgRGB(0xcc, 0xcc, 0xcc);
			midiWidget->channelChoice->color = nvgRGB(0xcc, 0xcc, 0xcc);
			addChild(midiWidget);
		}
		BlockChannel *blockChannel = createWidget<BlockChannel>(Vec(8.f,46.f));
		blockChannel->module = module;
		addChild(blockChannel);
		
		xPos = 102.f;
		yPos = 47.f;
		addParam(createParam<moDllzMidiPanic>(Vec(xPos, yPos), module, MIDI8MPE::RESETMIDI_PARAM));
		addChild(createLight<SmallLight<RedLight>>(Vec(xPos+3.f, yPos+3.f), module, MIDI8MPE::RESETMIDI_LIGHT));
		
		xPos = 8.f;
		yPos = 62.f;
		
		{
			PolyModeDisplayB *polyModeDisplay = new PolyModeDisplayB();
			polyModeDisplay->box.pos = Vec(xPos, yPos);
			polyModeDisplay->box.size = {132.f, 54.f};
			polyModeDisplay->module = module;
			addChild(polyModeDisplay);
		}
		
		yPos = 20.f;
		xPos = 145.f;
		addParam(createParam<minusButton>(Vec(xPos, yPos), module, MIDI8MPE::MINUSONE_PARAM));
		xPos = 169.f;
		addParam(createParam<plusButton>(Vec(xPos, yPos), module, MIDI8MPE::PLUSONE_PARAM));

		xPos = 147.f;
		yPos = 40.f;
////DATA KNOB
		addParam(createParam<springDataKnob>(Vec(xPos, yPos), module, MIDI8MPE::DATAKNOB_PARAM));
		yPos = 85.f;
		xPos = 145.5f;
		addParam(createParam<moDllzcursorL>(Vec(xPos, yPos), module, MIDI8MPE::LCURSOR_PARAM));
		xPos = 165.5f;
		addParam(createParam<moDllzcursorR>(Vec(xPos, yPos), module, MIDI8MPE::RCURSOR_PARAM));

		yPos = 118.f;
		float const xOffset = 32.f;
		for (int i = 0; i < 8; i++){
			xPos = 30.f;
			addChild(createLight<TinyLight<RedLight>>(Vec(xPos-7.f, yPos+10.f), module, MIDI8MPE::CH_LIGHT + i));
			addOutput(createOutput<moDllzPortDark>(Vec(xPos, yPos),  module, MIDI8MPE::X_OUTPUT + i));
			xPos += xOffset;
			addOutput(createOutput<moDllzPortDark>(Vec(xPos, yPos),  module, MIDI8MPE::Y_OUTPUT + i));
			xPos += xOffset;
			addOutput(createOutput<moDllzPortDark>(Vec(xPos, yPos),  module, MIDI8MPE::Z_OUTPUT + i));
			xPos += xOffset;
			addOutput(createOutput<moDllzPortDark>(Vec(xPos, yPos),  module, MIDI8MPE::VEL_OUTPUT + i));
			xPos += xOffset;
			addOutput(createOutput<moDllzPortDark>(Vec(xPos, yPos),  module, MIDI8MPE::GATE_OUTPUT + i));
			yPos += 25.f;
		}
		yPos = 336.f;
		xPos = 10.5f;
		for ( int i = 0; i < 6; i++){
			addOutput(createOutput<moDllzPort>(Vec(xPos, yPos),  module, MIDI8MPE::MMA_OUTPUT + i));
			xPos += 27.f;
		}
		
		yPos = 322.f;
		xPos = 9.f;
		for (int i = 0; i < 6; i++){
				MidiccDisplayB *MccDisplay = new MidiccDisplayB();
				MccDisplay->box.pos = Vec(xPos, yPos);
				MccDisplay->box.size = {26.f, 13.f};
				MccDisplay->displayID = i;// + 7;
				MccDisplay->module = module;
				addChild(MccDisplay);
				addParam(createParam<learnMccButton>(Vec(xPos, yPos), module, MIDI8MPE::LEARNCCA_PARAM + i));
			xPos += 27.f;
		}
		///Sustain hold notes		
		xPos = 173.f;
		yPos = 338.f;
		addParam(createParam<moDllzSwitchLed>(Vec(xPos, yPos), module, MIDI8MPE::SUSTHOLD_PARAM));
		addChild(createLight<TranspOffRedLight>(Vec(xPos, yPos), module, MIDI8MPE::SUSTHOLD_LIGHT));
	}
};

Model *modelMIDI8MPE = createModel<MIDI8MPE, MIDI8MPEWidget>("MIDI8MPE");
