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
#include "Render/RenderState.h"
#include "Render/RenderManager.h"
#include "Debug/Backtrace.h"
#include "Render/Shader.h"
#include "Platform/Thread.h"
#include "Utils/Utils.h"
#include "FileSystem/YamlParser.h"


namespace DAVA
{

#if defined(__DAVAENGINE_DIRECTX9__)
IDirect3DDevice9 * RenderState::direct3DDevice = 0; 
#endif

RenderState::RenderState()
{
    renderer = RenderManager::Instance()->GetRenderer();
	Reset(false);
}

RenderState::~RenderState()
{
    
}
    
//#define LOG_FINAL_RENDER_STATE
    
/**
    Function to reset state to original zero state.
 */
void RenderState::Reset(bool doHardwareReset)
{
    state = DEFAULT_2D_STATE;
    color.r = 1.0f;
    color.g = 1.0f;
    color.b = 1.0f;
    color.a = 1.0f;
    sourceFactor = BLEND_ONE;
    destFactor = BLEND_ZERO;
    for (uint32 idx = 0; idx < MAX_TEXTURE_LEVELS; ++idx)
        currentTexture[idx] = 0;
    alphaFunc = CMP_ALWAYS;
    alphaFuncCmpValue = 0;
	depthFunc = CMP_LESS;
    shader = 0;
    cullMode = FACE_BACK;
	scissorRect = Rect(0, 0, 0, 0);
	fillMode = FILLMODE_SOLID;
    
    if (doHardwareReset)
    {
        RenderManager::Instance()->LockNonMain();
//        Logger::Debug("Do hardware reset");
        // PrintBackTraceToLog();
        SetColorInHW();
        SetEnableBlendingInHW();
        SetBlendModeInHW();
        SetDepthTestInHW();
        SetDepthWriteInHW();
        SetColorMaskInHW();
        
        if (renderer != Core::RENDERER_OPENGL_ES_2_0)
        {
            SetAlphaTestInHW();
            SetAlphaTestFuncInHW();
        }
        
        for (uint32 textureLevel = 0; textureLevel < MAX_TEXTURE_LEVELS; ++textureLevel)
        {
            SetTextureLevelInHW(textureLevel);
        }
        RenderManager::Instance()->UnlockNonMain();

    }
}
bool RenderState::IsEqual(RenderState * anotherState)
{
    if (state != anotherState->state)
        return false;
    
    // check texture first for early rejection 
    if (currentTexture[0] != anotherState->currentTexture[0])return false;

    if (state & STATE_BLEND)
    {
        if (destFactor != anotherState->destFactor)return false;
        if (sourceFactor != anotherState->sourceFactor)return false;
    }
    
    if (color != anotherState->color)return false;
    
    if (currentTexture[1] != anotherState->currentTexture[1])return false;
    if (currentTexture[2] != anotherState->currentTexture[2])return false;
    if (currentTexture[3] != anotherState->currentTexture[3])return false;
    
    
    
    
    return true;
}

void RenderState::Flush(RenderState * hardwareState) const
{
    RenderManager::Instance()->LockNonMain();

#if defined (LOG_FINAL_RENDER_STATE)
    Logger::Debug("RenderState::Flush started");
#endif    

    uint32 diffState = state ^ hardwareState->state;
    if (diffState != 0)
    {
        if (diffState & STATE_BLEND)
            SetEnableBlendingInHW();
        
        if (diffState & STATE_DEPTH_TEST)
            SetDepthTestInHW();
        
        if (diffState & STATE_DEPTH_WRITE)
            SetDepthWriteInHW();
        
        if (diffState & STATE_CULL)
            SetCullInHW();
        
        if (diffState & STATE_COLORMASK_ALL)
            SetColorMaskInHW();

		if (diffState & STATE_STENCIL_TEST)
			SetStensilTestInHW();

		if (diffState & STATE_SCISSOR_TEST)
			SetScissorTestInHW();

        
        if (renderer != Core::RENDERER_OPENGL_ES_2_0)
            if (diffState & STATE_ALPHA_TEST)
                SetAlphaTestInHW();
                
		if (renderer != Core::RENDERER_OPENGL_ES_2_0)
		{
			for (uint32 textureLevel = 0; textureLevel < MAX_TEXTURE_LEVELS; ++textureLevel)
			{	
				if (diffState & (STATE_TEXTURE0 << textureLevel))
				{
					if (state & (STATE_TEXTURE0 << textureLevel))
					{
						RENDER_VERIFY(glActiveTexture(GL_TEXTURE0 + textureLevel));
						RENDER_VERIFY(glEnable(GL_TEXTURE_2D));
					}
					else 
					{
						RENDER_VERIFY(glActiveTexture(GL_TEXTURE0 + textureLevel));
						RENDER_VERIFY(glDisable(GL_TEXTURE_2D));
					}
				}
			}
		}

        hardwareState->state = state;
    }
    
    //if (changeSet != 0)
    {
        if (color != hardwareState->color)
        {
            SetColorInHW();
            hardwareState->color = color;
        }
        if (sourceFactor != hardwareState->sourceFactor || destFactor != hardwareState->destFactor)
        {
            SetBlendModeInHW();
            hardwareState->sourceFactor = sourceFactor;
            hardwareState->destFactor = destFactor;
        }
        
        if (cullMode != hardwareState->cullMode)
        {
            SetCullModeInHW();
            hardwareState->cullMode = cullMode;
        }
        
        if (renderer != Core::RENDERER_OPENGL_ES_2_0)
            if ((alphaFunc != hardwareState->alphaFunc) || (alphaFuncCmpValue != hardwareState->alphaFuncCmpValue))
            {
                SetAlphaTestFuncInHW();
                hardwareState->alphaFunc = alphaFunc;
                hardwareState->alphaFuncCmpValue = alphaFuncCmpValue;
            }

        if (hardwareState->depthFunc != depthFunc)
        {
            SetDepthFuncInHW();
            hardwareState->depthFunc = depthFunc;
        }

        if(hardwareState->scissorRect != scissorRect)
        {
            SetScissorRectInHW();
            hardwareState->scissorRect = scissorRect;
        }
        
		if(hardwareState->fillMode != fillMode)
		{
			SetFillModeInHW();
			hardwareState->fillMode = fillMode;
		}

        //if (changeSet & STATE_CHANGED_TEXTURE0)
		if (currentTexture[0] != hardwareState->currentTexture[0])	
		{
            SetTextureLevelInHW(0);
            hardwareState->currentTexture[0] = currentTexture[0];
        }
		//  if (changeSet & STATE_CHANGED_TEXTURE1)
		if (currentTexture[1] != hardwareState->currentTexture[1])	
        {
            SetTextureLevelInHW(1);
            hardwareState->currentTexture[1] = currentTexture[1];
        }
		//  if (changeSet & STATE_CHANGED_TEXTURE2)
		if (currentTexture[2] != hardwareState->currentTexture[2])	
        {
            SetTextureLevelInHW(2);
            hardwareState->currentTexture[2] = currentTexture[2];
        }
        //if (changeSet & STATE_CHANGED_TEXTURE3)
		if (currentTexture[3] != hardwareState->currentTexture[3])		
		{
            SetTextureLevelInHW(3);
            hardwareState->currentTexture[3] = currentTexture[3];
        }
		//if (changeSet & STATE_CHANGED_TEXTURE4)
		if (currentTexture[4] != hardwareState->currentTexture[4])
		{
			SetTextureLevelInHW(4);
			hardwareState->currentTexture[4] = currentTexture[4];
		}
		//if (changeSet & STATE_CHANGED_TEXTURE5)
		if (currentTexture[5] != hardwareState->currentTexture[5])
		{
			SetTextureLevelInHW(5);
			hardwareState->currentTexture[5] = currentTexture[5];
		}
		//if (changeSet & STATE_CHANGED_TEXTURE6)
		if (currentTexture[6] != hardwareState->currentTexture[6])
		{
			SetTextureLevelInHW(6);
			hardwareState->currentTexture[6] = currentTexture[6];
		}
		//if (changeSet & STATE_CHANGED_TEXTURE7)
		if (currentTexture[7] != hardwareState->currentTexture[7])
		{
			SetTextureLevelInHW(7);
			hardwareState->currentTexture[7] = currentTexture[7];
		}

		//if (changeSet & STATE_CHANGED_STENCIL_REF)
		//if (hardwareState->stencilState.ref != stencilState.ref)
		//{
		//	SetStencilRefInHW();
		//	hardwareState->stencilState.ref = stencilState.ref;
		//}
		////if (changeSet & STATE_CHANGED_STENCIL_MASK)
		//if(hardwareState->stencilState.mask != stencilState.mask)
		//{
		//	SetStencilMaskInHW();
		//	hardwareState->stencilState.mask = stencilState.mask;
		//}
		//if (changeSet & STATE_CHANGED_STENCIL_FUNC)
		if (hardwareState->stencilState.func[0] != stencilState.func[0] || 
			hardwareState->stencilState.func[1] != stencilState.func[1] ||
			hardwareState->stencilState.mask != stencilState.mask ||
			hardwareState->stencilState.ref != stencilState.ref)
		{
			SetStencilFuncInHW();
			hardwareState->stencilState.func[0] = stencilState.func[0];
			hardwareState->stencilState.func[1] = stencilState.func[1];

			hardwareState->stencilState.ref = stencilState.ref;
			hardwareState->stencilState.mask = stencilState.mask;
		}

		if (hardwareState->stencilState.pass[0] != stencilState.pass[0] ||
			hardwareState->stencilState.pass[1] != stencilState.pass[1] ||
			hardwareState->stencilState.fail[0] != stencilState.fail[0] ||
			hardwareState->stencilState.fail[1] != stencilState.fail[1] ||
			hardwareState->stencilState.zFail[0] != stencilState.zFail[0] ||
			hardwareState->stencilState.zFail[1] != stencilState.zFail[1])
		{
			SetStencilOpInHW();
			hardwareState->stencilState.pass[0] = stencilState.pass[0];
			hardwareState->stencilState.pass[1] = stencilState.pass[1];
			hardwareState->stencilState.fail[0] = stencilState.fail[0];
			hardwareState->stencilState.fail[1] = stencilState.fail[1];
			hardwareState->stencilState.zFail[0] = stencilState.zFail[0];
			hardwareState->stencilState.zFail[1] = stencilState.zFail[1];
		}

		if (hardwareState->stencilState.pass[0] != stencilState.pass[0] || 
			hardwareState->stencilState.pass[1] != stencilState.pass[1])
		{
			SetStencilPassInHW();
			hardwareState->stencilState.pass[0] = stencilState.pass[0];
			hardwareState->stencilState.pass[1] = stencilState.pass[1];
		}

		if (hardwareState->stencilState.fail[0] != stencilState.fail[0] ||
			hardwareState->stencilState.fail[1] != stencilState.fail[1])
		{
			SetStencilFailInHW();
			hardwareState->stencilState.fail[0] = stencilState.fail[0];
			hardwareState->stencilState.fail[1] = stencilState.fail[1];
		}

		if (hardwareState->stencilState.zFail[0] != stencilState.zFail[0] || 
			hardwareState->stencilState.zFail[1] != stencilState.zFail[1])
		{
			SetStencilZFailInHW();
			hardwareState->stencilState.zFail[0] = stencilState.zFail[0];
			hardwareState->stencilState.zFail[1] = stencilState.zFail[1];
		}

#if defined(__DAVAENGINE_OPENGL__)
        RENDER_VERIFY(glActiveTexture(GL_TEXTURE0));
#endif
//        if (changeSet & STATE_CHANGED_SHADER)
//        {
//            if (shader != previousState->shader)
//            {
//            }
//        }

    }
    if (shader)shader->Bind();
    else Shader::Unbind();
    hardwareState->shader = shader;
    
#if defined (LOG_FINAL_RENDER_STATE)
    Logger::Debug("RenderState::Flush finished");
#endif    
    RenderManager::Instance()->UnlockNonMain();

}
    
    
#if defined (__DAVAENGINE_OPENGL__)
inline void RenderState::SetColorInHW() const
{
    if (renderer != Core::RENDERER_OPENGL_ES_2_0)
    {
#if defined (LOG_FINAL_RENDER_STATE)
        Logger::Debug("RenderState::color = (%f, %f, %f, %f)", color.r, color.g, color.b, color.a);
#endif
        RENDER_VERIFY(glColor4f(color.r, color.g, color.b, color.a));
    }
}
    
inline void RenderState::SetColorMaskInHW() const
{
    GLboolean redMask = (state & STATE_COLORMASK_RED) != 0;
    GLboolean greenMask = (state & STATE_COLORMASK_GREEN) != 0;
    GLboolean blueMask = (state & STATE_COLORMASK_BLUE) != 0;
    GLboolean alphaMask = (state & STATE_COLORMASK_ALPHA) != 0;
    
#if defined (LOG_FINAL_RENDER_STATE)
    Logger::Debug("RenderState::colormask = %d %d %d %d", redMask, greenMask, blueMask, alphaMask);
#endif    
    RENDER_VERIFY(glColorMask(redMask, 
                              greenMask, 
                              blueMask, 
                              alphaMask));
}

inline void RenderState::SetStensilTestInHW() const
{
	if (state & STATE_STENCIL_TEST)
	{
#if defined (LOG_FINAL_RENDER_STATE)
        Logger::Debug("RenderState::stencil = true");
#endif    
		RENDER_VERIFY(glEnable(GL_STENCIL_TEST));
	}
	else
	{
#if defined (LOG_FINAL_RENDER_STATE)
        Logger::Debug("RenderState::stencil = false");
#endif    
		RENDER_VERIFY(glDisable(GL_STENCIL_TEST));
	}
}

inline void RenderState::SetEnableBlendingInHW() const
{
    if (state & STATE_BLEND)
    {
#if defined (LOG_FINAL_RENDER_STATE)
        Logger::Debug("RenderState::blend = true");
#endif    
        RENDER_VERIFY(glEnable(GL_BLEND));
    }
    else 
    {
#if defined (LOG_FINAL_RENDER_STATE)
        Logger::Debug("RenderState::blend = false");
#endif    
        RENDER_VERIFY(glDisable(GL_BLEND));
    }
}
    
inline void RenderState::SetCullInHW() const
{
    if (state & STATE_CULL)
    {
#if defined (LOG_FINAL_RENDER_STATE)
        Logger::Debug("RenderState::cullface = true");
#endif    

        RENDER_VERIFY(glEnable(GL_CULL_FACE));
    }
    else 
    {
#if defined (LOG_FINAL_RENDER_STATE)
        Logger::Debug("RenderState::cullface = false");
#endif    
        RENDER_VERIFY(glDisable(GL_CULL_FACE));
    }
}

inline void RenderState::SetCullModeInHW() const
{
#if defined (LOG_FINAL_RENDER_STATE)
    Logger::Debug("RenderState::cull_mode = %d", cullMode);
#endif    

    RENDER_VERIFY(glCullFace(CULL_FACE_MAP[cullMode]));
}


inline void RenderState::SetBlendModeInHW() const
{
#if defined (LOG_FINAL_RENDER_STATE)
    Logger::Debug("RenderState::blend_src_dst = (%d, %d)", sourceFactor, destFactor);
#endif    

    RENDER_VERIFY(glBlendFunc(BLEND_MODE_MAP[sourceFactor], BLEND_MODE_MAP[destFactor]));
}

inline void RenderState::SetTextureLevelInHW(uint32 textureLevel) const
{
	RENDER_VERIFY(glActiveTexture(GL_TEXTURE0 + textureLevel));
    if(currentTexture[textureLevel])
    {
#if defined (LOG_FINAL_RENDER_STATE)
        Logger::Debug("RenderState::bind_texture %d = (%d)", textureLevel, currentTexture[textureLevel]->id);
#endif    
        RenderManager::Instance()->HWglBindTexture(currentTexture[textureLevel]->id);
    }else
    {
#if defined (LOG_FINAL_RENDER_STATE)
        Logger::Debug("RenderState::bind_texture %d = (%d)", textureLevel, 0);
#endif    
        RenderManager::Instance()->HWglBindTexture(0);
    }    
}
inline void RenderState::SetDepthTestInHW() const
{
    if(state & STATE_DEPTH_TEST)
    {
#if defined (LOG_FINAL_RENDER_STATE)
        Logger::Debug("RenderState::depth_test = true");
#endif    
        RENDER_VERIFY(glEnable(GL_DEPTH_TEST));
    }
    else
    {
#if defined (LOG_FINAL_RENDER_STATE)
        Logger::Debug("RenderState::depth_test = false");
#endif    
        RENDER_VERIFY(glDisable(GL_DEPTH_TEST));
    }    
}

inline void RenderState::SetDepthWriteInHW() const
{
    if(state & STATE_DEPTH_WRITE)
    {
#if defined (LOG_FINAL_RENDER_STATE)
        Logger::Debug("RenderState::depth_mask = true");
#endif    

        RENDER_VERIFY(glDepthMask(GL_TRUE));
    }
    else
    {
#if defined (LOG_FINAL_RENDER_STATE)
        Logger::Debug("RenderState::depth_mask = false");
#endif    
        RENDER_VERIFY(glDepthMask(GL_FALSE));
    }
}
    
inline void RenderState::SetAlphaTestInHW() const
{
    if(state & STATE_ALPHA_TEST)
    {
#if defined (LOG_FINAL_RENDER_STATE)
        Logger::Debug("RenderState::alpha_test = true");
#endif    

        RENDER_VERIFY(glEnable(GL_ALPHA_TEST));
    }
    else
    {
#if defined (LOG_FINAL_RENDER_STATE)
        Logger::Debug("RenderState::alpha_test = false");
#endif    
        RENDER_VERIFY(glDisable(GL_ALPHA_TEST));
    }
}

inline void RenderState::SetAlphaTestFuncInHW() const
{
    if (renderer == Core::RENDERER_OPENGL)
    {
#if defined (LOG_FINAL_RENDER_STATE)
        Logger::Debug("RenderState::alpha func = (%d, %d)", alphaFunc, alphaFuncCmpValue);
#endif    

        RENDER_VERIFY(glAlphaFunc(COMPARE_FUNCTION_MAP[alphaFunc], (float32)alphaFuncCmpValue / 255.0f) );
    }else
    {
#if defined (LOG_FINAL_RENDER_STATE)
        Logger::Debug("RenderState::alpha func = (%d, %d)", alphaFunc, alphaFuncCmpValue);
#endif    
        RENDER_VERIFY(glAlphaFunc(COMPARE_FUNCTION_MAP[alphaFunc], alphaFuncCmpValue) );
    }
}

inline void RenderState::SetDepthFuncInHW() const
{
#if defined (LOG_FINAL_RENDER_STATE)
    Logger::Debug("RenderState::depth func = (%d)", depthFunc);
#endif    

	RENDER_VERIFY(glDepthFunc(COMPARE_FUNCTION_MAP[depthFunc]));
}

inline void RenderState::SetScissorTestInHW() const
{
	if(state & STATE_SCISSOR_TEST)
	{
#if defined (LOG_FINAL_RENDER_STATE)
        Logger::Debug("RenderState::scissor_test = true");
#endif  
		RENDER_VERIFY(glEnable(GL_SCISSOR_TEST));
	}
	else
	{
#if defined (LOG_FINAL_RENDER_STATE)
        Logger::Debug("RenderState::scissor_test = false");
#endif  
		RENDER_VERIFY(glDisable(GL_SCISSOR_TEST));
	}
}

inline void RenderState::SetScissorRectInHW() const
{
#if defined (LOG_FINAL_RENDER_STATE)
    Logger::Debug("RenderState::scissor_rect = (%d, %d, %d, %d)", scissorRect.x, scissorRect.y, scissorRect.dx, scissorRect.dy);
#endif  

	RENDER_VERIFY(glScissor((GLint)scissorRect.x, (GLint)scissorRect.y, (GLsizei)scissorRect.dx, (GLsizei)scissorRect.dy));
}

inline void RenderState::SetFillModeInHW() const
{
#if defined(__DAVAENGINE_MACOS__) || defined (__DAVAENGINE_WIN32__)
	RENDER_VERIFY(glPolygonMode(GL_FRONT_AND_BACK, FILLMODE_MAP[fillMode]));
#endif
}

inline void RenderState::SetStencilRefInHW() const
{
}

inline void RenderState::SetStencilMaskInHW() const
{
}

inline void RenderState::SetStencilFuncInHW() const
{
	if(stencilState.func[0] == (stencilState.func[1]))
	{
		RENDER_VERIFY(glStencilFunc(COMPARE_FUNCTION_MAP[stencilState.func[0]], stencilState.ref, stencilState.mask));
	}
	else
	{
		RENDER_VERIFY(glStencilFuncSeparate(CULL_FACE_MAP[FACE_FRONT], COMPARE_FUNCTION_MAP[stencilState.func[0]], stencilState.ref, stencilState.mask));
		RENDER_VERIFY(glStencilFuncSeparate(CULL_FACE_MAP[FACE_BACK], COMPARE_FUNCTION_MAP[stencilState.func[1]], stencilState.ref, stencilState.mask));
	}
}

inline void RenderState::SetStencilPassInHW() const
{
	//nothing
}

inline void RenderState::SetStencilFailInHW() const
{
	//nothing
}

inline void RenderState::SetStencilZFailInHW() const
{
	//nothing
}

inline void RenderState::SetStencilOpInHW() const
{
	RENDER_VERIFY(glStencilOpSeparate(CULL_FACE_MAP[FACE_FRONT], STENCIL_OP_MAP[stencilState.fail[0]], STENCIL_OP_MAP[stencilState.zFail[0]], STENCIL_OP_MAP[stencilState.pass[0]]));
	RENDER_VERIFY(glStencilOpSeparate(CULL_FACE_MAP[FACE_BACK], STENCIL_OP_MAP[stencilState.fail[1]], STENCIL_OP_MAP[stencilState.zFail[1]], STENCIL_OP_MAP[stencilState.pass[1]]));
}

    
#elif defined(__DAVAENGINE_DIRECTX9__)
    
inline void RenderState::SetColorInHW()
{
	//if (renderer != Core::RENDERER_OPENGL_ES_2_0)
	//	RENDER_VERIFY(glColor4f(color.r * color.a, color.g * color.a, color.b * color.a, color.a));
}
    
inline void RenderState::SetColorMaskInHW()
{
    DWORD flags = 0;
    if (state & STATE_COLORMASK_RED)
        flags |= D3DCOLORWRITEENABLE_RED;
    
    if (state & STATE_COLORMASK_GREEN)
        flags |= D3DCOLORWRITEENABLE_GREEN;
    
    if (state & STATE_COLORMASK_BLUE)
        flags |= D3DCOLORWRITEENABLE_BLUE;
    
    if (state & STATE_COLORMASK_ALPHA)
        flags |= D3DCOLORWRITEENABLE_ALPHA;
    
    RENDER_VERIFY(direct3DDevice->SetRenderState(D3DRS_COLORWRITEENABLE, flags));
}

inline void RenderState::SetEnableBlendingInHW()
{
	RENDER_VERIFY(direct3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, (state & STATE_BLEND) != 0));
}

