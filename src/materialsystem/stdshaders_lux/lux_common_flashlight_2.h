//
//========= ShiroDkxtro2's --ACROHS Ultimate Shaders Project-- ============//
//
//	Initial D.	:	24.01.2023 DMY
//	Last Change :	24.05.2023 DMY
//
//	Purpose of this File :	Shader Constant Register Declaration for
//							flashlight behaviour
//
//===========================================================================//

#ifndef LUX_COMMON_FLASHLIGHT_H_
#define LUX_COMMON_FLASHLIGHT_H_

//===========================================================================//
//	Declaring FLOAT PixelShader Constant Registers ( PSREG's )
//	There can be a maximum of 224 according to Microsoft Documentation
//	Ranging from 0-223. After this follows b0-b15 and i0 - i15
//===========================================================================//
const float4 g_ShadowTweaks					: register(c2); // Flashlight ShadowTweaks
const float4 g_FlashlightAttenuationFactors : register(c13);
const float4 g_FlashlightPos				: register(c14);
const float4x4 g_FlashlightWorldToTexture	: register(c15);
// 16--used by g_FlashlightWorldToTexture
// 17--used by g_FlashlightWorldToTexture
// 18--used by g_FlashlightWorldToTexture
const float4 cFlashlightColor				: register(c28); // common_fxc.h
const float4 cFlashlightScreenScale			: register(c31); // common_fxc.h - .zw are currently unused
#define f1FlashlightNoLambertValue			(cFlashlightColor.w) // This is either 0.0 or 2.0

#define NVIDIA_PCF_POISSON	0
#define ATI_NOPCF			1
#define ATI_NO_PCF_FETCH4	2

//===========================================================================//
//	Declaring Samplers. We only have 16 on SM3.0. Ranging from 0-15
//	So we have to reuse them depending on what shader we are on and what features we require...
//	Note : Local definitions might be different. We don't always need to have clear names.
//===========================================================================//
//#if FLASHLIGHT && !CASCADED_SHADOWS
sampler Sampler_ShadowDepth		: register(s13);
sampler Sampler_RandomRotation	: register(s14);
sampler Sampler_FlashlightCookie: register(s15);

float2 vPoissonOffset[8] = {	float2( 0.3475f,	0.0042f),
								float2( 0.8806f,	0.3430f),
								float2(-0.0041f,   -0.6197f),
								float2( 0.0472f,	0.4964f),
								float2(-0.3730f,	0.0874f),
								float2(-0.9217f,   -0.3177f),
								float2(-0.6289f,	0.7388f),
								float2( 0.5744f,   -0.7741f)	};

// Stock function.
float RemapValClamped(float val, float A, float B, float C, float D)
{
	float cVal = (val - A) / (B - A);
	cVal = saturate(cVal);

	return C + (D - C) * cVal;
}

