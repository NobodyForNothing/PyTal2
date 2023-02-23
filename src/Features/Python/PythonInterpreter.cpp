#include "PythonInterpreter.hpp"

#define PY_SSIZE_T_CLEAN
#include "Command.hpp"
#include "Modules/Engine.hpp"
#include "PythonExtension.hpp"
#include "PytalMain.hpp"

#include <filesystem>
#include <fstream>
#include <Python.h>
#include <string>


PythonInterpreter *pythonInterpreter;

PythonInterpreter::PythonInterpreter() {
	// initialize provided modules
	PyImport_AppendInittab("game", &PyInit_game);

	// initialize the python instance
	Py_Initialize();

	// create files if needed
	std::filesystem::create_directory(GetPythonScriptsDirectory());
	if(!std::filesystem::exists(GetPythonPluginLoadFile())) {
		std::ofstream LoadFile(GetPythonPluginLoadFile());
		LoadFile << "# ********************************************************************************************\n";
		LoadFile << "# pluginload.py is called on plugin load (usually on game start)\n";
		LoadFile << "# ********************************************************************************************\n";
		LoadFile.close();
		// TODO: implement call
	}
	if(!std::filesystem::exists(GetPythonMapsspawnFile())) {
			std::ofstream MapsspawnFile(GetPythonMapsspawnFile());
			MapsspawnFile << "# ********************************************************************************************\n";
			MapsspawnFile << "# mapsspawn.py is called on newgame or transitions\n";
			MapsspawnFile << "# ********************************************************************************************\n";
			MapsspawnFile.close();
	}

	// Execute load file
	PythonInterpreter::ExecuteFile(GetPythonPluginLoadFile().c_str());
	
}

// runs the python Interpreter on a file, specified in an absolute location
bool PythonInterpreter::ExecuteFile(const char *filePath) {
	bool success = false;

	if(!std::filesystem::exists(filePath)){
		// no cleanup needed
		return success;
	}

	FILE *scriptFile = fopen(filePath, "r");
	const char *fileName = std::filesystem::path(filePath).filename().c_str();

	if(!scriptFile) {
		console->Print("Failed to read File at %s\n");
		goto error;
	}

	if(PyRun_SimpleFile(scriptFile, fileName) == -1) {
		console->Print("An error occurred while running the script!\n");
		goto error;
	}
	success = true;

	error:
		/* cleanup code */
		fclose(scriptFile);
		return success;
}

// folder where scripts are stored. usually "../Portal 2/portal2/scripts/python"
std::string GetPythonScriptsDirectory() {
	return std::string(engine->GetGameDirectory()) + "/scripts/python";
}
// Location of file called on plugin load. usually "pluginLoad.py"
std::string GetPythonPluginLoadFile() {
	return GetPythonScriptsDirectory() + "/pluginLoad.py";
}
// Location of file called on newgame or transitions usually "mapsspawn.py"
std::string GetPythonMapsspawnFile() {
	return GetPythonScriptsDirectory() + "/mapsspawn.py";
}

// register commands
DECL_COMMAND_FILE_COMPLETION(pytal_python_run_script, ".py", GetPythonScriptsDirectory(), 1);
CON_COMMAND_F_COMPLETION(pytal_python_run_script, "pytal_python_run_script <file_name> - Runs a python script file\n", 0, AUTOCOMPLETION_FUNCTION(pytal_python_run_script)) {
	if (args.ArgC() != 2) {
		return console->Print("Please enter a String!\n");
	}
	std::string filePath = GetPythonScriptsDirectory() + "/" +const_cast<char *>(args[1]) + ".py";
	console->Print(("Running script at: " + filePath + "\n").c_str()); // TODO: Variable to toggle log
	pythonInterpreter->ExecuteFile(filePath.c_str());
}

CON_COMMAND(pytal_python_run, "pytal_python_run - Runs a python script String\n") {
	if (args.ArgC() != 2) {
		return console->Print("Please enter one String!\n");
	}
	if(PyRun_SimpleString(args[1]) == -1) {
		console->Print("An error occurred while running the String!\n");
	}
}

