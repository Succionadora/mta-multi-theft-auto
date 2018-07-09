/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        game_sa/CAnimBlendAssociationSA.h
 *  PURPOSE:     Header file for animation blend association class
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/

/*CAnimBlendAssociation: 1 per running animation?*/

#ifndef __CAnimBlendAssociationSA_H
#define __CAnimBlendAssociationSA_H

#include <game/CAnimBlendAssociation.h>
#include "Common.h"

class CAnimBlendAssocGroupSA;
class CAnimBlendHierarchySAInterface;

class CAnimBlendAssociationSAInterface
{
public:
    BYTE                            pad[14];                        // 0
    short                           sAnimGroup;                     // 14
    DWORD*                          pAnimBlendNodeArray;            // CAnimBlendNode pp?          // 16
    CAnimBlendHierarchySAInterface* pAnimHierarchy;                 // 20
    float                           fBlendAmount;                   // 24
    float                           fBlendDelta;                    // 28
    float                           fCurrentTime;                   // 32
    float                           fSpeed;                         // 36
    float                           fTimeStep;                      // 40
    short                           sAnimID;                        // 44
    short                           sFlags;                         // or1 = started?, or64 = referenced?   // 46
    DWORD*                          pCallback;                      // 48
    DWORD*                          pCallbackFunc;                  // 52
    DWORD*                          pCallbackData;                  // 56
                                                                    // Total: 60 bytes
};

class CAnimBlendAssociationSA : public CAnimBlendAssociation
{
public:
    CAnimBlendAssociationSA(CAnimBlendAssociationSAInterface* pInterface) { m_pInterface = pInterface; }

    CAnimBlendAssociationSAInterface*    Constructor(CAnimBlendStaticAssociationSAInterface& StaticAssociationByReference);
    CAnimBlendAssociationSAInterface*    Constructor(RpClump* pClump, CAnimBlendHierarchySAInterface* pAnimHierarchy);
    CAnimBlendAssociationSAInterface*    GetInterface() { return m_pInterface; }
    AssocGroupId                         GetAnimGroup() { return (AssocGroupId)m_pInterface->sAnimGroup; }
    AnimationId                          GetAnimID() { return (AnimationId)m_pInterface->sAnimID; }
    std::unique_ptr<CAnimBlendHierarchy> GetAnimHierarchy();

    float GetBlendAmount() { return m_pInterface->fBlendAmount; }
    void  SetBlendAmount(float fAmount) { m_pInterface->fBlendAmount = fAmount; }
    void  SetCurrentProgress(float fProgress);
    void  SetAnimID(short sAnimID) { m_pInterface->sAnimID = sAnimID; }
    void  SetAnimGroup(short sAnimGroup) { m_pInterface->sAnimGroup = sAnimGroup; }
    void  SetFlags(short sFlags) { m_pInterface->sFlags = sFlags; }

protected:
    CAnimBlendAssociationSAInterface* m_pInterface;
};

#endif
