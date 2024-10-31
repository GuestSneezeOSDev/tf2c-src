//========= ShiroDkxtro2's --ACROHS Ultimate Shaders Project-- ============//
//
//	Initial D.	:	22.05.2023 DMY
//	Last Change :	24.05.2023 DMY
//
//	Purpose of this File :	'Infected' Shader for Models.
//
//===========================================================================//

// File in which you can determine what to use.
// We do not use preprocessor definitions for this because this is honestly faster and easier to set up.
#include "../../ACROHS_Defines.h"

// All Shaders use the same struct and sampler definitions. This way multipass CSM doesn't need to re-setup the struct
#include "../../ACROHS_Shared.h"

// Custom Struct due to the difference to other shaders
#include "Infected.h"

// We need all of these
#include "../CommonFunctions.h"
#include "../../cpp_shader_constant_register_map.h"

// This shader has no additional renderpasses.

// Includes for Shaderfiles...
#include "lux_model_vs30.inc"
#include "lux_infected_ps30.inc"

extern ConVar mat_fullbright;
extern ConVar mat_printvalues;
static ConVar lux_debug("lux_debug", "0", NULL);
// There is no fallback for this Shader.
//extern ConVar mat_oldshaders;
extern ConVar mat_specular;
#ifdef LIGHTWARP
extern ConVar mat_disable_lightwarp;
#endif

// Used for random number generation on parameter init.
int RandomInteger(int min, int max)
{
	return min + rand() % (max - min + 1);
}

// COLOR is for brushes. COLOR2 for models
#define LuxInfected_Params()   void LuxInfected_Link_Params(Infected_Vars_t &info)\
{											 \
	info.m_nColor =  COLOR2;				  \
	info.m_nMultipurpose7 = MATERIALNAME;	   \
	info.m_nDisableVariation = DISABLEVARIATION;\
	info.m_nGradientTexture = GRADIENTTEXTURE;	 \
	info.m_nSheetIndex = SHEETINDEX;			  \
	info.m_nColorTintGradient = COLORTINTGRADIENT; \
	info.m_nSkinTintGradient = SKINTINTGRADIENT;	\
	info.m_nSkinPhongExponent = SKINPHONGEXPONENT;	 \
	info.m_nBloodColor = BLOODCOLOR;				  \
	info.m_nBloodPhongExponent = BLOODPHONGEXPONENT;   \
	info.m_nBloodSpecBoost = BLOODSPECBOOST;			\
	info.m_nBloodMaskRange = BLOODMASKRANGE;			 \
	info.m_nDefaultPhongExponent = DEFAULTPHONGEXPONENT;  \
	info.m_nBurning = BURNING;							   \
	info.m_nBurnStrength = BURNSTRENGTH;					\
	info.m_nBurnDetailTexture = BURNDETAILTEXTURE;			 \
	info.m_nEyeGlow = EYEGLOW;								  \
	info.m_nEyeGlowColor = EYEGLOWCOLOR;					   \
	info.m_nEyeGlowFlashlightBoost = EYEGLOWFLASHLIGHTBOOST;	\
	info.m_nRandom1 = RANDOM1;\
	info.m_nRandom2 = RANDOM2;\
	info.m_nRandom3 = RANDOM3;\
	info.m_nRandom4 = RANDOM4;\
	Link_MiscParameters()										 \
	Link_PhongParameters()										  \
	Link_GlobalParameters()										   \
	Link_DetailTextureParameters()									\
	Link_NormalTextureParameters()									 \
}//-------------------------------------------------------------------|

