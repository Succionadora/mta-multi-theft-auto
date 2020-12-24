/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        mods/shared_logic/logic/lua/CLuaPhysicsStaticCollision.h
 *  PURPOSE:     Static collision class
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/

class CLuaPhysicsElement;
class CLuaPhysicsStaticCollision;

#pragma once

#include "lua/physics/CPhysicsStaticCollisionProxy.h"

class CLuaPhysicsStaticCollision : public CLuaPhysicsWorldElement
{
public:
    CLuaPhysicsStaticCollision(std::shared_ptr<CLuaPhysicsShape> pShape);
    ~CLuaPhysicsStaticCollision();

    void    Initialize(std::shared_ptr<CLuaPhysicsStaticCollision> pStaticCollision);

    void    SetPosition(const CVector& vecPosition);
    const CVector GetPosition() const;
    void    SetRotation(const CVector& vecRotation);
    const CVector GetRotation() const;
    void          SetScale(const CVector& vecScale);
    const CVector GetScale() const;
    void    SetMatrix(const CMatrix& matrix);
    const CMatrix GetMatrix() const;

    void          RemoveDebugColor();
    void          SetDebugColor(const SColor& color);
    const SColor GetDebugColor() const;

    int  GetFilterGroup() const;
    void SetFilterGroup(int iGroup);
    int  GetFilterMask() const;
    void SetFilterMask(int mask);

    void Unlink();

    CPhysicsStaticCollisionProxy* GetCollisionObject() const { return m_btCollisionObject.get(); }

    void Update() {}
private:
    std::unique_ptr<CPhysicsStaticCollisionProxy> m_btCollisionObject;
    std::shared_ptr<CLuaPhysicsShape>  m_pShape;

    mutable std::mutex m_lock;
};
