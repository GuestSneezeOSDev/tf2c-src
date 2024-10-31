//
//========= ShiroDkxtro2's --ACROHS Ultimate Shaders Project-- ============//
//
//	Initial D.	:	24.01.2023 DMY
//	Last Change :	24.05.2023 DMY
//
//	Purpose of this File :	Shader Constant Register Declaration for
//							everything you would ever see on a Pixelshader
//
//===========================================================================//

#ifndef LUX_COMMON_PS_FXC_H_
#define LUX_COMMON_PS_FXC_H_

#include "common_pragmas.h"
#include "common_hlsl_cpp_consts.h"
#include "shader_constant_register_map.h"

//===========================================================================//
//	Declaring FLOAT PixelShader Constant Registers ( PSREG's )
//	There can be a maximum of 224 according to Microsoft Documentation
//	Ranging from 0-223. After this follows b0-b15 and i0 - i15
//===========================================================================//
// 0
	const float4	g_DiffuseModulation					: register(c1); // Colorspace
//	const float4	g_ShadowTweaks						: register(c2); // Flashlight ShadowTweaks
// 3
//	const float3	cAmbientCube[6]						: register(c4);
//	5-used by		cAmbientCube[1]
//	6-used by		cAmbientCube[2]
//	7-used by		cAmbientCube[3]
//	8-used by		cAmbientCube[4]
//	9-used by		cAmbientCube[5]
//	10
	const float4	g_EyePos							: register(c11);
	const float4	g_FogParams							: register(c12);
//	const float4	g_FlashlightAttenuationFactors		: register(c13);
//	const float4	g_FlashlightPos						: register(c14);
//	const float4x4	g_FlashlightWorldToTexture			: register(c15);
//	16--used by		g_FlashlightWorldToTexture
//	17--used by		g_FlashlightWorldToTexture
//	18--used by		g_FlashlightWorldToTexture
//	19
//	PixelShaderLightInfo cLightInfo[3]					: register(c20); // 4th light spread across w's
//	21-----------used by cLightInfo[1]
//	22-----------used by cLightInfo[2]
//	23-----------used by cLightInfo[2]
//	24-----------used by cLightInfo[3]
//	25-----------used by cLightInfo[3]
//	26
//	27
//	const float4	cFlashlightColor					: register(c28); // common_fxc.h
	const float4	g_LinearFogColor					: register(c29);
	const float4	cLightScale							: register(c30);
//	const float4	cFlashlightScreenScale				: register(c31); // common_fxc.h - .zw are currently unused
//	const float4	g_BaseTextureTint_Factor			: register(c32);
//	const float4	g_DetailTint_BlendFactor			: register(c33);
//	const float4	g_SelfIllumTint_Factor				: register(c34);
//	const float4	g_DetailMode						: register(c37);
//	const float4	CubeMapPos							: register(c38);
//	const float4x3	f4x3CorrectionMatrix				: register(c39);
//	40------used by f4x3CorrectionMatrix
//	41------used by f4x3CorrectionMatrix
//	const float4	g_PhongTint_Boost					: register(c42);
//	const float4	g_PhongFresnelRanges_Exponent		: register(c43);
//	const float4	g_PhongControls						: register(c44);
//	const float4	f4SelfIllumFresnel					: register(c45);
//	const float4	g_EnvMapSaturation_Contrast			: register(c46);
//	const float4	g_BaseTexture2Tint_Factor			: register(c47);
//	const float4	g_Detail2Tint_BlendFactor			: register(c48);
//	const float4	g_SelfIllum2Tint_Factor				: register(c49);
//	const float4	g_ParallaxControls					: register(c50); .zw empty

#define LINEAR_LIGHT_SCALE					(cLightScale.x)
#define LIGHT_MAP_SCALE						(cLightScale.y)
#define ENV_MAP_SCALE						(cLightScale.z)
#define GAMMA_LIGHT_SCALE					(cLightScale.w)
#define OO_DESTALPHA_DEPTH_RANGE			(g_LinearFogColor.w)

#define g_FogEndOverRange	g_FogParams.x
#define g_WaterZ			g_FogParams.y
#define g_FogMaxDensity		g_FogParams.z
#define g_FogOORange		g_FogParams.w

