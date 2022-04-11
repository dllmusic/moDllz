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

SvgBuffered::SvgBuffered() {
	fb = new widget::FramebufferWidget;
	addChild(fb);
	sw = new widget::SvgWidget;
	fb->addChild(sw);
}

void SvgBuffered::setSvg(std::shared_ptr<window::Svg> svg) {
	sw->setSvg(svg);
	fb->box.size = sw->box.size;
	box.size = sw->box.size;
}
