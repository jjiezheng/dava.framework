/*==================================================================================
    Copyright (c) 2008, DAVA Consulting, LLC
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    * Neither the name of the DAVA Consulting, LLC nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE DAVA CONSULTING, LLC AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL DAVA CONSULTING, LLC BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

    Revision History:
        * Created by Vitaliy Borodovsky 
=====================================================================================*/
#include "Render/Highlevel/RenderObject.h"
#include "Base/ObjectFactory.h"
#include "Debug/DVAssert.h"
#include "Utils/Utils.h"

namespace DAVA
{

REGISTER_CLASS(RenderObject)

RenderObject::RenderObject()
    :   type(TYPE_RENDEROBJECT)
    ,   flags(VISIBLE | VISIBLE_LOD | VISIBLE_SWITCH)
    ,   removeIndex(-1)
    ,   debugFlags(0)
    ,   worldTransform(0)
	,	renderSystem(0)
{
    
}
    
RenderObject::~RenderObject()
{
	uint32 size = renderBatchArray.size();
	for(uint32 i = 0; i < size; ++i)
	{
		DVASSERT(renderBatchArray[i]->GetOwnerLayer() == 0);
		renderBatchArray[i]->Release();
	}
}
  
void RenderObject::AddRenderBatch(RenderBatch * batch)
{
	batch->Retain();
	batch->SetRenderObject(this);
    renderBatchArray.push_back(batch);
    if (removeIndex != -1)
    {
        DVASSERT(renderSystem);
		renderSystem->AddRenderBatch(batch);
    }
    
    const AABBox3 & boundingBox = batch->GetBoundingBox();
//    DVASSERT(boundingBox.min.x != AABBOX_INFINITY &&
//             boundingBox.min.y != AABBOX_INFINITY &&
//             boundingBox.min.z != AABBOX_INFINITY);
    
    bbox.AddAABBox(boundingBox);
}

void RenderObject::RemoveRenderBatch(RenderBatch * batch)
{
    if (removeIndex != -1)
    {
        DVASSERT(renderSystem);
		renderSystem->RemoveRenderBatch(batch);
    }
    
    batch->SetRenderObject(0);
	batch->Release();

    FindAndRemoveExchangingWithLast(renderBatchArray, batch);
    RecalcBoundingBox();
}
    
void RenderObject::RecalcBoundingBox()
{
    bbox = AABBox3();
    
    uint32 size = (uint32)renderBatchArray.size();
    for (uint32 k = 0; k < size; ++k)
    {
        bbox.AddAABBox(renderBatchArray[k]->GetBoundingBox());
    }
}
    
uint32 RenderObject::GetRenderBatchCount()
{
    return (uint32)renderBatchArray.size();
}
RenderBatch * RenderObject::GetRenderBatch(uint32 batchIndex)
{
    return renderBatchArray[batchIndex];
}

RenderObject * RenderObject::Clone(RenderObject *newObject)
{
	if(!newObject)
	{
		DVASSERT_MSG(IsPointerToExactClass<RenderObject>(this), "Can clone only RenderObject");
		newObject = new RenderObject();
	}

	newObject->type = type;
	newObject->flags = flags;
	newObject->debugFlags = debugFlags;
	//ro->bbox = bbox;
	//ro->worldBBox = worldBBox;

	uint32 size = GetRenderBatchCount();
	for(uint32 i = 0; i < size; ++i)
	{
        RenderBatch *batch = GetRenderBatch(i)->Clone();
		newObject->AddRenderBatch(batch);
        batch->Release();
	}
    newObject->ownerDebugInfo = ownerDebugInfo;

	return newObject;
}

void RenderObject::Save(KeyedArchive * archive, SceneFileV2* sceneFile)
{
	AnimatedObject::Save(archive);

	if(NULL != archive)
	{
		archive->SetUInt32("ro.type", type);
		archive->SetUInt32("ro.flags", flags);
		archive->SetUInt32("ro.debugflags", debugFlags);
		archive->SetUInt32("ro.batchCount", GetRenderBatchCount());

		KeyedArchive *batchesArch = new KeyedArchive();
		for(uint32 i = 0; i < GetRenderBatchCount(); ++i)
		{
			RenderBatch *batch = GetRenderBatch(i);
			if(NULL != batch)
			{
				KeyedArchive *batchArch = new KeyedArchive();
				batch->Save(batchArch, sceneFile);
				if(batchArch->Count() > 0)
				{
					batchArch->SetString("rb.classname", batch->GetClassName());
				}
				batchesArch->SetArchive(KeyedArchive::GenKeyFromIndex(i), batchArch);
				batchArch->Release();
			} 
		}

		archive->SetArchive("ro.batches", batchesArch);
		batchesArch->Release();
	}
}

void RenderObject::Load(KeyedArchive * archive, SceneFileV2 *sceneFile)
{
	if(NULL != archive)
	{
		if(archive->IsKeyExists("ro.type")) type = archive->GetUInt32("ro.type");
		if(archive->IsKeyExists("ro.flags")) flags = archive->GetUInt32("ro.flags");
		if(archive->IsKeyExists("ro.debugflags")) debugFlags = archive->GetUInt32("ro.debugflags");

		if(archive->IsKeyExists("ro.batchCount"))
		{
			KeyedArchive *batchesArch = archive->GetArchive("ro.batches");
			for(uint32 i = 0; i < archive->GetUInt32("ro.batchCount"); ++i)
			{
				KeyedArchive *batchArch = batchesArch->GetArchive(KeyedArchive::GenKeyFromIndex(i));
				if(NULL != batchArch)
				{
					RenderBatch *batch = (RenderBatch *) ObjectFactory::Instance()->New(batchArch->GetString("rb.classname"));
					if(NULL != batch)
					{
						batch->Load(batchArch, sceneFile);
						AddRenderBatch(batch);
						batch->Release();
					}
				}
			}
		}
	}

	AnimatedObject::Load(archive);
}

void RenderObject::SetRenderSystem(RenderSystem * _renderSystem)
{
	renderSystem = _renderSystem;
}

RenderSystem * RenderObject::GetRenderSystem()
{
	return renderSystem;
}


};
