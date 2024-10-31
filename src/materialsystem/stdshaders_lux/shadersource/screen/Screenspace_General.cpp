//========= ShiroDkxtro2's --ACROHS Ultimate Shaders Project-- ============//
//
//	Initial D.	:	20.01.2023 DMY
//	Last Change :	11.06.2023 DMY
//
//	Purpose of this File :	Handles any incoming requests for post processing.
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
#include "lux_screenspace_vs30.inc"
#include "lux_post_alphablend_ps30.inc"

// X360APPCHOOSER Is missing. Why have that...
#define LuxScreenspace_Params() void LuxScreenspace_Link_Params(Screenspace_Vars_t &info)\
{																						\
	info.m_nPixelShader				= PIXSHADER;										\
	info.m_nDisableColorWrites		= DISABLE_COLOR_WRITES;								\
	info.m_nAlphatested				= ALPHATESTED;										\
	info.m_nLinearRead_BaseTexture	= LINEARREAD_BASETEXTURE;							\
	info.m_nLinearRead_Texture1		= LINEARREAD_TEXTURE1;								\
	info.m_nLinearRead_Texture2		= LINEARREAD_TEXTURE2;								\
	info.m_nLinearRead_Texture3		= LINEARREAD_TEXTURE3;								\
	info.m_nLinearWrite				= LINEARWRITE;										\
																						\
	info.m_nC0_X					= C0_X;												\
	info.m_nC0_Y					= C0_Y;												\
	info.m_nC0_Z					= C0_Z;												\
	info.m_nC0_W					= C0_W;												\
	info.m_nC1_X					= C1_X;												\
	info.m_nC1_Y					= C1_Y;												\
	info.m_nC1_Z					= C1_Z;												\
	info.m_nC1_W					= C1_W;												\
	info.m_nC2_X					= C2_X;												\
	info.m_nC2_Y					= C2_Y;												\
	info.m_nC2_Z					= C2_Z;												\
	info.m_nC2_W					= C2_W;												\
	info.m_nC3_X					= C3_X;												\
	info.m_nC3_Y					= C3_Y;												\
	info.m_nC3_Z					= C3_Z;												\
	info.m_nC3_W					= C3_W;												\
	info.m_nTexture1				= TEXTURE1;											\
	info.m_nTexture2				= TEXTURE2;											\
	info.m_nTexture3				= TEXTURE3;											\
																						\
	info.m_nAlphaBlendingEnabled	= ALPHABLEND;										\
	Link_GlobalParameters()																\
}//-------------------------------------------------------------------------------------|

