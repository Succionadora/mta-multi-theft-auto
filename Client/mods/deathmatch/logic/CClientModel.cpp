/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto
 *               (Shared logic for modifications)
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        mods/deathmatch/logic/CClientModel.h
 *  PURPOSE:     Model handling class
 *
 *****************************************************************************/

#include "StdInc.h"

CClientModel::CClientModel(CClientManager* pManager, int iModelID, eClientModelType eModelType)
{
    // Init
    m_pManager = pManager;
    m_pModelManager = pManager->GetModelManager();
    m_iModelID = iModelID;
    m_eModelType = eModelType;

    m_pModelManager->Add(this);
}

CClientModel::~CClientModel(void)
{
    Deallocate();

    m_pModelManager->Remove(this);
}

bool CClientModel::Allocate(void)
{
    m_bAllocatedByUs = true;

    CModelInfo* pModelInfo = g_pGame->GetModelInfo(m_iModelID, true);

    // Allocate only on free IDs
    if (!pModelInfo->IsValid())
    {
        switch (m_eModelType)
        {
            case CCLIENTMODELPED:
                pModelInfo->MakePedModel("PSYCHO");
                break;
            case CCLIENTMODELOBJECT:
                pModelInfo->MakeObjectModel(1337);
                break;
            default:
                return false;
        }
        return true;
    }
    return false;
}

bool CClientModel::Deallocate(void)
{
    if (!m_bAllocatedByUs)
        return false;

    CModelInfo* pModelInfo = g_pGame->GetModelInfo(m_iModelID, true);

    // ModelInfo must be valid
    if (pModelInfo->IsValid())
    {
        if (m_eModelType == CCLIENTMODELPED)
        {
            // If some ped is using this ID, change him to CJ
            CClientPedManager*                       pPedManager = g_pClientGame->GetManager()->GetPedManager();
            std::vector<CClientPed*>::const_iterator iter = pPedManager->IterBegin();
            for (; iter != pPedManager->IterEnd(); iter++)
            {
                if ((*iter)->GetModel() == m_iModelID)
                {
                    if ((*iter)->IsStreamedIn())
                    {
                        (*iter)->StreamOutForABit();
                    }
                    (*iter)->SetModel(0);
                }
            }
        }
        else if (m_eModelType == CCLIENTMODELOBJECT) {
            CClientObjectManager* pObjectManager = g_pClientGame->GetManager()->GetObjectManager();
            std::vector<CClientObject*>::const_iterator iter = pObjectManager->IterBegin();
            for (; iter != pObjectManager->IterEnd(); iter++)
            {
                if ((*iter)->GetModel() == m_iModelID)
                {
                    if ((*iter)->IsStreamedIn())
                    {
                        (*iter)->StreamOutForABit();
                    }
                    (*iter)->SetModel(1337);
                }
            }
        }

        // Restore DFF/TXD
        g_pClientGame->GetManager()->GetDFFManager()->RestoreModel(m_iModelID);

        // Restore COL (for non ped models)
        if (m_eModelType != CCLIENTMODELPED)
        {
            g_pClientGame->GetManager()->GetColModelManager()->RestoreModel(m_iModelID);
        }

        pModelInfo->DeallocateModel();

        this->SetParentResource(nullptr);
        return true;
    }
    return false;
}
