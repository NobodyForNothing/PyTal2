#include "PYTAL.hpp"

#include "Version.hpp"

#include <cstring>
#include <ctime>
#include <curl/curl.h>

#ifdef _WIN32
#	include <filesystem>
#endif

#include "Cheats.hpp"
#include "Command.hpp"
#include "CrashHandler.hpp"
#include "Event.hpp"
#include "Features/SeasonalASCII.hpp"
#include "Game.hpp"
#include "Hook.hpp"
#include "Interface.hpp"
#include "Modules.hpp"
#include "Variable.hpp"

PYTAL pytal;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(PYTAL, IServerPluginCallbacks, INTERFACEVERSION_ISERVERPLUGINCALLBACKS, pytal);


PYTAL::PYTAL()
	: modules(new Modules())
	, features(new Features())
	, cheats(new Cheats())
	, plugin(new Plugin())
	, game(Game::CreateNew()) {
}

bool PYTAL::Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory) {
	console = new Console();
	if (!console->Init())
		return false;

#ifdef _WIN32
	// The auto-updater can create this file on Windows; we should try
	// to delete it.
	try {
		if (std::filesystem::exists("pytal.dll.old-auto")) {
			std::filesystem::remove("pytal.dll.old-auto");
		}
		if (std::filesystem::exists("pytal.pdb.old-auto")) {
			std::filesystem::remove("pytal.pdb.old-auto");
		}
	} catch (...) {
	}
#endif

	if (this->game) {
		this->game->LoadOffsets();

		CrashHandler::Init();

		PytalInitHandler::RunAll();

		curl_global_init(CURL_GLOBAL_ALL);

		tier1 = new Tier1();
		if (tier1->Init()) {
			this->features->AddFeature<Cvars>(&cvars);
			this->features->AddFeature<Session>(&session);
			this->features->AddFeature<StepCounter>(&stepCounter);
			this->features->AddFeature<Summary>(&summary);
			this->features->AddFeature<Teleporter>(&teleporter);
			SpeedrunTimer::Init();
			this->features->AddFeature<Stats>(&stats);
			this->features->AddFeature<StatsCounter>(&statsCounter);
			this->features->AddFeature<Sync>(&synchro);
			this->features->AddFeature<ReloadedFix>(&reloadedFix);
			this->features->AddFeature<Timer>(&timer);
			this->features->AddFeature<EntityInspector>(&inspector);
			this->features->AddFeature<SeamshotFind>(&seamshotFind);
			this->features->AddFeature<ClassDumper>(&classDumper);
			this->features->AddFeature<EntityList>(&entityList);
			this->features->AddFeature<TasController>(&tasControllers[0]);
			this->features->AddFeature<TasController>(&tasControllers[1]);
			this->features->AddFeature<TasPlayer>(&tasPlayer);
			this->features->AddFeature<PauseTimer>(&pauseTimer);
			this->features->AddFeature<DataMapDumper>(&dataMapDumper);
			this->features->AddFeature<FovChanger>(&fovChanger);
			this->features->AddFeature<Camera>(&camera);
			this->features->AddFeature<GroundFramesCounter>(&groundFramesCounter);
			this->features->AddFeature<TimescaleDetect>(&timescaleDetect);
			this->features->AddFeature<PlayerTrace>(&playerTrace);
			toastHud.InitMessageHandler();

			this->modules->AddModule<InputSystem>(&inputSystem);
			this->modules->AddModule<Scheme>(&scheme);
			this->modules->AddModule<Surface>(&surface);
			this->modules->AddModule<VGui>(&vgui);
			this->modules->AddModule<Engine>(&engine);
			this->modules->AddModule<Client>(&client);
			this->modules->AddModule<Server>(&server);
			this->modules->AddModule<MaterialSystem>(&materialSystem);
			this->modules->AddModule<FileSystem>(&fileSystem);
			this->modules->InitAll();

			InitPYTALChecksum();

			if (engine && engine->hasLoaded) {
				engine->demoplayer->Init();
				engine->demorecorder->Init();

				this->cheats->Init();

				this->features->AddFeature<Listener>(&listener);

				if (this->game->Is(SourceGame_Portal2 | SourceGame_ApertureTag)) {
					this->features->AddFeature<WorkshopList>(&workshop);
				}

				if (listener) {
					listener->Init();
				}

				this->SearchPlugin();

				console->PrintActive("Loaded SourceAutoRecord, Version %s\n", PYTAL_VERSION);
				
				SeasonalASCII::Init();

				return true;
			} else {
				console->Warning("PYTAL: Failed to load engine module!\n");
			}
		} else {
			console->Warning("PYTAL: Failed to load tier1 module!\n");
		}
	} else {
		console->Warning("PYTAL: Game not supported!\n");
	}

	console->Warning("PYTAL: Failed to load SourceAutoRecord!\n");

	if (pytal.cheats) {
		pytal.cheats->Shutdown();
	}
	if (pytal.features) {
		pytal.features->DeleteAll();
	}

	if (pytal.modules) {
		pytal.modules->ShutdownAll();
	}

	// This isn't in pytal.modules
	if (tier1) {
		tier1->Shutdown();
	}

	Variable::ClearAllCallbacks();
	SAFE_DELETE(pytal.features)
	SAFE_DELETE(pytal.cheats)
	SAFE_DELETE(pytal.modules)
	SAFE_DELETE(pytal.plugin)
	SAFE_DELETE(pytal.game)
	SAFE_DELETE(tier1)
	SAFE_DELETE(console)
	CrashHandler::Cleanup();
	return false;
}

