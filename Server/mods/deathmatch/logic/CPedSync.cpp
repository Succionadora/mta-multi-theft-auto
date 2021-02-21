/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        mods/deathmatch/logic/CPedSync.cpp
 *  PURPOSE:     Ped entity synchronization class
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/

#include "StdInc.h"

CPedSync::CPedSync(CPlayerManager* pPlayerManager, CPedManager* pPedManager)
{
    m_pPlayerManager = pPlayerManager;
    m_pPedManager = pPedManager;
}

void CPedSync::DoPulse()
{
    // Time to update lists of players near peds?
    if (m_UpdateNearListTimer.Get() > 1000)
    {
        m_UpdateNearListTimer.Reset();
        UpdateNearPlayersList();
    }

    // Time to check for players that should no longer be syncing a ped or peds that should be synced?
    if (m_UpdateSyncerTimer.Get() > 500)
    {
        m_UpdateSyncerTimer.Reset();
        UpdateAllSyncer();
    }
}

bool CPedSync::ProcessPacket(CPacket& Packet)
{
    if (Packet.GetPacketID() == PACKET_ID_PED_SYNC)
    {
        Packet_PedSync(static_cast<CPedSyncPacket&>(Packet));
        return true;
    }

    return false;
}

void CPedSync::OverrideSyncer(CPed* pPed, CPlayer* pPlayer)
{
    CPlayer* pSyncer = pPed->GetSyncer();
    if (pSyncer)
    {
        if (pSyncer == pPlayer)
            return;

        StopSync(pPed);
    }

    if (pPlayer && !pPed->IsBeingDeleted())
        StartSync(pPlayer, pPed);
}

void CPedSync::UpdateAllSyncer()
{
    // Update all the ped's sync states
    for (auto iter = m_pPedManager->IterBegin(); iter != m_pPedManager->IterEnd(); iter++)
    {
        // It is a ped, yet not a player
        if (IS_PED(*iter) && !IS_PLAYER(*iter))
            UpdateSyncer(*iter);
    }
}

void CPedSync::UpdateSyncer(CPed* pPed)
{
    CPlayer* pSyncer = pPed->GetSyncer();

    // Handle no syncing
    if (!pPed->IsSyncable())
    {
        // This ped got a syncer?
        if (pSyncer)
        {
            // Tell the syncer to stop syncing
            StopSync(pPed);
        }
        return;
    }

    // This ped got a syncer?
    if (pSyncer)
    {
        // Is he close enough, and in the right dimension?
        if (IsPointNearPoint3D(pSyncer->GetPosition(), pPed->GetPosition(), (float)g_TickRateSettings.iPedSyncerDistance))
            if (pPed->GetDimension() == pSyncer->GetDimension())
                return;

        // Stop him from syncing it
        StopSync(pPed);
    }

    if (pPed->IsBeingDeleted())
        return;
            
    // Find a new syncer for it
    FindSyncer(pPed);
}

void CPedSync::FindSyncer(CPed* pPed)
{
    assert(pPed->IsSyncable());

    // Find a player close enough to him
    CPlayer* pPlayer = FindPlayerCloseToPed(pPed, g_TickRateSettings.iPedSyncerDistance - 20.0f);
    if (pPlayer)
    {
        // Tell him to start syncing it
        StartSync(pPlayer, pPed);
    }
}

void CPedSync::StartSync(CPlayer* pPlayer, CPed* pPed)
{
    if (!pPed->IsSyncable())
        return;

    // Tell the player
    pPlayer->Send(CPedStartSyncPacket(pPed));

    // Mark him as the syncing player
    pPed->SetSyncer(pPlayer);

    // Call the onElementStartSync event
    CLuaArguments Arguments;
    Arguments.PushElement(pPlayer);            // New syncer
    pPed->CallEvent("onElementStartSync", Arguments);
}

void CPedSync::StopSync(CPed* pPed)
{
    // Tell the player that used to sync it
    CPlayer* pSyncer = pPed->GetSyncer();
    pSyncer->Send(CPedStopSyncPacket(pPed->GetID()));

    // Unmark him as the syncing player
    pPed->SetSyncer(NULL);

    // Call the onElementStopSync event
    CLuaArguments Arguments;
    Arguments.PushElement(pSyncer);            // Old syncer
    pPed->CallEvent("onElementStopSync", Arguments);
}

CPlayer* CPedSync::FindPlayerCloseToPed(CPed* pPed, float fMaxDistance)
{
    // Grab the ped position
    CVector vecPedPosition = pPed->GetPosition();

    // See if any players are close enough
    CPlayer* pLastPlayerSyncing = nullptr;
    CPlayer* pPlayer = nullptr;
    for (auto iter = m_pPlayerManager->IterBegin(); iter != m_pPlayerManager->IterEnd(); iter++)
    {
        pPlayer = *iter;
        // Is he joined?
        if (!pPlayer->IsJoined())
            continue;

        // He's near enough?
        if (!IsPointNearPoint3D(vecPedPosition, pPlayer->GetPosition(), fMaxDistance))
            continue;

        // Same dimension?
        if (pPlayer->GetDimension() != pPed->GetDimension())
            continue;

        // He syncs less peds than the previous player close enough?
        if (!pLastPlayerSyncing || pPlayer->CountSyncingPeds() < pLastPlayerSyncing->CountSyncingPeds())
            pLastPlayerSyncing = pPlayer;
    }

    // Return the player we found that syncs the least number of peds
    return pLastPlayerSyncing;
}

