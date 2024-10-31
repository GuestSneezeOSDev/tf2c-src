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

struct ErrorShader_Vars_t
{
	ErrorShader_Vars_t() { memset(this, 0xFF, sizeof(*this)); }

	int m_nErrorTexture;
};

#define LuxErrorShader_Params() void LuxError_Link_Params(ErrorShader_Vars_t &info)\
{\
	info.m_nErrorTexture = ERRORTEXTURE;\
}

void LuxError_Init_Params(CBaseVSShader *pShader, IMaterialVar **params, const char *pMaterialName, ErrorShader_Vars_t &info)
{
	params[info.m_nErrorTexture]->SetStringValue("editor/flat");
}

void LuxError_Shader_Init(CBaseVSShader *pShader, IMaterialVar **params, ErrorShader_Vars_t &info)
{
	pShader->LoadTexture(info.m_nErrorTexture, 0); // Load the Missing Tex. This should quite literally return an error, this is why I became a programmer.
}

void LuxError_Shader_Draw(CBaseVSShader *pShader, IMaterialVar **params, IShaderDynamicAPI *pShaderAPI, IShaderShadow *pShaderShadow, ErrorShader_Vars_t &info, VertexCompressionType_t vertexCompression, CBasePerMaterialContextData **pContextDataPtr)
{
	SHADOW_STATE
	{
		pShaderShadow->EnableDepthWrites(false);
		pShaderShadow->EnableBlending(true);
		pShaderShadow->EnableCulling(false); // Overrides to no culling.

		// We are on a sm3.0 shader so YES we write sRGB
		pShaderShadow->EnableSRGBWrite(true);

		// ShiroDkxtro2: TODO: Write this shader.
		/*
		DECLARE_STATIC_VERTEX_SHADER(lux_error_vs30);
		SET_STATIC_VERTEX_SHADER(lux_error_vs30);

		DECLARE_STATIC_PIXEL_SHADER(lux_error_ps30);
		SET_STATIC_PIXEL_SHADER(lux_error_ps30);
		*/
	}

	DYNAMIC_STATE
	{
		// ShiroDkxtro2: TODO: See above
		/*
		DECLARE_DYNAMIC_VERTEX_SHADER(lux_error_vs30);
		SET_DYNAMIC_VERTEX_SHADER(lux_error_vs30);

		DECLARE_DYNAMIC_PIXEL_SHADER(lux_error_ps30);
		SET_DYNAMIC_PIXEL_SHADER(lux_error_ps30);
		*/
	}
	pShader->Draw();
}

// ShiroDkxtro2 : Brainchild of Totterynine.
// Declare param declarations separately then call that from within shader declaration.
// This makes it possible to easily run multiple shaders in one file
BEGIN_VS_SHADER(LUX_ErrorShader, "ShiroDkxtro2's ACROHS Shader-Rewrite Shader")

BEGIN_SHADER_PARAMS
SHADER_PARAM(ERRORTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "This is literally supposed to error out! Ignore any errors... or err.. Ignore console ones!");
END_SHADER_PARAMS

LuxErrorShader_Params()

SHADER_INIT_PARAMS()
{
	ErrorShader_Vars_t vars;
	LuxError_Link_Params(vars);
	LuxError_Init_Params(this, params, pMaterialName, vars);
}

SHADER_FALLBACK
{
	return 0;
}

SHADER_INIT
{
	ErrorShader_Vars_t vars;
	LuxError_Link_Params(vars);
	LuxError_Shader_Init(this, params, vars);
}

SHADER_DRAW
{
	ErrorShader_Vars_t vars;
	LuxError_Link_Params(vars);
	LuxError_Shader_Draw(this, params, pShaderAPI, pShaderShadow, vars, vertexCompression, pContextDataPtr);

#ifdef ACROHS_CSM
	// Function Here
#endif

}
END_SHADER