/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto
 *               (Shared logic for modifications)
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        mods/shared_logic/CClientPhysics.cpp
 *  PURPOSE:     Physics entity class
 *
 *****************************************************************************/

#include "StdInc.h"
#include <list>
#include "../Client/game_sa/CCameraSA.h"
#include "../mods/deathmatch/logic/Utils.h"
#include "lua/physics/CLuaPhysicsRigidBodyManager.h"
#include "lua/physics/CLuaPhysicsStaticCollisionManager.h"
#include "lua/physics/CLuaPhysicsConstraintManager.h"
#include "lua/physics/CLuaPhysicsShapeManager.h"
#include "CPhysicsDebugDrawer.h"
#include "lua/physics/CPhysicsStaticCollisionProxy.h"

CClientPhysics::CClientPhysics(CClientManager* pManager, ElementID ID, CLuaMain* luaMain) : ClassInit(this), CClientEntity(ID)
{
    // Init
    m_pManager = pManager;
    m_pPhysicsManager = pManager->GetPhysicsManager();
    m_pLuaMain = luaMain;
    m_bBuildWorld = false;

    SetTypeName("physics");

    m_pOverlappingPairCache = std::make_unique<btDbvtBroadphase>();
    m_pCollisionConfiguration = std::make_unique<btDefaultCollisionConfiguration>();
    m_pDispatcher = std::make_unique<btCollisionDispatcher>(m_pCollisionConfiguration.get());
    m_pSolver = std::make_unique<btSequentialImpulseConstraintSolver>();
    m_pDebugDrawer = std::make_unique<CPhysicsDebugDrawer>();
    m_pDebugDrawer->setDebugMode(btIDebugDraw::DBG_DrawWireframe);
    {
        std::lock_guard guard(dynamicsWorldLock);
        m_pDynamicsWorld =
            std::make_unique<btDiscreteDynamicsWorld>(m_pDispatcher.get(), m_pOverlappingPairCache.get(), m_pSolver.get(), m_pCollisionConfiguration.get());
        m_pDynamicsWorld->setGravity(reinterpret_cast<const btVector3&>(BulletPhysics::Defaults::Gravity));
        m_pDynamicsWorld->setDebugDrawer(m_pDebugDrawer.get());
    }

    // Add us to Physics manager's list
    m_pPhysicsManager->AddToList(this);
}

CClientPhysics::~CClientPhysics()
{
    WaitForSimulationToFinish();
    Clear();
}

void CClientPhysics::WaitForSimulationToFinish()
{
    while (isDuringSimulation)
        Sleep(1);
}

void CClientPhysics::Clear()
{
    for (const auto& pair : m_mapConstraints)
    {
        pair.second->Unlink();
    }

    for (const auto& pRigidBody : m_mapRigidBodies)
    {
        pRigidBody.second->Unlink();
    }
    for (const auto& pStaticCollision : m_mapStaticCollisions)
    {
        pStaticCollision.second->Unlink();
    }
    m_mapConstraints.clear();
    m_mapRigidBodies.clear();
    m_mapStaticCollisions.clear();
    m_mapShapes.clear();
}

void CClientPhysics::Unlink()
{
    m_pPhysicsManager->RemoveFromList(this);
}

void CClientPhysics::AddStaticCollision(btCollisionObject* pBtCollisionObject) const
{
    std::lock_guard guard(dynamicsWorldLock);

    m_pDynamicsWorld->addCollisionObject(pBtCollisionObject);
}

void CClientPhysics::RemoveStaticCollision(btCollisionObject* pBtCollisionObject) const
{
    std::lock_guard guard(dynamicsWorldLock);

    m_pDynamicsWorld->removeCollisionObject(pBtCollisionObject);
}

void CClientPhysics::AddRigidBody(btRigidBody* pBtRigidBody) const
{
    std::lock_guard guard(dynamicsWorldLock);

    m_pDynamicsWorld->addRigidBody(pBtRigidBody);
}

void CClientPhysics::RemoveRigidBody(btRigidBody* pBtRigidBody) const
{
    std::lock_guard guard(dynamicsWorldLock);

    m_pDynamicsWorld->removeRigidBody(pBtRigidBody);
}

void CClientPhysics::AddConstraint(btTypedConstraint* pBtTypedConstraint, bool bDisableCollisionsBetweenLinkedBodies) const
{
    std::lock_guard guard(dynamicsWorldLock);

    m_pDynamicsWorld->addConstraint(pBtTypedConstraint, bDisableCollisionsBetweenLinkedBodies);
}

void CClientPhysics::RemoveConstraint(btTypedConstraint* pBtTypedConstraint) const
{
    std::lock_guard guard(dynamicsWorldLock);

    m_pDynamicsWorld->removeConstraint(pBtTypedConstraint);
}

void CClientPhysics::SetGravity(const CVector& vecGravity) const
{
    std::lock_guard guard(dynamicsWorldLock);
    m_pDynamicsWorld->setGravity(reinterpret_cast<const btVector3&>(vecGravity));
}

CVector CClientPhysics::GetGravity() const
{
    std::lock_guard guard(dynamicsWorldLock);
    return reinterpret_cast<const CVector&>(m_pDynamicsWorld->getGravity());
}

bool CClientPhysics::GetUseContinous() const
{
    std::lock_guard guard(dynamicsWorldLock);
    return m_pDynamicsWorld->getDispatchInfo().m_useContinuous;
}

void CClientPhysics::SetUseContinous(bool bUse) const
{
    std::lock_guard guard(dynamicsWorldLock);
    m_pDynamicsWorld->getDispatchInfo().m_useContinuous = bUse;
}