#define LuxScreenspace_ParameterDeclaration()																													\
SHADER_PARAM(PIXSHADER					, SHADER_PARAM_TYPE_STRING	, "", "I'm too lazy to add a real description to this right now, fix it for me, thanks!")	\
SHADER_PARAM(DISABLE_COLOR_WRITES		, SHADER_PARAM_TYPE_BOOL	, "", "I'm too lazy to add a real description to this right now, fix it for me, thanks!")	\
SHADER_PARAM(ALPHATESTED				, SHADER_PARAM_TYPE_BOOL	, "", "I'm too lazy to add a real description to this right now, fix it for me, thanks!")	\
SHADER_PARAM(LINEARREAD_BASETEXTURE		, SHADER_PARAM_TYPE_BOOL	, "", "I'm too lazy to add a real description to this right now, fix it for me, thanks!")	\
SHADER_PARAM(LINEARREAD_TEXTURE1		, SHADER_PARAM_TYPE_BOOL	, "", "I'm too lazy to add a real description to this right now, fix it for me, thanks!")	\
SHADER_PARAM(LINEARREAD_TEXTURE2		, SHADER_PARAM_TYPE_BOOL	, "", "I'm too lazy to add a real description to this right now, fix it for me, thanks!")	\
SHADER_PARAM(LINEARREAD_TEXTURE3		, SHADER_PARAM_TYPE_BOOL	, "", "I'm too lazy to add a real description to this right now, fix it for me, thanks!")	\
SHADER_PARAM(LINEARWRITE				, SHADER_PARAM_TYPE_BOOL	, "", "I'm too lazy to add a real description to this right now, fix it for me, thanks!")	\
SHADER_PARAM(C0_X						, SHADER_PARAM_TYPE_FLOAT	, "", "I'm too lazy to add a real description to this right now, fix it for me, thanks!")	\
SHADER_PARAM(C0_Y						, SHADER_PARAM_TYPE_FLOAT	, "", "I'm too lazy to add a real description to this right now, fix it for me, thanks!")	\
SHADER_PARAM(C0_Z						, SHADER_PARAM_TYPE_FLOAT	, "", "I'm too lazy to add a real description to this right now, fix it for me, thanks!")	\
SHADER_PARAM(C0_W						, SHADER_PARAM_TYPE_FLOAT	, "", "I'm too lazy to add a real description to this right now, fix it for me, thanks!")	\
SHADER_PARAM(C1_X						, SHADER_PARAM_TYPE_FLOAT	, "", "I'm too lazy to add a real description to this right now, fix it for me, thanks!")	\
SHADER_PARAM(C1_Y						, SHADER_PARAM_TYPE_FLOAT	, "", "I'm too lazy to add a real description to this right now, fix it for me, thanks!")	\
SHADER_PARAM(C1_Z						, SHADER_PARAM_TYPE_FLOAT	, "", "I'm too lazy to add a real description to this right now, fix it for me, thanks!")	\
SHADER_PARAM(C1_W						, SHADER_PARAM_TYPE_FLOAT	, "", "I'm too lazy to add a real description to this right now, fix it for me, thanks!")	\
SHADER_PARAM(C2_X						, SHADER_PARAM_TYPE_FLOAT	, "", "I'm too lazy to add a real description to this right now, fix it for me, thanks!")	\
SHADER_PARAM(C2_Y						, SHADER_PARAM_TYPE_FLOAT	, "", "I'm too lazy to add a real description to this right now, fix it for me, thanks!")	\
SHADER_PARAM(C2_Z						, SHADER_PARAM_TYPE_FLOAT	, "", "I'm too lazy to add a real description to this right now, fix it for me, thanks!")	\
SHADER_PARAM(C2_W						, SHADER_PARAM_TYPE_FLOAT	, "", "I'm too lazy to add a real description to this right now, fix it for me, thanks!")	\
SHADER_PARAM(C3_X						, SHADER_PARAM_TYPE_FLOAT	, "", "I'm too lazy to add a real description to this right now, fix it for me, thanks!")	\
SHADER_PARAM(C3_Y						, SHADER_PARAM_TYPE_FLOAT	, "", "I'm too lazy to add a real description to this right now, fix it for me, thanks!")	\
SHADER_PARAM(C3_Z						, SHADER_PARAM_TYPE_FLOAT	, "", "I'm too lazy to add a real description to this right now, fix it for me, thanks!")	\
SHADER_PARAM(C3_W						, SHADER_PARAM_TYPE_FLOAT	, "", "I'm too lazy to add a real description to this right now, fix it for me, thanks!")	\
SHADER_PARAM(TEXTURE1					, SHADER_PARAM_TYPE_TEXTURE , "", "I'm too lazy to add a real description to this right now, fix it for me, thanks!")	\
SHADER_PARAM(TEXTURE2					, SHADER_PARAM_TYPE_TEXTURE , "", "I'm too lazy to add a real description to this right now, fix it for me, thanks!")	\
SHADER_PARAM(TEXTURE3					, SHADER_PARAM_TYPE_TEXTURE , "", "I'm too lazy to add a real description to this right now, fix it for me, thanks!")	\
SHADER_PARAM(ALPHABLEND					, SHADER_PARAM_TYPE_BOOL	, "", "I'm too lazy to add a real description to this right now, fix it for me, thanks!")	\
//--------------------------------------------------------------------------------------------------------------------------------------------------------------|

void LuxScreenspace_Init_Params(CBaseVSShader *pShader, IMaterialVar **params, const char *pMaterialName, Screenspace_Vars_t &info)
{
	SET_FLAGS2(MATERIAL_VAR2_NEEDS_FULL_FRAME_BUFFER_TEXTURE);
}

void LuxScreenspace_Shader_Init(CBaseVSShader *pShader, IMaterialVar **params, Screenspace_Vars_t &info)
{
	// TextureFlags don't matter here.
	LoadTextureWithCheck(info.m_nBaseTexture, 0)
	LoadTextureWithCheck(info.m_nTexture1, 0)
	LoadTextureWithCheck(info.m_nTexture2, 0)
	LoadTextureWithCheck(info.m_nTexture3, 0)
}

