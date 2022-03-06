/*
 moDllzComp.cpp components for MIDIpolyMPE
 
 Copyright (C) 2022 Pablo Delaloza.
 
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
//ParamLCD
void paramLCD::drawLayer(const DrawArgs &args, int layer) {
	if (layer != 1) return;
	font = APP->window->loadFont(mFONT_FILE);
	if (!(font && font->handle >= 0)) return;
	nvgFontFaceId(args.vg, font->handle);
	nvgFontSize(args.vg, mdfontSize);
	nvgTextAlign(args.vg, NVG_ALIGN_CENTER);
	if(*cursorIx != buttonId){
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
void paramLCD::onButton(const event::Button &e) {
	if ((e.button != GLFW_MOUSE_BUTTON_LEFT) || !(*canedit) || (e.action != GLFW_PRESS)) return;
	if (*cursorIx != buttonId){
		*cursorIx = buttonId;
		*updateKnob = true;
		flashFocus = 80;
		if(canlearn) *learnId = buttonId;//learnId;
	}else{
		*onFocus = 1;// next goes to 0
	}
}
////MIDIccs ParamLCD
void MIDIccLCD::drawLayer(const DrawArgs &args, int layer) {
	if (layer != 1) return;
	sDisplay = sNames[*param_val];
	paramLCD::drawLayer( args, layer);
}
////mpeZ ParamLCD Button dbl pntr **Val
void PTR_paramLCD::drawLayer(const DrawArgs &args, int layer) {
	if (layer != 1) return;
	sDisplay = sNames[**ptr_param_val];
	paramLCD::drawLayer( args, layer);
}
/// mpeY / Detune  ParamLCD Button
void MPEYdetuneLCD::drawLayer(const DrawArgs &args, int layer) {
	if (layer != 1) return;
	if (**ptr_param_val != 130){//ccs
		sDisplay = sNames[**ptr_param_val];
	}else{
		std::stringstream ss;
		ss << "±";
		ss << std::to_string(*detune_val);
		ss << "ct";
		sDisplay = ss.str();
	}
	paramLCD::drawLayer( args, layer);
}
//// MPE RelVel / PBend ParamLCD Button
void RelVelLCD::drawLayer(const DrawArgs &args, int layer) {
	if (layer != 1) return;
	sDisplay = pNames[**ptr_param_val];
	paramLCD::drawLayer( args, layer);
}
/// + - value  ParamLCD Button
void PlusMinusValLCD::drawLayer(const DrawArgs &args, int layer) {
	if (layer != 1) return;
	std::stringstream ss;
	if(*param_val > 0) ss << "+";
	ss << std::to_string(*param_val);
	sDisplay = ss.str();
	paramLCD::drawLayer( args, layer);
}
///Value Test LCD
void ValueTestLCD::drawLayer(const DrawArgs &args, int layer) {
	if (layer != 1) return;
	font = APP->window->loadFont(mFONT_FILE);
	std::string strout;
	if (!(font && font->handle >= 0)) return;
	if (intVal != nullptr) {
		strout = std::to_string(*intVal);
	}else if (floatVal != nullptr) {
		std::stringstream stream;
		stream << std::fixed << std::setprecision(6) << *floatVal;
		strout = stream.str();
	}
	nvgFontSize(args.vg, mdfontSize);
	nvgFontFaceId(args.vg, font->handle);
	nvgTextAlign(args.vg, NVG_ALIGN_CENTER);
	nvgBeginPath(args.vg);
	nvgRoundedRect(args.vg, 0.f, 0.f, box.size.x, box.size.y,3.f);
	nvgFillColor(args.vg, nvgRGB(0, 0, 0));
	nvgFill(args.vg);
	nvgFillColor(args.vg, nvgRGB(0xff, 0xff, 0));
	nvgTextBox(args.vg, 0.f, 12.f,box.size.x, strout.c_str(), NULL);
}

//	nvgBeginPath(args.vg);
//	nvgRoundedRect(args.vg, 0.f, 0.f, box.size.x, box.size.y,3.f);
//	nvgFillColor(args.vg, nvgRGB(0, 0, 0));
//	nvgFill(args.vg);

//}
///Selected Display
//void SelectedDisplay::drawLayer(const DrawArgs &args, int layer) {
//	if (layer != 1 || *cursorIx < 1) return;
//	
//	font = APP->window->loadFont(mFONT_FILE);
//	if (!(font && font->handle >= 0)) return;
//	nvgFontSize(args.vg, mdfontSize);
//	nvgFontFaceId(args.vg, font->handle);
//	nvgTextAlign(args.vg, NVG_ALIGN_CENTER);
//	nvgBeginPath(args.vg);
//	nvgRoundedRect(args.vg, 0.f, 0.f, box.size.x, box.size.y,3.f);
//	nvgFillColor(args.vg, nvgRGB(0x40, 0, 0));
//	nvgFill(args.vg);
//	nvgFillColor(args.vg, nvgRGB(0xff, 0, 0));
//	nvgTextBox(args.vg, 0.f, 26.f,box.size.x, std::to_string(**testval).c_str(), NULL);
//}


//void DataEntryButton::onButton(const event::Button &e) {
//	if ((e.button != GLFW_MOUSE_BUTTON_LEFT) || (e.action != GLFW_PRESS)) return;
//
//}

///Data Knob LED//
void DataEntryOnLed::drawLayer(const DrawArgs &args, int layer) {
	if ((layer != 1) || (*cursorIx < 1)) return;
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
void DataEntryOnLed::onButton(const event::Button &e) {
	//*cursorIx = 0;
	e.stopPropagating();
	return;
}

//transparentMidiButton::transparentMidiButton(){
//	box.size = {136.f, 40.f};
//}
//void transparentMidiButton::onButton(const event::Button &e) {
//	MidiButton::onButton(e);
//	e.stopPropagating();
//}
//void transparentMidiButton::drawLayer(const DrawArgs &args, int layer) {
//	if (layer != 1) return;
//	font = APP->window->loadFont(mFONT_FILE);
//	if (!((port) && (font && font->handle >= 0))) return;
//	float xcenter = box.size.x / 2;
//	nvgFontSize(args.vg, mdfontSize);
//	nvgFontFaceId(args.vg, font->handle);
//	nvgTextAlign(args.vg, NVG_ALIGN_CENTER);
//	nvgScissor(args.vg, 2.f, 0.f, box.size.x - 4.f, box.size.y);
//
//	std::string mdriver = (port && port->driver) ? port->getDriver()->getName() : "";
//	if (mdriver.empty()) {
//		mdriver = "(No driver)";
//		nvgFillColor(args.vg, unsetColor);
//		nvgText(args.vg, xcenter, 11.5f, mdriver.c_str(), NULL);
//	}else {
//		nvgFillColor(args.vg, okColor);
//		nvgText(args.vg, xcenter, 11.5f, mdriver.c_str(), NULL);
//
//		std::string mdevice = (port && port->device) ? port->getDevice()->getName() : "";
//		if (mdevice.empty()) {
//			mdevice = "(No device)";
//			nvgFillColor(args.vg, unsetColor);
//			nvgText(args.vg, xcenter , 24.5f, mdevice.c_str(), NULL);
//		}else {
//			nvgFillColor(args.vg, okColor);
//			nvgText(args.vg, xcenter , 24.5f, mdevice.c_str(), NULL);
//			///show channel if device set
//			std::string mchannel  = port ? port->getChannelName(port->getChannel()) : "Channel 1";
//			nvgText(args.vg, xcenter, 37.5f, mchannel.c_str(), NULL);
//		}
//	}
//}

///Kn8b LCD
//
//void Kn8bLCD::drawLayer(const DrawArgs &args, int layer) {
//	if (layer != 1) return;
//	font = APP->window->loadFont(mFONT_FILE);
//	if (!(font && font->handle >= 0)) return;
//	
//	std::string strout;
//	strout= std::to_string(id + 1 + *chOffset);
//	nvgFontFaceId(args.vg, font->handle);
//	nvgFontSize(args.vg, mdfontSize - 2.f);
//	nvgTextAlign(args.vg, NVG_ALIGN_LEFT);
//	nvgFillColor(args.vg, nvgRGB(0xee, 0xee, 0xee));
//	nvgTextBox(args.vg, 0.f, 10.f,20.f, strout.c_str(), NULL);
//	nvgFontSize(args.vg, mdfontSize);
//	nvgTextAlign(args.vg, NVG_ALIGN_RIGHT);
//	if (!*active){
//		strout = "ch.off";
//		nvgFillColor(args.vg, nvgRGB(0x44, 0x00, 0x00));
//		nvgTextBox(args.vg, 0.f, 12.f,box.size.x, strout.c_str(), NULL);
//		return;
//	}
//	std::stringstream stream;
//	switch (*displaymode) {
//		case 1: {//input -> Lcd
//			stream << std::fixed << std::setprecision(3) << *outLcd;
//			strout = stream.str() + "v";
//			nvgFillColor(args.vg, nvgRGB(0x44, 0x99, 0xff));
//			nvgTextBox(args.vg, 0.f, 12.f,box.size.x-10.f, strout.c_str(), NULL);
//			stream.str(std::string());
//			stream << std::fixed << std::setprecision(3) << *valLcd;
//			strout = modetxt[*operation * 2 + static_cast<int>(*valLcd < 0.f)] + stream.str();
//			nvgFillColor(args.vg, nvgRGB(0xff, 0xaa, 0x77));
//			nvgTextBox(args.vg, 0.f, 25.f,box.size.x, strout.c_str(), NULL);
//		} break;
//		case 2: {///out
//			stream << std::fixed << std::setprecision(3) << *outLcd;
//			strout = stream.str() + "v";
//			nvgFillColor(args.vg, nvgRGB(0xff, 0xff, 0x00));
//			nvgTextBox(args.vg, 0.f, 18.5f,box.size.x, strout.c_str(), NULL);
//		} break;
//		case 3: {//thru knobs
//			{
//			stream << std::fixed << std::setprecision(3) << *valLcd;
//			strout = modetxt[*operation * 2 + static_cast<int>(*valLcd < 0.f)] + stream.str();
//			nvgFillColor(args.vg, nvgRGB(0xff, 0xaa, 0x77));
//			nvgTextBox(args.vg, 0.f, 12.f,box.size.x- 10.f, strout.c_str(), NULL);
//			stream.str(std::string());
//			stream << std::fixed << std::setprecision(3) << *outLcd;
//			strout = stream.str() + "v";
//			nvgFillColor(args.vg, nvgRGB(0xff, 0xff, 0x00));
//			nvgTextBox(args.vg, 0.f, 25.f,box.size.x, strout.c_str(), NULL);
//			}
//		} break;
//	}
//}