void CPedSync::Packet_PedSync(CPedSyncPacket& Packet)
{
    // Grab the player
    CPlayer* pPlayer = Packet.GetSourcePlayer();
    if (!pPlayer || !pPlayer->IsJoined())
        return;

    // Grab the tick count
    long long llTickCountNow = GetModuleTickCount64();

    // Apply the data for each ped in the packet
    for (auto iter = Packet.IterBegin(); iter != Packet.IterEnd(); iter++)
    {
        CPedSyncPacket::SyncData* pData = *iter;

        // Grab the ped this packet is for
        CElement* pPedElement = CElementIDs::GetElement(pData->Model);
        if (!pPedElement || !IS_PED(pPedElement))
            continue;

        // Convert to a CPed
        CPed* pPed = static_cast<CPed*>(pPedElement);

        // Is the player syncing this ped?
        // Check if the time context matches.
        if (pPed->GetSyncer() != pPlayer || !pPed->CanUpdateSync(pData->ucSyncTimeContext))
            continue;

        // Apply the data to the ped
        if (pData->ucFlags & 0x01)
        {
            pPed->SetPosition(pData->vecPosition);
            g_pGame->GetColManager()->DoHitDetection(pPed->GetPosition(), pPed);
        }
        if (pData->ucFlags & 0x02)
            pPed->SetRotation(pData->fRotation);
        if (pData->ucFlags & 0x04)
            pPed->SetVelocity(pData->vecVelocity);

        if (pData->ucFlags & 0x08)
        {
            // Less health than last time?
            float fPreviousHealth = pPed->GetHealth();
            pPed->SetHealth(pData->fHealth);

            if (pData->fHealth < fPreviousHealth)
            {
                // Grab the delta health
                float fDeltaHealth = fPreviousHealth - pData->fHealth;

                if (fDeltaHealth > 0.0f)
                {
                    // Call the onPedDamage event
                    CLuaArguments Arguments;
                    Arguments.PushNumber(fDeltaHealth);
                    pPed->CallEvent("onPedDamage", Arguments);
                }
            }
        }

        if (pData->ucFlags & 0x10)
            pPed->SetArmor(pData->fArmor);

        if (pData->ucFlags & 0x20)
            pPed->SetOnFire(pData->bOnFire);

        if (pData->ucFlags & 0x40)
            pPed->SetInWater(pData->bIsInWater);

        // Is it time to sync to everyone
        bool bDoFarSync = llTickCountNow - pPed->GetLastFarSyncTick() >= g_TickRateSettings.iPedFarSync;

        if (!bDoFarSync && pPed->IsNearPlayersListEmpty())
            continue;

        // Create a new packet, containing only the struct for this ped
        CPedSyncPacket PedPacket(pData);
        if (!&PedPacket)
            continue;

        if (bDoFarSync)
        {
            // Store the tick
            pPed->SetLastFarSyncTick(llTickCountNow);
            // Send to everyone
            m_pPlayerManager->BroadcastOnlyJoined(PedPacket, pPlayer);
            continue;
        }

        // Send to players nearby the ped
        CSendList sendList;
        for (auto iter = pPed->NearPlayersIterBegin(); iter != pPed->NearPlayersIterEnd(); iter++)
        {
            CPlayer* pRemotePlayer = *iter;
            // If the syncer changes between UpdateNearPlayersList() he can be in the list, make sure we don't send to him
            if (pRemotePlayer && pRemotePlayer != pPlayer)
                sendList.push_back(pRemotePlayer);
        }

        if (!sendList.empty())
            m_pPlayerManager->Broadcast(PedPacket, sendList);
    }
}

void CPedSync::UpdateNearPlayersList()
{
    for (auto iter = m_pPedManager->IterBegin(); iter != m_pPedManager->IterEnd(); iter++)
    {
        CPed* pPed = *iter;
        // Clear the list
        pPed->ClearNearPlayersList();
    }

    for (auto iter = m_pPlayerManager->IterBegin(); iter != m_pPlayerManager->IterEnd(); iter++)
    {
        CPlayer* pPlayer = *iter;
        if (!pPlayer->IsJoined() || pPlayer->IsBeingDeleted())
            continue;

        // Grab the camera position
        CVector vecCameraPosition;
        pPlayer->GetCamera()->GetPosition(vecCameraPosition);

        // Do a query in the spatial database
        CElementResult resultNearCamera;
        GetSpatialDatabase()->SphereQuery(resultNearCamera, CSphere(vecCameraPosition, DISTANCE_FOR_NEAR_VIEWER));

        for (auto iter = resultNearCamera.begin(); iter != resultNearCamera.end(); ++iter)
        {
            // Make sure it's a ped
            if ((*iter)->GetType() != CElement::PED)
                continue;

            CPed* pPed = static_cast<CPed*>(*iter);

            // Check dimension matches
            if (pPlayer->GetDimension() != pPed->GetDimension())
                continue;

            // If the player is syncing it, don't add the player
            if (pPed->GetSyncer() == pPlayer)
                continue;

            // Check distance accurately because the spatial database is 2D
            if ((vecCameraPosition - pPed->GetPosition()).LengthSquared() < DISTANCE_FOR_NEAR_VIEWER * DISTANCE_FOR_NEAR_VIEWER)
                pPed->AddPlayerToNearList(pPlayer); 
        }
    }
}
