#include "Engine.hpp"

#include "Client.hpp"
#include "Console.hpp"
#include "Event.hpp"
#include "InputSystem.hpp"
#include "Game.hpp"
#include "Hook.hpp"
#include "Interface.hpp"
#include "PytalMain.hpp"
#include "Server.hpp"
#include "Utils.hpp"
#include "Variable.hpp"

#include <cstring>
#include <cmath>

#ifdef _WIN32
// clang-format off
#	include <Windows.h>
#	include <Memoryapi.h>
#	define strcasecmp _stricmp
// clang-format on
#else
#	include <sys/mman.h>
#endif

#define FPS_CHECK_WINDOW 0.5f

Variable host_framerate;
Variable net_showmsg;
Variable sv_portal_players;
Variable fps_max;
Variable mat_norendering;

float g_cur_fps = 0.0f;

REDECL(Engine::Disconnect);
REDECL(Engine::SetSignonState);
REDECL(Engine::ChangeLevel);
REDECL(Engine::ClientCommandKeyValues);
#ifndef _WIN32
REDECL(Engine::GetMouseDelta);
#endif
REDECL(Engine::Frame);
REDECL(Engine::PurgeUnusedModels);
REDECL(Engine::ReadCustomData);
REDECL(Engine::ReadConsoleCommand);
REDECL(Engine::plugin_load_callback);
REDECL(Engine::plugin_unload_callback);
REDECL(Engine::exit_callback);
REDECL(Engine::quit_callback);
REDECL(Engine::help_callback);
REDECL(Engine::gameui_activate_callback);
REDECL(Engine::unpause_callback);
REDECL(Engine::playvideo_end_level_transition_callback);
REDECL(Engine::stop_transition_videos_fadeout_callback);
REDECL(Engine::load_callback);
REDECL(Engine::give_callback);
REDECL(Engine::exec_callback);
#ifdef _WIN32
REDECL(Engine::ParseSmoothingInfo_Skip);
REDECL(Engine::ParseSmoothingInfo_Default);
REDECL(Engine::ParseSmoothingInfo_Continue);
REDECL(Engine::ParseSmoothingInfo_Mid);
REDECL(Engine::ParseSmoothingInfo_Mid_Trampoline);
#endif

void Engine::ExecuteCommand(const char *cmd, bool immediately) {
	if (immediately) {
		this->ExecuteClientCmd(this->engineClient->ThisPtr(), cmd);
	} else {
		this->ClientCmd(this->engineClient->ThisPtr(), cmd);
	}
}
int Engine::GetTick() {
	return (this->GetMaxClients() < 2) ? *this->tickcount : TIME_TO_TICKS(*this->net_time);
}
float Engine::ToTime(int tick) {
	return tick * *this->interval_per_tick;
}
int Engine::GetLocalPlayerIndex() {
	return this->GetLocalPlayer(this->engineClient->ThisPtr());
}
edict_t *Engine::PEntityOfEntIndex(int iEntIndex) {
	if (iEntIndex >= 0 && iEntIndex < server->gpGlobals->maxEntities) {
		auto pEdict = reinterpret_cast<edict_t *>((uintptr_t)server->gpGlobals->pEdicts + iEntIndex * sizeof(edict_t));
		if (!pEdict->IsFree()) {
			return pEdict;
		}
	}

	return nullptr;
}
QAngle Engine::GetAngles(int nSlot) {
	auto va = QAngle();
	if (this->GetLocalClient) {
		auto client = this->GetLocalClient(nSlot);
		if (client) {
			va = *reinterpret_cast<QAngle *>((uintptr_t)client + Offsets::viewangles);
		}
	} else {
		this->GetViewAngles(this->engineClient->ThisPtr(), va);
	}
	return va;
}
void Engine::SetAngles(int nSlot, QAngle va) {
	if (this->GetLocalClient) {
		auto client = this->GetLocalClient(nSlot);
		if (client) {
			auto viewangles = reinterpret_cast<QAngle *>((uintptr_t)client + Offsets::viewangles);
			viewangles->x = Math::AngleNormalize(va.x);
			viewangles->y = Math::AngleNormalize(va.y);
			viewangles->z = Math::AngleNormalize(va.z);
		}
	} else {
		this->SetViewAngles(this->engineClient->ThisPtr(), va);
	}
}
void Engine::SendToCommandBuffer(const char *text, int delay) {
	auto slot = this->GetActiveSplitScreenPlayerSlot(nullptr);
	this->Cbuf_AddText(slot, text, delay);
}
int Engine::PointToScreen(const Vector &point, Vector &screen) {
	return this->ScreenPosition(nullptr, point, screen);
}
void Engine::SafeUnload(const char *postCommand) {

	// give events some time to execute before plugin is disabled
	Event::Trigger<Event::PYTAL_UNLOAD>({});
	this->ExecuteCommand("pytal_exit");

	if (postCommand) {
		this->SendToCommandBuffer(postCommand, SAFE_UNLOAD_TICK_DELAY);
	}
}
bool Engine::isRunning() {
	return engine->hoststate->m_activeGame && engine->hoststate->m_currentState == HOSTSTATES::HS_RUN;
}
bool Engine::IsGamePaused() {
	return this->IsPaused(this->engineClient->ThisPtr());
}