std::shared_ptr<CLuaPhysicsStaticCollision> CClientPhysics::CreateStaticCollision(std::shared_ptr<CLuaPhysicsShape> pShape, CVector vecPosition,
                                                                                  CVector vecRotation)
{
    std::shared_ptr<CLuaPhysicsStaticCollision> pStaticCollision = std::make_shared<CLuaPhysicsStaticCollision>(pShape);
    AddStaticCollision(pStaticCollision);
    return pStaticCollision;
}

std::shared_ptr<CLuaPhysicsStaticCollision> CClientPhysics::CreateStaticCollisionFromModel(unsigned short usModelId, CVector vecPosition, CVector vecRotation)
{
    std::shared_ptr<CLuaPhysicsShape> pShape = CreateShapeFromModel(usModelId);
    if (pShape == nullptr)
        return nullptr;

    std::shared_ptr<CLuaPhysicsStaticCollision> pStaticCollision = CreateStaticCollision(pShape, vecPosition, vecRotation);
    return pStaticCollision;
}

std::shared_ptr<CLuaPhysicsShape> CClientPhysics::CreateShapeFromModel(unsigned short usModelId)
{
    CColDataSA* pColData = CLuaPhysicsSharedLogic::GetModelColData(usModelId);
    if (pColData == nullptr)
        return nullptr;            // model has no collision

    int iInitialSize = pColData->numColBoxes + pColData->numColSpheres;

    if (iInitialSize == 0 && pColData->numColTriangles == 0)
        return nullptr;            // don't create empty collisions

    CColSphereSA   pColSphere;
    CColBoxSA      pColBox;
    CColTriangleSA pColTriangle;
    CVector        position, halfSize;

    if (pColData->numColTriangles > 0)
        iInitialSize++;

    std::shared_ptr<CLuaPhysicsCompoundShape> pCompoundShape = std::make_shared<CLuaPhysicsCompoundShape>(this, iInitialSize);

    for (uint i = 0; pColData->numColBoxes > i; i++)
    {
        pColBox = pColData->pColBoxes[i];
        position = (pColBox.max + pColBox.min) / 2;
        halfSize = (pColBox.max - pColBox.min) * 0.5;
        pCompoundShape->AddShape(CreateBoxShape(halfSize), position);
    }

    for (uint i = 0; pColData->numColSpheres > i; i++)
    {
        pColSphere = pColData->pColSpheres[i];
        pCompoundShape->AddShape(CreateSphereShape(pColSphere.fRadius), position);
    }

    if (pColData->numColTriangles > 0)
    {
        std::vector<CVector> vecIndices;
        for (uint i = 0; pColData->numColTriangles > i; i++)
        {
            pColTriangle = pColData->pColTriangles[i];
            vecIndices.push_back(pColData->pVertices[pColTriangle.vertex[0]].getVector());
            vecIndices.push_back(pColData->pVertices[pColTriangle.vertex[1]].getVector());
            vecIndices.push_back(pColData->pVertices[pColTriangle.vertex[2]].getVector());
        }

        pCompoundShape->AddShape(CreateBhvTriangleMeshShape(vecIndices), CVector(0, 0, 0));
    }

    AddShape(pCompoundShape);
    return pCompoundShape;
}

void CClientPhysics::StartBuildCollisionFromGTA()
{
    if (m_bBuildWorld)
        return;

    m_bBuildWorld = true;
    if (!m_bObjectsCached)
    {
        CLuaPhysicsSharedLogic::CacheWorldObjects(pWorldObjects);
        m_bObjectsCached = true;
    }
}

void CClientPhysics::BuildCollisionFromGTAInRadius(CVector& center, float fRadius)
{
    if (!m_bObjectsCached)
    {
        CLuaPhysicsSharedLogic::CacheWorldObjects(pWorldObjects);
        m_bObjectsCached = true;
    }

    if (pWorldObjects.size() > 0)
    {
        for (auto it = pWorldObjects.begin(); it != pWorldObjects.end(); it++)
        {
            if (DistanceBetweenPoints3D(it->second.first, center) < fRadius)
            {
                if (CLuaPhysicsSharedLogic::GetModelColData(it->first))
                {
                    CreateStaticCollisionFromModel(it->first, it->second.first, it->second.second);
                    pWorldObjects.erase(it--);
                }
            }
        }
    }
}

void CClientPhysics::BuildCollisionFromGTA()
{
    if (pWorldObjects.size() > 0)
    {
        for (auto it = pWorldObjects.begin(); it != pWorldObjects.end(); it++)
        {
            if (CLuaPhysicsSharedLogic::GetModelColData(it->first))
            {
                CreateStaticCollisionFromModel(it->first, it->second.first, it->second.second / 180 / PI);
                pWorldObjects.erase(it--);
            }
        }
    }
}

SClosestConvexResultCallback CClientPhysics::ShapeCast(std::shared_ptr<CLuaPhysicsShape> pShape, const btTransform& from, const btTransform& to,
                                                       int iFilterGroup, int iFilterMask) const
{
    CVector fromPosition;
    CVector toPosition;
    CLuaPhysicsSharedLogic::GetPosition(from, fromPosition);
    CLuaPhysicsSharedLogic::GetPosition(to, toPosition);
    SClosestConvexResultCallback rayCallback(fromPosition, toPosition);

    rayCallback.m_collisionFilterGroup = iFilterGroup;
    rayCallback.m_collisionFilterMask = iFilterMask;
    {
        std::lock_guard guard(dynamicsWorldLock);
        m_pDynamicsWorld->convexSweepTest((btConvexShape*)(pShape->GetBtShape()), from, to, rayCallback, 0.0f);
    }

    rayCallback.m_closestPosition =
        reinterpret_cast<const CVector&>(rayCallback.m_convexFromWorld.lerp(rayCallback.m_convexToWorld, rayCallback.m_closestHitFraction));

    return rayCallback;
}

