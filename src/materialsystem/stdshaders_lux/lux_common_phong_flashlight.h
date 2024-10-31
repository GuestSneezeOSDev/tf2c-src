//
//========= ShiroDkxtro2's --ACROHS Ultimate Shaders Project-- ============//
//
//	Initial D.	:	21.02.2023 DMY
//	Last Change :	24.05.2023 DMY
//
//	Purpose of this File :	-Flashlight Constant Register Declarations
//							-Phong Functions
//
//===========================================================================//

#ifndef LUX_COMMON_PHONG_FLASHLIGHT
#define LUX_COMMON_PHONG_FLASHLIGHT

// Common Flashlight thingies,
// has shadow filters and constant register declarations for the flashlight
#include "lux_common_flashlight_2.h"

const float3 cAmbientCube[6]				: register(c4);
// 5-used by cAmbientCube[1]
// 6-used by cAmbientCube[2]
// 7-used by cAmbientCube[3]
// 8-used by cAmbientCube[4]
// 9-used by cAmbientCube[5]

// 38-41 are used by PCC. SO we avoid those for Brush Phong by using 42+
const float4	g_PhongTint_Boost				: register(c42);
const float4	g_PhongFresnelRanges_Exponent	: register(c43);
const float4	g_PhongControls					: register(c44);
#define			f3PhongTint						(g_PhongTint_Boost.xyz)
#define			f1PhongBoost					(g_PhongTint_Boost.w)
#define			f3PhongFresnelRanges			(g_PhongFresnelRanges_Exponent.xyz)
#define			f1PhongExponentParam			(g_PhongFresnelRanges_Exponent.w)
#define			f1AlbedoTintBoost				(g_PhongControls.x)
//#define		f1RimLightExponent				(g_PhongControls.y)
//#define		f1RimLightBoost					(g_PhongControls.z)

#define			bHasBaseAlphaPhongMask			Bools[3]
#define			bHasPhongAlbedoTint				Bools[4]
#define			bHasInvertedPhongMask			Bools[5]
//#define		bHasRimLightMask				Bools[6]
#define			bHasBasemapLuminancePhongMask	Bools[7]
#define			bHasPhongExponentTextureMask	Bools[8]
//#define		bHasRimLight					Bools[9]
#define			bHasPhongWarpTexture			Bools[10]

//===========================================================================//
//	Declaring Samplers. We only have 16 on SM3.0. Ranging from 0-15
//	So we have to reuse them depending on what shader we are on and what features we require...
//	Note : Local definitions might be different. We don't always need to have clear names.
//===========================================================================//

// Always!
sampler Sampler_PhongWarpTexture: register(s7);
sampler Sampler_PhongExpTexture : register(s8);

// Ripped from vertexlitgeneric header. And less function-routing
float3 AmbientLight(const float3 worldNormal, const float3 cAmbientCube[6])
{
	float3 linearColor, nSquared = worldNormal * worldNormal;
	float3 isNegative = (worldNormal >= 0.0) ? 0 : nSquared;
	float3 isPositive = (worldNormal >= 0.0) ? nSquared : 0;
	linearColor = isPositive.x * cAmbientCube[0] + isNegative.x * cAmbientCube[1] +
		isPositive.y * cAmbientCube[2] + isNegative.y * cAmbientCube[3] +
		isPositive.z * cAmbientCube[4] + isNegative.z * cAmbientCube[5];
	return linearColor;
}
// Special version with no rimlight.
float3 LUX_DoSpecularLight(float f1NdL, float3 f3Reflect, float3 f3LightDir, float3 f3LightColor, float f1SpecularExponent, float f1Fresnel)
{
	// L.R
	float	f1RdL = saturate(dot(f3Reflect, f3LightDir));
	float	f1Specular = pow(f1RdL, f1SpecularExponent); // Raise to the Power of the Exponent

	// Copy it around a bunch of times so we get a float3
	float3	f3Specular = float3(f1Specular, f1Specular, f1Specular);

	// Warp as a function of *Specular and Fresnel
	if (bHasPhongWarpTexture)
	{
		f3Specular *= tex2D(Sampler_PhongWarpTexture, float2(f3Specular.x, f1Fresnel));
	}
	else
	{	// If we didn't apply Fresnel through Warping, apply it manually.
		f3Specular *= f1Fresnel;
	}

	return (f3Specular * f1NdL * f3LightColor) ; // Mask with N.L and Modulation ( attenuation and color )
}

