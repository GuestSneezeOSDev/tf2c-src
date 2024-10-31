//
//========= ShiroDkxtro2's --ACROHS Ultimate Shaders Project-- ============//
//
//	Initial D.	:	21.02.2023 DMY
//	Last Change :	24.05.2023 DMY
//
//	Purpose of this File :	-SelfIllum Constant Register Declarations
//							-SelfIllum Sampler Declarations
//
//===========================================================================//

#ifndef LUX_COMMON_SELFILLUM_H_
#define LUX_COMMON_SELFILLUM_H_
#if !FLASHLIGHT && !CASCADED_SHADOWS // These are not rendered on the flashlight
//===========================================================================//
//	Constants you will usually find on all shaders. Geometry ones anyways
//	NOTE:	Only for SelfIllum
//===========================================================================//

#if (SELFILLUMMODE != 0)
const float4	g_SelfIllumTint_Factor			:	register(c34);
#define			f3SelfIllumTint					(g_SelfIllumTint_Factor.xyz)
#define			f1SelfIllumMaskFactor			(g_SelfIllumTint_Factor.w)
const float4	f4SelfIllumFresnel				:	register(c45);
#define			f1SelfIllumFresnelScale			(f4SelfIllumFresnel.x)
#define			f1SelfIllumFresnelBias			(f4SelfIllumFresnel.y)
#define			f1SelfIllumFresnelExponent		(f4SelfIllumFresnel.z)

#if defined(PHONG)
#define			bHasSelfIllumFresnel			Bools[12]
#endif
#endif // Selfillum

//===========================================================================//
//	Declaring Samplers. We only have 16 on SM3.0. Ranging from 0-15
//	So we have to reuse them depending on what shader we are on and what features we require...
//	Note : Local definitions might be different. We don't always need to have clear names.
//===========================================================================//

#if (SELFILLUMMODE == 1 || SELFILLUMMODE == 3) // $selfillummask or $selfillumtexture
sampler Sampler_SelfIllum		: register(s13);
#endif
#if defined(DISPLACEMENT) // && ((SELFILLUMMODE2 == 1) || (SELFILLUMMODE2 == 3))
sampler Sampler_SelfIllum2		: register(s15);
#endif
#endif // End !FLASHLIGHT && !CASCADED_SHADOWS

#endif // End of LUX_COMMON_SELFILLUM_H_