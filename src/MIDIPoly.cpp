#include "moDllz.hpp"
#include <list>

/*
 * MIDIpoly16 converts midi note on/off events, velocity , pitch wheel and mod wheel to CV
 * with 16 Midi Note Buttons with individual vel & gate
 */

struct MidiNoteData {
	uint8_t velocity = 0;
	uint8_t aftertouch = 0;
};

struct MIDIpoly16 : Module {
 static const int numPads = 16;
	
	enum ParamIds {
		ENUMS(KEYBUTTON_PARAM, 16),
		ENUMS(SEQSEND_PARAM, 16),
		LEARNPAD_PARAM,
		LOCKPAD_PARAM,
		SEQPAD_PARAM,
		PADXLOCK_PARAM,
		DRIFT_PARAM,
		POLYMODE_PARAM,
		MONOPITCH_PARAM,
		ARCADEON_PARAM,
		ARPEGON_PARAM,
		ARPEGRATIO_PARAM,
		ARPEGOCT_PARAM,
		ARPEGOCTALT_PARAM,
		MONORETRIG_PARAM,
		HOLD_PARAM,
		SEQSPEED_PARAM,
		SEQCLOCKRATIO_PARAM,
		SEQFIRST_PARAM,
		SEQSTEPS_PARAM,
		SEQRUN_PARAM,
		SEQGATERUN_PARAM,
		SEQRETRIG_PARAM,
		SEQRESET_PARAM,
		SEQRUNRESET_PARAM,
		SEQRUNGATE_PARAM,
		SEQCLOCKSRC_PARAM,
		SEQARPSWING_PARAM,
		SWINGTRI_PARAM,
		SEQOCT_PARAM,
		SEQOCTALT_PARAM,
		SEQSWING_PARAM,
		ARPSWING_PARAM,
		SEQTRANUP_PARAM,
		SEQTRANDWN_PARAM,
		POLYTRANUP_PARAM,
		POLYTRANDWN_PARAM,
		POLYUNISON_PARAM,
		LOCKEDPITCH_PARAM,
		LOCKEDRETRIG_PARAM,
		PLAYXLOCKED_PARAM,
		DISPLAYNOTENUM_PARAM,
		RESETMIDI_PARAM,
		TRIMPOLYSHIFT_PARAM,
		TRIMSEQSHIFT_PARAM,
		MUTESEQ_PARAM,
		MUTEMONO_PARAM,
		MUTELOCKED_PARAM,
		MUTEPOLYA_PARAM,
		MUTEPOLYB_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		POLYUNISON_INPUT,
		POLYSHIFT_INPUT,
		CLOCK_INPUT,
		SEQSPEED_INPUT,
		SEQRATIO_INPUT,
		SEQSTEPS_INPUT,
		SEQFIRST_INPUT,
		SEQRUN_INPUT,
		SEQRESET_INPUT,
		SEQSWING_INPUT,
		SEQSHIFT_INPUT,
		ARPEGRATIO_INPUT,
		ARPMODE_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(PITCH_OUTPUT,16),
		ENUMS(VEL_OUTPUT,16),
		ENUMS(GATE_OUTPUT,16),
		SEQPITCH_OUTPUT,
		SEQVEL_OUTPUT,
		SEQGATE_OUTPUT,
		LOCKEDPITCH_OUTPUT,
		LOCKEDVEL_OUTPUT,
		LOCKEDGATE_OUTPUT,
		MONOPITCH_OUTPUT,
		MONOVEL_OUTPUT,
		MONOGATE_OUTPUT,
		PBEND_OUTPUT,
		MOD_OUTPUT,
		PRESSURE_OUTPUT,
		SUSTAIN_OUTPUT,
		SEQSTART_OUTPUT,
		SEQSTOP_OUTPUT,
		SEQSTARTSTOP_OUTPUT,
		SEQCLOCK_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(SEQ_LIGHT,16),
		RESETMIDI_LIGHT,
		PLOCK_LIGHT,
		PLEARN_LIGHT,
		PXLOCK_LIGHT,
		PSEQ_LIGHT,
		SEQRUNNING_LIGHT,
		SEQRESET_LIGHT,
		ENUMS(SEQOCT_LIGHT,5),
		ENUMS(ARPOCT_LIGHT,5),
		ARCADEON_LIGHT,
		ARPEGON_LIGHT,
		MUTESEQ_LIGHT,
		MUTEMONO_LIGHT,
		MUTELOCKED_LIGHT,
		MUTEPOLY_LIGHT,
		MIDIBEAT_LIGHT,
		NUM_LIGHTS
	};
	
	
	midi::InputQueue midiInput;
	
	uint8_t mod = 0;
	dsp::ExponentialFilter modFilter;
	uint16_t pitch = 8192;
	dsp::ExponentialFilter pitchFilter;
	uint8_t sustain = 0;
	dsp::ExponentialFilter sustainFilter;
	uint8_t pressure = 0;
	dsp::ExponentialFilter pressureFilter;
	
	
	MidiNoteData noteData[128];
	
	enum buttonmodes{
		POLY_MODE,
		SEQ_MODE,
		LOCKED_MODE,
		XLOCK_MODE
	};
	
	struct noteButton{
		int key = 0;
		int vel = 0;
		float drift = 0.f;
		bool gate = false;
		bool button = false;
		bool newkey = true;
		int mode = 0;
		bool learn = false;
		int velseq = 127; //lastvel for seq
		int stamp = 0;
		bool gateseq = false;
		bool polyretrig = false;
	};
	
	noteButton noteButtons[numPads];
	
	std::list<int> noteBuffer; //buffered notes over polyphony
	

	int polyIndex = 0;
	int polyTopIndex = numPads-1;
	int polymode = 0;
	int lastpolyIndex = 0;
	int polyMaxVoices = 8;
	int stampIx = 0;
	int playingVoices = 0;
	int polyTransParam = 0;
	
	float arpPhase = 0.f;
	int arpegStatus = 0; //for Display : Mode / OutMono active
	int arpegMode = 0;
	bool arpegStep = false;
	bool arpSwingDwn = true;
	int arpclockRatio = 0;
	int arpegIx = 0;
	int arpegFrame = 0;
	int arpOctIx = 0;
	bool arpegStarted = false;
	int arpegCycle = 0;
	int arpOctValue = 0;
	bool syncArpPhase = false;
	int arpDisplayIx = -1;
	
	const int octaveShift[7][5] =
	{   {0,-1,-2,-1, 8},
		{0,-1,-2, 8, 8},
		{0,-1, 8, 8, 8},
		{0, 8, 8, 8, 8},
		{0, 1, 8, 8, 8},
		{0, 1, 2, 8, 8},
		{0, 1, 2, 1, 8}
	};
	
	int liveMono = 0;
	int lockedMono = 0;
	int lastMono = -1;
	int lastLocked = -1;
 
	bool pedal = false;
	bool sustainhold = true;
	
	float drift[numPads] = {0.f};
	
	bool padSetLearn = false;
	int padSetMode = 0;
	
	float seqPhase = 0.f;
	bool seqSwingDwn = true;
	bool seqrunning = false;
	bool seqResetNext = false;
	int seqSteps = 16;
	int seqOffset = 0;
	int seqStep = 0;
	int seqi = 0;
	int seqiWoct = 0;
	int seqOctIx = 0;
	int seqOctValue = 3;
	int seqTransParam = 0;
	int seqSampleCount = 0;
	int arpSampleCount = 0;
	int ClockSeqSamples = 1;
	int ClockArpSamples = 1;

	const float ClockRatios[13] ={0.50f, 2.f/3.f,0.75f, 1.f ,4.f/3.f,1.5f, 2.f, 8.f/3.f, 3.f, 4.f, 6.f, 8.f,12.f};
	const bool swingTriplet[13] = {true,true,false,true,true,false,true,true,false,true,false,true,false};
	
	int clockSource = 0;
	int seqclockRatio = 1;
	std::string mainDisplay[4] = {""};
	bool dispNotenumber = false;
	
	dsp::SchmittTrigger resetMidiTrigger;
	dsp::SchmittTrigger extClockTrigger;
	dsp::SchmittTrigger seqRunTrigger;
	dsp::SchmittTrigger seqResetTrigger;
	dsp::SchmittTrigger setLockTrigger;
	dsp::SchmittTrigger setLearnTrigger;
	dsp::SchmittTrigger setSeqTrigger;
	dsp::SchmittTrigger setPadTrigger;
	dsp::SchmittTrigger setArcadeTrigger;
	dsp::SchmittTrigger setArpegTrigger;
	dsp::SchmittTrigger octUpTrigger;
	dsp::SchmittTrigger octDwnTrigger;
	dsp::SchmittTrigger seqTransUpTrigger;
	dsp::SchmittTrigger seqTransDwnTrigger;
	dsp::SchmittTrigger polyTransUpTrigger;
	dsp::SchmittTrigger polyTransDwnTrigger;
	
	dsp::SchmittTrigger muteSeqTrigger;
	dsp::SchmittTrigger muteMonoTrigger;
	dsp::SchmittTrigger muteLockedTrigger;
	dsp::SchmittTrigger mutePolyATrigger;
	dsp::SchmittTrigger mutePolyBTrigger;
	
	bool muteSeq = false;
	bool muteMono = false;
	bool muteLocked = false;
	bool mutePoly = false;

	dsp::PulseGenerator gatePulse;
	dsp::PulseGenerator keyPulse;
	dsp::PulseGenerator monoPulse;
	dsp::PulseGenerator lockedPulse;
  
	dsp::PulseGenerator clockPulse;
	dsp::PulseGenerator startPulse;
	dsp::PulseGenerator stopPulse;
	
	bool clkMIDItick = false;
	bool MIDIstart = false;
	bool MIDIstop = false;
	bool MIDIcont = false;
	bool stopped = true;
	bool arpMIDItick = false;
	bool seqMIDItick = false;
	int seqTickCount = 0;
	int arpTickCount = 0;
	int sampleFrames = 0;
	int displayedBPM = 0;
	float BPMrate = 0.f;
	bool BPMdecimals = false;
	bool firstBPM = true; ///to hold if no clock ...and skip first BPM calc...
	bool extBPM = false;
	