void LuxScreenspace_Shader_Draw(CBaseVSShader *pShader, IMaterialVar **params, IShaderDynamicAPI *pShaderAPI, IShaderShadow *pShaderShadow, Screenspace_Vars_t &info, VertexCompressionType_t vertexCompression, CBasePerMaterialContextData **pContextDataPtr)
{
	bool bHasBaseTexture	=	IsTextureLoaded(info.m_nBaseTexture);
	bool bHasTexture1		=	IsTextureLoaded(info.m_nTexture1);
	bool bHasTexture2		=	IsTextureLoaded(info.m_nTexture2);
	bool bHasTexture3		=	IsTextureLoaded(info.m_nTexture3);
//	bool bDoAlphaBlending	=	GetBoolParamValue(info.m_nAlphaBlendingEnabled);

	if (pShader->IsSnapshotting())
	{
		pShaderShadow->EnableDepthWrites(false);
		pShaderShadow->EnableDepthTest(false);

		if (bHasBaseTexture)
		{
			ITexture *pBaseTexture = params[info.m_nBaseTexture]->GetTextureValue();
			ImageFormat BaseTextureIF = pBaseTexture->GetImageFormat();
			if (BaseTextureIF == IMAGE_FORMAT_RGBA16161616F || BaseTextureIF == IMAGE_FORMAT_RGBA16161616)
				EnableSampler(SHADER_SAMPLER0, false)
			else
			EnableSampler(SHADER_SAMPLER0, GetBoolParamValue(info.m_nLinearRead_BaseTexture))
			Warning("Screenspace Texture1 = %s \n", params[info.m_nBaseTexture]->GetStringValue());
		}

		if (bHasTexture1)
		{
			ITexture *pTexture1 = params[info.m_nTexture1]->GetTextureValue();
			ImageFormat Texture1IF = pTexture1->GetImageFormat();
			if (Texture1IF == IMAGE_FORMAT_RGBA16161616F || Texture1IF == IMAGE_FORMAT_RGBA16161616)
				EnableSampler(SHADER_SAMPLER1, false)
			else
			EnableSampler(SHADER_SAMPLER1, GetBoolParamValue(info.m_nLinearRead_Texture1))
			Warning("Screenspace Texture1 = %s \n", params[info.m_nTexture1]->GetStringValue());
		}

		if (bHasTexture2)
		{
			ITexture *pTexture2 = params[info.m_nTexture2]->GetTextureValue();
			ImageFormat Texture2IF = pTexture2->GetImageFormat();
			if (Texture2IF == IMAGE_FORMAT_RGBA16161616F || Texture2IF == IMAGE_FORMAT_RGBA16161616)
				EnableSampler(SHADER_SAMPLER1, false)
			else
			EnableSampler(SHADER_SAMPLER1, GetBoolParamValue(info.m_nLinearRead_Texture2))
			Warning("Screenspace Texture2 = %s \n", params[info.m_nTexture2]->GetStringValue());
		}

		if (bHasTexture3)
		{
			ITexture *pTexture3 = params[info.m_nTexture3]->GetTextureValue();
			ImageFormat Texture3IF = pTexture3->GetImageFormat();
			if (Texture3IF == IMAGE_FORMAT_RGBA16161616F || Texture3IF == IMAGE_FORMAT_RGBA16161616)
				EnableSampler(SHADER_SAMPLER1, false)
			else
			EnableSampler(SHADER_SAMPLER1, GetBoolParamValue(info.m_nLinearRead_Texture3))
			Warning("Screenspace Texture3 = %s \n", params[info.m_nTexture3]->GetStringValue());
		}

		int VertexShaderFlags = VERTEX_POSITION;
		pShaderShadow->VertexShaderVertexFormat(VertexShaderFlags, 1, 0, 0);
		pShaderShadow->EnableSRGBWrite(!GetBoolParamValue(info.m_nLinearWrite));

		DECLARE_STATIC_VERTEX_SHADER(lux_screenspace_vs30);
		SET_STATIC_VERTEX_SHADER(lux_screenspace_vs30);

		if (GetBoolParamValue(info.m_nDisableColorWrites))
			pShaderShadow->EnableColorWrites(false);

			pShaderShadow->EnableAlphaTest(true);
			pShaderShadow->AlphaFunc(SHADER_ALPHAFUNC_GREATER, 0.0);

		if (IS_FLAG_SET(MATERIAL_VAR_ADDITIVE))
		{
			pShader->EnableAlphaBlending(SHADER_BLEND_ONE, SHADER_BLEND_ONE);
		}

	}
	else // Dynamic state starts here.
	{
		// Whatever this is...
		float c0[] = {
			GetFloatParamValue(info.m_nC0_X),
			GetFloatParamValue(info.m_nC0_Y),
			GetFloatParamValue(info.m_nC0_Z),
			GetFloatParamValue(info.m_nC0_W),
			GetFloatParamValue(info.m_nC1_X),
			GetFloatParamValue(info.m_nC1_Y),
			GetFloatParamValue(info.m_nC1_Z),
			GetFloatParamValue(info.m_nC1_W),
			GetFloatParamValue(info.m_nC2_X),
			GetFloatParamValue(info.m_nC2_Y),
			GetFloatParamValue(info.m_nC2_Z),
			GetFloatParamValue(info.m_nC2_W),
			GetFloatParamValue(info.m_nC3_X),
			GetFloatParamValue(info.m_nC3_Y),
			GetFloatParamValue(info.m_nC3_Z),
			GetFloatParamValue(info.m_nC3_W)
		};
		pShaderAPI->SetPixelShaderConstant(0, c0, ARRAYSIZE(c0) / 4);


		f4Empty(f4EyePos);
		pShaderAPI->GetWorldSpaceCameraPosition(f4EyePos);
		pShaderAPI->SetPixelShaderConstant(PSREG_EYEPOS_SPEC_EXPONENT, f4EyePos, 1);

		DECLARE_DYNAMIC_VERTEX_SHADER(lux_screenspace_vs30);
		SET_DYNAMIC_VERTEX_SHADER(lux_screenspace_vs30);

		//pShaderAPI->SetVertexShaderIndex(0);
		//pShaderAPI->SetPixelShaderIndex(0);

		BindTextureWithCheck(bHasBaseTexture, SHADER_SAMPLER0, info.m_nBaseTexture, -1)
		BindTextureWithCheck(bHasTexture1, SHADER_SAMPLER1, info.m_nTexture1, -1)
		BindTextureWithCheck(bHasTexture2, SHADER_SAMPLER2, info.m_nTexture2, -1)
		BindTextureWithCheck(bHasTexture3, SHADER_SAMPLER3, info.m_nTexture3, -1)
	}

	pShader->Draw();
}

