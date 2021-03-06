//
//  ParticleEffectNode.cpp
//  FrameworkQt
//
//  Created by Yuri Coder on 11/22/12.
//
//

#include "ParticleEffectNode.h"
#include "FileSystem/KeyedArchive.h"
#include "Render/RenderManager.h"
#include "Render/RenderHelper.h"

using namespace DAVA;
REGISTER_CLASS(ParticleEffectNode);

ParticleEffectNode::ParticleEffectNode() : Entity()
{
    this->stopAfterNRepeats = -1;
    this->stopWhenEmpty = false;
    this->effectDuration = 0.0f;
    this->emittersCurrentlyStopped = 0;
    
    SetName("Particle effect");
}

void ParticleEffectNode::AddNode(Entity* node)
{
	//Dizz: commented due to ParticleEmitterNode => Component transition (ParticleEmitterNode on load, Component after conversion)
    //if (PrepareNewParticleEmitterNode(node))
    {
        Entity::AddNode(node);
    }
}

void ParticleEffectNode::InsertBeforeNode(Entity *newNode, Entity *beforeNode)
{
    if (PrepareNewParticleEmitterNode(newNode))
    {
        Entity::InsertBeforeNode(newNode, beforeNode);
    }
}

bool ParticleEffectNode::PrepareNewParticleEmitterNode(Entity* node)
{
    //// Only Particle Emitter nodes are allowed.
    //ParticleEmitterComponent * particleEmitterComponent = static_cast<ParticleEmitterComponent*>(node->GetComponent(Component::PARTICLE_EMITTER_COMPONENT));
    //if (!particleEmitterComponent)
    //{
    //    Logger::Warning("ParticleEffectNode::PrepareNewParticleEmitterNode() - attempt to add child node with wrong type!");
    //    return false;
    //}
    //
    //ParticleEmitter* emitter = particleEmitterComponent->GetParticleEmitter();
    //if (!emitter)
    //{
    //    Logger::Error("ParticleEffectNode::PrepareNewParticleEmitterNode() - no Emitter exists!");
    //    return false;
    //}

    //// Default Node State is Stopped.
    ////emitter->Stop();
    //
    //// The effect duration is the same as longest lifetime of the child nodes.
    //float32 newEmitterLifeTime = emitter->GetLifeTime();
    //if (newEmitterLifeTime > effectDuration)
    //{
    //    this->effectDuration = newEmitterLifeTime;
    //    UpdateDurationForChildNodes(newEmitterLifeTime);
    //}
    //else
    //{
    //    emitter->SetLifeTime(effectDuration);
    //}

    return true;
}

void ParticleEffectNode::UpdateDurationForChildNodes(float32 newEmitterLifeTime)
{
  //  int32 childrenCount = GetChildrenCount();
  //  for (int32 i = 0; i < childrenCount; i ++)
  //  {
		//ParticleEmitterComponent * emitterComponent = cast_if_equal<ParticleEmitterComponent*>(GetChild(i)->GetComponent(Component::PARTICLE_EMITTER_COMPONENT));
  //      if (emitterComponent)
  //      {
  //          emitterComponent->GetParticleEmitter()->SetLifeTime(newEmitterLifeTime);
  //      }
  //  }
}

void ParticleEffectNode::Start()
{
  //  int32 childrenCount = GetChildrenCount();
  //  for (int32 i = 0; i < childrenCount; i ++)
  //  {
		//ParticleEmitterComponent * component = static_cast<ParticleEmitterComponent*>(GetChild(i)->GetComponent(Component::PARTICLE_EMITTER_COMPONENT));
		//if(component)
		//{
		//	component->GetParticleEmitter()->Play();
		//}
  //  }

  //  this->emittersCurrentlyStopped = 0;
}

void ParticleEffectNode::Stop()
{
	//int32 childrenCount = GetChildrenCount();
	//for (int32 i = 0; i < childrenCount; i ++)
	//{
	//	ParticleEmitterComponent * component = static_cast<ParticleEmitterComponent*>(GetChild(i)->GetComponent(Component::PARTICLE_EMITTER_COMPONENT));
	//	if(component)
	//	{
	//		component->GetParticleEmitter()->Stop();
	//	}
	//	emittersCurrentlyStopped++;
	//}
}

void ParticleEffectNode::Restart()
{
	//int32 childrenCount = GetChildrenCount();
	//for (int32 i = 0; i < childrenCount; i ++)
	//{
	//	ParticleEmitterComponent * component = static_cast<ParticleEmitterComponent*>(GetChild(i)->GetComponent(Component::PARTICLE_EMITTER_COMPONENT));
	//	if(component)
	//	{
	//		component->GetParticleEmitter()->Restart(true);
	//	}
	//}
}

void ParticleEffectNode::StopAfterNRepeats(int32 numberOfRepeats)
{
    this->stopAfterNRepeats = numberOfRepeats;
}

void ParticleEffectNode::StopWhenEmpty(bool value)
{
    this->stopWhenEmpty = value;
}

void ParticleEffectNode::Update(float32 timeElapsed)
{
  //  int32 childrenCount = GetChildrenCount();
  //  for (int32 i = 0; i < childrenCount; i ++)
  //  {
		//ParticleEmitterComponent * emitterComponent = cast_if_equal<ParticleEmitterComponent*>(GetChild(i)->GetComponent(Component::PARTICLE_EMITTER_COMPONENT));
		//if(!emitterComponent)
		//{
		//	continue;
		//}
  //      ParticleEmitter* emitter = emitterComponent->GetParticleEmitter();
  //      if (IsStopEmitter(emitter))
  //      {
  //          emitter->Stop();
  //          this->emittersCurrentlyStopped ++;

  //          // Are all the emitters finished playback? Notify user if yes.
  //          CheckPlaybackComplete();
  //      }
  //  }
}

void DAVA::ParticleEffectNode::Draw()
{
	Entity::Draw();
}


bool ParticleEffectNode::IsStopEmitter(ParticleEmitter* emitter) const
{
    if (!emitter)
    {
        return true;
    }
    
    if (emitter->IsStopped())
    {
        // Already stopped - no need to stop again.
        return false;
    }

    // Check whether emitter is about to be stopped because of repeats count.
    if ((stopAfterNRepeats > 0) && (emitter->GetRepeatCount() >= stopAfterNRepeats))
    {
        return true;
    }

    // Check whether emitter is about to be stopped because of it is empty.
    if (stopWhenEmpty && emitter->GetParticleCount() == 0)
    {
        return true;
    }
    
    // No rules to stop emitter - continue its playback.
    return false;
}

void ParticleEffectNode::CheckPlaybackComplete()
{
    if (GetChildrenCount() == this->emittersCurrentlyStopped)
    {
        // Playback is finished!
        this->emittersCurrentlyStopped = 0;
        this->playbackComplete(this, 0);
    }
}

void ParticleEffectNode::SetPlaybackCompleteMessage(const Message& msg)
{
    this->playbackComplete = msg;
}

Entity* ParticleEffectNode::Clone(Entity *dstNode /*= NULL*/)
{
	if (!dstNode) 
	{
		DVASSERT_MSG(IsPointerToExactClass<ParticleEffectNode>(this), "Can clone only ParticleEffectNode");
		dstNode = new ParticleEffectNode();
	}

	Entity::Clone(dstNode);
	ParticleEffectNode *nd = (ParticleEffectNode *)dstNode;
	nd->effectDuration = effectDuration;

	return dstNode;
}