	///////////////
	MIDIpoly16() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		for (int i = 0; i < 16; i ++){
			configParam(KEYBUTTON_PARAM + i, 0.f, 1.f, 0.f);
			configParam(SEQSEND_PARAM + i, 0.f, 2.f, 0.f);
		}		
		configParam(RESETMIDI_PARAM, 0.f, 1.f, 0.f);
		configParam(POLYMODE_PARAM, 0.f, 2.f, 1.f);
		configParam(TRIMPOLYSHIFT_PARAM, 0.f, 48.f, 9.6f);
		configParam(POLYTRANUP_PARAM, 0.f, 1.f, 0.f);
		configParam(POLYTRANDWN_PARAM, 0.f, 1.f, 0.f);
		configParam(POLYUNISON_PARAM, 0.f, 2.f, 0.f);
		configParam(DRIFT_PARAM, 0.f, 0.1f, 0.f);
		configParam(DISPLAYNOTENUM_PARAM, 0.f, 1.f, 0.f);
		configParam(SEQPAD_PARAM, 0.f, 1.f, 0.f);
		configParam(PADXLOCK_PARAM, 0.f, 1.f, 0.f);
		configParam(LEARNPAD_PARAM, 0.f, 1.f, 0.f);
		configParam(LOCKPAD_PARAM, 0.f, 1.f, 0.f);
		configParam(SEQSPEED_PARAM,  20.f, 240.f, 120.f);
		configParam(SEQCLOCKRATIO_PARAM,  0.f, 12.f, 3.f);
		configParam(SEQSTEPS_PARAM, 1.f, 16.f, 16.f);
		configParam(SEQFIRST_PARAM, 0.f, 15.f, 0.f);
		configParam(SEQARPSWING_PARAM, -20.f, 20.f, 0.f);
		configParam(SEQSWING_PARAM, 0.f, 1.f, 0.f);
		configParam(ARPSWING_PARAM, 0.f, 1.f, 0.f);
		configParam(SWINGTRI_PARAM, 0.f, 1.f, 0.f);
		configParam(SEQOCT_PARAM, 0.f, 6.f, 3.f);
		configParam(ARPEGOCT_PARAM, 0.f, 6.f, 3.f);
		configParam(SEQOCTALT_PARAM, 0.f, 1.f, 0.f);
		configParam(ARPEGOCTALT_PARAM, 0.f, 1.f, 1.f);
		configParam(ARPEGRATIO_PARAM,  0.f, 12.f, 6.f);
		configParam(ARCADEON_PARAM, 0.f, 1.f, 0.f);
		configParam(ARPEGON_PARAM, 0.f, 1.f, 0.f);
		configParam(SEQCLOCKSRC_PARAM,  0.f, 2.f, 1.f);
		configParam(SEQGATERUN_PARAM, 0.f, 1.f, 0.f);
		configParam(SEQRUNRESET_PARAM, 0.f, 1.f, 0.f);
		configParam(SEQRUN_PARAM, 0.f, 1.f, 0.f);
		configParam(SEQRESET_PARAM, 0.f, 1.f, 0.f);
		configParam(TRIMSEQSHIFT_PARAM, 0.f, 48.f, 9.6f);
		configParam(SEQTRANUP_PARAM, 0.f, 1.f, 0.f);
		configParam(SEQTRANDWN_PARAM, 0.f, 1.f, 0.f);
		configParam(SEQRETRIG_PARAM, 0.f, 1.f, 1.f);
		configParam(MONOPITCH_PARAM, 0.f, 2.f, 1.f);
		configParam(MONORETRIG_PARAM, 0.f, 1.f, 0.f);
		configParam(HOLD_PARAM, 0.f, 1.f, 1.f);
		configParam(PLAYXLOCKED_PARAM, 0.f, 1.f, 1.f);
		configParam(LOCKEDPITCH_PARAM, 0.f, 2.f, 0.f);
		configParam(LOCKEDRETRIG_PARAM, 0.f, 1.f, 0.f);
		configParam(MUTESEQ_PARAM, 0.f, 1.f, 0.f);
		configParam(MUTEMONO_PARAM, 0.f, 1.f, 0.f);
		configParam(MUTELOCKED_PARAM, 0.f, 1.f, 0.f);
		configParam(MUTEPOLYA_PARAM, 0.f, 1.f, 0.f);
		configParam(MUTEPOLYB_PARAM, 0.f, 1.f, 0.f);
	 onReset();
	}

	~MIDIpoly16() {
	};
	
	void doSequencer();
	
	void process(const ProcessArgs &args) override;

	void processMessage(midi::Message msg);
	void processCC(midi::Message msg);
	void processSystem(midi::Message msg);
	
	void pressNote(int note, int vel);

	void releaseNote(int note);
	
	void releasePedalNotes();
	
	void setPolyIndex(int note);
	
	void initPolyIndex();
	
	void recoverBufferedNotes();
	
	void getBPM();
	
	float minmaxFit(float val, float minv, float maxv);
 
	void MidiPanic();

	void onSampleRateChange() override{
		onReset();
	}
	
	void onReset() override {
		// resetMidi();
		for (int i = 0; i < numPads; i++)
		{
			noteButtons[i].key= 48 + i;
			if (i < 8 ) noteButtons[i].mode = POLY_MODE;
			else if(i < 12) noteButtons[i].mode = SEQ_MODE;
			else noteButtons[i].mode = LOCKED_MODE;
		   //// else noteButtons[i].mode = XLOCK_MODE;
		}
		polyIndex = 0;
		polyTopIndex = 7;;
		lastpolyIndex = 0;
		seqTransParam = 0;
		for (int i = 0; i < NUM_OUTPUTS; i++){
			outputs[i].value= 0.f;
		}
		params[SEQRESET_PARAM].setValue(0.f);
		sustainFilter.lambda = 100.f * APP->engine->getSampleTime();
		modFilter.lambda = 100.f * APP->engine->getSampleTime();
		pressureFilter.lambda = 100.f * APP->engine->getSampleTime();
		pitchFilter.lambda = 100.f * APP->engine->getSampleTime();
	}
	
	json_t *dataToJson() override {
		json_t *rootJ = json_object();
		
		 json_object_set_new(rootJ, "midi", midiInput.toJson());
		for (int i = 0; i < numPads; i++) {
			json_object_set_new(rootJ, ("key" + std::to_string(i)).c_str(), json_integer(noteButtons[i].key));
			json_object_set_new(rootJ, ("mode" + std::to_string(i)).c_str(), json_integer(noteButtons[i].mode));
		}
		json_object_set_new(rootJ, "seqtransp", json_integer(seqTransParam));
		json_object_set_new(rootJ, "polytransp", json_integer(polyTransParam));
		json_object_set_new(rootJ, "arpegmode", json_integer(arpegMode));
		json_object_set_new(rootJ, "seqrunning", json_boolean(seqrunning));
		return rootJ;
	}
	
	void dataFromJson(json_t *rootJ) override {
		
		json_t *midiJ = json_object_get(rootJ, "midi");
		if (midiJ)
			midiInput.fromJson(midiJ);
		for (int i = 0; i < numPads; i++) {
			json_t *keyJ = json_object_get(rootJ,("key" + std::to_string(i)).c_str());
			 if (keyJ)
				noteButtons[i].key = json_integer_value(keyJ);
			json_t *modeJ = json_object_get(rootJ,("mode" + std::to_string(i)).c_str());
			 if (modeJ)
				noteButtons[i].mode = json_integer_value(modeJ);
			noteButtons[i].learn = false;
		}
		json_t *seqtranspJ = json_object_get(rootJ,("seqtransp"));
		if (seqtranspJ)
			 seqTransParam = json_integer_value(seqtranspJ);
		json_t *polytranspJ = json_object_get(rootJ,("polytransp"));
		if (polytranspJ)
			polyTransParam = json_integer_value(polytranspJ);
		json_t *arpegmodeJ = json_object_get(rootJ,("arpegmode"));
		if (arpegmodeJ)
			arpegMode = json_integer_value(arpegmodeJ);
		json_t *seqrunningJ = json_object_get(rootJ,("seqrunning"));
		if (seqrunningJ)
			seqrunning = json_is_true(seqrunningJ);
		
		padSetMode = POLY_MODE;
		padSetLearn = false;
		///midiInput.rtMidiIn->ignoreTypes(true,false,false);
		MidiPanic();
		initPolyIndex();
	}
};


void MIDIpoly16::initPolyIndex(){
	polyTopIndex = -1;
	int iPoly = 0;
	polyMaxVoices = 0;
	for (int i = 0 ; i < numPads; i++){
		if (noteButtons[i].mode == POLY_MODE){
			iPoly ++;
				polyMaxVoices = iPoly;
				polyTopIndex = i;
		}
	}
	polyIndex = polyTopIndex;
	noteBuffer.clear();
}

void MIDIpoly16::pressNote(int note, int vel) {
	stampIx ++ ; // update note press stamp for mono "last"
	if (playingVoices == polyMaxVoices) keyPulse.trigger(1e-3);
	if (params[MONORETRIG_PARAM].getValue() > 0.5f) monoPulse.trigger(1e-3);
	if (params[LOCKEDRETRIG_PARAM].getValue() > 0.5f) lockedPulse.trigger(1e-3);

	bool (Xlockedmatch) = false;
	for (int i = 0; i < numPads; i++){
		noteButtons[i].polyretrig = false;
		if (noteButtons[i].learn) {
			noteButtons[i].key= note;
			noteButtons[i].learn = false;
		}
 /////////play every matching locked note...
			if ((note == noteButtons[i].key)  && (noteButtons[i].mode > 1)){
				noteButtons[i].gate = true;
				noteButtons[i].newkey = true; //same key but true to display velocity
				noteButtons[i].vel = vel;
				noteButtons[i].velseq = vel;
				noteButtons[i].stamp = stampIx;
				//
				if (noteButtons[i].mode == XLOCK_MODE) Xlockedmatch = true;
			}
	}

	//if (params[PLAYXLOCKED_PARAM].getValue() < 0.5f)
		if (Xlockedmatch) return;
	
	///////////////////////
	if (polyIndex > -1){ //polyIndex -1 if all notes are locked or seq
		setPolyIndex(note);

		////////////////////////////////////////
		/// if gate is on set retrigg
		noteButtons[polyIndex].polyretrig = noteButtons[polyIndex].gate;
		noteButtons[polyIndex].stamp = stampIx;
		noteButtons[polyIndex].key = note;
		noteButtons[polyIndex].vel = vel;
		noteButtons[polyIndex].velseq = vel;
		noteButtons[polyIndex].newkey = true;
		noteButtons[polyIndex].gate = true;
	}
	return;
}

void MIDIpoly16::releaseNote(int note) {
   // auto it = std::find(noteBuffer.begin(), noteBuffer.end(), note);
	//if (it != noteBuffer.end()) noteBuffer.erase(it);
	noteBuffer.remove(note);
	if ((params[MONORETRIG_PARAM].getValue() > 0.5f) && (params[MONOPITCH_PARAM].getValue() != 1.f)) monoPulse.trigger(1e-3);
	if ((params[LOCKEDRETRIG_PARAM].getValue() > 0.5f) && (params[LOCKEDPITCH_PARAM].getValue() != 1.f)) lockedPulse.trigger(1e-3);
	for (int i = 0; i < numPads; i++)
	{
		if ((note == noteButtons[i].key) && (noteButtons[i].vel > 0)){
			// polyIndex = i;
			if (!noteButtons[i].button) noteButtons[i].vel = 0;
			
			noteButtons[i].gate = pedal && sustainhold;
			if ((noteButtons[i].mode == POLY_MODE) && (!noteButtons[i].gate) && (!noteBuffer.empty())){
				//recover if buffered over number of voices
				noteButtons[i].key = noteBuffer.front();
				noteBuffer.pop_front();
				noteButtons[i].gate = true;
				noteButtons[i].vel = noteButtons[i].velseq;
			}
		}
	}
	return;
}
void MIDIpoly16::releasePedalNotes() {
	//	gatePulse.trigger(1e-3);
	for (int i = 0; i < numPads; i++)
	{
		if (noteButtons[i].vel == 0){
			
			if (!noteBuffer.empty()){//recover if buffered over number of voices
			noteButtons[i].key = noteBuffer.front();
			noteButtons[i].vel = noteButtons[i].velseq;
			noteBuffer.pop_front();
			}else{
			noteButtons[i].gate = false;
			}
		}
	}
	return;
}

/////////////////////////////////////////////   SET POLY INDEX  /////////////////////////////////////////////
void MIDIpoly16::setPolyIndex(int note){

	 polymode = static_cast<int>(params[POLYMODE_PARAM].getValue());
	 if (polymode < 1) polyIndex ++;
	 else if (pedal && sustainhold){///check if note is held to recycle it....
			for (int i = 0; i < numPads; i ++){
						 if ((noteButtons[i].mode == POLY_MODE) && (noteButtons[i].gate) && (noteButtons[i].key == note)){
			//				///note is already on....
							lastpolyIndex = i;
							polyIndex = i;
							return;
						}
					}
			}else if (polymode > 1) polyIndex = 0;
 
	int ii = polyIndex;
	for (int i = 0; i < (polyTopIndex + 1); i ++){
		if (ii > polyTopIndex) ii = 0;
		//if (!(noteButtons[ii].locked)){
		  if (noteButtons[ii].mode == POLY_MODE){
			if (!noteButtons[ii].gate){
				lastpolyIndex = ii;
				polyIndex = ii;
				return;
			}
		}
		ii ++;
	}
	
	//////////scan to steal oldest note.......
	lastpolyIndex ++;
	ii = lastpolyIndex;
	for (int i = 0; i < (polyTopIndex + 1); i ++){
		if (ii > polyTopIndex) ii = 0;
	  //if (!(noteButtons[ii].locked)){
		if (noteButtons[ii].mode == POLY_MODE){
			lastpolyIndex = ii;
			polyIndex = ii;
			/// pulse for reTrigger
			//noteButtons[ii].polyretrig = false;
			///// save to Buffer /////

		   // int notebffr;
		   // notebffr = noteButtons[ii].key;
		  if (noteButtons[ii].vel > 0) noteBuffer.push_front(noteButtons[ii].key);		  ///////////////
			return;
		}
		ii ++;
	}
}

///////////////////////////////  END SET POLY INDEX  //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
//		MMM	      MMM   III   DDDDDDDD	   III
//		MMMM	 MMMM   III   DDD	DDD	   III
//		MMMMM   MMMMM   III   DDD	 DDD   III
//		MMMMMM MMMMMM   III   DDD	 DDD   III
//		MMM MMMMM MMM   III   DDD	 DDD   III
//		MMM  MMM  MMM   III   DDD	DDD	   III
//		MMM   M   MMM   III   DDDDDDDD	   III
//////////////////////////////////////////////////////////////////
void MIDIpoly16::processMessage(midi::Message msg) {
	if (msg.getStatus() == 0xf) {
		///clock
		processSystem(msg);
		return;
	}
	
	
	//if ((midiInput.channel < 0) || (midiInput.channel == msg.getChannel())){
		switch (msg.getStatus()) {
			 case 0x8: {
						releaseNote(msg.getNote() & 0x7f);
			 }
							 break;
			 case 0x9: {// note on
				 if (msg.getValue() > 0) {
					 pressNote((msg.getNote() & 0x7f), msg.getValue());
				 } else {
					 releaseNote(msg.getNote() & 0x7f);
				 }
			 }
				 break;
			 case 0xb: {
				 processCC(msg);
			 }
			  break;
				 // pitch wheel
			 case 0xe: {
				 pitch = msg.getValue() * 128 + msg.getNote();
			 }
			  break;
				 // channel aftertouch
			 case 0xd: {
				 pressure = msg.getNote();
			 }
			  break;

			 default: break;
		 }
	//}
}