//==================================================================================================
void LuxInfected_Init_Params(CBaseVSShader *pShader, IMaterialVar **params, const char *pMaterialName, Infected_Vars_t &info)
{
	// I wanted to debug print some values and needed to know which material they belong to. However pMaterialName is not available to the draw!
	// Well, now it is...
	params[info.m_nMultipurpose7]->SetStringValue(pMaterialName);

	if (!IsParamDefined(info.m_nBumpMap))
		params[info.m_nBumpMap]->SetStringValue("Balls"); // We want light data from static lights, string suggestion by roman_memes
	else
		params[info.m_nNormalTexture]->SetStringValue(params[info.m_nBumpMap]->GetStringValue());

	// Infected Shader only used on models.
	SET_FLAGS(MATERIAL_VAR_MODEL);

	// The usual...
	Flashlight_BorderColorSupportCheck();

	// Default Value is supposed to be 1.0f
	FloatParameterDefault(info.m_nDetailBlendFactor, 1.0f)

	// Default Value is supposed to be 4.0f
	FloatParameterDefault(info.m_nDetailScale, 4.0f)

#ifdef PHONG_REFLECTIONS
	if (IsParamDefined(info.m_nPhong) && GetBoolParamValue(info.m_nPhong))
	{
		// Default values are supposed to be 0, 0.5, 1
		Vec3ParameterDefault(info.m_nPhongFresnelRanges, 1.0f, 0.5f, 1.0f)
	}
#endif
	static const float Ratio = 1.0f / 32.0f;
	static const float Offset = (1.0f / 16.0f); 
	FloatParameterDefault(info.m_nRandom1, Ratio + (float)RandomInteger(0, 7) * Offset + 0.5f)
	FloatParameterDefault(info.m_nRandom2, Ratio + (float)RandomInteger(0, 7) * Offset)
	FloatParameterDefault(info.m_nRandom3, 0.5f * (float)RandomInteger(0, 1))
	FloatParameterDefault(info.m_nRandom4, 0.5f * (float)RandomInteger(0, 1))
	Warning("Current Randoms : %f %f %f %f \n", GetFloatParamValue(info.m_nRandom1), GetFloatParamValue(info.m_nRandom2), GetFloatParamValue(info.m_nRandom3), GetFloatParamValue(info.m_nRandom4));

	// Same Default Values as L4D2 below. Thank you Zappy for listing these in your Source Filmmaker Guide!
	Vec3ParameterDefault(info.m_nBloodColor, 1, 0, 0)	
	Vec2ParameterDefault(info.m_nBloodMaskRange, 0.0f, 1.0f)
	FloatParameterDefault(info.m_nDefaultPhongExponent, 5.0f)
	FloatParameterDefault(info.m_nBloodPhongExponent, 5.0f)
	FloatParameterDefault(info.m_nBloodSpecBoost, 1)
	FloatParameterDefault(info.m_nEyeGlowFlashlightBoost, 100.0f)
}

//==================================================================================================
void LuxInfected_Shader_Init(CBaseVSShader *pShader, IMaterialVar **params, Infected_Vars_t &info)
{
	// Always needed...
	pShader->LoadTexture(info.m_nFlashlightTexture, TEXTUREFLAGS_SRGB);
	SET_FLAGS2(MATERIAL_VAR2_SUPPORTS_HW_SKINNING);             // Required for skinning
	SET_FLAGS2(MATERIAL_VAR2_LIGHTING_VERTEX_LIT);              // Required for dynamic lighting
	SET_FLAGS2(MATERIAL_VAR2_NEEDS_BAKED_LIGHTING_SNAPSHOTS);   // Required for ambient cube
	SET_FLAGS2(MATERIAL_VAR2_SUPPORTS_FLASHLIGHT);				// Yes, Yes we support
	SET_FLAGS2(MATERIAL_VAR2_USE_FLASHLIGHT);					// Yes, Yes we use it, what did you think? "Yeah we support it but don't use it, please"(?)
	SET_FLAGS2(MATERIAL_VAR2_NEEDS_TANGENT_SPACES);             // Required for dynamic lighting
	SET_FLAGS2(MATERIAL_VAR2_DIFFUSE_BUMPMAPPED_MODEL);         // Required for dynamic lighting

	LoadTextureWithCheck(info.m_nBaseTexture, TEXTUREFLAGS_SRGB)
	LoadNormalTexture(info.m_nNormalTexture)
	LoadTextureWithCheck(info.m_nGradientTexture, 0)

	// I can't make sense of whatever they tried to do on the Stock Shaders
	// They load Detail as sRGB but then don't do sRGB Read... and vice versa.
	// We will just check if this  Texture has the flag and do sRGB read based on that
	LoadTextureWithCheck(info.m_nDetailTexture, 0)
}