#define HDR_INPUT_MAP_SCALE 16.0f
#define TONEMAP_SCALE_NONE 0
#define TONEMAP_SCALE_LINEAR 1
#define TONEMAP_SCALE_GAMMA 2
#define PIXEL_FOG_TYPE_NONE -1 //MATERIAL_FOG_NONE is handled by PIXEL_FOG_TYPE_RANGE, this is for explicitly disabling fog in the shader
#define PIXEL_FOG_TYPE_RANGE 0 //range+none packed together in ps2b. Simply none in ps20 (instruction limits)
#define PIXEL_FOG_TYPE_HEIGHT 1
// "If you change these, make the corresponding change in hardwareconfig.cpp"
// Yeah as if.
#define NVIDIA_PCF_POISSON	0
#define ATI_NOPCF			1
#define ATI_NO_PCF_FETCH4	2

//===========================================================================//
//	Constants you will usually find on all shaders. Geometry ones anyways
//	NOTE:	It doesn't matter too much if we just declare one of these,
//			They are constant, if we don't use them, we don't use them.
//			I don't think it has a big memory impact at all.
//			I still tried to optimise this down though.
//===========================================================================//

const float4	g_BaseTextureTint_Factor		: register(c32);
#define			f3BaseTextureTint				(g_BaseTextureTint_Factor.xyz)
#define			f1BlendTintFactor				(g_BaseTextureTint_Factor.w)

#if DETAILTEXTURE
const float4	g_DetailTint_BlendFactor		: register(c33);
#define			f3DetailTextureTint				(g_DetailTint_BlendFactor.xyz)
#define			f1DetailBlendFactor				(g_DetailTint_BlendFactor.w)
#endif

// Always
const float4	g_DetailMode					: register(c37); // zy empty
#define			f1DetailBlendMode				(g_DetailMode.x)
//				f1DetailBlendMode2 ( WVT ) 		(g_DetailMode.y)
//				f1FogScale    ( DecalModulate )	(g_DetailMode.y)
#define			f1DesaturationFactor			(g_DetailMode.z)
//				f1FogExponent ( DecalModulate )	(g_DetailMode.w)

//===========================================================================//
//	Declaring BOOLEAN PixelShader Constant Registers ( PSREG's )
//	After this follows i0 - i15
//	Apparently these don't quite work.
//===========================================================================//
const bool		Bools[16]						: register(b0);
// All Shaders use this as bHalfLambert, undefine if we use it for something else later...
#if !defined(BRUSH) // Brushes can't use this.
#define			bHalfLambert					Bools[0]
#endif
#define			bLightWarpTexture				Bools[1]
#if !defined(BRUSH) // Brushes can't have this.
#define			bAmbientLight					Bools[2]
#endif

// Not available for Phonged Shaders
#if !defined(PHONG) && defined(SEAMLESS)
#define			bSeamless_Base					Bools[3]
#define			bSeamless_Detail				Bools[4]
#define			bSeamless_Bump					Bools[5]
#define			bSeamless_EnvMapMask			Bools[6]
#endif

// Phong :
//				bHasBaseAlphaPhongMask			Bools[3]
//				bHasPhongAlbedoTint				Bools[4]
//				bHasInvertedPhongMask			Bools[5]
//				bHasRimLightMask				Bools[6]
//				bHasBasemapLuminancePhongMask	Bools[7]
//				bHasPhongExponentTextureMask	Bools[8]
//				bHasRimLight					Bools[9]
//				bHasPhongWarpTexture			Bools[10]
//				bHasEnvMapFresnel				Bools[11]
//				bHasSelfIllumFresnel			Bools[12]

// WVT :
//				Required for flipping the Vertex Alpha when in Hammer.
//				bHammerMode						Bools[10]

//				On UnlitGeneric, LightmappedGeneric, VertexLitGeneric
#define			bVertexColor					Bools[13]
#define			bVertexAlpha					Bools[14]

//				Always. This saves us a dynamic combo, thus speeding up compile time, a lot.
#define			bDepthToDestAlpha				Bools[15]

