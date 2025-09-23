module;

#include "httplib.h"

export module Extern.Httplib;

export namespace Httplib
{
using Client = httplib::Client;
constexpr auto to_string = httplib::to_string;
} // namespace Httplib