//==================================================================================================
void LuxInfected_Shader_Draw(
	CBaseVSShader *pShader, IMaterialVar **params, IShaderDynamicAPI *pShaderAPI,
	IShaderShadow *pShaderShadow, Infected_Vars_t &info, VertexCompressionType_t vertexCompression,
	CBasePerMaterialContextData **pContextDataPtr)
{
	// Bools
	bool bHasFlashlight				=	pShader->UsingFlashlight(params);
	bool bAlphatest					=	false; // IS_FLAG_SET(MATERIAL_VAR_ALPHATEST);
	bool bHalfLambert				=	IS_FLAG_SET(MATERIAL_VAR_HALFLAMBERT);
//	bool bBlendTintByBaseAlpha		=	GetIntParamValue(info.m_nBlendTintByBaseAlpha) != 0;

	// Texture related Boolean. We check for existing bools first because its faster
	bool	bHasBaseTexture			=	IsTextureLoaded(info.m_nBaseTexture);
	bool	bHasNormalTexture		=	IsTextureLoaded(info.m_nNormalTexture);
	bool	bHasGradientTexture		=	IsTextureLoaded(info.m_nGradientTexture);
	//==================================================================================================
#ifdef DETAILTEXTURING
	bool	bHasDetailTexture		=	IsTextureLoaded(info.m_nDetailTexture);
#else
	bool	bHasDetailTexture		=	false;
#endif
	//==================================================================================================
	//==================================================================================================
#ifdef PHONG_REFLECTIONS
//	bool	bHasPhong				=	GetIntParamValue(info.m_nPhong) != 0; // Can only use Phong if you have a NormalTexture

#else
	bool	bHasPhong				=	false;
#endif
	bool IsOpaque(bIsFullyOpaque, info.m_nBaseTexture, bAlphatest);

	//===========================================================================//
	// Snapshotting State
	// This gets setup basically once, or again when using the flashlight
	// Once this is done we use dynamic state.
	//===========================================================================//
	if (pShader->IsSnapshotting())
	{
		//===========================================================================//
		// Alphatesting, VertexShaderFormat, sRGBWrite...
		//===========================================================================//
		const bool bFastVertexTextures = g_pHardwareConfig->HasFastVertexTextures();
		
		pShaderShadow->EnableAlphaTest(bAlphatest);
		if (params[info.m_nAlphaTestReference]->GetFloatValue() > 0.0f) // 0 is default.
		{
			pShaderShadow->AlphaFunc(SHADER_ALPHAFUNC_GEQUAL, params[info.m_nAlphaTestReference]->GetFloatValue());
		}
		
		pShaderShadow->EnableAlphaWrites(bIsFullyOpaque);

		if (bFastVertexTextures)
		{
			// Required for Morphing
			SET_FLAGS2(MATERIAL_VAR2_USES_VERTEXID); // Use the Vertex ID Stream
		}

		//===========================================================================//
		// VertexFormat
		//===========================================================================//
		unsigned int flags = VERTEX_POSITION | VERTEX_NORMAL | VERTEX_FORMAT_COMPRESSED;
		if (IS_FLAG_SET(MATERIAL_VAR_VERTEXCOLOR) || IS_FLAG_SET(MATERIAL_VAR_VERTEXALPHA))
			flags |= VERTEX_COLOR;

		int iTexCoords = 1;
		if (g_pHardwareConfig->HasFastVertexTextures() && IS_FLAG_SET(MATERIAL_VAR_DECAL))
			iTexCoords = 3;
		int pTexCoordDim[3] = { 2, 0, 3 };

		// This enables morphing data for materials with bumpmaps. Lightmapped models don't morph... sooo
		int iUserDataSize = (bHasFlashlight || bHasNormalTexture) ? 4 : 0;

		pShaderShadow->VertexShaderVertexFormat(flags, iTexCoords, pTexCoordDim, iUserDataSize);
		
		pShaderShadow->EnableSRGBWrite(true); // We always do sRGB Writes. We use ps30.

		//===========================================================================//
		// Enable Samplers
		//===========================================================================//
		EnableSampler(SAMPLER_BASETEXTURE, false) // We always have a basetexture, and no its NOT sRGB
		EnableSampler(SAMPLER_NORMALTEXTURE, false)

		// Same Sampler as LightWarp Texture
		EnableSamplerWithCheck(bHasGradientTexture, SHADER_SAMPLER6, false)

#ifdef DETAILTEXTURING
		// ShiroDkxtro2: Stock Shaders do some cursed Ternary stuff with the detailblendmode
		// Which... If I interpret it correctly makes it read non-sRGB maps as sRGB and vice versa...
		if (bHasDetailTexture)
		{
			ITexture *pDetailTexture = params[info.m_nDetailTexture]->GetTextureValue();
			EnableSampler(SAMPLER_DETAILTEXTURE, (pDetailTexture->GetFlags() & TEXTUREFLAGS_SRGB) ? true : false)
		}
#endif

		EnableFlashlightSamplers(bHasFlashlight, SAMPLER_SHADOWDEPTH, SAMPLER_RANDOMROTATION, SAMPLER_FLASHLIGHTCOOKIE, info.m_nBaseTexture)

		//===========================================================================//
		// Declare Static Shaders using the function array
		//===========================================================================//
		DECLARE_STATIC_VERTEX_SHADER(lux_model_vs30);
		SET_STATIC_VERTEX_SHADER_COMBO(DECAL, 0);
		SET_STATIC_VERTEX_SHADER_COMBO(HALFLAMBERT, 0); // No Lighting.
		SET_STATIC_VERTEX_SHADER_COMBO(LIGHTMAP_UV, 0); // No Lighting.
		SET_STATIC_VERTEX_SHADER_COMBO(TREESWAY, 0);
		SET_STATIC_VERTEX_SHADER_COMBO(SEAMLESS_BASE, 0);
		SET_STATIC_VERTEX_SHADER_COMBO(DETAILTEXTURE_UV, 0);
		SET_STATIC_VERTEX_SHADER_COMBO(NORMALTEXTURE_UV, 1); // we need bumped lighting
		SET_STATIC_VERTEX_SHADER_COMBO(ENVMAPMASK_UV, 0);
		SET_STATIC_VERTEX_SHADER(lux_model_vs30);

		DECLARE_STATIC_PIXEL_SHADER(lux_infected_ps30);
		SET_STATIC_PIXEL_SHADER_COMBO(FLASHLIGHT, bHasFlashlight);
		SET_STATIC_PIXEL_SHADER_COMBO(FLASHLIGHTDEPTHFILTERMODE, bHasFlashlight ? g_pHardwareConfig->GetShadowFilterMode() : 0);
		SET_STATIC_PIXEL_SHADER_COMBO(DETAILTEXTURE, bHasDetailTexture); // bHasDetailTexture
		SET_STATIC_PIXEL_SHADER_COMBO(BLENDTINTBYBASEALPHA, 0);
		SET_STATIC_PIXEL_SHADER(lux_infected_ps30);


	}
	else // End of Snapshotting ------------------------------------------------------------------------------------------------------------------------------------------------------------------
	{
		// Getting the light state
		LightState_t LightState;
		pShaderAPI->GetDX9LightState(&LightState);

		//===========================================================================//
		// Bind Textures
		//===========================================================================//
		BindTextureWithCheckAndFallback(bHasBaseTexture, SAMPLER_BASETEXTURE, info.m_nBaseTexture, info.m_nBaseTextureFrame, TEXTURE_WHITE)
		BindTextureWithCheckAndFallback(bHasNormalTexture, SAMPLER_NORMALTEXTURE, info.m_nNormalTexture, info.m_nBumpFrame, TEXTURE_NORMALMAP_FLAT)
		BindTextureWithCheck(bHasGradientTexture, SHADER_SAMPLER6, info.m_nGradientTexture, 0)

		// if mat_fullbright 2. Bind a standard white texture...
#ifdef DEBUG_FULLBRIGHT2 
		if (mat_fullbright.GetInt() == 2 && !IS_FLAG_SET(MATERIAL_VAR_NO_DEBUG_OVERRIDE))
			BindTextureStandard(SAMPLER_BASETEXTURE, TEXTURE_GREY)
#endif

#ifdef DETAILTEXTURING
		BindTextureWithCheck(bHasDetailTexture, SAMPLER_DETAILTEXTURE, info.m_nDetailTexture, info.m_nDetailFrame)
#endif

	//===========================================================================//
	// Prepare floats for Constant Registers
	//===========================================================================//
	// f4Empty is just float4 Name = {1,1,1,1};
	// Yes I have some excessive problems with macro overusage...
		f4Empty(f4BloodColor__)
		f4Empty(f4DetailTint_BlendFactor)
		f4Empty(f4RandomisationFloats)
		f4Empty(f4BloodMaskAndPhong)
		f4Empty(f4PhongExponents)
		f4Empty(f4PhongTintBoost)
		f4Empty(f4PhongFresnelRanges__)

		f4RandomisationFloats[0] = GetFloatParamValue(info.m_nRandom1);
		f4RandomisationFloats[1] = GetFloatParamValue(info.m_nRandom2);
		f4RandomisationFloats[2] = GetFloatParamValue(info.m_nRandom3);
		f4RandomisationFloats[3] = GetFloatParamValue(info.m_nRandom4);

		GetVec2ParamValue(info.m_nBloodMaskRange, f4BloodMaskAndPhong);
		f4BloodMaskAndPhong[3]	 = GetFloatParamValue(info.m_nBloodSpecBoost);
		f4BloodMaskAndPhong[4]	 = GetFloatParamValue(info.m_nBloodPhongExponent);

		GetVec3ParamValue(info.m_nBloodColor, f4BloodColor__);
		f4BloodColor__[3] = lux_debug.GetInt();

		f4PhongExponents[0] = GetFloatParamValue(info.m_nDefaultPhongExponent);
		f4PhongExponents[1] = f4PhongExponents[0];
		f4PhongExponents[2] = GetFloatParamValue(info.m_nSkinPhongExponent);
		f4PhongExponents[3] = 0; // GetFloatParamValue(info.m_nDetailPhongExponent);

		GetVec3ParamValue(info.m_nPhongTint, f4PhongTintBoost);
		f4PhongTintBoost[3] = GetFloatParamValue(info.m_nPhongBoost);

		GetVec3ParamValue(info.m_nPhongFresnelRanges, f4PhongFresnelRanges__);

#ifdef DETAILTEXTURING
	if (bHasDetailTexture)
	{
		// $DetailTint and $DetailBlendFactor
		GetVec3ParamValue(info.m_nDetailTint, f4DetailTint_BlendFactor);
		f4DetailTint_BlendFactor[3] = GetFloatParamValue(info.m_nDetailBlendFactor);
	}
#endif

	BOOL BBools[16] = { false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false };
	BBools[0] = bHalfLambert; // Always used.

	// FIXME: reenable.
	BBools[2] = false; // LightState.m_bAmbientLight; // We do this instead of dynamic combo.

	// This does a lot of things for us!!
	// Just don't touch constants <32 and you should be fine c:
	bool bFlashlightShadows = false;
	SetupStockConstantRegisters(bHasFlashlight, SAMPLER_FLASHLIGHTCOOKIE, SAMPLER_RANDOMROTATION, info.m_nFlashlightTexture, info.m_nFlashlightTextureFrame, bFlashlightShadows)

	int iFogIndex = 0;
	bool bWriteDepthToAlpha = false;
	bool bWriteWaterFogToAlpha = false;
	SetupDynamicComboVariables(iFogIndex, bWriteDepthToAlpha, bWriteWaterFogToAlpha, bIsFullyOpaque)

	//==================================================================================================
	// Set Pixelshader Constant Registers (PSREG's)
	//==================================================================================================
	pShaderAPI->SetPixelShaderConstant(32, f4BloodColor__, 1);
	pShaderAPI->SetPixelShaderConstant(33, f4DetailTint_BlendFactor, 1);
	pShaderAPI->SetPixelShaderConstant(34, f4BloodMaskAndPhong, 1);
	pShaderAPI->SetPixelShaderConstant(35, f4PhongExponents, 1);
//	pShaderAPI->SetPixelShaderConstant(36, f4EnvMapFresnelRanges_, 1);
	pShaderAPI->SetPixelShaderConstant(37, f4RandomisationFloats, 1);

	pShaderAPI->SetPixelShaderConstant(42, f4PhongTintBoost, 1);
	pShaderAPI->SetPixelShaderConstant(43, f4PhongFresnelRanges__, 1);

	BBools[13] = IS_FLAG_SET(MATERIAL_VAR_VERTEXCOLOR);
	BBools[14] = IS_FLAG_SET(MATERIAL_VAR_VERTEXALPHA);
	BBools[15] = bWriteDepthToAlpha;

	// ALWAYS! Required for HalfLambert, LightWarpTexture, and more.
	pShaderAPI->SetBooleanPixelShaderConstant(0, BBools, 16, true);

	pShaderAPI->CommitPixelShaderLighting(PSREG_LIGHT_INFO_ARRAY);
	pShaderAPI->SetPixelShaderStateAmbientLightCube(PSREG_AMBIENT_CUBE, !LightState.m_bAmbientLight); // Force To Black

	//==================================================================================================
	// Set Vertexshader Constant Registers (VSREG's...?) Also some other stuff
	//==================================================================================================
	// Always having this
	pShader->SetVertexShaderTextureTransform(VERTEX_SHADER_SHADER_SPECIFIC_CONST_13, info.m_nBaseTextureTransform);

	/*
	if (g_pHardwareConfig->HasFastVertexTextures())
	{
		// This is a function that was previously... locked behind an #ifndef and shouldn't have been able to be used...
		SetHWMorphVertexShaderState(pShaderAPI, VERTEX_SHADER_SHADER_SPECIFIC_CONST_10, VERTEX_SHADER_SHADER_SPECIFIC_CONST_11, SHADER_VERTEXTEXTURE_SAMPLER0);

		bool bUnusedTexCoords[3] = { false, false, !pShaderAPI->IsHWMorphingEnabled() };
		pShaderAPI->MarkUnusedVertexFields(0, 3, bUnusedTexCoords);
	}
	*/

	//==================================================================================================
	// Declare Dynamic Shaders ( Using FunctionArray Lookup Table )
	//==================================================================================================
	bool bHasStaticPropLighting = bStaticLightVertex(LightState); // If you are wondering what this function does, check its description!
	bool bHasDynamicPropLighting = LightState.m_bAmbientLight || (LightState.m_nNumLights > 0) ? 1 : 0;

	DECLARE_DYNAMIC_VERTEX_SHADER(lux_model_vs30);
	SET_DYNAMIC_VERTEX_SHADER_COMBO(MORPHING, g_pHardwareConfig->HasFastVertexTextures() && pShaderAPI->IsHWMorphingEnabled());
	SET_DYNAMIC_VERTEX_SHADER_COMBO(SKINNING, (pShaderAPI->GetCurrentNumBones() > 0 ? 1 : 0));
	SET_DYNAMIC_VERTEX_SHADER_COMBO(STATICPROPLIGHTING, bHasStaticPropLighting);
	SET_DYNAMIC_VERTEX_SHADER_COMBO(DYNAMICPROPLIGHTING, bHasDynamicPropLighting);
	SET_DYNAMIC_VERTEX_SHADER(lux_model_vs30);

	DECLARE_DYNAMIC_PIXEL_SHADER(lux_infected_ps30);
	SET_DYNAMIC_PIXEL_SHADER_COMBO(PIXELFOGTYPE, iFogIndex);
	SET_DYNAMIC_PIXEL_SHADER_COMBO(WRITEWATERFOGTODESTALPHA, bWriteWaterFogToAlpha);
	SET_DYNAMIC_PIXEL_SHADER_COMBO(NUM_LIGHTS, LightState.m_nNumLights);
	SET_DYNAMIC_PIXEL_SHADER_COMBO(FLASHLIGHTSHADOWS, bFlashlightShadows);
	SET_DYNAMIC_PIXEL_SHADER(lux_infected_ps30);

	if (mat_printvalues.GetBool())
	{
		Warning("Material %s now printing its values. \n", params[info.m_nMultipurpose7]->GetStringValue());
		Warning("Randomisation : %f %f %f %f \n", f4RandomisationFloats[0], f4RandomisationFloats[1], f4RandomisationFloats[2], f4RandomisationFloats[3]);
	}

	} // End of Snapshot/Dynamic state

	if (mat_printvalues.GetBool())
	{
		mat_printvalues.SetValue(0);
	}


	// ShiroDkxtro2:	I had it happen a bunch of times...
	//		pShader->Draw(); MUST BE above the final }
	//		It is very easy to mess this up ( done so several times )
	//		Game will just crash and debug leads you to a bogus function.
	pShader->Draw();

} // End of lux*_Shader_Draw