bool CClientPhysics::LineCast(CVector from, CVector to, bool bFilterBackfaces, int iFilterGroup, int iFilterMask) const
{
    btCollisionWorld::ClosestRayResultCallback rayCallback(reinterpret_cast<btVector3&>(from), reinterpret_cast<btVector3&>(to));
    rayCallback.m_collisionFilterGroup = iFilterGroup;
    rayCallback.m_collisionFilterMask = iFilterMask;
    if (bFilterBackfaces)
        rayCallback.m_flags |= btTriangleRaycastCallback::kF_FilterBackfaces;

    {
        std::lock_guard guard(dynamicsWorldLock);
        m_pDynamicsWorld->rayTest(reinterpret_cast<btVector3&>(from), reinterpret_cast<btVector3&>(to), rayCallback);
    }
    return rayCallback.hasHit();
}

SClosestRayResultCallback CClientPhysics::RayCast(const CVector& from, const CVector& to, int iFilterGroup, int iFilterMask, bool bFilterBackfaces) const
{
    SClosestRayResultCallback rayCallback(from, to);
    rayCallback.m_collisionFilterGroup = iFilterGroup;
    rayCallback.m_collisionFilterMask = iFilterMask;

    if (bFilterBackfaces)
        rayCallback.m_flags |= btTriangleRaycastCallback::kF_FilterBackfaces;

    std::lock_guard guard(dynamicsWorldLock);
    m_pDynamicsWorld->rayTest(reinterpret_cast<const btVector3&>(from), reinterpret_cast<const btVector3&>(to), rayCallback);

    return rayCallback;
}

SAllRayResultCallback CClientPhysics::RayCastAll(CVector from, CVector to, int iFilterGroup, int iFilterMask, bool bFilterBackfaces) const
{
    SAllRayResultCallback rayCallback(reinterpret_cast<btVector3&>(from), reinterpret_cast<btVector3&>(to));
    rayCallback.m_collisionFilterGroup = iFilterGroup;
    rayCallback.m_collisionFilterMask = iFilterMask;
    if (bFilterBackfaces)
        rayCallback.m_flags |= btTriangleRaycastCallback::kF_FilterBackfaces;

    {
        std::lock_guard guard(dynamicsWorldLock);
        m_pDynamicsWorld->rayTest(reinterpret_cast<btVector3&>(from), reinterpret_cast<btVector3&>(to), rayCallback);
    }

    return rayCallback;
}

void CClientPhysics::DestroyElement(CLuaPhysicsElement* pPhysicsElement)
{
    switch (pPhysicsElement->GetClassType())
    {
        case EIdClassType::RIGID_BODY:
            DestroyRigidBody((CLuaPhysicsRigidBody*)pPhysicsElement);
            break;
        case EIdClassType::SHAPE:
            DestroyShape(Resolve((CLuaPhysicsShape*)pPhysicsElement));
            break;
        case EIdClassType::STATIC_COLLISION:
            DestroyStaticCollision((CLuaPhysicsStaticCollision*)pPhysicsElement);
            break;
        case EIdClassType::CONSTRAINT:
            DestroyCostraint((CLuaPhysicsConstraint*)pPhysicsElement);
            break;
    }
}

std::shared_ptr<CLuaPhysicsShape> CClientPhysics::Resolve(CLuaPhysicsShape* pLuaShape)
{
    uint id = pLuaShape->GetScriptID();
    return m_mapShapes[id];
}

void CClientPhysics::DestroyRigidBody(CLuaPhysicsRigidBody* pLuaRigidBody)
{
    m_pLuaMain->GetPhysicsRigidBodyManager()->RemoveRigidBody(pLuaRigidBody);
    m_mapRigidBodies.erase(pLuaRigidBody->GetScriptID());
}

void CClientPhysics::DestroyShape(std::shared_ptr<CLuaPhysicsShape> pLuaShape)
{
    m_pLuaMain->GetPhysicsShapeManager()->RemoveShape(pLuaShape);
    m_mapShapes.erase(pLuaShape->GetScriptID());
}

void CClientPhysics::DestroyCostraint(CLuaPhysicsConstraint* pLuaConstraint)
{
    m_pLuaMain->GetPhysicsConstraintManager()->RemoveContraint(pLuaConstraint);
    m_mapConstraints.erase(pLuaConstraint->GetScriptID());
}

void CClientPhysics::DestroyStaticCollision(CLuaPhysicsStaticCollision* pStaticCollision)
{
    m_pLuaMain->GetPhysicsStaticCollisionManager()->RemoveStaticCollision(pStaticCollision);
    m_mapStaticCollisions.erase(pStaticCollision->GetScriptID());
}

void CClientPhysics::AddStaticCollision(std::shared_ptr<CLuaPhysicsStaticCollision> pStaticCollision)
{
    m_pLuaMain->GetPhysicsStaticCollisionManager()->AddStaticCollision(pStaticCollision);
    m_mapStaticCollisions.emplace(pStaticCollision->GetScriptID(), pStaticCollision);
    m_InitializeStaticCollisionsQueue.push(pStaticCollision);
}

void CClientPhysics::AddShape(std::shared_ptr<CLuaPhysicsShape> pShape)
{
    m_pLuaMain->GetPhysicsShapeManager()->AddShape(pShape);
    m_mapShapes.emplace(pShape->GetScriptID(), pShape);
}

void CClientPhysics::AddRigidBody(std::shared_ptr<CLuaPhysicsRigidBody> pRigidBody)
{
    m_mapRigidBodies.emplace(pRigidBody->GetScriptID(), pRigidBody);
    m_pLuaMain->GetPhysicsRigidBodyManager()->AddRigidBody(pRigidBody);
    m_InitializeRigidBodiesQueue.push(pRigidBody);
}

