/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        mods/shared_logic/logic/lua/CLuaPhysicsCapsuleShape.h
 *  PURPOSE:     Lua physics shape class
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/

class CLuaPhysicsCapsuleShape;

#pragma once

// Define includes
#include "../LuaCommon.h"
#include "../CLuaArguments.h"

class CLuaPhysicsCapsuleShape : public CLuaPhysicsShape
{
public:
    CLuaPhysicsCapsuleShape(CClientPhysics* pPhysics, float fRadius, float fHeight);
    ~CLuaPhysicsCapsuleShape();

    bool SetRadius(float fRadius);
    bool GetRadius(float& fRadius);
    bool SetHeight(float fHeight);
    bool GetHeight(float& fHeight);
    void Update() {}
};
