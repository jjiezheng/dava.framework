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

#include "Scene/System/GridSystem.h"

// framework
#include "Render/RenderManager.h"
#include "Render/RenderHelper.h"

SceneGridSystem::SceneGridSystem(DAVA::Scene * scene)
	: DAVA::SceneSystem(scene)
{

}

SceneGridSystem::~SceneGridSystem()
{

}

void SceneGridSystem::Update(float timeElapsed)
{

}

void SceneGridSystem::ProcessUIEvent(DAVA::UIEvent *event)
{

}

void SceneGridSystem::Draw()
{
	const DAVA::float32 GRIDMAX = 500.0f;
	const DAVA::float32 GRIDSTEP = 10.0f;
	DAVA::RenderManager* rm = DAVA::RenderManager::Instance();
	DAVA::RenderHelper* rh = DAVA::RenderHelper::Instance();

	DAVA::uint32 oldState = rm->GetState();	

	rm->SetState(DAVA::RenderState::STATE_COLORMASK_ALL | DAVA::RenderState::STATE_DEPTH_WRITE | DAVA::RenderState::STATE_DEPTH_TEST); 
	rm->SetColor(0.4f, 0.4f, 0.4f, 1.0f);
	for(DAVA::float32 x = -GRIDMAX; x <= GRIDMAX; x += GRIDSTEP)
	{
		DAVA::Vector3 v1(x, -GRIDMAX, 0);
		DAVA::Vector3 v2(x, GRIDMAX, 0);

		DAVA::Vector3 v3(-GRIDMAX, x, 0);
		DAVA::Vector3 v4(GRIDMAX, x, 0);

		if (x!= 0.0f)
		{
			rh->DrawLine(v1, v2);
			rh->DrawLine(v3, v4);		
		}
	}

	rm->SetColor(0.0f, 0.0f, 0.0f, 1.0f);
	rh->DrawLine(DAVA::Vector3(-GRIDMAX, 0, 0), DAVA::Vector3(GRIDMAX, 0, 0));
	rh->DrawLine(DAVA::Vector3(0, -GRIDMAX, 0), DAVA::Vector3(0, GRIDMAX, 0));

	rm->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
	rm->SetState(oldState);
}