// PYTAL has to disable itself in the plugin list or the game might crash because of missing callbacks
// This is a race condition though
bool PYTAL::GetPlugin() {
	auto s_ServerPlugin = reinterpret_cast<uintptr_t>(engine->s_ServerPlugin->ThisPtr());
	auto m_Size = *reinterpret_cast<int *>(s_ServerPlugin + CServerPlugin_m_Size);
	if (m_Size > 0) {
		auto m_Plugins = *reinterpret_cast<uintptr_t *>(s_ServerPlugin + CServerPlugin_m_Plugins);
		for (auto i = 0; i < m_Size; ++i) {
			auto ptr = *reinterpret_cast<CPlugin **>(m_Plugins + sizeof(uintptr_t) * i);
			if (!std::strcmp(ptr->m_szName, PYTAL_PLUGIN_SIGNATURE)) {
				this->plugin->ptr = ptr;
				this->plugin->index = i;
				return true;
			}
		}
	}
	return false;
}
void PYTAL::SearchPlugin() {
	this->findPluginThread = std::thread([this]() {
		GO_THE_FUCK_TO_SLEEP(1000);
		if (this->GetPlugin()) {
			this->plugin->ptr->m_bDisable = true;
		} else {
			console->DevWarning("PYTAL: Failed to find PYTAL in the plugin list!\nTry again with \"plugin_load\".\n");
		}
	});
	this->findPluginThread.detach();
}

void PYTAL::Unload() {
	if (unloading) return;
	unloading = true;

	curl_global_cleanup();
	statsCounter->RecordDatas(session->GetTick());
	statsCounter->ExportToFile(pytal_statcounter_filePath.GetString());

	networkManager.Disconnect();

	Variable::ClearAllCallbacks();

	Hook::DisableAll();

	if (pytal.cheats) {
		pytal.cheats->Shutdown();
	}
	if (pytal.features) {
		pytal.features->DeleteAll();
	}

	if (pytal.GetPlugin()) {
		// PYTAL has to unhook CEngine some ticks before unloading the module
		auto unload = std::string("plugin_unload ") + std::to_string(pytal.plugin->index);
		engine->SendToCommandBuffer(unload.c_str(), SAFE_UNLOAD_TICK_DELAY);
	}

	if (pytal.modules) {
		pytal.modules->ShutdownAll();
	}

	// This isn't in pytal.modules
	if (tier1) {
		tier1->Shutdown();
	}

	SAFE_DELETE(pytal.features)
	SAFE_DELETE(pytal.cheats)
	SAFE_DELETE(pytal.modules)
	SAFE_DELETE(pytal.plugin)
	SAFE_DELETE(pytal.game)

	console->Print("Cya :)\n");

	SAFE_DELETE(tier1)
	SAFE_DELETE(console)
	CrashHandler::Cleanup();
}