void CClientPhysics::AddConstraint(std::shared_ptr<CLuaPhysicsConstraint> pConstraint)
{
    m_mapConstraints.emplace(pConstraint->GetScriptID(), pConstraint);
    m_pLuaMain->GetPhysicsConstraintManager()->AddConstraint(pConstraint);
    m_InitializeConstraintsQueue.push(pConstraint);
}

void CClientPhysics::StepSimulation()
{
    if (!m_bSimulationEnabled)
        return;

    std::lock_guard guard(dynamicsWorldLock);
    m_pDynamicsWorld->stepSimulation(((float)m_iDeltaTimeMs) / 1000.0f * m_fSpeed, m_iSubSteps);
}

void CClientPhysics::ClearOutsideWorldRigidBodies()
{
    CLuaPhysicsRigidBodyManager*                       pRigidBodyManager = m_pLuaMain->GetPhysicsRigidBodyManager();
    std::vector<std::shared_ptr<CLuaPhysicsRigidBody>> vecRigidBodiesToRemove;
    CVector                                            vecRigidBody;
    for (auto iter = pRigidBodyManager->IterBegin(); iter != pRigidBodyManager->IterEnd(); ++iter)
    {
        std::shared_ptr<CLuaPhysicsRigidBody> pRigidBody = *iter;
        if (!pRigidBody->IsSleeping())
        {
            vecRigidBody = pRigidBody->GetPosition();
            if (vecRigidBody.fZ <= -m_vecWorldSize.fZ || vecRigidBody.fZ >= m_vecWorldSize.fZ)
            {
                vecRigidBodiesToRemove.push_back(pRigidBody);
            }
        }
    }
    for (std::shared_ptr<CLuaPhysicsRigidBody> pRigidBody : vecRigidBodiesToRemove)
    {
        CLuaArguments Arguments;
        Arguments.PushPhysicsRigidBody(pRigidBody.get());
        if (!CallEvent("onPhysicsRigidBodyFallOutsideWorld", Arguments, true))
            pRigidBodyManager->RemoveRigidBody(pRigidBody.get());
    }
}

std::vector<std::shared_ptr<CLuaPhysicsShape>> CClientPhysics::GetShapes()
{
    std::vector<std::shared_ptr<CLuaPhysicsShape>> shapes;
    shapes.reserve(m_mapShapes.size());
    for (const auto& pair : m_mapShapes)
    {
        shapes.push_back(pair.second);
    }
    return shapes;
}

std::vector<std::shared_ptr<CLuaPhysicsRigidBody>> CClientPhysics::GetRigidBodies()
{
    std::vector<std::shared_ptr<CLuaPhysicsRigidBody>> rigidBodies;
    rigidBodies.reserve(m_mapRigidBodies.size());
    for (const auto& pair : m_mapRigidBodies)
    {
        rigidBodies.push_back(pair.second);
    }
    return rigidBodies;
}

std::vector<std::shared_ptr<CLuaPhysicsStaticCollision>> CClientPhysics::GetStaticCollisions()
{
    std::vector<std::shared_ptr<CLuaPhysicsStaticCollision>> staticCollisions;
    staticCollisions.reserve(m_mapStaticCollisions.size());
    for (const auto& pair : m_mapStaticCollisions)
    {
        staticCollisions.push_back(pair.second);
    }
    return staticCollisions;
}

std::vector<std::shared_ptr<CLuaPhysicsConstraint>> CClientPhysics::GetConstraints()
{
    std::vector<std::shared_ptr<CLuaPhysicsConstraint>> constraints;
    constraints.reserve(m_mapConstraints.size());
    for (const auto& pair : m_mapConstraints)
    {
        constraints.push_back(pair.second);
    }
    return constraints;
}

std::shared_ptr<CLuaPhysicsRigidBody> CClientPhysics::GetSharedRigidBody(CLuaPhysicsRigidBody* pRigidBody)
{
    uint id = pRigidBody->GetScriptID();
    return m_mapRigidBodies[id];
}

std::shared_ptr<CLuaPhysicsStaticCollision> CClientPhysics::GetSharedStaticCollision(CLuaPhysicsStaticCollision* pStaticCollision)
{
    uint id = pStaticCollision->GetScriptID();
    return m_mapStaticCollisions[id];
}

std::shared_ptr<CLuaPhysicsStaticCollision> CClientPhysics::GetStaticCollisionFromCollisionShape(const btCollisionObject* pCollisionObject)
{
    for (std::shared_ptr<CLuaPhysicsStaticCollision> pStaticCollision : GetStaticCollisions())
    {
        if (pStaticCollision->GetCollisionObject() == pCollisionObject)
            return pStaticCollision;
    }

    return nullptr;
}

std::shared_ptr<CLuaPhysicsRigidBody> CClientPhysics::GetRigidBodyFromCollisionShape(const btCollisionObject* pCollisionObject)
{
    for (auto& pRigidBody : GetRigidBodies())
        if (pRigidBody->GetBtRigidBody() == pCollisionObject)
            return pRigidBody;

    return nullptr;
}

