/*
 midiDllz.cpp Midi Device Display
 
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
#include "moDllz.hpp"
namespace rack {
MIDIdisplay::MIDIdisplay(){}
///////////////////////////////////////////////////////////////////////////////////////
DispBttnL::DispBttnL(){
	momentary = true;
	addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/midi/DispBttnL.svg")));
}
///////////////////////////////////////////////////////////////////////////////////////
DispBttnR::DispBttnR(){
	momentary = true;
	addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/midi/DispBttnR.svg")));
}
///////////////////////////////////////////////////////////////////////////////////////
void DispBttnL::onButton(const ButtonEvent &e) {
	Widget::onButton(e);
	e.stopPropagating();
	if ((e.button == GLFW_MOUSE_BUTTON_LEFT) && (e.action == GLFW_PRESS)){
		md->updateMidiSettings(id, -1);
		if (!e.isConsumed()) e.consume(this);
	}
}
///////////////////////////////////////////////////////////////////////////////////////
void DispBttnR::onButton(const ButtonEvent &e) {
	Widget::onButton(e);
	e.stopPropagating();
	if ((e.button == GLFW_MOUSE_BUTTON_LEFT) && (e.action == GLFW_PRESS)){
		md->updateMidiSettings(id, 1);
		if (!e.isConsumed()) e.consume(this);
	}
}
///////////////////////////////////////////////////////////////////////////////////////
void MIDIdisplay::updateMidiSettings (int dRow, int incr){
	*resetMidi = true;
	switch (dRow) {
		case 0: {
			bool resetdr = true;
			int drIx = 0;
			int midiDrivers = static_cast<int>(midi::getDriverIds().size());
			for (int driverId : midi::getDriverIds()) {
				if (driverId == midiInput->driverId){
					if (incr > 0){
						if (drIx < midiDrivers - 1) midiInput->setDriverId(midi::getDriverIds().at(drIx + 1));
						else midiInput->setDriverId(midi::getDriverIds().front());
					}else {//val down
						if (drIx > 0) midiInput->setDriverId(midi::getDriverIds().at(drIx - 1));
						else midiInput->setDriverId(midi::getDriverIds().back());
					}
					resetdr = false;
					break;
				}
				drIx ++;
			}
			if (resetdr) midiInput->setDriverId(midi::getDriverIds().front());
			if (midiInput->getDeviceIds().size() > 0){
				midiInput->setDeviceId(0);
				*mdriverJ = midiInput->driverId;//valid for saving
				*mdeviceJ = midiInput->getDeviceName(0);
			}else midiInput->setDeviceId(-1);
			mchannelMem = -1;
			*mchannelJ = -1;
		} break;
		case 1: {
			int deIx = 0;
			bool devhere = false;
			int midiDevs = static_cast<int>(midiInput->getDeviceIds().size());
			if (midiDevs > 0){
				for (int deviceId : midiInput->getDeviceIds()) {
					if (deviceId == midiInput->deviceId){
						if (incr > 0){
							if (deIx < midiDevs - 1) midiInput->setDeviceId(midiInput->getDeviceIds().at(deIx + 1));
							else midiInput->setDeviceId(midiInput->getDeviceIds().front());
						}else {//val down
							if (deIx > 0) midiInput->setDeviceId(midiInput->getDeviceIds().at(deIx - 1));
							else midiInput->setDeviceId(midiInput->getDeviceIds().back());
						}
						devhere = true;
						break;
					}
					deIx ++;
				}
				if (!devhere) midiInput->setDeviceId(midiInput->getDeviceIds().front());
				*mdeviceJ = midiInput->getDeviceName(midiInput->deviceId);//valid for saving
				mchannelMem = -1;
				*mchannelJ = -1;
			}
		} break;
		case 2:{
			if (!showchannel) break;
			if (i_mpeMode){
				*mpeChn = (*mpeChn + 16 + incr) % 16;
			}else{
				mchannelMem = ((mchannelMem + 18 + incr) % 17) - 1;
				*mchannelJ = mchannelMem;//valid for saving
			}
		} break;
	}
	searchdev = false;
	reDisplay();
	return;
}
///////////////////////////////////////////////////////////////////////////////////////
void MIDIdisplay::reDisplay(){
	mdriver = midi::getDriver(midiInput->driverId)->getName();
	// driver name
	if (midiInput->deviceId > -1){
		textColor = nvgRGB(0xcc,0xcc,0xcc);
		mdevice = midiInput->getDeviceName(midiInput->deviceId);// device
		showchannel = (mdriver.substr(0,17) != "Computer keyboard");
		if (showchannel){
			if (i_mpeMode) { //channel MPE
				mchannel = "mpe master " + std::to_string(*mpeChn + 1);
				midiInput->channel = -1;
			}else { // channel
				midiInput->channel = mchannelMem;
				if (midiInput->channel < 0) mchannel = "ALLch";
				else mchannel =  "ch " + std::to_string(midiInput->channel + 1);
			}
		} else mchannel = "";
		isdevice = true;
	}else {
		textColor = nvgRGB(0xaa,0xaa,0x00);
		showchannel = false;
		mdevice = "(no device)";
		mchannel = "";
		isdevice = false;
	}
	drawframe = 0;
}
///////////////////////////////////////////////////////////////////////////////////////
void MIDIdisplay::drawLayer(const DrawArgs& args, int layer){
	if (layer != 1) return ;// || *cursorIx > 0) return;
	font = APP->window->loadFont(mFONT_FILE);
	if (!((font && font->handle >= 0) || midiInput)) return;
		if (i_mpeMode != *mpeMode) {
			i_mpeMode = *mpeMode;
			reDisplay();
		}
		if (drawframe++ > 50){
			drawframe = 0;
			if (isdevice && (midiInput->getDeviceName(midiInput->deviceId) != *mdeviceJ)) searchdev = true;
			if (searchdev) {
				showchannel = false;
				if (!(mdeviceJ->empty())){/// if previously saved
					textColor = nvgRGB(0xFF,0x64,0x64);
					midiInput->setDriverId(*mdriverJ);
					mdevice = *mdeviceJ;
					mchannel = "...disconnected...";
					for (int deviceId : midiInput->getDeviceIds()) {
						if (midiInput->getDeviceName(deviceId) == *mdeviceJ) {
							midiInput->setDeviceId(deviceId);
							mchannelMem = *mchannelJ;
							searchdev = false;
							isdevice = true;
							showchannel = true;
							reDisplay();
							break;
						}
					}
				}else{ //initial searchdev / (no saved devices)
					midiInput->setDriverId(midi::getDriverIds().front());
					if (midiInput->getDeviceIds().size() > 0){
						midiInput->setDeviceId(midiInput->getDeviceIds().front());
						*mdriverJ = midiInput->driverId;//valid for saving
						*mdeviceJ = midiInput->getDeviceName(midiInput->deviceId);
					}else midiInput->setDeviceId(-1);
					*mchannelJ = -1;
					mchannelMem = -1;
					reDisplay();
				}
			}
		}
		if (*midiActiv > 7) {
			nvgBeginPath(args.vg);
			nvgRoundedRect(args.vg, 0.f, 0.f, box.size.x, box.size.y, 3.f);
			nvgFillColor(args.vg, nvgRGBA(0x44,0x44 , 0x44, *midiActiv));
			nvgFill(args.vg);
			*midiActiv -= 8;
		}
		else *midiActiv = 0;//clip to 0
		
		nvgFontSize(args.vg, mdfontSize);
		nvgFontFaceId(args.vg, font->handle);
		nvgTextAlign(args.vg, NVG_ALIGN_CENTER);
		nvgScissor(args.vg, 2.f, 0.f, box.size.x - 4.f, box.size.y);
		nvgFillColor(args.vg, textColor);
		nvgText(args.vg, xcenter, 11.5f, mdriver.c_str(), NULL);
		nvgText(args.vg, xcenter, 24.5f, mdevice.c_str(), NULL);
		nvgText(args.vg, xcenter, 37.5f, mchannel.c_str(), NULL);
}
///////////////////////////////////////////////////////////////////////////////////////
void MIDIdisplay::onButton(const ButtonEvent &e) {
	e.stopPropagating();
	if ((e.button == GLFW_MOUSE_BUTTON_LEFT) && (e.action == GLFW_PRESS)){
		//resetMidi();
		*resetMidi = true;
		if (!e.isConsumed()) e.consume(this);
	}
}
///////////////////////////////////////////////////////////////////////////////////////
void MIDIscreen::setMidiPort(midi::Port *port,bool *mpeMode,int *mpeChn,unsigned char *midiActiv, int *mdriver, std::string *mdevice, int *mchannel, bool *resetMidi){
	//}, int *cursorIx){
	clearChildren();
	MIDIdisplay *md = createWidget<MIDIdisplay>(Vec(10.f,0.f));
	md->midiInput = port;
	md->mpeMode = mpeMode;
	md->mpeChn = mpeChn;
	md->midiActiv = midiActiv;
	md->box.size = Vec(box.size.x - 20.f, box.size.y);
	md->xcenter = md->box.size.x / 2.f;
	md->mdriverJ = mdriver;
	md->mdeviceJ = mdevice;
	md->mchannelJ = mchannel;
	md->resetMidi = resetMidi;
	md->searchdev = true;
	//md->cursorIx = cursorIx;
	addChild(md);
	DispBttnL *drvBttnL = createWidget<DispBttnL>(Vec(1.f,1.f));
	drvBttnL->md = md;
	addChild(drvBttnL);
	DispBttnL *devBttnL = createWidget<DispBttnL>(Vec(1.f,14.f));
	devBttnL->id = 1;
	devBttnL->md = md;
	addChild(devBttnL);
	DispBttnL *chnBttnL = createWidget<DispBttnL>(Vec(1.f,27.f));
	chnBttnL->id = 2;
	chnBttnL->md = md;
	addChild(chnBttnL);
	DispBttnR *drvBttnR = createWidget<DispBttnR>(Vec(box.size.x - 11.f,1.f));
	drvBttnR->md = md;
	addChild(drvBttnR);
	DispBttnR *devBttnR = createWidget<DispBttnR>(Vec(box.size.x - 11.f,14.f));
	devBttnR->id = 1;
	devBttnR->md = md;
	addChild(devBttnR);
	DispBttnR *chnBttnR = createWidget<DispBttnR>(Vec(box.size.x - 11.f,27.f));
	chnBttnR->id = 2;
	chnBttnR->md = md;
	addChild(chnBttnR);
}
///////////////////////////////////////////////////////////////////////////////////////
MIDIscreen::MIDIscreen(){}
} // namespace rack