inline void RenderState::SetCullInHW()
{
	if (!(state & STATE_CULL))
	{
		RENDER_VERIFY(direct3DDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE));
	}
}

inline void RenderState::SetCullModeInHW()
{
	if ((state & STATE_CULL))
	{
		RENDER_VERIFY(direct3DDevice->SetRenderState(D3DRS_CULLMODE, CULL_FACE_MAP[cullMode]));
	}
}


inline void RenderState::SetBlendModeInHW()
{
	RENDER_VERIFY(direct3DDevice->SetRenderState(D3DRS_SRCBLEND, BLEND_MODE_MAP[sourceFactor]));
	RENDER_VERIFY(direct3DDevice->SetRenderState(D3DRS_DESTBLEND, BLEND_MODE_MAP[destFactor]));
}

inline void RenderState::SetTextureLevelInHW(uint32 textureLevel)
{
	IDirect3DTexture9 * texture = 0;
	if(currentTexture[textureLevel])
		texture = currentTexture[textureLevel]->id;

	if (state & (STATE_TEXTURE0 << textureLevel))
	{
		RENDER_VERIFY(direct3DDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE ));
		RENDER_VERIFY(direct3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE ));
	}else 
	{
		RENDER_VERIFY(direct3DDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG2 ));
		RENDER_VERIFY(direct3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2));
	}
	RENDER_VERIFY(direct3DDevice->SetTexture(textureLevel, texture));
}
inline void RenderState::SetDepthTestInHW()
{
	RENDER_VERIFY(direct3DDevice->SetRenderState(D3DRS_ZENABLE, (state & STATE_DEPTH_TEST) != 0));
}