// ShiroDkxtro2 : Brainchild of Totterynine.
// Declare param declarations separately then call that from within shader declaration.
// This makes it possible to easily run multiple shaders in one file
BEGIN_VS_SHADER(infected, "ShiroDkxtro2's ACROHS Shader-Rewrite Shader")

BEGIN_SHADER_PARAMS
Declare_MiscParameters()
Declare_NormalTextureParameters()
SHADER_PARAM(MATERIALNAME, SHADER_PARAM_TYPE_STRING, "", "") // Can't use pMaterialName on Draw so we put it on this parameter instead...

// Parameters unique to Infected Shader start here.

// Exists on L4D2 but doesn't actually do anything.
//SHADER_PARAM(CHEAPDIFFUSE, SHADER_PARAM_TYPE_BOOL, "", "")

// Exists on L4D2 but only causes rendering issues when used.
//SHADER_PARAM(RTTSHADOWBUILD, SHADER_PARAM_TYPE_BOOL, "", "")

// Exists on L4D2 but use is unknown. Is it really a string?
//SHADER_PARAM(TRANSLUCENT_MATERIAL, SHADER_PARAM_TYPE_STRING, "", "")

// Exists on L4D2 but we don't have controlable SSAO effects here.
//SHADER_PARAM(AMBIENTOCCLUSION, SHADER_PARAM_TYPE_BOOL, "", "")

