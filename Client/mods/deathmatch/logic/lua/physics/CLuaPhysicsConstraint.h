/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        mods/shared_logic/logic/lua/CLuaPhysicsConstraint.h
 *  PURPOSE:     Physics constraint class
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/

class CLuaPhysicsConstraint;

#pragma once

// Define includes
#include "../LuaCommon.h"
#include "../CLuaArguments.h"

enum ePhysicsConstraint;

class CLuaPhysicsConstraint : public CLuaPhysicsElement
{
public:
    CLuaPhysicsConstraint(CClientPhysics* pPhysics, CLuaPhysicsRigidBody* pRigidBody, bool bDisableCollisionsBetweenLinkedBodies = true);
    CLuaPhysicsConstraint(CClientPhysics* pPhysics, CLuaPhysicsRigidBody* pRigidBodyA, CLuaPhysicsRigidBody* pRigidBodyB,
                          bool bDisableCollisionsBetweenLinkedBodies = true);
    ~CLuaPhysicsConstraint();

    void  SetBreakingImpulseThreshold(float fThreshold);
    float GetBreakingImpulseThreshold();
    float GetAppliedImpulse();

    bool IsBroken() const { return !m_pConstraint->isEnabled(); }
    bool BreakingStatusHasChanged();

    btTypedConstraint* GetConstraint() const { return m_pConstraint; }
    btJointFeedback*   GetJoinFeedback() { return m_pJointFeedback.get(); }

    virtual void Initialize(){};
    virtual void Unlink();

protected:
    virtual void InternalInitialize(btTypedConstraint* pConstraint);
    bool                               m_bDisableCollisionsBetweenLinkedBodies;
    uint                               m_uiScriptID;
    btTypedConstraint* m_pConstraint;
    std::unique_ptr<btJointFeedback>   m_pJointFeedback;
    bool                               m_bLastBreakingStatus;
    CLuaPhysicsRigidBody*              m_pRigidBodyA;
    CLuaPhysicsRigidBody*              m_pRigidBodyB;
};