inline void RenderState::SetDepthWriteInHW()
{
	RENDER_VERIFY(direct3DDevice->SetRenderState(D3DRS_ZWRITEENABLE , (state & STATE_DEPTH_WRITE) !=0));
}

inline void RenderState::SetAlphaTestInHW()
{
	RENDER_VERIFY(direct3DDevice->SetRenderState(D3DRS_ALPHATESTENABLE, (state & STATE_ALPHA_TEST) != 0));
}

inline void RenderState::SetAlphaTestFuncInHW()
{
	RENDER_VERIFY(direct3DDevice->SetRenderState(D3DRS_ALPHAFUNC, COMPARE_FUNCTION_MAP[alphaFunc]));
	RENDER_VERIFY(direct3DDevice->SetRenderState(D3DRS_ALPHAREF , alphaFuncCmpValue));
}

inline void RenderState::SetDepthFuncInHW()
{
	RENDER_VERIFY(direct3DDevice->SetRenderState(D3DRS_ZFUNC, COMPARE_FUNCTION_MAP[alphaFunc]));
}

inline void RenderState::SetScissorTestInHW()
{
	RENDER_VERIFY(direct3DDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, (state & STATE_SCISSOR_TEST)));
}

inline void RenderState::SetScissorRectInHW()
{
	RECT rect;
	rect.left = (LONG)scissorRect.x;
	rect.right = (LONG)scissorRect.dx;
	rect.top = (LONG)scissorRect.y;
	rect.bottom = (LONG)scissorRect.dy;
	RENDER_VERIFY(direct3DDevice->SetScissorRect(&rect));
}

