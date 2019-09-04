using namespace rack;

struct MIDIdisplay : OpaqueWidget {
	MIDIdisplay();
	midi::Port *midiInput = NULL;
	int i_mpeChn = 0;
	int *mpeChn = &i_mpeChn;
	bool *mpeMode = NULL;
	bool i_mpeMode = false;
	int initpointer0 = 0;
	int *midiActiv = &initpointer0;
	bool *resetMidi = NULL;
	int *mdriverJ;
	int *mchannelJ;
	bool showchannel = true;
	bool isdevice = false;
	std::string *mdeviceJ;
	std::string mdriver = "";
	std::string mdevice = "";
	std::string mchannel = "";
	bool searchdev = false;
	
	int cursorId = 0;
	float mdfontSize = 12.f;	
	float xcenter = 0.f;
	std::shared_ptr<Font> font;
	NVGcolor textColor = nvgRGB(0x80,0x80,0x80);

	void draw(const DrawArgs &args) override;
	void updateMidiSettings(int dRow, bool valup);
	void reDisplay();
	void onButton(const event::Button &e) override;
};

struct DispBttnL : SvgSwitch {
	DispBttnL();
	MIDIdisplay *md = NULL;
	int id = 0;
	void onButton(const event::Button &e) override;
	void randomize() override{
	}
};
struct DispBttnR : SvgSwitch {
	DispBttnR();
	MIDIdisplay *md = NULL;
	int id = 0;
	void onButton(const event::Button &e) override;
	void randomize() override{
	}
};

struct MIDIscreen : OpaqueWidget{
	MIDIscreen();
	DispBttnL *drvL;
	DispBttnL *devL;
	DispBttnL *chnL;
	DispBttnR *drvR;
	DispBttnR *devR;
	DispBttnR *chnR;
	MIDIdisplay *md;
	void setMidiPort(midi::Port *port,bool *mpeMode,int *mpeChn,int *midiActiv, int *mdriver, std::string *mdevice, int *mchannel, bool *resetMidi);
};