// The following parameters are for debugging purposes, for the wound system we don't have.
//SHADER_PARAM(DEBUGELLIPSOIDS, SHADER_PARAM_TYPE_BOOL, "", "")
//SHADER_PARAM(ELLIPSOIDCENTER, SHADER_PARAM_TYPE_VEC3, "", "")
//SHADER_PARAM(ELLIPSOIDUP, SHADER_PARAM_TYPE_VEC3, "", "")
//SHADER_PARAM(ELLIPSOIDLOOKAT, SHADER_PARAM_TYPE_VEC3, "", "")
//SHADER_PARAM(ELLIPSOIDSCALE, SHADER_PARAM_TYPE_VEC3, "", "")
//SHADER_PARAM(ELLIPSOIDCENTER2, SHADER_PARAM_TYPE_VEC3, "", "")
//SHADER_PARAM(ELLIPSOIDUP2, SHADER_PARAM_TYPE_VEC3, "", "")
//SHADER_PARAM(ELLIPSOIDLOOKAT2, SHADER_PARAM_TYPE_VEC3, "", "")
//SHADER_PARAM(ELLIPSOIDSCALE2, SHADER_PARAM_TYPE_VEC3, "", "")
//SHADAR_PARAM(ELLIPSOID2CULLTYPE, SHADER_PARAM_TYPE_INT, "", "")