inline void RenderState::SetStensilTestInHW()
{
	RENDER_VERIFY(direct3DDevice->SetRenderState(D3DRS_STENCILENABLE, (state & STATE_STENCIL_TEST) != 0));
}

inline void RenderState::SetStencilRefInHW()
{
	RENDER_VERIFY(direct3DDevice->SetRenderState(D3DRS_STENCILREF, stencilState.ref));
}

inline void RenderState::SetStencilMaskInHW()
{
	RENDER_VERIFY(direct3DDevice->SetRenderState(D3DRS_STENCILMASK, stencilState.mask));
}

inline void RenderState::SetStencilFuncInHW()
{
	RENDER_VERIFY(direct3DDevice->SetRenderState(D3DRS_STENCILFUNC, stencilState.func[0]));
	RENDER_VERIFY(direct3DDevice->SetRenderState(D3DRS_CCW_STENCILFUNC, stencilState.func[1]));
	RENDER_VERIFY(direct3DDevice->SetRenderState(D3DRS_STENCILREF, stencilState.ref));
	RENDER_VERIFY(direct3DDevice->SetRenderState(D3DRS_STENCILMASK, stencilState.mask));
}

inline void RenderState::SetStencilPassInHW()
{
	RENDER_VERIFY(direct3DDevice->SetRenderState(D3DRS_STENCILPASS, stencilState.pass[0]));
	RENDER_VERIFY(direct3DDevice->SetRenderState(D3DRS_CCW_STENCILPASS, stencilState.pass[1]));
}

