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

#define LuxLightMappedGenericDecal_Params() void LuxLightMappedGenericDecal_Link_Params(Brush_Vars_t &info)\
{									   \
	info.m_nColor = COLOR;				\
	info.m_nMultipurpose7 = MATERIALNAME;\
}

void LuxLightMappedGenericDecal_Init_Params(CBaseVSShader *pShader, IMaterialVar **params, const char *pMaterialName, Brush_Vars_t &info)
{
	// I wanted to debug print some values and needed to know which material they belong to. However pMaterialName is not available to the draw!
	// Well, now it is...
	params[info.m_nMultipurpose7]->SetStringValue(pMaterialName);

}

void LuxLightMappedGenericDecal_Shader_Init(CBaseVSShader *pShader, IMaterialVar **params, Brush_Vars_t &info)
{

}

void LuxLightMappedGenericDecal_Shader_Draw(CBaseVSShader *pShader, IMaterialVar **params, IShaderDynamicAPI *pShaderAPI, IShaderShadow *pShaderShadow, Brush_Vars_t &info, VertexCompressionType_t vertexCompression, CBasePerMaterialContextData **pContextDataPtr)
{

	pShader->Draw(false);
}

// ShiroDkxtro2 : Brainchild of Totterynine.
// Declare param declarations separately then call that from within shader declaration.
// This makes it possible to easily run multiple shaders in one file
BEGIN_VS_SHADER(LUX_LightMappedGeneric_Decal, "ShiroDkxtro2's ACROHS Shader-Rewrite Shader")

BEGIN_SHADER_PARAMS
SHADER_PARAM(MATERIALNAME, SHADER_PARAM_TYPE_STRING, "", "") // Can't use pMaterialName on Draw so we put it on this parameter instead...
END_SHADER_PARAMS

LuxLightMappedGenericDecal_Params()

SHADER_INIT_PARAMS()
{
	Brush_Vars_t vars;
	LuxLightMappedGenericDecal_Link_Params(vars);
	LuxLightMappedGenericDecal_Init_Params(this, params, pMaterialName, vars);
}

SHADER_FALLBACK
{
	// OVERRIDE
	// TODO: REMOVE LATER WHEN THIS SHADER WAS DONE
	return "lightmappedgeneric_decal";

	if (g_pHardwareConfig->GetDXSupportLevel() < 90)
	{
		Warning("Game run at DXLevel < 90 \n");
		return "Wireframe";
	}
	return 0;
}

SHADER_INIT
{
	Brush_Vars_t vars;
	LuxLightMappedGenericDecal_Link_Params(vars);
	LuxLightMappedGenericDecal_Shader_Init(this, params, vars);
}

SHADER_DRAW
{
	Brush_Vars_t vars;
	LuxLightMappedGenericDecal_Link_Params(vars);
	LuxLightMappedGenericDecal_Shader_Draw(this, params, pShaderAPI, pShaderShadow, vars, vertexCompression, pContextDataPtr);

#ifdef ACROHS_CSM
	// Function Here
#endif

}
END_SHADER