//===========================================================================//
//	Stock SDK Shadow filters. We used to have a 5x5 gaussian set up,
//	However it turns out to only work on Nvidia so we are falling back to
//	The stock ones. No feature creep for better shadow filters on other cards!
//===========================================================================//
float FilterShadow(const int iFilterMode, float f1ObjectDepth, float3 RMatTop, float3 RMatBottom)
{
	// Prepare these..
	float f1Shadow = 0.0f;
	float4 f4LightDepths = 0.0f;
	float4 f4Accumulated = 0.0f;
	float2 f2RotationOffset = 0.0f;

	if (iFilterMode == NVIDIA_PCF_POISSON) // NVIDIA_PCF_POISSON
	{
		// ShiroDkxtro2 NOTE:	We need multiple shadowfilters because as it turns out, some of them don't work on different Hardware.
		//						I tried this one and the 5x5 Gaussian on my AMD RX570, and they do not work!
		//						Assumption : For whatever reason, my specific or all AMD Card(s) do not support the tex2Dproj instruction!
		f2RotationOffset.x = dot(RMatTop.xy, vPoissonOffset[0].xy) + RMatTop.z;
		f2RotationOffset.y = dot(RMatBottom.xy, vPoissonOffset[0].xy) + RMatBottom.z;
		f4LightDepths.x += tex2Dproj(Sampler_ShadowDepth, float4(f2RotationOffset, f1ObjectDepth, 1)).x;

		f2RotationOffset.x = dot(RMatTop.xy, vPoissonOffset[1].xy) + RMatTop.z;
		f2RotationOffset.y = dot(RMatBottom.xy, vPoissonOffset[1].xy) + RMatBottom.z;
		f4LightDepths.y += tex2Dproj(Sampler_ShadowDepth, float4(f2RotationOffset, f1ObjectDepth, 1)).x;

		f2RotationOffset.x = dot(RMatTop.xy, vPoissonOffset[2].xy) + RMatTop.z;
		f2RotationOffset.y = dot(RMatBottom.xy, vPoissonOffset[2].xy) + RMatBottom.z;
		f4LightDepths.z += tex2Dproj(Sampler_ShadowDepth, float4(f2RotationOffset, f1ObjectDepth, 1)).x;

		f2RotationOffset.x = dot(RMatTop.xy, vPoissonOffset[3].xy) + RMatTop.z;
		f2RotationOffset.y = dot(RMatBottom.xy, vPoissonOffset[3].xy) + RMatBottom.z;
		f4LightDepths.w += tex2Dproj(Sampler_ShadowDepth, float4(f2RotationOffset, f1ObjectDepth, 1)).x;

		f2RotationOffset.x = dot(RMatTop.xy, vPoissonOffset[4].xy) + RMatTop.z;
		f2RotationOffset.y = dot(RMatBottom.xy, vPoissonOffset[4].xy) + RMatBottom.z;
		f4LightDepths.x += tex2Dproj(Sampler_ShadowDepth, float4(f2RotationOffset, f1ObjectDepth, 1)).x;

		f2RotationOffset.x = dot(RMatTop.xy, vPoissonOffset[5].xy) + RMatTop.z;
		f2RotationOffset.y = dot(RMatBottom.xy, vPoissonOffset[5].xy) + RMatBottom.z;
		f4LightDepths.y += tex2Dproj(Sampler_ShadowDepth, float4(f2RotationOffset, f1ObjectDepth, 1)).x;

		f2RotationOffset.x = dot(RMatTop.xy, vPoissonOffset[6].xy) + RMatTop.z;
		f2RotationOffset.y = dot(RMatBottom.xy, vPoissonOffset[6].xy) + RMatBottom.z;
		f4LightDepths.z += tex2Dproj(Sampler_ShadowDepth, float4(f2RotationOffset, f1ObjectDepth, 1)).x;

		f2RotationOffset.x = dot(RMatTop.xy, vPoissonOffset[7].xy) + RMatTop.z;
		f2RotationOffset.y = dot(RMatBottom.xy, vPoissonOffset[7].xy) + RMatBottom.z;
		f4LightDepths.w += tex2Dproj(Sampler_ShadowDepth, float4(f2RotationOffset, f1ObjectDepth, 1)).x;

		f1Shadow = dot(f4LightDepths, float4(0.25, 0.25, 0.25, 0.25));
	}
	else if (iFilterMode == ATI_NO_PCF_FETCH4) // ATI_NO_PCF_FETCH4
	{
		// Original VALVE TODO :
		/*
		TODO: Fix this contact hardening stuff

		float flNumCloserSamples = 1;
		float flAccumulatedCloserSamples = f1ObjectDepth;
		float4 vBlockerDepths;

		// First, search for blockers
		for( int j=0; j<8; j++ )
		{
		f2RotationOffset.x = dot (RMatTop.xy,    vPoissonOffset[j].xy) + RMatTop.z;
		f2RotationOffset.y = dot (RMatBottom.xy, vPoissonOffset[j].xy) + RMatBottom.z;
		vBlockerDepths = tex2D( Sampler_ShadowDepth, f2RotationOffset.xy );

		// Which samples are closer than the pixel we're rendering?
		float4 vCloserSamples = (vBlockerDepths < f1ObjectDepth.xxxx );				// Binary comparison results
		flNumCloserSamples += dot( vCloserSamples, float4(1, 1, 1, 1) );		// How many samples are closer than receiver?
		flAccumulatedCloserSamples += dot (vCloserSamples, vBlockerDepths );	// Total depths from samples closer than receiver
		}

		float flBlockerDepth = flAccumulatedCloserSamples / flNumCloserSamples;
		float flContactHardeningScale = (f1ObjectDepth - flBlockerDepth) / flBlockerDepth;

		// Scale the kernel
		RMatTop.xy    *= flContactHardeningScale;
		RMatBottom.xy *= flContactHardeningScale;
		*/

		for (int i = 0; i<8; i++)
		{
			f2RotationOffset.x = dot(RMatTop.xy, vPoissonOffset[i].xy) + RMatTop.z;
			f2RotationOffset.y = dot(RMatBottom.xy, vPoissonOffset[i].xy) + RMatBottom.z;
			f4LightDepths = tex2D(Sampler_ShadowDepth, f2RotationOffset.xy);
			f4Accumulated += (f4LightDepths > f1ObjectDepth.xxxx);
		}

		f1Shadow = dot(f4Accumulated, float4(1.0f / 32.0f, 1.0f / 32.0f, 1.0f / 32.0f, 1.0f / 32.0f));
	}
	else if (iFilterMode == ATI_NOPCF) // ATI_NOPCF
	{
		for (int i = 0; i<2; i++)
		{
			f2RotationOffset.x = dot(RMatTop.xy, vPoissonOffset[4 * i + 0].xy) + RMatTop.z;
			f2RotationOffset.y = dot(RMatBottom.xy, vPoissonOffset[4 * i + 0].xy) + RMatBottom.z;
			f4LightDepths.x = tex2D(Sampler_ShadowDepth, f2RotationOffset.xy).x;

			f2RotationOffset.x = dot(RMatTop.xy, vPoissonOffset[4 * i + 1].xy) + RMatTop.z;
			f2RotationOffset.y = dot(RMatBottom.xy, vPoissonOffset[4 * i + 1].xy) + RMatBottom.z;
			f4LightDepths.y = tex2D(Sampler_ShadowDepth, f2RotationOffset.xy).x;

			f2RotationOffset.x = dot(RMatTop.xy, vPoissonOffset[4 * i + 2].xy) + RMatTop.z;
			f2RotationOffset.y = dot(RMatBottom.xy, vPoissonOffset[4 * i + 2].xy) + RMatBottom.z;
			f4LightDepths.z = tex2D(Sampler_ShadowDepth, f2RotationOffset.xy).x;

			f2RotationOffset.x = dot(RMatTop.xy, vPoissonOffset[4 * i + 3].xy) + RMatTop.z;
			f2RotationOffset.y = dot(RMatBottom.xy, vPoissonOffset[4 * i + 3].xy) + RMatBottom.z;
			f4LightDepths.w = tex2D(Sampler_ShadowDepth, f2RotationOffset.xy).x;

			f4Accumulated += (f4LightDepths > f1ObjectDepth.xxxx);
		}

		f1Shadow = dot(f4Accumulated, float4(0.125, 0.125, 0.125, 0.125));
	}

	return f1Shadow;
}


