#include "PythonExtension.hpp"

#include <Python.h>

#include "PytalMain.hpp"
#include "Modules/Engine.hpp"
#include "Features/EntityList.hpp"
#include "Offsets.hpp"
#include "Modules/Server.hpp"

#include "Features/Python/types/PythonEntity.hpp"

static PyObject* game_print(PyObject *self, PyObject *args);
static PyObject* game_execute_console(PyObject *self, PyObject *args);
static PyObject* game_entities(PyObject *self, PyObject *Py_UNUSED(args));
static PyObject* game_entity_exists(PyObject *self, PyObject *args); // checks if handle valid
static PyObject* game_get_entity_name_by_handle(PyObject *self, PyObject *args);
static PyObject* game_get_entity_classname_by_handle(PyObject *self, PyObject *args);
static PyObject* game_create_entity_by_classname(PyObject *self, PyObject *args);
static PyObject* game_get_player(PyObject *self, PyObject *Py_UNUSED(args));

// game module - gives access to methods available across games
static PyMethodDef GameMethods[] = {
	{"print", game_print, METH_VARARGS, "Print text to the game console."},
	{"send_to_console", game_execute_console, METH_VARARGS, "Execute console command. (asynchronous)"},
	{"get_entity_list", game_entities, METH_VARARGS, "Get tuple with all entity handles currently existing."},
	{"entity_exists", game_entity_exists, METH_VARARGS, "Returns true if there is a Entity referenced by number else False."},
	{"entity_name", game_get_entity_name_by_handle, METH_VARARGS, "Returns name of the entity or None if not available. Crashes game if entity isn't valid."},
	{"entity_class", game_get_entity_classname_by_handle, METH_VARARGS, "Returns classname of the entity or None if not available. Crashes game if entity isn't valid."},
	{"entity_by_classname", game_create_entity_by_classname, METH_VARARGS, "Creates entity from classname and returns the handle."},
	{"tmp", game_get_player, METH_VARARGS, "Creates entity from classname and returns the handle."},
	{nullptr, nullptr, 0, nullptr}
};
// todo: remove old methods
static PyModuleDef GameModule = {
	PyModuleDef_HEAD_INIT, "game", nullptr, -1, GameMethods,
	nullptr, nullptr, nullptr, nullptr
};

PyObject* PyInit_game(void) {
	PyObject *gameModule;

	if(PyType_Ready(&EntityType) < 0) return nullptr;

	gameModule = PyModule_Create(&GameModule);
	if(gameModule == nullptr) return nullptr;

	Py_INCREF(&EntityType);
	if (PyModule_AddObject(gameModule, "Custom", (PyObject *) &EntityType) < 0) {
		Py_DECREF(&EntityType);
		Py_DECREF(gameModule);
		return NULL;
	}

	return gameModule;
}

static PyObject* game_get_player(PyObject *self, PyObject *Py_UNUSED(args)) {
	for (auto index = 1; index <= Offsets::NUM_ENT_ENTRIES; ++index) {
		auto info = entityList->GetEntityInfoByIndex(index);
		if (info->m_pEntity == nullptr) {
			continue;
		}
		// save entity pointer
		auto p = (Entity *) Custom_new(&EntityType, nullptr, nullptr);
		p->entity = info->m_pEntity;
		return (PyObject *) p;
	}
	return Py_None;
}

// game functions
static PyObject* game_print(PyObject *self, PyObject *args) {
	const char *printText;
	if (!PyArg_ParseTuple(args, "s", &printText)) {
		console->DevMsg("An error occurred while trying to parse print input!\n");
		return nullptr;
	}
	console->Print(printText);
	console->Print("\n");
	return Py_None;
}

static PyObject* game_execute_console(PyObject *self, PyObject *args) {
	const char *commandString;
	if (!PyArg_ParseTuple(args, "s", &commandString)) {
		console->DevMsg("An error occurred while trying to parse console input!\n");
		return nullptr;
	}
	engine->ExecuteCommand(commandString, true);
	return Py_None;
}

// return list with handles (pointers) of all entities in the game
static PyObject* game_entities(PyObject *self, PyObject *Py_UNUSED(args)) {
	std::uintptr_t pointers[Offsets::NUM_ENT_ENTRIES];
	int numEntities = 0;

	// iterate over entities
	for (auto index = 1; index <= Offsets::NUM_ENT_ENTRIES; ++index) {
		auto info = entityList->GetEntityInfoByIndex(index);
		if (info->m_pEntity == nullptr) {
			continue;
		}
		// save entity pointer
		pointers[numEntities] = reinterpret_cast<std::uintptr_t>(info->m_pEntity);
		if (pointers[numEntities] > 0)
		numEntities++;
	}

	// object creation
	PyObject *handleList = PyTuple_New(numEntities);
	for (auto index = 0; index < numEntities; ++index) {
		PyObject *pointerObject = PyLong_FromUnsignedLong(pointers[index]);
		PyTuple_SetItem(handleList, index, pointerObject);
	}

	return handleList;
}

// checks if handle valid
static PyObject* game_entity_exists(PyObject *self, PyObject *args) {
	const std::uintptr_t *entityHandle;
	if (!PyArg_ParseTuple(args, "k", &entityHandle)) {
		console->DevMsg("An error occurred while trying to parse print input!\n");
		return nullptr;
	}

	for (auto index = 0; index < Offsets::NUM_ENT_ENTRIES; ++index) {
		auto info = entityList->GetEntityInfoByIndex(index);
		if(info->m_pEntity == entityHandle) {
			return Py_True;
		}
	}
	return Py_False;

}

static PyObject* game_get_entity_name_by_handle(PyObject *self, PyObject *args) {
	const std::uintptr_t *entityHandle;
	if (!PyArg_ParseTuple(args, "k", &entityHandle)) {
		console->DevMsg("An error occurred while trying to parse input!\n");
		return nullptr;
	}

	char *entName = server->GetEntityName((void *) entityHandle);
	if(entName == nullptr) {
		console->DevMsg("Couldn't find entity name!\n");
		return Py_None;
	}

	PyObject *entNameObject = PyUnicode_FromString(entName);
	return entNameObject;
}

static PyObject* game_get_entity_classname_by_handle(PyObject *self, PyObject *args) {
	const std::uintptr_t *entityHandle;
	if (!PyArg_ParseTuple(args, "k", &entityHandle)) {
		console->DevMsg("An error occurred while trying to parse input!\n");
		return nullptr;
	}

	char *entClass = server->GetEntityClassName((void *) entityHandle);

	if(entClass == nullptr) {
		console->DevMsg("Couldn't find entity class!\n");
		return Py_None;
	}

	PyObject *entNameObject = PyUnicode_FromString(entClass);
	return entNameObject;
}

static PyObject* game_create_entity_by_classname(PyObject *self, PyObject *args) {
	// get args
	const char *entityClassName;
	if (!PyArg_ParseTuple(args, "s", &entityClassName)) {
		console->DevMsg("An error occurred while trying to parse console input!\n");
		return nullptr;
	}

	// create entity from classname
	void* entity = server->CreateEntityByName(nullptr, entityClassName);
	auto p = (Entity *) Custom_new(&EntityType, nullptr, nullptr);
	p->entity = entity;
	return (PyObject *) p;
	if (entity == nullptr) {
		console->DevMsg("CreateEntityByName() failed!\n");
		return nullptr;
	}


	// return entity string
	PyObject *entityObject = PyLong_FromVoidPtr(entity);
	return entityObject;
}
