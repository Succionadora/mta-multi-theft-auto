/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *               (Shared logic for modifications)
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        sdk/core/CDiscordInterface.h
 *  PURPOSE:     Discord interface class
 *
 *****************************************************************************/

#pragma once

class CDiscordInterface
{
public:
    virtual ~CDiscordInterface() = default;
    virtual void UpdatePresence() = 0;
    virtual void SetPresenceState(const char* szState) = 0;
    virtual bool SetPresenceDetails(const char* szDetails, bool bCustom = false) = 0;
    virtual void SetPresenceStartTimestamp(const unsigned long ulStart) = 0;
    //virtual void SetPresenceEndTimestamp(const unsigned long ulEnd) = 0;

    virtual bool SetDiscordRPCEnabled(bool bEnabled = false) = 0;
    virtual bool IsDiscordRPCEnabled() = 0;
};
