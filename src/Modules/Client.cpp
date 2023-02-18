#include "Client.hpp"

#include "Command.hpp"
#include "Console.hpp"
#include "Engine.hpp"
#include "Event.hpp"
#include "Game.hpp"
#include "Hook.hpp"
#include "Interface.hpp"
#include "Offsets.hpp"
#include "Server.hpp"
#include "Utils.hpp"

#include <cstdarg>
#include <cstdint>
#include <cstring>
#include <deque>

Variable cl_showpos;
Variable cl_sidespeed;
Variable cl_backspeed;
Variable cl_forwardspeed;
Variable in_forceuser;
Variable crosshairVariable;
Variable cl_fov;
Variable prevent_crouch_jump;
Variable r_portaltestents;
Variable r_portalsopenall;
Variable r_drawviewmodel;


REDECL(Client::LevelInitPreEntity);
REDECL(Client::CreateMove);
REDECL(Client::CreateMove2);
REDECL(Client::GetName);
REDECL(Client::ShouldDraw_BasicInfo);
REDECL(Client::ShouldDraw_SaveStatus);
REDECL(Client::MsgFunc_SayText2);
REDECL(Client::GetTextColorForClient);
REDECL(Client::DecodeUserCmdFromBuffer);
REDECL(Client::CInput_CreateMove);
REDECL(Client::GetButtonBits);
REDECL(Client::SteamControllerMove);
REDECL(Client::playvideo_end_level_transition_callback);
REDECL(Client::OverrideView);
REDECL(Client::ProcessMovement);
REDECL(Client::DrawTranslucentRenderables);
REDECL(Client::DrawOpaqueRenderables);
REDECL(Client::CalcViewModelLag);

CMDECL(Client::GetAbsOrigin, Vector, m_vecAbsOrigin);
CMDECL(Client::GetAbsAngles, QAngle, m_angAbsRotation);
CMDECL(Client::GetLocalVelocity, Vector, m_vecVelocity);
CMDECL(Client::GetViewOffset, Vector, m_vecViewOffset);
CMDECL(Client::GetPortalLocal, CPortalPlayerLocalData, m_PortalLocal);
CMDECL(Client::GetPlayerState, CPlayerState, pl);


ClientEnt *Client::GetPlayer(int index) {
	return this->GetClientEntity(this->s_EntityList->ThisPtr(), index);
}
void Client::CalcButtonBits(int nSlot, int &bits, int in_button, int in_ignore, kbutton_t *button, bool reset) {
	auto pButtonState = &button->m_PerUser[nSlot];
	if (pButtonState->state & 3) {
		bits |= in_button;
	}

	int clearmask = ~2;
	if (in_ignore & in_button) {
		clearmask = ~3;
	}

	if (reset) {
		pButtonState->state &= clearmask;
	}
}
Client *client;
