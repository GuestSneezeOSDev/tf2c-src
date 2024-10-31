//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: Medium file for the colourblind pattern shader.
//
// $NoKeywords: $
//===========================================================================//

#include "BaseVSShader.h"

#include "SDK_screenspaceeffect_vs20.inc"
#include "colourblindpattern_ps30.inc"
#include "desaturate_vs30.inc"

BEGIN_VS_SHADER_FLAGS(colourblindpattern, "Colourblind Assistance shader.", SHADER_NOT_EDITABLE)
BEGIN_SHADER_PARAMS
SHADER_PARAM(FBTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "_rt_FullFrameFB", "")
SHADER_PARAM(SCRATCHBUFFER, SHADER_PARAM_TYPE_TEXTURE, "_rt_FullFrameFB1", "") // was _rt_SmallFB1
SHADER_PARAM(PATTERNTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "effects/colourblind/16x16_Patterns_RGBA_v5", "")
SHADER_PARAM(PATTERNCHANNEL, SHADER_PARAM_TYPE_FLOAT, "0", "")
SHADER_PARAM(UVSCALING, SHADER_PARAM_TYPE_FLOAT, "15.0", "")
SHADER_PARAM(TEXELSIZE, SHADER_PARAM_TYPE_VEC2, "[0.0 0.0]", "")
SHADER_PARAM(SATURATIONMAXIMA, SHADER_PARAM_TYPE_VEC4, "[0.9 0.9 0.9 0.9]", "")
SHADER_PARAM(SATURATIONMINIMA, SHADER_PARAM_TYPE_VEC4, "[0.4 0.4 0.4 0.4]", "")
SHADER_PARAM(TARGETHUES, SHADER_PARAM_TYPE_VEC4, "[0.0 0.0 0.0 0.0]", "")
SHADER_PARAM(HUEDELTAMAXIMA, SHADER_PARAM_TYPE_VEC4, "[0.9 0.9 0.9 0.9]", "")
SHADER_PARAM(HUEDELTAMINIMA, SHADER_PARAM_TYPE_VEC4, "[0.4 0.4 0.4 0.4]", "")
SHADER_PARAM(INTENSITYMULTIPLIER, SHADER_PARAM_TYPE_FLOAT, "0.75", "")
END_SHADER_PARAMS

SHADER_INIT
{
	if (params[FBTEXTURE]->IsDefined())
	{
		LoadTexture(FBTEXTURE);
	}
	if (params[SCRATCHBUFFER]->IsDefined())
	{
		LoadTexture(SCRATCHBUFFER);
	}
	if (params[PATTERNTEXTURE]->IsDefined())
	{
		LoadTexture(PATTERNTEXTURE);
	}
}

SHADER_FALLBACK
{
	// Requires DX9 + above (In the year of our Lord 2020? Who the fuck is running this on Windows 98? My phone probably does DX11. My fuckin pacemaker probably runs DX9 at least.)
	if (g_pHardwareConfig->GetDXSupportLevel() < 90)
	{
		Assert(0);
		return "Wireframe";
	}
	return 0;
}

SHADER_DRAW
{
	SHADOW_STATE
	{
		

		pShaderShadow->EnableDepthWrites(false);

		pShaderShadow->EnableTexture(SHADER_SAMPLER0, true); // FB
		pShaderShadow->EnableTexture(SHADER_SAMPLER1, true); // Pattern Image RGBA
		pShaderShadow->EnableTexture(SHADER_SAMPLER2, true); // Scratch buffer

		pShaderShadow->VertexShaderVertexFormat(VERTEX_POSITION, 1, 0, 0);

		// Pre-cache shaders
		DECLARE_STATIC_VERTEX_SHADER(desaturate_vs30);
		SET_STATIC_VERTEX_SHADER(desaturate_vs30);

		DECLARE_STATIC_PIXEL_SHADER(colourblindpattern_ps30);
		SET_STATIC_PIXEL_SHADER(colourblindpattern_ps30);

		//if (params[LINEARWRITE]->GetFloatValue())
	}

	DYNAMIC_STATE
	{
		BindTexture(SHADER_SAMPLER0, FBTEXTURE, 0);			// -> _Sampler_FB
		BindTexture(SHADER_SAMPLER1, PATTERNTEXTURE, 1);	// -> _Sampler_Pattern
		BindTexture(SHADER_SAMPLER2, SCRATCHBUFFER, 2);		// -> _Sampler_Scratch

		ITexture *src_texture = params[FBTEXTURE]->GetTextureValue();
		int width = src_texture->GetActualWidth();
		int height = src_texture->GetActualHeight();
		float g_TexelSize[2] = { 1.0f / float(width), 1.0f / float(height) };

		pShaderAPI->SetPixelShaderConstant(17, g_TexelSize);
		//pShaderAPI->SetPixelShaderConstant(17, params[TEXELSIZE]->GetVecValue(), true);

		float uvScaling = params[UVSCALING]->GetFloatValue();
		pShaderAPI->SetPixelShaderConstant(18, &uvScaling, true);
		pShaderAPI->SetPixelShaderConstant(19, params[INTENSITYMULTIPLIER]->GetVecValue(), true);
		pShaderAPI->SetPixelShaderConstant(20, params[SATURATIONMINIMA]->GetVecValue(), true);
		pShaderAPI->SetPixelShaderConstant(21, params[SATURATIONMAXIMA]->GetVecValue(), true);
		pShaderAPI->SetPixelShaderConstant(22, params[TARGETHUES]->GetVecValue(), true);
		pShaderAPI->SetPixelShaderConstant(23, params[HUEDELTAMINIMA]->GetVecValue(), true);
		pShaderAPI->SetPixelShaderConstant(24, params[HUEDELTAMAXIMA]->GetVecValue(), true);
		float nPatternChannel = params[PATTERNCHANNEL]->GetIntValue();
		pShaderAPI->SetPixelShaderConstant(25, &nPatternChannel, true);

		DECLARE_DYNAMIC_VERTEX_SHADER(sdk_screenspaceeffect_vs20);
		SET_DYNAMIC_VERTEX_SHADER(sdk_screenspaceeffect_vs20);

		DECLARE_DYNAMIC_PIXEL_SHADER(colourblindpattern_ps30);
		SET_DYNAMIC_PIXEL_SHADER(colourblindpattern_ps30);
	}
	Draw();
}
END_SHADER
