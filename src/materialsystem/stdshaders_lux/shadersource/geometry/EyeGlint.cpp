//========= ShiroDkxtro2's --ACROHS Ultimate Shaders Project-- ============//
//
//	Initial D.	:	20.01.2023 DMY
//	Last Change :	24.05.2023 DMY
//
//	Purpose of this File :	
//
//===========================================================================//

// File in which you can determine what to use.
// We do not use preprocessor definitions for this because this is honestly faster and easier to set up.
#include "../../ACROHS_Defines.h"

// All Shaders use the same struct and sampler definitions. This way multipass CSM doesn't need to re-setup the struct
#include "../../ACROHS_Shared.h"

// We need all of these
#include "../CommonFunctions.h"
#include "../../cpp_shader_constant_register_map.h"

// this is required for our Function Array's
#include <functional>

// Includes for Shaderfiles...

void LuxEyeGlint_Shader_Draw(CBaseVSShader *pShader, IMaterialVar **params, IShaderDynamicAPI *pShaderAPI, IShaderShadow *pShaderShadow, VertexCompressionType_t vertexCompression, CBasePerMaterialContextData **pContextDataPtr)
{
	SHADOW_STATE
	{
		pShaderShadow->EnableDepthWrites(false);
		pShaderShadow->EnableBlending(true);
		pShaderShadow->BlendFunc(SHADER_BLEND_ONE, SHADER_BLEND_ONE); // Additive Blending

		// TexCoord Int Array to tell the shader we want these specific texcoords...
		int pTexCoords[3] = { 2, 2, 3 };
		pShaderShadow->VertexShaderVertexFormat(VERTEX_POSITION, 3, pTexCoords, 0);
		pShaderShadow->EnableCulling(false); // Overrides to no culling.
		// We are on a sm3.0 shader so YES we write sRGB
		pShaderShadow->EnableSRGBWrite(true);

		// ShiroDkxtro2: TODO: Write this shader.
		/*
		DECLARE_STATIC_VERTEX_SHADER(lux_eyeglint_vs30);
		SET_STATIC_VERTEX_SHADER(lux_eyeglint_vs30);

		DECLARE_STATIC_PIXEL_SHADER(lux_eyeglint_ps30);
		SET_STATIC_PIXEL_SHADER(lux_eyeglint_ps30);
		*/
	}

	DYNAMIC_STATE
	{
		// ShiroDkxtro2: TODO: See above
		/*
		DECLARE_DYNAMIC_VERTEX_SHADER(lux_eyeglint_vs30);
		SET_DYNAMIC_VERTEX_SHADER(lux_eyeglint_vs30);

		DECLARE_DYNAMIC_PIXEL_SHADER(lux_eyeglint_ps30);
		SET_DYNAMIC_PIXEL_SHADER(lux_eyeglint_ps30);
		*/
	}
	pShader->Draw();
}

// Lux shaders will replace whatever already exists.
DEFINE_FALLBACK_SHADER(SDK_EyeGlint, LUX_EyeGlint)
DEFINE_FALLBACK_SHADER(SDK_EyeGlint_dx9, LUX_EyeGlint)

// ShiroDkxtro2 : Brainchild of Totterynine.
// Declare param declarations separately then call that from within shader declaration.
// This makes it possible to easily run multiple shaders in one file
BEGIN_VS_SHADER(LUX_EyeGlint, "ShiroDkxtro2's ACROHS Shader-Rewrite Shader")

BEGIN_SHADER_PARAMS
END_SHADER_PARAMS

// Set Up Vars here

SHADER_INIT_PARAMS()
{
}

SHADER_FALLBACK
{
	// OVERRIDE
	// TODO: REMOVE LATER WHEN THIS SHADER WAS DONE
	return "EyeGlint";

	if (g_pHardwareConfig->GetDXSupportLevel() < 90)
	{
		Warning("Game run at DXLevel < 90 \n");
		return "Wireframe";
	}
	return 0;
}

SHADER_INIT
{
}

SHADER_DRAW
{
	LuxEyeGlint_Shader_Draw(this, params, pShaderAPI, pShaderShadow, vertexCompression, pContextDataPtr);

#ifdef ACROHS_CSM
	// Function Here
#endif

}
END_SHADER