void MIDIpoly16::processCC(midi::Message msg) {
	switch (msg.getNote()) {
			// mod
		case 0x01: {
			mod = msg.getValue();
		} break;
			// sustain
		case 0x40: {
			sustain = msg.getValue();
				if (sustain >= 64) {
					pedal = sustainhold;
				}else {
					pedal = false;
					if (sustainhold) releasePedalNotes();
				}
		} break;
		default: break;
	}
}

void MIDIpoly16::processSystem(midi::Message msg) {
		switch (msg.getChannel()) {
			case 0x8: {
			 //   debug("timing clock");
				clkMIDItick = true;
				seqMIDItick = true;
				arpMIDItick = true;
			} break;
			case 0xa: {
			  //  debug("start");
				MIDIstart = true;
			} break;
			case 0xb: {
			  //  debug("continue");
				MIDIcont = true;
			} break;
			case 0xc: {
			  //  debug("stop");
				MIDIstop = true;
			} break;
			default: break;
		}
}

void MIDIpoly16::MidiPanic() {
	pitch = 8192;
	outputs[PBEND_OUTPUT].setVoltage(0.f);
	mod = 0;
	outputs[MOD_OUTPUT].setVoltage(0.f);
	pressure = 0;
	outputs[PRESSURE_OUTPUT].setVoltage(0.f);
	sustain = 0;
	outputs[SUSTAIN_OUTPUT].setVoltage(0.f);
	pedal = false;
	for (int i = 0; i < numPads; i++)
	{
		noteButtons[i].vel = 0;
		noteButtons[i].gate = false;
		noteButtons[i].button = false;
	}
	noteBuffer.clear();
}
/////////////////////////////
////////////////////////////////// DISPLAY
//////////////////////////////////////////
void MIDIpoly16::getBPM(){
	if (firstBPM){
		firstBPM = false;
		sampleFrames = 0;
		return;
	}
	
	float fBPM =  APP->engine->getSampleRate() * 600.f / static_cast<float>(sampleFrames); // /  ;
	sampleFrames = 0;
	displayedBPM = static_cast<int>(fBPM + 0.5f);
}


float MIDIpoly16::minmaxFit(float val, float minv, float maxv){
	if (val < minv) val = minv;
	else if (val > maxv) val = maxv;
	return val;
}


///////////////////         ////           ////          ////        //////////////////////
/////////////////   ///////////////  /////////  ////////////  //////  ////////////////////
/////////////////         ////////  /////////      ////////         /////////////////////
///////////////////////   ///////  /////////  ////////////  ////////////////////////////
//////////////         /////////  /////////          ////  ////////////////////////////

