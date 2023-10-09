/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        mods/deathmatch/logic/CClientModelManager.h
 *  PURPOSE:     Model manager class
 *
 *  Multi Theft Auto is available from https://multitheftauto.com/
 *
 *****************************************************************************/

class CClientModelManager;

#pragma once

#include <list>
#include <vector>
#include <memory>
#include "CClientModel.h"

#define MAX_MODEL_DFF_ID 20000
#define MAX_MODEL_TXD_ID 25000
#define MAX_MODEL_ID     25000

class CClientModelManager
{
    friend class CClientModel;

public:
    CClientModelManager::CClientModelManager();
    ~CClientModelManager(void);

    void RemoveAll(void);

    void Add(const std::shared_ptr<CClientModel>& pModel);
    bool Remove(const int modelId);
    bool RemoveClientModel(const int modelId);

    int GetFirstFreeModelID(void);
    int GetFreeTxdModelID();

    std::shared_ptr<CClientModel> FindModelByID(int iModelID);
    std::shared_ptr<CClientModel> Request(CClientManager* pManager, int iModelID, eClientModelType eType);

    std::vector<std::shared_ptr<CClientModel>> GetModelsByType(eModelInfoType type, const unsigned int minModelID = 0);

    void DeallocateModelsAllocatedByResource(CResource* pResource);
    bool AllocateModelFromParent(uint32_t usModelID, uint32_t usParentModel);

private:
    std::unique_ptr<std::shared_ptr<CClientModel>[]> m_Models;
    unsigned int                                     m_modelCount = 0;
};
