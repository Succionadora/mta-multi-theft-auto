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
    virtual bool SetPresenceDetails(const char* szDetails, bool bCustom = false) = 0;
    virtual bool SetApplicationID(const char* szAppID) = 0;
    virtual bool ResetDiscordData() = 0;
    virtual bool SetPresenceState(const char* szState, bool bCustom) = 0;
    virtual void SetAssetLargeData(const char* szAsset, const char* szAssetText) = 0;
    virtual void SetAssetSmallData(const char* szAsset, const char* szAssetText) = 0;
    virtual void SetPresenceStartTimestamp(const unsigned long ulStart) = 0;
    virtual void SetPresenceEndTimestamp(const unsigned long ulEnd) = 0;
    virtual bool SetPresenceButtons(unsigned short int iIndex, const char* szName, const char* szUrl) = 0;
    virtual void SetPresencePartySize(int iSize, int iMax) = 0;
    //virtual void SetPresenceEndTimestamp(const unsigned long ulEnd) = 0;

    virtual bool SetDiscordRPCEnabled(bool bEnabled = false) = 0;
    virtual bool IsDiscordRPCEnabled() const = 0;
    virtual bool IsDiscordCustomDetailsDisallowed() const = 0;
};
