/*
 MIDI8MPE : Midi to 16ch CV with MPE and regular Polyphonic modes
 
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

//#include "moDllz.hpp"
#include "moDllzComp.hpp"

struct MIDIpolyMPE : Module {
	
	float TEST_FLOAT = 0.f;
	
	enum ParamIds {
		MINUSONE_PARAM,
		PLUSONE_PARAM,
		SUSTHOLD_PARAM,
		RETRIG_PARAM,
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
		ENUMS(MM_OUTPUT, 8),
		NUM_OUTPUTS
	};
	enum LightIds {
		RESETMIDI_LIGHT,
		ENUMS(CH_LIGHT, 16),
		SUSTHOLD_LIGHT,
		NUM_LIGHTS
	};
	//////MIDI
	midi::InputQueue midiInput;
	
	int MPEmasterCh = 0;// 0 ~ 15
	unsigned char midiActivity = 0;
	bool resetMidi = false;
	int mdriverJx = -1;
	int mchannelJx = -1;
	std::string mdeviceJx = "";
	/////
	enum PolyMode {
		MPE_MODE,
		MPEPLUS_MODE,
		ROTATE_MODE,
		REUSE_MODE,
		RESET_MODE,
		RESTACK_MODE,
		UNISON_MODE,
		UNISONLWR_MODE,
		UNISONUPR_MODE,
		NUM_POLYMODES
	};
	enum paramsIx {
		_noSelection,
		polyModeId,
		numVoCh,
		noteMin,
		noteMax,
		velMin,
		velMax,
		RNDetune,
		mpeYcc,
		mpeZcc,
		mpeRelPb,
		pbMPE,
		transpose,
		pbDwn,
		pbUp,
		ENUMS(midiCCs,8),
		NumParamsIx
	};
	int paramsMap [paramsIx::NumParamsIx];
	struct NoteData {
		uint8_t velocity = 0;
		uint8_t aftertouch = 0;
	};
	NoteData noteData[128];
	std::vector<uint8_t> cachedNotes;// Stolen notes (UNISON_MODE and RESTACK_MODE cache all played)
	std::vector<uint8_t> cachedMPE[16];// MPE stolen notes
	uint8_t notes[16] = {0};
	uint8_t vels[16] = {0};
	uint8_t rvels[16] = {0};
	int16_t mpex[16] = {0};
	uint16_t mpey[16] = {0};
	uint16_t mpez[16] = {0};
	uint8_t mpePlusLB[16] = {0};
	int16_t mrPBend = 0;
	uint8_t midiCCsValues[129] = {0};
	
	bool gates[16] = {false};
	bool hold[16] = {false};
	float xpitch[16] = {0.f};
	float drift[16] = {0.f};
	bool pedal = false;
	int rotateIndex = 0;
	int stealIndex = 0;
	int nVoChMPE = 16;
	int learnId = 0;
	int cursorIx = 0;
	int selectedmidich = 0;
	bool MPEmode = false;
	bool Ycanedit = true;
	bool Zcanedit = true;
	bool RPcanedit = false;
	int ProcessFrame = 0;
	int onFocus = 0;
	bool updateDataKnob = false;
	///pointers
	int Yhaken = 131;
	int Zhaken = 132;
	int NOTEAFT = 129;
	int Detune130 = 130;
	int one = 1;
	int zero = 0;
	int *Z_ptr = &NOTEAFT;
	int *Y_ptr = &Yhaken;
	int *RP_ptr = &zero;
	
	int PROCESS_RATE = static_cast<int>(APP->engine->getSampleRate() * 0.0005); //.5ms
	int Focus_SEC =  10 * 2000;///^^^ sec * 2000 (process rate .5 ms)
	
	dsp::ExponentialFilter MPExFilter[16];
	dsp::ExponentialFilter MPEyFilter[16];
	dsp::ExponentialFilter MPEzFilter[16];
	dsp::ExponentialFilter MCCsFilter[8];
	dsp::ExponentialFilter mrPBendFilter;
	dsp::PulseGenerator reTrigger[16];	// retrigger for stolen notes
	dsp::SchmittTrigger PlusOneTrigger;
	dsp::SchmittTrigger MinusOneTrigger;
	
	std::string ccLongNames[133];
	MIDIpolyMPE() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configButton(PLUSONE_PARAM,"(Selected) +1");
		configButton(MINUSONE_PARAM,"(Selected) -1");
		configSwitch(SUSTHOLD_PARAM, false, true, false,"cc64 Sustain hold",{"off","ON"});
		configSwitch(RETRIG_PARAM, 0.f, 2.f, 1.f,"Retrigger",{"FirstOn","NotesOn","NotesOn+Recovered"});
		configParam(DATAKNOB_PARAM, 999.9f, 1000.1f, 1000.f,"Data Entry");
		configOutput(X_OUTPUT, "1V/oct pitch");
		configOutput(VEL_OUTPUT, "0~10V Velocity");
		configOutput(GATE_OUTPUT, "10V Gate");
		configOutput(PBEND_OUTPUT, "Master Pitch Bend");
		
		///////////////////////////////////////
		for (int i = 0 ; i < 128; i++){
			ccLongNames[i].assign("cc" + std::to_string(i));
		}
		ccLongNames[1].assign("cc1 Modulation Wheel");
		ccLongNames[2].assign("cc2 Breath Controller");
		ccLongNames[7].assign("cc7 Volume");
		ccLongNames[10].assign("cc10 Panning");
		ccLongNames[11].assign("cc11 Expression Pedal");
		ccLongNames[64].assign("cc64 Sustain Pedal");
		ccLongNames[128].assign("Channel Aftertouch");
		ccLongNames[129].assign("Note Aftertouch");
		ccLongNames[130].assign("Detuned");
		ccLongNames[131].assign("Y Axis 14bit");
		ccLongNames[132].assign("Z Axis 14bit");
		
		for (int c = 0; c < 16; c++) {
			MPExFilter[c].setTau(1 / 30.f);
			MPEyFilter[16].setTau(1 / 30.f);
			MPEzFilter[16].setTau(1 / 30.f);
		}
		for (int c = 0; c < 8; c++) {
			MCCsFilter[c].setTau(1 / 30.f);
		}
		mrPBendFilter.setTau(1/30.f);
		onReset();
	}
	///////////////////////////////////////////////////////////////////////////////////////
	json_t* miditoJson() {//saves last valid driver/device/chn
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "driver", json_integer(mdriverJx));
		json_object_set_new(rootJ, "deviceName", json_string(mdeviceJx.c_str()));
		json_object_set_new(rootJ, "channel", json_integer(mchannelJx));
		return rootJ;
	}
	///////////////////////////////////////////////////////////////////////////////////////
	
	json_t *dataToJson() override {
		json_t *rootJ = json_object();
		json_object_set_new(rootJ, "midi", miditoJson());
		//json_object_set_new(rootJ, "midi", midiInput.toJson());
		json_object_set_new(rootJ, "polyModeId", json_integer(paramsMap[polyModeId]));
		json_object_set_new(rootJ, "pbDwn", json_integer(paramsMap[pbDwn]));
		json_object_set_new(rootJ, "pbUp", json_integer(paramsMap[pbUp]));
		json_object_set_new(rootJ, "pbMPE", json_integer(paramsMap[pbMPE]));
		json_object_set_new(rootJ, "mpeRelPb", json_integer(paramsMap[mpeRelPb]));
		json_object_set_new(rootJ, "numVoCh", json_integer(paramsMap[numVoCh]));
		json_object_set_new(rootJ, "MPEmasterCh", json_integer(MPEmasterCh));
		json_object_set_new(rootJ, "midiAcc", json_integer(paramsMap[midiCCs + 0]));
		json_object_set_new(rootJ, "midiBcc", json_integer(paramsMap[midiCCs + 1]));
		json_object_set_new(rootJ, "midiCcc", json_integer(paramsMap[midiCCs + 2]));
		json_object_set_new(rootJ, "midiDcc", json_integer(paramsMap[midiCCs + 3]));
		json_object_set_new(rootJ, "midiEcc", json_integer(paramsMap[midiCCs + 4]));
		json_object_set_new(rootJ, "midiFcc", json_integer(paramsMap[midiCCs + 5]));
		json_object_set_new(rootJ, "midiGcc", json_integer(paramsMap[midiCCs + 6]));
		json_object_set_new(rootJ, "midiHcc", json_integer(paramsMap[midiCCs + 7]));
		json_object_set_new(rootJ, "mpeYcc", json_integer(paramsMap[mpeYcc]));
		json_object_set_new(rootJ, "mpeZcc", json_integer(paramsMap[mpeZcc]));
		json_object_set_new(rootJ, "RNDetune", json_integer(paramsMap[RNDetune]));
		json_object_set_new(rootJ, "trnsps", json_integer(paramsMap[transpose]));
		json_object_set_new(rootJ, "noteMin", json_integer(paramsMap[noteMin]));
		json_object_set_new(rootJ, "noteMax", json_integer(paramsMap[noteMax]));
		json_object_set_new(rootJ, "velMin", json_integer(paramsMap[velMin]));
		json_object_set_new(rootJ, "velMax", json_integer(paramsMap[velMax]));
		return rootJ;
	}
	///////////////////////////////////////////////////////////////////////////////////////
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
		//		json_t* midiJ = json_object_get(rootJ, "midi");
		//		if (midiJ) midiInput.fromJson(midiJ);
		json_t *polyModeIdJ = json_object_get(rootJ, "polyModeId");
		if (polyModeIdJ) paramsMap[polyModeId] = json_integer_value(polyModeIdJ);
		MPEmode = (paramsMap[polyModeId] < ROTATE_MODE);
		json_t *pbDwnJ = json_object_get(rootJ, "pbDwn");
		if (pbDwnJ) paramsMap[pbDwn] = json_integer_value(pbDwnJ);
		json_t *pbUpJ = json_object_get(rootJ, "pbUp");
		if (pbUpJ) paramsMap[pbUp] = json_integer_value(pbUpJ);
		json_t *pbMPEJ = json_object_get(rootJ, "pbMPE");
		if (pbMPEJ) paramsMap[pbMPE] = json_integer_value(pbMPEJ);
		json_t *mpeRelPbJ = json_object_get(rootJ, "mpeRelPb");
		if (mpeRelPbJ) paramsMap[mpeRelPb] = json_integer_value(mpeRelPbJ);
		json_t *numVoJ = json_object_get(rootJ, "numVoCh");
		if (numVoJ) paramsMap[numVoCh] = json_integer_value(numVoJ);
		json_t *MPEmasterChJ = json_object_get(rootJ, "MPEmasterCh");
		if (MPEmasterChJ) MPEmasterCh = json_integer_value(MPEmasterChJ);
		json_t *midiAccJ = json_object_get(rootJ, "midiAcc");
		if (midiAccJ) paramsMap[midiCCs + 0] = json_integer_value(midiAccJ);
		json_t *midiBccJ = json_object_get(rootJ, "midiBcc");
		if (midiBccJ) paramsMap[midiCCs + 1] = json_integer_value(midiBccJ);
		json_t *midiCccJ = json_object_get(rootJ, "midiCcc");
		if (midiCccJ) paramsMap[midiCCs + 2] = json_integer_value(midiCccJ);
		json_t *midiDccJ = json_object_get(rootJ, "midiDcc");
		if (midiDccJ) paramsMap[midiCCs + 3] = json_integer_value(midiDccJ);
		json_t *midiEccJ = json_object_get(rootJ, "midiEcc");
		if (midiEccJ) paramsMap[midiCCs + 4] = json_integer_value(midiEccJ);
		json_t *midiFccJ = json_object_get(rootJ, "midiFcc");
		if (midiFccJ) paramsMap[midiCCs + 5] = json_integer_value(midiFccJ);
		json_t *midiFccG = json_object_get(rootJ, "midiGcc");
		if (midiFccG) paramsMap[midiCCs + 6] = json_integer_value(midiFccG);
		json_t *midiFccH = json_object_get(rootJ, "midiHcc");
		if (midiFccH) paramsMap[midiCCs + 7] = json_integer_value(midiFccH);
		json_t *mpeYccJ = json_object_get(rootJ, "mpeYcc");
		if (mpeYccJ) paramsMap[mpeYcc] = json_integer_value(mpeYccJ);
		json_t *mpeZccJ = json_object_get(rootJ, "mpeZcc");
		if (mpeZccJ) paramsMap[mpeZcc] = json_integer_value(mpeZccJ);
		json_t *RNDetuneJ = json_object_get(rootJ, "RNDetune");
		if (RNDetuneJ) paramsMap[RNDetune] = json_integer_value(RNDetuneJ);
		json_t *trnspsJ = json_object_get(rootJ, "trnsps");
		if (trnspsJ) paramsMap[transpose] = json_integer_value(trnspsJ);
		json_t *noteMinJ = json_object_get(rootJ, "noteMin");
		if (noteMinJ) paramsMap[noteMin] = json_integer_value(noteMinJ);
		json_t *noteMaxJ = json_object_get(rootJ, "noteMax");
		if (noteMaxJ) paramsMap[noteMax] = json_integer_value(noteMaxJ);
		json_t *velMinJ = json_object_get(rootJ, "velMin");
		if (velMinJ) paramsMap[velMin] = json_integer_value(velMinJ);
		json_t *velMaxJ = json_object_get(rootJ, "velMax");
		if (velMaxJ) paramsMap[velMax] = json_integer_value(velMaxJ);
	}
	/////////////////////////////////////////////////////////////////////////////////////////
	
	void onReset() override{
		midiInput.reset();
		initParamsSettings();
		initVoices();
		MPEmasterCh = 0;// 0 ~ 15
		cursorIx = 0;
		outputInfos[Y_OUTPUT]->name = "1V/Oct detuned";
		outputInfos[Z_OUTPUT]->name = "Note Aftertouch";
		outputInfos[RVEL_OUTPUT]->name = "Release Velocity";
		for ( int i = 0 ; i < 8 ; i++ ){
			outputInfos[MM_OUTPUT + i]->name =  ccLongNames[paramsMap[midiCCs + i]];
		}
		disableDataKnob();
	}
	
	void resetVoices(){
		initVoices();
		if (paramsMap[polyModeId] < ROTATE_MODE) {
			if (paramsMap[polyModeId] > 0){// Haken MPE Plus
				Y_ptr = &Yhaken;//131;
				Z_ptr = &Zhaken;//132;
				RP_ptr = &one;
				Ycanedit = false;
				Zcanedit = false;
				RPcanedit = false;
				outputInfos[Y_OUTPUT]->name = "MPE Y 14bit";
				outputInfos[Z_OUTPUT]->name = "MPE Z 14bit";
			}else{ /// normal MPE
				Y_ptr = &paramsMap[mpeYcc];
				Z_ptr = &paramsMap[mpeZcc];
				RP_ptr = &paramsMap[mpeRelPb];
				Ycanedit = true;
				Zcanedit = true;
				RPcanedit = true;
				outputInfos[Y_OUTPUT]->name = "MPE Y";
				outputInfos[Z_OUTPUT]->name = "MPE Z";
			}
			outputInfos[RVEL_OUTPUT]->name = "Channel PitchBend";
			nVoChMPE = 16;
		}else {
			Y_ptr = &Detune130;
			Z_ptr = &NOTEAFT;//129
			RP_ptr = &zero;
			Ycanedit = true;// editing RND detune
			Zcanedit = false;
			RPcanedit = false;
			nVoChMPE = paramsMap[numVoCh];
			outputInfos[Y_OUTPUT]->name = "1V/Oct detuned";
			outputInfos[Z_OUTPUT]->name = "Note Aftertouch";
			outputInfos[RVEL_OUTPUT]->name = "Release Velocity";
		}
		//		float lambdaf = 100.f * APP->engine->getSampleTime();
		for (int i=0; i < 8; i++){
			//			MCCsFilter[i].lambda = lambdaf;
			midiCCsValues[paramsMap[midiCCs + i]] = 0;
		}
		//		mrPBendFilter.lambda = lambdaf;
		midiActivity = 127;
		resetMidi = false;
	}
	
	void initVoices(){
		//float lambdaf = 100.f * APP->engine->getSampleTime();
		pedal = false;
		lights[SUSTHOLD_LIGHT].setBrightness(0.f);
		for (int i = 0; i < 16; i++) {
			notes[i] = 60;
			gates[i] = false;
			hold[i] = false;
			mpey[i] = 0;
			vels[i] = 0;
			rvels[i] = 0;
			xpitch[i] = {0.f};
			mpex[i] = 0;
			mpez[i] = 0;
			cachedMPE[i].clear();
			mpePlusLB[i] = 0;
			lights[CH_LIGHT+ i].setBrightness(0.f);
			outputs[GATE_OUTPUT].setVoltage( 0.f, i);
		}
		rotateIndex = -1;
		cachedNotes.clear();
	}
	
	///////////////////////////////////////////////////////////////////////////////////////
	//	void onAdd() override{
	//		initVoices();
	//		for ( int i = 0 ; i < 8 ; i++ ){
	//			outputInfos[MM_OUTPUT + i]->name =  ccLongNames[paramsMap[midiCCs + i]];
	//		}
	//	}
	//	/////////////////////////////////////////////////////////////////////////////////////////
	
	////
	void initParamsSettings(){
		paramsMap[_noSelection] = 0;
		paramsMap[polyModeId] = ROTATE_MODE;
		paramsMap[numVoCh] = 8;
		paramsMap[pbDwn] = -12;
		paramsMap[pbUp] = 12;
		paramsMap[pbMPE] = 96;
		paramsMap[mpeRelPb] = 0;//0 rel, 1 PB
		paramsMap[RNDetune] = 10;
		paramsMap[transpose] = 0;
		paramsMap[mpeYcc] = 74;
		paramsMap[mpeZcc] = 128; //chn aftT
		paramsMap[noteMin] = 0;
		paramsMap[noteMax] = 127;
		paramsMap[velMin] = 1;
		paramsMap[velMax] = 127;
		paramsMap[midiCCs + 0] = 128;//chn aftT
		paramsMap[midiCCs + 1] = 1;//ModW
		paramsMap[midiCCs + 2] = 2;//BrCt
		paramsMap[midiCCs + 3] = 7;//Vol
		paramsMap[midiCCs + 4] = 10;//Pan
		paramsMap[midiCCs + 5] = 11;//Exp
		paramsMap[midiCCs + 6] = 74;//FltCtff
		paramsMap[midiCCs + 7] = 64;//Sust
		Z_ptr = &NOTEAFT;
		Y_ptr = &Detune130;
		RP_ptr = &zero;
	}
	
	void onRandomize(const RandomizeEvent& e) override {
		// ...
		//
		// Module::onRandomize(e);
	}
	void onSampleRateChange() override {
		PROCESS_RATE = static_cast<int>(APP->engine->getSampleRate() * 0.0005); //.5ms
		resetVoices();
	}
	/////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////
	int getPolyIndex(int nowIndex) {
		for (int i = 0; i < paramsMap[numVoCh]; i++) {
			nowIndex = (nowIndex + 1) % paramsMap[numVoCh];
			//if (!(gates[nowIndex] || pedalgates[nowIndex])) {
			if (!gates[nowIndex]) {
				stealIndex = nowIndex;
				return nowIndex;
			}
		}
		// All taken = send note to cache and steal voice
		stealIndex = (stealIndex + 1) % paramsMap[numVoCh];
		if ((paramsMap[polyModeId] < UNISON_MODE) && (gates[stealIndex]))//&&(polyMode > MPE_MODE).cannot reach here on MPE mode
			cachedNotes.push_back(notes[stealIndex]);
		return stealIndex;
	}
	/////////////////////////////////////////////////////////////////////////////////////////
	void pressNote(uint8_t channel, uint8_t note, uint8_t vel) {
		switch (learnId){
			case 0:
				break;
			case noteMin:{
				paramsMap[noteMin] = note;
				if (paramsMap[noteMax] < note) paramsMap[noteMax] = note;
				onFocus = 1;
			}return;
			case noteMax:{
				paramsMap[noteMax] = note;
				if (paramsMap[noteMin] > note) paramsMap[noteMin] = note;
				onFocus = 1;
			}return;
			case velMin:{
				paramsMap[velMin] = vel;
				if (paramsMap[velMax] < vel) paramsMap[velMax] = vel;
				onFocus = 1;
			}return;
			case velMax:{
				paramsMap[velMax] = vel;
				if (paramsMap[velMin] > vel) paramsMap[velMin] = vel;
				onFocus = 1;
			}return;
		}
		///////// escape if notes out of range
		if ((note < paramsMap[noteMin]) || (note > paramsMap[noteMax]) || (vel < paramsMap[velMin]) || (vel > paramsMap[velMax])) return;
		/////////////////
		switch (paramsMap[polyModeId]) {// Set notes and gates
			case MPE_MODE:
			case MPEPLUS_MODE:{
				if (channel == MPEmasterCh) return; /////  R E T U R N !!!!!!!
				//uint8_t ixch;
				if (channel + 1 > nVoChMPE) nVoChMPE = channel + 1;
				rotateIndex = channel; // ASSIGN VOICE Index
				if (gates[channel]) cachedMPE[channel].push_back(notes[channel]);///if
			} break;
			case ROTATE_MODE: {
				rotateIndex = getPolyIndex(rotateIndex);
			} break;
			case REUSE_MODE: {
				bool reuse = false;
				for (int i = 0; i < paramsMap[numVoCh]; i++) {
					if (notes[i] == note) {
						rotateIndex = i;
						reuse = true;
						break;
					}
				}
				if (!reuse)
					rotateIndex = getPolyIndex(rotateIndex);
			} break;
			case RESET_MODE:
			case RESTACK_MODE: {
				rotateIndex = getPolyIndex(-1);
			} break;
			case UNISON_MODE: {
				cachedNotes.push_back(note);
				bool retrignow = (params[RETRIG_PARAM].getValue() > 0.f);
				for (int i = 0; i < paramsMap[numVoCh]; i++) {
					notes[i] = note;
					vels[i] = vel;
					gates[i] = true;
					hold[i] = pedal;
					drift[i] = static_cast<float>((rand() % 200  - 100) * paramsMap[RNDetune]) / 120000.f;
					if (retrignow) reTrigger[i].trigger(1e-3);
				}
				return;/////  R E T U R N !!!!!!!
			} break;
			case UNISONLWR_MODE: {
				cachedNotes.push_back(note);
				uint8_t lnote = *min_element(cachedNotes.begin(),cachedNotes.end());
				bool retrignow = ((params[RETRIG_PARAM].getValue() > 0.f) && (lnote < notes[0]));
				for (int i = 0; i < paramsMap[numVoCh]; i++) {
					notes[i] = lnote;
					vels[i] = vel;
					gates[i] = true;
					hold[i] = pedal;
					drift[i] = static_cast<float>((rand() % 200  - 100) * paramsMap[RNDetune]) / 120000.f;
					if (retrignow) reTrigger[i].trigger(1e-3);
				}
				return;/////  R E T U R N !!!!!!!
			} break;
			case UNISONUPR_MODE:{
				cachedNotes.push_back(note);
				uint8_t unote = *max_element(cachedNotes.begin(),cachedNotes.end());
				bool retrignow = (params[RETRIG_PARAM].getValue() > 0.f) && (unote > notes[0]);
				for (int i = 0; i < paramsMap[numVoCh]; i++) {
					notes[i] = unote;
					vels[i] = vel;
					gates[i] = true;
					hold[i] = pedal;
					drift[i] = static_cast<float>((rand() % 200  - 100) * paramsMap[RNDetune]) / 120000.f;
					if (retrignow) reTrigger[i].trigger(1e-3);
				}
				return;/////  R E T U R N !!!!!!!
			} break;
			default: break;
		}
		notes[rotateIndex] = note;
		vels[rotateIndex] = vel;
		gates[rotateIndex] = true;
		// Set notes and gates
		if (params[RETRIG_PARAM].getValue() > 0.f) reTrigger[rotateIndex].trigger(1e-3);
		hold[rotateIndex] = pedal;
		drift[rotateIndex] = static_cast<float>((rand() % 1000 - 500) * paramsMap[RNDetune]) / 1200000.f;
		midiActivity = vel;
	}
	///////////////////////////////////////////////////////////////////////////////////////
	void releaseNote(uint8_t channel, uint8_t note, uint8_t vel) {
		bool backnote = false;
		if (paramsMap[polyModeId] > MPEPLUS_MODE) {
			// Remove the note
			if (!cachedNotes.empty()) backnote = (note == cachedNotes.back());
			std::vector<uint8_t>::iterator it = std::find(cachedNotes.begin(), cachedNotes.end(), note);
			if (it != cachedNotes.end()) cachedNotes.erase(it);
		}else {
			if (channel == MPEmasterCh) return;
			//get channel from dynamic map
			std::vector<uint8_t>::iterator it = std::find(cachedMPE[channel].begin(), cachedMPE[channel].end(), note);
			if (it != cachedMPE[channel].end()) cachedMPE[channel].erase(it);
		}
		switch (paramsMap[polyModeId]) {
				// default ROTATE_MODE REUSE_MODE RESET_MODE
			default: {
				for (int i = 0; i < paramsMap[numVoCh]; i++) {
					if (notes[i] == note) {
						gates[i] = false;
						if (!cachedNotes.empty()) {
							notes[i] = cachedNotes.back();
							cachedNotes.pop_back();
							gates[i] = true;
							//Retrigger recovered notes
							if (params[RETRIG_PARAM].getValue() > 1.f) reTrigger[i].trigger(1e-3);
						}
						rvels[i] = vel;
					}
				}
			} break;
			case MPE_MODE:
			case MPEPLUS_MODE:{
				if (note == notes[channel]) {
					if (hold[channel]) {
						gates[channel] = false;
					}
					//					/// check for cachednotes on MPE buffers...
					else {
						if (!cachedMPE[channel].empty()) {
							notes[channel] = cachedMPE[channel].back();
							cachedMPE[channel].pop_back();
							//Retrigger recovered notes
							if (params[RETRIG_PARAM].getValue() > 1.f) reTrigger[channel].trigger(1e-3);
						}else gates[channel] = false;
					}
					rvels[channel] = vel;
				}
			} break;
			case RESTACK_MODE: {
				for( int i = 0 ; i < paramsMap[numVoCh] ; i++){
					if (notes[i] == note) {
						if (hold[i]){
							/// don't restack if pedal hold
							gates[i] = false;
						}else {
							int clearii = i;
							///restack upper voices
							for (int ii = i; ii < paramsMap[numVoCh] - 1; ii++){
								if (gates[ii+1]) { /// if next is active...
									notes[ii] = notes[ii + 1];
									clearii = ii + 1;
									///if (params[RETRIG_PARAM].getValue() > 1.f) reTrigger[ii].trigger(1e-3);
								}else{///next off
									if (!cachedNotes.empty()) {
										notes[ii] = cachedNotes.back();
										cachedNotes.pop_back();
										clearii = ii + 1;
										break;
									}else{//no notes cached
										gates[ii] = false;
										clearii = ii;
										///gate off  upper
										
										break;
									}
								}
							}
							for (int iii = clearii; iii < paramsMap[numVoCh]; iii++){
								gates[iii] = false;
							}
						}
						break;
					}
				}
				//rvels[i] = vel;
			} break;
			case UNISON_MODE: {
				if (vel > 128) vel = 64;
				if (!cachedNotes.empty()) {
					uint8_t backnote = cachedNotes.back();
					bool retrignow = (params[RETRIG_PARAM].getValue() > 1.f) && (backnote != notes[0]);
					for (int i = 0; i < paramsMap[numVoCh]; i++) {
						notes[i] = backnote;
						gates[i] = true;
						rvels[i] = vel;
						if (retrignow) reTrigger[i].trigger(1e-3);
					}
				}else {
					for (int i = 0; i < paramsMap[numVoCh]; i++) {
						gates[i] = false;
						rvels[i] = vel;
					}
				}
			} break;
			case UNISONLWR_MODE: {
				if (vel > 128) vel = 64;
				if (!cachedNotes.empty()) {
					uint8_t lnote = *min_element(cachedNotes.begin(),cachedNotes.end());
					//Retrigger recovered notes
					bool retrignow = (params[RETRIG_PARAM].getValue() > 1.f) && (lnote != notes[0]);
					for (int i = 0; i < paramsMap[numVoCh]; i++) {
						notes[i] = lnote;
						gates[i] = true;
						rvels[i] = vel;
						if (retrignow) reTrigger[i].trigger(1e-3);
					}
				}
				else {
					for (int i = 0; i < paramsMap[numVoCh]; i++) {
						gates[i] = false;
						rvels[i] = vel;
					}
				}
			} break;
			case UNISONUPR_MODE: {
				if (vel > 128) vel = 64;
				if (!cachedNotes.empty()) {
					uint8_t unote = *max_element(cachedNotes.begin(),cachedNotes.end());
					//Retrigger recovered notes
					bool retrignow = (params[RETRIG_PARAM].getValue() == 2.f) && (unote != notes[0]);
					for (int i = 0; i < paramsMap[numVoCh]; i++) {
						notes[i] = unote;
						gates[i] = true;
						rvels[i] = vel;
						if (retrignow) reTrigger[i].trigger(1e-3);
					}
				}
				else {
					for (int i = 0; i < paramsMap[numVoCh]; i++) {
						gates[i] = false;
						rvels[i] = vel;
					}
				}
			} break;
		}
		midiActivity = vel;
	}
	///////////////////////////////////////////////////////////////////////////////////////
	void pressPedal() {
		if (params[SUSTHOLD_PARAM].getValue() < 0.5f) {
			pedal = false;
			return;
		}
		pedal = true;
		lights[SUSTHOLD_LIGHT].setBrightness(10.f);
		if (paramsMap[polyModeId] < ROTATE_MODE) { ///MPE or MPE+
			for (int i = 0; i < nVoChMPE; i++) {
				hold[i] = gates[i];
			}
		}else {
			for (int i = 0; i < paramsMap[numVoCh]; i++) {
				hold[i] = gates[i];
			}
		}
	}
	///////////////////////////////////////////////////////////////////////////////////////
	void releasePedal() {
		if (params[SUSTHOLD_PARAM].getValue() < 0.5f) return;
		pedal = false;
		lights[SUSTHOLD_LIGHT].setBrightness(0.f);
		// When pedal is released, recover cached notes for pressed keys if they were overtaked.
		if (paramsMap[polyModeId] < ROTATE_MODE) {
			for (int i = 0; i < nVoChMPE; i++) {
				if (gates[i] && !cachedMPE[i].empty()) {
					notes[i] = cachedMPE[i].back();
					cachedMPE[i].pop_back();
					bool retrignow = (params[RETRIG_PARAM].getValue() == 2.f);
					if (retrignow) reTrigger[i].trigger(1e-3);
				}
				hold[i] = false;
			}
		}else{
			for (int i = 0; i < paramsMap[numVoCh]; i++) {
				if (hold[i] && !cachedNotes.empty()) {
					if  (paramsMap[polyModeId] < RESTACK_MODE){
						notes[i] = cachedNotes.back();
						cachedNotes.pop_back();
						bool retrignow = gates[i] && (params[RETRIG_PARAM].getValue() == 2.f);
						if (retrignow) reTrigger[i].trigger(1e-3);
					}
				}
				hold[i] = false;
			}
			//			/////// revise
			//			if (paramsMap[polyModeId] == RESTACK_MODE) {
			//				for (int i = 0; i < paramsMap[numVoCh]; i++) {
			//					if (i < (int) cachedNotes.size()) {
			//						notes[i] = cachedNotes[i];
			//						gates[i] = true;
			//					}
			//					else {
			//						gates[i] = false;
			//					}
			//				hold[i] = false;
			//				}
			//			}
		}
	}
	///////////////////////////////////////////////////////////////////////////////////////
	void processMessage(midi::Message msg) {
		switch (msg.getStatus()) {
				// note off
			case 0x8: {
				if ((paramsMap[polyModeId] < ROTATE_MODE) && (msg.getChannel() == MPEmasterCh)) return;
				releaseNote(msg.getChannel(), msg.getNote(), msg.getValue());
			} break;
				// note on
			case 0x9: {
				if ((paramsMap[polyModeId] < ROTATE_MODE) && (msg.getChannel() == MPEmasterCh)) return;
				if (msg.getValue() > 0) {
					pressNote(msg.getChannel(), msg.getNote(), msg.getValue());
				}
				else {
					releaseNote(msg.getChannel(), msg.getNote(), 64);//64 is the default rel velocity.
				}
			} break;
				// note (poly) aftertouch
			case 0xa: {
				if (paramsMap[polyModeId] < ROTATE_MODE) return;
				noteData[msg.getNote()].aftertouch = msg.getValue();
				midiActivity = msg.getValue();
			} break;
				// channel aftertouch
			case 0xd: {
				if (learnId > 0) {// learn enabled ???
					paramsMap[learnId] = 128;
					onFocus = 1;
					return;
				}////////////////////////////////////////
				else if (paramsMap[polyModeId] < ROTATE_MODE){
					uint8_t channel = msg.getChannel();
					if (channel == MPEmasterCh){
						midiCCsValues[128] = msg.getNote();
						//chAfTch = msg.getNote();
					}else if (paramsMap[polyModeId] > 0){
						mpez[channel] =  msg.getNote() * 128 + mpePlusLB[channel];
						mpePlusLB[channel] = 0;
					}else {
						if (paramsMap[mpeZcc] == 128)
							mpez[channel] = msg.getNote() * 128;
						if (paramsMap[mpeYcc] == 128)
							mpey[channel] = msg.getNote() * 128;
					}
				}else{
					//chAfTch = msg.getNote();
					midiCCsValues[128] = msg.getNote();
				}
				midiActivity = msg.getNote();
			} break;
				// pitch Bend
			case 0xe:{
				//				if (learnId > 0) {// learn pitch bend ???
				//					paramsMap[learnId] = 128;
				//					onFocus = 1;
				//					return;
				//				}////////////////////////////////////////
				//				else
				if (paramsMap[polyModeId] < ROTATE_MODE){
					uint8_t channel = msg.getChannel();
					if (channel == MPEmasterCh){
						mrPBend = msg.getValue() * 128 + msg.getNote()  - 8192;
					}else{
						mpex[channel] = msg.getValue() * 128 + msg.getNote()  - 8192;
					}
				}else{
					mrPBend = msg.getValue() * 128 + msg.getNote() - 8192; //14bit Pitch Bend
				}
				midiActivity = msg.getValue();
			} break;
				// cc
			case 0xb: {
				if (paramsMap[polyModeId] < ROTATE_MODE){
					uint8_t channel = msg.getChannel();
					if (channel == MPEmasterCh){
						if (learnId > 0) {///////// LEARN CC MPE master
							paramsMap[learnId] = msg.getNote();
							onFocus = 1;
							return;
						}else processCC(msg);
					}else if (paramsMap[polyModeId] == MPEPLUS_MODE){ //Continuum
						if (msg.getNote() == 87){
							mpePlusLB[channel] = msg.getValue();
						}else if (msg.getNote() == 74){
							mpey[channel] =  msg.getValue() * 128 + mpePlusLB[channel];
							mpePlusLB[channel] = 0;
						}
					}else if (msg.getNote() == paramsMap[mpeYcc]){
						//cc74 0x4a default
						mpey[channel] = msg.getValue() * 128;
					}else if (msg.getNote() == paramsMap[mpeZcc]){
						mpez[channel] = msg.getValue() * 128;
					}
				}else if (learnId) {///////// LEARN CC Poly
					paramsMap[learnId] = msg.getNote();
					onFocus = 1;
					return;
				}else processCC(msg);
				midiActivity = msg.getValue();
			} break;
			default: break;
		}
	}
	///////////////////////////////////////////////////////////////////////////////////////
	void processCC(midi::Message msg) {
		if (msg.getNote() ==  0x40) { //get note gets cc number...0x40=internal sust pedal
			if (msg.getValue() >= 64) pressPedal();
			else releasePedal();
		}
		midiCCsValues[msg.getNote()] = msg.getValue();
	}
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////CONFIG DATA KNOB
	void configDataKnob(float min, float max, float val){
		TEST_FLOAT = val;
		paramQuantities[DATAKNOB_PARAM]->minValue = min;
		paramQuantities[DATAKNOB_PARAM]->maxValue = max;
		paramQuantities[DATAKNOB_PARAM]->defaultValue = val; //doubleclick restore prev value
		paramQuantities[DATAKNOB_PARAM]->setSmoothValue(val);
		//paramQuantities[DATAKNOB_PARAM]->reset();
		updateDataKnob = false;
		onFocus = Focus_SEC;
	}
	void disableDataKnob(){
		paramQuantities[DATAKNOB_PARAM]->minValue = 1000.f;
		paramQuantities[DATAKNOB_PARAM]->maxValue = 1000.f;
		paramQuantities[DATAKNOB_PARAM]->defaultValue = 1000.f;
		paramQuantities[DATAKNOB_PARAM]->reset();
		TEST_FLOAT = 1000.f;
	}
	//// DATA PLUS MINUS
	void dataPlusMinus(int val){
		onFocus = Focus_SEC;
		int newVal = 0;
		bool doreset = false;
		switch (cursorIx){
			case _noSelection: {
				return;
			}break;
			case polyModeId: {
				newVal = paramsMap[polyModeId] + val;
				if ((newVal < 0) || (newVal > (NUM_POLYMODES - 1))) return;
				MPEmode = (newVal < ROTATE_MODE);//bool flag
				doreset = true;
			}break;
			case numVoCh: {
				if (paramsMap[polyModeId] < ROTATE_MODE) {
					newVal = paramsMap[pbMPE] + val;
					if ((newVal < 0) || (newVal > 96)) return;
				} else {//num voices
					newVal = paramsMap[numVoCh] + val; ///1  to 16//
					if ((newVal < 1) || (newVal > 16)) return;
					doreset= true;
				}
			}break;
			case noteMin: {//notemin
				newVal = paramsMap[noteMin] + val;//0 to noteMax
				if ( (newVal < 0) || (newVal > paramsMap[noteMax])) return;
			}break;
			case noteMax: {//notemax
				newVal = paramsMap[noteMax] + val;//noteMin to 127
				if ((newVal < paramsMap[noteMin]) || (newVal > 127)) return;
			}break;
			case velMin: {//velmin
				newVal = paramsMap[velMin] + val; //1 to velMax
				if ( (newVal < 1) || (newVal > paramsMap[velMax])) return;
			}break;
			case velMax: {//velmax
				newVal = paramsMap[velMax] + val;//velMin to 127
				if ((newVal < paramsMap[velMin]) || (newVal > 127)) return;
			}break;
			case mpeYcc: {
				if (paramsMap[polyModeId] == MPE_MODE) {
					newVal = paramsMap[mpeYcc] + val;//0 to 128
					if ((newVal < 0) || (newVal > 128)) return;
					paramsMap[mpeYcc] = newVal;
				}else{///RNDetune
					newVal =  paramsMap[RNDetune] + val;//0 to 99
					if ((newVal < 0) || (newVal > 99)) return;
				}
			}break;
			case mpeZcc: {
				newVal = paramsMap[mpeZcc] + val;//0 to 128
				if ((newVal < 0) || (newVal > 128)) return;
				paramsMap[mpeZcc] = newVal;
			}break;
			case mpeRelPb: {
				newVal = (paramsMap[mpeRelPb] + val);// 0 1
				if ((newVal < 0) || (newVal > 1)) return;
			}break;
			case transpose: {
				newVal = paramsMap[transpose] + val;//-48 to 48
				if ((newVal < -48) || (newVal > 48)) return;
			}break;
			case pbDwn: {
				newVal = paramsMap[pbDwn] + val;//-96 to 96
				if ((newVal < -96) || (newVal > 96)) return;
			}break;
			case pbUp: {
				newVal = paramsMap[pbUp] + val;//-96 to 96
				if ((newVal < -96) || (newVal > 96)) return;
			}break;
			default: {//CCs x 8
				newVal = paramsMap[cursorIx] + val;//0 to 128
				if ((newVal < 0) || (newVal > 128)) return;
				outputInfos[MM_OUTPUT + cursorIx - midiCCs]->name = ccLongNames[newVal];
			}break;
		}
		paramQuantities[DATAKNOB_PARAM]->setSmoothValue(static_cast<float>(newVal));
		if (doreset) resetVoices();
	}
	///DATA Entry
	void updateData(){
		switch (cursorIx){
			case _noSelection: {
				return;
			}break;
			case polyModeId: {///POLYMODE
				if (updateDataKnob){ configDataKnob((float)MPE_MODE,(float)UNISONUPR_MODE,(float)paramsMap[polyModeId]);
					return;
				}
				int newVal = static_cast<int>(params[DATAKNOB_PARAM].getValue());
				if (paramsMap[polyModeId] != newVal){
					paramsMap[polyModeId] = newVal;
					MPEmode = (paramsMap[polyModeId] < ROTATE_MODE);//bool flag
					onFocus = Focus_SEC;
					resetVoices();
				}
			}break;
			case numVoCh: {
				if (paramsMap[polyModeId] < ROTATE_MODE) {///PITCHBEND MPE
					if (updateDataKnob) {
						configDataKnob(0.f,96.f,(float)paramsMap[pbMPE]);
						return;
					}
					int newVal = static_cast<int>(params[DATAKNOB_PARAM].getValue());
					if (paramsMap[pbMPE] != newVal) {
						paramsMap[pbMPE] = newVal;
						onFocus = Focus_SEC;
					}
				} else {//num Voices
					if (updateDataKnob) {
						configDataKnob(1.f,16.f,(float)paramsMap[numVoCh]);
						return;
					}
					int newVal = static_cast<int>(params[DATAKNOB_PARAM].getValue());
					if (paramsMap[numVoCh] != newVal) {
						paramsMap[numVoCh] = newVal;
						resetVoices();
						onFocus = Focus_SEC;
					}
				}
			}break;
			case noteMin: {///NOTE MIN
				if (updateDataKnob) {
					configDataKnob(0.f,(float)paramsMap[noteMax],(float)paramsMap[noteMin]);
					return;
				}
				int newVal = static_cast<int>(params[DATAKNOB_PARAM].getValue());
				if (paramsMap[noteMin] != newVal){
					paramsMap[noteMin] = newVal;
					onFocus = Focus_SEC;
				}
			}break;
			case noteMax: {///NOTE MAX
				if (updateDataKnob) {
					configDataKnob((float)paramsMap[noteMin],127.f,(float)paramsMap[noteMax]);
					return;
				}
				int newVal = static_cast<int>(params[DATAKNOB_PARAM].getValue());
				if (paramsMap[noteMax] != newVal){
					paramsMap[noteMax] = newVal;
					onFocus = Focus_SEC;
				}
			}break;
			case velMin: {///VEL MIN
				if (updateDataKnob) {
					configDataKnob(1.f,(float)paramsMap[velMax],(float)paramsMap[velMin]);
					return;
				}
				int newVal = static_cast<int>(params[DATAKNOB_PARAM].getValue());
				if (paramsMap[velMin] != newVal){
					paramsMap[velMin] = newVal;
					onFocus = Focus_SEC;
				}
			}break;
			case velMax: {//VEL MAX
				if (updateDataKnob) {
					configDataKnob((float)paramsMap[velMin], 127.f, (float)paramsMap[velMax]);
					return;
				}
				int newVal = static_cast<int>(params[DATAKNOB_PARAM].getValue());
				if (paramsMap[velMax] != newVal){
					paramsMap[velMax] = newVal;
					onFocus = Focus_SEC;
				}
			}break;// mpeY / drift
			case mpeYcc: {
				if (paramsMap[polyModeId] == MPE_MODE) {///mpeY
					if (updateDataKnob) {
						configDataKnob(0.f, 128.f,(float)paramsMap[mpeYcc]);
						return;
					}
					int newVal = (int) params[DATAKNOB_PARAM].getValue();
					if (paramsMap[mpeYcc] != newVal){
						paramsMap[mpeYcc] = newVal;
						onFocus = Focus_SEC;
					}
				}else {// RNDetune
					if (updateDataKnob) {
						configDataKnob(0.f, 99.f,(float)paramsMap[RNDetune]);
						return;
					}
					int newVal = (int) params[DATAKNOB_PARAM].getValue();
					if (paramsMap[RNDetune] != newVal){
						paramsMap[RNDetune] = newVal;
						onFocus = Focus_SEC;
					}
				}
			}break;
			case mpeZcc: {//mpeZ
				if (updateDataKnob) {
					configDataKnob(0.f,128.f,(float)paramsMap[mpeZcc]);
					return;
				}
				int newVal = (int) params[DATAKNOB_PARAM].getValue();
				if (paramsMap[mpeZcc] != newVal){
					paramsMap[mpeZcc] = newVal;
					onFocus = Focus_SEC;
				}
			}break;
			case mpeRelPb: {//mpePBend out
				if (updateDataKnob) {
					configDataKnob(0.f,1.f,(float)paramsMap[mpeRelPb]);
					return;
				}
				int newVal = (int) params[DATAKNOB_PARAM].getValue();
				if (paramsMap[mpeRelPb] != newVal) {
					paramsMap[mpeRelPb] = newVal;
					onFocus = Focus_SEC;
				}
			}break;
			case transpose: {///TRANSPOSE
				if (updateDataKnob) {
					configDataKnob(-48.f,48.f,(float)paramsMap[transpose]);
					return;
				}
				int newVal = (int) params[DATAKNOB_PARAM].getValue();
				if (paramsMap[transpose] != newVal){
					paramsMap[transpose] = newVal;
					onFocus = Focus_SEC;
				}
			}break;
			case pbDwn: { ///MAIN PBend DOWN
				if (updateDataKnob) {
					configDataKnob(-96.f,96.f,(float)paramsMap[pbDwn]);
					return;
				}
				int newVal = (int) params[DATAKNOB_PARAM].getValue();
				if (paramsMap[pbDwn] != newVal){
					paramsMap[pbDwn] = newVal;
					onFocus = Focus_SEC;
				}
			}break;
			case pbUp: { ///MAIN PBend UP
				if (updateDataKnob) {
					configDataKnob(-96.f,96.f,(float)paramsMap[pbUp]);
					return;
				}
				int newVal = (int) params[DATAKNOB_PARAM].getValue();
				if (paramsMap[pbUp] != newVal){
					paramsMap[pbUp] = newVal;
					onFocus = Focus_SEC;
				}
			}break;
			default: {// midi CCs
				if (updateDataKnob) {
					configDataKnob(0.f, 128.f, (float)paramsMap[cursorIx]);
					return;
				}
				int newVal = (int) params[DATAKNOB_PARAM].getValue();
				if (paramsMap[cursorIx] != newVal){
					paramsMap[cursorIx] = newVal;
					outputInfos[MM_OUTPUT + cursorIx - midiCCs]->name = ccLongNames[paramsMap[cursorIx]];
					onFocus = Focus_SEC;
				}
			}break;
		}
		if (MinusOneTrigger.process(params[MINUSONE_PARAM].getValue())) {
			dataPlusMinus(-1);
			return;
		}
		if (PlusOneTrigger.process(params[PLUSONE_PARAM].getValue())) {
			dataPlusMinus(1);
			return;
		}
	}
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////   P  R  O  C  E  S  S   /////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	void process(const ProcessArgs &args) override {
		if (ProcessFrame++ < PROCESS_RATE) return;
		ProcessFrame = 0;
		midi::Message msg;
		while (midiInput.tryPop(&msg, args.frame)) {
			processMessage(msg);
		}
		outputs[X_OUTPUT].setChannels(nVoChMPE);
		outputs[Y_OUTPUT].setChannels(nVoChMPE);
		outputs[Z_OUTPUT].setChannels(nVoChMPE);
		outputs[VEL_OUTPUT].setChannels(nVoChMPE);
		outputs[RVEL_OUTPUT].setChannels(nVoChMPE);
		outputs[GATE_OUTPUT].setChannels(nVoChMPE);
		
		float pbVo = 0.f, pbVoice = 0.f;
		if (mrPBend < 0){
			pbVo = mrPBendFilter.process(1.f ,rescale(mrPBend, -8192, 0, -5.f, 0.f));
			pbVoice = -1.f * pbVo * paramsMap[pbDwn] / 60.f;
		} else {
			pbVo = mrPBendFilter.process(1.f ,rescale(mrPBend, 0, 8191, 0.f, 5.f));
			pbVoice = pbVo * paramsMap[pbUp] / 60.f;
		}
		outputs[PBEND_OUTPUT].setVoltage(pbVo);
		
		if (paramsMap[polyModeId] > MPEPLUS_MODE){
			for (int i = 0; i < paramsMap[numVoCh]; i++) {
				float outGate = ((gates[i] || hold[i]) && !reTrigger[i].process(args.sampleTime)) ? 10.f : 0.f;
				outputs[GATE_OUTPUT].setVoltage(outGate, i);
				float outPitch = ((notes[i] - 60 + paramsMap[transpose]) / 12.f) + pbVoice;
				outputs[X_OUTPUT].setVoltage(outPitch, i);
				outputs[Y_OUTPUT].setVoltage(outPitch + drift[i] , i);	//drifted out
				outputs[VEL_OUTPUT].setVoltage(rescale(vels[i], 0, 127, 0.f, 10.f), i);
				outputs[RVEL_OUTPUT].setVoltage(rescale(rvels[i], 0, 127, 0.f, 10.f), i);
				outputs[Z_OUTPUT].setVoltage(rescale(noteData[notes[i]].aftertouch, 0, 127, 0.f, 10.f), i);
				lights[CH_LIGHT+ i].setBrightness(((i == rotateIndex)? 0.2f : 0.f) + (outGate * .08f));
			}
		} else {/// MPE MODE!!!
			for (int i = 0; i < nVoChMPE; i++) {
				float outGate = ((gates[i] || hold[i]) && !reTrigger[i].process(args.sampleTime)) ? 10.f : 0.f;
				outputs[GATE_OUTPUT].setVoltage(outGate, i);
				if (mpex[i] < 0) xpitch[i] = (MPExFilter[i].process(1.f ,rescale(mpex[i], -8192, 0, -5.f, 0.f)));
				else xpitch[i] = (MPExFilter[i].process(1.f ,rescale(mpex[i], 0, 8191, 0.f, 5.f)));
				outputs[X_OUTPUT].setVoltage(xpitch[i]  * paramsMap[pbMPE] / 60.f + ((notes[i] - 60) / 12.f) + pbVoice, i);
				outputs[VEL_OUTPUT].setVoltage(rescale(vels[i], 0, 127, 0.f, 10.f), i);
				if ((paramsMap[mpeRelPb] > 1)|| (paramsMap[polyModeId] > MPE_MODE)) outputs[RVEL_OUTPUT].setVoltage(xpitch[i], i);
				else outputs[RVEL_OUTPUT].setVoltage(rescale(rvels[i], 0, 127, 0.f, 10.f), i);
				outputs[Y_OUTPUT].setVoltage(MPEyFilter[i].process(1.f ,rescale(mpey[i], 0, 16383, 0.f, 10.f)), i);
				outputs[Z_OUTPUT].setVoltage(MPEzFilter[i].process(1.f ,rescale(mpez[i], 0, 16383, 0.f, 10.f)), i);
				lights[CH_LIGHT + i].setBrightness(((i == rotateIndex)? 0.2f : 0.f) + (outGate * .08f));
			}
		}
		for (int i = 0; i < 8; i++){
			outputs[MM_OUTPUT + i].setVoltage(MCCsFilter[i].process(1.f ,rescale(midiCCsValues[paramsMap[midiCCs + i]], 0, 127, 0.f, 10.f)));
		}
		if (resetMidi) {// resetMidi from MIDI widget
			resetVoices();
			return;
		}
		if (cursorIx < 1) return;
		if (onFocus > 0){
			onFocus--;
			if (onFocus < 1){
				cursorIx = 0;
				learnId = 0;
				disableDataKnob();
				return;
			}
		}
		updateData();
	}
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////   PROCESS END  ///////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////
};
//// Main Display ///////////////////////////////////////////////////////////////////////////////////////
struct PolyModeDisplay : TransparentWidget {
	PolyModeDisplay(){
		//font = APP->window->loadFont(mFONT_FILE);
	}
	MIDIpolyMPE *module;
	float mdfontSize = 13.f;
	std::string sMode = "";
	std::string sVo = "";
	std::string sPBM = "";
	std::string snoteMin = "";
	std::string snoteMax = "";
	std::string svelMin = "";
	std::string svelMax = "";
	std::string yyDisplay = "";
	std::string zzDisplay = "";
	std::shared_ptr<Font> font;
	std::string polyModeStr[9] = {
		"M. P. E.",
		"M. P. E. Plus",
		"R O T A T E",
		"R E U S E",
		"R E S E T",
		"R E S T A C K",
		"U N I S O N",
		"U N I S O N Lower",
		"U N I S O N Upper",
	};
	std::string noteName[12] = {"C","C#","D","Eb","E","F","F#","G","Ab","A","Bb","B"};
	int drawFrame = 0;
	int cursorIxI = 0;
	int timedFocus = 0;
	const unsigned char hiInk = 0xff;
	const unsigned char base = 0xdd;
	const unsigned char alphabck = 0x64;
	unsigned char redSel = 0x7f;
	unsigned char red[6];
	unsigned char grn[6];
	unsigned char blu[6];
	bool canlearn = true;
	void drawLayer(const DrawArgs &args, int layer) override {
		if (layer != 1) return;
		font = APP->window->loadFont(mFONT_FILE);
		if (!(font && font->handle >= 0)) return;
		if (cursorIxI != module->cursorIx){
			cursorIxI = module->cursorIx;
			timedFocus = 48;
		}
		sMode = polyModeStr[module->paramsMap[MIDIpolyMPE::polyModeId]];
		if (module->paramsMap[MIDIpolyMPE::polyModeId] < 2){
			sVo =  "Vo chnl PBend: " + std::to_string(module->paramsMap[MIDIpolyMPE::pbMPE]);
		}else{
			sVo = "Voice channels: " + std::to_string(module->paramsMap[MIDIpolyMPE::numVoCh]);
		}
		snoteMin = noteName[module->paramsMap[MIDIpolyMPE::noteMin] % 12] + std::to_string((module->paramsMap[MIDIpolyMPE::noteMin] / 12) - 2);
		snoteMax = noteName[module->paramsMap[MIDIpolyMPE::noteMax] % 12] + std::to_string((module->paramsMap[MIDIpolyMPE::noteMax] / 12) - 2);
		svelMin = std::to_string(module->paramsMap[MIDIpolyMPE::velMin]);
		svelMax = std::to_string(module->paramsMap[MIDIpolyMPE::velMax]);
		for (int i = 0; i < 6 ; i++){
			red[i]=base;
			grn[i]=base;
			blu[i]=base;
		}
		if ((cursorIxI > 0) && (cursorIxI < 7)){
			red[cursorIxI - 1] = hiInk;
			grn[cursorIxI - 1] = 0;
			blu[cursorIxI - 1] = 0;
			nvgBeginPath(args.vg);
			canlearn = true;
			switch (cursorIxI){
				case MIDIpolyMPE::polyModeId:{ // PolyMode
					nvgRoundedRect(args.vg, 1.f, 1.f, 134.f, 12.f, 3.f);
					canlearn = false;
				}break;
				case MIDIpolyMPE::numVoCh:{ //numVoices/PB MPE
					nvgRoundedRect(args.vg, 1.f, 14.f, 134.f, 12.f, 3.f);
					canlearn = false;
				}break;
				case MIDIpolyMPE::noteMin:{ //minNote
					nvgRoundedRect(args.vg, 19.f, 28.f, 29.f, 12.f, 3.f);
				}break;
				case MIDIpolyMPE::noteMax:{ //maxNote
					nvgRoundedRect(args.vg, 48.f, 28.f, 29.f, 12.f, 3.f);
				}break;
				case MIDIpolyMPE::velMin:{ //minVel
					nvgRoundedRect(args.vg, 93.f, 28.f, 20.f, 12.f, 3.f);
				}break;
				case MIDIpolyMPE::velMax:{ //maxVel
					nvgRoundedRect(args.vg, 113.f, 28.f, 20.f, 12.f, 3.f);
				}break;
				default:{
					goto noFocus;
				}
			}
			if (timedFocus > 0)	timedFocus  -=2;
			redSel = (canlearn)? (redSel + 24) % 256 : 0x7f;
			nvgFillColor(args.vg, nvgRGBA(redSel, 0, 0, alphabck + timedFocus)); //SELECTED
			nvgFill(args.vg);
		}
	noFocus:
		nvgFontSize(args.vg, mdfontSize);
		nvgFontFaceId(args.vg, font->handle);
		nvgTextAlign(args.vg, NVG_ALIGN_CENTER);
		nvgFillColor(args.vg, nvgRGB(red[0], grn[0], blu[0]));///polymode
		nvgTextBox(args.vg, 1.f, 11.0f, 134.f, sMode.c_str(), NULL);
		nvgFillColor(args.vg, nvgRGB(red[1], grn[1], blu[1]));///voices
		nvgTextBox(args.vg, 1.f, 24.f, 134.f, sVo.c_str(), NULL);
		nvgFillColor(args.vg, nvgRGB(red[2]|red[3], grn[2]|grn[3], blu[2]|blu[3]));//note
		nvgTextBox(args.vg, 1.f, 37.f, 18.f, "nte:", NULL);
		nvgFillColor(args.vg, nvgRGB(red[2], grn[2], blu[2]));//...min
		nvgTextBox(args.vg, 19.f, 37.f, 29.f, snoteMin.c_str(), NULL);
		nvgFillColor(args.vg, nvgRGB(red[3], grn[3], blu[3]));//...max
		nvgTextBox(args.vg, 48.f, 37.f, 29.f, snoteMax.c_str(), NULL);
		nvgFillColor(args.vg, nvgRGB(red[4]|red[5], grn[4]|grn[5], blu[4]|blu[5]));//Vel
		nvgTextBox(args.vg, 77.f, 37.f, 16.f, "vel:", NULL);
		nvgFillColor(args.vg, nvgRGB(red[4], grn[4], blu[4]));//min
		nvgTextBox(args.vg, 93.f, 37.f, 20.f, svelMin.c_str(), NULL);
		nvgFillColor(args.vg, nvgRGB(red[5], grn[5], blu[5]));//max
		nvgTextBox(args.vg, 113.f, 37.f, 20.f, svelMax.c_str(), NULL);
	}
	void onButton(const event::Button &e) override {
		if ((e.button == GLFW_MOUSE_BUTTON_LEFT) && (e.action == GLFW_PRESS)){
			int cmap = 0;
			int i = static_cast<int>(e.pos.y / 13.f) + 1;
			if (i > 2) {//Third line...
				if (e.pos.x < 45.3f) cmap = MIDIpolyMPE::noteMin;
				else if (e.pos.x < 68.f) cmap = MIDIpolyMPE::noteMax;
				else if (e.pos.x < 113.3f) cmap = MIDIpolyMPE::velMin;
				else cmap = MIDIpolyMPE::velMax;
				if (module->cursorIx != cmap){
					module->cursorIx = cmap;
					module->learnId = cmap;
					module->updateDataKnob = true;
				}else {
					module->onFocus = 1;
				}
			} else {///first or second line
				module->learnId = 0;
				if (module->cursorIx != i)	{
					module->cursorIx = i;
					module->updateDataKnob = true;
				}else {
					module->onFocus = 1;
				}
			}
		}
	}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////// MODULE WIDGET ////////
/////////////////////////////////////////////////
struct MIDIpolyMPEWidget : ModuleWidget {
	MIDIpolyMPEWidget(MIDIpolyMPE *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance,"res/modules/MIDIpolyMPE.svg")));
		//		//Screws
		addChild(createWidget<ScrewBlack>(Vec(0.f, 0.f)));
		addChild(createWidget<ScrewBlack>(Vec(135.f, 0.f)));
		addChild(createWidget<ScrewBlack>(Vec(0.f, 365.f)));
		addChild(createWidget<ScrewBlack>(Vec(135.f, 365.f)));
		float xPos = 7.f;
		float yPos = 19.f;
		if (module) {
			//MIDI
			MIDIscreen *dDisplay = createWidget<MIDIscreen>(Vec(xPos,yPos));
			dDisplay->box.size = {136.f, 40.f};
			dDisplay->setMidiPort (&module->midiInput, &module->MPEmode, &module->MPEmasterCh, &module->midiActivity, &module->mdriverJx, &module->mdeviceJx, &module->mchannelJx, &module->resetMidi);
			addChild(dDisplay);
			//Midi w Menu
			//			transparentMidiButton* midiButton = createWidget<transparentMidiButton>(Vec(xPos,yPos));
			//			 midiButton->setMidiPort(&module->midiInput);
			//			 addChild(midiButton);
			
			//PolyModes LCD
			xPos = 7.f;
			yPos = 61.f;
			PolyModeDisplay *polyModeDisplay = createWidget<PolyModeDisplay>(Vec(xPos,yPos));
			polyModeDisplay->box.size = {136.f, 40.f};
			polyModeDisplay->module = module;
			addChild(polyModeDisplay);
			xPos = 17.f;
			yPos = 202.f;
			{//MPE Ycc
				MPEYdetuneLCD *MccLCD = createWidget<MPEYdetuneLCD>(Vec(xPos,yPos));
				MccLCD->box.size = {34.f, 13.f};
				MccLCD->buttonId = MIDIpolyMPE::mpeYcc;
				MccLCD->cursorIx = &module->cursorIx;
				MccLCD->ptr_param_val = &module->Y_ptr;
				MccLCD->detune_val = &module->paramsMap[MIDIpolyMPE::RNDetune];
				MccLCD->updateKnob = &module->updateDataKnob;
				MccLCD->learnId = &module->learnId;
				MccLCD->onFocus = &module->onFocus;
				MccLCD->canlearn = false;
				MccLCD->canedit = &module->Ycanedit;
				addChild(MccLCD);
			}
			xPos = 60.f;
			{///MPE Zcc
				PTR_paramLCD *MccLCD = createWidget<PTR_paramLCD>(Vec(xPos,yPos));
				MccLCD->box.size = {34.f, 13.f};
				MccLCD->buttonId = MIDIpolyMPE::mpeZcc;
				MccLCD->cursorIx = &module->cursorIx;
				MccLCD->ptr_param_val = &module->Z_ptr;
				MccLCD->updateKnob = &module->updateDataKnob;
				MccLCD->learnId = &module->learnId;
				MccLCD->onFocus = &module->onFocus;
				MccLCD->canlearn = false;
				MccLCD->canedit = &module->Zcanedit;
				addChild(MccLCD);
			}
			xPos = 103.f;
			{// RelPbendMPE LCD
				RelVelLCD *MccLCD = createWidget<RelVelLCD>(Vec(xPos,yPos));
				MccLCD->box.size = {34.f, 13.f};
				MccLCD->buttonId = MIDIpolyMPE::mpeRelPb;
				MccLCD->cursorIx = &module->cursorIx;
				MccLCD->ptr_param_val = &module->RP_ptr;
				MccLCD->updateKnob = &module->updateDataKnob;
				MccLCD->learnId = &module->learnId;
				MccLCD->onFocus = &module->onFocus;
				MccLCD->canlearn = false;
				MccLCD->canedit = &module->RPcanedit;
				addChild(MccLCD);
			}
			xPos = 11.5f;
			yPos = 253.5f;
			{///Transpose
				PlusMinusValLCD *MccLCD = createWidget<PlusMinusValLCD>(Vec(xPos,yPos));
				MccLCD->box.size = {30.f, 13.f};
				MccLCD->buttonId = MIDIpolyMPE::transpose;
				MccLCD->cursorIx = &module->cursorIx;
				MccLCD->param_val = &module->paramsMap[MIDIpolyMPE::transpose];
				MccLCD->updateKnob = &module->updateDataKnob;
				MccLCD->learnId = &module->learnId;
				MccLCD->onFocus = &module->onFocus;
				MccLCD->canlearn = false;
				addChild(MccLCD);
			}
			xPos = 47.5f;
			{///PBend Down
				PlusMinusValLCD *MccLCD = createWidget<PlusMinusValLCD>(Vec(xPos,yPos));
				MccLCD->box.size = {23.f, 13.f};
				MccLCD->buttonId = MIDIpolyMPE::pbDwn;
				MccLCD->cursorIx = &module->cursorIx;
				MccLCD->param_val = &module->paramsMap[MIDIpolyMPE::pbDwn];
				MccLCD->updateKnob = &module->updateDataKnob;
				MccLCD->learnId = &module->learnId;
				MccLCD->onFocus = &module->onFocus;
				MccLCD->canlearn = false;
				addChild(MccLCD);
			}
			xPos += 23.f;
			{///PBend Up
				PlusMinusValLCD *MccLCD = createWidget<PlusMinusValLCD>(Vec(xPos,yPos));
				MccLCD->box.size = {23.f, 13.f};
				MccLCD->buttonId = MIDIpolyMPE::pbUp;
				MccLCD->canlearn = false;
				MccLCD->cursorIx = &module->cursorIx;
				MccLCD->param_val = &module->paramsMap[MIDIpolyMPE::pbUp];
				MccLCD->updateKnob = &module->updateDataKnob;
				MccLCD->learnId = &module->learnId;
				MccLCD->onFocus = &module->onFocus;
				addChild(MccLCD);
			}
			// CC's x 8
			for ( int i = 0; i < 8; i++){
				xPos = 10.5f + (float)(i % 4) * 33.f;
				yPos = 283.f + (float)((int)((float)i * 0.25f) % 4) * 40.f;
				MIDIccLCD *MccLCD = createWidget<MIDIccLCD>(Vec(xPos,yPos));
				MccLCD->box.size = {30.f, 13.f};
				MccLCD->buttonId = MIDIpolyMPE::midiCCs + i;
				MccLCD->canlearn = true;
				MccLCD->cursorIx = &module->cursorIx;
				MccLCD->param_val = &module->paramsMap[MIDIpolyMPE::midiCCs + i];
				MccLCD->updateKnob = &module->updateDataKnob;
				MccLCD->learnId = &module->learnId;
				MccLCD->onFocus = &module->onFocus;
				addChild(MccLCD);
			}
			//dataEntry
			DataEntyOnLed *dataEntyOnLed = createWidget<DataEntyOnLed>(Vec(21.5f,107.5f));
			dataEntyOnLed->cursorIx = &module->cursorIx;
			addChild(dataEntyOnLed);
		}///end if module
		yPos = 110.5f;
		xPos = 57.f;
		////DATA KNOB + -
		addParam(createParam<DataEntryKnob>(Vec(xPos, yPos), module, MIDIpolyMPE::DATAKNOB_PARAM));
		//+ - Buttons
		yPos = 117.f;
		xPos = 24.f;
		addParam(createParam<minusButtonB>(Vec(xPos, yPos), module, MIDIpolyMPE::MINUSONE_PARAM));
		xPos = 103.f;
		addParam(createParam<plusButtonB>(Vec(xPos, yPos), module, MIDIpolyMPE::PLUSONE_PARAM));
		// channel Leds x 16
		yPos = 101.f;
		xPos = 13.f;
		for (int i = 0; i < 16; i++){
			addChild(createLight<VoiceChRedLed>(Vec(xPos + i * 8.f, yPos), module, MIDIpolyMPE::CH_LIGHT + i));
		}
		yPos = 170.5f;
		xPos = 81.f;
		// Sustain hold notes switch
		addParam(createParam<moDllzSwitchLed>(Vec(xPos, yPos), module, MIDIpolyMPE::SUSTHOLD_PARAM));
		addChild(createLight<TranspOffRedLight>(Vec(xPos, yPos), module, MIDIpolyMPE::SUSTHOLD_LIGHT));
		//Retrig
		addParam(createParam<moDllzSwitchT>(Vec(98.5f, yPos), module, MIDIpolyMPE::RETRIG_PARAM));
		////  POLY OUTPUTS
		yPos = 171.f;
		xPos = 19.5f;//Pitch
		addOutput(createOutput<moDllzPolyO>(Vec(xPos, yPos),  module, MIDIpolyMPE::X_OUTPUT));
		xPos = 50.5f;//Gate
		addOutput(createOutput<moDllzPolyO>(Vec(xPos, yPos),  module, MIDIpolyMPE::GATE_OUTPUT));
		xPos = 114.5;//Vel
		addOutput(createOutput<moDllzPolyO>(Vec(xPos, yPos),  module, MIDIpolyMPE::VEL_OUTPUT));
		yPos = 218.5f;
		xPos = 22.5f;//Y
		addOutput(createOutput<moDllzPolyO>(Vec(xPos, yPos),  module, MIDIpolyMPE::Y_OUTPUT));
		xPos = 65.5f;//Z
		addOutput(createOutput<moDllzPolyO>(Vec(xPos, yPos),  module, MIDIpolyMPE::Z_OUTPUT));
		xPos = 108.5f;// RVel
		addOutput(createOutput<moDllzPolyO>(Vec(xPos, yPos),  module, MIDIpolyMPE::RVEL_OUTPUT));
		// Main PBend Out
		yPos = 251.f;
		xPos = 119.f;
		addOutput(createOutput<moDllzPortO>(Vec(xPos, yPos),  module, MIDIpolyMPE::PBEND_OUTPUT));
		//CC outputs
		yPos = 296.f;
		for ( int r = 0; r < 2; r++){
			xPos = 14.f;
			for ( int i = 0; i < 4; i++){
				addOutput(createOutput<moDllzPortO>(Vec(xPos, yPos),  module, MIDIpolyMPE::MM_OUTPUT + i + r * 4));
				xPos += 33.f;
			}
			yPos += 40.f;
		}
		//		if (module){ //////TEST VALUE TOP
		//			ValueTestLCD *MccLCD = createWidget<ValueTestLCD>(Vec(0.f,0.f));
		//			MccLCD->box.size = {60.f, 15.f};
		//			MccLCD->testval = &module->TEST_FLOAT;
		//			addChild(MccLCD);
		//		}
	}
};
Model *modelMIDIpolyMPE = createModel<MIDIpolyMPE, MIDIpolyMPEWidget>("MIDIpolyMPE");