void CClientPhysics::PostProcessCollisions()
{
    int numManifolds;
    {
        std::lock_guard guard(dynamicsWorldLock);
        numManifolds = m_pDynamicsWorld->getDispatcher()->getNumManifolds();
    }

    for (const auto& element : m_vecLastContact)
        element->ClearCollisionReport();

    m_vecLastContact.clear();

    //if (m_bTriggerConstraintEvents)
    //{
    //    std::vector<std::shared_ptr<CLuaPhysicsConstraint>>::const_iterator iter = pConstraintManager->IterBegin();
    //    for (; iter != pConstraintManager->IterEnd(); iter++)
    //    {
    //        if ((*iter)->BreakingStatusHasChanged())
    //        {
    //            /* if ((*iter)->IsBroken())
    //             {*/
    //            CLuaArguments Arguments;
    //            Arguments.PushPhysicsConstraint((*iter).get());

    //            CallEvent("onPhysicsConstraintBreak", Arguments, true);
    //            //}
    //        }
    //    }
    //}

    btCollisionObject* objectA = nullptr;
    btCollisionObject* objectB = nullptr;
    btCollisionShape*  shapeA = nullptr;
    btCollisionShape*  shapeB = nullptr;

    for (int i = 0; i < numManifolds; i++)
    {
        btPersistentManifold* contactManifold = m_pDynamicsWorld->getDispatcher()->getManifoldByIndexInternal(i);
        if (contactManifold == nullptr)
            continue;

        int numContacts = contactManifold->getNumContacts();
        if (numContacts == 0)
            continue;

        objectA = (btCollisionObject*)(contactManifold->getBody0());
        objectB = (btCollisionObject*)(contactManifold->getBody1());
        shapeA = objectA->getCollisionShape();
        shapeB = objectB->getCollisionShape();

        std::shared_ptr<CLuaPhysicsRigidBody>       pRigidRigidA = GetRigidBodyFromCollisionShape(objectA);
        std::shared_ptr<CLuaPhysicsRigidBody>       pRigidRigidB = GetRigidBodyFromCollisionShape(objectB);
        std::shared_ptr<CLuaPhysicsStaticCollision> pStaticCollisionA = GetStaticCollisionFromCollisionShape(objectA);
        std::shared_ptr<CLuaPhysicsStaticCollision> pStaticCollisionB = GetStaticCollisionFromCollisionShape(objectB);

        std::unique_ptr<SPhysicsCollisionReport> collisionReportA = std::make_unique<SPhysicsCollisionReport>();
        std::unique_ptr<SPhysicsCollisionReport> collisionReportB = std::make_unique<SPhysicsCollisionReport>();

        if (pRigidRigidA)
            collisionReportA->pElement = pRigidRigidA;
        else
            collisionReportA->pElement = pStaticCollisionA;

        if (pRigidRigidB)
            collisionReportB->pElement = pRigidRigidB;
        else
            collisionReportB->pElement = pStaticCollisionB;

        for (int j = 0; j < numContacts; j++)
        {
            btManifoldPoint& manifoldPoint = contactManifold->getContactPoint(j);
            std::shared_ptr<SPhysicsCollisionContact> contactA = std::make_shared<SPhysicsCollisionContact>();
            std::shared_ptr<SPhysicsCollisionContact> contactB = std::make_shared<SPhysicsCollisionContact>();
            contactA->vecPositionWorldOn = reinterpret_cast<const CVector&>(manifoldPoint.getPositionWorldOnA());
            contactB->vecPositionWorldOn = reinterpret_cast<const CVector&>(manifoldPoint.getPositionWorldOnB());
            contactA->vecLocalPoint = reinterpret_cast<const CVector&>(manifoldPoint.m_localPointA);
            contactB->vecLocalPoint = reinterpret_cast<const CVector&>(manifoldPoint.m_localPointB);
            contactA->vecLateralFrictionDir = reinterpret_cast<const CVector&>(manifoldPoint.m_lateralFrictionDir1);
            contactB->vecLateralFrictionDir = reinterpret_cast<const CVector&>(manifoldPoint.m_lateralFrictionDir2);
            contactA->contactTriangle = manifoldPoint.m_partId0;
            contactB->contactTriangle = manifoldPoint.m_partId1;
            contactA->appliedImpulse = manifoldPoint.getAppliedImpulse();
            contactB->appliedImpulse = manifoldPoint.getAppliedImpulse();
            contactA->appliedImpulseLiteral = manifoldPoint.m_appliedImpulseLateral1;
            contactB->appliedImpulseLiteral = manifoldPoint.m_appliedImpulseLateral2;
            collisionReportA->m_vecContacts.push_back(std::move(contactA));
            collisionReportB->m_vecContacts.push_back(std::move(contactB));
        }

        if (pRigidRigidA)
        {
            m_vecLastContact.push_back(pRigidRigidA);
            pRigidRigidA->ReportCollision(std::move(collisionReportB));
        }

        if (pRigidRigidB)
        {
            m_vecLastContact.push_back(pRigidRigidB);
            pRigidRigidB->ReportCollision(std::move(collisionReportA));
        }

        if (pStaticCollisionA)
        {
            m_vecLastContact.push_back(pStaticCollisionA);
            pStaticCollisionA->ReportCollision(std::move(collisionReportB));
        }

        if (pStaticCollisionB)
        {
            m_vecLastContact.push_back(pStaticCollisionB);
            pStaticCollisionB->ReportCollision(std::move(collisionReportA));
        }
    }
}

std::shared_ptr<CLuaPhysicsBoxShape> CClientPhysics::CreateBoxShape(CVector vector)
{
    std::shared_ptr<CLuaPhysicsBoxShape> pShape = std::make_shared<CLuaPhysicsBoxShape>(this, vector);
    AddShape(pShape);
    return pShape;
}

std::shared_ptr<CLuaPhysicsSphereShape> CClientPhysics::CreateSphereShape(float radius)
{
    assert(radius > 0);

    std::shared_ptr<CLuaPhysicsSphereShape> pShape = std::make_shared<CLuaPhysicsSphereShape>(this, radius);
    AddShape(pShape);
    return pShape;
}

std::shared_ptr<CLuaPhysicsCapsuleShape> CClientPhysics::CreateCapsuleShape(float fRadius, float fHeight)
{
    assert(fRadius > 0);
    assert(fHeight > 0);

    std::shared_ptr<CLuaPhysicsCapsuleShape> pShape = std::make_shared<CLuaPhysicsCapsuleShape>(this, fRadius, fHeight);
    AddShape(pShape);
    return pShape;
}