// Lux shaders will replace whatever already exists.
DEFINE_FALLBACK_SHADER(SDK_Screenspace_general, LUX_Screenspace_General)

// ShiroDkxtro2 : Brainchild of Totterynine.
// Declare param declarations separately then call that from within shader declaration.
// This makes it possible to easily run multiple shaders in one file
BEGIN_VS_SHADER(LUX_Screenspace_General, "ShiroDkxtro2's ACROHS Shader-Rewrite Shader")

BEGIN_SHADER_PARAMS
LuxScreenspace_ParameterDeclaration()
END_SHADER_PARAMS

LuxScreenspace_Params()

SHADER_INIT_PARAMS()
{
	Screenspace_Vars_t vars;
	LuxScreenspace_Link_Params(vars);
	LuxScreenspace_Init_Params(this, params, pMaterialName, vars);
}

SHADER_FALLBACK
{
	// OVERRIDE
	// TODO: REMOVE LATER WHEN THIS SHADER WAS DONE
//	return "Screenspace_General"; // This will not work on Mapbase.

	if (g_pHardwareConfig->GetDXSupportLevel() < 90)
	{
		Warning("Game run at DXLevel < 90 \n");
		return "Wireframe";
	}
	return 0;
}

SHADER_INIT
{
	Screenspace_Vars_t vars;
	LuxScreenspace_Link_Params(vars);
	LuxScreenspace_Shader_Init(this, params, vars);
}

SHADER_DRAW
{
	Screenspace_Vars_t vars;
	LuxScreenspace_Link_Params(vars);
	LuxScreenspace_Shader_Draw(this, params, pShaderAPI, pShaderShadow, vars, vertexCompression, pContextDataPtr);

#ifdef ACROHS_CSM
	// Function Here
#endif

}
END_SHADER