void MIDIpoly16::process(const ProcessArgs &args) {
	
	//// mono modes and indexes
	int liveMonoMode = static_cast <int>(params[MONOPITCH_PARAM].getValue());
	int liveIx = 128 - liveMonoMode * 65; //first index -2 for Upper,(63 not used for Last), 128 for Lower
	int liveMonoIx = 0;
	int lockedMonoMode = static_cast <int>(params[LOCKEDPITCH_PARAM].getValue());
	int lockedIx =  128 - lockedMonoMode * 65; //first index -2 for Upper,(63 not used for Last), 128 for Lower
	int lockedMonoIx = 0;
	////
	clockSource = static_cast<int>(params[SEQCLOCKSRC_PARAM].getValue());
	if (clockSource == 1){
		if (inputs[SEQSPEED_INPUT].isConnected()){
			BPMdecimals = true;
			float vcBPMtime = minmaxFit(inputs[SEQSPEED_INPUT].getVoltage(),-10.f,10.f) * 24.f;
			BPMrate = minmaxFit(params[SEQSPEED_PARAM].getValue() + vcBPMtime, 12.f, 360.f);
		} else{
			BPMrate =  params[SEQSPEED_PARAM].getValue();
			BPMdecimals = false;
		}
		displayedBPM = static_cast<int>(BPMrate * 100.f + 0.5f);
	}


////////// MIDI MESSAGE ////////

   
	midi::Message msg;
	while (midiInput.shift(&msg)) {
		processMessage(msg);
	}
	
	bool analogdrift = (params[DRIFT_PARAM].getValue() > 0.0001f);
	bool newdrift = false;
	if (analogdrift){
		static int framedrift = 0;
		if (framedrift > args.sampleRate)
		{
			newdrift = true;
			framedrift = 4 * rand() % static_cast<int>(args.sampleRate/2.f);
		}else{
			newdrift = false;
			framedrift++;
		}
	}
	//////////////////////////////
	playingVoices = 0;
	bool monogate = false;
 //   bool retrigLive = false;
	bool lockedgate = false;
 //   bool retrigLocked = false;
	static int lockedM = 0;
	static int liveM = 0;
	for (int i = 0; i < numPads; i++)
	{
		if ((!noteButtons[i].button) && (params[KEYBUTTON_PARAM + i].getValue() > 0.5f)){ ///button ON
			if (noteButtons[i].mode == 0) noteButtons[i].polyretrig = noteButtons[i].gate;
			noteButtons[i].vel = 127;
			noteButtons[i].button = true;
			//noteButtons[i].monoretrig = true;
			/// update note press stamp for mono "last"
			stampIx ++;
			noteButtons[i].stamp = stampIx;
			//if ((noteButtons[i].mode <2)&&((params[MONORETRIG_PARAM].getValue() > 0.5f)||(params[LOCKEDRETRIG_PARAM].getValue() > 0.5f))){
			if (noteButtons[i].mode != SEQ_MODE) {
			keyPulse.trigger(1e-3);
			monoPulse.trigger(1e-3);
			lockedPulse.trigger(1e-3);
			}
			///////////// SET BUTTON MODE
			if (padSetMode>0) {
				//noteButtons[i].locked = !noteButtons[i].locked;
				if (noteButtons[i].mode != padSetMode) noteButtons[i].mode = padSetMode;
				else noteButtons[i].mode = POLY_MODE;
				initPolyIndex();
			}else if ((padSetLearn) && (noteButtons[i].mode >0)) ///learn if pad locked or Seq
				noteButtons[i].learn = !noteButtons[i].learn;
			////////////////////////////////////////////
			//if (!noteButtons[i].gate) noteButtons[i].vel = 64;

			
		} else if ((noteButtons[i].button) && (params[KEYBUTTON_PARAM + i].getValue() < 0.5f)){///button off
			if ((noteButtons[i].mode == POLY_MODE) && (liveMonoMode != 1)) monoPulse.trigger(1e-3);
			else if ((noteButtons[i].mode > SEQ_MODE) && (lockedMonoMode != 1)) lockedPulse.trigger(1e-3);
			noteButtons[i].button = false;
			if (!noteButtons[i].gate) noteButtons[i].vel = 0;
		}
		
		bool thisgate = noteButtons[i].gate || noteButtons[i].button;
		//// retrigger ...
		
		bool outgate = thisgate;
		if (noteButtons[i].polyretrig) outgate = !keyPulse.process(args.sampleTime);
		outputs[GATE_OUTPUT + i].setVoltage( !mutePoly && outgate ? 10.f : 0.f);
		
		if (thisgate){
		
		outputs[VEL_OUTPUT + i].setVoltage(noteButtons[i].velseq / 127.f * 10.f); //(velocity from seq 'cause it doesn't update to zero)
		if (!padSetLearn) noteButtons[i].learn = false;
		
		// get mono values
		if (noteButtons[i].mode == POLY_MODE){
			playingVoices ++;
			switch (liveMonoMode){
				case 0:{
					//// get lowest pressed note
					if (noteButtons[i].key < liveIx) {
						liveIx = noteButtons[i].key;
						liveM = i;
						monogate = true;
					}
					}break;
				case 1:{
					if (noteButtons[i].stamp > liveMonoIx ) {
						liveMonoIx = noteButtons[i].stamp;
						liveM = i;
						monogate = true;
					}
					}break;
				default:{
					//// get highest pressed note
					if (noteButtons[i].key > liveIx) {
						liveIx = noteButtons[i].key;
						liveM = i;
						monogate = true;
					}
					}break;
			}
		}else if ((noteButtons[i].mode == LOCKED_MODE) || ((noteButtons[i].mode == XLOCK_MODE)&&(params[PLAYXLOCKED_PARAM].getValue()>0.5))){
			switch (lockedMonoMode){
				case 0:{
					//// get lowest pressed note
					if (noteButtons[i].key < lockedIx) {
						lockedIx = noteButtons[i].key;
						lockedM = i;
						lockedgate = true;
					}
					}break;
				case 1:{
					if (noteButtons[i].stamp > lockedMonoIx ) {
						lockedMonoIx = noteButtons[i].stamp;
						lockedM= i;
						lockedgate = true;
					}
					}break;
				default:{
					//// get highest pressed note
					if (noteButtons[i].key > lockedIx) {
						lockedIx = noteButtons[i].key;
						lockedM = i;
						lockedgate = true;
					}
					}break;
			}
		}

	   ///////// POLY PITCH OUTPUT///////////////////////
			if (analogdrift){
				static int bounced[numPads] = {0};
				 float dlimit = (0.1f + params[DRIFT_PARAM].getValue() )/ 26.4f;
				const float driftfactor = 1e-9f;
				if (newdrift) {
					bounced[i] = 0;
					int randrange = static_cast<int>(1.f + params[DRIFT_PARAM].getValue()) * 300;
					drift[i] = (static_cast<float>(rand() % randrange * 2 - randrange)) * driftfactor * ( 0.5f + params[DRIFT_PARAM].getValue());
				}	   //
				noteButtons[i].drift += drift[i];
				if ((bounced[i] < 1) && (noteButtons[i].drift > dlimit ))  {
					drift[i] = -drift[i];
					bounced[i] = 1;
				}else if ((bounced[i] > -1) && (noteButtons[i].drift < - dlimit ))  {
					drift[i] = -drift[i];
					bounced[i] = -1;
				}
			
			}else noteButtons[i].drift = 0.f; // no analog drift
	 
			float noteUnison = 0.f;
			
			if (inputs[POLYUNISON_INPUT].isConnected())
			noteUnison = (minmaxFit(inputs[POLYUNISON_INPUT].getVoltage() * 0.1f ,-10.f,10.f)) * params[POLYUNISON_PARAM].getValue() * (noteButtons[liveMono].key - noteButtons[i].key);
			else  noteUnison = params[POLYUNISON_PARAM].getValue() * (noteButtons[liveMono].key - noteButtons[i].key);
			
			outputs[PITCH_OUTPUT + i].setVoltage(noteButtons[i].drift + (inputs[POLYSHIFT_INPUT].getVoltage()/48.f * params[TRIMPOLYSHIFT_PARAM].getValue()) + (polyTransParam + noteButtons[i].key + noteUnison - 60) / 12.f);	  //
			//////////////////////////////////////////////////
		}
		if (seqrunning) lights[SEQ_LIGHT + i].value = (seqStep == i) ? 1.f : 0.f;

		
	   }  //// end for i to numPads
	lockedMono = lockedM;
	liveMono = liveM;

	sustainhold = params[HOLD_PARAM].getValue() > 0.5f;
   

	//  Output Live Mono note
   // bool monogate = (noteButtons[liveMono].gate || noteButtons[liveMono].button);
	if (outputs[MONOPITCH_OUTPUT].isConnected()) {
		
		//////////////////////////////////////
		/////////////////////////////////  ///
		/// A R C A D E // S C A N ///	  //
		///////////////////////////////   ////
		////////////////////////////// // ////
		///// A R P E G G I A T O R //////////
		if (arpegMode > 0) {
			
			int updArpClockRatio = 0;
			if (inputs[ARPEGRATIO_INPUT].isConnected())
			updArpClockRatio = static_cast<int>(minmaxFit(inputs[ARPEGRATIO_INPUT].getVoltage() + params[ARPEGRATIO_PARAM].getValue(), 0.f, 11.f));
			else
			updArpClockRatio = static_cast<int>(params[ARPEGRATIO_PARAM].getValue());
			
			if (monogate){
				
				if (!arpegStarted) {
					arpegStarted = true;
					arpegIx = liveMono;
					arpegCycle = 0;
					arpOctIx = 0;
					arpSampleCount = 0;
					arpPhase = 0.f;
					arpTickCount = 0;
				   /////////////////////////////////
					syncArpPhase = seqrunning;///// SYNC with seq if running
				   /////////////////////////////////
				}
				
			   if (arpegMode > 1){/////// ARCADE / / / / / / /
				   arpegFrame ++;
				   if (arpegFrame > (args.sampleRate / 60.f)){
					   arpegStep = true;
					   arpegFrame = 0;
				   }
			   }else {//if (!seqrunning){ /// set clocks if sequence is not running.....
				float arpswingPhase;
				float  swingknob = params[SEQARPSWING_PARAM].getValue() / 40.f;
				   bool swingThis ((params[ARPSWING_PARAM].getValue() > 0.5f) && ((swingTriplet[arpclockRatio]) || (params[SWINGTRI_PARAM].getValue() > 0.5f)));
				   if (clockSource < 2) {
					   if (clockSource == 1) ClockArpSamples = static_cast<int>(args.sampleRate * 60.f/BPMrate / ClockRatios[arpclockRatio] + 0.5f);
						   arpPhase = static_cast<float>(arpSampleCount)/static_cast<float>(ClockArpSamples);
						   arpSampleCount++;
					   
					   if (swingThis){
							   if (arpSwingDwn) arpswingPhase = 1.f + swingknob;
							   else arpswingPhase = 1.f - swingknob;
						   } else arpswingPhase = 1.f;
					   
						   if (arpPhase >= arpswingPhase) {
							   arpPhase = 0.f;
							   arpSwingDwn = !arpSwingDwn;
							   arpegStep = true;
							   arpSampleCount = 0;
						   }
				   }else{
					   int arpMidiPhase;
					   int swingtick = static_cast<int>(swingknob * (24/ClockRatios[seqclockRatio]));
					   
					   if (arpMIDItick){
						   arpMIDItick = false;
						   arpTickCount ++; /// arpeggiator sync with sequencer
						   if (swingThis){
							   if (arpSwingDwn) arpMidiPhase = 24/ClockRatios[arpclockRatio] + swingtick;
							   else arpMidiPhase = 24/ClockRatios[arpclockRatio] - swingtick;
						   } else arpMidiPhase = 24/ClockRatios[arpclockRatio];
						   
						   if (arpTickCount >= arpMidiPhase) {
							   arpTickCount = 0;
							   arpSwingDwn = !arpSwingDwn;
							   arpegStep = true;
						   }
					   }
				   }
			}// END IF MODE ARP
			  /// first gate --> set up indexes
 
			}else{ //////////	GATE OFF //arpeg ON no gate (all notes off) reset..
				arpegStarted = false;
				//if (!seqrunning) {
					arpSampleCount = 0;
					arpPhase = 0.f;
					arpSwingDwn = true;
			   // }
		}
		bool notesFirst = (params[ARPEGOCTALT_PARAM].getValue() > 0.5f);
		
		if ((arpegStep) && (monogate)){
			
			if  (arpclockRatio != updArpClockRatio){
				arpclockRatio = updArpClockRatio;
				arpPhase = 0.f;
				arpSwingDwn = true;
				arpSampleCount = 0;
			}///update ratio....
			
			int arpOctaveKnob = static_cast<int>(params[ARPEGOCT_PARAM].getValue());
				 for (int i = 0 ; i < 5; i++){
				   lights[ARPOCT_LIGHT + i].value = 0.f;
				 }
			arpegStep=false;

				if (notesFirst) {
			   ////////// NOTE to arp and CYCLE FLAG
					arpegIx ++;
					if (arpegIx > polyTopIndex) arpegIx = 0;
					for (int i = 0; i <= polyTopIndex; i++){
						if ((noteButtons[arpegIx].mode == POLY_MODE) && ((noteButtons[arpegIx].gate) || (noteButtons[arpegIx].button))) {
							arpegCycle++;
							if (arpegCycle >= playingVoices){
								arpegCycle = 0;
								arpOctValue = arpOctaveKnob;
								if (octaveShift[arpOctValue][arpOctIx + 1] > 2){
									arpOctIx = 0;
								} else {
									arpOctIx ++;
								}
							}
							break;
						}
						arpegIx ++;
						if (arpegIx > polyTopIndex) arpegIx = 0;
						}
				///////////
				}else {
				   arpOctValue = arpOctaveKnob;
					if (octaveShift[arpOctValue][arpOctIx + 1] > 2) {
						arpegIx ++;
						arpOctIx= 0;
					for (int i = 0; i <= polyTopIndex; i++){
						if ((noteButtons[arpegIx].mode == POLY_MODE) && ((noteButtons[arpegIx].gate) || (noteButtons[arpegIx].button))) {
							break;
						}
						arpegIx ++;
						if (arpegIx > polyTopIndex) arpegIx = 0;
						}
					} else {
						arpOctIx ++;
					}
				}
			lights[ARPOCT_LIGHT + 2 + octaveShift[arpOctValue][arpOctIx]].value = 1.f;
			arpDisplayIx = arpegIx;
		}else if (!monogate){
			///keysUp...update arp ratio
			arpclockRatio = updArpClockRatio;
			arpDisplayIx = -1;
			for (int i = 0 ; i < 5; i++){
				lights[ARPOCT_LIGHT + i].value = 0.f;
			}
		}
			 outputs[MONOPITCH_OUTPUT].setVoltage(octaveShift[arpOctValue][arpOctIx] + (noteButtons[arpegIx].key - 60) / 12.f);
		}else{
			/// Normal Mono /////////////
			outputs[MONOPITCH_OUTPUT].setVoltage(noteButtons[liveMono].drift + (noteButtons[liveMono].key - 60) / 12.f);
		}
	} /// end if output active
	
	outputs[MONOVEL_OUTPUT].setVoltage(noteButtons[liveMono].vel / 127.f * 10.f);
	bool monoRtgGate = monogate;
	if ((params[MONORETRIG_PARAM].getValue() > 0.5f) && (noteButtons[liveMono].key != lastMono)){///if retrig
		monoRtgGate = !monoPulse.process(args.sampleTime);
		if (monoRtgGate) lastMono = noteButtons[liveMono].key;
	}
	
	outputs[MONOGATE_OUTPUT].setVoltage(!muteMono && monoRtgGate ? 10.f : 0.f);
 //////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
	
	//  Output locked Mono note
	outputs[LOCKEDVEL_OUTPUT].setVoltage(noteButtons[lockedMono].vel / 127.f * 10.f);
	outputs[LOCKEDPITCH_OUTPUT].setVoltage(noteButtons[lockedMono].drift + (noteButtons[lockedMono].key - 60) / 12.f);
	
	bool lockedRtgGate = lockedgate;
	if ((params[LOCKEDRETRIG_PARAM].getValue() > 0.5f) && (noteButtons[lockedMono].key != lastLocked)) {
		lockedRtgGate = !lockedPulse.process(args.sampleTime);
		if (lockedRtgGate) lastLocked = noteButtons[lockedMono].key; ///set new lastLive
	}
	outputs[LOCKEDGATE_OUTPUT].setVoltage(!muteLocked && lockedRtgGate ? 10.f : 0.f);
	
	if (pitch < 8192){
		outputs[PBEND_OUTPUT].setVoltage(pitchFilter.process(1.f, rescale(pitch, 0, 8192, -5.f, 0.f)));
	} else {
		outputs[PBEND_OUTPUT].setVoltage(pitchFilter.process(1.f, rescale(pitch, 8192, 16383, 0.f, 5.f)));
	}
	outputs[MOD_OUTPUT].setVoltage(modFilter.process(1.f, rescale(mod, 0, 127, 0.f, 10.f)));
	outputs[SUSTAIN_OUTPUT].setVoltage(sustainFilter.process(1.f, rescale(sustain, 0, 127, 0.f, 10.f)));
	outputs[PRESSURE_OUTPUT].setVoltage(pressureFilter.process(1.f, rescale(pressure, 0, 127, 0.f, 10.f)));
	
	
//////////////////// S E Q U E N C E R ////////////////////////////
	doSequencer(); /////// SEQ //////	/////// SEQ //////	/////// SEQ //////	/////// SEQ //////	/////// SEQ //////
//////////////////// S E Q U E N C E R ////////////////////////////
	
	
	///// RESET MIDI LIGHT
	if (resetMidiTrigger.process(params[RESETMIDI_PARAM].getValue())) {
		lights[RESETMIDI_LIGHT].value= 1.f;
		MidiPanic();
		return;
	}
	if (lights[RESETMIDI_LIGHT].value > 0.0001f)
		lights[RESETMIDI_LIGHT].value -= 0.0001f ; // fade out light

	bool pulseClk = clockPulse.process(1.f / args.sampleRate);
	outputs[SEQCLOCK_OUTPUT].setVoltage(pulseClk ? 10.f : 0.f);
	
	bool pulseStart = startPulse.process(1.f / args.sampleRate);
	outputs[SEQSTART_OUTPUT].setVoltage(pulseStart ? 10.f : 0.f);
	
	bool pulseStop = stopPulse.process(1.f / args.sampleRate);
	outputs[SEQSTOP_OUTPUT].setVoltage(pulseStop ? 10.f : 0.f);
	
	outputs[SEQSTARTSTOP_OUTPUT].setVoltage((pulseStop || pulseStart) ? 10.f : 0.f);
	
	////// TRIGGER BUTTONS....
	if (setSeqTrigger.process(params[SEQPAD_PARAM].getValue())){
		padSetMode = (padSetMode != SEQ_MODE)? SEQ_MODE:POLY_MODE;
		lights[PSEQ_LIGHT].value = (padSetMode==SEQ_MODE) ? 1.f : 0.f;
		lights[PLOCK_LIGHT].value = 0.f;
		lights[PXLOCK_LIGHT].value = 0.f;
		padSetLearn = false;
		lights[PLEARN_LIGHT].value = 0.f;
		for (int i = 0; i < numPads; i++) noteButtons[i].learn = false;
	}
	if (setLockTrigger.process(params[LOCKPAD_PARAM].getValue())){
		padSetMode = (padSetMode != LOCKED_MODE) ? LOCKED_MODE:POLY_MODE;
		lights[PLOCK_LIGHT].value = (padSetMode==LOCKED_MODE) ? 1.f : 0.f;
		lights[PSEQ_LIGHT].value = 0.f;
		lights[PXLOCK_LIGHT].value = 0.f;
		padSetLearn = false;
		lights[PLEARN_LIGHT].value = 0.f;
		for (int i = 0; i < numPads; i++) noteButtons[i].learn = false;
	}
	if (setPadTrigger.process(params[PADXLOCK_PARAM].getValue())){
		padSetMode = (padSetMode != XLOCK_MODE)? XLOCK_MODE:POLY_MODE;
		lights[PXLOCK_LIGHT].value = (padSetMode==XLOCK_MODE) ? 1.f : 0.f;
		lights[PLOCK_LIGHT].value = 0.f;
		lights[PSEQ_LIGHT].value = 0.f;
		padSetLearn = false;
		lights[PLEARN_LIGHT].value = 0.f;
		for (int i = 0; i < numPads; i++) noteButtons[i].learn = false;
	}
	if (setLearnTrigger.process(params[LEARNPAD_PARAM].getValue())){
		padSetLearn = !padSetLearn;
		lights[PLEARN_LIGHT].value = padSetLearn ? 1.f : 0.f;
		padSetMode = POLY_MODE;
		lights[PLOCK_LIGHT].value = 0.f;
		lights[PSEQ_LIGHT].value = 0.f;
		lights[PXLOCK_LIGHT].value = 0.f;
		if (!padSetLearn){///turn off learn on buttons
			for (int i = 0; i < numPads; i++) noteButtons[i].learn = false;
		}
	}

	
	if (inputs[ARPMODE_INPUT].isConnected()) {
		arpegMode = static_cast<int>(minmaxFit(inputs[ARPMODE_INPUT].getVoltage() * 0.3, 0.f, 2.f));
		arpegStatus = ((outputs[MONOPITCH_OUTPUT].isConnected())? 0 : 3 ) + arpegMode;
		lights[ARPEGON_LIGHT].value = (arpegMode==1) ? 1.f : 0.f;
		lights[ARCADEON_LIGHT].value = (arpegMode==2) ? 1.f : 0.f;
	}else{
		if (setArpegTrigger.process(params[ARPEGON_PARAM].getValue())){
			arpegMode = (arpegMode != 1)? 1:0;
			arpegStatus = ((outputs[MONOPITCH_OUTPUT].isConnected())? 0 : 3 ) + arpegMode;
			lights[ARPEGON_LIGHT].value = (arpegMode==1) ? 1.f : 0.f;
			lights[ARCADEON_LIGHT].value = (arpegMode==2) ? 1.f : 0.f;
		}
		if (setArcadeTrigger.process(params[ARCADEON_PARAM].getValue())){
			arpegMode = (arpegMode != 2)? 2:0;
			arpegStatus = ((outputs[MONOPITCH_OUTPUT].isConnected())? 0 : 3 ) + arpegMode;
			lights[ARPEGON_LIGHT].value = (arpegMode==1) ? 1.f : 0.f;
			lights[ARCADEON_LIGHT].value = (arpegMode==2) ? 1.f : 0.f;
		}
	}
	
	if (muteSeqTrigger.process(params[MUTESEQ_PARAM].getValue())){
		muteSeq = !muteSeq;
		lights[MUTESEQ_LIGHT].value = (muteSeq) ? 1.f : 0.f;
	}
	if (muteMonoTrigger.process(params[MUTEMONO_PARAM].getValue())){
		muteMono = !muteMono;
		lights[MUTEMONO_LIGHT].value = (muteMono) ? 1.f : 0.f;
	}
	if (muteLockedTrigger.process(params[MUTELOCKED_PARAM].getValue())){
		muteLocked = !muteLocked;
		lights[MUTELOCKED_LIGHT].value = (muteLocked) ? 1.f : 0.f;
	}
	if (mutePolyATrigger.process(params[MUTEPOLYA_PARAM].getValue())){
		mutePoly = !mutePoly;
		lights[MUTEPOLY_LIGHT].value = (mutePoly) ? 1.f : 0.f;
	}
	if (mutePolyBTrigger.process(params[MUTEPOLYB_PARAM].getValue())){
		mutePoly = !mutePoly;
		lights[MUTEPOLY_LIGHT].value = (mutePoly) ? 1.f : 0.f;
	}
	
	if (polyTransUpTrigger.process(params[POLYTRANUP_PARAM].getValue())){
		if (padSetLearn){
			for (int i = 0; i < numPads; i++)
			{
				if ((noteButtons[i].learn) && (noteButtons[i].key<127)) noteButtons[i].key ++;
			}
		}else if (polyTransParam < 48) polyTransParam ++;
	}
	if (polyTransDwnTrigger.process(params[POLYTRANDWN_PARAM].getValue())){
		if (padSetLearn){
			for (int i = 0; i < numPads; i++)
			{
				if ((noteButtons[i].learn) && (noteButtons[i].key > 0)) noteButtons[i].key --;
			}
		}else if (polyTransParam > -48) polyTransParam --;
	}
		if ((seqTransUpTrigger.process(params[SEQTRANUP_PARAM].getValue())) && (seqTransParam < 48)) seqTransParam ++;
		if ((seqTransDwnTrigger.process(params[SEQTRANDWN_PARAM].getValue())) && (seqTransParam > -48)) seqTransParam --;
	

	//if	(dispNotenumber != (params[DISPLAYNOTENUM_PARAM].getValue() > 0.5f))
		dispNotenumber = (params[DISPLAYNOTENUM_PARAM].getValue() > 0.5f);

}
/////////////////////// * * * ///////////////////////////////////////////////// * * *
//					  * * *		 E  N  D	  O  F	 S  T  E  P		  * * *
/////////////////////// * * * ///////////////////////////////////////////////// * * *