/*
Important notes for Boolean constant registers :
2. You must define b0 and then use an array[16] if you want to use all the registers...
Yes, you can't do static const bool X : (b5)
The compiler will cry and tell you its non-proper semantics, so what you have to do is
static const bool mhm[6] : (b0)
#define bool x mhm[5]
If you think this is very inconvinient. YES. YES IT IS. And I had great fun figuring this issue out!

2. Boolean constants MUST be used with IF-Statements. They cannot be used with Ternary.
I had to figure this out the hard way ( SCell555 told me after 5 hours of me trying to get phong working :/ )
---Example code that works---
if(bHasSomething)
{
f1Value = NewValue;
}

---Example code that doesn't work---
f1Value = BHasSomething ? NewValue : f1Value;

Note that if-statements in the ASM Byte instructions will always have the else part. So don't think you can't go overboard with it.
*/

//===========================================================================//
//	Declaring INTEGER PixelShader Constant Registers ( PSREG's )
//	After this follow s0-s15
//===========================================================================//

//===========================================================================//
//	Declaring Samplers. We only have 16 on SM3.0. Ranging from 0-15
//	So we have to reuse them depending on what shader we are on and what features we require...
//	Note : Local definitions might be different. We don't always need to have clear names.
//===========================================================================//

sampler Sampler_BaseTexture		: register(s0);

#if defined(NORMALTEXTURE)
sampler Sampler_NormalTexture	: register(s2);
#endif

#if DETAILTEXTURE
sampler Sampler_DetailTexture	: register(s4);
#endif
// s7 used when Phong is defined
// s8 used when Phong is defined
// s9 EnvMapMask2 
// s10 Detail2
// s11 Lightmap Sampler
// s12 BlendModulate
// s13 Flashlight Depth || Selfillum
// s14 Flashlight RandomRot || Selfillum2 ( displacements )
// s15 Flashlight Cookie || EnvMap

//===========================================================================//
//	Common variables
//===========================================================================//

#if defined(HDTV_LUM_COEFFICIENTS)
static const float3 f3LumCoefficients = { 0.2126f, 0.7152f, 0.0722f }; // Rec. 709 HDTV
#else
static const float3 f3LumCoefficients = { 0.299f, 0.587f, 0.114f }; // NTSC Analog Television Standard
#endif

//===========================================================================//
// Function for retrieving a texture based on seamless coordinates.
//===========================================================================//
#if defined(SEAMLESS)
float4 tex2D_Seamless(sampler SamplerName, float3 f3UVW, float4 f4Weights)
{
	float4 f4Result = float4(0,0,0,0);
	float3 f3Weights = f4Weights.xyz;

	// TODO: Explain this. Why - 0.3f? And why the sum
	f3Weights = max( f3Weights - 0.3f, 0.0f );
	// TODO: Should we use a dot instead again for less instructions?
	// Also, this is probably used to average the three results we get so that we remain in linear space
	f3Weights *= 1.0f / ( f3Weights.x + f3Weights.y + f3Weights.z );
	
	// [branch] will make it use branching instructions.
	// In other terms, it will skip the else statement and maybe improve performance.
	// However not all hardware supports this. ( it was introduced in 2004, yeah all hardware probably supports it lol )
	//[branch]
	if (f3Weights.x > 0)
	{
		f4Result  += f3Weights.x * tex2D(SamplerName,  f3UVW.zy );
	}

	//[branch]
	if (f3Weights.y > 0)
	{
		f4Result += f3Weights.y * tex2D(SamplerName, f3UVW.xz);
	}

	//[branch]
	if (f3Weights.z > 0)
	{
		f4Result += f3Weights.z * tex2D(SamplerName, f3UVW.xy);
	}

	return f4Result;
}
#endif

#if defined(NORMALTEXTURE)
//===========================================================================//
// Compute the matrix used to transform tangent space normals to world space
// Taken from the Thexa4 PBR Shader. Doing it this way is ideal because we won't need vertex shader outputs!
// This expects DirectX normal maps in Mikk Tangent Space http://www.mikktspace.com
//===========================================================================//
float3x3 Compute_Tangent_Frame(float3 N, float3 P, float2 uv, out float3 T, out float3 B, out float sign_det)
{
	float3 dp1 = ddx(P);
	float3 dp2 = ddy(P);
	float2 duv1 = ddx(uv);
	float2 duv2 = ddy(uv);

	sign_det = dot(dp2, cross(N, dp1)) > 0.0 ? -1 : 1;

	float3x3 M = float3x3(dp1, dp2, cross(dp1, dp2));
	float2x3 inverseM = float2x3(cross(M[1], M[2]), cross(M[2], M[0]));
	T = normalize(mul(float2(duv1.x, duv2.x), inverseM));
	B = normalize(mul(float2(duv1.y, duv2.y), inverseM));
	return float3x3(T, B, N);
}
#endif // NORMALTEXTURE