inline void RenderState::SetStencilFailInHW()
{
	RENDER_VERIFY(direct3DDevice->SetRenderState(D3DRS_STENCILFAIL, stencilState.fail[0]));
	RENDER_VERIFY(direct3DDevice->SetRenderState(D3DRS_CCW_STENCILFAIL, stencilState.fail[1]));
}

inline void RenderState::SetStencilZFailInHW()
{
	RENDER_VERIFY(direct3DDevice->SetRenderState(D3DRS_STENCILZFAIL, stencilState.zFail[0]));
	RENDER_VERIFY(direct3DDevice->SetRenderState(D3DRS_CCW_STENCILZFAIL, stencilState.zFail[1]));
}

inline void RenderState::SetStencilOpInHW()
{
}


#endif 
    
RenderState::StencilState::StencilState()
{
	ref = 0;
	mask = 0xFFFFFFFF;
	func[0] = func[1] = CMP_ALWAYS;
	pass[0] = pass[1] = STENCILOP_KEEP;
	fail[0] = fail[1] = STENCILOP_KEEP;
	zFail[0] = zFail[1] = STENCILOP_KEEP;
} 

void RenderState::LoadFromYamlFile(const String & filePath)
{
	YamlParser * parser = YamlParser::Create(filePath);
	if (!parser)
	{
		Logger::Error("Failed to open yaml file: %s", filePath.c_str());
		return;
	}

	YamlNode * rootNode = parser->GetRootNode();
	if (!rootNode)
	{
		Logger::Warning("yaml file: %s is empty", filePath.c_str());
		return;
	}

	LoadFromYamlNode(rootNode);

	SafeRelease(parser);
}