std::shared_ptr<CLuaPhysicsConeShape> CClientPhysics::CreateConeShape(float fRadius, float fHeight)
{
    assert(fRadius > 0);
    assert(fHeight > 0);

    std::shared_ptr<CLuaPhysicsConeShape> pShape = std::make_shared<CLuaPhysicsConeShape>(this, fRadius, fHeight);
    AddShape(pShape);
    return pShape;
}

std::shared_ptr<CLuaPhysicsCylinderShape> CClientPhysics::CreateCylinderShape(CVector half)
{
    assert(half.fX > 0);
    assert(half.fY > 0);
    assert(half.fZ > 0);

    std::shared_ptr<CLuaPhysicsCylinderShape> pShape = std::make_shared<CLuaPhysicsCylinderShape>(this, half);
    AddShape(pShape);
    return pShape;
}

std::shared_ptr<CLuaPhysicsCompoundShape> CClientPhysics::CreateCompoundShape(int iInitialChildCapacity)
{
    assert(iInitialChildCapacity > 0);

    std::shared_ptr<CLuaPhysicsCompoundShape> pShape = std::make_shared<CLuaPhysicsCompoundShape>(this, iInitialChildCapacity);
    AddShape(pShape);
    return pShape;
}

std::shared_ptr<CLuaPhysicsConvexHullShape> CClientPhysics::CreateConvexHullShape(std::vector<CVector>& vecPoints)
{
    assert(vecPoints.size() >= 3);

    std::shared_ptr<CLuaPhysicsConvexHullShape> pShape = std::make_shared<CLuaPhysicsConvexHullShape>(this, vecPoints);
    AddShape(pShape);
    return pShape;
}

std::shared_ptr<CLuaPhysicsConvexHullShape> CClientPhysics::CreateConvexHullShape(std::vector<float>& vecFloats)
{
    assert(vecFloats.size() >= 9);

    std::vector<CVector> vecPoints;
    vecPoints.reserve(vecFloats.size() / 3);
    for (int i = 0; i < vecFloats.size(); i+=3)
        vecPoints.emplace_back(vecFloats[i], vecFloats[i + 1], vecFloats[i + 2]);

    return CreateConvexHullShape(vecPoints);
}

std::shared_ptr<CLuaPhysicsBvhTriangleMeshShape> CClientPhysics::CreateBhvTriangleMeshShape(std::vector<CVector>& vecVertices)
{
    assert(vecVertices.size() >= 3);

    std::shared_ptr<CLuaPhysicsBvhTriangleMeshShape> pShape = std::make_shared<CLuaPhysicsBvhTriangleMeshShape>(this, vecVertices);
    AddShape(pShape);
    return pShape;
}

std::shared_ptr<CLuaPhysicsBvhTriangleMeshShape> CClientPhysics::CreateBhvTriangleMeshShape(std::vector<float>& vecFloats)
{
    std::vector<CVector> vecPoints;
    vecPoints.reserve(vecFloats.size() / 3);
    for (int i = 0; i < vecFloats.size(); i += 3)
        vecPoints.emplace_back(vecFloats[i], vecFloats[i + 1], vecFloats[i + 2]);

    return CreateBhvTriangleMeshShape(vecPoints);
}

std::shared_ptr<CLuaPhysicsGimpactTriangleMeshShape> CClientPhysics::CreateGimpactTriangleMeshShape(std::vector<CVector>& vecVertices)
{
    assert(vecVertices.size() >= 3);

    std::shared_ptr<CLuaPhysicsGimpactTriangleMeshShape> pShape = std::make_shared<CLuaPhysicsGimpactTriangleMeshShape>(this, vecVertices);
    AddShape(pShape);
    return pShape;
}

std::shared_ptr<CLuaPhysicsGimpactTriangleMeshShape> CClientPhysics::CreateGimpactTriangleMeshShape(std::vector<float>& vecFloats)
{
    std::vector<CVector> vecPoints;
    vecPoints.reserve(vecFloats.size() / 3);
    for (int i = 0; i < vecFloats.size(); i += 3)
        vecPoints.emplace_back(vecFloats[i], vecFloats[i + 1], vecFloats[i + 2]);

    return CreateGimpactTriangleMeshShape(vecPoints);
}

std::shared_ptr<CLuaPhysicsHeightfieldTerrainShape> CClientPhysics::CreateHeightfieldTerrainShape(int iSizeX, int iSizeY)
{
    std::vector<float> vecHeights(iSizeX * iSizeY, 0);
    std::shared_ptr<CLuaPhysicsHeightfieldTerrainShape> pShape = std::make_shared<CLuaPhysicsHeightfieldTerrainShape>(this, iSizeX, iSizeY, vecHeights);
    AddShape(pShape);
    return pShape;
}

std::shared_ptr<CLuaPhysicsHeightfieldTerrainShape> CClientPhysics::CreateHeightfieldTerrainShape(int iSizeX, int iSizeY, std::vector<float>& vecHeights)
{
    std::shared_ptr<CLuaPhysicsHeightfieldTerrainShape> pShape = std::make_shared<CLuaPhysicsHeightfieldTerrainShape>(this, iSizeX, iSizeY, vecHeights);
    AddShape(pShape);
    return pShape;
}

std::shared_ptr<CLuaPhysicsRigidBody> CClientPhysics::CreateRigidBody(std::shared_ptr<CLuaPhysicsShape> pShape, float fMass, CVector vecLocalInertia,
                                                                      CVector vecCenterOfMass)
{
    std::shared_ptr<CLuaPhysicsRigidBody> pRigidBody = std::make_shared<CLuaPhysicsRigidBody>(pShape, fMass, vecLocalInertia, vecCenterOfMass);
    AddRigidBody(pRigidBody);
    return pRigidBody;
}

