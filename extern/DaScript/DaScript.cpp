module;

#include "DaScriptInit.hpp"

#include "daScript/daScript.h"

export module Extern.DaScript;

export namespace DaScript
{
using Context = das::Context;
using TextPrinter = das::TextPrinter;
using ModuleGroup = das::ModuleGroup;
using FsFileAccess = das::FsFileAccess;
using TextFileInfo = das::TextFileInfo;

using Module = das::Module;

template<class T, class... Args>
constexpr auto make_smart = das::make_smart<T, Args...>;

constexpr auto compileDaScript(const das::string& fileName,
                               const das::FileAccessPtr& access,
                               das::TextWriter& logs,
                               das::ModuleGroup& libGroup,
                               das::CodeOfPolicies policies = das::CodeOfPolicies())
{
    return das::compileDaScript(fileName, access, logs, libGroup, policies);
}

constexpr auto NeedModuleBuiltIn = InitNeedModuleBuiltIn;
constexpr auto NeedModuleMath = InitNeedModuleMath;
constexpr auto NeedModuleRaster = InitNeedModuleRaster;
constexpr auto NeedModuleStrings = InitNeedModuleStrings;
constexpr auto NeedModuleRtti = InitNeedModuleRtti;
constexpr auto NeedModuleAst = InitNeedModuleAst;
constexpr auto NeedModuleDebugger = InitNeedModuleDebugger;
constexpr auto NeedModuleJit = InitNeedModuleJit;
constexpr auto NeedModuleFIO = InitNeedModuleFIO;
constexpr auto NeedModuleDASBIND = InitNeedModuleDASBIND;
constexpr auto NeedModuleNETWORK = InitNeedModuleNETWORK;
constexpr auto NeedAllDefaultModules = InitNeedAllDefaultModules;
} // namespace DaScript
