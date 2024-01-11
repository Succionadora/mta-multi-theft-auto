/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        core/CDynamicLibrary.h
 *  PURPOSE:     Dynamic library handling class
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/

#pragma once

#ifdef WIN32
#include <windows.h>
#else
using HMODULE = void*;
#endif

using FuncPtr_t = void(*)();

class CDynamicLibrary
{
public:
    CDynamicLibrary();
    ~CDynamicLibrary();

    bool Load(const char* szFilename);
    void Unload();
    bool IsLoaded();

    FuncPtr_t GetProcedureAddress(const char* szProcName);
    bool      CheckMtaVersion(const char* szLibName);

private:
    HMODULE m_hModule;
};