int Engine::GetMapIndex(const std::string map) {
	auto it = std::find(Game::mapNames.begin(), Game::mapNames.end(), map);
	if (it != Game::mapNames.end()) {
		return std::distance(Game::mapNames.begin(), it);
	} else {
		return -1;
	}
}

std::string Engine::GetCurrentMapName() {
	static std::string last_map;

	std::string map = this->GetLevelNameShort(this->engineClient->ThisPtr());

	return last_map;
}

bool Engine::IsCoop() {
	if (*client->gamerules) {
		using _IsMultiplayer = bool (__rescall *)(void *thisptr);
		return Memory::VMT<_IsMultiplayer>(*client->gamerules, Offsets::IsMultiplayer)(*client->gamerules);
	}
	return sv_portal_players.GetInt() == 2 || (engine->GetMaxClients() >= 2);
}

bool Engine::IsOrange() {
	static bool isOrange;
	return isOrange;
}
bool Engine::IsSplitscreen() {
	if (!engine->IsCoop()) return false;

	for (int i = 0; i < 2; ++i) {
		ClientEnt *player = client->GetPlayer(i + 1);
		if (!player) continue;

		// m_hSplitScreenPlayers
		auto &splitscreen_players = player->fieldOff<CUtlVector<CBaseHandle>>("m_szLastPlaceName", 40);
		if (splitscreen_players.m_Size > 0) return true;
	}

	return false;
}

bool Engine::Trace(Vector &pos, QAngle &angle, float distMax, int mask, CTraceFilterSimple &filter, CGameTrace &tr) {
	float X = DEG2RAD(angle.x), Y = DEG2RAD(angle.y);
	auto cosX = std::cos(X), cosY = std::cos(Y);
	auto sinX = std::sin(X), sinY = std::sin(Y);

	Vector dir(cosY * cosX, sinY * cosX, -sinX);

	Vector finalDir = Vector(dir.x, dir.y, dir.z).Normalize() * distMax;

	Ray_t ray;
	ray.m_IsRay = true;
	ray.m_IsSwept = true;
	ray.m_Start = VectorAligned(pos.x, pos.y, pos.z);
	ray.m_Delta = VectorAligned(finalDir.x, finalDir.y, finalDir.z);
	ray.m_StartOffset = VectorAligned();
	ray.m_Extents = VectorAligned();

	engine->TraceRay(this->engineTrace->ThisPtr(), ray, mask, &filter, &tr);

	if (tr.fraction >= 1) {
		return false;
	}
	return true;
}

bool Engine::TraceFromCamera(float distMax, int mask, CGameTrace &tr) {
	void *player = server->GetPlayer(GET_SLOT() + 1);

	if (player == nullptr || (int)player == -1)
		return false;

	Vector camPos = server->GetAbsOrigin(player) + server->GetViewOffset(player);
	QAngle angle = engine->GetAngles(GET_SLOT());

	CTraceFilterSimple filter;
	filter.SetPassEntity(server->GetPlayer(GET_SLOT() + 1));

	return this->Trace(camPos, angle, distMax, mask, filter, tr);
}

