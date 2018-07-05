#include <queue>
#include <list>
#include "midi.hpp"
#include "dsp/filter.hpp"
#include "dsp/digital.hpp"
#include "moDllz.hpp"

struct 		MIDI8MPE : Module {
	enum ParamIds {
		RESETMIDI_PARAM,
		LCURSOR_PARAM,
		RCURSOR_PARAM,
		PLUSONE_PARAM,
		MINUSONE_PARAM,
		LEARNCCY_PARAM,
		LEARNCCZ_PARAM,
		LEARNCCA_PARAM,
		LEARNCCB_PARAM,
		SUSTAINHOLD_PARAM,
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
		BEND_OUTPUT,
		MOD_OUTPUT,
		AFT_OUTPUT,
		CCA_OUTPUT,
		CCB_OUTPUT,
		SUS_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		RESETMIDI_LIGHT,
		ENUMS(CH_LIGHT, 8),
		//LEARNCCA_LIGHT,
		//LEARNCCB_LIGHT,
		NUM_LIGHTS
	};

	MidiInputQueue midiInput;

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
	uint8_t mpey[8] = {0};
	uint8_t mpez[8] = {0};
	bool gates[8] = {false};
	uint8_t Mmod = 0;
	uint8_t Mcca = 0;
	uint8_t Mccb = 0;
	uint8_t Maft = 0;
	uint8_t Msus = 0;
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
	int MPEmasterCh = 0;// 0 ~ 15
	int MPEfirstCh = 1;// 0 ~ 15
	int midiAcc = 11;
	int midiBcc = 7;
	
	int savedMidiCh = -1;//to reset channel from MPE all channels
	
	int displayYcc = 74;
	int displayZcc = 128;

	bool learnAcc = false;
	bool learnBcc = false;
	bool learnYcc = false;
	bool learnZcc = false;
	
	int cursorIx = 0;
	int cursorI = 0;
	int selectedmidich = 0;
	int cursorPoly[5] = {0,1,3,7,8};
	int cursorMPE[8] = {0,2,3,4,5,6,7,8};
	float dummy = 0.f;
	float *dataKnob = &dummy;
	int frameData = 100000;

	ExponentialFilter MpitFilter;
	ExponentialFilter MmodFilter;
	ExponentialFilter MccaFilter;
	ExponentialFilter MccbFilter;
	ExponentialFilter MsusFilter;
	ExponentialFilter MaftFilter;
	ExponentialFilter MPExFilter[8];
	ExponentialFilter MPEyFilter[8];
	ExponentialFilter MPEzFilter[8];
	
	// retrigger for stolen notes (when gates already open)
	PulseGenerator reTrigger[8];
	SchmittTrigger resetMidiTrigger;
	SchmittTrigger PlusOneTrigger;
	SchmittTrigger MinusOneTrigger;
	SchmittTrigger LcursorTrigger;
	SchmittTrigger RcursorTrigger;
	
	SchmittTrigger learnCCATrigger;
	SchmittTrigger learnCCBTrigger;
	SchmittTrigger learnCCYTrigger;
	SchmittTrigger learnCCZTrigger;
	
	
	MIDI8MPE() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {
		onReset();
	}

	json_t *toJson() override {
		json_t *rootJ = json_object();
		json_object_set_new(rootJ, "midi", midiInput.toJson());
		json_object_set_new(rootJ, "polyMode", json_integer(polyMode));
		json_object_set_new(rootJ, "pbMain", json_integer(pbMain));
		json_object_set_new(rootJ, "pbMPE", json_integer(pbMPE));
		json_object_set_new(rootJ, "numVo", json_integer(numVo));
		json_object_set_new(rootJ, "MPEmasterCh", json_integer(MPEmasterCh));
		json_object_set_new(rootJ, "MPEfirstCh", json_integer(MPEfirstCh));
		json_object_set_new(rootJ, "midiAcc", json_integer(midiAcc));
		json_object_set_new(rootJ, "midiBcc", json_integer(midiBcc));
		json_object_set_new(rootJ, "mpeYcc", json_integer(mpeYcc));
		json_object_set_new(rootJ, "mpeZcc", json_integer(mpeZcc));
		return rootJ;
	}

