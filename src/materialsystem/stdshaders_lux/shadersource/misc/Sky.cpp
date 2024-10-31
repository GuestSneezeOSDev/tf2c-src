//========= ShiroDkxtro2's --ACROHS Ultimate Shaders Project-- ============//
//
//	Initial D.	:	20.01.2023 DMY
//	Last Change :	24.05.2023 DMY
//
//	Purpose of this File :	TODO: DELETE
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
#include "lux_brush_vs30.inc"
#include "lux_model_vs30.inc"
#include "lux_sky_hdri_ps30.inc"

#if 0
void LuxSky_Link_Params(Geometry_Vars_t &info)
{

}

void LuxSky_Init_Params(CBaseVSShader *pShader, IMaterialVar **params, const char *pMaterialName, Geometry_Vars_t &info)
{

}

void LuxSky_Shader_Init(CBaseVSShader *pShader, IMaterialVar **params, Geometry_Vars_t &info)
{

}

void LuxSky_Shader_Draw(CBaseVSShader *pShader, IMaterialVar **params, IShaderDynamicAPI *pShaderAPI, IShaderShadow *pShaderShadow, Geometry_Vars_t &info, VertexCompressionType_t vertexCompression, CBasePerMaterialContextData **pContextDataPtr)
{

}

// Lux shaders will replace whatever already exists.
DEFINE_FALLBACK_SHADER(SDK_Sky_DX9, LUX_Sky)
DEFINE_FALLBACK_SHADER(SDK_Sky_DX6, LUX_Sky)

// ShiroDkxtro2 : Brainchild of Totterynine.
// Declare param declarations separately then call that from within shader declaration.
// This makes it possible to easily run multiple shaders in one file
BEGIN_VS_SHADER(LUX_Sky, "ShiroDkxtro2's ACROHS Shader-Rewrite Shader")

BEGIN_SHADER_PARAMS

END_SHADER_PARAMS

// Set Up Vars here

SHADER_INIT_PARAMS()
{
	Geometry_Vars_t vars;
	LuxSky_Link_Params(vars);
	LuxSky_Init_Params(this, params, pMaterialName, vars);
}

SHADER_FALLBACK
{
	// OVERRIDE
	// TODO: REMOVE LATER WHEN THIS SHADER WAS DONE
	return "Sky";

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
	LuxSky_Link_Params(vars);
	LuxSky_Shader_Init(this, params, vars);
}

SHADER_DRAW
{
	Geometry_Vars_t vars;
	LuxSky_Link_Params(vars);
	LuxSky_Shader_Draw(this, params, pShaderAPI, pShaderShadow, vars, vertexCompression, pContextDataPtr);

#ifdef ACROHS_CSM
	// Function Here
#endif

}
END_SHADER
#endif

//////////////////////////////////////////////////////////////
//					SKY_HDRI starts here					//
//////////////////////////////////////////////////////////////

struct Sky_HDRI_Vars_t
{
	Sky_HDRI_Vars_t() { memset(this, 0xFF, sizeof(*this)); }
	Declare_GloballyDefinedStructVars();

	int m_nMode;
	int m_nStretch;
	int m_nHalfHDRI;	// Allows the use of cut in half HDRI's
	int m_nHDR;
	int m_nSRGBWrite;
	int m_nTextureSRGB;
	int m_nRotation;
	int m_nHDRITint;
	/*
	how m_nMode works :
						o o o o
		Mode 1		->	x o o o
	Maximum res of 4096x2048 ( 1x1 Textures )
	The aspect ratio of a single texture will be 2:1

						o o o o
		Mode 2		->	x x o o
	Maximum res of 8192x4096 ( 2x1 Textures ) - Full res HDRI
	Maximum res of 8192x2048 ( 2x1 HalfHDRI ) - Cut in half to save filesize and performance.

						x x x x
		Mode 3		->	x x x x
	Maximum res of 16384x8192 ( 4x2 Textures ) - Full res HDRI
	Maximum res of 16384x4096 ( 4x1 HalfHDRI ) - Cut in half to save filesize and performance.
	*/
	int m_nSkyTextureX1Y1; // If this exists, mode 1
	int m_nSkyTextureX2Y1; // If this exists, mode 2
	int m_nSkyTextureX3Y1; // If this exists, mode 3
	int m_nSkyTextureX4Y1; // If this exists, mode 3
	int m_nSkyTextureX1Y2;
	int m_nSkyTextureX2Y2;
	int m_nSkyTextureX3Y2;
	int m_nSkyTextureX4Y2; 
};