void RenderState::LoadFromYamlNode(YamlNode * rootNode)
{
	if (!rootNode)
		return;

	YamlNode * renderStateNode = rootNode->Get("RenderState");
	if(renderStateNode)
	{
		YamlNode * stateNode = renderStateNode->Get("state");
		if(stateNode)
		{
			Vector<String> states;
			Split(stateNode->AsString(), "| ", states);
			uint32 currentState = 0;
			for(Vector<String>::const_iterator it = states.begin(); it != states.end(); it++)
				currentState |= GetRenderStateByName((*it));
			
			state = currentState;
		}

		YamlNode * blendSrcNode = renderStateNode->Get("blendSrc");
		YamlNode * blendDestNode = renderStateNode->Get("blendDest");
		if(blendSrcNode && blendDestNode)
		{
			eBlendMode newBlendScr = GetBlendModeByName(blendSrcNode->AsString());
			eBlendMode newBlendDest = GetBlendModeByName(blendDestNode->AsString());
			SetBlendMode(newBlendScr, newBlendDest);
		}

		YamlNode * cullModeNode = renderStateNode->Get("cullMode");
		if(cullModeNode)
		{
			int32 newCullMode = (int32)GetFaceByName(cullModeNode->AsString());
			SetCullMode(newCullMode);
		}

		YamlNode * depthFuncNode = renderStateNode->Get("depthFunc");
		if(depthFuncNode)
		{
			eCmpFunc newDepthFunc = GetCmpFuncByName(depthFuncNode->AsString());
			SetDepthFunc(newDepthFunc);
		}

		YamlNode * alphaFuncNode = renderStateNode->Get("alphaFunc");
		YamlNode * alphaFuncCmpValueNode = renderStateNode->Get("alphaFuncCmpValue");
		if(alphaFuncNode && alphaFuncCmpValueNode)
		{
			eCmpFunc newAlphaFunc = GetCmpFuncByName(alphaFuncNode->AsString());
			float32 newCmpValue = alphaFuncCmpValueNode->AsFloat();
			SetAlphaFunc(newAlphaFunc, newCmpValue);
		}

		YamlNode * stencilNode = renderStateNode->Get("stencil");
		if(stencilNode)
		{
			YamlNode * stencilRefNode = stencilNode->Get("ref");
			if(stencilRefNode)
				SetStencilRef(stencilRefNode->AsInt32());

			YamlNode * stencilMaskNode = stencilNode->Get("mask");
			if(stencilMaskNode)
				SetStencilMask(stencilMaskNode->AsUInt32());

			YamlNode * stencilFuncNode = stencilNode->Get("funcFront");
			if(stencilFuncNode)
				SetStencilFunc(FACE_FRONT, GetCmpFuncByName(stencilFuncNode->AsString()));
			stencilFuncNode = stencilNode->Get("funcBack");
			if(stencilFuncNode)
				SetStencilFunc(FACE_BACK, GetCmpFuncByName(stencilFuncNode->AsString()));

			YamlNode * stencilPassNode = stencilNode->Get("passFront");
			if(stencilPassNode)
				SetStencilPass(FACE_FRONT, GetStencilOpByName(stencilPassNode->AsString()));
			stencilPassNode = stencilNode->Get("passBack");
			if(stencilPassNode)
				SetStencilPass(FACE_BACK, GetStencilOpByName(stencilPassNode->AsString()));

			YamlNode * stencilFailNode = stencilNode->Get("failFront");
			if(stencilFailNode)
				SetStencilFail(FACE_FRONT, GetStencilOpByName(stencilFailNode->AsString()));
			stencilFailNode = stencilNode->Get("failBack");
			if(stencilFailNode)
				SetStencilFail(FACE_BACK, GetStencilOpByName(stencilFailNode->AsString()));

			YamlNode * stencilZFailNode = stencilNode->Get("zFailFront");
			if(stencilZFailNode)
				SetStencilZFail(FACE_FRONT, GetStencilOpByName(stencilZFailNode->AsString()));
			stencilZFailNode = stencilNode->Get("zFailBack");
			if(stencilZFailNode)
				SetStencilZFail(FACE_BACK, GetStencilOpByName(stencilZFailNode->AsString()));
		}
	}
}

