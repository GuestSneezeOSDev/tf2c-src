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

void LuxShadowModel_Link_Params(Geometry_Vars_t &info)
{

}

void LuxShadowModel_Init_Params(CBaseVSShader *pShader, IMaterialVar **params, const char *pMaterialName, Geometry_Vars_t &info)
{

}

void LuxShadowModel_Shader_Init(CBaseVSShader *pShader, IMaterialVar **params, Geometry_Vars_t &info)
{

}

void LuxShadowModel_Shader_Draw(CBaseVSShader *pShader, IMaterialVar **params, IShaderDynamicAPI *pShaderAPI, IShaderShadow *pShaderShadow, Geometry_Vars_t &info, VertexCompressionType_t vertexCompression, CBasePerMaterialContextData **pContextDataPtr)
{

}

// Lux shaders will replace whatever already exists.
DEFINE_FALLBACK_SHADER(SDK_ShadowModel, LUX_ShadowModel)
DEFINE_FALLBACK_SHADER(SDK_ShadowModel_DX9, LUX_ShadowModel)
DEFINE_FALLBACK_SHADER(SDK_ShadowModel_DX8, LUX_ShadowModel)

// ShiroDkxtro2 : Brainchild of Totterynine.
// Declare param declarations separately then call that from within shader declaration.
// This makes it possible to easily run multiple shaders in one file
BEGIN_VS_SHADER(LUX_ShadowModel, "ShiroDkxtro2's ACROHS Shader-Rewrite Shader")

BEGIN_SHADER_PARAMS

END_SHADER_PARAMS

// Set Up Vars here

SHADER_INIT_PARAMS()
{
	Geometry_Vars_t vars;
	LuxShadowModel_Link_Params(vars);
	LuxShadowModel_Init_Params(this, params, pMaterialName, vars);
}

SHADER_FALLBACK
{
	// OVERRIDE
	// TODO: REMOVE LATER WHEN THIS SHADER WAS DONE
	return "ShadowModel";

	if (g_pHardwareConfig->GetDXSupportLevel() < 90)
	{
		Warning("Game run at DXLevel < 90 \n");
		return "Wireframe";
	}
	return 0;
}

SHADER_INIT
{
	Geometry_Vars_t vars;
	LuxShadowModel_Link_Params(vars);
	LuxShadowModel_Shader_Init(this, params, vars);
}

SHADER_DRAW
{
	Geometry_Vars_t vars;
	LuxShadowModel_Link_Params(vars);
	LuxShadowModel_Shader_Draw(this, params, pShaderAPI, pShaderShadow, vars, vertexCompression, pContextDataPtr);

#ifdef ACROHS_CSM
	// Function Here
#endif

}
END_SHADER