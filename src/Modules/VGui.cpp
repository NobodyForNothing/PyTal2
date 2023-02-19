#include "VGui.hpp"

#include "Modules/Engine.hpp"
#include "Modules/Server.hpp"
#include "Modules/Surface.hpp"
#include "PytalMain.hpp"

#include <algorithm>

REDECL(VGui::Paint);

// CEngineVGui::Paint
DETOUR(VGui::Paint, PaintMode_t mode) {
	if ((mode & PAINT_UIPANELS) && GET_SLOT() == 0) {
		surface->StartDrawing(surface->matsurface->ThisPtr());
		surface->FinishDrawing();
	}

	auto result = VGui::Paint(thisptr, mode);

	surface->StartDrawing(surface->matsurface->ThisPtr());

	surface->FinishDrawing();

	return result;
}


bool VGui::IsUIVisible() {
	return this->IsGameUIVisible(this->enginevgui->ThisPtr());
}

bool VGui::Init() {
	this->enginevgui = Interface::Create(this->Name(), "VEngineVGui001");
	if (this->enginevgui) {
		this->IsGameUIVisible = this->enginevgui->Original<_IsGameUIVisible>(Offsets::IsGameUIVisible);

		this->enginevgui->Hook(VGui::Paint_Hook, VGui::Paint, Offsets::Paint);
	}

	return this->hasLoaded = this->enginevgui;
}
void VGui::Shutdown() {
	Interface::Delete(this->enginevgui);
	this->huds.clear();
}

VGui *vgui;
