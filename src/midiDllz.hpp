/*
midiDllz.hpp Midi Device Display

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
	
	int initpointer_1 = -1;
	int *mdriverJ = &initpointer_1;
	int *mchannelJ = &initpointer_1;
	int mchannelMem = -1;
	
	bool *resetMidi = NULL;
	bool showchannel = true;
	bool isdevice = false;
	bool searchdev = false;
	std::string *mdeviceJ;
	std::string mdriver = "initalizing";
	std::string mdevice = "";
	std::string mchannel = "";
	
	int cursorId = 0;
	float mdfontSize = 12.f;	
	float xcenter = 0.f;
	int drawframe = 0;
	std::shared_ptr<Font> font;
	NVGcolor textColor = nvgRGB(0x88,0x88,0x64);

	void updateMidiSettings(int dRow, bool valup);
	void reDisplay();
	void draw(const DrawArgs &args) override;
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

