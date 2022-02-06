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
#pragma once
#include <app/common.hpp>
#include <midi.hpp>
namespace rack {


struct MIDIdisplay : OpaqueWidget {
	MIDIdisplay();
	midi::Port *midiInput;
	int initpointer_1 = -1;
	
	int *mpeChn = &initpointer_1;
	bool i_bool = false;
	bool i_mpeMode = false;
	bool *mpeMode = &i_bool;
	unsigned char  initpointer0 = 0;
	unsigned char *midiActiv = &initpointer0;
	
	int *mdriverJ = &initpointer_1;
	int *mchannelJ = &initpointer_1;
	int mchannelMem = -1;

	bool *resetMidi = &i_bool;
	
	bool showchannel = true;
	bool isdevice = false;
	bool searchdev = false;
	std::string *mdeviceJ;
	std::string mdriver = "initalizing";
	std::string mdevice = "";
	std::string mchannel = "";
	
	int cursorId = 0;
	float mdfontSize = 13.f;
	float xcenter = 0.f;
	char drawframe = 0;
	std::shared_ptr<Font> font;
	NVGcolor textColor = nvgRGB(0x88,0x88,0x88);
	
	void updateMidiSettings(int dRow, bool valup);
	void reDisplay();
	//void draw(const DrawArgs &args) override;
	void drawLayer(const DrawArgs &args, int layer) override;
	void onButton(const event::Button &e) override;
};

struct DispBttnL : SvgSwitch {
	DispBttnL();
	MIDIdisplay *md = nullptr;
	int id = 0;
	void onButton(const event::Button &e) override;
};
struct DispBttnR : SvgSwitch {
	DispBttnR();
	MIDIdisplay *md = nullptr;
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
	void setMidiPort(midi::Port *port,bool *mpeMode,int *mpeChn,unsigned char *midiActiv, int *mdriver, std::string *mdevice, int *mchannel,
					 // void (*resetMidi)());
					 bool *resetMidi);
};

} // namespace rack