void MIDIpoly16::doSequencer(){

	/////// SEQ //////	/////// SEQ //////	/////// SEQ //////	/////// SEQ //////	/////// SEQ //////
	/////// SEQ //////	/////// SEQ //////	/////// SEQ //////	/////// SEQ //////	/////// SEQ //////
	
	
   //////// GET RATIO STEP OFFSET with CVs ....
	int updSeqClockRatio = 0;
	if (inputs[SEQRATIO_INPUT].isConnected())
		updSeqClockRatio = static_cast<int>(minmaxFit(inputs[SEQRATIO_INPUT].getVoltage() + params[SEQCLOCKRATIO_PARAM].getValue(), 0.f, 11.f));
	else
		updSeqClockRatio = static_cast<int>(params[SEQCLOCKRATIO_PARAM].getValue());
	
	if (updSeqClockRatio != seqclockRatio){
		seqclockRatio = updSeqClockRatio;
		syncArpPhase = true;
	}
	int updSeqFirst = 0;
	if (inputs[SEQFIRST_INPUT].isConnected())
		updSeqFirst = static_cast<int>(minmaxFit(inputs[SEQFIRST_INPUT].getVoltage() * 1.55f + params[SEQFIRST_PARAM].getValue(), 0.f, 15.f));
	else
		updSeqFirst = static_cast<int>(params[SEQFIRST_PARAM].getValue());
	if (updSeqFirst != seqOffset)
	{
		seqOffset = updSeqFirst;
		seqStep = seqOffset;
	}
	if (inputs[SEQSTEPS_INPUT].isConnected()) {
		seqSteps = static_cast <int>  (minmaxFit((inputs[SEQSTEPS_INPUT].getVoltage() * 1.55f + params[SEQSTEPS_PARAM].getValue()), 1.f,16.f));
	}else{
		seqSteps = static_cast <int> (params[SEQSTEPS_PARAM].getValue());
	}
   ///////////////////////////////////////////////
	
	bool seqResetNow = false;
	
	if ((params[SEQGATERUN_PARAM].getValue() > 0.5f) && (inputs[SEQRUN_INPUT].isConnected())) {
		if ((!seqrunning) && (inputs[SEQRUN_INPUT].getVoltage() > 3.f)){
			seqrunning = true;
			if (params[SEQRUNRESET_PARAM].getValue() > 0.5f) seqResetNow = true;
		} else  if ((seqrunning) && (inputs[SEQRUN_INPUT].getVoltage() < 2.f)){
			seqrunning = false;
		}
	} else { /// trigger selected for run or disconnected gate input...
		if (seqRunTrigger.process(params[SEQRUN_PARAM].getValue() + inputs[SEQRUN_INPUT].getVoltage())) {
			if (params[SEQRUNRESET_PARAM].getValue() > 0.5f){// if linked run > reset
				if (!seqrunning){
					seqrunning = true;
					seqResetNow = true;
				} else {
					seqrunning = false;
				}
			}else{ // reset not linked...regular toggle
				seqrunning = !seqrunning;
			}
		}
	}
	if (clockSource == 2){
		
		static int MIDIframe = 0;
			if (extBPM && (sampleFrames < (3 *  static_cast<int>(APP->engine->getSampleRate()))))
				sampleFrames ++;
			else////// 3 seconds without a tick......(20 bmp min)
			{
				extBPM = false;
				firstBPM = true;
				displayedBPM = 0;
			  //  MIDIframe = 0;
			}
		if (clkMIDItick){
			MIDIframe++;
			lights[MIDIBEAT_LIGHT].value = 0.f;
			if (MIDIframe >= 24)
			{
				extBPM = true;
				MIDIframe = 0;
				lights[MIDIBEAT_LIGHT].value = 1.f;
				getBPM();
			}
			clkMIDItick = false;
		}
		if(!inputs[SEQRUN_INPUT].isConnected()){
				if (MIDIstart){
					seqrunning = true;
					seqTickCount = 0;
					arpTickCount = 0;
					MIDIstart = false;
					seqResetNow = true;
				}
				if (MIDIcont){
					seqrunning = true;
					seqTickCount = 0;
					arpTickCount = 0;
					MIDIcont = false;
				}
				if (MIDIstop){
					seqrunning = false;
					MIDIstop = false;
				}
		}
	}else if (clockSource == 0) {
	   if (extBPM && (inputs[CLOCK_INPUT].isConnected()) && (sampleFrames < (3 * static_cast<int>(APP->engine->getSampleRate()))))
			sampleFrames ++;
		else////// 3 seconds without a tick......(20 bmp min)
			{
				extBPM = false;
				firstBPM = true;
				displayedBPM = 0;
			}
		
		if (extClockTrigger.process(inputs[CLOCK_INPUT].getVoltage())) {
			///// EXTERNAL CLOCK
			extBPM = true;
			ClockSeqSamples = static_cast<int>(static_cast<float>(sampleFrames) / ClockRatios[seqclockRatio]+0.5f);
			ClockArpSamples = static_cast<int>(static_cast<float>(sampleFrames) / ClockRatios[arpclockRatio]+0.5f);
			getBPM();
		}
	}
	// Reset manual
	if ((seqResetTrigger.process(params[SEQRESET_PARAM].getValue())) || ((inputs[SEQRESET_INPUT].isConnected()) && (seqResetTrigger.process(inputs[SEQRESET_INPUT].getVoltage())))) {
		lights[SEQRESET_LIGHT].value = 1.f;
		seqOctIx = 0;
		if (seqrunning) {
			seqResetNext = true;
		}else {
			seqResetNow = true;
		}
	}
	
	///
	if (seqResetNow){
		seqPhase = 0.f;
		seqi = 0;
		seqiWoct = 0;
		seqStep = seqOffset;
		seqOctIx = 0;
		seqSwingDwn = true;
		arpSwingDwn = true;
		arpPhase = 0.f;
		seqTickCount = 0;
		arpTickCount = 0;
	}
	////// seq run // // // // // // // // //
	bool nextStep = false;

	if (seqrunning) {
		
		int seqOctaveKnob = static_cast<int> (params[SEQOCT_PARAM].getValue());
		if (stopped){
			stopped = false;
			startPulse.trigger(1e-3);
			clockPulse.trigger(1e-3);
			seqOctValue = seqOctaveKnob;
			seqSampleCount = 0;
			seqPhase = 0.f;
			seqSwingDwn = true;
			arpSampleCount = 0;
			arpPhase = 0.f;
			arpSwingDwn = true;
			seqTickCount = 0;
			arpTickCount = 0;
		}
		float swingknob = params[SEQARPSWING_PARAM].getValue() / 40.f;
		float seqswingPhase;
		int swingtick;
		int swingMidiPhase;
		
		bool notesFirst = (params[SEQOCTALT_PARAM].getValue() > 0.5f);
		bool DontSwing = true;
		if ((params[SEQSWING_PARAM].getValue() > 0.5f) && ((swingTriplet[seqclockRatio]) || (params[SWINGTRI_PARAM].getValue() > 0.5f))){
		int lastStepByOct = seqSteps * (1 + std::abs(seqOctValue-3));
		///if steps is Odd Don't swing last step..if Octaves first last Step is seqSteps * Octave cycles....if notes first last step is seqSteps...
		DontSwing = (((seqSteps % 2 == 1) && (notesFirst) && ((seqStep - seqOffset) == (seqSteps - 1))) ||
							((lastStepByOct % 2 == 1) && (!notesFirst) && (seqiWoct == (lastStepByOct - 1))));
		}
		if (clockSource < 2) {
			if (clockSource == 1) ClockSeqSamples = static_cast<int>(APP->engine->getSampleRate() * 60.f/BPMrate / ClockRatios[seqclockRatio] + 0.5f);
 
				seqPhase = static_cast<float>(seqSampleCount)/static_cast<float>(ClockSeqSamples);
				seqSampleCount++;
			
				if (DontSwing) {
					seqswingPhase = 1.f;
				}else{
					if (seqSwingDwn){
						seqswingPhase = 1.f + swingknob;
					}else{
						seqswingPhase = 1.f - swingknob;
					}
				}
				if (seqPhase >= seqswingPhase) {
					seqPhase = 0.f;
					seqSwingDwn = !seqSwingDwn;
					nextStep = true;
					seqSampleCount = 0;
					if (DontSwing) seqSwingDwn = true;
				}
		}else{
			
			swingtick = static_cast<int>(swingknob * (24/ClockRatios[seqclockRatio]));
			if (seqMIDItick){
				seqMIDItick = false;
				seqTickCount ++;
				if (DontSwing) {
					swingMidiPhase = 24/ClockRatios[seqclockRatio];
				}else{
					if (seqSwingDwn){
						swingMidiPhase = 24/ClockRatios[seqclockRatio] + swingtick;
					}else{
						swingMidiPhase = 24/ClockRatios[seqclockRatio] - swingtick;
					}
				}
				if (seqTickCount >= swingMidiPhase){
					seqTickCount = 0;
					seqSwingDwn = !seqSwingDwn;
					nextStep = true;
					if (DontSwing) seqSwingDwn = true;
				}
			}
			
		}
		
		bool gateOut;
		bool pulseTrig = gatePulse.process(1.f / APP->engine->getSampleRate());
		gateOut = (noteButtons[seqStep].velseq > 0) && (!(pulseTrig && (params[SEQRETRIG_PARAM].getValue() > 0.5f)));
		//// if note goes out to seq...(if not the outputs hold the last played value)
		if (params[SEQSEND_PARAM + seqStep].getValue() > 0.5f){
			outputs[SEQPITCH_OUTPUT].setVoltage(noteButtons[seqStep].drift + octaveShift[seqOctValue][seqOctIx] + (inputs[SEQSHIFT_INPUT].getVoltage() * params[TRIMSEQSHIFT_PARAM].getValue() /48.f ) + (seqTransParam + noteButtons[seqStep].key - 60) / 12.f);
			outputs[SEQVEL_OUTPUT].setVoltage(noteButtons[seqStep].velseq / 127.f * 10.f);
			outputs[SEQGATE_OUTPUT].setVoltage(!muteSeq && gateOut ? 10.f : 0.f);
			///if individual gate / vel
			if (params[SEQSEND_PARAM + seqStep].getValue() > 1.5f){
				noteButtons[seqStep].gateseq = true;
			 //   outputs[PITCH_OUTPUT + seqStep].setVoltage(noteButtons[seqStep].drift + octaveShift[seqOctValue][seqOctIx] + (seqTransParam + noteButtons[seqStep].key - 60) / 12.f);
				outputs[VEL_OUTPUT + seqStep].setVoltage(noteButtons[seqStep].velseq / 127.f * 10.f);
				outputs[GATE_OUTPUT + seqStep].setVoltage(!muteSeq && gateOut ? 10.f : 0.f);
			}
		}else{
			outputs[SEQGATE_OUTPUT].setVoltage(0.f);
		}
		
		  if (nextStep) {
		// restore gates and vel from individual outputs...
		if (params[SEQSEND_PARAM + seqStep].getValue() > 1.5f){
			noteButtons[seqStep].gateseq = false;
		   // outputs[PITCH_OUTPUT + seqStep].setVoltage(noteButtons[seqStep].drift + (noteButtons[seqStep].key - 60) / 12.f);
			outputs[VEL_OUTPUT + seqStep].setVoltage(noteButtons[seqStep].vel / 127.f * 10.f);
			outputs[GATE_OUTPUT + seqStep].setVoltage(noteButtons[seqStep].gate ? 10.f : 0.f);
		}
			for (int i = 0 ; i < 5; i++){
				lights[SEQOCT_LIGHT + i].value = 0.f;
			}
			if ((syncArpPhase) && (seqSwingDwn)) {
				arpSampleCount = 0;
				arpPhase = 0.f;
				arpSwingDwn = true;
				syncArpPhase = false;
				arpTickCount = 0;
			}

			clockPulse.trigger(1e-3);
			gatePulse.trigger(1e-3);
			////////////////////////
			
			if (seqResetNext){ ///if reset while running
				seqResetNext = false;
				seqPhase = 0.f;
				seqi = 0;
				seqiWoct = 0;
				seqOctIx = 0;
				seqStep = seqOffset;
				seqSwingDwn = true;
				arpPhase = 0.f;
				arpSwingDwn = true;
				seqTickCount = 0;
				arpTickCount = 0;
				seqSampleCount = 0;
				arpSampleCount = 0; /// SYNC THE ARPEGG
  
			}else{

			seqiWoct ++;
					if (notesFirst) {
					////// ADVANCE STEP ///////
					seqi ++;
						if ( seqi > (seqSteps - 1)) { ///seq Cycle ... change octave ?
								seqOctValue = seqOctaveKnob;
										if (octaveShift[seqOctValue][seqOctIx + 1] > 2) {
											seqOctIx= 0;
											seqiWoct = 0;
										}else{
											seqOctIx ++;
										}
								seqi = 0;// next cycle
							}
				   }else{
					   if (octaveShift[seqOctValue][seqOctIx + 1] > 2) {
						   seqOctValue = seqOctaveKnob;
						   seqOctIx = 0;
						   seqi ++;
						   if ( seqi > (seqSteps - 1)) {
							   seqi = 0;// next cycle
							   seqiWoct = 0;
						   }
						   
					   }else{
						   seqOctIx ++;
					   }
				  }
				 seqStep = ((seqi % seqSteps) + seqOffset) % numPads;
			}
		lights[SEQOCT_LIGHT + 2 + octaveShift[seqOctValue][seqOctIx]].value = 1.f;
		lights[SEQRUNNING_LIGHT].value = 1.f;
		}
		
	} else { ///stopped shut down gate....
		if (!stopped){
			lights[SEQRUNNING_LIGHT].value = 0.f;
			noteButtons[seqStep].gateseq = false;
			stopped = true;
			stopPulse.trigger(1e-3);
			//seqOctIx = 0;
			//seqi = 0;
			//seqiWoct = 0;
			for (int i = 0 ; i < 5; i++){
				lights[SEQOCT_LIGHT + i].value = 0.f;
			}
			outputs[SEQGATE_OUTPUT].setVoltage(0.f);
		}
	}
	
	if (lights[SEQRESET_LIGHT].value > 0.0001f)
		lights[SEQRESET_LIGHT].value -= 0.0001f;
	
	/////// SEQ ////// ------- E N D  ---------  ------- E N D  ---------  ------- E N D  --------- /////// SEQ //////
	return;
}

