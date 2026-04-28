module;

#include <cstdint>
#include <memory>
#include <cstring>

export module Scripting.DaScript;

import Scripting.ScriptingEngine;

import Extern.DaScript;
import Extern.Spdlog;

constexpr const char* const TUTORIAL_TEXT = R""""(
[export]
def main
    print("daScript initialized\n")
)"""";

namespace Soldank
{
export class DaScriptScriptingEngine : public IScriptingEngine
{
public:
    DaScriptScriptingEngine()
    {
        // make file access, introduce string as if it was a file
        auto f_access = DaScript::make_smart<DaScript::FsFileAccess>();
        auto file_info = std::make_unique<DaScript::TextFileInfo>(
          TUTORIAL_TEXT, uint32_t(strlen(TUTORIAL_TEXT)), false);
        f_access->setFileInfo("dummy.das", std::move(file_info));
        // compile script
        DaScript::TextPrinter tout;
        DaScript::ModuleGroup dummy_lib_group;
        auto program = DaScript::compileDaScript("dummy.das", f_access, tout, dummy_lib_group);
        if (program->failed()) {
            Spdlog::error("daScript compilation failed");
        }
        // create context
        DaScript::Context ctx(program->getContextStackSize());
        if (!program->simulate(ctx, tout)) {
            Spdlog::error("daScript context creation failed");
        }
        // find function. its up to application to check, if function is not null
        auto* function = ctx.findFunction("main");
        if (function == nullptr) {
            Spdlog::error("daScript could not find funtion main");
        }
        // call context function
        ctx.evalWithCatch(function, nullptr);
    }

    static void Init()
    {
        DaScript::NeedModuleBuiltIn();
        DaScript::Module::Initialize();
    }

    static void Shutdown()
    {
        // shut-down daScript, free all memory
        DaScript::Module::Shutdown();
    }
};
} // namespace Soldank
