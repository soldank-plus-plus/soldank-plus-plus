module;

#include "daScript/daScript.h"

#include "spdlog/spdlog.h"

export module Scripting.DaScript;

import Scripting.ScriptingEngine;

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
        auto f_access = das::make_smart<das::FsFileAccess>();
        auto file_info = std::make_unique<das::TextFileInfo>(
          TUTORIAL_TEXT, uint32_t(strlen(TUTORIAL_TEXT)), false);
        f_access->setFileInfo("dummy.das", das::move(file_info));
        // compile script
        das::TextPrinter tout;
        das::ModuleGroup dummy_lib_group;
        auto program = compileDaScript("dummy.das", f_access, tout, dummy_lib_group);
        if (program->failed()) {
            spdlog::error("daScript compilation failed");
        }
        // create context
        das::Context ctx(program->getContextStackSize());
        if (!program->simulate(ctx, tout)) {
            spdlog::error("daScript context creation failed");
        }
        // find function. its up to application to check, if function is not null
        auto* function = ctx.findFunction("main");
        if (function == nullptr) {
            spdlog::error("daScript could not find funtion main");
        }
        // call context function
        ctx.evalWithCatch(function, nullptr);
    }
};
} // namespace Soldank