///////////BUTTONS' DISPLAY

 struct NoteDisplay : TransparentWidget {
	MIDIpoly16::noteButton *nButton = new MIDIpoly16::noteButton();
	NoteDisplay() {
			font = APP->window->loadFont(FONT_FILE);
	}
	float dfontsize = 12.f;
	MIDIpoly16 *module;
	int key = 0;
	int vel = 0;
	bool gate = false;
	bool gateseq = false;
	bool newkey = false;
	int frame = 0;
	int framevel = 0;
	bool showvel = true;
	int polyIndex = 0;
	int id = 0;
	int mode = 0;
	bool learn = false;
	bool dispNotenumber = false;
	int arpIx = 0;
	bool arp = false;
	std::shared_ptr<Font> font;
	std::string to_display;
	std::string displayNoteName(int key, bool notenumber);
	 
	void draw(const DrawArgs &args) {
		if (module) {
			nButton = &(module->noteButtons[id]);
		
		frame ++;
		if (frame > 5){
			frame = 0;
			newkey = nButton->newkey;
			key =  nButton->key;
			vel = nButton->vel;
			gate = nButton->gate;
			gateseq = nButton->gateseq;
			mode = nButton->mode;
			learn = nButton->learn;
		}
		int rrr,ggg,bbb,aaa;
		if (learn) {
			rrr=0xff; ggg=0xff; bbb=0x00;
			aaa=128;
		} else { switch (mode)  {
			case 0:{/// Poly
				rrr=0xff; ggg=0xff; bbb=0xff;
				aaa= vel;
			}break;
			case 1:{/// Seq
				rrr=0x00; ggg=0x99; bbb=0xff;
				aaa= 64 + vel * 1.5f;
			}break;
			case 2:{/// Lock
				rrr=0xff; ggg=0x00; bbb=0x00;
				aaa= 64 + vel * 1.5f;
			}break;
			default:{/// xLock
				rrr=0xff; ggg=0x80; bbb=0x00;
				aaa= 64 + vel * 1.5f;
			}break;
			}
		}
		NVGcolor backgroundColor = nvgRGBA(rrr*0.6f, ggg*0.6f, bbb*0.6f, aaa);
		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 6.f);
		nvgFillColor(args.vg, backgroundColor);
		nvgFill(args.vg);
		if (newkey) {
			showvel = true;
			framevel = 0;
			nButton->newkey = false;
		}else{
			to_display = displayNoteName(key, module->dispNotenumber);
		}
		if (learn) {
			NVGcolor borderColor = nvgRGB(rrr, ggg, bbb);
			nvgStrokeWidth(args.vg, 2.f);
			nvgStrokeColor(args.vg, borderColor);
			nvgStroke(args.vg);
		} else if (gate || gateseq){
			if (showvel) {
					   if (++framevel <= 20) {
						to_display = "v" + std::to_string(vel);
					   } else {
						   showvel = false;
						   framevel = 0;
					   }
				}
			NVGcolor borderColor = nvgRGB(rrr, ggg, bbb);
			nvgStrokeWidth(args.vg, 2.f);
			nvgStrokeColor(args.vg, borderColor);
			nvgStroke(args.vg);
		}
		if (module->arpDisplayIx == id)
		{ ///arp led
			NVGcolor ledColor = nvgRGB(0xff, 0xff, 0xff);
			nvgBeginPath(args.vg);
			nvgRect(args.vg, 4 , 8, 2, 2);
			nvgFillColor(args.vg, ledColor);
			nvgFill(args.vg);
		   // nButton->arp = false;
		}
		if (module->polyIndex == id)
		{ ///led
			NVGcolor ledColor = nvgRGB(0xff, 0xff, 0xff);
			nvgBeginPath(args.vg);
			nvgRect(args.vg, 4 , 4, 2, 2);
			nvgFillColor(args.vg, ledColor);
			nvgFill(args.vg);
		}
		nvgFontSize(args.vg, dfontsize);
		nvgFontFaceId(args.vg, font->handle);
		//nvgTextLetterSpacing(args.vg, 2.f);
		NVGcolor textColor = nvgRGB(rrr, ggg, bbb);
		nvgFillColor(args.vg, textColor);
	   // nvgTextLetterSpacing(args.vg, 3.f);
		nvgTextAlign(args.vg, NVG_ALIGN_CENTER);
		//Vec textPos = Vec(47 - 14 * to_display.length(), 18.f);
		//Vec textPos = Vec(5.f, 20.f);
		nvgTextBox(args.vg, 0.f, 19.f, 48.f ,to_display.c_str(), NULL);
	}
 }
};

/////////// MIDI NOTES to Text
std::string NoteDisplay::displayNoteName(int value, bool notenumber)
{
	std::string notename[12] = {"C","C#","D","Eb","E","F","F#","G","Ab","A","Bb","B"};
	if (notenumber){
		return "n"+ std::to_string(value);
	}else{
	   int octaveint = (value / 12) - 2;
	   return notename[value % 12] + std::to_string(octaveint);
	}
}

///// MAIN DISPLAY //////


struct digiDisplay : TransparentWidget {
	digiDisplay() {
		font = APP->window->loadFont(FONT_FILE);
	}
	MIDIpoly16 *module;
	
	int initpointer = 0;
	bool initpointerb = false;
	float mdfontSize = 10.f;
	std::shared_ptr<Font> font;

	const std::string stringClockRatios[13] ={"1/2", "1/4d","1/2t", "1/4", "1/8d", "1/4t","1/8","1/16d","1/8t","1/16","1/16t","1/32","1/32t"};
	
	std::string seqDisplayedTr = "";
	std::string polyDisplayedTr = "";
	std::string clockDisplay = "";
	std::string seqDisplay = "";
	std::string arpegDisplay = "";
	std::string voicesDisplay = "";

	bool BPMdecimalsP = false;
	int clockSourceP = 0;
	int displayedBPMP = 0;
	int seqclockRatioP = 1;
	int seqStepsP = 16;
	int seqOffsetP = 0;
	int arpclockRatioP = 1;
	int arpegStatusP = 0;
	int polyMaxVoicesP = 16;
	int playingVoicesP = 0;
	int seqtransP = 0;
	int polytransP = 0;
	

	
	float thirdlineoff = 0.f;
	bool thirdline = false;
	int frame = 0;
	void draw(const DrawArgs &args) override
	{
		if (module){
			clockSourceP = module->clockSource;
			BPMdecimalsP = module->BPMdecimals;
			displayedBPMP = module->displayedBPM;
			seqclockRatioP = module->seqclockRatio;
			seqStepsP = module->seqSteps;
			seqOffsetP = module->seqOffset;
			polyMaxVoicesP = module->polyMaxVoices;
			playingVoicesP = module->playingVoices;
			arpegStatusP = module->arpegStatus;
			arpclockRatioP = module->arpclockRatio;
			seqtransP = module->seqTransParam;
			polytransP = module->polyTransParam;
		
		if (frame ++ > 5 )
		{
			
			switch (arpegStatusP){
				case 0:{
					arpegDisplay = "";
					thirdline = false;
				}break;
				case 1:{
					arpegDisplay = "Arpeggiator: " + stringClockRatios[arpclockRatioP];
					thirdline = true;
				}break;
				case 2:{
					arpegDisplay = "Arpeggiator: Arcade";
					thirdline = true;
				}break;
				case 3:{
					arpegDisplay = "";
					thirdline = false;
				}break;
				default:{
					arpegDisplay = "Arpeg.connect mono out";
					thirdline = true;
				}break;
			}

			switch (clockSourceP){
				case 0:{
					int BPMint = static_cast<int>(displayedBPMP / 10.f);
					int BPMdec = static_cast<int>(displayedBPMP) % 10;
					if (displayedBPMP < 1){
						clockDisplay = "EXT ...no clock...";
					} else {
						clockDisplay = "EXT " + std::to_string(BPMint) + "." + std::to_string(BPMdec) +" bpm "+ stringClockRatios[seqclockRatioP];
					}
				}break;
				case 1:{
					int BPMint = static_cast<int>(displayedBPMP / 100.f);
					if (BPMdecimalsP){
						int BPMdec = static_cast<int>(displayedBPMP) % 100;
						clockDisplay = "INT " + std::to_string(BPMint) + "." + std::to_string(BPMdec) +" bpm "+ stringClockRatios[seqclockRatioP];
					} else{
						clockDisplay = "INT " + std::to_string(BPMint) + " bpm "+ stringClockRatios[seqclockRatioP];
					}

				}break;
				case 2:{
					int BPMint = static_cast<int>(displayedBPMP / 10.f);
					int BPMdec = static_cast<int>(displayedBPMP) % 10;
					if (displayedBPMP < 1){
						clockDisplay = "MIDI ...no clock...";
					} else {
						clockDisplay = "MIDI " + std::to_string(BPMint) + "." + std::to_string(BPMdec) +" bpm "+ stringClockRatios[seqclockRatioP];
					}
				}break;
			}
		
			seqDisplay = "Steps: " + std::to_string(seqStepsP) + " First: " + std::to_string(seqOffsetP + 1);
			voicesDisplay = std::to_string(playingVoicesP)+"/"+std::to_string(polyMaxVoicesP);
			seqDisplayedTr = std::to_string(seqtransP);
			polyDisplayedTr = std::to_string(polytransP);
			
			frame = 0;
		}
		
		nvgFillColor(args.vg, nvgRGBA(0xFF,0xFF,0xFF,0xFF));
		nvgFontSize(args.vg, mdfontSize);
		nvgFontFaceId(args.vg, font->handle);
		nvgTextAlign(args.vg, NVG_ALIGN_CENTER);
		
		if (thirdline) {
			nvgTextBox(args.vg, 0.f, 9.f + mdfontSize * 3.f, box.size.x, arpegDisplay.c_str(), NULL);
			thirdlineoff = 0.f;
		} else {thirdlineoff = 6.f;}
		nvgTextBox(args.vg, 0.f, 3.f + mdfontSize + thirdlineoff, box.size.x, clockDisplay.c_str(), NULL);
		nvgTextBox(args.vg, 0.f, 6.f + mdfontSize * 2.f + thirdlineoff, box.size.x, seqDisplay.c_str(), NULL);
		nvgTextBox(args.vg, 87.f, -21.f, 48.f, voicesDisplay.c_str(), NULL);//PolyVoices Display
		nvgTextBox(args.vg, 240.f, -21.f,30.f, polyDisplayedTr.c_str(), NULL);//VoicesTransp Display
		nvgTextBox(args.vg, -7.f, 284.f,30.f, seqDisplayedTr.c_str(), NULL);//SeqTransp Display
	}
	}
};