#define LuxSkyHDRI_Params() void LuxSkyHDRI_Link_Params(Sky_HDRI_Vars_t &info)\
{																						\
	info.m_nSkyTextureX1Y1	=	SKYTEXTUREX1Y1;											\
	info.m_nSkyTextureX2Y1	=	SKYTEXTUREX2Y1;											\
	info.m_nSkyTextureX3Y1	=	SKYTEXTUREX3Y1;											\
	info.m_nSkyTextureX4Y1	=	SKYTEXTUREX4Y1;											\
	info.m_nSkyTextureX1Y2	=	SKYTEXTUREX1Y2;											\
	info.m_nSkyTextureX2Y2	=	SKYTEXTUREX2Y2;											\
	info.m_nSkyTextureX3Y2	=	SKYTEXTUREX3Y2;											\
	info.m_nSkyTextureX4Y2	=	SKYTEXTUREX4Y2;											\
	info.m_nHalfHDRI		=	HALFHDRI;												\
	info.m_nMode			=	MODE;													\
	info.m_nStretch			=	STRETCH;												\
	info.m_nHDR				=	HDR;													\
	info.m_nSRGBWrite		=	WRITESRGB;												\
	info.m_nTextureSRGB		=	TEXTURESRGB;											\
	info.m_nRotation		=	ROTATION;												\
	info.m_nHDRITint		=	HDRITINT;												\
	Link_GlobalParameters()																\
}//-------------------------------------------------------------------------------------|

#define LuxScreenspace_ParameterDeclaration()																													\
SHADER_PARAM(SKYTEXTUREX1Y1				, SHADER_PARAM_TYPE_TEXTURE , "", "Full or Part of an HDRI Texture")													\
SHADER_PARAM(SKYTEXTUREX2Y1				, SHADER_PARAM_TYPE_TEXTURE , "", "Part of an HDRI Texture")															\
SHADER_PARAM(SKYTEXTUREX3Y1				, SHADER_PARAM_TYPE_TEXTURE , "", "Part of an HDRI Texture")															\
SHADER_PARAM(SKYTEXTUREX4Y1				, SHADER_PARAM_TYPE_TEXTURE , "", "Part of an HDRI Texture")															\
SHADER_PARAM(SKYTEXTUREX1Y2				, SHADER_PARAM_TYPE_TEXTURE , "", "Part of an HDRI Texture")															\
SHADER_PARAM(SKYTEXTUREX2Y2				, SHADER_PARAM_TYPE_TEXTURE , "", "Part of an HDRI Texture")															\
SHADER_PARAM(SKYTEXTUREX3Y2				, SHADER_PARAM_TYPE_TEXTURE , "", "Part of an HDRI Texture")															\
SHADER_PARAM(SKYTEXTUREX4Y2				, SHADER_PARAM_TYPE_TEXTURE , "", "Part of an HDRI Texture")															\
SHADER_PARAM(HALFHDRI					, SHADER_PARAM_TYPE_BOOL	, "", "I'm too lazy to add a real description to this right now, fix it for me, thanks!")	\
SHADER_PARAM(MODE						, SHADER_PARAM_TYPE_INTEGER	, "", "INTERNAL DO NOT MESS WITH THIS PARAMETER")											\
SHADER_PARAM(STRETCH					, SHADER_PARAM_TYPE_BOOL	, "", "I'm too lazy to add a real description to this right now, fix it for me, thanks!")	\
SHADER_PARAM(HDR						, SHADER_PARAM_TYPE_BOOL	, "", "Set to 1 if your texture(s) are HDR and need to be multiplied by 16.0f, ( rgba16161616 and rgba16161616f textures )")\
SHADER_PARAM(WRITESRGB					, SHADER_PARAM_TYPE_BOOL	, "", "Determines how Shader Output is being used. If set to 1 it will convert from Linear to sRGB")\
SHADER_PARAM(TEXTURESRGB				, SHADER_PARAM_TYPE_BOOL	, "", "Load textures as sRGB...")\
SHADER_PARAM(ROTATION					, SHADER_PARAM_TYPE_INTEGER , "", "Rotation of the HDRI, in degrees.")\
SHADER_PARAM(HDRITINT					, SHADER_PARAM_TYPE_COLOR	, "", "multiplied by HDR factor..")\
//--------------------------------------------------------------------------------------------------------------------------------------------------------------|