float3 BlendPixelFog(const float3 vShaderColor, float pixelFogFactor, const float3 vFogColor, const int iPIXELFOGTYPE)
{
	if (iPIXELFOGTYPE == PIXEL_FOG_TYPE_RANGE) //either range fog or no fog depending on fog parameters and whether this is ps20 or ps2b
	{
		pixelFogFactor = saturate(pixelFogFactor);
		return lerp(vShaderColor.rgb, vFogColor.rgb, pixelFogFactor * pixelFogFactor); //squaring the factor will get the middle range mixing closer to hardware fog
	}
	else if (iPIXELFOGTYPE == PIXEL_FOG_TYPE_HEIGHT)
	{
		return lerp(vShaderColor.rgb, vFogColor.rgb, saturate(pixelFogFactor));
	}
	else if (iPIXELFOGTYPE == PIXEL_FOG_TYPE_NONE)
	{
		return vShaderColor;
	}
}

// From common_ps_fxc.h ( Stock Shaders )
float CalcWaterFogAlpha(const float flWaterZ, const float flEyePosZ, const float flWorldPosZ, const float flProjPosZ, const float flFogOORange)
{
	//	float flDepthFromWater = flWaterZ - flWorldPosZ + 2.0f; // hackity hack . .this is for the DF_FUDGE_UP in view_scene.cpp
	float flDepthFromWater = flWaterZ - flWorldPosZ;

	// if flDepthFromWater < 0, then set it to 0
	// This is the equivalent of moving the vert to the water surface if it's above the water surface
	// We'll do this with the saturate at the end instead.
	//	flDepthFromWater = max( 0.0f, flDepthFromWater );

	// Calculate the ratio of water fog to regular fog (ie. how much of the distance from the viewer
	// to the vert is actually underwater.
	float flDepthFromEye = flEyePosZ - flWorldPosZ;
	float f = saturate(flDepthFromWater * (1.0 / flDepthFromEye));

	// $tmp.w is now the distance that we see through water.
	return saturate(f * flProjPosZ * flFogOORange);
}

float CalcRangeFog(const float flProjPosZ, const float flFogStartOverRange, const float flFogMaxDensity, const float flFogOORange)
{
	return saturate(min(flFogMaxDensity, (flProjPosZ * flFogOORange) - flFogStartOverRange));
}

float CalcPixelFogFactor(int iPIXELFOGTYPE, const float4 fogParams, const float flEyePosZ, const float flWorldPosZ, const float flProjPosZ)
{
	float retVal;
	if (iPIXELFOGTYPE == PIXEL_FOG_TYPE_NONE)
	{
		retVal = 0.0f;
	}
	if (iPIXELFOGTYPE == PIXEL_FOG_TYPE_RANGE) //range fog, or no fog depending on fog parameters
	{
		retVal = CalcRangeFog(flProjPosZ, fogParams.x, fogParams.z, fogParams.w);
	}
	else if (iPIXELFOGTYPE == PIXEL_FOG_TYPE_HEIGHT) //height fog
	{
		retVal = CalcWaterFogAlpha(fogParams.y, flEyePosZ, flWorldPosZ, flProjPosZ, fogParams.w);
	}

	return retVal;
}

