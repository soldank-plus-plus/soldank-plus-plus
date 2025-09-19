#include "scripting/dascript/DaScriptInit.hpp"

#include "daScript/daScript.h"

using namespace das;

// This cannot be a module because the macro NEED_MODULE(Module_BuiltIn) doesn't work with C++
// modules
bool InitDaScriptModule()
{
    // request all da-script built in modules
    NEED_MODULE(Module_BuiltIn);
    // Initialize modules
    das::Module::Initialize();

    return true;
}

void ShutdownDaScriptModule()
{
    // shut-down daScript, free all memory
    das::Module::Shutdown();
}