void LuxSkyHDRI_Init_Params(CBaseVSShader *pShader, IMaterialVar **params, const char *pMaterialName, Sky_HDRI_Vars_t &info)
{

	// Default Mode us 0
// 	SetIntParamValue(info.m_nMode, 0)

	// See struct ( Sky_HDRI_Vars_t ) for how this works

	SET_FLAGS(MATERIAL_VAR_NOFOG);
	SET_FLAGS(MATERIAL_VAR_IGNOREZ);

	if (IsParamDefined(info.m_nSkyTextureX2Y1))
		SetIntParamValue(info.m_nMode, 1)

	if (IsParamDefined(info.m_nSkyTextureX3Y1))
		SetIntParamValue(info.m_nMode, 2)
}


void LuxSkyHDRI_Shader_Init(CBaseVSShader *pShader, IMaterialVar **params, Sky_HDRI_Vars_t &info)
{
	// TextureFlags don't matter here
	LoadTextureWithCheck(info.m_nSkyTextureX1Y1, 0)
	LoadTextureWithCheck(info.m_nSkyTextureX2Y1, 0)
	LoadTextureWithCheck(info.m_nSkyTextureX3Y1, 0)
	LoadTextureWithCheck(info.m_nSkyTextureX4Y1, 0)
	LoadTextureWithCheck(info.m_nSkyTextureX1Y2, 0)
	LoadTextureWithCheck(info.m_nSkyTextureX2Y2, 0)
	LoadTextureWithCheck(info.m_nSkyTextureX3Y2, 0)
	LoadTextureWithCheck(info.m_nSkyTextureX4Y2, 0)
}