// TODO: Fix this load of crap.
// Ideas:	Use return after computing a result
//			Switch instead of if's
/*
texture combining modes for combining base and detail textures
ShiroDkxtro2   :
GIGA IMPORTANT :	
				The order in which these are listed is NOT 0-11 but actually 0, 7, 1, 4, 8, 2, 9, 3, 11.	5,6 for post-lighting
				Note : The Stock shaders order the blendmodes as			 7, 0, 1, 2, 3, 4, 8, 9, 11.	5,6 for post-lighting
				Here is why I changed the order...
				We no longer use a static combo to determine which of these to use, we just compute that at runtime.
				This means that if we determine the least likely blendmodes first we are wasting time, to boost runtime performance we change the order.
				Mr.Kleiner searched all vmt's from tf2, p2, l4d2, hl2, ep1, ep2, csgo, bms and asw. ( Which btw is a ton. Tf2 alone has 21k vmt's )
				Using advanced statistical analysis on the probability of detailblendmodes ( I ordered them 'Blendmode <-> Results' then used my eyes )
				I determined that '0, 7, 1, 4, 8, 10, 2, 9, 3, 11 ; 5,6' is the ideal ordering. For hl2,ep1,ep2,asw anyways.
				We have data for other games here however we don't *really* consider it and its just here for sake of documentation.
				You can use the data below to determine what'd be the best order for you.

				If you are a working for Valve and implementing this in some game.
				Which I doubt HAHAHA, AS IF, imagine something getting fixed in 10 year old valve-games and then its shaders of all things... LOL
				Anyways enough slander. Use 6,5 for TF2. 4466 vmt's use it, which is more than all other blendmodes in all games combined.
				If you are wondering, its used for the burning effect

				Our Results (23.02.2023) :

				This has to be considered because maybe they weren't used as much BECAUSE they don't work.
				However setting it to this order garuantees more performance for the games that already have this order established.
				mode 10-11 is pretty much never supported and we didn't get results for it ( except p2 ) so its XXXXX'd
				sdk13,tf2	- LMG mode 2-11		 not supported.
				CS:GO		- LMG mode 1-6 and 8 not supported. 9 not with $bumpmap, $envmap, or $selfillum
				asw			- LMG mode 6		 not supported. 7 not with $softedges
				sdk13,tf2	- WVT mode 2-9		 not supported.
				CS:GO		- WVT mode 1-6, 8-9	 not supported. 9 not with $bumpmap, $envmap, or $selfillum, 7 not with $blendmodulatetexture
				asw			- WVT mode 6		 not supported. 7 not with $blendmodulatetexture, or $bumpmap2
				sdk13,tf2	- VLG mode 10-11	 not supported. 5-9 not with $bumpmap, 5-6 only with $phong
				CS:GO		- VLG mode 10-11	 not supported. 5-6, 8-9 not with $bumpmap, 5-6 only with $phong
				asw			- VLG mode 10-11	 not supported. 5-9 not with $bumpmap, 5-6 only with $phong

				XXXXX = Shouldn't even work... Aka we had 0 results anyways
				bms, l4d2 and p2 are pretty much unknowns

				LightmappedGeneric																	VertexLitGeneric								
				+------+-------+-------+-------+-------+-------+-------+-------+-------+-------+	+------+-------+-------+-------+-------+-------+-------+-------+-------+-------+
				| Mode |  hl2  |  ep1  |  ep2  |  asw  |  bms  | csgo  | l4d2  |  p2   |  tf2  |	| Mode |  hl2  |  ep1  |  ep2  |  asw  |  bms  | csgo  | l4d2  |  p2   |  tf2  |
				+------+-------+-------+-------+-------+-------+-------+-------+-------+-------+	+------+-------+-------+-------+-------+-------+-------+-------+-------+-------+
				|  0   |  943  |   2   |  96   |  67   |  897  |  164  |  22   |  52   |  308  |	|  0   |  42   |   3   |  27   |   0   |  566  |  161  |  41   |  15   |  177  |
				+------+-------+-------+-------+-------+-------+-------+-------+-------+-------+	+------+-------+-------+-------+-------+-------+-------+-------+-------+-------+
				|  1   |   0   |   3   |   0   |   0   |   3   | XXXXX |   0   |   8   |   1   |	|  1   |   0   |   0   |   0   |  18   |   2   |   0   |   0   |   0   |  33   |
				+------+-------+-------+-------+-------+-------+-------+-------+-------+-------+	+------+-------+-------+-------+-------+-------+-------+-------+-------+-------+
				|  2   | XXXXX | XXXXX | XXXXX |   0   |   0   | XXXXX |   0   |   0   | XXXXX |	|  2   |   1   |   0   |   0   |   0   |   2   |   1   |   2   |  20   |   0   |
				+------+-------+-------+-------+-------+-------+-------+-------+-------+-------+	+------+-------+-------+-------+-------+-------+-------+-------+-------+-------+
				|  3   | XXXXX | XXXXX | XXXXX |   0   |   0   | XXXXX |   0   |   0   | XXXXX |	|  3   |   0   |   0   |   1   |   0   |   0   |   0   |   0   |   0   |   0   |
				+------+-------+-------+-------+-------+-------+-------+-------+-------+-------+	+------+-------+-------+-------+-------+-------+-------+-------+-------+-------+
				|  4   | XXXXX | XXXXX | XXXXX |   0   |   0   | XXXXX |   0   |   0   | XXXXX |	|  4   |   0   |   0   |   0   |   0   |   1   |  18   |   0   |   0   |   5   |
				+------+-------+-------+-------+-------+-------+-------+-------+-------+-------+	+------+-------+-------+-------+-------+-------+-------+-------+-------+-------+
				|  5   | XXXXX | XXXXX | XXXXX |   0   |   0   | XXXXX |   0   |   1   | XXXXX |	|  5   |   0   |   0   |   0   |  35   |   0   |   1   |   1   |   4   | 4466  | <--Bruh
				+------+-------+-------+-------+-------+-------+-------+-------+-------+-------+	+------+-------+-------+-------+-------+-------+-------+-------+-------+-------+
				|  6   | XXXXX | XXXXX | XXXXX | XXXXX |   0   |   0   |   0   |   0   | XXXXX |	|  6   |   0   |   0   |   0   |   0   |   0   |   0   |   0   |   0   |   0   |
				+------+-------+-------+-------+-------+-------+-------+-------+-------+-------+	+------+-------+-------+-------+-------+-------+-------+-------+-------+-------+
				|  7   | XXXXX | XXXXX | XXXXX |   2   |   0   |   4   |   1   |  28   | XXXXX |	|  7   |   0   |   0   |   0   |   0   |   4   |   0   |   1   |   9   |   0   |
				+------+-------+-------+-------+-------+-------+-------+-------+-------+-------+	+------+-------+-------+-------+-------+-------+-------+-------+-------+-------+
				|  8   | XXXXX | XXXXX | XXXXX |   0   |   0   | XXXXX |   0   |   0   | XXXXX |	|  8   |   0   |   0   |   0   |   0   |  16   |   2   |   0   |   0   |   0   |
				+------+-------+-------+-------+-------+-------+-------+-------+-------+-------+	+------+-------+-------+-------+-------+-------+-------+-------+-------+-------+
				|  9   | XXXXX | XXXXX | XXXXX |   0   |   0   |   1?  |   0   |   0   | XXXXX |	|  9   |   0   |   0   |   0   |   0   |   0   |   3   |   0   |   0   |   0   |
				+------+-------+-------+-------+-------+-------+-------+-------+-------+-------+	+------+-------+-------+-------+-------+-------+-------+-------+-------+-------+
				| 10   | XXXXX | XXXXX | XXXXX | XXXXX | XXXXX | XXXXX | XXXXX |  14   | XXXXX |	| 10   | XXXXX | XXXXX | XXXXX | XXXXX | XXXXX | XXXXX | XXXXX | XXXXX | XXXXX |
				+------+-------+-------+-------+-------+-------+-------+-------+-------+-------+	+------+-------+-------+-------+-------+-------+-------+-------+-------+-------+
				| 11   | XXXXX | XXXXX | XXXXX | XXXXX | XXXXX | XXXXX | XXXXX | XXXXX | XXXXX |	| 11   | XXXXX | XXXXX | XXXXX | XXXXX | XXXXX | XXXXX | XXXXX | XXXXX | XXXXX |
				+------+-------+-------+-------+-------+-------+-------+-------+-------+-------+	+------+-------+-------+-------+-------+-------+-------+-------+-------+-------+
																   ? 9 is not supported...
																															We haven't searched VMT's for this
				WorldVertexTransition																Lightmapped_4WayBlend	We don't have this Shader on sdk13/asw
				+------+-------+-------+-------+-------+-------+-------+-------+-------+-------+	+------+-------+
				| Mode |  hl2  |  ep1  |  ep2  |  asw  |  bms  | csgo  | l4d2  |  p2   |  tf2  |	| Mode | csgo  |
				+------+-------+-------+-------+-------+-------+-------+-------+-------+-------+	+------+-------+
				|  0   |   0   |   0   |   1   |   1   |   0   |   4   |   0   |   0   |   4   |	|  0   |   ?   |
				+------+-------+-------+-------+-------+-------+-------+-------+-------+-------+	+------+-------+
				|  1   |   0   |   0   |   0   |   0   |   0   | XXXXX |   0   |   0   |   0   |	|  1   |   ?   |
				+------+-------+-------+-------+-------+-------+-------+-------+-------+-------+	+------+-------+
				|  2   | XXXXX | XXXXX | XXXXX |   0   |   0   | XXXXX |   0   |   0   | XXXXX |	|  2   |   ?   |
				+------+-------+-------+-------+-------+-------+-------+-------+-------+-------+	+------+-------+
				|  3   | XXXXX | XXXXX | XXXXX |   0   |   0   | XXXXX |   0   |   0   | XXXXX |	|  3   |   ?   |
				+------+-------+-------+-------+-------+-------+-------+-------+-------+-------+	+------+-------+
				|  4   | XXXXX | XXXXX | XXXXX |   0   |   0   | XXXXX |   0   |   0   | XXXXX |	|  4   |   ?   |
				+------+-------+-------+-------+-------+-------+-------+-------+-------+-------+	+------+-------+
				|  5   | XXXXX | XXXXX | XXXXX |   0   |   0   | XXXXX |   0   |   0   | XXXXX |	|  5   |   ?   |
				+------+-------+-------+-------+-------+-------+-------+-------+-------+-------+	+------+-------+
				|  6   | XXXXX | XXXXX | XXXXX |   0   |   0   |   0   |   0   |   0   | XXXXX |	|  6   |   ?   |
				+------+-------+-------+-------+-------+-------+-------+-------+-------+-------+	+------+-------+
				|  7   | XXXXX | XXXXX | XXXXX |   0   |   0   |   0   |   0   |   0   | XXXXX |	|  7   |   ?   |
				+------+-------+-------+-------+-------+-------+-------+-------+-------+-------+	+------+-------+
				|  8   | XXXXX | XXXXX | XXXXX |   0   |   0   | XXXXX |   0   |   0   | XXXXX |	|  8   |   ?   |
				+------+-------+-------+-------+-------+-------+-------+-------+-------+-------+	+------+-------+
				|  9   | XXXXX | XXXXX | XXXXX |   0   |   0   |   0   |   0   |   0   | XXXXX |	|  9   |   ?   |
				+------+-------+-------+-------+-------+-------+-------+-------+-------+-------+	+------+-------+
				| 10   | XXXXX | XXXXX | XXXXX | XXXXX | XXXXX | XXXXX | XXXXX | XXXXX | XXXXX |	| 10   |   ?   |
				+------+-------+-------+-------+-------+-------+-------+-------+-------+-------+	+------+-------+
				| 11   | XXXXX | XXXXX | XXXXX | XXXXX | XXXXX | XXXXX | XXXXX | XXXXX | XXXXX |	| 11   |   ?   |
				+------+-------+-------+-------+-------+-------+-------+-------+-------+-------+	+------+-------+
*/
#if DETAILTEXTURE || DETAILTEXTURE2
#define TCOMBINE_RGB_EQUALS_BASE_x_DETAILx2 0				// original mode
#define TCOMBINE_RGB_ADDITIVE 1								// base.rgb+detail.rgb*fblend
#define TCOMBINE_DETAIL_OVER_BASE 2
#define TCOMBINE_FADE 3										// straight fade between base and detail.
#define TCOMBINE_BASE_OVER_DETAIL 4                         // use base alpha for blend over detail
#define TCOMBINE_RGB_ADDITIVE_SELFILLUM 5                   // add detail color post lighting
#define TCOMBINE_RGB_ADDITIVE_SELFILLUM_THRESHOLD_FADE 6
#define TCOMBINE_MOD2X_SELECT_TWO_PATTERNS 7				// use alpha channel of base to select between mod2x channels in r+a of detail
#define TCOMBINE_MULTIPLY 8
#define TCOMBINE_MASK_BASE_BY_DETAIL_ALPHA 9                // use alpha channel of detail to mask base
#define TCOMBINE_SSBUMP_BUMP 10								// use detail to modulate lighting as an ssbump
#define TCOMBINE_SSBUMP_NOBUMP 11					// detail is an ssbump but use it as an albedo. shader does the magic here - no user needs to specify mode 11

