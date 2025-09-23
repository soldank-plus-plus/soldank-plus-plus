#include "DaScriptInit.hpp"

#include "daScript/daScript.h"

void InitNeedModuleBuiltIn()
{
    NEED_MODULE(Module_BuiltIn);
}

void InitNeedModuleMath()
{
    NEED_MODULE(Module_Math);
}
void InitNeedModuleRaster()
{
    NEED_MODULE(Module_Raster);
}
void InitNeedModuleStrings()
{
    NEED_MODULE(Module_Strings);
}
void InitNeedModuleRtti()
{
    NEED_MODULE(Module_Rtti);
}
void InitNeedModuleAst()
{
    NEED_MODULE(Module_Ast);
}
void InitNeedModuleDebugger()
{
    NEED_MODULE(Module_Debugger);
}
void InitNeedModuleJit()
{
    NEED_MODULE(Module_Jit);
}
void InitNeedModuleFIO()
{
    NEED_MODULE(Module_FIO);
}
void InitNeedModuleDASBIND()
{
    NEED_MODULE(Module_DASBIND);
}
void InitNeedModuleNETWORK()
{
    NEED_MODULE(Module_Network);
}
void InitNeedAllDefaultModules()
{
    NEED_ALL_DEFAULT_MODULES;
}
