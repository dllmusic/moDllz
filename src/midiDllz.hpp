//#pragma once
//#include "rack.hpp"
using namespace rack;

struct MIDIdisplay : OpaqueWidget {
	MIDIdisplay();
	midi::Port *midiInput = NULL;
	int i_mpeOff = 1;
	int i_mpeChn = 1;
	int *mpeOff = &i_mpeOff;
	int *mpeChn = &i_mpeChn;
	int initpointer0 = 1;
	int *midiActiv = &initpointer0;
	float mdfontSize = 12.f;
	int cursorId = 0;
	bool reDisplay = false;
	float xcenter = 0.f;
	std::string mdriver = "";
	std::string mdevice = "";
	std::string mchannel = "";
	int *mdriverJ;
	std::string *mdeviceJ;
	int *mchannelJ;
	bool bchannel = true;
	std::shared_ptr<Font> font;
	NVGcolor textColor = nvgRGB(0x80,0x80,0x80);

	void draw(const DrawArgs &args) override;
	void updateMidiSettings(int dRow, bool valup);
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
	void setMidiPort(midi::Port *port,int *mpeOff,int *mpeChn,int *midiActiv, int *mdriver, std::string *mdevice, int *mchannel);
};