float4 TextureCombine(float4 baseColor, float4 detailColor, int combine_mode,
	float fBlendFactor)
{
	if (combine_mode == TCOMBINE_MOD2X_SELECT_TWO_PATTERNS)
	{
		float3 dc = lerp(detailColor.r, detailColor.a, baseColor.a);
		baseColor.rgb *= lerp(float3(1, 1, 1), 2.0*dc, fBlendFactor);
	}
	if (combine_mode == TCOMBINE_RGB_EQUALS_BASE_x_DETAILx2)
		baseColor.rgb *= lerp(float3(1, 1, 1), 2.0*detailColor.rgb, fBlendFactor);
	if (combine_mode == TCOMBINE_RGB_ADDITIVE)
		baseColor.rgb += fBlendFactor * detailColor.rgb;
	if (combine_mode == TCOMBINE_DETAIL_OVER_BASE)
	{
		float fblend = fBlendFactor * detailColor.a;
		baseColor.rgb = lerp(baseColor.rgb, detailColor.rgb, fblend);
	}
	if (combine_mode == TCOMBINE_FADE)
	{
		baseColor = lerp(baseColor, detailColor, fBlendFactor);
	}
	if (combine_mode == TCOMBINE_BASE_OVER_DETAIL)
	{
		float fblend = fBlendFactor * (1 - baseColor.a);
		baseColor.rgb = lerp(baseColor.rgb, detailColor.rgb, fblend);
		baseColor.a = detailColor.a;
	}
	if (combine_mode == TCOMBINE_MULTIPLY)
	{
		baseColor = lerp(baseColor, baseColor*detailColor, fBlendFactor);
	}

	if (combine_mode == TCOMBINE_MASK_BASE_BY_DETAIL_ALPHA)
	{
		baseColor.a = lerp(baseColor.a, baseColor.a*detailColor.a, fBlendFactor);
	}
	if (combine_mode == TCOMBINE_SSBUMP_NOBUMP)
	{
		baseColor.rgb = baseColor.rgb * dot(detailColor.rgb, 2.0 / 3.0);
	}
	return baseColor;
}

