#include "moDllz.hpp"
MIDIdisplay::MIDIdisplay(){
	font = APP->window->loadFont(mFONT_FILE);
}
///////////////////////////////////////////////////////////////////////////////////////
DispBttnL::DispBttnL(){
	momentary = true;
	addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/DispBttnL.svg")));
}
///////////////////////////////////////////////////////////////////////////////////////
DispBttnR::DispBttnR(){
	momentary = true;
	addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/DispBttnR.svg")));
}
///////////////////////////////////////////////////////////////////////////////////////
void DispBttnL::onButton(const event::Button &e) {
	Widget::onButton(e);
	e.stopPropagating();
	if ((e.button == GLFW_MOUSE_BUTTON_LEFT) && (e.action == GLFW_PRESS)){
		md->updateMidiSettings(id, false);
		if (!e.isConsumed()) e.consume(this);
	}
}
///////////////////////////////////////////////////////////////////////////////////////
void DispBttnR::onButton(const event::Button &e) {
	Widget::onButton(e);
	e.stopPropagating();
	if ((e.button == GLFW_MOUSE_BUTTON_LEFT) && (e.action == GLFW_PRESS)){
		md->updateMidiSettings(id, true);
		if (!e.isConsumed()) e.consume(this);
	}
}
///////////////////////////////////////////////////////////////////////////////////////
void MIDIdisplay::updateMidiSettings (int dRow, bool valup){
	switch (dRow) {
		case 0: {
			bool resetdr = true;
			int drIx = 0;
			int midiDrivers = static_cast<int>(midiInput->getDriverIds().size());
			for (int driverId : midiInput->getDriverIds()) {
				if (driverId == midiInput->driverId){
					if (valup){
						if (drIx < midiDrivers - 1) midiInput->setDriverId(midiInput->getDriverIds().at(drIx + 1));
						else midiInput->setDriverId(midiInput->getDriverIds().front());
					}else {//val down
						if (drIx > 0) midiInput->setDriverId(midiInput->getDriverIds().at(drIx - 1));
						else midiInput->setDriverId(midiInput->getDriverIds().back());
					}
					resetdr = false;
					break;
				}
				drIx ++;
			}
			if (resetdr) midiInput->setDriverId(1);
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
						if (valup){
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
				if (!devhere) midiInput->setDeviceId(midiInput->getDeviceIds().at(0));
				*mdeviceJ = midiInput->getDeviceName(midiInput->deviceId);//valid for saving
				mchannelMem = -1;
				*mchannelJ = -1;
			}
		} break;
		case 2:{
			if (i_mpeMode){
				if (valup){
					if (*mpeChn < 15 ) *mpeChn = *mpeChn + 1;
					else *mpeChn = 0;
				}else{
					if (*mpeChn > 0 ) *mpeChn = *mpeChn - 1;
					else *mpeChn = 15;
				}
			}else{
				if (!showchannel) return;
				if (valup){
					if (mchannelMem < 15 ) mchannelMem ++;
					else mchannelMem = -1;
				}else{
					if (mchannelMem > -1 ) mchannelMem --;
					else mchannelMem = 15;
				}
				//*mpeChn = (mchannelMem > -1)? mchannelMem : 0;
				*mchannelJ = mchannelMem;//valid for saving
			}
		} break;
	}
	searchdev = false;
	*midiActiv = 64;
	reDisplay();
	return;
}
///////////////////////////////////////////////////////////////////////////////////////
void MIDIdisplay::reDisplay(){
	mdriver = midiInput->getDriverName(midiInput->driverId);// driver name
	if (midiInput->deviceId > -1){
		textColor = nvgRGB(0xbb,0xbb,0xbb);
		mdevice = midiInput->getDeviceName(midiInput->deviceId);// device
		showchannel = (mdriver != "Computer keyboard");
		if (i_mpeMode) { //channel MPE
			mchannel = "mpe master " + std::to_string(i_mpeChn + 1);
			midiInput->channel = -1;
		}else { // channel
			if (showchannel){
				midiInput->channel = mchannelMem;
				if (midiInput->channel < 0) mchannel = "ALLch";
				else mchannel =  "ch " + std::to_string(midiInput->channel + 1);
				//showchannel = true;
			} else mchannel = "";
		}
		isdevice = true;
	}else {
		textColor = nvgRGB(0x88,0x88,0x00);
		showchannel = false;
		isdevice = false;
		mdevice = "(no device)";
		mchannel = "";
	}
	drawframe = 0;
}
///////////////////////////////////////////////////////////////////////////////////////
void MIDIdisplay::draw(const DrawArgs &args){
	if (midiInput){
		if (i_mpeMode != *mpeMode) {
			i_mpeMode = *mpeMode;
			reDisplay();
		}
		if ((i_mpeChn != *mpeChn) && (i_mpeMode)){
			i_mpeChn = *mpeChn;
			mchannel = "mpe master " + std::to_string(i_mpeChn + 1);
		}
		if (drawframe++ > 100){
			drawframe = 0;
			if (isdevice && (midiInput->getDeviceName(midiInput->deviceId) != *mdeviceJ)) searchdev = true;
			if (searchdev) {
				showchannel = false;
				textColor = nvgRGB(0xFF,0x64,0x64);
				midiInput->setDriverId(*mdriverJ);
				mdriver = midiInput->getDriverName(midiInput->driverId);
				if (*mdeviceJ != ""){
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
				}else{
					if (midiInput->getDeviceIds().size() > 0){
						midiInput->setDeviceId(0);
						*mdeviceJ = midiInput->getDeviceName(0);
					}else midiInput->setDeviceId(-1);
					//mchannelMem = -1;
					//*mchannelJ = -1;
				}
			}
		}
		if (*midiActiv > 3) *midiActiv -= 4;
		else *midiActiv = 0;//clip to 0
		//nvgGlobalCompositeBlendFunc(args.vg,  NVG_ONE , NVG_ONE);
		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, 0.f, 0.f, box.size.x, box.size.y, 4.f);
		nvgFillColor(args.vg, nvgRGB(static_cast<unsigned char>(*midiActiv * .5),static_cast<unsigned char>(*midiActiv * .5) ,0));
		nvgFill(args.vg);
		nvgFontSize(args.vg, mdfontSize);
		nvgFontFaceId(args.vg, font->handle);
		nvgTextAlign(args.vg, NVG_ALIGN_CENTER);
		nvgScissor(args.vg, 12.f, 0.f, box.size.x - 24.f, box.size.y);
		nvgFillColor(args.vg, textColor);
		nvgText(args.vg, xcenter, 11.5f, mdriver.c_str(), NULL);
		nvgText(args.vg, xcenter, 24.5f, mdevice.c_str(), NULL);
		nvgText(args.vg, xcenter, 37.5f, mchannel.c_str(), NULL);
	}
}
///////////////////////////////////////////////////////////////////////////////////////
void MIDIdisplay::onButton(const event::Button &e) {
	e.stopPropagating();
	if ((e.button == GLFW_MOUSE_BUTTON_LEFT) && (e.action == GLFW_PRESS)){
		*resetMidi = true;
		if (!e.isConsumed()) e.consume(this);
	}
}
///////////////////////////////////////////////////////////////////////////////////////
void MIDIscreen::setMidiPort(midi::Port *port,bool *mpeMode,int *mpeChn,int *midiActiv, int *mdriver, std::string *mdevice, int *mchannel, bool *resetMidi){
	clearChildren();
	
	MIDIdisplay *md = createWidget<MIDIdisplay>(Vec(0.f,0.f));
	md->midiInput = port;
	md->mpeMode = mpeMode;
	md->mpeChn = mpeChn;
	md->midiActiv = midiActiv;
	md->box.size = box.size;
	md->xcenter = md->box.size.x / 2.f;
	md->mdriverJ = mdriver;
	md->mdeviceJ = mdevice;
	md->mchannelJ = mchannel;
	md->resetMidi = resetMidi;
	md->searchdev = true;
	addChild(md);
	
	DispBttnL *drvBttnL = createWidget<DispBttnL>(Vec(1.f,0.f));
	drvBttnL->md = md;
	addChild(drvBttnL);
	
	DispBttnL *devBttnL = createWidget<DispBttnL>(Vec(1.f,13.f));
	devBttnL->id = 1;
	devBttnL->md = md;
	addChild(devBttnL);
	
	DispBttnL *chnBttnL = createWidget<DispBttnL>(Vec(1.f,26.f));
	chnBttnL->id = 2;
	chnBttnL->md = md;
	addChild(chnBttnL);
	
	DispBttnR *drvBttnR = createWidget<DispBttnR>(Vec(box.size.x - 11.f,0.f));
	drvBttnR->md = md;
	addChild(drvBttnR);

	DispBttnR *devBttnR = createWidget<DispBttnR>(Vec(box.size.x - 11.f,13.f));
	devBttnR->id = 1;
	devBttnR->md = md;
	addChild(devBttnR);

	DispBttnR *chnBttnR = createWidget<DispBttnR>(Vec(box.size.x - 11.f,26.f));
	chnBttnR->id = 2;
	chnBttnR->md = md;
	addChild(chnBttnR);
	
	
}
///////////////////////////////////////////////////////////////////////////////////////
MIDIscreen::MIDIscreen(){
}
