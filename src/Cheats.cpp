#include "Cheats.hpp"

#include "Event.hpp"
#include "Game.hpp"
#include "Modules/Client.hpp"
#include "Modules/Console.hpp"
#include "Modules/Engine.hpp"
#include "Modules/Server.hpp"
#include "Offsets.hpp"

#include <cstring>

Variable sv_laser_cube_autoaim;
Variable ui_loadingscreen_transition_time;
Variable ui_loadingscreen_fadein_time;
Variable ui_loadingscreen_mintransition_time;
Variable ui_transition_effect;
Variable ui_transition_time;
Variable hide_gun_when_holding;
Variable cl_viewmodelfov;
Variable r_flashlightbrightness;

void Cheats::Init() {
	sv_laser_cube_autoaim = Variable("sv_laser_cube_autoaim");
	ui_loadingscreen_transition_time = Variable("ui_loadingscreen_transition_time");
	ui_loadingscreen_fadein_time = Variable("ui_loadingscreen_fadein_time");
	ui_loadingscreen_mintransition_time = Variable("ui_loadingscreen_mintransition_time");
	ui_transition_effect = Variable("ui_transition_effect");
	ui_transition_time = Variable("ui_transition_time");
	hide_gun_when_holding = Variable("hide_gun_when_holding");
	cl_viewmodelfov = Variable("cl_viewmodelfov");
	r_flashlightbrightness = Variable("r_flashlightbrightness");

	Variable::RegisterAll();
	Command::RegisterAll();

}
void Cheats::Shutdown() {
	Variable::UnregisterAll();
	Command::UnregisterAll();
}


// Place for Event registration
ON_EVENT(PROCESS_MOVEMENT) {
	if (!server->AllowsMovementChanges()) return;

}
