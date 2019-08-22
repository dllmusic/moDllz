#include "moDllz.hpp"
MIDIdisplay::MIDIdisplay(){
	font = APP->window->loadFont(mFONT_FILE);
}

DispBttnL::DispBttnL(){
	momentary = true;
	addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/DispBttnL.svg")));
}

void DispBttnL::onButton(const event::Button &e) {
	Widget::onButton(e);
	e.stopPropagating();
	if ((e.button == GLFW_MOUSE_BUTTON_LEFT) && (e.action == GLFW_PRESS)){
		md->updateMidiSettings(id, false);
		if (!e.isConsumed())
			e.consume(this);
	}
}

void DispBttnR::onButton(const event::Button &e) {
	Widget::onButton(e);
	e.stopPropagating();
	if ((e.button == GLFW_MOUSE_BUTTON_LEFT) && (e.action == GLFW_PRESS)){
		md->updateMidiSettings(id, true);
		if (!e.isConsumed())
			e.consume(this);
	}
}

DispBttnR::DispBttnR(){
	momentary = true;
	addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/DispBttnR.svg")));
}

void MIDIdisplay::updateMidiSettings (int dRow, bool valup){
	switch (dRow) {
		case 0: {
			bool resetdr = true;
			int drIx = 0;
			int midiDrivers = static_cast<int>(midiInput->getDriverIds().size());
			for (int driverId : midiInput->getDriverIds()) {
				if (driverId == midiInput->driverId){
					if (valup){
						if (drIx < midiDrivers - 1){
							midiInput->setDriverId(midiInput->getDriverIds().at(drIx + 1));
						}else{
							midiInput->setDriverId(midiInput->getDriverIds().front());
						}
					}else{//val down
						if (drIx > 0){
							midiInput->setDriverId(midiInput->getDriverIds().at(drIx - 1));
						}else{
							midiInput->setDriverId(midiInput->getDriverIds().back());
						}
					}
					resetdr = false;
					break;
				}
				drIx ++;
			}
			if (resetdr) midiInput->setDriverId(1);
			midiInput->setDeviceId((midiInput->getDeviceIds().size() > 0)? 0 : -1);
			midiInput->channel = -1;
		}
			break;
		case 1: {
			int deIx = 0;
			int midiDevs = static_cast<int>(midiInput->getDeviceIds().size());
			for (int deviceId : midiInput->getDeviceIds()) {
				if (deviceId == midiInput->deviceId){
					if (valup){
						if (deIx < midiDevs - 1){
							midiInput->setDeviceId(midiInput->getDeviceIds().at(deIx + 1));
						}else{
							midiInput->setDeviceId(midiInput->getDeviceIds().front());
						}
					}else{//val down
						if (deIx > 0){
							midiInput->setDeviceId(midiInput->getDeviceIds().at(deIx - 1));
						}else{
							midiInput->setDeviceId(midiInput->getDeviceIds().back());
						}
					}
					break;
				}
				deIx ++;
			}
		}
			break;
		case 2:{
			if (bchannel){
					if (valup){
						if (midiInput->channel < 15 ) midiInput->channel ++;
						else midiInput->channel = -1;
					}else{
						if (midiInput->channel > -1 ) midiInput->channel --;
						else midiInput->channel = 15;
					}
			}else{
				if (valup){
					if (*mpeChn < 15 ) *mpeChn = *mpeChn + 1;
					else *mpeChn = 0;
				}else{
					if (*mpeChn > 0 ) *mpeChn = *mpeChn - 1;
					else *mpeChn = 15;
				}
			}
			break;
		}
	}
	*midiActiv = 64;
	reDisplay = true;
	return;
}

void MIDIdisplay::draw(const DrawArgs &args){
	if (midiInput){
		if (i_mpeOff != *mpeOff) {
			reDisplay = true;
			i_mpeOff = *mpeOff;
		}
		if (i_mpeChn != *mpeChn) {
			reDisplay = true;
			i_mpeChn = *mpeChn;
		}
		if (reDisplay) {
			mdriver = midiInput->getDriverName(midiInput->driverId);
			if (midiInput->deviceId > -1){
				textColor = nvgRGB(0x90,0x90,0x90);
				mdevice = midiInput->getDeviceName(midiInput->deviceId);
				if (i_mpeOff > 1) {
					if (midiInput->channel < 0) mchannel = "ALLch";
					else mchannel =  "ch " + std::to_string(midiInput->channel + 1);
					bchannel = true;
				}else{
					bchannel = false;
					mchannel = "mpe master " + std::to_string(i_mpeChn + 1);
					midiInput->channel = -1;
				}
			}else {
				textColor = nvgRGB(0xFF,0x64,0x64);
				if (*mdriverJ == midiInput->driverId){
					mdevice = *mdeviceJ;
					if (*mchannelJ < 0) mchannel = "ALLch";
					else mchannel =  "ch " + std::to_string(*mchannelJ + 1);
				}else{
					mdevice = "(no device)";
					mchannel = "";
				}
				bchannel = false;
			}
			if (mdriver.compare("Computer keyboard") == 0) {
				bchannel = false;
				mchannel = "";
			}
			reDisplay = false;
		}
		if (*midiActiv > 0) *midiActiv -= 4;
			//nvgGlobalCompositeBlendFunc(args.vg,  NVG_ONE , NVG_ONE);
			nvgBeginPath(args.vg);
			nvgRoundedRect(args.vg, 0.f, 0.f, box.size.x, box.size.y, 4.f);
			nvgFillColor(args.vg, nvgRGB(*midiActiv,*midiActiv/2,0));
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
void MIDIdisplay::onButton(const event::Button &e) {
	Widget::onButton(e);
	e.stopPropagating();
	if ((e.button == GLFW_MOUSE_BUTTON_LEFT) && (e.action == GLFW_PRESS)){
		if (midiInput->deviceId < 0) {// disconnected device
			for (int deviceId : midiInput->getDeviceIds()) {
				if (midiInput->getDeviceName(deviceId) == *mdeviceJ) {
					midiInput->setDeviceId(deviceId);
					midiInput->channel = *mchannelJ;
					break;
				}
			}
		}
		*midiActiv = -1;
		reDisplay = true;
		if (!e.isConsumed())
			e.consume(this);
	}
}

void MIDIscreen::setMidiPort(midi::Port *port,int *mpeOff,int *mpeChn,int *midiActiv, int *mdriver, std::string *mdevice, int *mchannel){
	clearChildren();
	//math::Vec pos;
	MIDIdisplay *md = createWidget<MIDIdisplay>(Vec(0.f,0.f));
	md->mpeOff = mpeOff;
	md->mpeChn = mpeChn;
	md->midiActiv = midiActiv;
	md->midiInput = port;
	md->box.size = box.size;
	md->reDisplay = true;
	md->xcenter = md->box.size.x / 2.f;
	md->mdriverJ = mdriver;
	md->mdeviceJ = mdevice;
	md->mchannelJ = mchannel;	
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

MIDIscreen::MIDIscreen(){
}