// The following parameters, we don't know what they do.
//SHADER_PARAM(WOUNDCUTOUTTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "")
//SHADER_PARAM(CUTOUTTEXTUREBIAS, SHADER_PARAM_TYPE_FLOAT, "", "")
//SHADER_PARAM(CUTOUTDECALFALLOFF, SHADER_PARAM_TYPE_FLOAT, "", "")
//SHADER_PARAM(CUTOUTDECALMAPPINGSCALE, SHADER_PARAM_TYPE_FLOAT, "", "")

SHADER_PARAM(DISABLEVARIATION, SHADER_PARAM_TYPE_BOOL, "", "Param since Last Stand, disables diffuse variations, $basetexture will act as actual $basetexture, Alpha may be used as Specular Mask")
SHADER_PARAM(GRADIENTTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "1D Color palette, applied to the model via Gradient Mapping")
SHADER_PARAM(SHEETINDEX, SHADER_PARAM_TYPE_INTEGER, "", "Pick a specific part of the basetexture instead of determining which one to use via randomisation.")
SHADER_PARAM(COLORTINTGRADIENT, SHADER_PARAM_TYPE_INTEGER, "", "Force a specific horizontal gradient slice for color ( lower 8 slices, usually clothing ) to be used, instead of randomly picking a slice.")
SHADER_PARAM(DEFAULTPHONGEXPONENT, SHADER_PARAM_TYPE_FLOAT, "", "Same as $PhongExponent but its only used on unmasked parts of 'the texture'")
SHADER_PARAM(SKINTINTGRADIENT, SHADER_PARAM_TYPE_INTEGER, "", "Force a specific horizontal gradient slice for skin to be used, instead of randomly picking a slice.")
SHADER_PARAM(SKINPHONGEXPONENT, SHADER_PARAM_TYPE_FLOAT, "", "Equivalent to $PhongExponent, except for skin parts of the texture ( Blue Channel )")
SHADER_PARAM(BLOODCOLOR, SHADER_PARAM_TYPE_VEC3, "[1 1 1]", "Color of the blood 'pass'")
SHADER_PARAM(BLOODPHONGEXPONENT, SHADER_PARAM_TYPE_FLOAT, "", "Equivalent to $PhongExponent, except for blood parts of the texture ( Green Channel )")
SHADER_PARAM(BLOODSPECBOOST, SHADER_PARAM_TYPE_FLOAT, "", "Equivalent to $phongboost, except for bloody parts of the texture ( Green Channel )")
SHADER_PARAM(BLOODMASKRANGE, SHADER_PARAM_TYPE_VEC2, "", "Not sure how exactly this was supposed to work.")
SHADER_PARAM(BURNING, SHADER_PARAM_TYPE_BOOL, "", "This is non functional right now. Controls whether the material should appear burning. It needs to be in a different VMT with a '_burning' suffix.")
SHADER_PARAM(BURNSTRENGTH, SHADER_PARAM_TYPE_FLOAT, "", "This is non functional right now.")
SHADER_PARAM(BURNDETAILTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "This is non functional right now.")
SHADER_PARAM(EYEGLOW, SHADER_PARAM_TYPE_BOOL, "0", "Set to 1 to enable the effect. Causes an additive color overlay, aswell as different phong settings, eyeglow is controlled by the bottom half of the red channel of the $basetexture.")
SHADER_PARAM(EYEGLOWCOLOR, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "Color of the effect, not sure if this tints or is shadowcolor")
SHADER_PARAM(EYEGLOWFLASHLIGHTBOOST, SHADER_PARAM_TYPE_FLOAT, "", "Phong boost for projected Textures. ( Such as, the flashlight )")
SHADER_PARAM(RANDOM1, SHADER_PARAM_TYPE_FLOAT, "", "Used internally for the randomisation")
SHADER_PARAM(RANDOM2, SHADER_PARAM_TYPE_FLOAT, "", "Used internally for the randomisation")
SHADER_PARAM(RANDOM3, SHADER_PARAM_TYPE_FLOAT, "", "Used internally for the randomisation")
SHADER_PARAM(RANDOM4, SHADER_PARAM_TYPE_FLOAT, "", "Used internally for the randomisation")