std::shared_ptr<CLuaPhysicsPointToPointConstraint> CClientPhysics::CreatePointToPointConstraint(CLuaPhysicsRigidBody* pRigidBodyA,
                                                                                                CLuaPhysicsRigidBody* pRigidBodyB,
                                                                                                bool                  bDisableCollisionsBetweenLinkedBodies)
{
    assert(pRigidBodyA != pRigidBodyB);
    assert(pRigidBodyA->GetPhysics() == pRigidBodyB->GetPhysics());

    CVector vecPivotA = pRigidBodyB->GetPosition() - pRigidBodyA->GetPosition();
    CVector vecPivotB = pRigidBodyA->GetPosition() - pRigidBodyB->GetPosition();

    std::shared_ptr<CLuaPhysicsPointToPointConstraint> pConstraint =
        std::make_shared<CLuaPhysicsPointToPointConstraint>(pRigidBodyA, pRigidBodyB, vecPivotA / 2, vecPivotB / 2, bDisableCollisionsBetweenLinkedBodies);
    AddConstraint(pConstraint);
    return pConstraint;
}

std::shared_ptr<CLuaPhysicsPointToPointConstraint> CClientPhysics::CreatePointToPointConstraint(CLuaPhysicsRigidBody* pRigidBody, const CVector& position,
                                                                                                bool bDisableCollisionsBetweenLinkedBodies)
{
    std::shared_ptr<CLuaPhysicsPointToPointConstraint> pConstraint =
        std::make_shared<CLuaPhysicsPointToPointConstraint>(pRigidBody, position, bDisableCollisionsBetweenLinkedBodies);
    AddConstraint(pConstraint);
    return pConstraint;
}

std::shared_ptr<CLuaPhysicsPointToPointConstraint> CClientPhysics::CreatePointToPointConstraint(CLuaPhysicsRigidBody* pRigidBodyA,
                                                                                                CLuaPhysicsRigidBody* pRigidBodyB, const CVector& vecPivotA,
                                                                                                const CVector& vecPivotB,
                                                                                                bool           bDisableCollisionsBetweenLinkedBodies)
{
    assert(pRigidBodyA != pRigidBodyB);
    assert(pRigidBodyA->GetPhysics() == pRigidBodyB->GetPhysics());

    std::shared_ptr<CLuaPhysicsPointToPointConstraint> pConstraint =
        std::make_shared<CLuaPhysicsPointToPointConstraint>(pRigidBodyA, pRigidBodyB, vecPivotA, vecPivotB, bDisableCollisionsBetweenLinkedBodies);
    AddConstraint(pConstraint);
    return pConstraint;
}

std::shared_ptr<CLuaPhysicsFixedConstraint> CClientPhysics::CreateFixedConstraint(CLuaPhysicsRigidBody* pRigidBodyA, CLuaPhysicsRigidBody* pRigidBodyB,
                                                                                  bool bDisableCollisionsBetweenLinkedBodies)
{
    assert(pRigidBodyA->GetPhysics() == pRigidBodyB->GetPhysics());

    CVector vecPositionA = pRigidBodyA->GetPosition() - pRigidBodyB->GetPosition();
    CVector vecRotationA;
    CVector vecPositionB = pRigidBodyB->GetPosition() - pRigidBodyA->GetPosition();
    CVector vecRotationB;

    std::shared_ptr<CLuaPhysicsFixedConstraint> pConstraint = std::make_shared<CLuaPhysicsFixedConstraint>(
        pRigidBodyA, pRigidBodyB, vecPositionA, vecRotationA, vecPositionB, vecRotationB, bDisableCollisionsBetweenLinkedBodies);
    AddConstraint(pConstraint);
    return pConstraint;
}

bool CClientPhysics::CanDoPulse()
{
    return (m_pLuaMain != nullptr && !m_pLuaMain->BeingDeleted());
}

void CClientPhysics::DrawDebugLines()
{
    if (m_bDrawDebugNextTime)
    {
        for (auto const& line : m_pDebugDrawer->m_vecLines)
            g_pCore->GetGraphics()->DrawLine3DQueued(line.from, line.to, m_pDebugDrawer->GetLineWidth(), line.color, false);
        m_bDrawDebugNextTime = false;
    }
}

void CClientPhysics::QueryBox(const CVector& min, const CVector& max, std::vector<CLuaPhysicsRigidBody*>& vecRigidBodies,
                              std::vector<CLuaPhysicsStaticCollision*>& vecStaticCollisions, short collisionGroup, int collisionMask)
{
    btAlignedObjectArray<btCollisionObject*> collisionObjectArray;
    BroadphaseAabbCallback                   callback(collisionObjectArray, collisionGroup, collisionMask);

    {
        std::lock_guard guard(dynamicsWorldLock);
        m_pDynamicsWorld->getBroadphase()->aabbTest(reinterpret_cast<const btVector3&>(min), reinterpret_cast<const btVector3&>(max), callback);
    }

    for (int i = 0; i < callback.m_collisionObjectArray.size(); ++i)
    {
        auto const& btObject = callback.m_collisionObjectArray[i];
        if (CPhysicsRigidBodyProxy* pRigidBody = dynamic_cast<CPhysicsRigidBodyProxy*>(btObject))
        {
            vecRigidBodies.push_back((CLuaPhysicsRigidBody*)pRigidBody->getUserPointer());
        }
        else if (CPhysicsStaticCollisionProxy* pStaticCollision = dynamic_cast<CPhysicsStaticCollisionProxy*>(btObject))
        {
            vecStaticCollisions.push_back((CLuaPhysicsStaticCollision*)pStaticCollision->getUserPointer());
        }
    }
}

void CClientPhysics::AddToActivationStack(CLuaPhysicsRigidBody* pRigidBody)
{
    m_StackRigidBodiesActivation.push(pRigidBody);
}