struct SelectorKnob : moDllzSelector32 {
	SelectorKnob() {
	minAngle = -0.88*M_PI;
	maxAngle = 1.f*M_PI;
	}
//	void onAction(EventAction &e) override {
//	}
};
struct RatioKnob : moDllzSmSelector {
	RatioKnob() {
	  //  box.size ={36,36};
		minAngle = -0.871*M_PI;
		maxAngle = 1.0*M_PI;
	}
};


struct SelectorOct : moDllzSelector32 {//Oct
	SelectorOct() {
	   // box.size ={32,32};
		minAngle = -0.44f*M_PI;
		maxAngle = 0.44f*M_PI;
	}
};
struct Knob26 : moDllzKnob26 {///Unison
	Knob26() {
		//snap = true;
		minAngle = -0.75*M_PI;
		maxAngle = 0.75*M_PI;
	}
};
struct Knob26Snap : moDllzKnob26 {///swing
	Knob26Snap() {
		//box.size ={26,26};
		snap = true;
		minAngle = -0.75*M_PI;
		maxAngle = 0.75*M_PI;
	}
};
struct KnobSnap : moDllzKnobM {//BPM
	KnobSnap() {
		snap = true;
		minAngle = -0.836*M_PI;
		maxAngle = 1.0*M_PI;
	}
};

struct TangerineLight : GrayModuleLightWidget {
	TangerineLight() {
		addBaseColor(nvgRGB(0xff, 0x80, 0x00));
	}
};
struct WhiteYLight : GrayModuleLightWidget {
	WhiteYLight() {
		addBaseColor(nvgRGB(0xee, 0xee, 0x88));
	}
};


/////////////////////////////////////////////// WIDGET ///////////////////////////////////////////////

