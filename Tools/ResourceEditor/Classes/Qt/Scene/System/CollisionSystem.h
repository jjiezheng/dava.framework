/*==================================================================================
    Copyright (c) 2008, DAVA, INC
    All rights reserved.

    Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of the DAVA, INC nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE DAVA, INC AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL DAVA, INC BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=====================================================================================*/

#ifndef __SCENE_COLLISION_SYSTEM_H__
#define __SCENE_COLLISION_SYSTEM_H__

#include <QMap>
#include "Scene/EntityGroup.h"
#include "Scene/SceneTypes.h"

// bullet
#include "bullet/btBulletCollisionCommon.h"

// framework
#include "UI/UIEvent.h"
#include "Entity/SceneSystem.h"
#include "Render/Highlevel/Landscape.h"
#include "Render/RenderManager.h"
#include "Render/RenderHelper.h"

class CollisionBaseObject;
class SceneCollisionDebugDrawer;

class SceneCollisionSystem : public DAVA::SceneSystem
{
	friend class SceneEditorProxy;
	friend class EntityModificationSystem;

public:
	SceneCollisionSystem(DAVA::Scene * scene);
	~SceneCollisionSystem();

	void SetDrawMode(int mode);
	int GetDebugDrawFlags();

	DAVA::AABBox3 GetBoundingBox(DAVA::Entity *entity);

	const EntityGroup* ObjectsRayTest(const DAVA::Vector3 &from, const DAVA::Vector3 &to);
	const EntityGroup* ObjectsRayTestFromCamera();

	DAVA::Vector3 LandRayTest(const DAVA::Vector3 &from, const DAVA::Vector3 &to);
	DAVA::Vector3 LandRayTestFromCamera();

	void UpdateCollisionObject(DAVA::Entity *entity);

protected:
	void Update(DAVA::float32 timeElapsed);
	void ProcessUIEvent(DAVA::UIEvent *event);
	void Draw();

	virtual void AddEntity(DAVA::Entity * entity);
	virtual void RemoveEntity(DAVA::Entity * entity);

protected:
	int drawMode;

	DAVA::Vector3 lastRayFrom;
	DAVA::Vector3 lastRayTo;
	DAVA::Vector2 lastMousePos;

	EntityGroup rayIntersectedEntities;
	bool rayIntersectCached;

	DAVA::Vector3 lastLandRayFrom;
	DAVA::Vector3 lastLandRayTo;
	DAVA::Vector3 lastLandCollision;

	btDefaultCollisionConfiguration* objectsCollConf;
	btCollisionDispatcher* objectsCollDisp;
	btAxisSweep3* objectsBroadphase;
	btCollisionWorld *objectsCollWorld;
	SceneCollisionDebugDrawer *objectsDebugDrawer;

	btDefaultCollisionConfiguration* landCollConf;
	btCollisionDispatcher* landCollDisp;
	btAxisSweep3* landBroadphase;
	btCollisionWorld *landCollWorld;
	SceneCollisionDebugDrawer *landDebugDrawer;

	QMap<DAVA::Entity*, CollisionBaseObject*> entityToCollision;
	QMap<btCollisionObject*, DAVA::Entity*> collisionToEntity;

	void BuildFromEntity(DAVA::Entity * entity);
	void DestroyFromEntity(DAVA::Entity * entity);
};

class SceneCollisionDebugDrawer : public btIDebugDraw
{
public:
	SceneCollisionDebugDrawer();

	virtual void drawLine(const btVector3& from, const btVector3& to, const btVector3& color);
	virtual void drawContactPoint(const btVector3& PointOnB,const btVector3& normalOnB,btScalar distance,int lifeTime,const btVector3& color);
	virtual void reportErrorWarning(const char* warningString);
	virtual void draw3dText(const btVector3& location,const char* textString);
	virtual void setDebugMode(int debugMode);
	virtual int	getDebugMode() const;

protected:
	int dbgMode;
	DAVA::RenderManager *manager;
	DAVA::RenderHelper *helper;
};

#endif // __SCENE_COLLISION_SYSTEM_H__