#ifdef DETAILTEXTURING
Declare_DetailTextureParameters()
#endif

#ifdef PHONG_REFLECTIONS
Declare_PhongParameters()
#endif

END_SHADER_PARAMS

// Setup Params
LuxInfected_Params()
/*
LuxCloak_Params()
LuxEmissiveBlend_Params()
LuxFleshInterior_Params()
*/

SHADER_INIT_PARAMS()
{
	Infected_Vars_t vars;
	LuxInfected_Link_Params(vars);
	LuxInfected_Init_Params(this, params, pMaterialName, vars);

	/*
	if (IS_FLAG2_SET(MATERIAL_VAR2_CLOAK))
	{
		Cloak_Vars_t CloakVars;
		LuxCloak_Link_Params(CloakVars);
		LuxCloak_Init_Params(this, params, pMaterialName, CloakVars);
	}

	if (IS_FLAG2_SET(MATERIAL_VAR2_EMISSIVEBLEND))
	{
		EmissiveBlend_Vars_t EmissiveBlendVars;
		LuxEmissiveBlend_Link_Params(EmissiveBlendVars);
		LuxEmissiveBlend_Init_Params(this, params, pMaterialName, EmissiveBlendVars);
	}

	if (IS_FLAG2_SET(MATERIAL_VAR2_FLESHINTERIOR))
	{
		FleshInterior_Vars_t FleshInteriorVars;
		LuxFleshInterior_Link_Params(FleshInteriorVars);
		LuxFleshInterior_Init_Params(this, params, pMaterialName, FleshInteriorVars);
	}
	*/
}

SHADER_FALLBACK
{
	if(IS_FLAG2_SET(MATERIAL_VAR2_DISTANCEALPHA))
	{
		return "LUX_DistanceAlpha";
	}

	if (g_pHardwareConfig->GetDXSupportLevel() < 90)
	{
		Warning("Game run at DXLevel < 90 \n");
		return "Wireframe";
	}
	return 0;
}

