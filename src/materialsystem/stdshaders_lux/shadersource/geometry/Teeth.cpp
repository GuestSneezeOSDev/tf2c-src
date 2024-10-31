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

#define LuxTeeth_Setup_Params() void LuxTeeth_Link_Params(Geometry_Vars_t &info)\
{\
		Link_GlobalParameters()\
		Link_NormalTextureParameters()\
		Link_MiscParameters()\
		info.m_nMultipurpose1 = ILLUMFACTOR;\
		info.m_nMultipurpose2 = FORWARD;\
		info.m_nMultipurpose3 = PHONGEXPONENT;\
		info.m_nMultipurpose4 = INTRO;\
		info.m_nMultipurpose5 = ENTITYORIGIN;\
		info.m_nMultipurpose6 = WARPPARAM;\
}

//#define m_nTeethIllumFactor		m_nMultipurpose1	//	For use in Teeth.cpp ONLY
//#define m_nTeethForward			m_nMultipurpose2	//	For use in Teeth.cpp ONLY
//#define m_nTeethPhongExponent	m_nMultipurpose3	//	For use in Teeth.cpp ONLY
//#define m_nTeethIntro			m_nMultipurpose4	//	For use in Teeth.cpp ONLY
//#define m_nTeethEntityOrigin	m_nMultipurpose5	//	For use in Teeth.cpp ONLY
//#define m_nTeethWarpParam		m_nMultipurpose6	//	For use in Teeth.cpp ONLY

void LuxTeeth_Init_Params(CBaseVSShader *pShader, IMaterialVar **params, const char *pMaterialName, Geometry_Vars_t &info)
{

}

void LuxTeeth_Shader_Init(CBaseVSShader *pShader, IMaterialVar **params, Geometry_Vars_t &info)
{

}

void LuxTeeth_Shader_Draw(CBaseVSShader *pShader, IMaterialVar **params, IShaderDynamicAPI *pShaderAPI, IShaderShadow *pShaderShadow, Geometry_Vars_t &info, VertexCompressionType_t vertexCompression, CBasePerMaterialContextData **pContextDataPtr)
{

}

// Lux shaders will replace whatever already exists.
DEFINE_FALLBACK_SHADER(SDK_Teeth, LUX_Teeth)
DEFINE_FALLBACK_SHADER(SDK_Teeth_DX9, LUX_Teeth)
DEFINE_FALLBACK_SHADER(SDK_Teeth_DX8, LUX_Teeth)
DEFINE_FALLBACK_SHADER(SDK_Teeth_DX6, LUX_Teeth)

// ShiroDkxtro2 : Brainchild of Totterynine.
// Declare param declarations separately then call that from within shader declaration.
// This makes it possible to easily run multiple shaders in one file
BEGIN_VS_SHADER(LUX_Teeth, "ShiroDkxtro2's ACROHS Shader-Rewrite Shader")

BEGIN_SHADER_PARAMS
	Declare_NormalTextureParameters();
	Declare_MiscParameters();
	SHADER_PARAM(ILLUMFACTOR, SHADER_PARAM_TYPE_FLOAT, "1", "Amount to darken or brighten the teeth");
	SHADER_PARAM(FORWARD, SHADER_PARAM_TYPE_VEC3, "[1 0 0]", "Forward direction vector for teeth lighting");
	SHADER_PARAM(PHONGEXPONENT, SHADER_PARAM_TYPE_FLOAT, "100", "phong exponent");
	SHADER_PARAM(INTRO, SHADER_PARAM_TYPE_BOOL, "0", "is teeth in the ep1 intro");
	SHADER_PARAM(ENTITYORIGIN, SHADER_PARAM_TYPE_VEC3, "0.0", "center if the model in world space");
	SHADER_PARAM(WARPPARAM, SHADER_PARAM_TYPE_FLOAT, "0.0", "animation param between 0 and 1");

END_SHADER_PARAMS

LuxTeeth_Setup_Params()

SHADER_INIT_PARAMS()
{
	Geometry_Vars_t vars;
	LuxTeeth_Link_Params(vars);
	LuxTeeth_Init_Params(this, params, pMaterialName, vars);
}

SHADER_FALLBACK
{
	// OVERRIDE
	// TODO: REMOVE LATER WHEN THIS SHADER WAS DONE
	return "Teeth";

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
	LuxTeeth_Link_Params(vars);
	LuxTeeth_Shader_Init(this, params, vars);
}

SHADER_DRAW
{
	Geometry_Vars_t vars;
	LuxTeeth_Link_Params(vars);
	LuxTeeth_Shader_Draw(this, params, pShaderAPI, pShaderShadow, vars, vertexCompression, pContextDataPtr);

#ifdef ACROHS_CSM
	// Function Here
#endif

}
END_SHADER