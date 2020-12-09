/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        mods/shared_logic/logic/lua/CLuaPhysicsConeShape.cpp
 *  PURPOSE:     Lua physics cone shape class
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/

#include <StdInc.h>
#include "CLuaPhysicsSharedLogic.h"
#include "CLuaPhysicsShape.h"
#include "CLuaPhysicsConeShape.h"

CLuaPhysicsConeShape::CLuaPhysicsConeShape(CClientPhysics* pPhysics, float fRadius, float fHeight) : CLuaPhysicsShape(pPhysics, std::move(CLuaPhysicsSharedLogic::CreateCone(fRadius, fHeight)))
{
}

CLuaPhysicsConeShape::~CLuaPhysicsConeShape()
{

}

bool CLuaPhysicsConeShape::SetRadius(float fRadius)
{
    ((btConeShape*)GetBtShape())->setRadius(fRadius);
    UpdateRigids();
    return true;
}

bool CLuaPhysicsConeShape::GetRadius(float& fRadius)
{
    fRadius = ((btConeShape*)GetBtShape())->getRadius();
    return true;
}

bool CLuaPhysicsConeShape::SetHeight(float fHeight)
{
    btConeShape* pCone = (btConeShape*)GetBtShape();
    pCone->setHeight(fHeight);
    return true;
}

bool CLuaPhysicsConeShape::GetHeight(float& fHeight)
{
    btConeShape* pCone = (btConeShape*)GetBtShape();
    fHeight = pCone->getHeight();
    return true;
}
