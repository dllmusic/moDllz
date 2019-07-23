#pragma once
#include "rack.hpp"

//#include <math.h>

using namespace rack;


struct MIDIdisplay : OpaqueWidget {
	MIDIdisplay();
	//widget::FramebufferWidget *fb;
	//widget::OpaqueWidget *ow;
	midi::Port *midiInput = NULL;
	//int *midiUpdate = 0;
	//int *cursorIx = NULL;
	//int midiInCH = 0;
	int i_mpeOff = 1;
	int i_mpeChn = 1;
	int initpointer = 1;
	int initpointer0 = 1;
	int *mpeOff = &initpointer;
	int *mpeChn = &initpointer;
	int *flashDisplay = &initpointer0;
	float mdfontSize = 12.f;
	int cursorId = 0;
	float cursorfy = 0.f;
	bool reDisplay = false;
	float xcenter = 0.f;
	std::string mdriver = "";
	std::string mdevice = "";
	std::string mchannel = "";
	bool bchannel = true;
	
	std::shared_ptr<Font> font;
	NVGcolor textColor = nvgRGB(0x80,0x80,0x80);

	void draw(const DrawArgs &args) override;
	void updateMidiSettings(int dRow, bool valup);
	//void setMidiPort(midi::Port *port);
	// void onButton(const event::Button &e) override;
};

struct DispBttnL : SvgSwitch {
	DispBttnL();
	MIDIdisplay *md = NULL;
	int id = 0;
	void onButton(const event::Button &e) override;
};
struct DispBttnR : SvgSwitch {
	DispBttnR();
	MIDIdisplay *md = NULL;
	int id = 0;
	void onButton(const event::Button &e) override;
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
	void setMidiPort(midi::Port *port,int *mpeOff,int *mpeChn,int *midiActivity);
	//void setMidiVars(int *mpeOff,int *mpeChn,int *midiActivity);
};