void CClientPhysics::AddToUpdateAABBStack(CLuaPhysicsRigidBody* pRigidBody)
{
    m_StackRigidBodiesUpdateAABB.push(pRigidBody);
}

void CClientPhysics::AddToChangesStack(CLuaPhysicsElement* pElement)
{
    m_StackElementChanges.push(pElement);
}

void CClientPhysics::AddToUpdateStack(CLuaPhysicsElement* pElement)
{
    m_StackElementUpdates.push(pElement);
}

void CClientPhysics::DoPulse()
{
    std::lock_guard<std::mutex> guard(lock);
    assert(!isDuringSimulation);

    CBulletPhysicsProfiler::Clear();

    {
        BT_PROFILE("initializeStaticCollisions");
        while (!m_InitializeStaticCollisionsQueue.empty())
        {
            std::shared_ptr<CLuaPhysicsStaticCollision> pStaticCollision = m_InitializeStaticCollisionsQueue.top();
            pStaticCollision->Initialize(pStaticCollision);

            m_InitializeStaticCollisionsQueue.pop();
        }
    }

    {
        BT_PROFILE("initializeRigidBodies");
        while (!m_InitializeRigidBodiesQueue.empty())
        {
            std::shared_ptr<CLuaPhysicsRigidBody> pRigidBody = m_InitializeRigidBodiesQueue.top();
            pRigidBody->Initialize(pRigidBody);

            m_InitializeRigidBodiesQueue.pop();
        }
    }

    {
        BT_PROFILE("initializeConstraints");
        while (!m_InitializeConstraintsQueue.empty())
        {
            std::shared_ptr<CLuaPhysicsConstraint> pConstraint = m_InitializeConstraintsQueue.top();
            pConstraint->Initialize();

            m_InitializeConstraintsQueue.pop();
        }
    }

    {
        BT_PROFILE("activateRigidBodies");
        while (!m_StackRigidBodiesActivation.empty())
        {
            CLuaPhysicsRigidBody* pRigidBody = m_StackRigidBodiesActivation.top();
            pRigidBody->Activate();
            m_pDynamicsWorld->getBroadphase()->getOverlappingPairCache()->cleanProxyFromPairs(pRigidBody->GetBtRigidBody()->getBroadphaseHandle(),
                                                                                              m_pDynamicsWorld->getDispatcher());
            m_StackRigidBodiesActivation.pop();
        }
    }
    {
        BT_PROFILE("updateRigidBodiesAABB");
        while (!m_StackRigidBodiesUpdateAABB.empty())
        {
            CLuaPhysicsRigidBody* pRigidBody = m_StackRigidBodiesUpdateAABB.top();
            m_pDynamicsWorld->updateSingleAabb(pRigidBody->GetBtRigidBody());
            pRigidBody->AABBUpdated();
            m_StackRigidBodiesUpdateAABB.pop();
        }
    }

    {
        BT_PROFILE("applyChanges");
        while (!m_StackElementChanges.empty())
        {
            CLuaPhysicsElement* pElement = m_StackElementChanges.top();
            pElement->ApplyChanges();

            m_StackElementChanges.pop();
        }
    }

    {
        BT_PROFILE("update");
        while (!m_StackElementUpdates.empty())
        {
            CLuaPhysicsElement* pElement = m_StackElementUpdates.top();
            pElement->Update();

            m_StackElementUpdates.pop();
        }
    }

    CTickCount tickCountNow = CTickCount::Now();

    m_iDeltaTimeMs = (int)(tickCountNow - m_LastTimeMs).ToLongLong();
    int iDeltaTimeBuildWorld = (int)(tickCountNow - m_LastTimeBuildWorld).ToLongLong();
    m_LastTimeMs = tickCountNow;

    // if (m_bBuildWorld)
    //{
    //    if (iDeltaTimeBuildWorld > 1000)
    //    {
    //        m_LastTimeBuildWorld = tickCountNow;
    //        BuildCollisionFromGTA();
    //    }
    //}

    isDuringSimulation = true;
    {
        BT_PROFILE("stepSimulation");
        StepSimulation();
    }

    if (m_bDrawDebugNextTime)
    {
        m_pDebugDrawer->Clear();

        CVector vecPosition, vecLookAt;
        float   fRoll, fFOV;
        CStaticFunctionDefinitions::GetCameraMatrix(vecPosition, vecLookAt, fRoll, fFOV);
        m_pDebugDrawer->SetCameraPosition(vecPosition);
        {
            std::lock_guard guard(dynamicsWorldLock);
            m_pDynamicsWorld->debugDrawWorld();
        }
    }

    {
        BT_PROFILE("cacheActiveRigidBodies");
        std::lock_guard guardVecActiveRigidBodies(m_vecActiveRigidBodiesLock);
        std::lock_guard guardDynamicsWorld(dynamicsWorldLock);
        m_vecActiveRigidBodies.clear();

        btAlignedObjectArray<btRigidBody*>& nonStaticRigidBodies = m_pDynamicsWorld->getNonStaticRigidBodies();
        for (int i = 0; i < nonStaticRigidBodies.size(); i++)
        {
            if (nonStaticRigidBodies[i]->isActive())
            {
                CLuaPhysicsRigidBody* pRigidBody = (CLuaPhysicsRigidBody*)nonStaticRigidBodies[i]->getUserPointer();
                pRigidBody->HasMoved();
                m_vecActiveRigidBodies.push_back(pRigidBody);
            }
        }
    }

    //{
    //    BT_PROFILE("postProcessCollisions");
    //    PostProcessCollisions();
    //}

    m_mapProfileTimings = CBulletPhysicsProfiler::GetProfileTimings();

    isDuringSimulation = false;
    // ClearOutsideWorldRigidBodies();
}