SHADER_INIT
{
	Infected_Vars_t vars;
	LuxInfected_Link_Params(vars);
	LuxInfected_Shader_Init(this, params, vars);

	/*
	if (IS_FLAG2_SET(MATERIAL_VAR2_CLOAK))
	{
		Cloak_Vars_t CloakVars;
		LuxCloak_Link_Params(CloakVars);
		LuxCloak_Shader_Init(this, params, CloakVars);
	}

	if (IS_FLAG2_SET(MATERIAL_VAR2_EMISSIVEBLEND))
	{
		EmissiveBlend_Vars_t EmissiveBlendVars;
		LuxEmissiveBlend_Link_Params(EmissiveBlendVars);
		LuxEmissiveBlend_Shader_Init(this, params, EmissiveBlendVars);
	}

	if (IS_FLAG2_SET(MATERIAL_VAR2_FLESHINTERIOR))
	{
		FleshInterior_Vars_t FleshInteriorVars;
		LuxFleshInterior_Link_Params(FleshInteriorVars);
		LuxFleshInterior_Shader_Init(this, params, FleshInteriorVars);
	}
	*/
}

SHADER_DRAW
{
	// We only want a StandardPass if we could actually see it.
	bool bStandardPass = true;
	/*
	if (IS_FLAG2_SET(MATERIAL_VAR2_CLOAK) && (pShaderShadow == NULL)) // NOT Snapshotting.
	{
		Cloak_Vars_t CloakVars;
		LuxCloak_Link_Params(CloakVars);
		LuxCloak_Shader_Init(this, params, CloakVars);

		// Replicating Valve Code, This should  give the same result...
		float	CloakFactor = GetFloatParamValue(CloakVars.m_nCloakFactor);
		float	LerpCloakFactor = 1.0f + (clamp(CloakFactor, 0.0f, 1.0f) * (-0.35 - 1.0f));
				LerpCloakFactor = clamp(LerpCloakFactor, 0.0f, 1.0f); // Could potentially be negative, so clamp it... TODO: Math this out and optimise.

		if (LerpCloakFactor <= 0.4f)
			bStandardPass = false;
	}
	*/
	if (bStandardPass)
	{
		Infected_Vars_t vars;
		LuxInfected_Link_Params(vars);
		LuxInfected_Shader_Draw(this, params, pShaderAPI, pShaderShadow, vars, vertexCompression, pContextDataPtr);
	}
	else
	{
		Draw(false); // Skip the pass...
	}
	/*
	if (IS_FLAG2_SET(MATERIAL_VAR2_CLOAK))
	{
		Cloak_Vars_t CloakVars;
		LuxCloak_Link_Params(CloakVars);
		LuxCloak_Shader_Init(this, params, CloakVars);

		// Replicating Valve Code, This should  give the same result...
		float	CloakFactor = GetFloatParamValue(CloakVars.m_nCloakFactor);
		// Must Snapshot, when in Snapshot state otherwise the game will crash because there is no shader set up for use.
		// We also only want to render if CloakFactor is between 0.0f and 1.0f
		if ((pShaderShadow == NULL) || ((CloakFactor > 0.0f) && (CloakFactor < 1.0f)))
		{
			LuxCloak_Shader_Draw(this, params, pShaderAPI, pShaderShadow, CloakVars, vertexCompression, pContextDataPtr);
		}
		else
		{
			Draw(false);
		}
	}

	if (IS_FLAG2_SET(MATERIAL_VAR2_EMISSIVEBLEND))
	{
		EmissiveBlend_Vars_t EmissiveBlendVars;
		LuxEmissiveBlend_Link_Params(EmissiveBlendVars);

		if ((pShaderShadow != NULL) || params[EMISSIVEBLENDSTRENGTH]->GetFloatValue() > 0.0f)
		{
			LuxEmissiveBlend_Shader_Draw(this, params, pShaderAPI, pShaderShadow, EmissiveBlendVars, vertexCompression, pContextDataPtr);
		}
		else
		{
			// We're not snapshotting and we don't need to draw this frame.
			// This is important because we still have another renderpass set up that expects to do something for us.
			Draw(false);
		}
	}

	if (IS_FLAG2_SET(MATERIAL_VAR2_FLESHINTERIOR))
	{
		// Valve : If Snapshotting or "( true )"
		// Aka it always renders... What a waste of lines
		FleshInterior_Vars_t FleshInteriorVars;
		LuxFleshInterior_Link_Params(FleshInteriorVars);
		LuxFleshInterior_Shader_Draw(this, params, pShaderAPI, pShaderShadow, FleshInteriorVars, vertexCompression, pContextDataPtr);
	}
	*/

#ifdef ACROHS_CSM
	// Function Here
#endif

}
END_SHADER