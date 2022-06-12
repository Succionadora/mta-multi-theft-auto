/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        mods/deathmatch/logic/models/CModelManager.h
 *  PURPOSE:     Model manager class
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/

class CModelManager;

#pragma once

#include "StdInc.h"
#include "CModelBase.h"
#include "CModelAtomic.h"
#include "CModelVehicle.h"

class CModelManager
{
public:
    CModelManager();
    ~CModelManager();

    // Register generic GTA:SA model info
    void RegisterModel(CModelBase* pModelHandler);
    bool AllocateModelFromParent(uint32_t uiNewModelID, uint32_t uiParentModel);
    bool UnloadCustomModel(uint32 uiModelID);

    std::vector<CModelBase*>& GetModels() { return m_vModels; };

    CModelVehicle* GetVehicleModel(uint32_t iModelID) { return dynamic_cast<CModelVehicle*>(m_vModels[iModelID]); };

    std::list<CModelBase*> GetSimpleAllocatedModels() { return m_vSimpleAllocatedModels; };

private:
    // modelID - CModelBase
    std::vector<CModelBase*> m_vModels;
    std::list<CModelBase*> m_vSimpleAllocatedModels;
};