float3 TextureCombinePostLighting(float3 lit_baseColor, float4 detailColor, int combine_mode,
	float fBlendFactor)
{
	if (combine_mode == TCOMBINE_RGB_ADDITIVE_SELFILLUM)
		lit_baseColor += fBlendFactor * detailColor.rgb;
	if (combine_mode == TCOMBINE_RGB_ADDITIVE_SELFILLUM_THRESHOLD_FADE)
	{
		// fade in an unusual way - instead of fading out color, remap an increasing band of it from
		// 0..1
		//if (fBlendFactor > 0.5)
		//	lit_baseColor += min(1, (1.0/fBlendFactor)*max(0, detailColor.rgb-(1-fBlendFactor) ) );
		//else
		//	lit_baseColor += 2*fBlendFactor*2*max(0, detailColor.rgb-.5);

		float f = fBlendFactor - 0.5;
		float fMult = (f >= 0) ? 1.0 / fBlendFactor : 4 * fBlendFactor;
		float fAdd = (f >= 0) ? 1.0 - fMult : -0.5*fMult;
		lit_baseColor += saturate(fMult * detailColor.rgb + fAdd);
	}
	return lit_baseColor;
}
#endif // DETAILTEXTURE


float4 FinalOutput( const float4 vShaderColor, float pixelFogFactor, const int iPIXELFOGTYPE, const int iTONEMAP_SCALE_TYPE, bool bWriteDepthToDestAlpha = false, const float flProjZ = 1.0f )
{
	float4 result;
	if( iTONEMAP_SCALE_TYPE == TONEMAP_SCALE_LINEAR )
	{
		result.rgb = vShaderColor.rgb * LINEAR_LIGHT_SCALE;
	}
	else if( iTONEMAP_SCALE_TYPE == TONEMAP_SCALE_GAMMA )
	{
		result.rgb = vShaderColor.rgb * GAMMA_LIGHT_SCALE;
	}
	else if( iTONEMAP_SCALE_TYPE == TONEMAP_SCALE_NONE )
	{
		result.rgb = vShaderColor.rgb;
	}
	
	if( bWriteDepthToDestAlpha )
		result.a = flProjZ * OO_DESTALPHA_DEPTH_RANGE;
	else
		result.a = vShaderColor.a;

	result.rgb = BlendPixelFog( result.rgb, pixelFogFactor, g_LinearFogColor.rgb, iPIXELFOGTYPE );

	return result;
}

#endif // End of LUX_COMMON_PS_FXC_H_