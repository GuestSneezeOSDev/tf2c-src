//========= ShiroDkxtro2's --ACROHS Ultimate Shaders Project-- ============//
//
//	Initial D.	:	20.01.2023 DMY
//	Last Change :	24.05.2023 DMY
//
//	Purpose of this File :	Shader usually utilised for inverse lighting effects such as SSS
//							The original use of this pass was for the Alyx healing scene in ep2
//
//===========================================================================//

// File in which you can determine what to use.
// We do not use preprocessor definitions for this because this is honestly faster and easier to set up.
#include "../../ACROHS_Defines.h"

// All Shaders use the same struct and sampler definitions. This way multipass CSM doesn't need to re-setup the struct
#include "../../ACROHS_Shared.h"

// We need all of these
#include "FleshInterior.h"
#include "../CommonFunctions.h"
#include "../../cpp_shader_constant_register_map.h"

// this is required for our Function Array's
#include <functional>

// Includes for Shaderfiles...

// This is in FleshInterior.h
// #define LuxFleshInterior_Params() void LuxDistanceAlpha_Link_Params(FleshInterior_Vars_t &info)

void LuxDistanceAlpha_Init_Params(CBaseVSShader *pShader, IMaterialVar **params, const char *pMaterialName, FleshInterior_Vars_t &info)
{

}

void LuxDistanceAlpha_Shader_Init(CBaseVSShader *pShader, IMaterialVar **params, FleshInterior_Vars_t &info)
{

}

void LuxDistanceAlpha_Shader_Draw(CBaseVSShader *pShader, IMaterialVar **params, IShaderDynamicAPI *pShaderAPI, IShaderShadow *pShaderShadow, FleshInterior_Vars_t &info, VertexCompressionType_t vertexCompression, CBasePerMaterialContextData **pContextDataPtr)
{

}