//===========================================================================//
//	New Flashlight Shadow Function
//===========================================================================//
float3 LUX_DoFlashlight(float3 f3WorldPosition, float3 f3FaceNormal, const int iFilterMode = 0,
						const bool bDoShadows = false, float3 f3ProjPos = float3(0.0f, 0.0f, 0.0f), const bool bHasNormalTexture = false)
{
	// Speeds up rendering by not doing anything if this is the case.
	// TODO: What the heck is in the .w
//	if (g_FlashlightPos.w < 0)
	{
//		return float3(0, 0, 0);
	}
// ShiroDkxtro2 :	For some reason commenting this fixed the issue where Decals outside the Flashlight area would start glowing in the dark.
//					Makes me wonder what's going on here. We removed the check above from the PBR shader because it would screw the flashlight entirely.
//					kinda odd...
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
			float f1ScaleOverMapSize =	g_ShadowTweaks.x * 2;		// Tweak parameters to shader
			float2 f2NoiseOffset	 =	g_ShadowTweaks.zw;

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

		// constant bool, compiler will treat it as a static again
		if (bHasNormalTexture)
		{
			f3DiffuseLighting *= saturate(dot(f3L.xyz, f3FaceNormal) + f1FlashlightNoLambertValue); // NoLambertValue is either 0 or 2
		}
		else
		{
			// This code makes no sense. What the heck? We cap (1.0f + 0 or 2) at 0-1 range???
			// I assume the comment is a lie and that in fact the flashlight no lambert value is *negative*
			// That way this code would at least do *anything*
			f3DiffuseLighting *= saturate(1.0f + f1FlashlightNoLambertValue);
		}
		
		f3DiffuseLighting *= f3FlashlightColor;
		f3DiffuseLighting *= f1EndFalloffFactor;

		return f3DiffuseLighting;
	}
}
//#endif // #if FLASHLIGHT


#endif // End of LUX_COMMON_PS_FXC_H_