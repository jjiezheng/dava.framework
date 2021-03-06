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
#include "NotPassableTerrain.h"
#include "LandscapeRenderer.h"

using namespace DAVA;

NotPassableTerrain::NotPassableTerrain()
    : EditorLandscape()
{
    // RETURN TO THIS CODE LATER
    //SetName(String("Landscape_NotPassable"));
    LoadColorsArray();

    notPassableAngleTan = (float32)tan(DegToRad((float32)NotPassableTerrain::NOT_PASSABLE_ANGLE));
    notPassableMapSprite = Sprite::CreateAsRenderTarget((float32)TEXTURE_TILE_FULL_SIZE, (float32)TEXTURE_TILE_FULL_SIZE, DAVA::FORMAT_RGBA8888);
}

NotPassableTerrain::~NotPassableTerrain()
{
    SafeRelease(notPassableMapSprite);
}


void NotPassableTerrain::HeihghtmapUpdated(const DAVA::Rect &forRect)
{
    EditorLandscape::HeihghtmapUpdated(forRect);
    
    AABBox3 boundingBox = nestedLandscape->GetBoundingBox();
    Vector3 landSize = boundingBox.max - boundingBox.min;

    float32 angleCellDistance = landSize.x / (float32)(heightmap->Size() - 1);
    float32 angleHeightDelta = landSize.z / (float32)(Heightmap::MAX_VALUE - 1);
    float32 tanCoef = angleHeightDelta / angleCellDistance;
 
    Texture *notPassableMap = notPassableMapSprite->GetTexture();
    float32 dx = (float32)notPassableMap->GetWidth() / (float32)(heightmap->Size() - 1);
    
    RenderManager::Instance()->LockNonMain();
    RenderManager::Instance()->SetRenderTarget(notPassableMapSprite);

    Rect drawRect(forRect.x * dx, forRect.y * dx, (forRect.dx - 1)* dx, (forRect.dy - 1) * dx);
    RenderManager::Instance()->ClipPush();
    RenderManager::Instance()->ClipRect(drawRect);

    DrawFullTiledTexture(notPassableMap, drawRect);
    
    int32 lastY = (int32)(forRect.y + forRect.dy);
    int32 lastX = (int32)(forRect.x + forRect.dx);
    for (int32 y = (int32)forRect.y; y < lastY; ++y)
    {
        int32 yOffset = y * heightmap->Size();
        for (int32 x = (int32)forRect.x; x < lastX; ++x)
        {
            uint16 currentPoint = heightmap->Data()[yOffset + x];
            uint16 rightPoint = heightmap->Data()[yOffset + x + 1];
            uint16 bottomPoint = heightmap->Data()[yOffset + x + heightmap->Size()];
            
            uint16 deltaRight = (uint16)abs((int32)currentPoint - (int32)rightPoint);
            uint16 deltaBottom = (uint16)abs((int32)currentPoint - (int32)bottomPoint);
            
            float32 tanRight = (float32)deltaRight * tanCoef;
            float32 tanBottom = (float32)deltaBottom * tanCoef;
            
            float32 ydx = y * dx;
            float32 xdx = x * dx;

            RenderManager* renderManager = RenderManager::Instance();
            RenderHelper* renderHelper = RenderHelper::Instance();
            
            Color color;

            if(PickColor(tanRight, color))
            {
                renderManager->SetColor(color);
                renderHelper->DrawLine(Vector2(xdx, ydx), Vector2((xdx + dx), ydx));
            }
            
            if(PickColor(tanBottom, color))
            {
                renderManager->SetColor(color);
                renderHelper->DrawLine(Vector2(xdx, ydx), Vector2(xdx, (ydx + dx)));
            }
            
        }
    }

    RenderManager::Instance()->ResetColor();
    
    RenderManager::Instance()->ClipPop();
    
    RenderManager::Instance()->RestoreRenderTarget();
    RenderManager::Instance()->UnlockNonMain();
}


void NotPassableTerrain::SetDisplayedTexture()
{
    SetTexture(Landscape::TEXTURE_TILE_FULL, notPassableMapSprite->GetTexture());
}

void NotPassableTerrain::LoadColorsArray()
{
    YamlParser* parser = YamlParser::Create("~res:/Configs/LandscapeAngle.yaml");
    
    if (parser != 0)
    {
        YamlNode* rootNode = parser->GetRootNode();
        int32 anglesCount = rootNode->GetCount();
        
        for (int32 i = 0; i < anglesCount; ++i)
        {
            YamlNode* node = rootNode->Get(i);
            if (!node || node->GetCount() != 3)
                continue;
            
            float32 angle1 = node->Get(0)->AsFloat();
            float32 angle2 = node->Get(1)->AsFloat();
            
            angle1 = Min(angle1, 89.f);
            angle2 = Min(angle2, 89.f);
            
            float32 tangentMin = tan(DegToRad(angle1));
            float32 tangentMax = tan(DegToRad(angle2));
            
            YamlNode* colorNode = node->Get(2);
            if (!colorNode || colorNode->GetCount() != 4)
                continue;
         
            Color color(colorNode->Get(0)->AsFloat()/255.f,
                        colorNode->Get(1)->AsFloat()/255.f,
                        colorNode->Get(2)->AsFloat()/255.f,
                        colorNode->Get(3)->AsFloat()/255.f);
            
            angleColor.push_back(TerrainColor(Vector2(tangentMin, tangentMax), color));
        }
    }

    SafeRelease(parser);
}

bool NotPassableTerrain::PickColor(float32 tan, Color& color) const
{
    for (uint32 i = 0; i < angleColor.size(); ++i)
    {
		if(tan >= angleColor[i].angleRange.x && tan < angleColor[i].angleRange.y)
        {
            color = angleColor[i].color;
            return true;
		}
	}
    return false;
}

Texture * NotPassableTerrain::GetDisplayedTexture()
{
    return notPassableMapSprite->GetTexture();
}
