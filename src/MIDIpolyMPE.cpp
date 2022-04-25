/*
 MIDIpolyMPE.cpp : Midi to 16ch CV with MPE and Polyphonic modes
 
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
struct MIDIpolyMPE : Module {
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
	enum PolyMode {
		ROTATE_MODE,
		REUSE_MODE,
		RESET_MODE,
		REASSIGN_MODE,
		SHARE_MODE,
		DUAL_MODE,
		UNISON_MODE,
		UNISONLWR_MODE,
		UNISONUPR_MODE,
		MPE_MODE,
		MPEROLI_MODE,
		MPEHAKEN_MODE,
		CHANNEL_MODE,
		NUM_POLYMODES
	};
	enum ParaMapEnum {
		_noSelection,
		PM_polyModeId,
		PM_numVoCh,
		PM_xpander,
		PM_stealMode,
		PM_spread,
		PM_noteMin,
		PM_noteMax,
		PM_velMin,
		PM_velMax,
		PM_pbMPE,
		PM_detune,
		PM_mpeYcc,
		PM_mpeZcc,
		PM_mpeRvPb,
		PM_transpose,
		PM_pbDwn,
		PM_pbUp,
		ENUMS(PM_midiCCs,8),
		NUM_ParaMapEnum
	};
	float dataMin[ParaMapEnum::NUM_ParaMapEnum];
	float dataMax[ParaMapEnum::NUM_ParaMapEnum];
	int dataMap [ParaMapEnum::NUM_ParaMapEnum];
	enum stealModes {
		STEAL_OLDEST,
		STEAL_NEWEST,
		STEAL_LOWEST,
		STEAL_HIGHEST,
		STEAL_NO,
		NUM_STEALMODES
	};
	midi::InputQueue midiInput;//////MIDI
	int MPEmasterCh = 0;// 0 ~ 15
	unsigned char midiActivity = 0;
	bool resetMidi = false;
	int mdriverJx = -1;
	int mchannelJx = -1;
	std::string mdeviceJx;
	std::vector<uint8_t> cachedNotes;// Stolen notes (UNISON_MODE and REASSIGN_MODE cache all played)
	std::vector<uint8_t> cachedMPE[32];// MPE stolen notes
	uint8_t notes[32] = {0};
	float vels[32] = {0.f};
	float rvels[32] = {0.f};
	float naft[128] ={0.f};
	float mpex[16] = {0.f};
	float mpey[16] = {0.f};
	float mpez[16] = {0.f};
	float xpitch[16] = {0.f};
	float *mappedRvelPb = rvels;
	uint8_t mpePlusLB[16] = {0};
	float outpitch[32] = {0.f};
	float outgates[32] = {0.f};
	bool gates[32] = {false};
	bool hold[32] = {false};
	float drift[32] = {0.f};
	float spread[32] = {0.f};
	bool pedal = false;
	int rotateIndex = 0;
	int stealIndex = 0;
	float mrPBend = 0.f;
	float pbUpDwn = 0.f;
	float PM_midiCCsValues[129] = {0.f};//0~127=ccs 128=chAft
	const float scaledByte = 10.f/127.f;
	const float scaledDualBytes = 10.f/16383.f;
	const float scaledPBendDn = 5.f/8192.f;
	const float scaledPBendUp = 5.f/8191.f;
	uint8_t noteMinIx = 0;
	uint8_t noteMaxIx = 0;
	int outofbounds = 0;
	int outboundTimer = 0;
	int nVoCh = 8;
	int outCh = 8;
	sharedXpander *sharedXp = new sharedXpander(); 	//expander
	int xpanderId = -1;
	bool xpandAlt = false;
	int xpand2x = 1;
	int learnId = 0;
	int cursorIx = 0;// main cursor
	int cursorIxLast = 1;
	int selectedmidich = 0;
	bool MPEmode = false;
	bool UNImode = false;
	bool Ycanedit = true;
	bool Zcanedit = true;
	bool RPcanedit = false;
	int NOTEAFT = 129;///pointers for displayed names index
	int Detune130 = 130;
	int Yroli = 131;
	int Zroli = 132;
	int Yhaken = 133;
	int Zhaken = 134;
	int one = 1;
	int zero = 0;
	int *Z_ptr = &NOTEAFT;
	int *Y_ptr = &Yhaken;
	int *RP_ptr = &zero;
	int ProcessFrame = 0;//process clock
	int onFocus = 0;
	int PROCESS_RATE = static_cast<int>(APP->engine->getSampleRate() * 0.0005); //.5ms
	const int Focus_SEC =  10 * 2000;///^^^ sec * 2000 (process rate .5 ms)
	const int dataSetup_SEC = 500; //250ms to set dataKnob when selecting param without triggering change
	dsp::ExponentialFilter MPExFilter[16];
	dsp::ExponentialFilter MPEyFilter[16];
	dsp::ExponentialFilter MPEzFilter[16];
	dsp::ExponentialFilter MCCsFilter[8];
	dsp::ExponentialFilter mrPBendFilter;
	dsp::PulseGenerator reTrigger[32];
	dsp::SchmittTrigger PlusOneTrigger;
	dsp::SchmittTrigger MinusOneTrigger;
	std::string ccLongNames[135];
	MIDIpolyMPE() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configButton(PLUSONE_PARAM,"(Selected) +1");
		configButton(MINUSONE_PARAM,"(Selected) -1");
		configSwitch(SUSTHOLD_PARAM, false, true, false,"cc64 Sustain hold",{"off","ON"});
		configSwitch(RETRIG_PARAM, 0.f, 2.f, 1.f,"Retrigger",{"FirstOn","NotesOn","NotesOn+Recovered"});
		configParam(DATAKNOB_PARAM, 999.9f, 1000.1f, 1000.f,"Data Entry");
		configParam(DATAKNOB_PARAM, 0.f, 1.f, 0.f,"Data Entry");
		configOutput(X_OUTPUT, "1V/oct pitch");
		configOutput(VEL_OUTPUT, "0~10V Velocity");
		configOutput(GATE_OUTPUT, "10V Gate");
		configOutput(PBEND_OUTPUT, "Master Pitch Bend");
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
		ccLongNames[130].assign("Random Detuned");
		ccLongNames[131].assign("Slide");//Roli
		ccLongNames[132].assign("Press");//Roli
		ccLongNames[133].assign("Y Axis 14bit");
		ccLongNames[134].assign("Z Axis 14bit");
		setFilters();
		onReset();
	}
	~MIDIpolyMPE() {
		if (xpanderId > 0) sharedXpander::xpandch[xpanderId] = 0;
		delete sharedXp;
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
		json_object_set_new(rootJ, "polyModeId", json_integer(dataMap[PM_polyModeId]));
		json_object_set_new(rootJ, "pbDwn", json_integer(dataMap[PM_pbDwn]));
		json_object_set_new(rootJ, "pbUp", json_integer(dataMap[PM_pbUp]));
		json_object_set_new(rootJ, "pbMPE", json_integer(dataMap[PM_pbMPE]));
		json_object_set_new(rootJ, "mpeRvPb", json_integer(dataMap[PM_mpeRvPb]));
		json_object_set_new(rootJ, "numVoCh", json_integer(dataMap[PM_numVoCh]));
		json_object_set_new(rootJ, "stealMode", json_integer(dataMap[PM_stealMode]));
		json_object_set_new(rootJ, "MPEmasterCh", json_integer(MPEmasterCh));
		json_object_set_new(rootJ, "midiAcc", json_integer(dataMap[PM_midiCCs + 0]));
		json_object_set_new(rootJ, "midiBcc", json_integer(dataMap[PM_midiCCs + 1]));
		json_object_set_new(rootJ, "midiCcc", json_integer(dataMap[PM_midiCCs + 2]));
		json_object_set_new(rootJ, "midiDcc", json_integer(dataMap[PM_midiCCs + 3]));
		json_object_set_new(rootJ, "midiEcc", json_integer(dataMap[PM_midiCCs + 4]));
		json_object_set_new(rootJ, "midiFcc", json_integer(dataMap[PM_midiCCs + 5]));
		json_object_set_new(rootJ, "midiGcc", json_integer(dataMap[PM_midiCCs + 6]));
		json_object_set_new(rootJ, "midiHcc", json_integer(dataMap[PM_midiCCs + 7]));
		json_object_set_new(rootJ, "mpeYcc", json_integer(dataMap[PM_mpeYcc]));
		json_object_set_new(rootJ, "mpeZcc", json_integer(dataMap[PM_mpeZcc]));
		json_object_set_new(rootJ, "detune", json_integer(dataMap[PM_detune]));
		json_object_set_new(rootJ, "spread", json_integer(dataMap[PM_spread]));
		json_object_set_new(rootJ, "trnsps", json_integer(dataMap[PM_transpose]));
		json_object_set_new(rootJ, "noteMin", json_integer(dataMap[PM_noteMin]));
		json_object_set_new(rootJ, "noteMax", json_integer(dataMap[PM_noteMax]));
		json_object_set_new(rootJ, "velMin", json_integer(dataMap[PM_velMin]));
		json_object_set_new(rootJ, "velMax", json_integer(dataMap[PM_velMax]));
		json_object_set_new(rootJ, "xpander", json_integer(dataMap[PM_xpander]));
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
		json_t *PM_polyModeIdJ = json_object_get(rootJ, "polyModeId");
		if (PM_polyModeIdJ) dataMap[PM_polyModeId] = json_integer_value(PM_polyModeIdJ);
		MPEmode = (dataMap[PM_polyModeId] < ROTATE_MODE);
		json_t *PM_pbDwnJ = json_object_get(rootJ, "pbDwn");
		if (PM_pbDwnJ) dataMap[PM_pbDwn] = json_integer_value(PM_pbDwnJ);
		json_t *PM_pbUpJ = json_object_get(rootJ, "pbUp");
		if (PM_pbUpJ) dataMap[PM_pbUp] = json_integer_value(PM_pbUpJ);
		json_t *PM_pbMPEJ = json_object_get(rootJ, "pbMPE");
		if (PM_pbMPEJ) dataMap[PM_pbMPE] = json_integer_value(PM_pbMPEJ);
		json_t *PM_mpeRvPbJ = json_object_get(rootJ, "mpeRvPb");
		if (PM_mpeRvPbJ) dataMap[PM_mpeRvPb] = json_integer_value(PM_mpeRvPbJ);
		json_t *numVoJ = json_object_get(rootJ, "numVoCh");
		if (numVoJ) dataMap[PM_numVoCh] = json_integer_value(numVoJ);
		json_t *stealModeJ = json_object_get(rootJ, "stealMode");
		if (stealModeJ) dataMap[PM_stealMode] = json_integer_value(stealModeJ);
		json_t *MPEmasterChJ = json_object_get(rootJ, "MPEmasterCh");
		if (MPEmasterChJ) MPEmasterCh = json_integer_value(MPEmasterChJ);
		json_t *midiAccJ = json_object_get(rootJ, "midiAcc");
		if (midiAccJ) dataMap[PM_midiCCs + 0] = json_integer_value(midiAccJ);
		json_t *midiBccJ = json_object_get(rootJ, "midiBcc");
		if (midiBccJ) dataMap[PM_midiCCs + 1] = json_integer_value(midiBccJ);
		json_t *midiCccJ = json_object_get(rootJ, "midiCcc");
		if (midiCccJ) dataMap[PM_midiCCs + 2] = json_integer_value(midiCccJ);
		json_t *midiDccJ = json_object_get(rootJ, "midiDcc");
		if (midiDccJ) dataMap[PM_midiCCs + 3] = json_integer_value(midiDccJ);
		json_t *midiEccJ = json_object_get(rootJ, "midiEcc");
		if (midiEccJ) dataMap[PM_midiCCs + 4] = json_integer_value(midiEccJ);
		json_t *midiFccJ = json_object_get(rootJ, "midiFcc");
		if (midiFccJ) dataMap[PM_midiCCs + 5] = json_integer_value(midiFccJ);
		json_t *midiFccG = json_object_get(rootJ, "midiGcc");
		if (midiFccG) dataMap[PM_midiCCs + 6] = json_integer_value(midiFccG);
		json_t *midiFccH = json_object_get(rootJ, "midiHcc");
		if (midiFccH) dataMap[PM_midiCCs + 7] = json_integer_value(midiFccH);
		json_t *PM_mpeYccJ = json_object_get(rootJ, "mpeYcc");
		if (PM_mpeYccJ) dataMap[PM_mpeYcc] = json_integer_value(PM_mpeYccJ);
		json_t *PM_mpeZccJ = json_object_get(rootJ, "mpeZcc");
		if (PM_mpeZccJ) dataMap[PM_mpeZcc] = json_integer_value(PM_mpeZccJ);
		json_t *PM_detuneJ = json_object_get(rootJ, "detune");
		if (PM_detuneJ) dataMap[PM_detune] = json_integer_value(PM_detuneJ);
		json_t *PM_spreadJ = json_object_get(rootJ, "spread");
		if (PM_spreadJ) dataMap[PM_spread] = json_integer_value(PM_spreadJ);
		json_t *trnspsJ = json_object_get(rootJ, "trnsps");
		if (trnspsJ) dataMap[PM_transpose] = json_integer_value(trnspsJ);
		json_t *PM_noteMinJ = json_object_get(rootJ, "noteMin");
		if (PM_noteMinJ) dataMap[PM_noteMin] = json_integer_value(PM_noteMinJ);
		json_t *PM_noteMaxJ = json_object_get(rootJ, "noteMax");
		if (PM_noteMaxJ) dataMap[PM_noteMax] = json_integer_value(PM_noteMaxJ);
		json_t *PM_velMinJ = json_object_get(rootJ, "velMin");
		if (PM_velMinJ) dataMap[PM_velMin] = json_integer_value(PM_velMinJ);
		json_t *PM_velMaxJ = json_object_get(rootJ, "velMax");
		if (PM_velMaxJ) dataMap[PM_velMax] = json_integer_value(PM_velMaxJ);
		json_t *PM_xpanderJ = json_object_get(rootJ, "xpander");
		if (PM_xpanderJ) {
			int savedxp = json_integer_value(PM_xpanderJ);
			if (savedxp > -1){//check if already w open channels
				int xpId = (savedxp - 1) / 2 ;
				dataMap[PM_xpander] = (sharedXpander::xpandch[xpId] > 0)? 0 : savedxp;
			}
		}
	}
	/////////////////////////////////////////////////////////////////////////////////////////
	void onRandomize(const RandomizeEvent& e) override {}
	void onSampleRateChange() override {
		PROCESS_RATE = static_cast<int>(APP->engine->getSampleRate() * 0.0005); //.5ms
		setFilters();
		resetVoices();
	}
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
			outputInfos[MM_OUTPUT + i]->name =  ccLongNames[dataMap[PM_midiCCs + i]];
		}
		disableDataKnob();
	}
	void resetVoices(){
		initVoices();
		UNImode = false;
		switch (dataMap[PM_polyModeId]){
			case MPE_MODE :/// normal MPE
			case CHANNEL_MODE: {//w/o Master ch
				MPEmode = true;
				outCh = 1;//mpe #voiceCh 1 + dynamically grow when new ch received
				Y_ptr = &dataMap[PM_mpeYcc];
				Z_ptr = &dataMap[PM_mpeZcc];
				RP_ptr = &dataMap[PM_mpeRvPb];
				Ycanedit = true;
				Zcanedit = true;
				RPcanedit = true;
				outputInfos[Y_OUTPUT]->name = "MPE Y";
				outputInfos[Z_OUTPUT]->name = "MPE Z";
				if (dataMap[PM_mpeRvPb] > 0){
					outputInfos[RVEL_OUTPUT]->name = "Channel PitchBend";
					mappedRvelPb = xpitch;
				}else{
					outputInfos[RVEL_OUTPUT]->name = "Release Velocity";
					mappedRvelPb = rvels;
				}
			}break;
			case MPEROLI_MODE :{// Roli names
				MPEmode = true;
				outCh = 1;//mpe #voiceCh 1 + dynamically grow when new ch received
				Y_ptr = &Yroli;//131;
				Z_ptr = &Zroli;//132;
				dataMap[PM_mpeYcc] = 74;
				dataMap[PM_mpeZcc] = 128;
				RP_ptr = &dataMap[PM_mpeRvPb];
				Ycanedit = false;
				Zcanedit = false;
				RPcanedit = true;
				outputInfos[Y_OUTPUT]->name = "Slide";
				outputInfos[Z_OUTPUT]->name = "Press";
				if (dataMap[PM_mpeRvPb] > 0){
					outputInfos[RVEL_OUTPUT]->name = "Channel PitchBend";
					mappedRvelPb = xpitch;
				}else{
					outputInfos[RVEL_OUTPUT]->name = "Release Velocity";
					mappedRvelPb = rvels;
				}
			}break;
			case MPEHAKEN_MODE :{// Haken MPE Plus
				MPEmode = true;
				outCh = 1;//mpe #voiceCh 1 + dynamically grow when new ch received
				Y_ptr = &Yhaken;//133;
				Z_ptr = &Zhaken;//134;
				RP_ptr = &one;
				Ycanedit = false;
				Zcanedit = false;
				RPcanedit = false;
				outputInfos[Y_OUTPUT]->name = "MPE Y 14bit";
				outputInfos[Z_OUTPUT]->name = "MPE Z 14bit";
				outputInfos[RVEL_OUTPUT]->name = "Channel PitchBend";
				mappedRvelPb = xpitch;
			}break;
			default: {
				MPEmode = false;
				UNImode = !((dataMap[PM_polyModeId] < SHARE_MODE) || (dataMap[PM_polyModeId] > UNISONUPR_MODE));
				if ((dataMap[PM_polyModeId]==DUAL_MODE) && (dataMap[PM_numVoCh] < 2)) dataMap[PM_numVoCh] = 2;
				Y_ptr = &Detune130;
				Z_ptr = &NOTEAFT;//129 : note_aftertouch
				RP_ptr = &zero;//releaseVel
				Ycanedit = true;// editing RND detune
				Zcanedit = false;
				RPcanedit = false;
				outCh = dataMap[PM_numVoCh];
				if(xpanderId > -1) sharedXpander::xpandch[xpanderId] = 0;
				if (dataMap[PM_xpander] > 0){
					xpanderId = (dataMap[PM_xpander] - 1) / 2 ;
					xpandAlt = dataMap[PM_xpander]  % 2 != 1;
					nVoCh = outCh * 2;
					sharedXpander::xpandch[xpanderId] = outCh;
					xpand2x = 2;
				}else{
					xpanderId = -1;
					xpandAlt = false;
					nVoCh = outCh;
					xpand2x = 1;
				}
				outputInfos[Y_OUTPUT]->name = "1V/Oct detuned";
				outputInfos[Z_OUTPUT]->name = "Note Aftertouch";
				outputInfos[RVEL_OUTPUT]->name = "Release Velocity";
			}
		}
		if (UNImode) spreadVoices();
		for (int i=0; i < 8; i++){
			PM_midiCCsValues[dataMap[PM_midiCCs + i]] = 0.f;
		}
		midiActivity = 127;
		resetMidi = false;
	}
	void initVoices(){
		pedal = false;
		lights[SUSTHOLD_LIGHT].setBrightness(0.f);
		for (int i = 0; i < 32; i++) {
			notes[i] = 60;
			gates[i] = false;
			hold[i] = false;
			vels[i] = 0.f;
			rvels[i] = 0.f;
			outpitch[i] = 0.f;
			outgates[i] = 0.f;
		}
		for (int i = 0; i < 16; i++) {
			cachedMPE[i].clear();
			mpePlusLB[i] = 0;
			xpitch[i] = 0.f;
			mpex[i] = 0.f;
			mpey[i] = 0.f;
			mpez[i] = 0.f;
			lights[CH_LIGHT+ i].setBrightness(0.f);
			outputs[GATE_OUTPUT].setVoltage(0.f, i);
		}
		rotateIndex = -1;
		cachedNotes.clear();
	}
	void initParamsSettings(){
		dataMap[_noSelection] = 0;
		dataMap[PM_polyModeId] = ROTATE_MODE;
		dataMin[PM_polyModeId] = 0.f;
		dataMax[PM_polyModeId] = static_cast<float>(NUM_POLYMODES - 1);
		dataMap[PM_numVoCh] = 8;
		dataMin[PM_numVoCh] = 1.f;
		dataMax[PM_numVoCh] = 16.f;
		dataMap[PM_xpander] = 0.f;
		dataMin[PM_xpander] = 0.f;
		dataMax[PM_xpander] = 8.f;
		dataMap[PM_stealMode] = 0;
		dataMin[PM_stealMode] = 0.f;
		dataMax[PM_stealMode] = 4.f;
		dataMap[PM_spread] = 0;
		dataMin[PM_spread] = 0.f;
		dataMax[PM_spread] = 250.f;
		dataMap[PM_pbDwn] = -12;
		dataMin[PM_pbDwn] = -60.f;
		dataMax[PM_pbDwn] = 60.f;
		dataMap[PM_pbUp] = 12;
		dataMin[PM_pbUp] = -60.f;
		dataMax[PM_pbUp] = 60.f;
		dataMap[PM_pbMPE] = 96;
		dataMin[PM_pbMPE] = 0.f;
		dataMax[PM_pbMPE] = 96.f;
		dataMap[PM_mpeRvPb] = 0;//0 rel, 1 PB
		dataMin[PM_mpeRvPb] = 0.f;
		dataMax[PM_mpeRvPb] = 1.f;
		dataMap[PM_detune] = 10;
		dataMin[PM_detune] = 0.f;
		dataMax[PM_detune] = 99.f;
		dataMap[PM_transpose] = 0;
		dataMin[PM_transpose] = -48.f;
		dataMax[PM_transpose] = 48.f;
		dataMap[PM_mpeYcc] = 74;
		dataMin[PM_mpeYcc] = 0.f;
		dataMax[PM_mpeYcc] = 128.f;
		dataMap[PM_mpeZcc] = 128; //chn aftT
		dataMin[PM_mpeZcc] = 0.f;
		dataMax[PM_mpeZcc] = 128.f;
		dataMap[PM_noteMin] = 0;
		dataMin[PM_noteMin] = 0.f;
		dataMax[PM_noteMin] = 127.f;
		dataMap[PM_noteMax] = 127;
		dataMin[PM_noteMax] = 1.f;
		dataMax[PM_noteMax] = 127.f;
		dataMap[PM_velMin] = 1;
		dataMin[PM_velMin] = 1.f;
		dataMax[PM_velMin] = 127.f;
		dataMap[PM_velMax] = 127;
		dataMin[PM_velMax] = 1.f;
		dataMax[PM_velMax] = 127.f;
		dataMap[PM_midiCCs + 0] = 128;//chn aftT
		dataMap[PM_midiCCs + 1] = 1;//ModW
		dataMap[PM_midiCCs + 2] = 2;//BrCt
		dataMap[PM_midiCCs + 3] = 7;//Vol
		dataMap[PM_midiCCs + 4] = 10;//Pan
		dataMap[PM_midiCCs + 5] = 11;//Exp
		dataMap[PM_midiCCs + 6] = 74;//FltCtff
		dataMap[PM_midiCCs + 7] = 64;//Sust
		for (int i = 0; i< 8; i++){
			dataMin[PM_midiCCs + i] = 0.f;
			dataMax[PM_midiCCs + i] = 128.f;
		}
		Z_ptr = &NOTEAFT;
		Y_ptr = &Detune130;
		RP_ptr = &zero;
		xpanderId = -1;
	}
	void setFilters(){
		float lambdaf = 500.f * APP->engine->getSampleTime();
		for (int i = 0; i < 16; i++) {
			MPExFilter[i].lambda = lambdaf;
			MPEyFilter[i].lambda = lambdaf;
			MPEzFilter[i].lambda = lambdaf;
		}
		for (int i=0; i < 8; i++){
			MCCsFilter[i].lambda = lambdaf;
		}
		mrPBendFilter.lambda = lambdaf;
	}
	float bytetoVolts(uint8_t mididata){
		return static_cast<float>(mididata) * scaledByte;
	}
	float dualbytetoVolts(uint8_t mididata){
		return static_cast<float>(mididata) * scaledDualBytes;
	}
	float pbendtoVolts(int16_t midipb){
		if (midipb < 0){
			pbUpDwn =  dataMap[PM_pbDwn] / -60.f;
			return static_cast<float>(midipb) * scaledPBendDn;
		}else {
			pbUpDwn = dataMap[PM_pbUp] / 60.f;
			return static_cast<float>(midipb) * scaledPBendUp;
		}
	}
	///////////////// P O L Y  I N D E X and S T E A L ////////////////////////////////////////////////////////////////////////
	int getPolyIndex(int nowIndex) {
		for (int i = 0; i < nVoCh; i++) {
			nowIndex = (nowIndex + 1) % nVoCh;
			if (!gates[nowIndex]) {
				stealIndex = nowIndex;
				return nowIndex;
			}
		}// All taken = send note to cache and steal voice
		switch (dataMap[PM_stealMode]){
			case STEAL_OLDEST :{
				stealIndex = (stealIndex + 1) % nVoCh;
			}break;
			case STEAL_NEWEST : // = keep latest stealIndex
			break;
			case STEAL_LOWEST :{
				int lower = 127;
				for (int i = 0; i < nVoCh; i++) {
					if (notes[i]< lower) {
						lower = notes[i];
						stealIndex = i;
					}
				}
			}break;
			case STEAL_HIGHEST :{
				int upper = 0;
				for (int i = 0; i < nVoCh; i++) {
					if (notes[i]> upper) {
						upper = notes[i];
						stealIndex = i;
					}
				}
			}break;
			default:{///NO STEAL // R E T U R N
				return -1;
			}
		}
		cachedNotes.push_back(notes[stealIndex]);
		return stealIndex;
	}
	//////////////// M I D I    N O T E   O N /////////////////////////////////////////////////////////////////////////
	void pressNote(uint8_t channel, uint8_t note, uint8_t vel) {
		switch (learnId){
			case 0:
				break;
			case PM_noteMin:{
				dataMap[PM_noteMin] = note;
				if (dataMap[PM_noteMax] < note) dataMap[PM_noteMax] = note;
				onFocus = 1;
			}break;
			case PM_noteMax:{
				dataMap[PM_noteMax] = note;
				if (dataMap[PM_noteMin] > note) dataMap[PM_noteMin] = note;
				onFocus = 1;
			}break;
			case PM_velMin:{
				dataMap[PM_velMin] = vel;
				if (dataMap[PM_velMax] < vel) dataMap[PM_velMax] = vel;
				onFocus = 1;
			}break;
			case PM_velMax:{
				dataMap[PM_velMax] = vel;
				if (dataMap[PM_velMin] > vel) dataMap[PM_velMin] = vel;
				onFocus = 1;
			}break;
		}
		///////// escape if notes out of range
		if (note < dataMap[PM_noteMin]){
			outofbounds = 1;
			outboundTimer = 0;
			goto midiActivity;
		}
		if (note > dataMap[PM_noteMax]){
			outofbounds = 2;
			outboundTimer = 0;
			goto midiActivity;
		}
		if (vel < dataMap[PM_velMin]){
			outofbounds = 3;
			outboundTimer = 0;
			goto midiActivity;
		}
		if (vel > dataMap[PM_velMax]){
			outofbounds = 4;
			outboundTimer = 0;
			goto midiActivity;
		}
		outofbounds = 0;
		/////////////////
		switch (dataMap[PM_polyModeId]) {// Set notes and gates
			case MPE_MODE:
			case MPEROLI_MODE:
			case MPEHAKEN_MODE:{
				if (channel == MPEmasterCh) return; /////  R E T U R N !!!!!!!
				if (channel + 1 > outCh) outCh = channel + 1;
				rotateIndex = channel; // ASSIGN VOICE Index
				if (gates[channel]) cachedMPE[channel].push_back(notes[channel]);///if
			} break;
			case CHANNEL_MODE:{
				if (channel + 1 > outCh) outCh = channel + 1;
				rotateIndex = channel; // ASSIGN VOICE Index
				if (gates[channel]) cachedMPE[channel].push_back(notes[channel]);///if
			} break;
			case ROTATE_MODE: {
				int tryIndex = getPolyIndex(rotateIndex);
				if (tryIndex < 0) goto midiActivity;
				rotateIndex = tryIndex;
			} break;
			case REUSE_MODE: {
				bool reuse = false;
				for (int i = 0; i < nVoCh; i++) {
					if (notes[i] == note) {
						rotateIndex = i;
						reuse = true;
						break;
					}
				}
				if (!reuse) {
					int tryIndex = getPolyIndex(rotateIndex);
					if (tryIndex < 0) goto midiActivity;
					rotateIndex = tryIndex;
				}
			} break;
			case RESET_MODE:
			case REASSIGN_MODE: {
				int tryIndex = getPolyIndex(-1);
				if (tryIndex < 0) goto midiActivity;
				rotateIndex = tryIndex;
			} break;
			case SHARE_MODE: {
				cachedNotes.push_back(note);
				bool retrignow = (params[RETRIG_PARAM].getValue() > 0.f);
				mapShare(retrignow, true, vel);
			} goto midiActivity;//break;
			case DUAL_MODE: {
				cachedNotes.push_back(note);
				bool retrignow = (params[RETRIG_PARAM].getValue() > 0.f);
				mapDual(retrignow, true, vel);
			} goto midiActivity;//break;
			case UNISON_MODE: {
				cachedNotes.push_back(note);
				bool retrignow = (params[RETRIG_PARAM].getValue() > 0.f);
				mapUnison(retrignow, note, vel);
			} goto midiActivity;//break;
			case UNISONLWR_MODE: {
				cachedNotes.push_back(note);
				uint8_t lnote = *min_element(cachedNotes.begin(),cachedNotes.end());
				bool retrignow = ((params[RETRIG_PARAM].getValue() > 0.f) && (lnote < notes[0]));
				mapUnison(retrignow, lnote, vel);
			} goto midiActivity;//break;
			case UNISONUPR_MODE:{
				cachedNotes.push_back(note);
				uint8_t unote = *max_element(cachedNotes.begin(),cachedNotes.end());
				bool retrignow = (params[RETRIG_PARAM].getValue() > 0.f) && (unote > notes[0]);
				mapUnison(retrignow, unote, vel);
			} goto midiActivity;//break;
			default: break;
		}
		notes[rotateIndex] = note;
		vels[rotateIndex] = bytetoVolts(vel);
		gates[rotateIndex] = true;
		// Set notes and gates
		if (params[RETRIG_PARAM].getValue() > 0.f) reTrigger[rotateIndex].trigger(1e-3);
		hold[rotateIndex] = pedal;
		detuneVoice(rotateIndex);
	midiActivity:
		midiActivity = vel;
	}
	//////////////// M I D I    N O T E   O F F //////////////////////////////////////////////////////////////////////
	void releaseNote(uint8_t channel, uint8_t note, uint8_t vel) {
		bool backnote = false;
		if (!MPEmode) {
			// Remove the note
			if (!cachedNotes.empty()) backnote = (note == cachedNotes.back());
			std::vector<uint8_t>::iterator it = std::find(cachedNotes.begin(), cachedNotes.end(), note);
			if (it != cachedNotes.end()) cachedNotes.erase(it);
		}else{
			if (channel == MPEmasterCh && dataMap[PM_polyModeId]!= CHANNEL_MODE) return;/////// R E T U R N //////////////get channel from dynamic map
			std::vector<uint8_t>::iterator it = std::find(cachedMPE[channel].begin(), cachedMPE[channel].end(), note);
			if (it != cachedMPE[channel].end()) cachedMPE[channel].erase(it);
		}
		switch (dataMap[PM_polyModeId]) {// default ROTATE_MODE REUSE_MODE RESET_MODE
			default: {
				for (int i = 0; i < nVoCh; i++) {
					if (notes[i] == note) {
						gates[i] = false;
						if (!cachedNotes.empty()) {
							notes[i] = cachedNotes.back();
							cachedNotes.pop_back();
							gates[i] = true;
							//Retrigger recovered notes
							if (params[RETRIG_PARAM].getValue() > 1.f) reTrigger[i].trigger(1e-3);
						}
						rvels[i] = bytetoVolts(vel);
					}
				}
			} break;
			case MPE_MODE:
			case MPEROLI_MODE:
			case MPEHAKEN_MODE:
			case CHANNEL_MODE:{
				if (note == notes[channel]) {
					if (hold[channel]) {
						gates[channel] = false;
					}/// check for cached notes on MPE buffers...
					else {
						if (!cachedMPE[channel].empty()) {
							notes[channel] = cachedMPE[channel].back();
							cachedMPE[channel].pop_back();
							//Retrigger recovered notes
							if (params[RETRIG_PARAM].getValue() > 1.f) reTrigger[channel].trigger(1e-3);
						}else gates[channel] = false;
					}
					rvels[channel] = bytetoVolts(vel);
				}
			} break;
			case REASSIGN_MODE: {
				for( int i = 0 ; i < nVoCh ; i++){
					if (notes[i] == note) {
						if (hold[i]){/// don't restack if pedal hold
							gates[i] = false;
						}else {
							int clearii = i;///restack upper voices
							for (int ii = i; ii < nVoCh - 1; ii++){
								if (gates[ii+1]) { /// if next is active...
									notes[ii] = notes[ii + 1];
									clearii = ii + 1;
								}else{///next off
									if (!cachedNotes.empty()) {
										notes[ii] = cachedNotes.back();
										cachedNotes.pop_back();
										//Retrigger recovered notes
										if (params[RETRIG_PARAM].getValue() > 1.f) reTrigger[channel].trigger(1e-3);
										clearii = ii + 1;
										break;
									}else{//no notes cached
										gates[ii] = false;
										clearii = ii;
										break;
									}
								}
							}
							for (int iii = clearii; iii < nVoCh; iii++){
								gates[iii] = false;//gates off upper
							}
						}
						break;
					}
				}//rvels[i]...(?)... it may be better not to reassign vels
			} break;
			case SHARE_MODE:{
				if (vel > 128) vel = 64;
				if (!cachedNotes.empty()) {
					bool retrignow = (params[RETRIG_PARAM].getValue() > 1.f);
					mapShare(retrignow, false, vel);
				}else emptyVoices(vel);
			}break;
			case DUAL_MODE: {
				if (vel > 128) vel = 64;
				if (!cachedNotes.empty()) {
					bool retrignow = (params[RETRIG_PARAM].getValue() > 1.f);
					mapDual(retrignow, false, vel);
				}else emptyVoices(vel);
			}break;
			case UNISON_MODE: {
				if (!cachedNotes.empty()) {
					uint8_t backnote = cachedNotes.back();
					bool retrignow = (params[RETRIG_PARAM].getValue() > 1.f) && (backnote != notes[0]);
					mapUnison(retrignow, backnote, vel);
				}else emptyVoices(vel);
			} break;
			case UNISONLWR_MODE: {
				if (vel > 128) vel = 64;
				if (!cachedNotes.empty()) {
					uint8_t lnote = *min_element(cachedNotes.begin(),cachedNotes.end());
					bool retrignow = (params[RETRIG_PARAM].getValue() > 1.f) && (lnote != notes[0]);
					mapUnison(retrignow, lnote, vel);
				}else emptyVoices(vel);
			} break;
			case UNISONUPR_MODE: {
				if (vel > 128) vel = 64;
				if (!cachedNotes.empty()) {
					uint8_t unote = *max_element(cachedNotes.begin(),cachedNotes.end());
					bool retrignow = (params[RETRIG_PARAM].getValue() == 2.f) && (unote != notes[0]);
					mapUnison(retrignow, unote, vel);
				}else emptyVoices(vel);
			} break;
		}
		midiActivity = vel;
	}
	void detuneVoice(int v){
			drift[v] = static_cast<float>((rand() % 200  - 100) * dataMap[PM_detune]) * 8.3333e-6;
	}
	void spreadVoices(){
		for (int i = 0; i < nVoCh; i++){
			spread[i] = static_cast<float>(i * ( 1 - ((i % 2) * 2)) * dataMap[PM_spread]) * 8.3333e-5;
		}
	}
	void mapDual(bool retrig, bool keyon, uint8_t vel){
		uint8_t noteupper = 0;
		uint8_t notelower = 120;
		for (uint8_t & el : cachedNotes) {
			if (el > noteupper) noteupper = el;
			if (el < notelower) notelower = el;
		}
		for (int i = 0; i < nVoCh; i+=2) {
			if (notes[i]!= notelower || (keyon && !gates[i])){
				notes[i] = notelower;
				vels[i] = bytetoVolts(vel);
				gates[i] = true;
				hold[i] = pedal;
				detuneVoice(i);
				if (retrig) reTrigger[i].trigger(1e-3);
			}
		}
		for (int i = 1; i < nVoCh; i+=2) {
			if (notes[i]!= noteupper || (keyon && !gates[i])){
				notes[i] = noteupper;
				vels[i] = bytetoVolts(vel);
				gates[i] = true;
				hold[i] = pedal;
				detuneVoice(i);
				if (retrig) reTrigger[i].trigger(1e-3);
			}
		}
	}
	void mapShare(bool retrig, bool keyon, uint8_t vel){
		uint8_t notecached = 0;
		int cachesize = cachedNotes.size();
		for (int i = 0; i < nVoCh; i++) {
			notecached = cachedNotes[i % cachesize];
			if (notes[i]!= notecached || (keyon && !gates[i])){
				notes[i] = notecached;
				vels[i] = bytetoVolts(vel);
				gates[i] = true;
				hold[i] = pedal;
				detuneVoice(i);
				if (retrig) reTrigger[i].trigger(1e-3);
			}
		}
	}
	void mapUnison(bool retrig, uint8_t noteu, uint8_t vel){
		for (int i = 0; i < nVoCh; i++) {
			notes[i] = noteu;
			vels[i] = bytetoVolts(vel);
			gates[i] = true;
			hold[i] = pedal;
			detuneVoice(i);
			if (retrig) reTrigger[i].trigger(1e-3);
		}
	}
	void emptyVoices(uint8_t rvel){
		for (int i = 0; i < nVoCh; i++) {
			gates[i] = false;
			rvels[i] = bytetoVolts(rvel);
		}
	}
	///////////   M I D I    M E S S A G E  ////////////////////////////////////////////////////////////////////////////
	void processMessage(midi::Message msg) {
		switch (msg.getStatus()) {
			case 0x8: { // note off //(msg.getChannel() == MPEmasterCh)) checked on releaseNote;
				releaseNote(msg.getChannel(), msg.getNote(), msg.getValue());
			} break;
			case 0x9: {// note on //(msg.getChannel() == MPEmasterCh)) checked on press/releaseNote;
				if (msg.getValue() > 0) pressNote(msg.getChannel(), msg.getNote(), msg.getValue());
				else releaseNote(msg.getChannel(), msg.getNote(), 64);//64 is the default rel velocity.
			} break;
			case 0xa: {// note (poly) aftertouch
				naft[msg.getNote()]=bytetoVolts(msg.getValue());
				midiActivity = msg.getValue();
			} break;
			case 0xd: {// channel aftertouch
				if (learnId > 0) {// learn enabled ???
					dataMap[learnId] = 128;
					onFocus = 1;
					return;/////// R E T U R N ////////////
				}
				if (!MPEmode){
						PM_midiCCsValues[128] = bytetoVolts(msg.getNote());
				}else{
					uint8_t channel = msg.getChannel();
					if ((dataMap[PM_polyModeId]!= CHANNEL_MODE)&&(channel == MPEmasterCh)) PM_midiCCsValues[128] = bytetoVolts(msg.getNote());
					if(dataMap[PM_polyModeId] != MPEHAKEN_MODE){
							if (dataMap[PM_mpeZcc] == 128) mpez[channel] = bytetoVolts(msg.getNote());
							if (dataMap[PM_mpeYcc] == 128) mpey[channel] = bytetoVolts(msg.getNote());
					}else{//HAKEN HI-RES
							mpez[channel] =  dualbytetoVolts(msg.getNote() * 128 + mpePlusLB[channel]);
							mpePlusLB[channel] = 0;
					}
				}
				midiActivity = msg.getNote();
			} break;
			case 0xe:{// pitch Bend
				int16_t pbval = (msg.getValue() * 128 + msg.getNote()  - 8192);
				if (!MPEmode){
					mrPBend = pbendtoVolts(pbval);
				}else{
					uint8_t channel = msg.getChannel();
					if (channel == MPEmasterCh) mrPBend = pbendtoVolts(pbval);
					if ((dataMap[PM_polyModeId] == CHANNEL_MODE) || (channel != MPEmasterCh)) mpex[channel] = pbendtoVolts(pbval);
				}
				midiActivity = msg.getValue();
			} break;
			case 0xb: { // CCs
				if (MPEmode){
					uint8_t channel = msg.getChannel();
					if (channel == MPEmasterCh){
						if (learnId > 0) {///////// LEARN CC MPE master
							dataMap[learnId] = msg.getNote();
							onFocus = 1;
							return;/////// R E T U R N ////////////
						}else processCC(msg);
						if (dataMap[PM_polyModeId] == CHANNEL_MODE){
							if (msg.getNote() == dataMap[PM_mpeYcc]){
								mpey[channel] = bytetoVolts(msg.getValue());
							}else if (msg.getNote() == dataMap[PM_mpeZcc]){
								mpez[channel] = bytetoVolts(msg.getValue());
							}
						}
					}else if (dataMap[PM_polyModeId] == MPEHAKEN_MODE){ //Continuum
						if (msg.getNote() == 87){
							mpePlusLB[channel] = msg.getValue();
						}else if (msg.getNote() == 74){
							mpey[channel] = dualbytetoVolts(msg.getValue() * 128 + mpePlusLB[channel]);
							mpePlusLB[channel] = 0;
						}
					}else if (msg.getNote() == dataMap[PM_mpeYcc]){//cc74 0x4a default
						mpey[channel] = bytetoVolts(msg.getValue());
					}else if (msg.getNote() == dataMap[PM_mpeZcc]){
						mpez[channel] = bytetoVolts(msg.getValue());
					}
				}else if (learnId) {///////// LEARN CC Poly
					dataMap[learnId] = msg.getNote();
					onFocus = 1;
					return;/////// R E T U R N ////////////
				}else processCC(msg);
				midiActivity = msg.getValue();
			} break;
			default: break;
		}
	}
	void processCC(midi::Message msg) {////////////// C C s /////////////////////////////////////////////////////////////////////////
		if (msg.getNote() ==  0x40) { //get note gets cc number...0x40=internal sust pedal
			if (msg.getValue() >= 64) pressPedal();
			else releasePedal();
		}
		PM_midiCCsValues[msg.getNote()] = bytetoVolts(msg.getValue());
	}
	void pressPedal() {////////////// P E D A L  ON /////////////////////////////////////////////////////////////////////////
		if (params[SUSTHOLD_PARAM].getValue() < 0.5f) {
			pedal = false;
			return;/////// R E T U R N ////////////
		}
		pedal = true;
		lights[SUSTHOLD_LIGHT].setBrightness(10.f);
			for (int i = 0; i < nVoCh; i++) {
				hold[i] = gates[i];
			}
	}
	void releasePedal() {////////////// P E D A L  OFF /////////////////////////////////////////////////////////////////////////
		if (params[SUSTHOLD_PARAM].getValue() < 0.5f) return;/////// R E T U R N ////////////
		pedal = false;
		lights[SUSTHOLD_LIGHT].setBrightness(0.f);
		if (MPEmode) {//recover cached notes for pressed keys
			for (int i = 0; i < nVoCh; i++) {
				if (gates[i] && !cachedMPE[i].empty()) {
					notes[i] = cachedMPE[i].back();
					cachedMPE[i].pop_back();
					bool retrignow = (params[RETRIG_PARAM].getValue() == 2.f);
					if (retrignow) reTrigger[i].trigger(1e-3);
				}
				hold[i] = false;
			}
		}else{//recover cached notes for pressed keys
			for (int i = 0; i < nVoCh; i++) {
				if (hold[i] && !cachedNotes.empty()) {
					if  (dataMap[PM_polyModeId] < REASSIGN_MODE){
						notes[i] = cachedNotes.back();
						cachedNotes.pop_back();
						bool retrignow = gates[i] && (params[RETRIG_PARAM].getValue() == 2.f);
						if (retrignow) reTrigger[i].trigger(1e-3);
					}
				}
				hold[i] = false;
			}
		}
	}
	////////////////////// CONFIG DATA KNOB //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void configDataKnob(){
		onFocus = Focus_SEC + dataSetup_SEC;//(250ms >focus_SEC) : doesn't trigger data update
		paramQuantities[DATAKNOB_PARAM]->minValue = dataMin[cursorIx];// - knobborder;
		paramQuantities[DATAKNOB_PARAM]->maxValue = dataMax[cursorIx] + 0.9f;
		paramQuantities[DATAKNOB_PARAM]->setSmoothValue(static_cast<float>(dataMap[cursorIx]));
	}
	void disableDataKnob(){
		paramQuantities[DATAKNOB_PARAM]->minValue = 1000.f;//1000 different to any value
		paramQuantities[DATAKNOB_PARAM]->maxValue = 1000.f;
		paramQuantities[DATAKNOB_PARAM]->defaultValue = 1000.f;
		paramQuantities[DATAKNOB_PARAM]->reset();
	}
	void setNewValue(float newval){///SET NEW VALUE
		int updatedval = static_cast<int>(newval);
		if (updatedval != dataMap[cursorIx]){
			if(cursorIx != PM_xpander) dataMap[cursorIx] = updatedval;//xpander checks before update
			switch (cursorIx){
				case PM_noteMin:
					dataMin[PM_noteMax] = static_cast<float>(updatedval);
					break;
				case PM_noteMax:
					dataMax[PM_noteMin] = static_cast<float>(updatedval);
					break;
				case PM_velMin:
					dataMin[PM_velMax] = static_cast<float>(updatedval);
					break;
				case PM_velMax:
					dataMax[PM_velMin] = static_cast<float>(updatedval);
					break;
				case PM_xpander:
					{
						if (updatedval > 0){
							int tryxp = (updatedval-1)/2;
							if (tryxp != xpanderId){
								if (sharedXpander::xpandch[tryxp] == 0){
									dataMap[PM_xpander] = updatedval;
									resetVoices();
								}
							}else{//same expander ID change mode Alt
								dataMap[PM_xpander] = updatedval;
								xpandAlt = dataMap[PM_xpander]  % 2 != 1;
							}
						}else{
							dataMap[PM_xpander] = 0;
							resetVoices();
						}
					}
					break;
				case PM_polyModeId:
				case PM_numVoCh:
					resetVoices();
					break;
				case PM_mpeRvPb:
					outputInfos[RVEL_OUTPUT]->name = (dataMap[PM_mpeRvPb] > 0)? "Channel PitchBend" : "Release Velocity";
					break;
				case PM_spread:
					spreadVoices();
					break;
			}
			onFocus = Focus_SEC;
		}
	}
	void dataPlusMinus(int val){//// DATA PLUS MINUS
		if ((val > 0) && ((dataMap[cursorIx] + val) > (static_cast<int>(dataMax[cursorIx])))) return;/////// R E T U R N ////////////
		if ((val < 0) && ((dataMap[cursorIx] + val) < (static_cast<int>(dataMin[cursorIx])))) return;/////// R E T U R N ////////////
		float newval;
		if(cursorIx != PM_xpander) newval = static_cast<float>(dataMap[cursorIx] + val);
		else{
			newval = static_cast<float>(dataMap[PM_xpander]);
			int updatedval = dataMap[PM_xpander] + val;
			int tryxp = (updatedval-1)/2;
			if (tryxp != xpanderId){
				if (val > 0){//val++
					for (int i = dataMap[PM_xpander]; i< dataMax[PM_xpander]; i++ ){
						updatedval = dataMap[PM_xpander] + val;
						tryxp = (updatedval-1)/2;
						if (sharedXpander::xpandch[tryxp] == 0) {
							newval = static_cast<float>(dataMap[PM_xpander] + val);
							break;
						}else val++;
					}
				}else{//val--
					for (int i = dataMap[PM_xpander]; i>=0; i-- ){
						updatedval = dataMap[PM_xpander] + val;
						tryxp = (updatedval-1)/2;
						if (sharedXpander::xpandch[tryxp] == 0) {
							newval = static_cast<float>(dataMap[PM_xpander] + val);
							break;
						}else val--;
					}
				}
			}else{
				newval = static_cast<float>(updatedval);
			}
		}
		paramQuantities[DATAKNOB_PARAM]->setSmoothValue(newval);
	}
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////   P  R  O  C  E  S  S   ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void process(const ProcessArgs &args) override {
		if (ProcessFrame++ < PROCESS_RATE) return;/////// R E T U R N ////////////
		ProcessFrame = 0;
		midi::Message msg;
		while (midiInput.tryPop(&msg, args.frame)) {
			processMessage(msg);
		}
		outputs[X_OUTPUT].setChannels(outCh);
		outputs[Y_OUTPUT].setChannels(outCh);
		outputs[Z_OUTPUT].setChannels(outCh);
		outputs[VEL_OUTPUT].setChannels(outCh);
		outputs[RVEL_OUTPUT].setChannels(outCh);
		outputs[GATE_OUTPUT].setChannels(outCh);
		float pbVo = mrPBendFilter.process(1.f ,mrPBend);
		float pbDirect = pbVo * pbUpDwn;
		outputs[PBEND_OUTPUT].setVoltage(pbVo);
		if (MPEmode) {/// MPE MODE!!!
			for (int i = 0; i < outCh; i++) {
				outgates[i] = ((gates[i] || hold[i]) && !reTrigger[i].process(args.sampleTime)) ? 10.f : 0.f;
				xpitch[i] = MPExFilter[i].process(1.f ,mpex[i]);
				outpitch[i] = xpitch[i]  * dataMap[PM_pbMPE] / 60.f + (static_cast<float>((notes[i] - 60)) / 12.f) + pbDirect;
				outputs[X_OUTPUT].setVoltage(outpitch[i], i);
				outputs[GATE_OUTPUT].setVoltage(outgates[i], i);
				outputs[VEL_OUTPUT].setVoltage(vels[i],i);
				outputs[Y_OUTPUT].setVoltage(MPEyFilter[i].process(1.f ,mpey[i]), i);
				outputs[Z_OUTPUT].setVoltage(MPEzFilter[i].process(1.f ,mpez[i]), i);
				outputs[RVEL_OUTPUT].setVoltage(mappedRvelPb[i], i);
				lights[CH_LIGHT + i].setBrightness(((i == rotateIndex)? 0.2f : 0.f) + (outgates[i] * .08f));
			}
		} else {/// POLY MODE!!!
			float unispread = static_cast<float>(UNImode);
			if (!xpandAlt){///normal Poly
				for (int i = 0; i < outCh; i++){ //regular Outs
					outgates[i] = ((gates[i] || hold[i]) && !reTrigger[i].process(args.sampleTime)) ? 10.f : 0.f;
					outpitch[i] = (static_cast<float>(notes[i] - 60 + dataMap[PM_transpose]) / 12.f) + pbDirect + unispread * spread[i];
					sendOutPoly(i, i);
				}
				for (int i = outCh; i < nVoCh; i++) {///Expanded   nVoCh >  outCh (twice voices)
					int xpi = i - outCh;
					outgates[i]  = ((gates[i] || hold[i]) && !reTrigger[i].process(args.sampleTime)) ? 10.f : 0.f;
					outpitch[i]  = (static_cast<float>(notes[i] - 60 + dataMap[PM_transpose]) / 12.f) + pbDirect + unispread * spread[i];
					sendOutXpander(xpi, i);
				}
			}else{///expanded  Alternate Outs
				for (int i = 0; i < outCh; i++){
					int i2 = i * 2;
					outgates[i2] = ((gates[i2] || hold[i2]) && !reTrigger[i2].process(args.sampleTime)) ? 10.f : 0.f;
					outpitch[i2] = (static_cast<float>(notes[i2] - 60 + dataMap[PM_transpose]) / 12.f) + pbDirect + unispread * spread[i2];
					sendOutPoly(i, i2);
					int ix = i2 + 1;
					outgates[ix]  = ((gates[ix] || hold[ix]) && !reTrigger[ix].process(args.sampleTime)) ? 10.f : 0.f;
					outpitch[ix]  = (static_cast<float>(notes[ix] - 60 + dataMap[PM_transpose]) / 12.f) + pbDirect + unispread * spread[ix];
					sendOutXpander(i, ix);
				}
			}
		}
		for (int i = 0; i < 8; i++){
			outputs[MM_OUTPUT + i].setVoltage(MCCsFilter[i].process(1.f,PM_midiCCsValues[dataMap[PM_midiCCs + i]]));
		}
		if (resetMidi) {// resetMidi from MIDI widget
			resetVoices();
			return;/////// R E T U R N ////////////
		}
		if (cursorIx < 1) return;/////// R E T U R N ////////////
		if (onFocus > 0){
			onFocus--;
			if (onFocus < 1){
				cursorIxLast = cursorIx;
				cursorIx = 0;
				learnId = 0;
				disableDataKnob();
				return;/////// R E T U R N ////////////
			}
			if (MinusOneTrigger.process(params[MINUSONE_PARAM].getValue())) {
				dataPlusMinus(-1);
				return;/////// R E T U R N ////////////
			}
			if (PlusOneTrigger.process(params[PLUSONE_PARAM].getValue())) {
				dataPlusMinus(1);
				return;/////// R E T U R N ////////////
			}
		}
	}
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////   PROCESS END  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void sendOutPoly(int ch, int i){
		outputs[X_OUTPUT].setVoltage(outpitch[i], ch);
		outputs[GATE_OUTPUT].setVoltage(outgates[i], ch);
		outputs[VEL_OUTPUT].setVoltage(vels[i], ch);
		outputs[Y_OUTPUT].setVoltage(outpitch[i] + drift[i] , ch);//drifted out
		outputs[Z_OUTPUT].setVoltage(naft[notes[i]], ch);
		outputs[RVEL_OUTPUT].setVoltage(rvels[i], ch);
		lights[CH_LIGHT+ ch].setBrightness(static_cast<float>(ch != rotateIndex) * -.15f + outgates[i] * .08f + .2f);
	}
	void sendOutXpander(int xpi, int i){
		sharedXpander::xpPitch[xpanderId][xpi] = outpitch[i];
		sharedXpander::xpGate[xpanderId][xpi] = outgates[i];
		sharedXpander::xpVel[xpanderId][xpi] = vels[i];
		sharedXpander::xpDetP[xpanderId][xpi] = outpitch[i] + drift[i];
		sharedXpander::xpNafT[xpanderId][xpi] = naft[notes[i]];
		sharedXpander::xpRvel[xpanderId][xpi] = rvels[i];
		sharedXpander::xpLed[xpanderId][xpi] = static_cast<float>(i != rotateIndex) * -.15f + outgates[i] * .08f + .2f;
	}
};//// Struct END  MIDIpolyMPE  ///////////////////////////////////////////////////////////////////////////////////////////
//// Main Display ///////////////////////////////////////////////////////////////////////////////////////
struct PolyModeDisplay : TransparentWidget {
	MIDIpolyMPE *module;
	float mdfontSize = 13.f;
	std::string sMode;
	std::string sVo;
	std::string sMPEpb;
	std::string sSteal;
	std::string sPBM;
	std::string sPM_noteMin;
	std::string sPM_noteMax;
	std::string sPM_velMin;
	std::string sPM_velMax;
	std::string yyDisplay;
	std::string zzDisplay;
	std::shared_ptr<Font> font;
	std::string polyModeStr[13] = {
		"R O T A T E",
		"R E U S E",
		"R E S E T",
		"R E A S S I G N",
		"S T A C K & S H A R E",
		"D U A L <lower >upper",
		"U N I S O N",
		"U N I S O N <lower",
		"U N I S O N >upper",
		"M. P. E.",
		"M. P. E. ROLI",
		"M. P. E. Haken Continuum",
		"C H A N N E L"
	};
	std::string xpandStr[9] = {"--","A+","Ax","B+","Bx","C+","Cx","D+","Dx"};
	std::string stealStr[5] {"Old","New","Low","Hi","No"};
	std::string noteName[12] = {"C","C#","D","Eb","E","F","F#","G","Ab","A","Bb","B"};
	int drawFrame = 0;
	int cursorIxI = 0;
	int timedFocus = 0;
	const unsigned char hiInk = 0xff;
	const unsigned char base = 0xdd;
	const unsigned char alphabck = 0x64;
	unsigned char redSel = 0x7f;
	NVGcolor baseG = nvgRGB(0xdd, 0xdd, 0xdd);
	NVGcolor redR = nvgRGB(0xff, 0x00, 0x00);
	NVGcolor itemColor[12];
	float outboundX[5] = {0.f, 23.5f,52.5f,93.f,113.f};//xPos note min,max .. vel min, max
	bool canlearn = true;
	void drawLayer(const DrawArgs &args, int layer) override {
		if (layer != 1) return;/////// R E T U R N ////////////
		font = APP->window->loadFont(mFONT_FILE);
		if (!(font && font->handle >= 0)) return;/////// R E T U R N ////////////
		if (cursorIxI != module->cursorIx){
			cursorIxI = module->cursorIx;
			timedFocus = 48;
		}
		sMode = polyModeStr[module->dataMap[MIDIpolyMPE::PM_polyModeId]];
		sPM_noteMin = noteName[module->dataMap[MIDIpolyMPE::PM_noteMin] % 12] + std::to_string((module->dataMap[MIDIpolyMPE::PM_noteMin] / 12) - 2);
		sPM_noteMax = noteName[module->dataMap[MIDIpolyMPE::PM_noteMax] % 12] + std::to_string((module->dataMap[MIDIpolyMPE::PM_noteMax] / 12) - 2);
		sPM_velMin = std::to_string(module->dataMap[MIDIpolyMPE::PM_velMin]);
		sPM_velMax = std::to_string(module->dataMap[MIDIpolyMPE::PM_velMax]);
		for (int i = 0; i < 12 ; i++){
			itemColor[i] = baseG;
		}
		if ((!module->MPEmode) && (module->xpanderId>-1) && (sharedXpander::xpandnum[module->xpanderId]>0)){
			itemColor[10] = nvgRGB(0x00, 0xff, 0x00);
			itemColor[11] = nvgRGB(0x00, 0xff, 0x00);
		}
		nvgBeginPath(args.vg);
		canlearn = true;
		switch (cursorIxI){
			case 0: goto noFocus;
			case MIDIpolyMPE::PM_polyModeId:{ // PolyMode
				nvgRoundedRect(args.vg, 1.f, 1.f, 134.f, 12.f, 3.f);
				canlearn = false;
				itemColor[0] = redR;
			}break;
			case MIDIpolyMPE::PM_pbMPE:{///PB MPE
				nvgRoundedRect(args.vg, 1.f, 14.f, 134.f, 12.f, 3.f);
				canlearn = false;
				itemColor[1] = redR;
			}break;
			case MIDIpolyMPE::PM_numVoCh:{///Voices
				nvgRoundedRect(args.vg, 1.f, 14.f, 52.f, 12.f, 3.f);
				canlearn = false;
				itemColor[1] = redR;
				itemColor[11] = redR;
			}break;
			case MIDIpolyMPE::PM_xpander:{///Xpander
				nvgRoundedRect(args.vg, 52.f, 14.f, 28.f, 12.f, 3.f);
				canlearn = false;
				itemColor[10] = redR;
			}break;
			case MIDIpolyMPE::PM_spread:{///
				nvgRoundedRect(args.vg, 80.f, 14.f, 54.f, 12.f, 3.f);
				canlearn = false;
				itemColor[2] = redR;
			}break;
			case MIDIpolyMPE::PM_stealMode:{ //stealMode
				nvgRoundedRect(args.vg, 80.f, 14.f, 54.f, 12.f, 3.f);
				canlearn = false;
				itemColor[3] = redR;
			}break;
			case MIDIpolyMPE::PM_noteMin:{ //minNote
				nvgRoundedRect(args.vg, 19.f, 28.f, 29.f, 12.f, 3.f);
				itemColor[4] = redR;
				itemColor[5] = redR;
			}break;
			case MIDIpolyMPE::PM_noteMax:{ //maxNote
				nvgRoundedRect(args.vg, 48.f, 28.f, 29.f, 12.f, 3.f);
				itemColor[4] = redR;
				itemColor[6] = redR;
			}break;
			case MIDIpolyMPE::PM_velMin:{ //minVel
				nvgRoundedRect(args.vg, 93.f, 28.f, 20.f, 12.f, 3.f);
				itemColor[7] = redR;
				itemColor[8] = redR;
			}break;
			case MIDIpolyMPE::PM_velMax:{ //maxVel
				nvgRoundedRect(args.vg, 113.f, 28.f, 20.f, 12.f, 3.f);
				itemColor[7] = redR;
				itemColor[9] = redR;
			}break;
			default:{
				goto noFocus;
			}
		}
		if (timedFocus > 0)	timedFocus  -=2;
		redSel = (canlearn)? (redSel + 24) % 256 : 0x7f;
		nvgFillColor(args.vg, nvgRGBA(redSel, 0, 0, alphabck + timedFocus)); //SELECTED
		nvgFill(args.vg);
	noFocus://///////////////////////////////////////////////////////////////////// L A B E L  noFocus
		nvgFontSize(args.vg, mdfontSize);
		nvgFontFaceId(args.vg, font->handle);
		nvgTextAlign(args.vg, NVG_ALIGN_CENTER);
		nvgFillColor(args.vg, itemColor[0]);///polymode
		nvgTextBox(args.vg, 1.f, 11.0f, 134.f, sMode.c_str(), NULL);
		nvgFillColor(args.vg, itemColor[1]);///voices
		if (module->MPEmode){
			nvgTextBox(args.vg, 1.f, 24.f, 134.f, ("voice ch PBend: " + std::to_string(module->dataMap[MIDIpolyMPE::PM_pbMPE])).c_str(), NULL);
		}else{
			std::string voi = "Voices:";
			nvgTextBox(args.vg, 1.f, 24.f, 38.f, voi.c_str(), NULL);
			nvgFillColor(args.vg, itemColor[11]);///voicenumber
			nvgTextBox(args.vg, 38.f, 24.f, 14.f, (std::to_string(module->dataMap[MIDIpolyMPE::PM_numVoCh] * module->xpand2x)).c_str(), NULL);
			nvgFillColor(args.vg, itemColor[10]);///Xpander
			nvgTextBox(args.vg, 52.f, 24.f, 28.f,("xp." + xpandStr[module->dataMap[MIDIpolyMPE::PM_xpander]]).c_str(), NULL);
			if (module->UNImode){
				nvgFillColor(args.vg, itemColor[2]);///spread
				nvgTextBox(args.vg, 80.f, 24.f, 54.f, ("Sprd: " + std::to_string(module->dataMap[MIDIpolyMPE::PM_spread])).c_str(), NULL);
			}else{
				nvgFillColor(args.vg, itemColor[3]);///steal
				nvgTextBox(args.vg, 80.f, 24.f, 54.f, ("Steal: " + stealStr[module->dataMap[MIDIpolyMPE::PM_stealMode]]).c_str(), NULL);
			}
		}
		nvgFillColor(args.vg, itemColor[4]);//note
		nvgTextBox(args.vg, 1.f, 37.f, 18.f, "nte:", NULL);
		nvgFillColor(args.vg, itemColor[5]);//...min
		nvgTextBox(args.vg, 19.f, 37.f, 29.f, sPM_noteMin.c_str(), NULL);
		nvgFillColor(args.vg, itemColor[6]);//...max
		nvgTextBox(args.vg, 48.f, 37.f, 29.f, sPM_noteMax.c_str(), NULL);
		nvgFillColor(args.vg, itemColor[7]);//Vel
		nvgTextBox(args.vg, 77.f, 37.f, 16.f, "vel:", NULL);
		nvgFillColor(args.vg, itemColor[8]);//min
		nvgTextBox(args.vg, 93.f, 37.f, 20.f, sPM_velMin.c_str(), NULL);
		nvgFillColor(args.vg, itemColor[9]);//max
		nvgTextBox(args.vg, 113.f, 37.f, 20.f, sPM_velMax.c_str(), NULL);
		if (module->outofbounds > 0){
			nvgBeginPath(args.vg);
			nvgRoundedRect(args.vg, outboundX[module->outofbounds], 28.f, 20.f, 12.f, 3.f);
			nvgFillColor(args.vg, nvgRGBA(0xff,0,0,128 - module->outboundTimer));
			nvgFill(args.vg);
			if (module->outboundTimer++ > 80){
				module->outofbounds = 0;
			}
		}
	}
	void onButton(const event::Button &e) override {
		if ((e.button == GLFW_MOUSE_BUTTON_LEFT) && (e.action == GLFW_PRESS)){
			int cmap = 0;
			int dolearn = 0;
			int i = static_cast<int>(e.pos.y / 13.f) + 1;
			switch (i) {
				case 3 :{ //Third line...
					if (e.pos.x < 45.3f) cmap = MIDIpolyMPE::PM_noteMin;
					else if (e.pos.x < 68.f) cmap = MIDIpolyMPE::PM_noteMax;
					else if (e.pos.x < 113.3f) cmap = MIDIpolyMPE::PM_velMin;
					else cmap = MIDIpolyMPE::PM_velMax;
					dolearn = cmap;
				}break;
				case 2: {/// second line
					if (module->MPEmode) cmap = MIDIpolyMPE::PM_pbMPE;
					else if (e.pos.x < 52.f) cmap = MIDIpolyMPE::PM_numVoCh;
					else if (e.pos.x < 80.f) cmap = MIDIpolyMPE::PM_xpander;
					else cmap = (module->UNImode)? MIDIpolyMPE::PM_spread : MIDIpolyMPE::PM_stealMode;
				}break;
				case 1: {///first  line
					cmap = MIDIpolyMPE::PM_polyModeId;
				}break;
			}
			if (module->cursorIx != cmap){
				module->cursorIx = cmap;
				module->learnId = dolearn;
				module->configDataKnob();
			}else module->onFocus = 1;
		}
	}
};
struct LCDbutton : OpaqueWidget {
	MIDIpolyMPE *module = nullptr;
	int buttonId = 0;
	int *buttonIdMap = &buttonId;
	bool boolptr = true;
	bool *canedit = &boolptr;
	bool canlearn = false;
	unsigned char tcol = 0xdd; //base color x x x
	unsigned char redSel = 0xcc; // sel red.
	int flashFocus = 0;
	float mdfontSize = 13.f;
	std::shared_ptr<Font> font;
	std::string sDisplay = "";
	void drawLayer(const DrawArgs &args, int layer) override {
		if (layer != 1) return;/////// R E T U R N ////////////
		font = APP->window->loadFont(mFONT_FILE);
		if (!(font && font->handle >= 0)) return;/////// R E T U R N ////////////
		nvgFontFaceId(args.vg, font->handle);
		nvgFontSize(args.vg, mdfontSize);
		nvgTextAlign(args.vg, NVG_ALIGN_CENTER);
		if(module->cursorIx != *buttonIdMap){
			unsigned char alphaedit =  static_cast<unsigned char> (*canedit) * 0x44 + 0xbb;
			nvgFillColor(args.vg, nvgRGBA(tcol, tcol, tcol, alphaedit));//text
		}else{//if selected draw backg and change colors
			redSel = (canlearn)? (redSel + 24) % 256 : 0x7f;
			nvgBeginPath(args.vg);
			nvgRoundedRect(args.vg, 0.f, 0.f, box.size.x, box.size.y,3.f);
			if (flashFocus > 0) flashFocus -=2;
			nvgFillColor(args.vg, nvgRGBA(redSel, 0, 0, 0x64 + flashFocus));//backgr
			nvgFill(args.vg);
			nvgFillColor(args.vg, nvgRGB(0xff , 0, 0));//text Sel
		}
		nvgTextBox(args.vg, 0.f, 10.5f,box.size.x, sDisplay.c_str(), NULL);
	}
	void onButton(const event::Button &e) override {
		if ((e.button != GLFW_MOUSE_BUTTON_LEFT) || !(*canedit) || (e.action != GLFW_PRESS)) return;/////// R E T U R N ////////////
		if (module->cursorIx != *buttonIdMap){
			module->cursorIx = *buttonIdMap;
			module->configDataKnob();
			flashFocus = 80;
			if(canlearn) module->learnId = *buttonIdMap;//learnId;
		}else{
			module->onFocus = 1;// next goes to 0
		}
	}
};
struct MIDIccLCDbutton : LCDbutton{
	MIDIccLCDbutton() {
		for (int i = 0 ; i < 128; i++){
			sNames[i].assign("cc" + std::to_string(i));
		}
		sNames[1].assign("Mod");
		sNames[2].assign("BrC");
		sNames[7].assign("Vol");
		sNames[10].assign("Pan");
		sNames[11].assign("Expr");
		sNames[64].assign("Sust");
		sNames[128].assign("chAT");
		sNames[129].assign("nteAT");///130 detune mapped with value + -
		sNames[131].assign("Slide");
		sNames[132].assign("Press");
		sNames[133].assign("cc74+");
		sNames[134].assign("chAT+");
	}
	std::string sNames[135];
	void drawLayer(const DrawArgs &args, int layer) override{
		if (layer != 1) return;/////// R E T U R N ////////////
		sDisplay = sNames[module->dataMap[buttonId]];
		LCDbutton::drawLayer( args, layer);
	}
};
struct MPEYdetuneLCDbutton : MIDIccLCDbutton{
	int buttonId2 = 0;
	void drawLayer(const DrawArgs &args, int layer) override{
		if (layer != 1) return;/////// R E T U R N ////////////
		if (*module->Y_ptr != 130){//ccs
			sDisplay = sNames[*module->Y_ptr];
		}else{
			int det = module->dataMap[MIDIpolyMPE::PM_detune];
			sDisplay = "rn±" + std::to_string(det) + "¢";
		}
		LCDbutton::drawLayer(args, layer);
	}
	void onButton(const event::Button &e) override {
		if ((e.button != GLFW_MOUSE_BUTTON_LEFT) || !(*canedit) || (e.action != GLFW_PRESS)) return;/////// R E T U R N ////////////
		buttonIdMap =  (module->MPEmode)? &buttonId : &buttonId2;
		LCDbutton::onButton(e);
	}
};
struct MPEzLCDbutton : MIDIccLCDbutton{
	void drawLayer(const DrawArgs &args, int layer) override {
		if (layer != 1) return;/////// R E T U R N ////////////
		sDisplay = sNames[*module->Z_ptr];
		LCDbutton::drawLayer( args, layer);
	}
};
struct RelVelLCDbutton : LCDbutton{
	std::string pNames[2] = {"RelVel","chPB"};
	std::string roliNames[2] = {"Lift","chGlide"};
	void drawLayer(const DrawArgs &args, int layer) override {
		if (layer != 1) return;/////// R E T U R N ////////////
		sDisplay = (module->dataMap[MIDIpolyMPE::PM_polyModeId] != MIDIpolyMPE::MPEROLI_MODE)? pNames[*module->RP_ptr] : roliNames[*module->RP_ptr];
		LCDbutton::drawLayer( args, layer);
	}
};
struct PlusMinusLCDbutton : LCDbutton{
	void drawLayer(const DrawArgs &args, int layer) override{
		if (layer != 1) return;/////// R E T U R N ////////////
		std::stringstream ss;
		if(module->dataMap[buttonId] > 0) ss << "+";
		ss << std::to_string(module->dataMap[buttonId]);
		sDisplay = ss.str();
		LCDbutton::drawLayer( args, layer);
	}
};
struct dataKnob : RoundKnob {
	dataKnob(){
		minAngle = -0.75f*M_PI;
		maxAngle = 0.75f*M_PI;
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/knobs/dataKnob_fg.svg")));
		bg->setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/knobs/dataKnob_bg.svg")));
		shadow->opacity = 0.f;
	}
	MIDIpolyMPE *module;
	void onEnter(const EnterEvent& e) override {}//// no tooltip
	void onLeave(const LeaveEvent& e) override {}//// no tooltip destroy
	void onButton(const ButtonEvent& e) override {
		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_RIGHT && module->cursorIx < 1) return;/////// R E T U R N ////////////
		RoundKnob::onButton(e);
	}
	void onChange(const ChangeEvent& e) override{
		if (!module) return;/////// R E T U R N ////////////
		RoundKnob::onChange(e);
		if ((module->cursorIx < 1) || (module->onFocus > module->Focus_SEC)) return;/////// R E T U R N ////////////
		engine::ParamQuantity* pq = getParamQuantity();
		if (pq)module->setNewValue(pq->getSmoothValue());
	}
	void onAction(const ActionEvent& e) override {//// "press" the knob to close edit ...or recall last edit
		if (!module) return;/////// R E T U R N ////////////
		if (module->cursorIx < 1) {
			module->cursorIx = module->cursorIxLast;
			module->configDataKnob();
		}else{
			module->onFocus = 1;
		}
	}
};
struct DataEntryOnLed : TransparentWidget {
	DataEntryOnLed(){
		box.size.x = 107.f;
		box.size.y = 42.f;
	}
	int dummyInit = 0;
	int *cursorIx = &dummyInit;
	void drawLayer(const DrawArgs &args, int layer) override{
		if ((layer != 1) || (*cursorIx < 1)) return;/////// R E T U R N ////////////
		for (int i = 0; i < 8; i++){
			nvgBeginPath(args.vg);
			nvgStrokeWidth(args.vg, 1.f);
			float p = static_cast<float>(i);
			float h = p * .5f;
			float a = 1.f/ (1.f + p * 2.f);
			nvgStrokeColor(args.vg, nvgRGBA(0xff, 0, 0, 0xcc * a));
			nvgRoundedRect(args.vg, 2.f - h, 9.f - h, 24.f + p, 24.f + p,5.9f + p * .25);
			nvgRoundedRect(args.vg, 81.f - h, 9.f - h, 24.f + p, 24.f + p,5.9f + p * .25);
			nvgCircle(args.vg, 53.5f, 21.f, 18.5f + p);
			nvgStroke(args.vg);
		}
	}
};
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////// MODULE WIDGET ////////
/////////////////////////////////////////////////
struct MIDIpolyMPEWidget : ModuleWidget {
	MIDIpolyMPEWidget(MIDIpolyMPE *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance,"res/modules/MIDIpolyMPE.svg")));
		addChild(createWidget<ScrewBlack>(Vec(0.f, 0.f)));//Screws
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 15.f, 0.f)));
		addChild(createWidget<ScrewBlack>(Vec(0.f, 365.f)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 15.f, 365.f)));
		float xPos = 7.f;
		float yPos = 19.f;
		if (module) {//MIDI
			MIDIscreen *dDisplay = createWidget<MIDIscreen>(Vec(xPos,yPos));
			dDisplay->box.size = {136.f, 40.f};
			dDisplay->setMidiPort (&module->midiInput, &module->MPEmode, &module->MPEmasterCh, &module->midiActivity, &module->mdriverJx, &module->mdeviceJx, &module->mchannelJx, &module->resetMidi);
			addChild(dDisplay);
			xPos = 7.f;//PolyModes LCD
			yPos = 61.f;
			PolyModeDisplay *polyModeDisplay = createWidget<PolyModeDisplay>(Vec(xPos,yPos));
			polyModeDisplay->box.size = {136.f, 40.f};
			polyModeDisplay->module = module;
			addChild(polyModeDisplay);
			xPos = 17.f;
			yPos = 202.f;
			{//MPE Ycc
				MPEYdetuneLCDbutton *MccLCD = createWidget<MPEYdetuneLCDbutton>(Vec(xPos,yPos));
				MccLCD->box.size = {34.f, 13.f};
				MccLCD->module = module;
				MccLCD->buttonId = MIDIpolyMPE::PM_mpeYcc;
				MccLCD->buttonId2 = MIDIpolyMPE::PM_detune;
				MccLCD->canlearn = false;
				MccLCD->canedit = &module->Ycanedit;
				addChild(MccLCD);
			}
			xPos = 60.f;
			{///MPE Zcc
				MPEzLCDbutton *MccLCD = createWidget<MPEzLCDbutton>(Vec(xPos,yPos));
				MccLCD->box.size = {34.f, 13.f};
				MccLCD->module = module;
				MccLCD->buttonId = MIDIpolyMPE::PM_mpeZcc;
				MccLCD->canlearn = false;
				MccLCD->canedit = &module->Zcanedit;
				addChild(MccLCD);
			}
			xPos = 103.f;
			{// RelPbendMPE LCD
				RelVelLCDbutton *MccLCD = createWidget<RelVelLCDbutton>(Vec(xPos,yPos));
				MccLCD->box.size = {34.f, 13.f};
				MccLCD->module = module;
				MccLCD->buttonId = MIDIpolyMPE::PM_mpeRvPb;
				MccLCD->canlearn = false;
				MccLCD->canedit = &module->RPcanedit;
				addChild(MccLCD);
			}
			xPos = 11.5f;
			yPos = 253.5f;
			{///PM_transpose
				PlusMinusLCDbutton *MccLCD = createWidget<PlusMinusLCDbutton>(Vec(xPos,yPos));
				MccLCD->box.size = {30.f, 13.f};
				MccLCD->module = module;
				MccLCD->buttonId = MIDIpolyMPE::PM_transpose;
				MccLCD->canlearn = false;
				addChild(MccLCD);
			}
			xPos = 47.5f;
			{///PBend Down
				PlusMinusLCDbutton *MccLCD = createWidget<PlusMinusLCDbutton>(Vec(xPos,yPos));
				MccLCD->box.size = {23.f, 13.f};
				MccLCD->module = module;
				MccLCD->buttonId = MIDIpolyMPE::PM_pbDwn;
				MccLCD->canlearn = false;
				addChild(MccLCD);
			}
			xPos += 23.f;
			{///PBend Up
				PlusMinusLCDbutton *MccLCD = createWidget<PlusMinusLCDbutton>(Vec(xPos,yPos));
				MccLCD->box.size = {23.f, 13.f};
				MccLCD->module = module;
				MccLCD->buttonId = MIDIpolyMPE::PM_pbUp;
				MccLCD->canlearn = false;
				addChild(MccLCD);
			}
			for ( int i = 0; i < 8; i++){// CC's x 8
				xPos = 10.5f + (float)(i % 4) * 33.f;
				yPos = 283.f + (float)((int)((float)i * 0.25f) % 4) * 40.f;
				MIDIccLCDbutton *MccLCD = createWidget<MIDIccLCDbutton>(Vec(xPos,yPos));
				MccLCD->box.size = {30.f, 13.f};
				MccLCD->module = module;
				MccLCD->buttonId = MIDIpolyMPE::PM_midiCCs + i;
				MccLCD->canlearn = true;
				addChild(MccLCD);
			}//dataEntryLed
			DataEntryOnLed *dataEntryOnLed = createWidget<DataEntryOnLed>(Vec(21.5f,107.5f));
			dataEntryOnLed->cursorIx = &module->cursorIx;
			addChild(dataEntryOnLed);
		}///end if module
		////DATA KNOB + -
		yPos = 110.5f;
		xPos = 57.f;
		dataKnob * knb = new dataKnob;
		knb->box.pos = Vec(xPos, yPos);
		knb->module = module;
		knb->app::ParamWidget::module = module;
		knb->app::ParamWidget::paramId = MIDIpolyMPE::DATAKNOB_PARAM;
		knb->initParamQuantity();
		addParam(knb);
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
			addChild(createLight<VoiceChGreenLed>(Vec(xPos + i * 8.f, yPos), module, MIDIpolyMPE::CH_LIGHT + i));
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
//		if (module){
//			ValueTestLCD *MccLCD = createWidget<ValueTestLCD>(Vec(0.f,0.f));
//			MccLCD->box.size = {15.f, 15.f};
//			MccLCD->intVal = &module->xpanderId;
//			addChild(MccLCD);
//		}
	}
};
Model *modelMIDIpolyMPE = createModel<MIDIpolyMPE, MIDIpolyMPEWidget>("MIDIpolyMPE");
