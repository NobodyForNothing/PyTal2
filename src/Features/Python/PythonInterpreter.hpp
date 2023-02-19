#pragma once

#include "Command.hpp"
#include "Features/Feature.hpp"

class PythonInterpreter : public Feature {
public:
	PythonInterpreter();
	static bool ExecuteFile(const char *filePath);
};
std::string GetPythonScriptsDirectory();
std::string GetPythonPluginLoadFile();
std::string GetPythonMapsspawnFile();

extern PythonInterpreter *pythonInterpreter;
extern Command pytal_python_run_script;
extern Command pytal_python_run;
