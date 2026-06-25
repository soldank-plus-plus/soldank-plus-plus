module;

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#include "httplib.h"

export module Extern.Httplib;

export namespace Httplib
{
using Client = httplib::Client;
constexpr auto to_string = httplib::to_string;
} // namespace Httplib