struct MIDIpoly16Widget : ModuleWidget
{
	MIDIpoly16Widget(MIDIpoly16 *module){
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/MIDIPoly.svg")));
		//Screws
		addChild(createWidget<ScrewBlack>(Vec(0, 0)));
		addChild(createWidget<ScrewBlack>(Vec(135, 0)));
		addChild(createWidget<ScrewBlack>(Vec(435, 0)));
		addChild(createWidget<ScrewBlack>(Vec(585, 0)));
		addChild(createWidget<ScrewBlack>(Vec(0, 365)));
		addChild(createWidget<ScrewBlack>(Vec(135, 365)));
		addChild(createWidget<ScrewBlack>(Vec(435, 365)));
		addChild(createWidget<ScrewBlack>(Vec(585, 365)));
	
  
	float xPos = 8.f;//61;
	float yPos = 18.f;
		
		MidiWidget *midiWidget = createWidget<MidiWidget>(Vec(xPos,yPos));
		midiWidget->box.size = Vec(128.f,36.f);
		midiWidget->setMidiPort(module ? &module->midiInput : NULL);
		
		midiWidget->driverChoice->box.size.y = 12.f;
		midiWidget->deviceChoice->box.size.y = 12.f;
		midiWidget->channelChoice->box.size.y = 12.f;
		
		midiWidget->driverChoice->box.pos = Vec(0.f, 0.f);
		midiWidget->deviceChoice->box.pos = Vec(0.f, 12.f);
		midiWidget->channelChoice->box.pos = Vec(0.f, 24.f);
		
		midiWidget->driverSeparator->box.pos = Vec(0.f, 12.f);
		midiWidget->deviceSeparator->box.pos = Vec(0.f, 24.f);
		
//		midiWidget->driverChoice->font = APP->window->loadFont(mFONT_FILE);
//		midiWidget->deviceChoice->font = APP->window->loadFont(mFONT_FILE);
//		midiWidget->channelChoice->font = APP->window->loadFont(mFONT_FILE);
		
		midiWidget->driverChoice->textOffset = Vec(2.f,10.f);
		midiWidget->deviceChoice->textOffset = Vec(2.f,10.f);
		midiWidget->channelChoice->textOffset = Vec(2.f,10.f);
		
		midiWidget->driverChoice->color = nvgRGB(0xdd, 0xdd, 0xdd);
		midiWidget->deviceChoice->color = nvgRGB(0xdd, 0xdd, 0xdd);
		midiWidget->channelChoice->color = nvgRGB(0xdd, 0xdd, 0xdd);
		addChild(midiWidget);
		
		xPos = 98.f;
		yPos = 42.f;
	addParam(createParam<moDllzMidiPanic>(Vec(xPos, yPos), module, MIDIpoly16::RESETMIDI_PARAM));
	addChild(createLight<SmallLight<RedLight>>(Vec(xPos + 3.f, yPos + 4.f), module, MIDIpoly16::RESETMIDI_LIGHT));

	yPos = 20.f;
	// PolyMode
   
	xPos = 205.f;
	addParam(createParam<moDllzSwitchT>(Vec(xPos,yPos), module, MIDIpoly16::POLYMODE_PARAM));
	//Shift
	xPos = 252.f;
	addInput(createInput<moDllzPort>(Vec(xPos,yPos - 0.5f),  module, MIDIpoly16::POLYSHIFT_INPUT));
	xPos = 277.f;
	addParam(createParam<TTrimSnap>(Vec(xPos,yPos + 4.f), module, MIDIpoly16::TRIMPOLYSHIFT_PARAM));
	
	
	// Poly Transp ...
	xPos = 333.f;
	addParam(createParam<moDllzPulseUp>(Vec(xPos,yPos), module, MIDIpoly16::POLYTRANUP_PARAM));
	addParam(createParam<moDllzPulseDwn>(Vec(xPos,yPos + 11.f), module, MIDIpoly16::POLYTRANDWN_PARAM));
	addChild(createLight<TinyLight<YellowLight>>(Vec(xPos + 8.f, yPos + 25.f), module, MIDIpoly16::PLEARN_LIGHT));

	
	//Unison Drift
	xPos = 351.f;
	addInput(createInput<moDllzPort>(Vec(xPos, yPos+7.5f),  module, MIDIpoly16::POLYUNISON_INPUT));
	xPos = 379.f;
	addParam(createParam<moDllzKnob32>(Vec(xPos ,yPos), module, MIDIpoly16::POLYUNISON_PARAM) );
	xPos = 433.f;
	addParam(createParam<Knob26>(Vec(xPos-1,yPos-2), module, MIDIpoly16::DRIFT_PARAM));
	
	//Midi Numbers / notes
	xPos = 472.f;
	addParam(createParam<moDllzSwitch>(Vec(xPos,yPos+10), module, MIDIpoly16::DISPLAYNOTENUM_PARAM));
	
	xPos = 509;
	// seq xLock
	yPos = 21;
	addParam(createParam<moDllzClearButton>(Vec(xPos, yPos), module, MIDIpoly16::SEQPAD_PARAM));
	addChild(createLight<SmallLight<BlueLight>>(Vec(xPos+28, yPos + 3), module, MIDIpoly16::PSEQ_LIGHT));
	yPos = 37;
	addParam(createParam<moDllzClearButton>(Vec(xPos, yPos), module, MIDIpoly16::PADXLOCK_PARAM));
	addChild(createLight<SmallLight<TangerineLight>>(Vec(xPos+28, yPos + 3), module, MIDIpoly16::PXLOCK_LIGHT));
	xPos = 550;
	// Lock Learn
	yPos = 21;
	addParam(createParam<moDllzClearButton>(Vec(xPos, yPos), module, MIDIpoly16::LEARNPAD_PARAM));
	addChild(createLight<SmallLight<YellowLight>>(Vec(xPos+28, yPos + 3), module, MIDIpoly16::PLEARN_LIGHT));
	yPos = 37;
	addParam(createParam<moDllzClearButton>(Vec(xPos, yPos), module, MIDIpoly16::LOCKPAD_PARAM));
	addChild(createLight<SmallLight<RedLight>>(Vec(xPos+28, yPos + 3), module, MIDIpoly16::PLOCK_LIGHT));
	
	//////// Note Buttons X 16 ///////////
	yPos = 66;
	xPos = 272;
	for (int i = 0; i < MIDIpoly16::numPads; i++)
	{
		addParam(createParam<moDllzMoButton>(Vec(xPos, yPos), module, MIDIpoly16::KEYBUTTON_PARAM + i));
		{
			NoteDisplay *notedisplay = new NoteDisplay();
			notedisplay->box.pos = Vec(xPos,yPos);
			notedisplay->box.size = Vec(48, 27);
			notedisplay->module = module;
			notedisplay->id = i;
//			notedisplay->polyIndex = &(module->polyIndex);
//			notedisplay->arpIx = &(module->arpDisplayIx);
//			notedisplay->dispNotenumber = &(module->dispNotenumber);
//			notedisplay->nButton = &(module->noteButtons[i]);
			addChild(notedisplay);
		}
		addParam(createParam<moDllzSwitchLedHT>(Vec(xPos + 54 ,yPos + 11), module, MIDIpoly16::SEQSEND_PARAM + i));
		addChild(createLight<TinyLight<BlueLight>>(Vec(xPos + 64.2, yPos + 4), module, MIDIpoly16::SEQ_LIGHT + i));
		addOutput(createOutput<moDllzPortDark>(Vec(xPos + 81, yPos + 3.5f),  module, MIDIpoly16::PITCH_OUTPUT + i));
		addOutput(createOutput<moDllzPortDark>(Vec(xPos + 105, yPos + 3.5f),  module, MIDIpoly16::VEL_OUTPUT + i));
		addOutput(createOutput<moDllzPortDark>(Vec(xPos + 129, yPos + 3.5f),  module, MIDIpoly16::GATE_OUTPUT + i));
		
		yPos += 31;
		if (yPos > 300){
			yPos = 66;
			xPos += 165;
		}
	}
	////SEQ////


	yPos = 125;
	//Seq SPEED and Ratio knobs
	xPos = 79;
	addParam(createParam<KnobSnap>(Vec(xPos,yPos), module, MIDIpoly16::SEQSPEED_PARAM));
	xPos = 146;
	addParam(createParam<RatioKnob>(Vec(xPos,yPos+2), module, MIDIpoly16::SEQCLOCKRATIO_PARAM));
	
	yPos = 167.5f;
	//Seq SPEED and Ratio inputs
	xPos = 71.5f;
	addInput(createInput<moDllzPort>(Vec(xPos,yPos),  module, MIDIpoly16::SEQSPEED_INPUT));
	xPos = 134.5f;
	addInput(createInput<moDllzPort>(Vec(xPos,yPos),  module, MIDIpoly16::SEQRATIO_INPUT));
	
	xPos = 89;
	// STEP FIRST KNOBS
	yPos = 207;
	addParam(createParam<SelectorKnob>(Vec(xPos,yPos), module, MIDIpoly16::SEQSTEPS_PARAM));
	yPos = 266;
	addParam(createParam<SelectorKnob>(Vec(xPos,yPos), module, MIDIpoly16::SEQFIRST_PARAM));
	
	xPos = 67.5f;
	// STEP FIRST > > > > > > > >  INPUTS
	yPos = 226.5f;
	addInput(createInput<moDllzPort>(Vec(xPos,yPos),  module, MIDIpoly16::SEQSTEPS_INPUT));
	yPos = 285.5f;
	addInput(createInput<moDllzPort>(Vec(xPos,yPos),  module, MIDIpoly16::SEQFIRST_INPUT));

	yPos = 262;
	// Swing..... swing Triplets
	xPos = 134;
	addParam(createParam<Knob26Snap>(Vec(xPos,yPos), module, MIDIpoly16::SEQARPSWING_PARAM));
	addParam(createParam<moDllzSwitchLed>(Vec(xPos+31,yPos+9), module, MIDIpoly16::SEQSWING_PARAM));
	addParam(createParam<moDllzSwitchLed>(Vec(xPos+48,yPos+9), module, MIDIpoly16::ARPSWING_PARAM));
	addParam(createParam<moDllzSwitchLedH>(Vec(xPos+20,yPos+37), module, MIDIpoly16::SWINGTRI_PARAM));
	
	
	///SEQ OCT
	yPos = 210;
	xPos = 148;
	addParam(createParam<moDllzSmSelector>(Vec(xPos,yPos), module, MIDIpoly16::SEQOCT_PARAM));
	///ARPEG OCT
	xPos = 213;
	addParam(createParam<moDllzSmSelector>(Vec(xPos,yPos), module, MIDIpoly16::ARPEGOCT_PARAM));
	
	///// oct lights
	yPos=248;
	xPos=145;
	for (int i = 0 ; i < 5 ; i++){
		addChild(createLight<TinyLight<BlueLight>>(Vec(xPos + (10 * i), yPos), module, MIDIpoly16::SEQOCT_LIGHT + i));
	}
	
	xPos=211;
	for (int i = 0 ; i < 5 ; i++){
	   addChild(createLight<TinyLight<WhiteYLight>>(Vec(xPos + (10 * i), yPos), module, MIDIpoly16::ARPOCT_LIGHT + i));
	}
	yPos = 197;
	xPos = 156;
	addParam(createParam<moDllzSwitchH>(Vec(xPos,yPos), module, MIDIpoly16::SEQOCTALT_PARAM));
	
	xPos = 222;
	addParam(createParam<moDllzSwitchH>(Vec(xPos,yPos), module, MIDIpoly16::ARPEGOCTALT_PARAM));
	
	//ARPEGG ............. RATIO . . . . . . .
	xPos = 215;
	yPos = 127;
	addParam(createParam<RatioKnob>(Vec(xPos,yPos), module, MIDIpoly16::ARPEGRATIO_PARAM));
	xPos = 203.5f;
	yPos = 167.5f;
	addInput(createInput<moDllzPort>(Vec(xPos,yPos),  module, MIDIpoly16::ARPEGRATIO_INPUT));

	
	//// ARP ARC INPUT
	xPos = 217.5f;
	yPos = 288.5f;
	addInput(createInput<moDllzPort>(Vec(xPos, yPos),  module, MIDIpoly16::ARPMODE_INPUT));
	xPos = 240;
	yPos = 259;
	addParam(createParam<moDllzRoundButton>(Vec(xPos, yPos), module, MIDIpoly16::ARCADEON_PARAM));
	addChild(createLight<SmallLight<RedLight>>(Vec(xPos+3.75f, yPos + 3.75f), module, MIDIpoly16::ARCADEON_LIGHT));
	yPos += 18;
	addParam(createParam<moDllzRoundButton>(Vec(xPos, yPos), module, MIDIpoly16::ARPEGON_PARAM));
	addChild(createLight<SmallLight<YellowLight>>(Vec(xPos+3.75f, yPos + 3.75f), module, MIDIpoly16::ARPEGON_LIGHT));
	
	////// LEFT COL
	
	/// Clock IN
	xPos= 9.5f;
	yPos = 58.5f;
	addInput(createInput<moDllzPort>(Vec(xPos, yPos),  module, MIDIpoly16::CLOCK_INPUT));
	xPos = 18;
	yPos = 84;
	addParam(createParam<moDllzSwitchTH>(Vec(xPos, yPos), module, MIDIpoly16:: SEQCLOCKSRC_PARAM));
	addChild(createLight<SmallLight<BlueLight>>(Vec(xPos+31.f, yPos+2.f), module, MIDIpoly16::MIDIBEAT_LIGHT));
	// SEQ START gate
	xPos = 21.5;
	yPos = 118;
	addParam(createParam<moDllzSwitchH>(Vec(xPos,yPos), module, MIDIpoly16::SEQGATERUN_PARAM));
	xPos = 13;
	yPos += 60;
	//Link Run Reset
	addParam(createParam<moDllzSwitchLedH>(Vec(xPos,yPos), module, MIDIpoly16::SEQRUNRESET_PARAM));
	
	///seq Run Reset Inputs
	xPos = 9.5f;
	yPos = 134.5;
	addInput(createInput<moDllzPort>(Vec(xPos, yPos),  module, MIDIpoly16::SEQRUN_INPUT));
	yPos += 60;
	addInput(createInput<moDllzPort>(Vec(xPos, yPos),  module, MIDIpoly16::SEQRESET_INPUT));
	///seq Run Reset BUTTONS
	xPos += 30;
	yPos = 140;
	addParam(createParam<moDllzRoundButton>(Vec(xPos,yPos), module, MIDIpoly16::SEQRUN_PARAM));
	addChild(createLight<SmallLight<BlueLight>>(Vec(xPos+3.75f,yPos+3.75f), module, MIDIpoly16::SEQRUNNING_LIGHT));
	yPos += 60;
	addParam(createParam<moDllzRoundButton>(Vec(xPos,yPos), module, MIDIpoly16::SEQRESET_PARAM));
	addChild(createLight<SmallLight<BlueLight>>(Vec(xPos+3.75f,yPos+3.75f), module, MIDIpoly16::SEQRESET_LIGHT));
	
	xPos = 9.5f;
	// START STOP CLOCK OUTS
	yPos = 241.5f;
	addOutput(createOutput<moDllzPort>(Vec(xPos, yPos),  module, MIDIpoly16::SEQSTART_OUTPUT));
	addOutput(createOutput<moDllzPort>(Vec(xPos+23, yPos),  module, MIDIpoly16::SEQSTOP_OUTPUT));
	yPos += 40;
	addOutput(createOutput<moDllzPort>(Vec(xPos, yPos),  module, MIDIpoly16::SEQSTARTSTOP_OUTPUT));
	addOutput(createOutput<moDllzPort>(Vec(xPos+23, yPos),  module, MIDIpoly16::SEQCLOCK_OUTPUT));
	
	// Transp ...
	yPos = 328;
	xPos = 8.5f;
	addInput(createInput<moDllzPort>(Vec(xPos, yPos-4.5f),  module, MIDIpoly16::SEQSHIFT_INPUT));
	xPos = 33;
	addParam(createParam<TTrimSnap>(Vec(xPos,yPos), module, MIDIpoly16::TRIMSEQSHIFT_PARAM));
	xPos = 87;
	addParam(createParam<moDllzPulseUp>(Vec(xPos,yPos-3), module, MIDIpoly16::SEQTRANUP_PARAM));
	addParam(createParam<moDllzPulseDwn>(Vec(xPos,yPos+8), module, MIDIpoly16::SEQTRANDWN_PARAM));
	
	
	yPos = 327.5;
	// SEQ OUTS
	xPos = 99.5f;
	addOutput(createOutput<moDllzPort>(Vec(xPos, yPos),  module, MIDIpoly16::SEQPITCH_OUTPUT));
	xPos += 24;
	addOutput(createOutput<moDllzPort>(Vec(xPos, yPos),  module, MIDIpoly16::SEQVEL_OUTPUT));
	xPos += 24;
	addOutput(createOutput<moDllzPort>(Vec(xPos, yPos),  module, MIDIpoly16::SEQGATE_OUTPUT));
	xPos += 27.5f;
	addParam(createParam<moDllzSwitchLed>(Vec(xPos,yPos+8.5f), module, MIDIpoly16::SEQRETRIG_PARAM));
	
	//// MIDI OUTS
	xPos = 202;
	addParam(createParam<moDllzSwitchT>(Vec(xPos,yPos-2.5f), module, MIDIpoly16::MONOPITCH_PARAM));
	xPos = 234.5f;
	addOutput(createOutput<moDllzPort>(Vec(xPos,yPos),  module, MIDIpoly16::MONOPITCH_OUTPUT));
	xPos += 24;
	addOutput(createOutput<moDllzPort>(Vec(xPos,yPos),  module, MIDIpoly16::MONOVEL_OUTPUT));
	xPos += 24;
	addOutput(createOutput<moDllzPort>(Vec(xPos,yPos),  module, MIDIpoly16::MONOGATE_OUTPUT));
	xPos += 27.5f;
	addParam(createParam<moDllzSwitchLed>(Vec(xPos,yPos+8.5f), module, MIDIpoly16::MONORETRIG_PARAM));
	xPos += 14.5;
	addOutput(createOutput<moDllzPort>(Vec(xPos,yPos),  module, MIDIpoly16::PBEND_OUTPUT));
	xPos += 24;
	addOutput(createOutput<moDllzPort>(Vec(xPos,yPos),  module, MIDIpoly16::MOD_OUTPUT));
	xPos += 24;
	addOutput(createOutput<moDllzPort>(Vec(xPos,yPos),  module, MIDIpoly16::PRESSURE_OUTPUT));
	xPos += 24;
	addOutput(createOutput<moDllzPort>(Vec(xPos,yPos),  module, MIDIpoly16::SUSTAIN_OUTPUT));
	xPos += 27.5f;
	addParam(createParam<moDllzSwitchLed>(Vec(xPos,yPos+8.5f), module, MIDIpoly16::HOLD_PARAM));

 
	///LOCKED OUTS
	xPos= 451;
	addParam(createParam<moDllzSwitchLed>(Vec(xPos,yPos+8.5f), module, MIDIpoly16::PLAYXLOCKED_PARAM));
	xPos += 18;
	addParam(createParam<moDllzSwitchT>(Vec(xPos,yPos-2.5f), module, MIDIpoly16::LOCKEDPITCH_PARAM));
	xPos = 500.5f;
	addOutput(createOutput<moDllzPort>(Vec(xPos, yPos),  module, MIDIpoly16::LOCKEDPITCH_OUTPUT));
	xPos += 24;
	addOutput(createOutput<moDllzPort>(Vec(xPos, yPos),  module, MIDIpoly16::LOCKEDVEL_OUTPUT));
	xPos += 24;
	addOutput(createOutput<moDllzPort>(Vec(xPos, yPos),  module, MIDIpoly16::LOCKEDGATE_OUTPUT));
	xPos += 27.5f;
	addParam(createParam<moDllzSwitchLed>(Vec(xPos,yPos+10.5f), module, MIDIpoly16::LOCKEDRETRIG_PARAM));

///mute buttons

	yPos=318;
	xPos=102;
	addParam(createParam<moDllzMuteG>(Vec(xPos,yPos), module, MIDIpoly16::MUTESEQ_PARAM));
	addChild(createLight<SmallLight<RedLight>>(Vec(xPos+57.5f, yPos+3), module, MIDIpoly16::MUTESEQ_LIGHT));
	xPos=237;
	addParam(createParam<moDllzMuteG>(Vec(xPos,yPos), module, MIDIpoly16::MUTEMONO_PARAM));
	addChild(createLight<SmallLight<RedLight>>(Vec(xPos+57.5f, yPos+3), module, MIDIpoly16::MUTEMONO_LIGHT));
	xPos=503;
	addParam(createParam<moDllzMuteG>(Vec(xPos,yPos), module, MIDIpoly16::MUTELOCKED_PARAM));
	addChild(createLight<SmallLight<RedLight>>(Vec(xPos+57.5f, yPos+3), module, MIDIpoly16::MUTELOCKED_LIGHT));
	yPos=56;
	xPos=401;
	addParam(createParam<moDllzMuteGP>(Vec(xPos,yPos), module, MIDIpoly16::MUTEPOLYA_PARAM));
	addChild(createLight<TinyLight<RedLight>>(Vec(xPos+20, yPos+4), module, MIDIpoly16::MUTEPOLY_LIGHT));
	xPos=566;
	addParam(createParam<moDllzMuteGP>(Vec(xPos,yPos), module, MIDIpoly16::MUTEPOLYB_PARAM));
	addChild(createLight<TinyLight<RedLight>>(Vec(xPos+20, yPos+4), module, MIDIpoly16::MUTEPOLY_LIGHT));

		{
			digiDisplay *mainDisplay = new digiDisplay();
			mainDisplay->box.pos = Vec(63, 57);
			mainDisplay->box.size = {198, 44};
			mainDisplay->module = module;
			addChild(mainDisplay);
		}
	}
};


Model *modelMIDIpoly16 = createModel<MIDIpoly16, MIDIpoly16Widget>("MIDIpoly16");