void LuxSkyHDRI_Shader_Draw(CBaseVSShader *pShader, IMaterialVar **params, IShaderDynamicAPI *pShaderAPI, IShaderShadow *pShaderShadow, Sky_HDRI_Vars_t &info, VertexCompressionType_t vertexCompression, CBasePerMaterialContextData **pContextDataPtr)
{
	int iMode = GetIntParamValue(info.m_nMode);
	bool bIsHalfHDRI	= GetBoolParamValue(info.m_nHalfHDRI);
	bool bHasStretch	= GetBoolParamValue(info.m_nStretch);
	bool bHDR			= GetBoolParamValue(info.m_nHDR);
	bool bWriteSRGB		= GetBoolParamValue(info.m_nSRGBWrite);
	bool bTextureSRGB	= GetBoolParamValue(info.m_nTextureSRGB);

	// Always required, minimum of the 2:1 ( Mode 0 )
	bool bHasTextureX1Y1 = IsTextureLoaded(info.m_nSkyTextureX1Y1);

	// Only Available with 2 textures on X ( Mode 1 )
	bool bHasTextureX2Y1 = IsTextureLoaded(info.m_nSkyTextureX2Y1) && (iMode == 1);

	// Only Available with 4 textures on X ( Mode 2 )
	bool bHasTextureX3Y1 = IsTextureLoaded(info.m_nSkyTextureX3Y1) && (iMode == 2);
	bool bHasTextureX4Y1 = IsTextureLoaded(info.m_nSkyTextureX4Y1) && (iMode == 2);

	// Only Available with 4 textures on X ( Mode 3 ). NOT usable with HalfHDRI's
	bool bHasTextureX1Y2 = IsTextureLoaded(info.m_nSkyTextureX1Y2) && (iMode == 2) && !bIsHalfHDRI;
	bool bHasTextureX2Y2 = IsTextureLoaded(info.m_nSkyTextureX2Y2) && (iMode == 2) && !bIsHalfHDRI;
	bool bHasTextureX3Y2 = IsTextureLoaded(info.m_nSkyTextureX3Y2) && (iMode == 2) && !bIsHalfHDRI;
	bool bHasTextureX4Y2 = IsTextureLoaded(info.m_nSkyTextureX4Y2) && (iMode == 2) && !bIsHalfHDRI;

	SHADOW_STATE
	{
		pShader->SetInitialShadowState();
//		bool HDR = pShader->IsHDREnabled();
		//		No. use the below.
//		pShaderShadow->VertexShaderVertexFormat(VERTEX_POSITION, 1, NULL, 0);
		if (IS_FLAG_SET(MATERIAL_VAR_MODEL))
		{
			GetVertexShaderFormat_Model()
		}
		else
		{
			GetVertexShaderFormat_Brush()
		}

		bool IsOpaque(bIsFullyOpaque, info.m_nBaseTexture, false);
		pShaderShadow->EnableAlphaWrites(bIsFullyOpaque);
		pShaderShadow->EnableSRGBWrite(true); // bWriteSRGB is handled in the shader otherwise we get graphical issues on brushes for some reason

		// I don't give a damn, just enable all of them regardless...
		EnableSampler(SHADER_SAMPLER0, bTextureSRGB);
		EnableSampler(SHADER_SAMPLER1, bTextureSRGB);
		EnableSampler(SHADER_SAMPLER2, bTextureSRGB);
		EnableSampler(SHADER_SAMPLER3, bTextureSRGB);
		EnableSampler(SHADER_SAMPLER4, bTextureSRGB);
		EnableSampler(SHADER_SAMPLER5, bTextureSRGB);
		EnableSampler(SHADER_SAMPLER6, bTextureSRGB);
		EnableSampler(SHADER_SAMPLER7, bTextureSRGB);
		/*
		EnableSamplerWithCheck(bHasTextureX1Y1, SHADER_SAMPLER0, !bHDR)
		EnableSamplerWithCheck(bHasTextureX2Y1, SHADER_SAMPLER1, !bHDR)
		EnableSamplerWithCheck(bHasTextureX3Y1, SHADER_SAMPLER2, !bHDR)
		EnableSamplerWithCheck(bHasTextureX4Y1, SHADER_SAMPLER3, !bHDR)
		EnableSamplerWithCheck(bHasTextureX1Y2, SHADER_SAMPLER4, !bHDR)
		EnableSamplerWithCheck(bHasTextureX2Y2, SHADER_SAMPLER5, !bHDR)
		EnableSamplerWithCheck(bHasTextureX3Y2, SHADER_SAMPLER6, !bHDR)
		EnableSamplerWithCheck(bHasTextureX4Y2, SHADER_SAMPLER7, !bHDR)
		*/

		// Literally all we want is position...
		if (IS_FLAG_SET(MATERIAL_VAR_MODEL))
		{
			DECLARE_STATIC_VERTEX_SHADER(lux_model_vs30);
			SET_STATIC_VERTEX_SHADER_COMBO(DECAL, 0);
			SET_STATIC_VERTEX_SHADER_COMBO(HALFLAMBERT, 0);
			SET_STATIC_VERTEX_SHADER_COMBO(LIGHTMAP_UV, 0);
			SET_STATIC_VERTEX_SHADER_COMBO(TREESWAY, 0);
			SET_STATIC_VERTEX_SHADER_COMBO(SEAMLESS_BASE, 0);
			SET_STATIC_VERTEX_SHADER_COMBO(DETAILTEXTURE_UV, 0);
			SET_STATIC_VERTEX_SHADER_COMBO(NORMALTEXTURE_UV, 0);
			SET_STATIC_VERTEX_SHADER_COMBO(ENVMAPMASK_UV, 0);
			SET_STATIC_VERTEX_SHADER(lux_model_vs30);
		}
		else
		{
			DECLARE_STATIC_VERTEX_SHADER(lux_brush_vs30);
			SET_STATIC_VERTEX_SHADER_COMBO(VERTEX_RGBA, 0); // This does not work with the below.
			SET_STATIC_VERTEX_SHADER_COMBO(SEAMLESS_BASE, 0);
			SET_STATIC_VERTEX_SHADER_COMBO(DETAILTEXTURE_UV, 0);
			SET_STATIC_VERTEX_SHADER_COMBO(NORMALTEXTURE_UV, 0); // No Lighting.
			SET_STATIC_VERTEX_SHADER_COMBO(ENVMAPMASK_UV, 0);
			SET_STATIC_VERTEX_SHADER(lux_brush_vs30);
		}

		DECLARE_STATIC_PIXEL_SHADER(lux_sky_hdri_ps30);
		SET_STATIC_PIXEL_SHADER_COMBO(MODE, iMode);
		SET_STATIC_PIXEL_SHADER_COMBO(HALF, bIsHalfHDRI);
		SET_STATIC_PIXEL_SHADER_COMBO(STRETCH, bHasStretch);
		SET_STATIC_PIXEL_SHADER_COMBO(TOSRGB, bWriteSRGB);
		SET_STATIC_PIXEL_SHADER(lux_sky_hdri_ps30);

	}

	DYNAMIC_STATE
	{

		BindTextureWithCheck(bHasTextureX1Y1, SHADER_SAMPLER0, info.m_nSkyTextureX1Y1, 0)
		BindTextureWithCheck(bHasTextureX2Y1, SHADER_SAMPLER1, info.m_nSkyTextureX2Y1, 0)
		BindTextureWithCheck(bHasTextureX3Y1, SHADER_SAMPLER2, info.m_nSkyTextureX3Y1, 0)
		BindTextureWithCheck(bHasTextureX4Y1, SHADER_SAMPLER3, info.m_nSkyTextureX4Y1, 0)
		BindTextureWithCheck(bHasTextureX1Y2, SHADER_SAMPLER4, info.m_nSkyTextureX1Y2, 0)
		BindTextureWithCheck(bHasTextureX2Y2, SHADER_SAMPLER5, info.m_nSkyTextureX2Y2, 0)
		BindTextureWithCheck(bHasTextureX3Y2, SHADER_SAMPLER6, info.m_nSkyTextureX3Y2, 0)
		BindTextureWithCheck(bHasTextureX4Y2, SHADER_SAMPLER7, info.m_nSkyTextureX4Y2, 0)

		// I force disable morphing, imagine if someone tried to morph the sky...
		// 'Warning, Stalkers! Emission is approaching! Find Shelter immediately!'
		// 'Внимание. Начинается выброс. Срочно ищите укрытие.' <- I put the russian version here because its the original.
		// Literally all we want is position...
		if (IS_FLAG_SET(MATERIAL_VAR_MODEL))
		{
			DECLARE_DYNAMIC_VERTEX_SHADER(lux_model_vs30);
			SET_DYNAMIC_VERTEX_SHADER_COMBO(MORPHING, 0); // g_pHardwareConfig->HasFastVertexTextures() && pShaderAPI->IsHWMorphingEnabled()
			SET_DYNAMIC_VERTEX_SHADER_COMBO(SKINNING, 0); // pShaderAPI->GetCurrentNumBones() > 0 ? true : false
			SET_DYNAMIC_VERTEX_SHADER_COMBO(STATICPROPLIGHTING, 0);
			SET_DYNAMIC_VERTEX_SHADER_COMBO(DYNAMICPROPLIGHTING, 0);
			SET_DYNAMIC_VERTEX_SHADER(lux_model_vs30);
		}
		else
		{
			DECLARE_DYNAMIC_VERTEX_SHADER(lux_brush_vs30);
			SET_DYNAMIC_VERTEX_SHADER(lux_brush_vs30);
		}
		DECLARE_DYNAMIC_PIXEL_SHADER(lux_sky_hdri_ps30);
		SET_DYNAMIC_PIXEL_SHADER_COMBO(PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo()); // pShaderAPI->GetPixelFogCombo()
		SET_DYNAMIC_PIXEL_SHADER_COMBO(WRITE_DEPTH_TO_DESTALPHA, pShaderAPI->ShouldWriteDepthToDestAlpha());
		SET_DYNAMIC_PIXEL_SHADER_COMBO(WRITEWATERFOGTODESTALPHA, 0);
		SET_DYNAMIC_PIXEL_SHADER(lux_sky_hdri_ps30);

		BOOL BBools[16] = { false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false };
		pShaderAPI->SetBooleanPixelShaderConstant(0, BBools, 16, true);


		f4Empty(f4EyePos);
		pShaderAPI->GetWorldSpaceCameraPosition(f4EyePos);
		// HDR Textures need to be multiplied by 16.0f
		// Non HDR Textures don't. This way we don't need to do much on our Shader...
		
		pShaderAPI->SetPixelShaderConstant(PSREG_EYEPOS_SPEC_EXPONENT, f4EyePos, 1);

		f4Empty(f4Controls);
		GetVec3ParamValue(info.m_nHDRITint, f4Controls);

		// Rotational Factor.
		static const float OneDividedBy360 = 1.0f / 360.0f;
		f4Controls[3] = GetIntParamValue(info.m_nRotation) * OneDividedBy360;
		if (bHDR)
		{
			f4Controls[0] *= 16.0f;
			f4Controls[1] *= 16.0f;
			f4Controls[2] *= 16.0f;
		}
		pShaderAPI->SetPixelShaderConstant(PSREG_DIFFUSE_MODULATION, f4Controls, 1);
		

//		DECLARE_DYNAMIC_VERTEX_SHADER(lux_screenspace_vs30);
//		SET_DYNAMIC_VERTEX_SHADER(lux_screenspace_vs30);

//		BindTextureWithCheck(bHasBaseTexture, SHADER_SAMPLER0, info.m_nBaseTexture, 0)
	}

	pShader->Draw();
}

// ShiroDkxtro2 : Brainchild of Totterynine.
// Declare param declarations separately then call that from within shader declaration.
// This makes it possible to easily run multiple shaders in one file
BEGIN_VS_SHADER(LUX_HDRI, "ShiroDkxtro2's ACROHS Shader-Rewrite Shader")

BEGIN_SHADER_PARAMS
LuxScreenspace_ParameterDeclaration()
END_SHADER_PARAMS

LuxSkyHDRI_Params()

SHADER_INIT_PARAMS()
{

	Sky_HDRI_Vars_t vars;
	LuxSkyHDRI_Link_Params(vars);
	LuxSkyHDRI_Init_Params(this, params, pMaterialName, vars);
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
	Sky_HDRI_Vars_t vars;
	LuxSkyHDRI_Link_Params(vars);
	LuxSkyHDRI_Shader_Init(this, params, vars);
}

SHADER_DRAW
{
	Sky_HDRI_Vars_t vars;
	LuxSkyHDRI_Link_Params(vars);
	LuxSkyHDRI_Shader_Draw(this, params, pShaderAPI, pShaderShadow, vars, vertexCompression, pContextDataPtr);
}
END_SHADER