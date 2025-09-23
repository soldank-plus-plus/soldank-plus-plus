module;

#ifdef _WIN32
// Need to include winsock2 for windows because of this error:
// error C2375: 'shutdown': redefinition; different linkage
// Somehow if not included before SimpleIni, it doesn't compile lol
// TODO: This was moved from server/Application so check if it's still needed
#include <winsock2.h>
#endif

#include <SimpleIni.h>

export module Extern.SimpleIni;

export namespace SimpleIni
{
using CSimpleIniA = CSimpleIniA;
using SI_Error = SI_Error;
} // namespace SimpleIni
