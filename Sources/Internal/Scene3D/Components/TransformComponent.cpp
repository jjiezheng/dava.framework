#include "Scene3D/Components/TransformComponent.h"
#include "Scene3D/SceneNode.h"
#include "Scene3D/Scene.h"
#include "Scene3D/Systems/EventSystem.h"

namespace DAVA
{
    
TransformComponent::TransformComponent()
{
    localMatrix = Matrix4::IDENTITY;
	worldMatrix = Matrix4::IDENTITY;
	parentMatrix = 0;
	parent = 0;
}
    
TransformComponent::~TransformComponent()
{
    
}

Component * TransformComponent::Clone(SceneNode * toEntity)
{
    TransformComponent * newTransform = new TransformComponent();
	SetEntity(toEntity);
	newTransform->localMatrix = localMatrix;
	newTransform->worldMatrix = worldMatrix;
    newTransform->parent = this->parent;

	Scene::GetActiveScene()->ImmediateEvent(entity, GetType(), EventSystem::TRANSFORM_PARENT_CHANGED);

    return newTransform;
}


void TransformComponent::SetLocalTransform(const Matrix4 * transform)
{
	localMatrix = *transform;
	if(!parent)
	{
		worldMatrix = *transform;
	}

	Scene::GetActiveScene()->ImmediateEvent(entity, GetType(), EventSystem::LOCAL_TRANSFORM_CHANGED);
}

void TransformComponent::SetParent(SceneNode * node)
{
	parent = node;

	if(node)
	{
		parentMatrix = ((TransformComponent*)node->GetComponent(Component::TRANSFORM_COMPONENT))->GetWorldTransformPtr();
	}
	else
	{
		parentMatrix = 0;
	}

	Scene::GetActiveScene()->ImmediateEvent(entity, GetType(), EventSystem::TRANSFORM_PARENT_CHANGED);
}

Matrix4 & TransformComponent::ModifyLocalTransform()
{
	Scene::GetActiveScene()->ImmediateEvent(entity, GetType(), EventSystem::LOCAL_TRANSFORM_CHANGED);
	return localMatrix;
}

};