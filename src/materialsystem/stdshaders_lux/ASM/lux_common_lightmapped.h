//
//========= ShiroDkxtro2's --ACROHS Ultimate Shaders Project-- ============//
//
//	Initial D.	:	21.02.2023 DMY
//	Last Change :	24.05.2023 DMY
//
//	Purpose of this File :	-Lightmapping related Constant Register Declarations
//							-Lightmap Sampler Declarations
//
//===========================================================================//

#ifndef LUX_COMMON_LIGHTMAPPED_H_
#define LUX_COMMON_LIGHTMAPPED_H_
//===========================================================================//
//	Declaring FLOAT PixelShader Constant Registers ( PSREG's )
//	There can be a maximum of 224 according to Microsoft Documentation
//	Ranging from 0-223. After this follows b0-b15 and i0 - i15
//===========================================================================//
// 0
// const float4 g_DiffuseModulation			: register(c1); // Colorspace
// const float4 cLightScale					: register(c30); // common_fxc.h

#define LIGHT_MAP_SCALE						(cLightScale.y)
#define f3Modul								(g_DiffuseModulation.xyz)

#if (LIGHTMAPPED_MODEL || defined(BRUSH)) // Not rendered in flashlight pass
sampler Sampler_LightMap		: register(s11);
#endif
#if defined(BRUSH)
sampler Sampler_LightWarpTexture : register(s6);

//===========================================================================//
// Straight up cleaned copy from common_lightmappedgeneric_fxc.h
//===========================================================================//
float3 LUX_LightMapSample(float2 vTexCoord)
{
	float3 sample = tex2D(Sampler_LightMap, vTexCoord);

	return sample;
}

//===========================================================================//
//	BumpBasis for Bumped Lightmaps
//===========================================================================//
#if defined(NORMALTEXTURE)
#define OO_SQRT_3 0.57735025882720947f
static const float3 LUX_BumpBasis[3] = {
	float3(0.81649661064147949f, 0.0f, OO_SQRT_3),
	float3(-0.40824833512306213f, 0.70710676908493042f, OO_SQRT_3),
	float3(-0.40824821591377258f, -0.7071068286895752f, OO_SQRT_3)
};
static const float3 LUX_BumpBasisTranspose[3] = {
	float3(0.81649661064147949f, -0.40824833512306213f, -0.40824833512306213f),
	float3(0.0f, 0.70710676908493042f, -0.7071068286895752f),
	float3(OO_SQRT_3, OO_SQRT_3, OO_SQRT_3)
};

//===========================================================================//
// Straight up cleaned copy from common_lightmappedgeneric_fxc.h
//===========================================================================//
void LUX_ComputeBumpedLightmapCoordinates(float4 f4Lightmap1and2Coord, float2 f2Lightmap3Coord, out float2 f2BumpCoord1, out float2 f2BumpCoord2, out float2 f2BumpCoord3)
{
	f2BumpCoord1 = f4Lightmap1and2Coord.xy;
	f2BumpCoord2 = f4Lightmap1and2Coord.wz; // reversed order!!!
	f2BumpCoord3 = f2Lightmap3Coord;
}

//===========================================================================//
// Mostly copied from Thexa4's PBR Shader. Slightly modified, but almost the same as Stock LMG anyways.
//===========================================================================//
float3 LUX_BumpedLightmap(float3 f3TextureNormal, float4 f4LightmapTexCoord1And2, float4 f4LightmapTexCoord3, int iDetailBlendMode = 0, float3 f3DetailTexture = float3(0.5f, 0.5f, 0.5f))
{
	float2 f2BumpCoord1;
	float2 f2BumpCoord2;
	float2 f2BumpCoord3;

	LUX_ComputeBumpedLightmapCoordinates( f4LightmapTexCoord1And2, f4LightmapTexCoord3.xy,	f2BumpCoord1, f2BumpCoord2, f2BumpCoord3);

	float3 f3LightmapColor1 = LUX_LightMapSample(f2BumpCoord1);
	float3 f3LightmapColor2 = LUX_LightMapSample(f2BumpCoord2);
	float3 f3LightmapColor3 = LUX_LightMapSample(f2BumpCoord3);

	float3 dp;
	dp.x = saturate(dot(f3TextureNormal, LUX_BumpBasis[0]));
	dp.y = saturate(dot(f3TextureNormal, LUX_BumpBasis[1]));
	dp.z = saturate(dot(f3TextureNormal, LUX_BumpBasis[2]));
	dp *= dp; // Saturate the result?

#if DETAILTEXTURE
	if(iDetailBlendMode == 10) // Modulate Lighting by Detailtexture, Blendmode = DETAIL_SSBUMP_BUMP
		dp *= 2.0f * f3DetailTexture;
#endif

	float3 f3DiffuseLighting =	dp.x * f3LightmapColor1 +
								dp.y * f3LightmapColor2 +
								dp.z * f3LightmapColor3;

	float sum = dot(dp, float3(1, 1, 1));
	f3DiffuseLighting *= f3Modul / sum;

	return f3DiffuseLighting;
}
#endif // NORMALTEXTURE

#if SSBUMP
//===========================================================================//
// Same as the above but returns SSBump related data.
//===========================================================================//
float3 LUX_SSBumpedLightmap(float3 f3SSBump, float4 f4LightmapTexCoord1And2, float4 f4LightmapTexCoord3, int iDetailBlendMode = 0, float3 f3DetailTexture = float3(0.5f, 0.5f, 0.5f))
{
	float2 f2BumpCoord1;
	float2 f2BumpCoord2;
	float2 f2BumpCoord3;

	LUX_ComputeBumpedLightmapCoordinates(f4LightmapTexCoord1And2, f4LightmapTexCoord3.xy, f2BumpCoord1, f2BumpCoord2, f2BumpCoord3);

	float3 f3LightmapColor1 = LUX_LightMapSample(f2BumpCoord1);
	float3 f3LightmapColor2 = LUX_LightMapSample(f2BumpCoord2);
	float3 f3LightmapColor3 = LUX_LightMapSample(f2BumpCoord3);
	
	// I have no idea what exactly p2-onward does here 
	// Referencing the original bumped lighting code ( and how it uses the ssbump ), this should make sense
	// In case of the Bumped Lightmap sampling it will multiply the sum of dp's with the detail texture
	// And this would be the same place just that we multiply the SSBump... by the SSBump...
	// If this comment is still here then it probably looked the same as the reference ( Portal 2 Panel Material )
#if DETAILTEXTURE
	if(iDetailBlendMode == 10)
		f3SSBump *= 2.0f * f3DetailTexture;
#endif

	float3 f3DiffuseLighting =	f3SSBump.x * f3LightmapColor1 +
								f3SSBump.y * f3LightmapColor2 +
								f3SSBump.z * f3LightmapColor3;
	f3DiffuseLighting *= f3Modul;
//	f3SSBump = normalize( bumpBasis[0]*vNormal.x + bumpBasis[1]*vNormal.y + bumpBasis[2]*vNormal.z )

	return f3DiffuseLighting;
}

#endif

#endif // BRUSH
#endif // End of LUX_COMMON_LIGHTMAPPED_H_