#include "Server.hpp"

#include "Client.hpp"
#include "Engine.hpp"
#include "Event.hpp"
#include "Features/EntityList.hpp"
#include "Features/FCPS.hpp"
#include "Game.hpp"
#include "Hook.hpp"
#include "Interface.hpp"
#include "Offsets.hpp"
#include "Utils.hpp"
#include "Variable.hpp"

#include "Features/OverlayRender.hpp"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <cfloat>

#define RESET_COOP_PROGRESS_MESSAGE_TYPE "coop-reset"
#define CM_FLAGS_MESSAGE_TYPE "cm-flags"

Variable sv_cheats;
Variable sv_footsteps;
Variable sv_alternateticks;
Variable sv_bonus_challenge;
Variable sv_accelerate;
Variable sv_airaccelerate;
Variable sv_paintairacceleration;
Variable sv_friction;
Variable sv_maxspeed;
Variable sv_stopspeed;
Variable sv_maxvelocity;
Variable sv_gravity;

REDECL(Server::CheckJumpButton);
REDECL(Server::CheckJumpButtonBase);
REDECL(Server::PlayerMove);
REDECL(Server::FinishGravity);
REDECL(Server::AirMove);
REDECL(Server::AirMoveBase);
REDECL(Server::GameFrame);
REDECL(Server::OnRemoveEntity);
REDECL(Server::PlayerRunCommand);
REDECL(Server::ViewPunch);
REDECL(Server::IsInPVS);
REDECL(Server::ProcessMovement);
REDECL(Server::GetPlayerViewOffset);
REDECL(Server::StartTouchChallengeNode);
REDECL(Server::say_callback);

SMDECL(Server::GetPortals, int, iNumPortalsPlaced);
SMDECL(Server::GetAbsOrigin, Vector, m_vecAbsOrigin);
SMDECL(Server::GetAbsAngles, QAngle, m_angAbsRotation);
SMDECL(Server::GetLocalVelocity, Vector, m_vecVelocity);
SMDECL(Server::GetFlags, int, m_fFlags);
SMDECL(Server::GetEFlags, int, m_iEFlags);
SMDECL(Server::GetMaxSpeed, float, m_flMaxspeed);
SMDECL(Server::GetGravity, float, m_flGravity);
SMDECL(Server::GetViewOffset, Vector, m_vecViewOffset);
SMDECL(Server::GetPortalLocal, CPortalPlayerLocalData, m_PortalLocal);
SMDECL(Server::GetEntityName, char *, m_iName);
SMDECL(Server::GetEntityClassName, char *, m_iClassname);
SMDECL(Server::GetPlayerState, CPlayerState, pl);

ServerEnt *Server::GetPlayer(int index) {
	return this->UTIL_PlayerByIndex(index);
}
bool Server::IsPlayer(void *entity) {
	return Memory::VMT<bool (*)(void *)>(entity, Offsets::IsPlayer)(entity);
}
bool Server::AllowsMovementChanges() {
	return sv_cheats.GetBool();
}
int Server::GetSplitScreenPlayerSlot(void *entity) {
	// Simplified version of CBasePlayer::GetSplitScreenPlayerSlot
	for (auto i = 0; i < Offsets::MAX_SPLITSCREEN_PLAYERS; ++i) {
		if (server->UTIL_PlayerByIndex(i + 1) == entity) {
			return i;
		}
	}

	return 0;
}
void Server::KillEntity(void *entity) {
	variant_t val = {0};
	val.fieldType = FIELD_VOID;
	void *player = this->GetPlayer(1);
	server->AcceptInput(entity, "Kill", player, player, val, 0);
}

float Server::GetCMTimer() {
	ServerEnt *sv_player = this->GetPlayer(1);
	if (!sv_player) {
		ClientEnt *cl_player = client->GetPlayer(1);
		if (!cl_player) return 0.0f;
		return cl_player->field<float>("fNumSecondsTaken");
	}
	return sv_player->field<float>("fNumSecondsTaken");
}