//===========================================================================//
//	New Flashlight Shadow Function
//===========================================================================//
float3 LUX_DoFlashlightSpecular(float3 f3WorldPosition, float3 f3FaceNormal, const int iFilterMode,
const bool bDoShadows, float3 f3ProjPos, float3 f3Reflect, float f1SpecularExponent, float f1Fresnel, out float3 f3Specular)
{
	// Speeds up rendering by not doing anything if this is the case.
	// TODO: What the heck is in the .w
//	if (g_FlashlightPos.w < 0)
//	{
//		return float3(0, 0, 0);
//	}
//	else
	{
		float4 f4FlashlightSpacePosition = mul(float4(f3WorldPosition, 1.0), g_FlashlightWorldToTexture);
		clip(f4FlashlightSpacePosition.w);

		float3 f3ProjCoords = f4FlashlightSpacePosition.xyz / f4FlashlightSpacePosition.w;

		float3 f3FlashlightColor = tex2D(Sampler_FlashlightCookie, f3ProjCoords.xy).xyz;

		// Removed some ifdef for Shadermodels here.
		f3FlashlightColor *= cFlashlightColor.xyz;

		float3	f3Delta = g_FlashlightPos.xyz - f3WorldPosition;
		float3	f3L = normalize(f3Delta);
		float	f1DistSquared = dot(f3Delta, f3Delta);
		float	f1Dist = sqrt(f1DistSquared);

		float	f1FarZ = g_FlashlightAttenuationFactors.w;
		float	f1EndFalloffFactor = RemapValClamped(f1Dist, f1FarZ, 0.6f * f1FarZ, 0.0f, 1.0f);

		// Attenuation for light and to fade out shadow over distance
		float	f1Atten = saturate(dot(g_FlashlightAttenuationFactors.xyz, float3(1.0f, 1.0f / f1Dist, 1.0f / f1DistSquared)));

		// Shadowing and coloring terms
		if (bDoShadows)
		{
			float f1Shadow = 0.0f;
			// These are stock Flashlight terms.
			float f1ScaleOverMapSize = g_ShadowTweaks.x * 2;		// Tweak parameters to shader
			float2 f2NoiseOffset = g_ShadowTweaks.zw;

			float2 f2ShadowMapCenter = f3ProjCoords.xy;			// Center of shadow filter
			float f1ObjectDepth = min(f3ProjCoords.z, 0.99999);	// Object depth in shadow space

			// 2D Rotation Matrix setup
			float3 f3RMatTop = 0, f3RMatBottom = 0;
			f3RMatTop.xy = tex2D(Sampler_RandomRotation, cFlashlightScreenScale.xy * ((f3ProjPos.xy / f3ProjPos.z) * 0.5f + 0.5f) + f2NoiseOffset) * 2.0f - 1.0f;
			f3RMatBottom.xy = float2(-1.0, 1.0) * f3RMatTop.yx;	// 2x2 rotation matrix in 4-tuple

			f3RMatTop *= f1ScaleOverMapSize;				// Scale up kernel while accounting for texture resolution
			f3RMatBottom *= f1ScaleOverMapSize;

			f3RMatTop.z = f2ShadowMapCenter.x;				// To be added in d2adds generated below
			f3RMatBottom.z = f2ShadowMapCenter.y;

			//////////////////////////////////////////////////////////////
			//	Filters are reused for Phong, now less duplicate code!	//
			//////////////////////////////////////////////////////////////
			f1Shadow = FilterShadow(iFilterMode, f1ObjectDepth, f3RMatTop, f3RMatBottom);

			float f1Attenuated = lerp(saturate(f1Shadow), 1.0f, g_ShadowTweaks.y);	// Blend between fully attenuated and not attenuated

			f1Shadow = saturate(lerp(f1Attenuated, f1Shadow, f1Atten));	// Blend between shadow and above, according to light attenuation
			f3FlashlightColor *= f1Shadow;									// Shadow term
		}

		float3 f3DiffuseLighting = f1Atten;
		f3DiffuseLighting *= saturate(dot(f3L.xyz, f3FaceNormal) + f1FlashlightNoLambertValue); // NoLambertValue is either 0 or 2


		f3DiffuseLighting *= f3FlashlightColor;
		f3DiffuseLighting *= f1EndFalloffFactor;

		float	f1L1NdL = saturate(dot(f3FaceNormal, f3L));
		f3Specular = LUX_DoSpecularLight(f1L1NdL, f3Reflect, f3L, f3DiffuseLighting, f1SpecularExponent, f1Fresnel);
		return f3DiffuseLighting;
	}
}
//#endif // #if FLASHLIGHT


float3 LUX_DoSpecular
(float3 f3Normal, float3 f3WorldPos,
float f1NdotV, float3 f3EyeDir, float f1SpecularExponent, float f1Fresnel,
float3 f3PhongModulation, float3 f3BaseTexture, const int iFilterMode = 0,
const bool bDoShadows = false, float3 f3ProjPos = float3(0.0f, 0.0f, 0.0f))
{
	float3	f3BumpedLight = float3(0, 0, 0);
	float3	f3Specular = float3(0, 0, 0);
	float f1PhongFresnel;

	// This code was commented from common_vertexlitgeneric_dx9.h but it actually works by comparison... Using whats t here will cause weird purple artifacts and look nothing like the original ( Hunter model )
	if (f1Fresnel > 0.5f)
	{
		f1PhongFresnel = lerp(f3PhongFresnelRanges.y, f3PhongFresnelRanges.z, (2.0f * f1Fresnel) - 1.0f);
	}
	else
	{
		f1PhongFresnel = lerp(f3PhongFresnelRanges.x, f3PhongFresnelRanges.y, 2.0f * f1Fresnel);
	}

	if (bAmbientLight)
	{
		f3BumpedLight = AmbientLight(f3Normal, cAmbientCube);
	}

	// Eye -> Normal -> Reflected
	float3	f3Reflect = 2 * f3Normal * f1NdotV - f3EyeDir;

	// This also does Specular by outputting to f3Specular
	f3BumpedLight = LUX_DoFlashlightSpecular(f3WorldPos, f3Normal, iFilterMode, bDoShadows, f3ProjPos, f3Reflect, f1SpecularExponent, f1PhongFresnel, f3Specular);

	// f3PhongModulation is basically the Masks used for Phong.
	f3Specular *= f3PhongModulation * f1PhongBoost;

	// f3PhongModulation is basically the Masks used for Phong.
	f3Specular *= f3PhongModulation * f1PhongBoost;

	f3Specular *= f3PhongTint; // $PhongTint

	f3Specular += f3BaseTexture * f3BumpedLight;
	return f3Specular;
}
#endif // End of LUX_COMMON_PHONG_FLASHLIGHT