// ON_EVENT(PRE_TICK) {}

ON_EVENT(PRE_TICK) {
	if (engine->shouldPauseForSync && event.tick >= 0) {
		engine->ExecuteCommand("pause", true);
		engine->shouldPauseForSync = false;
	}
}

bool Engine::ConsoleVisible() {
	return this->Con_IsVisible(this->engineClient->ThisPtr());
}

float Engine::GetHostFrameTime() {
	return this->HostFrameTime(this->engineTool->ThisPtr());
}

float Engine::GetClientTime() {
	return this->ClientTime(this->engineTool->ThisPtr());
}

float Engine::GetHostTime() {
	return this->engineTool->Original<float (__rescall *)(void *thisptr)>(Offsets::HostTick - 1)(this->engineTool->ThisPtr());
}

// CClientState::Disconnect
DETOUR(Engine::Disconnect, bool bShowMainMenu) {
	return Engine::Disconnect(thisptr, bShowMainMenu);
}


// CVEngineServer::ChangeLevel
DETOUR(Engine::ChangeLevel, const char *s1, const char *s2) {
	if (s1 && engine->GetCurrentMapName() != s1) engine->isLevelTransition = true;
	return Engine::ChangeLevel(thisptr, s1, s2);
}

#ifndef _WIN32
// CVEngineClient::GetMouseDelta
DETOUR_T(void, Engine::GetMouseDelta, int &x, int &y, bool ignore_next) {
	Engine::GetMouseDelta(thisptr, x, y, ignore_next);
	inputSystem->DPIScaleDeltas(x, y);
}
#endif

void Engine::GetTicks(int &host, int &server, int &client) {
	auto &et = this->engineTool;
	using _Fn = int(__rescall *)(void *thisptr);
	host = et->Original<_Fn>(Offsets::HostTick)(et->ThisPtr());
	server = et->Original<_Fn>(Offsets::ServerTick)(et->ThisPtr());
	client = et->Original<_Fn>(Offsets::ClientTick)(et->ThisPtr());
}

// CEngine::Frame
//DETOUR(Engine::Frame) {}

DETOUR_COMMAND(Engine::plugin_load) {
	// Prevent crash when trying to load PYTAL twice or try to find the module in
	// the plugin list if the initial search thread failed
	if (args.ArgC() >= 2) {
		auto file = std::string(args[1]);
		if (Utils::EndsWith(file, std::string(MODULE("pytal"))) || Utils::EndsWith(file, std::string("pytal"))) {
			if (pytal.GetPlugin()) {
				pytal.plugin->ptr->m_bDisable = true;
				console->PrintActive("PYTAL: Plugin fully loaded!\n");
			}
			return;
		}
	}
	Engine::plugin_load_callback(args);
}
DETOUR_COMMAND(Engine::plugin_unload) {
	if (args.ArgC() >= 2 && pytal.GetPlugin() && std::atoi(args[1]) == pytal.plugin->index) {
		engine->SafeUnload();
	} else {
		engine->plugin_unload_callback(args);
	}
}
DETOUR_COMMAND(Engine::exit) {
	engine->SafeUnload("exit");
}
DETOUR_COMMAND(Engine::quit) {
	engine->SafeUnload("quit");
}
DETOUR_COMMAND(Engine::help) {
}
DETOUR_COMMAND(Engine::gameui_activate) {
	if (engine->shouldSuppressPause) {
		engine->shouldSuppressPause = false;
		return;
	}

	Engine::gameui_activate_callback(args);
}

Color Engine::GetLightAtPoint(Vector point) {
#ifdef _WIN32
	// MSVC bug workaround - COM interfaces apparently don't quite follow
	// the behaviour implemented by '__thiscall' in some cases, and
	// returning structs is one of them! The compiler flips around the
	// first two args otherwise, with predictably disastrous results.
	Vector light;
	engine->GetLightForPoint(engine->engineClient->ThisPtr(), light, point, true);
#else
	Vector light = engine->GetLightForPoint(engine->engineClient->ThisPtr(), point, true);
#endif

	return Color{(uint8_t)(light.x * 255), (uint8_t)(light.y * 255), (uint8_t)(light.z * 255), 255};
}

