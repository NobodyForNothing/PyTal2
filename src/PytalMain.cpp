#include "PytalMain.hpp"

#include "Version.hpp"

#include <cstring>
#include <ctime>

#ifdef _WIN32
#	include <filesystem>
#endif

#include "Cheats.hpp"
#include "Command.hpp"
#include "CrashHandler.hpp"
#include "Event.hpp"
#include "Features/EntityList.hpp"
#include "Features/OffsetFinder.hpp"
#include "Game.hpp"
#include "Hook.hpp"
#include "Interface.hpp"
#include "Modules.hpp"
#include "Variable.hpp"

Plugin::Plugin()
	: ptr(nullptr)
	, index(0) {
}

PytalMain pytal;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(PytalMain, IServerPluginCallbacks, INTERFACEVERSION_ISERVERPLUGINCALLBACKS, pytal);



PytalMain::PytalMain()
	: modules(new Modules())
	, cheats(new Cheats())
	, plugin(new Plugin())
	, game(Game::CreateNew()) {
}

bool PytalMain::Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory) {
	console = new Console();
	if (!console->Init())
		return false;

	if (this->game) {
		this->game->LoadOffsets();

		CrashHandler::Init();

		PytalInitHandler::RunAll();

		tier1 = new Tier1();
		if (tier1->Init()) {
			this->modules->AddModule<InputSystem>(&inputSystem);
			this->modules->AddModule<Scheme>(&scheme);
			this->modules->AddModule<Surface>(&surface);
			this->modules->AddModule<VGui>(&vgui);
			this->modules->AddModule<Engine>(&engine);
			this->modules->AddModule<Client>(&client);
			this->modules->AddModule<Server>(&server);
			this->modules->InitAll();

			if (engine && engine->hasLoaded) {

				this->cheats->Init();

				this->SearchPlugin();

				console->PrintActive("Loaded " PLUGIN_NAME ", Version %s\n", PYTAL_VERSION);
				return true;
			} else {
				console->Warning(PLUGIN_NAME ": Failed to load engine module!\n");
			}
		} else {
			console->Warning(PLUGIN_NAME ": Failed to load tier1 module!\n");
		}
	} else {
		console->Warning(PLUGIN_NAME ": Game not supported!\n");
	}

	console->Warning(PLUGIN_NAME ": Failed to load the plugin!\n");

	if (pytal.cheats) {
		pytal.cheats->Shutdown();
	}

	if (pytal.modules) {
		pytal.modules->ShutdownAll();
	}

	Variable::ClearAllCallbacks();
	SAFE_DELETE(pytal.cheats)
	SAFE_DELETE(pytal.modules)
	SAFE_DELETE(pytal.plugin)
	SAFE_DELETE(pytal.game)
	SAFE_DELETE(tier1)
	SAFE_DELETE(console)
	CrashHandler::Cleanup();
	return false;
}

// Plugin has to disable itself in the plugin list or the game might crash because of missing callbacks
// This is a race condition though
bool PytalMain::GetPlugin() {
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
void PytalMain::SearchPlugin() {
	this->findPluginThread = std::thread([this]() {
		GO_THE_FUCK_TO_SLEEP(1000);
		if (this->GetPlugin()) {
			this->plugin->ptr->m_bDisable = true;
		} else {
			console->DevWarning(PLUGIN_NAME": Failed to find the plugin in the plugin list!\nTry again with \"plugin_load\".\n");
		}
	});
	this->findPluginThread.detach();
}

//TODO: create a macro for plugin-named command creation

CON_COMMAND(pytal_about, "pytal_about - prints info about Pytal plugin\n") {
	console->Print(PLUGIN_DESC "\n");
	console->Print("More information at: " PLUGIN_WEB "\n");
	console->Print("Game: %s\n", pytal.game->Version());
	console->Print("Version: " PYTAL_VERSION "\n");
	console->Print("Built: " PLUGIN_BUILT "\n");
}

CON_COMMAND(pytal_exit, "pytal_exit - removes all function hooks, registered commands and unloads the module\n") {
	Variable::ClearAllCallbacks();

	Hook::DisableAll();

	if (pytal.cheats) {
		pytal.cheats->Shutdown();
	}

	if (pytal.GetPlugin()) {
		// Plugin has to unhook CEngine some ticks before unloading the module
		auto unload = std::string("plugin_unload ") + std::to_string(pytal.plugin->index);
		engine->SendToCommandBuffer(unload.c_str(), SAFE_UNLOAD_TICK_DELAY);
	}

	if (pytal.modules) {
		pytal.modules->ShutdownAll();
	}

	SAFE_DELETE(pytal.cheats)
	SAFE_DELETE(pytal.modules)
	SAFE_DELETE(pytal.plugin)
	SAFE_DELETE(pytal.game)

	console->Print(PLUGIN_NAME ": Disabling...\n");

	SAFE_DELETE(tier1)
	SAFE_DELETE(console)
	CrashHandler::Cleanup();
}

#pragma region Unused callbacks
void PytalMain::Unload() {
}
void PytalMain::Pause() {
}
void PytalMain::UnPause() {
}
const char *PytalMain::GetPluginDescription() {
	return PYTAL_PLUGIN_SIGNATURE;
}
void PytalMain::LevelInit(char const *pMapName) {
}
void PytalMain::ServerActivate(void *pEdictList, int edictCount, int clientMax) {
}
void PytalMain::GameFrame(bool simulating) {
}
void PytalMain::LevelShutdown() {
}
void PytalMain::ClientFullyConnect(void *pEdict) {
}
void PytalMain::ClientActive(void *pEntity) {
}
void PytalMain::ClientDisconnect(void *pEntity) {
}
void PytalMain::ClientPutInServer(void *pEntity, char const *playername) {
}
void PytalMain::SetCommandClient(int index) {
}
void PytalMain::ClientSettingsChanged(void *pEdict) {
}
int PytalMain::ClientConnect(bool *bAllowConnect, void *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen) {
	return 0;
}
int PytalMain::ClientCommand(void *pEntity, const void *&args) {
	return 0;
}
int PytalMain::NetworkIDValidated(const char *pszUserName, const char *pszNetworkID) {
	return 0;
}
void PytalMain::OnQueryCvarValueFinished(int iCookie, void *pPlayerEntity, int eStatus, const char *pCvarName, const char *pCvarValue) {
}
void PytalMain::OnEdictAllocated(void *edict) {
}
void PytalMain::OnEdictFreed(const void *edict) {
}
#pragma endregion