bool RenderState::SaveToYamlFile(const String & filePath)
{
	YamlParser * parser = YamlParser::Create();
	DVASSERT(parser);

	YamlNode* resultNode = SaveToYamlNode();
	parser->SaveToYamlFile(filePath, resultNode, true);

	SafeRelease(parser);

	return true;
}
	
YamlNode * RenderState::SaveToYamlNode(YamlNode * parentNode /* = 0 */)
{
	if(!parentNode)
		parentNode = new YamlNode(YamlNode::TYPE_MAP);

	YamlNode * rootNode = new YamlNode(YamlNode::TYPE_MAP);

	YamlNode * stencilNode = new YamlNode(YamlNode::TYPE_MAP);
	stencilNode->Add("ref", stencilState.ref);
	stencilNode->Add("mask", (int32)stencilState.mask);
	stencilNode->Add("funcFront", CmpFuncNames[(int32)stencilState.func[FACE_FRONT]]);
	stencilNode->Add("funcBack", CmpFuncNames[(int32)stencilState.func[FACE_BACK]]);
	stencilNode->Add("passFront", StencilOpNames[(int32)stencilState.pass[FACE_FRONT]]);
	stencilNode->Add("passBack", StencilOpNames[(int32)stencilState.pass[FACE_BACK]]);
	stencilNode->Add("failFront", StencilOpNames[(int32)stencilState.fail[FACE_FRONT]]);
	stencilNode->Add("failBack", StencilOpNames[(int32)stencilState.fail[FACE_BACK]]);
	stencilNode->Add("zFailFront", StencilOpNames[(int32)stencilState.zFail[FACE_FRONT]]);
	stencilNode->Add("zFailBack", StencilOpNames[(int32)stencilState.zFail[FACE_BACK]]);
	rootNode->AddNodeToMap("stencil", stencilNode);

	rootNode->Add("blendSrc", BlendModeNames[(int32)sourceFactor]);
	rootNode->Add("blendDest", BlendModeNames[(int32)destFactor]);
	rootNode->Add("cullMode", FaceNames[(int32)cullMode]);
	rootNode->Add("depthFunc", CmpFuncNames[(int32)depthFunc]);
	rootNode->Add("alphaFunc", CmpFuncNames[(int32)alphaFunc]);
	rootNode->Add("alphaFuncCmpValue", alphaFuncCmpValue/255.f);

	Vector<String> statesStrs;
	GetCurrentStateStrings(statesStrs);
	String stateString;
	for(Vector<String>::const_iterator it = statesStrs.begin(); it != statesStrs.end(); it++)
	{
		stateString += (*it);
		if((it+1) != statesStrs.end())
			stateString += " | ";
	}

	rootNode->Add("state", stateString);

	parentNode->AddNodeToMap("RenderState", rootNode);

	return parentNode;
}

