//========= ShiroDkxtro2's --ACROHS Ultimate Shaders Project-- ============//
//
//	Initial D.	:	20.01.2023 DMY
//	Last Change :	24.05.2023 DMY
//
//	Purpose of this File :	Shader to handle distance alpha effects
//							Mostly only functional for some tf2 text-textures
//							This doesn't really get used a lot elsewhere, so there are some compromises
//
//===========================================================================//

// File in which you can determine what to use.
// We do not use preprocessor definitions for this because this is honestly faster and easier to set up.
#include "../../ACROHS_Defines.h"

// All Shaders use the same struct and sampler definitions. This way multipass CSM doesn't need to re-setup the struct
#include "../../ACROHS_Shared.h"

// We need all of these
#include "../renderpasses/DistanceAlpha.h"
#include "../CommonFunctions.h"
#include "../../cpp_shader_constant_register_map.h"

// this is required for our Function Array's
#include <functional>

// Includes for Shaderfiles...

#define LuxUnlitGeneric_Params() void LuxDistanceAlpha_Link_Params(DistanceAlpha_Vars_t &info)\
{\
}

void LuxDistanceAlpha_Init_Params(CBaseVSShader *pShader, IMaterialVar **params, const char *pMaterialName, DistanceAlpha_Vars_t &info)
{
	// I wanted to debug print some values and needed to know which material they belong to. However pMaterialName is not available to the draw!
	// Well, now it is...
	params[info.m_nMultipurpose7]->SetStringValue(pMaterialName);
}

void LuxDistanceAlpha_Shader_Init(CBaseVSShader *pShader, IMaterialVar **params, DistanceAlpha_Vars_t &info)
{

}

void LuxDistanceAlpha_Shader_Draw(CBaseVSShader *pShader, IMaterialVar **params, IShaderDynamicAPI *pShaderAPI, IShaderShadow *pShaderShadow, DistanceAlpha_Vars_t &info, VertexCompressionType_t vertexCompression, CBasePerMaterialContextData **pContextDataPtr)
{

}

// Lux shaders will replace whatever already exists.
DEFINE_FALLBACK_SHADER(SDK_DistanceAlpha, LUX_DistanceAlpha)
DEFINE_FALLBACK_SHADER(SDK_DistanceAlpha_DX9, LUX_DistanceAlpha)

// ShiroDkxtro2 : Brainchild of Totterynine.
// Declare param declarations separately then call that from within shader declaration.
// This makes it possible to easily run multiple shaders in one file
BEGIN_VS_SHADER(LUX_DISTANCEALPHA, "ShiroDkxtro2's ACROHS Shader-Rewrite Shader")

BEGIN_SHADER_PARAMS

END_SHADER_PARAMS

LuxUnlitGeneric_Params()

SHADER_INIT_PARAMS()
{
	DistanceAlpha_Vars_t vars;
	LuxDistanceAlpha_Link_Params(vars);
	LuxDistanceAlpha_Init_Params(this, params, pMaterialName, vars);
}

SHADER_FALLBACK
{
	if (g_pHardwareConfig->GetDXSupportLevel() < 90)
	{
		Warning("Game run at DXLevel < 90 \n");
		return "Wireframe";
	}
	return 0;
}

SHADER_INIT
{
	DistanceAlpha_Vars_t vars;
	LuxDistanceAlpha_Link_Params(vars);
	LuxDistanceAlpha_Shader_Init(this, params, vars);
}

SHADER_DRAW
{
	DistanceAlpha_Vars_t vars;
	LuxDistanceAlpha_Link_Params(vars);
	LuxDistanceAlpha_Shader_Draw(this, params, pShaderAPI, pShaderShadow, vars, vertexCompression, pContextDataPtr);

#ifdef ACROHS_CSM
	// Function Here
#endif

}
END_SHADER