static _CommandCompletionCallback exec_orig_completion;
DECL_COMMAND_FILE_COMPLETION(exec, ".cfg", Utils::ssprintf("%s/cfg", engine->GetGameDirectory()), 1)

bool Engine::Init() {
	this->engineClient = Interface::Create(this->Name(), "VEngineClient015");
	this->s_ServerPlugin = Interface::Create(this->Name(), "ISERVERPLUGINHELPERS001", false);

	if (this->engineClient) {
		this->GetScreenSize = this->engineClient->Original<_GetScreenSize>(Offsets::GetScreenSize);
		this->ClientCmd = this->engineClient->Original<_ClientCmd>(Offsets::ClientCmd);
		this->ExecuteClientCmd = this->engineClient->Original<_ExecuteClientCmd>(Offsets::ExecuteClientCmd);
		this->GetLocalPlayer = this->engineClient->Original<_GetLocalPlayer>(Offsets::GetLocalPlayer);
		this->GetViewAngles = this->engineClient->Original<_GetViewAngles>(Offsets::GetViewAngles);
		this->SetViewAngles = this->engineClient->Original<_SetViewAngles>(Offsets::SetViewAngles);
		this->GetMaxClients = this->engineClient->Original<_GetMaxClients>(Offsets::GetMaxClients);
		this->GetGameDirectory = this->engineClient->Original<_GetGameDirectory>(Offsets::GetGameDirectory);
		this->GetSaveDirName = this->engineClient->Original<_GetSaveDirName>(Offsets::GetSaveDirName);
		this->DebugDrawPhysCollide = this->engineClient->Original<_DebugDrawPhysCollide>(Offsets::DebugDrawPhysCollide);
		this->IsPaused = this->engineClient->Original<_IsPaused>(Offsets::IsPaused);
		this->Con_IsVisible = this->engineClient->Original<_Con_IsVisible>(Offsets::Con_IsVisible);
		this->GetLevelNameShort = this->engineClient->Original<_GetLevelNameShort>(Offsets::GetLevelNameShort);
		this->GetLightForPoint = this->engineClient->Original<_GetLightForPoint>(Offsets::GetLightForPoint);

#ifndef _WIN32
		this->engineClient->Hook(Engine::GetMouseDelta_Hook, Engine::GetMouseDelta, Offsets::GetMouseDelta);
#endif

		Memory::Read<_Cbuf_AddText>((uintptr_t)this->ClientCmd + Offsets::Cbuf_AddText, &this->Cbuf_AddText);
		Memory::Deref<void *>((uintptr_t)this->Cbuf_AddText + Offsets::s_CommandBuffer, &this->s_CommandBuffer);

		Memory::Read((uintptr_t)this->SetViewAngles + Offsets::GetLocalClient, &this->GetLocalClient);

		this->m_bWaitEnabled = reinterpret_cast<bool *>((uintptr_t)s_CommandBuffer + Offsets::m_bWaitEnabled);
		this->m_bWaitEnabled2 = reinterpret_cast<bool *>((uintptr_t)this->m_bWaitEnabled + Offsets::CCommandBufferSize);

		auto GetSteamAPIContext = this->engineClient->Original<uintptr_t (*)()>(Offsets::GetSteamAPIContext);

		if (this->g_VEngineServer = Interface::Create(this->Name(), "VEngineServer022")) {
			this->g_VEngineServer->Hook(Engine::ChangeLevel_Hook, Engine::ChangeLevel, Offsets::ChangeLevel);
			this->g_VEngineServer->Hook(Engine::ClientCommandKeyValues_Hook, Engine::ClientCommandKeyValues, Offsets::ClientCommandKeyValues);
			this->ClientCommand = this->g_VEngineServer->Original<_ClientCommand>(Offsets::ClientCommand);
			this->IsServerPaused = this->g_VEngineServer->Original<_IsServerPaused>(Offsets::IsServerPaused);
			this->ServerPause = this->g_VEngineServer->Original<_ServerPause>(Offsets::ServerPause);
		}

		typedef void *(*_GetClientState)();
		auto GetClientState = Memory::Read<_GetClientState>((uintptr_t)this->ClientCmd + Offsets::GetClientStateFunction);
		void *clPtr = GetClientState();

		this->GetActiveSplitScreenPlayerSlot = this->engineClient->Original<_GetActiveSplitScreenPlayerSlot>(Offsets::GetActiveSplitScreenPlayerSlot);

		if (this->cl = Interface::Create(clPtr)) {

			// this->cl->Hook(Engine::SetSignonState_Hook, Engine::SetSignonState, Offsets::Disconnect - 1);
			this->cl->Hook(Engine::Disconnect_Hook, Engine::Disconnect, Offsets::Disconnect);
#if _WIN32
			auto IServerMessageHandler_VMT = Memory::Deref<uintptr_t>((uintptr_t)this->cl->ThisPtr() + IServerMessageHandler_VMT_Offset);
			auto ProcessTick = Memory::Deref<uintptr_t>(IServerMessageHandler_VMT + sizeof(uintptr_t) * Offsets::ProcessTick);
#else
			auto ProcessTick = this->cl->Original(Offsets::ProcessTick);
#endif

			tickcount = Memory::Deref<int *>(ProcessTick + Offsets::tickcount);

			interval_per_tick = Memory::Deref<float *>(ProcessTick + Offsets::interval_per_tick);

			auto SetSignonState = this->cl->Original(Offsets::Disconnect - 1);
			auto HostState_OnClientConnected = Memory::Read(SetSignonState + Offsets::HostState_OnClientConnected);
			Memory::Deref<CHostState *>(HostState_OnClientConnected + Offsets::hoststate, &hoststate);
		}

		if (this->engineTrace = Interface::Create(this->Name(), "EngineTraceServer004")) {
			this->TraceRay = this->engineTrace->Original<_TraceRay>(Offsets::TraceRay);
			this->PointOutsideWorld = this->engineTrace->Original<_PointOutsideWorld>(Offsets::TraceRay + 14);
		}
	}

	if (this->engineTool = Interface::Create(this->Name(), "VENGINETOOL003", false)) {
		auto GetCurrentMap = this->engineTool->Original(Offsets::GetCurrentMap);
		this->m_szLevelName = Memory::Deref<char *>(GetCurrentMap + Offsets::m_szLevelName);

		this->m_bLoadgame = reinterpret_cast<bool *>((uintptr_t)this->m_szLevelName + Offsets::m_bLoadGame);

		this->HostFrameTime = this->engineTool->Original<_HostFrameTime>(Offsets::HostFrameTime);
		this->ClientTime = this->engineTool->Original<_ClientTime>(Offsets::ClientTime);

		this->PrecacheModel = this->engineTool->Original<_PrecacheModel>(Offsets::PrecacheModel);
	}

	if (auto s_EngineAPI = Interface::Create(this->Name(), "VENGINE_LAUNCHER_API_VERSION004", false)) {
		auto IsRunningSimulation = s_EngineAPI->Original(Offsets::IsRunningSimulation);
		void *engAddr;
		engAddr = Memory::DerefDeref<void *>(IsRunningSimulation + Offsets::eng);

		Interface::Delete(s_EngineAPI);
	}

	this->s_GameEventManager = Interface::Create(this->Name(), "GAMEEVENTSMANAGER002", false);
	if (this->s_GameEventManager) {
		this->AddListener = this->s_GameEventManager->Original<_AddListener>(Offsets::AddListener);
		this->RemoveListener = this->s_GameEventManager->Original<_RemoveListener>(Offsets::RemoveListener);

		auto FireEventClientSide = s_GameEventManager->Original(Offsets::FireEventClientSide);
		auto FireEventIntern = Memory::Read(FireEventClientSide + Offsets::FireEventIntern);
		Memory::Read<_ConPrintEvent>(FireEventIntern + Offsets::ConPrintEvent, &this->ConPrintEvent);
	}

#ifdef _WIN32
	// Note: we don't get readCustomDataAddr anymore as we find this
	// below anyway
	auto parseSmoothingInfoAddr = Memory::Scan(this->Name(), "55 8B EC 0F 57 C0 81 EC ? ? ? ? B9 ? ? ? ? 8D 85 ? ? ? ? EB", 178);

	if (parseSmoothingInfoAddr) {
		MH_HOOK_MID(Engine::ParseSmoothingInfo_Mid, parseSmoothingInfoAddr);  // Hook switch-case
		Engine::ParseSmoothingInfo_Continue = parseSmoothingInfoAddr + 8;     // Back to original function
		Engine::ParseSmoothingInfo_Default = parseSmoothingInfoAddr + 133;    // Default case
		Engine::ParseSmoothingInfo_Skip = parseSmoothingInfoAddr - 29;        // Continue loop

	}
#endif

	// This is the address of the one interesting call to ReadCustomData - the E8 byte indicates the start of the call instruction
#ifdef _WIN32
	this->readCustomDataInjectAddr = Memory::Scan(this->Name(), "8D 45 E8 50 8D 4D BC 51 8D 4F 04 E8 ? ? ? ? 8B 4D BC 83 F9 FF", 12);
	this->readConsoleCommandInjectAddr = Memory::Scan(this->Name(), "8B 45 F4 50 68 FE 04 00 00 68 ? ? ? ? 8D 4D 90 E8 ? ? ? ? 8D 4F 04 E8", 26);
#else
	if (pytal.game->Is(SourceGame_EIPRelPIC)) {
		this->readCustomDataInjectAddr = Memory::Scan(this->Name(), "8D 85 C4 FE FF FF 83 EC 04 8D B5 E8 FE FF FF 56 50 FF B5 94 FE FF FF E8", 24);
		this->readConsoleCommandInjectAddr = Memory::Scan(this->Name(), "FF B5 AC FE FF FF 8D B5 E8 FE FF FF 68 FE 04 00 00 68 ? ? ? ? 56 E8 ? ? ? ? 58 FF B5 94 FE FF FF E8", 36);
	} else {
		this->readCustomDataInjectAddr = Memory::Scan(this->Name(), "89 44 24 08 8D 85 B0 FE FF FF 89 44 24 04 8B 85 8C FE FF FF 89 04 24 E8", 24);
		this->readConsoleCommandInjectAddr = Memory::Scan(this->Name(), "89 44 24 0C 8D 85 C0 FE FF FF 89 04 24 E8 ? ? ? ? 8B 85 8C FE FF FF 89 04 24 E8", 28);
	}
#endif

	// Pesky memory protection doesn't want us overwriting code - we
	// get around it with a call to mprotect or VirtualProtect
	Memory::UnProtect((void *)this->readCustomDataInjectAddr, 4);
	Memory::UnProtect((void *)this->readConsoleCommandInjectAddr, 4);

	// It's a relative call, so we have to do some weird fuckery lol
	Engine::ReadCustomData = reinterpret_cast<_ReadCustomData>(*(uint32_t *)this->readCustomDataInjectAddr + (this->readCustomDataInjectAddr + 4));
	*(uint32_t *)this->readCustomDataInjectAddr = (uint32_t)&ReadCustomData_Hook - (this->readCustomDataInjectAddr + 4);  // Add 4 to get address of next instruction

	if (auto debugoverlay = Interface::Create(this->Name(), "VDebugOverlay004", false)) {
		ScreenPosition = debugoverlay->Original<_ScreenPosition>(Offsets::ScreenPosition);
		Interface::Delete(debugoverlay);
	}

	Command::Hook("plugin_load", Engine::plugin_load_callback_hook, Engine::plugin_load_callback);
	Command::Hook("plugin_unload", Engine::plugin_unload_callback_hook, Engine::plugin_unload_callback);
	Command::Hook("exit", Engine::exit_callback_hook, Engine::exit_callback);
	Command::Hook("quit", Engine::quit_callback_hook, Engine::quit_callback);
	Command::Hook("help", Engine::help_callback_hook, Engine::help_callback);
	//Command::Hook("load", Engine::load_callback_hook, Engine::load_callback);
	Command::Hook("give", Engine::give_callback_hook, Engine::give_callback);
	Command::Hook("exec", Engine::exec_callback_hook, Engine::exec_callback);
	Command::HookCompletion("exec", AUTOCOMPLETION_FUNCTION(exec), exec_orig_completion);

	Command::Hook("gameui_activate", Engine::gameui_activate_callback_hook, Engine::gameui_activate_callback);
	Command::Hook("playvideo_end_level_transition", Engine::playvideo_end_level_transition_callback_hook, Engine::playvideo_end_level_transition_callback);
	Command::Hook("stop_transition_videos_fadeout", Engine::stop_transition_videos_fadeout_callback_hook, Engine::stop_transition_videos_fadeout_callback);

	host_framerate = Variable("host_framerate");
	net_showmsg = Variable("net_showmsg");
	sv_portal_players = Variable("sv_portal_players");
	fps_max = Variable("fps_max");
	mat_norendering = Variable("mat_norendering");

	// Dumb fix for valve cutting off convar descriptions at 80
	// characters for some reason
	/* TODO Memory::Scan data segments
	char *s = (char *)Memory::Scan(this->Name(), "25 2d 38 30 73 20 2d 20 25 2e 38 30 73 0a 00");  // "%-80s - %.80s"
	if (s) {
		Memory::UnProtect(s, 11);
		strcpy(s, "%-80s - %s");
	}*/

	if (this->g_physCollision = Interface::Create(MODULE("vphysics"), "VPhysicsCollision007")) {
		this->CreateDebugMesh = this->g_physCollision->Original<_CreateDebugMesh>(Offsets::CreateDebugMesh);
		this->DestroyDebugMesh = this->g_physCollision->Original<_DestroyDebugMesh>(Offsets::DestroyDebugMesh);
	}

	return this->hasLoaded = this->engineClient && this->s_ServerPlugin && this->engineTrace;
}
void Engine::Shutdown() {
	if (this->engineClient) {
		auto GetSteamAPIContext = this->engineClient->Original<uintptr_t (*)()>(Offsets::GetSteamAPIContext);
	}

	Interface::Delete(this->engineClient);
	Interface::Delete(this->s_ServerPlugin);
	Interface::Delete(this->cl);
	Interface::Delete(this->eng);
	Interface::Delete(this->s_GameEventManager);
	Interface::Delete(this->engineTool);
	Interface::Delete(this->engineTrace);
	Interface::Delete(this->g_VEngineServer);
	Interface::Delete(this->g_physCollision);

	// Reset to the offsets that were originally in the code
#ifdef _WIN32
	*(uint32_t *)this->readCustomDataInjectAddr = 0x50E8458D;
	*(uint32_t *)this->readConsoleCommandInjectAddr = 0x000491E3;
#else
	*(uint32_t *)this->readCustomDataInjectAddr = 0x08244489;
	*(uint32_t *)this->readConsoleCommandInjectAddr = 0x0008155A;
#endif

#ifdef _WIN32
	MH_UNHOOK(Engine::ParseSmoothingInfo_Mid);

#endif
	Command::Unhook("plugin_load", Engine::plugin_load_callback);
	Command::Unhook("plugin_unload", Engine::plugin_unload_callback);
	Command::Unhook("exit", Engine::exit_callback);
	Command::Unhook("quit", Engine::quit_callback);
	Command::Unhook("help", Engine::help_callback);
	Command::Unhook("load", Engine::load_callback);
	Command::Unhook("give", Engine::give_callback);
	Command::UnhookCompletion("exec", exec_orig_completion);
	Command::Unhook("gameui_activate", Engine::gameui_activate_callback);
	Command::Unhook("playvideo_end_level_transition", Engine::playvideo_end_level_transition_callback);
	Command::Unhook("stop_transition_videos_fadeout", Engine::stop_transition_videos_fadeout_callback);
}

Engine *engine;