void RenderState::GetCurrentStateStrings(Vector<String> & statesStrs)
{
	statesStrs.clear();

	if(state & STATE_BLEND)
		statesStrs.push_back("STATE_BLEND");
	if(state & STATE_DEPTH_TEST)
		statesStrs.push_back("STATE_DEPTH_TEST");
	if(state & STATE_DEPTH_WRITE)
		statesStrs.push_back("STATE_DEPTH_WRITE");
	if(state & STATE_STENCIL_TEST)
		statesStrs.push_back("STATE_STENCIL_TEST");
	if(state & STATE_CULL)
		statesStrs.push_back("STATE_CULL");
	if(state & STATE_ALPHA_TEST)
		statesStrs.push_back("STATE_ALPHA_TEST");
	if(state & STATE_SCISSOR_TEST)
		statesStrs.push_back("STATE_SCISSOR_TEST");
	if(state & STATE_COLORMASK_RED)
		statesStrs.push_back("STATE_COLORMASK_RED");
	if(state & STATE_COLORMASK_GREEN)
		statesStrs.push_back("STATE_COLORMASK_GREEN");
	if(state & STATE_COLORMASK_BLUE)
		statesStrs.push_back("STATE_COLORMASK_BLUE");
	if(state & STATE_COLORMASK_ALPHA)
		statesStrs.push_back("STATE_COLORMASK_ALPHA");
}

uint32 RenderState::GetRenderStateByName(const String & str)
{
	if(str == "STATE_BLEND")
		return (uint32)STATE_BLEND;
	else if (str == "STATE_DEPTH_TEST")
		return (uint32)STATE_DEPTH_TEST;
	else if (str == "STATE_DEPTH_WRITE")
		return (uint32)STATE_DEPTH_WRITE;
	else if (str == "STATE_STENCIL_TEST")
		return (uint32)STATE_STENCIL_TEST;
	else if (str == "STATE_CULL")
		return (uint32)STATE_CULL;
	else if (str == "STATE_ALPHA_TEST")
		return (uint32)STATE_ALPHA_TEST;
	else if (str == "STATE_SCISSOR_TEST")
		return (uint32)STATE_SCISSOR_TEST;
	else if (str == "STATE_COLORMASK_RED")
		return (uint32)STATE_COLORMASK_RED;
	else if (str == "STATE_COLORMASK_GREEN")
		return (uint32)STATE_COLORMASK_GREEN;
	else if (str == "STATE_COLORMASK_BLUE")
		return (uint32)STATE_COLORMASK_BLUE;
	else if (str == "STATE_COLORMASK_ALPHA")
		return (uint32)STATE_COLORMASK_ALPHA;
	else if (str == "STATE_COLORMASK_ALL")
		return (uint32)STATE_COLORMASK_ALL;
	else if (str == "DEFAULT_2D_STATE")
		return DEFAULT_2D_STATE;
	else if (str == "DEFAULT_2D_STATE_BLEND")
		return DEFAULT_2D_STATE_BLEND;
	else if (str == "DEFAULT_3D_STATE")
		return DEFAULT_3D_STATE;
	else if (str == "DEFAULT_3D_STATE_BLEND")
		return DEFAULT_3D_STATE_BLEND;
	else return 0;
}

};