CON_COMMAND(pytal_session, "pytal_session - prints the current tick of the server since it has loaded\n") {
	auto tick = session->GetTick();
	console->Print("Session Tick: %i (%.3f)\n", tick, engine->ToTime(tick));
	if (*engine->demorecorder->m_bRecording) {
		tick = engine->demorecorder->GetTick();
		console->Print("Demo Recorder Tick: %i (%.3f)\n", tick, engine->ToTime(tick));
	}
	if (engine->demoplayer->IsPlaying()) {
		tick = engine->demoplayer->GetTick();
		console->Print("Demo Player Tick: %i (%.3f)\n", tick, engine->ToTime(tick));
	}
}
CON_COMMAND(pytal_about, "pytal_about - prints info about PYTAL plugin\n") {
	console->Print("SourceAutoRecord is a speedrun plugin for Source Engine games.\n");
	console->Print("More information at: https://github.com/p2sr/SourceAutoRecord or https://wiki.portal2.sr/PYTAL\n");
	console->Print("Game: %s\n", pytal.game->Version());
	console->Print("Version: " PYTAL_VERSION "\n");
	console->Print("Built: " PYTAL_BUILT "\n");
}
CON_COMMAND(pytal_cvars_dump, "pytal_cvars_dump - dumps all cvars to a file\n") {
	std::ofstream file("game.cvars", std::ios::out | std::ios::trunc | std::ios::binary);
	auto result = cvars->Dump(file);
	file.close();

	console->Print("Dumped %i cvars to game.cvars!\n", result);
}
CON_COMMAND(pytal_cvars_dump_doc, "pytal_cvars_dump_doc - dumps all PYTAL cvars to a file\n") {
	std::ofstream file("pytal.cvars", std::ios::out | std::ios::trunc | std::ios::binary);
	auto result = cvars->DumpDoc(file);
	file.close();

	console->Print("Dumped %i cvars to pytal.cvars!\n", result);
}
CON_COMMAND(pytal_cvars_lock, "pytal_cvars_lock - restores default flags of unlocked cvars\n") {
	cvars->Lock();
}
CON_COMMAND(pytal_cvars_unlock, "pytal_cvars_unlock - unlocks all special cvars\n") {
	cvars->Unlock();
}
CON_COMMAND(pytal_cvarlist, "pytal_cvarlist - lists all PYTAL cvars and unlocked engine cvars\n") {
	cvars->ListAll();
}
CON_COMMAND(pytal_rename, "pytal_rename <name> - changes your name\n") {
	if (args.ArgC() != 2) {
		return console->Print(pytal_rename.ThisPtr()->m_pszHelpString);
	}

	auto name = Variable("name");
	if (!!name) {
		name.DisableChange();
		name.SetValue(args[1]);
		name.EnableChange();
	}
}
CON_COMMAND(pytal_exit, "pytal_exit - removes all function hooks, registered commands and unloads the module\n") {
	pytal.Unload();
}

#pragma region Unused callbacks
void PYTAL::Pause() {
}
void PYTAL::UnPause() {
}
const char *PYTAL::GetPluginDescription() {
	return PYTAL_PLUGIN_SIGNATURE;
}
void PYTAL::LevelInit(char const *pMapName) {
}
void PYTAL::ServerActivate(void *pEdictList, int edictCount, int clientMax) {
}
void PYTAL::GameFrame(bool simulating) {
}
void PYTAL::LevelShutdown() {
}
void PYTAL::ClientFullyConnect(void *pEdict) {
}
void PYTAL::ClientActive(void *pEntity) {
}
void PYTAL::ClientDisconnect(void *pEntity) {
}
void PYTAL::ClientPutInServer(void *pEntity, char const *playername) {
}
void PYTAL::SetCommandClient(int index) {
}
void PYTAL::ClientSettingsChanged(void *pEdict) {
}
int PYTAL::ClientConnect(bool *bAllowConnect, void *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen) {
	return 0;
}
int PYTAL::ClientCommand(void *pEntity, const void *&args) {
	return 0;
}
int PYTAL::NetworkIDValidated(const char *pszUserName, const char *pszNetworkID) {
	return 0;
}
void PYTAL::OnQueryCvarValueFinished(int iCookie, void *pPlayerEntity, int eStatus, const char *pCvarName, const char *pCvarValue) {
}
void PYTAL::OnEdictAllocated(void *edict) {
}
void PYTAL::OnEdictFreed(const void *edict) {
}
#pragma endregion