	void fromJson(json_t *rootJ) override {
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
			midiAcc = json_integer_value(midiAccJ);
		json_t *midiBccJ = json_object_get(rootJ, "midiBcc");
		if (midiBccJ)
			midiBcc = json_integer_value(midiBccJ);
		json_t *mpeYccJ = json_object_get(rootJ, "mpeYcc");
		if (mpeYccJ){
			mpeYcc = json_integer_value(mpeYccJ);
			if (polyModeIx > 0){
				displayYcc = 129;
			}else{
				displayYcc = mpeYcc;
			}
		}
		json_t *mpeZccJ = json_object_get(rootJ, "mpeZcc");
		if (mpeZccJ){
			mpeZcc = json_integer_value(mpeZccJ);
			if (polyModeIx > 0){
				displayZcc = 130;
			}else{
				displayZcc = mpeZcc;
			}
		}
	}
///////////////////////////ON RESET
	void onReset() override {
		for (int i = 0; i < numVo; i++) {
			notes[i] = 60;
			gates[i] = false;
			pedalgates[i] = false;
		}
		rotateIndex = -1;
		cachedNotes.clear();
		if (polyMode == MPE_MODE) {
			savedMidiCh = midiInput.channel;//save selected channel to reset
			(midiInput.channel = -1);
			for (int i = 0; i < 8; i++) {
				mpex[i] = 0;
				cachedMPE[i].clear();
			}
			displayYcc = mpeYcc;
			displayZcc = mpeZcc;
		} else {
			displayYcc = 129;
			displayZcc = 130;
		}
		resetLearncc();
	}
	void resetLearncc(){
		learnAcc = false;
		learnBcc = false;
		learnYcc = false;
		learnZcc = false;
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
					}
				}
			} break;

			case UNISON_MODE: {
				if (!cachedNotes.empty()) {
					uint8_t backnote = cachedNotes.back();
					for (int i = 0; i < numVo; i++) {
						notes[i] = backnote;
						gates[i] = true;
						mpey[i] = vel;
					}
				}
				else {
					for (int i = 0; i < numVo; i++) {
						gates[i] = false;
						mpey[i] = vel;
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
						mpey[i] = vel;
					}
				}
			} break;
		}
	}

	void pressPedal() {
		pedal = true;
		for (int i = 0; i < numVo; i++) {
			pedalgates[i] = gates[i];
		}
	}

	void releasePedal() {
		pedal = false;
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
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////
//////   STEP START
///////////////////////
	
	
	void step() override {

		MidiMessage msg;
		while (midiInput.shift(&msg)) {
			processMessage(msg);
		}

		float pbVo = 0.f;
		MpitFilter.lambda = 100.f * engineGetSampleTime();
		if (Mpit < 8192){
			pbVo = MpitFilter.process(rescale(Mpit, 0, 8192, -5.f, 0.f));
		} else {
			pbVo = MpitFilter.process(rescale(Mpit, 8192, 16383, 0.f, 5.f));
		}
		outputs[BEND_OUTPUT].value = pbVo;
		bool sustainHold = (params[SUSTAINHOLD_PARAM].value > .5 );

		if (polyMode > PolyMode::MPE_MODE){
			for (int i = 0; i < 8; i++) {
				float lastGate = ((gates[i] || (sustainHold && pedalgates[i])) && (!(reTrigger[i].process(engineGetSampleTime()))))? 10.f : 0.f;
				outputs[GATE_OUTPUT + i].value = lastGate;
				outputs[X_OUTPUT + i].value = ((notes[i] - 60) / 12.f) + (pbVo * static_cast<float>(pbMain) / 60.f);
				outputs[VEL_OUTPUT + i].value = rescale(vels[i], 0, 127, 0.f, 10.f);
				outputs[Y_OUTPUT + i].value = rescale(mpey[i], 0, 127, 0.f, 10.f);
				outputs[Z_OUTPUT + i].value = rescale(noteData[notes[i]].aftertouch, 0, 127, 0.f, 10.f);
				lights[CH_LIGHT + i].value = ((i == rotateIndex)? 0.2f : 0.f) + (lastGate * .08f);
			}
		} else {
			for (int i = 0; i < numVo; i++) {
				float lastGate = ((gates[i] || (sustainHold && pedalgates[i])) && (!(reTrigger[i].process(engineGetSampleTime())))) ? 10.f : 0.f;
				outputs[GATE_OUTPUT + i].value = lastGate ;
				if ( mpex[i] < 0){
					xpitch[i] = (MPExFilter[i].process(rescale(mpex[i], -8192 , 0, -5.f, 0.f))) * pbMPE / 60.f;
				} else {
					xpitch[i] = (MPExFilter[i].process(rescale(mpex[i], 0, 8191, 0.f, 5.f))) * pbMPE / 60.f;
				}
				outputs[X_OUTPUT + i].value = xpitch[i] + ((notes[i] - 60) / 12.f) + (pbVo * static_cast<float>(pbMain) / 60.f);
				outputs[VEL_OUTPUT + i].value = rescale(vels[i], 0, 127, 0.f, 10.f);
				outputs[Y_OUTPUT + i].value = MPEyFilter[i].process(rescale(mpey[i], 0, 127, 0.f, 10.f));
				outputs[Z_OUTPUT + i].value = MPEzFilter[i].process(rescale(mpez[i], 0, 127, 0.f, 10.f));
				lights[CH_LIGHT + i].value = ((i == rotateIndex)? 0.2f : 0.f) + (lastGate * .08f);
			}
		}
		MmodFilter.lambda = 100.f * engineGetSampleTime();
		outputs[MOD_OUTPUT].value = MmodFilter.process(rescale(Mmod, 0, 127, 0.f, 10.f));

		MaftFilter.lambda = 100.f * engineGetSampleTime();
		outputs[AFT_OUTPUT].value = MaftFilter.process(rescale(Maft, 0, 127, 0.f, 10.f));

		MccaFilter.lambda = 100.f * engineGetSampleTime();
		outputs[CCA_OUTPUT].value = MccaFilter.process(rescale(Mcca, 0, 127, 0.f, 10.f));

		MccbFilter.lambda = 100.f * engineGetSampleTime();
		outputs[CCB_OUTPUT].value = MccbFilter.process(rescale(Mccb, 0, 127, 0.f, 10.f));

		MsusFilter.lambda = 100.f * engineGetSampleTime();
		outputs[SUS_OUTPUT].value = MsusFilter.process(rescale(Msus, 0, 127, 0.f, 10.f));

		//// PANEL KNOB AND BUTTONS
		float f_dataKnob = *dataKnob;
		if ( f_dataKnob > 0.07f){
			int knobInterval = static_cast<int>(0.05 * static_cast<float>(engineGetSampleRate()) / f_dataKnob);
			if (frameData ++ > knobInterval){
				frameData = 0;
				dataPlus();
			}
		}else if(f_dataKnob < -0.07f){
			int knobInterval = static_cast<int>(0.05 * static_cast<float>(engineGetSampleRate()) / -f_dataKnob);
			if (frameData ++ > knobInterval){
				frameData = 0;
				dataMinus();
			}
		}
		
		if (PlusOneTrigger.process(params[PLUSONE_PARAM].value)) {
			dataPlus();
			return;
		}
		if (MinusOneTrigger.process(params[MINUSONE_PARAM].value)) {
			dataMinus();
			return;
		}
		if (LcursorTrigger.process(params[LCURSOR_PARAM].value)) {
			if (polyMode == MPE_MODE){
				if (cursorI > 0) cursorI --;
				else cursorI = 7;
				cursorIx = cursorMPE[cursorI];
			}else{
				if (cursorI > 0) cursorI --;
				else cursorI = 4;
				cursorIx = cursorPoly[cursorI];
			}
			resetLearncc();
			return;
		}
		if (RcursorTrigger.process(params[RCURSOR_PARAM].value)) {
			if (polyMode == MPE_MODE){
				if (cursorI < 7) cursorI ++;
				else cursorI = 0;
				cursorIx = cursorMPE[cursorI];
			}else{
				if (cursorI < 4) cursorI ++;
				else cursorI = 0;
				cursorIx = cursorPoly[cursorI];
			}
			resetLearncc();
			return;
		}

		if (learnCCATrigger.process(params[LEARNCCA_PARAM].value)) {
			learnAcc = !learnAcc;
			learnBcc = false;
			learnYcc = false;
			learnZcc = false;
			return;
		}
		if (learnCCBTrigger.process(params[LEARNCCB_PARAM].value)) {
			learnBcc = !learnBcc;
			learnAcc = false;
			learnYcc = false;
			learnZcc = false;
			return;
		}
		if (learnCCYTrigger.process(params[LEARNCCY_PARAM].value)) {
			if (polyMode == MPE_MODE){
				learnYcc = !learnYcc;
				learnBcc = false;
				learnAcc = false;
				learnZcc = false;
				return;
			}
		}
		if (learnCCZTrigger.process(params[LEARNCCZ_PARAM].value)) {
			if (polyMode == MPE_MODE){
				learnZcc = !learnZcc;
				learnBcc = false;
				learnAcc = false;
				learnYcc = false;
				return;
			}
		}
		
		///// RESET MIDI
		if (resetMidiTrigger.process(params[RESETMIDI_PARAM].value)) {
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

	
	void dataPlus(){
		switch (cursorIx){
			case 0: {
				bool wasMPE = (polyModeIx == 0);
				if (polyMode < UNISON_MODE) polyMode = (PolyMode) (polyMode + 1);
				else polyMode = MPE_MODE;
				onReset();
				polyModeIx = polyMode;
				if ((wasMPE) && (polyModeIx > 0 ))
					midiInput.channel = savedMidiCh;//restore channel

			}break;
			case 1: {
				if (numVo < 8) numVo ++;
				else numVo = 2;
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
				if (mpeYcc <128) {
					mpeYcc ++;
					displayYcc = mpeYcc;
				}
			}break;
			case 6: {
				if (mpeZcc <128) {
					mpeZcc ++;
					displayZcc = mpeZcc;
				}
			}break;
			case 7: {
				if (midiAcc < 127) midiAcc ++;
			}break;
			case 8: {
				if (midiBcc < 127) midiBcc ++;
			}break;
		}
		resetLearncc();
		return;
	}
	void dataMinus(){
		switch (cursorIx){
			case 0: {
				bool wasMPE = (polyModeIx == 0);
				if (polyMode > MPE_MODE) polyMode = (PolyMode) (polyMode - 1);
				else polyMode = UNISON_MODE;
				onReset();
				polyModeIx = polyMode;
				if ((wasMPE) && (polyModeIx > 0 ))
					midiInput.channel = savedMidiCh;//restore channel
			}break;
			case 1: {
				if (numVo > 2) numVo --;
				else numVo = 8;
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
				if (mpeYcc > 0) {
					mpeYcc --;
					displayYcc = mpeYcc;
				}
			}break;
			case 6: {
				if (mpeZcc > 0) {
					mpeZcc --;
					displayZcc = mpeZcc;
				}
			}break;
			case 7: {
				if (midiAcc > 0) midiAcc --;
			}break;
			case 8: {
				if (midiBcc > 0) midiBcc --;
			}break;
		}
	
		resetLearncc();
		return;
	}
	void processMessage(MidiMessage msg) {


		switch (msg.status()) {
			// note off
			case 0x8: {
				if ((polyMode == MPE_MODE) && (msg.channel() == MPEmasterCh)) return;
				releaseNote(msg.channel(), msg.note(), msg.value());
			} break;
			// note on
			case 0x9: {
				if ((polyMode == MPE_MODE) && (msg.channel() == MPEmasterCh)) return;
				if (msg.value() > 0) {
					//noteData[msg.note()].velocity = msg.value();
					pressNote(msg.channel(), msg.note(), msg.value());
				}
				else {
					releaseNote(msg.channel(), msg.note(), 64);
				}
			} break;
			// note (poly) aftertouch
			case 0xa: {
				if (polyMode == MPE_MODE) return;
				noteData[msg.note()].aftertouch = msg.value();
			} break;
				
			// channel aftertouch
			case 0xd: {
				if (polyMode == MPE_MODE){
					if (msg.channel() == MPEmasterCh){
						Maft = msg.data1;
					}else if (learnYcc) {
						mpeYcc = 128;
						displayYcc = mpeYcc;
						learnYcc = false;
						return;
					}else if (learnZcc) {
						mpeZcc = 128;
						displayZcc = mpeZcc;
						learnZcc = false;
						return;
					}else {
						if (mpeZcc == 128)
							mpez[msg.channel() - MPEfirstCh] = msg.data1;
						if (mpeYcc == 128)
							mpey[msg.channel() - MPEfirstCh] = msg.data1;
						}
				}else{
					Maft = msg.data1;
				}
			} break;
				// pitch wheel
			case 0xe:{
				if (polyMode == MPE_MODE){
					if (msg.channel() == MPEmasterCh){
						Mpit = msg.data2 * 128 + msg.data1;
					}else{
						mpex[msg.channel() - MPEfirstCh] = msg.data2 * 128 + msg.data1 - 8192;
					}
				}else{
					Mpit = msg.data2 * 128 + msg.data1;
				}
			} break;
			// cc
			case 0xb: {
				if (polyMode == MPE_MODE){
					if (msg.channel() == MPEmasterCh){
						processCC(msg);
					}else if (learnYcc) {
						mpeYcc = msg.note();
						displayYcc = mpeYcc;
						learnYcc = false;
						return;
					}else if (learnZcc) {
						mpeZcc = msg.note();
						displayZcc = mpeZcc;
						learnZcc = false;
						return;
					}else if (msg.note() == mpeYcc){
					//cc74 0x4a default
						mpey[msg.channel() - MPEfirstCh] = msg.data2;
					}else if (msg.note() == mpeZcc){
						mpez[msg.channel() - MPEfirstCh] = msg.data2;
					}
				}else{
				  processCC(msg);
				}
			} break;
			default: break;
		}
	}

	void processCC(MidiMessage msg) {

		switch (msg.note()) {
			case 0x01: // mod
				Mmod = msg.data2;
				break;
//			case 0x07: // vol
//				Mvol = msg.data2;
//				break;
//			case 0x0B: // Expression
//				Mexp = msg.data2;
//				break;
			// sustain
			case 0x40: {
				Msus = msg.data2;
				if (msg.value() >= 64)
					pressPedal();
				else
					releasePedal();
			} break;
			default: {
				///////// LEARN CC   ???
				if (learnAcc) {
					midiAcc = msg.note();
					learnAcc = false;
					//lights[LEARNCCA_LIGHT].value = 0.f;
					return;
				}else if (learnBcc) {
					midiBcc = msg.note();
					learnBcc = false;
					//lights[LEARNCCB_LIGHT].value = 0.f;
					return;
				}
				/////////////////////////
				
				if (msg.note() == midiAcc){
					Mcca = msg.data2;
				}else if (msg.note() == midiBcc){
					Mccb = msg.data2;
				}
			} break;
		}
	}
	void MidiPanic() {
		onReset();
		Mpit = 8192;
		Mmod = 0;
		Maft = 0;
		Msus = 0;
		Mcca = 0;
		Mccb = 0;
		pedal = false;
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
			outputs[i].value = 0.f;
		}
	}
};
// Main Display
struct PolyModeDisplay : TransparentWidget {
	PolyModeDisplay(){
		font = Font::load(mFONT_FILE);
	}
	float mdfontSize = 12.f;
	std::string sMode ="";
	std::string sVo ="";
	std::string sPBM ="";
	std::string sPBMPE ="";
	std::string sMPEmidiCh = "";
	std::shared_ptr<Font> font;
	std::string polyModeStr[6] = {
		"M. P. E.",
		"C Y C L E",
		"R E U S E",
		"R E S E T",
		"R E A S S I G N",
		"U N I S O N",
	};
	int *p_polyMode;
	int polyModeI = -1;
	int *p_numVo;
	int numVoI = -1;
	int *p_pbMain;
	int pbMainI = -1;
	int *p_pbMPE;
	int pbMPEI = -1;
	int *p_MPEmasterCh;
	int MPEmasterChI = -1;
	int *p_MPEfirstCh;
	int MPEfirstChI = -1;
	int *p_cursorIx;
	int cursorIxI = 0;
	int flashFocus = 0;
	void draw(NVGcontext* vg) {
		
		if (polyModeI !=  *p_polyMode) {
			polyModeI = *p_polyMode;
			sMode = polyModeStr[polyModeI];
			
		}
		if (numVoI != *p_numVo){
			numVoI = *p_numVo;
			sVo = "Poly "+ std::to_string(numVoI) +" Vo outs";
			
		}
		if (pbMainI != *p_pbMain){
			pbMainI = *p_pbMain;
			sPBM = "PBend:" + std::to_string(pbMainI);
			
		}
		if (pbMPEI != *p_pbMPE){
			pbMPEI = *p_pbMPE;
			sPBMPE = " CH PBend:" + std::to_string(pbMPEI);
			
		}

		if  ((MPEmasterChI != *p_MPEmasterCh) || (MPEfirstChI != *p_MPEfirstCh)){
			MPEmasterChI = *p_MPEmasterCh;
			MPEfirstChI = *p_MPEfirstCh;
			sMPEmidiCh = "channels M:" + std::to_string(MPEmasterChI + 1) + " Vo:" + std::to_string(MPEfirstChI + 1) + "++";
			
		}
		
		nvgFontSize(vg, mdfontSize);
		nvgFontFaceId(vg, font->handle);
		nvgFillColor(vg, nvgRGB(0xcc, 0xcc, 0xcc));//Text

		//nvgGlobalCompositeOperation(vg, NVG_SOURCE_OUT);
		nvgTextAlign(vg, NVG_ALIGN_CENTER);
		nvgTextBox(vg, 4.f, 11.0f,124.f, sMode.c_str(), NULL);
		
		if (polyModeI < 1){
			nvgTextBox(vg, 4.f, 24.0f,124.f, sMPEmidiCh.c_str(), NULL);// MPE Channels
			nvgTextAlign(vg, NVG_ALIGN_LEFT);
			nvgTextBox(vg, 58.f, 37.0f,66.f, sPBMPE.c_str(), NULL);//MPE PitchBend
		} else {
			nvgTextBox(vg, 4.f, 24.0f,124.f, sVo.c_str(), NULL);
		}
		
		nvgTextAlign(vg, NVG_ALIGN_LEFT);
		nvgTextBox(vg, 4.f, 37.0f, 50.f, sPBM.c_str(), NULL);
		
		if (cursorIxI != *p_cursorIx){
			cursorIxI = *p_cursorIx;
			flashFocus = 64;
		}
		
		nvgGlobalCompositeBlendFunc(vg,  NVG_ONE , NVG_ONE);
		
		nvgBeginPath(vg);
		switch (cursorIxI){
			case 0:{ // PolyMode
				nvgRoundedRect(vg, 2.f, 1.f, 128.f, 12.f, 1.f);
			}break;
			case 1:{ //numVoices Poly
				nvgRoundedRect(vg, 2.f, 14.f, 128.f, 12.f, 1.f);
			}break;
			case 2:{ //MPE channels
				nvgRoundedRect(vg, 2.f, 14.f, 128.f, 12.f, 1.f);
			}break;
			case 3:{//mainPB
				nvgRoundedRect(vg, 2.f, 27.f, 52.f, 12.f, 1.f);
			}break;
			case 4:{//mpePB
				nvgRoundedRect(vg, 52.f, 27.f, 76.f, 12.f, 1.f);
			}break;
		}

		if (flashFocus > 0)
			flashFocus -= 2;
		int rgbint = 0x44 + flashFocus;
		nvgFillColor(vg, nvgRGB(rgbint, rgbint, rgbint)); //SELECTED
//		nvgStrokeColor(vg, nvgRGB(0x66, 0x66, 0x66));
//		nvgStroke(vg);
		
		nvgFill(vg);
		
	}
};

struct MidiccDisplay : TransparentWidget {
	MidiccDisplay(){
		font = Font::load(mFONT_FILE);
	}
	float mdfontSize = 12.f;
	std::string sDisplay = "";
	int *p_cursor;
	int cursorI = -1;
	int displayID = 0;//set on each instance
	int *p_ccNumber;
	int ccNumber = -1;
	bool *p_learnOn;
	bool learnOn = false;
	int flashFocus = 0;
	int screenFrames = 0;
	std::shared_ptr<Font> font;
	void draw(NVGcontext* vg) {
		
		if  ((ccNumber != *p_ccNumber)||(learnOn != *p_learnOn)){
			ccNumber = *p_ccNumber;
			learnOn = *p_learnOn;
			if (learnOn)
				sDisplay = "LRN";
			else if (ccNumber < 128)
				sDisplay = "cc" + std::to_string(ccNumber);
			else if (ccNumber < 129)
				sDisplay = "chnAT";
			else if (ccNumber < 130) //(locked)  Rel Vel
				sDisplay = "rVel";
			else //(locked)  note AfterT
				sDisplay = "nteAT";
		}

		if (learnOn) {
			nvgBeginPath(vg);
			nvgRoundedRect(vg, 0.f, 0.f, 29.5f, 13.f,3.f);
			nvgStrokeColor(vg, nvgRGB(0xdd, 0x0, 0x0));
			nvgStroke(vg);
			nvgRoundedRect(vg, 0.f, 0.f, 29.5f, 13.f,3.f);

			nvgFillColor(vg, nvgRGBA(0xcc, 0x0, 0x0,0x44));
			nvgFill(vg);
			///text color
			nvgFillColor(vg, nvgRGB(0xff, 0x00, 0x00));//LEARN
		}else{
			///text color
			nvgFillColor(vg, nvgRGB(0xcc, 0xcc, 0xcc));//Normal
		}
		nvgFontSize(vg, mdfontSize);
		nvgFontFaceId(vg, font->handle);
		nvgTextAlign(vg, NVG_ALIGN_CENTER);
		nvgTextBox(vg, 0.f, 10.f,29.5f, sDisplay.c_str(), NULL);

		if (cursorI != *p_cursor){
			cursorI = *p_cursor;
			if (*p_cursor == displayID)
				flashFocus = 64;
		}
		if ((displayID == cursorI) && (!learnOn)){
			nvgGlobalCompositeBlendFunc(vg,  NVG_ONE , NVG_ONE);
			nvgBeginPath(vg);
			nvgRoundedRect(vg, 0.f, 0.f, 29.5f, 13.f,3.f);
//			nvgStrokeColor(vg, nvgRGB(0x66, 0x66, 0x66));
//			nvgStroke(vg);
			if (flashFocus > 0)
				flashFocus -= 2;
			int rgbint = 0x44 + flashFocus;
			nvgFillColor(vg, nvgRGB(rgbint, rgbint, rgbint)); //SELECTED
			nvgFill(vg);
		}
	}
};


struct BlockChannel : OpaqueWidget {
	int *p_polyMode;
	void draw(NVGcontext* vg) {
		if ( *p_polyMode > 0) {
				box.size = Vec(0.f,0.f);
			}else{
				box.size = Vec(94.f,13.f);
				NVGcolor ledColor = nvgRGBA(0x00, 0x00, 0x00,0xaa);
				nvgBeginPath(vg);
				nvgRoundedRect(vg, 0.f, 0.f, 94.f, 13.f,3.f);
				nvgFillColor(vg, ledColor);
				nvgFill(vg);
			}
	}
};

///MIDIlearnMCC
struct learnMccButton : SVGSwitch, MomentarySwitch {
	learnMccButton() {
		//     box.size = Vec(29.5, 13);
		addFrame(SVG::load(assetPlugin(plugin, "res/learnMcc_0.svg")));
		addFrame(SVG::load(assetPlugin(plugin, "res/learnMcc_1.svg")));
	}
};


struct springDataKnob : SVGKnob {
		int *p_frameData;

	springDataKnob() {
		minAngle = -0.75*M_PI;
		maxAngle = 0.75*M_PI;
		setSVG(SVG::load(assetPlugin(plugin, "res/dataKnob.svg")));
		shadow->opacity = 0.f;
	}
		void onMouseUp(EventMouseUp &e){
			this->value = 0.f;
			*p_frameData = 100000; //reset frame Counter to start (over sampleRate counter)
		}
};



struct MIDI8MPEWidget : ModuleWidget {
	MIDI8MPEWidget(MIDI8MPE *module) : ModuleWidget(module) {
		setPanel(SVG::load(assetPlugin(plugin,"res/MIDI8MPE.svg")));
		//Screws
		addChild(Widget::create<ScrewBlack>(Vec(0, 0)));
		addChild(Widget::create<ScrewBlack>(Vec(180, 0)));
		addChild(Widget::create<ScrewBlack>(Vec(0, 365)));
		addChild(Widget::create<ScrewBlack>(Vec(180, 365)));
		
		float xPos = 8.f;//61;
		float yPos = 18.f;
		{
			MidiWidget *midiWidget = Widget::create<MidiWidget>(Vec(xPos,yPos));
			midiWidget->box.size = Vec(132.f,41.f);
			midiWidget->midiIO = &module->midiInput;
			
			midiWidget->driverChoice->box.size.y = 12.f;
			midiWidget->deviceChoice->box.size.y = 12.f;
			midiWidget->channelChoice->box.size.y = 12.f;

			midiWidget->driverChoice->box.pos = Vec(0.f, 2.f);
			midiWidget->deviceChoice->box.pos = Vec(0.f, 15.f);
			midiWidget->channelChoice->box.pos = Vec(0.f, 28.f);

			midiWidget->driverSeparator->box.pos = Vec(0.f, 15.f);
			midiWidget->deviceSeparator->box.pos = Vec(0.f, 28.f);

			midiWidget->driverChoice->font = Font::load(mFONT_FILE);
			midiWidget->deviceChoice->font = Font::load(mFONT_FILE);
			midiWidget->channelChoice->font = Font::load(mFONT_FILE);

			midiWidget->driverChoice->textOffset = Vec(2.f,10.f);
			midiWidget->deviceChoice->textOffset = Vec(2.f,10.f);
			midiWidget->channelChoice->textOffset = Vec(2.f,10.f);
			
			midiWidget->driverChoice->color = nvgRGB(0xdd, 0xdd, 0xdd);
			midiWidget->deviceChoice->color = nvgRGB(0xdd, 0xdd, 0xdd);
			midiWidget->channelChoice->color = nvgRGB(0xdd, 0xdd, 0xdd);
			addChild(midiWidget);
		}
		BlockChannel *blockChannel = Widget::create<BlockChannel>(Vec(8.f,46.f));
		blockChannel->p_polyMode = &(module->polyModeIx);
		addChild(blockChannel);
		
		xPos = 102.f;
		yPos = 47.f;
		addParam(ParamWidget::create<moDllzMidiPanic>(Vec(xPos, yPos), module, MIDI8MPE::RESETMIDI_PARAM, 0.0f, 1.0f, 0.0f));
		addChild(ModuleLightWidget::create<SmallLight<RedLight>>(Vec(xPos+3.f, yPos+3.f), module, MIDI8MPE::RESETMIDI_LIGHT));
		
		xPos = 8.f;
		yPos = 62.f;
		
		{
			PolyModeDisplay *polyModeDisplay = new PolyModeDisplay();
			polyModeDisplay->box.pos = Vec(xPos, yPos);
			polyModeDisplay->box.size = {132.f, 40.f};
			polyModeDisplay->p_polyMode = &(module->polyModeIx);
			polyModeDisplay->p_numVo = &(module->numVo);
			polyModeDisplay->p_pbMain = &(module->pbMain);
			polyModeDisplay->p_pbMPE = &(module->pbMPE);
			polyModeDisplay->p_MPEmasterCh = &(module->MPEmasterCh);
			polyModeDisplay->p_MPEfirstCh = &(module->MPEfirstCh);
			polyModeDisplay->p_cursorIx = &(module->cursorIx);
			addChild(polyModeDisplay);
		}
		
		yPos = 20.f;
		xPos = 145.f;
		addParam(ParamWidget::create<minusButton>(Vec(xPos, yPos), module, MIDI8MPE::MINUSONE_PARAM, 0.0f, 1.0f, 0.0f));
		xPos = 169.f;
		addParam(ParamWidget::create<plusButton>(Vec(xPos, yPos), module, MIDI8MPE::PLUSONE_PARAM, 0.0f, 1.0f, 0.0f));

		
		xPos = 147.f;
		yPos = 40.f;
////DATA KNOB
		
		{   springDataKnob *sDataKnob = new springDataKnob();
			sDataKnob->box.pos = Vec(xPos, yPos);
			sDataKnob->box.size = {36.f, 36.f};
			sDataKnob->minValue = -1.f;
			sDataKnob->maxValue = 1.f;
			sDataKnob->defaultValue = 0.f;
			sDataKnob->p_frameData = &(module->frameData);
			module->dataKnob = &(sDataKnob->value);
			addChild(sDataKnob);
		}
		
		yPos = 85.f;
		xPos = 145.5f;
		addParam(ParamWidget::create<moDllzcursorL>(Vec(xPos, yPos), module, MIDI8MPE::LCURSOR_PARAM, 0.0f, 1.0f, 0.0f));
		xPos = 165.5f;
		addParam(ParamWidget::create<moDllzcursorR>(Vec(xPos, yPos), module, MIDI8MPE::RCURSOR_PARAM, 0.0f, 1.0f, 0.0f));

		yPos = 104.f;
		xPos = 59.f;
		{
			MidiccDisplay *mpeYDisplay = new MidiccDisplay();
			mpeYDisplay->box.pos = Vec(xPos, yPos);
			mpeYDisplay->box.size = {29.5f, 13.f};
			mpeYDisplay->displayID = 5;
			mpeYDisplay->p_cursor = &(module->cursorIx);
			mpeYDisplay->p_ccNumber = &(module->displayYcc);
			mpeYDisplay->p_learnOn = &(module->learnYcc);
			addChild(mpeYDisplay);
		}
		
		addParam(ParamWidget::create<learnMccButton>(Vec(xPos, yPos), module, MIDI8MPE::LEARNCCY_PARAM, 0.0, 1.0, 0.0));
		xPos = 89.5f;
		{
			MidiccDisplay *mpeZDisplay = new MidiccDisplay();
			mpeZDisplay->box.pos = Vec(xPos, yPos);
			mpeZDisplay->box.size = {29.5f, 13.f};
			mpeZDisplay->displayID = 6;
			mpeZDisplay->p_cursor = &(module->cursorIx);
			mpeZDisplay->p_ccNumber = &(module->displayZcc);
			mpeZDisplay->p_learnOn = &(module->learnZcc);
			addChild(mpeZDisplay);
		}
		addParam(ParamWidget::create<learnMccButton>(Vec(xPos, yPos), module, MIDI8MPE::LEARNCCZ_PARAM, 0.0, 1.0, 0.0));
		
		
		
		yPos = 118.f;
		float const xOffset = 31.f;
		for ( int i = 0; i < 8; i++){
			xPos = 30.f;
			addChild(ModuleLightWidget::create<TinyLight<RedLight>>(Vec(xPos-7.f, yPos+10.f), module, MIDI8MPE::CH_LIGHT + i));
			addOutput(Port::create<moDllzPortDark>(Vec(xPos, yPos), Port::OUTPUT, module, MIDI8MPE::X_OUTPUT + i));
			xPos += xOffset;
			addOutput(Port::create<moDllzPortDark>(Vec(xPos, yPos), Port::OUTPUT, module, MIDI8MPE::Y_OUTPUT + i));
			xPos += xOffset;
			addOutput(Port::create<moDllzPortDark>(Vec(xPos, yPos), Port::OUTPUT, module, MIDI8MPE::Z_OUTPUT + i));
			xPos += xOffset;
			addOutput(Port::create<moDllzPortDark>(Vec(xPos + 3.f, yPos), Port::OUTPUT, module, MIDI8MPE::VEL_OUTPUT + i));
			xPos += xOffset;
			addOutput(Port::create<moDllzPortDark>(Vec(xPos + 3.f, yPos), Port::OUTPUT, module, MIDI8MPE::GATE_OUTPUT + i));
			yPos += 25.f;
		}
		yPos = 336.f;
		xPos = 13.f;

		addOutput(Port::create<moDllzPort>(Vec(xPos, yPos), Port::OUTPUT, module, MIDI8MPE::BEND_OUTPUT));
		xPos += 26.f;
		addOutput(Port::create<moDllzPort>(Vec(xPos, yPos), Port::OUTPUT, module, MIDI8MPE::MOD_OUTPUT));
		xPos += 26.f;
		addOutput(Port::create<moDllzPort>(Vec(xPos, yPos), Port::OUTPUT, module, MIDI8MPE::AFT_OUTPUT));
		
		xPos = 90.f;
		addOutput(Port::create<moDllzPort>(Vec(xPos, yPos), Port::OUTPUT, module, MIDI8MPE::CCA_OUTPUT));
		xPos = 121.f;
		addOutput(Port::create<moDllzPort>(Vec(xPos, yPos), Port::OUTPUT, module, MIDI8MPE::CCB_OUTPUT));
		xPos = 148.f;
		addOutput(Port::create<moDllzPort>(Vec(xPos, yPos), Port::OUTPUT, module, MIDI8MPE::SUS_OUTPUT));
		
		///Sustain hold notes
		xPos = 173.f;
		yPos = 338.f;
		addParam(ParamWidget::create<moDllzSwitchLed>(Vec(xPos, yPos), module, MIDI8MPE::SUSTAINHOLD_PARAM, 0.0, 1.0, 1.0));
		
		yPos = 322.f;
		xPos = 87.f;
		{
			MidiccDisplay *MccADisplay = new MidiccDisplay();
			MccADisplay->box.pos = Vec(xPos, yPos);
			MccADisplay->box.size = {29.5f, 13.f};
			MccADisplay->displayID = 7;
			MccADisplay->p_cursor = &(module->cursorIx);
			MccADisplay->p_ccNumber = &(module->midiAcc);
			MccADisplay->p_learnOn = &(module->learnAcc);
			addChild(MccADisplay);
		}
		
		addParam(ParamWidget::create<learnMccButton>(Vec(xPos, yPos), module, MIDI8MPE::LEARNCCA_PARAM, 0.0, 1.0, 0.0));
		xPos = 117.5f;
		{
			MidiccDisplay *MccBDisplay = new MidiccDisplay();
			MccBDisplay->box.pos = Vec(xPos, yPos);
			MccBDisplay->box.size = {29.5f, 13.f};
			MccBDisplay->displayID = 8;
			MccBDisplay->p_cursor = &(module->cursorIx);
			MccBDisplay->p_ccNumber = &(module->midiBcc);
			MccBDisplay->p_learnOn = &(module->learnBcc);
			addChild(MccBDisplay);
		}
		addParam(ParamWidget::create<learnMccButton>(Vec(xPos, yPos), module, MIDI8MPE::LEARNCCB_PARAM, 0.0, 1.0, 0.0));
//		{
//		        testDisplay *mDisplay = new testDisplay();
//		        mDisplay->box.pos = Vec(0.0f, 360.0f);
//		        mDisplay->box.size = {165.0f, 20.0f};
//		        mDisplay->valP = module->dataKnob;
//		        addChild(mDisplay);
//		    }
	}
};


Model *modelMIDI8MPE = Model::create<MIDI8MPE, MIDI8MPEWidget>("moDllz", "MIDI8MPE", "MIDI 8cv MPE", MIDI_TAG, EXTERNAL_TAG, MULTIPLE_TAG);

