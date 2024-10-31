#ifndef FleshInterior_H
#define FleshInterior_H
#ifdef _WIN32
#pragma once
#endif

#include <string.h>

//void LuxFleshInterior_Link_Params(FleshInterior_Vars_t &info);
void LuxFleshInterior_Init_Params(CBaseVSShader *pShader, IMaterialVar **params, const char *pMaterialName, FleshInterior_Vars_t &info);
void LuxFleshInterior_Shader_Init(CBaseVSShader *pShader, IMaterialVar **params, FleshInterior_Vars_t &info);
void LuxFleshInterior_Shader_Draw(CBaseVSShader *pShader, IMaterialVar **params, IShaderDynamicAPI *pShaderAPI,
	IShaderShadow *pShaderShadow, FleshInterior_Vars_t &info, VertexCompressionType_t vertexCompression,
	CBasePerMaterialContextData **pContextDataPtr);

#define LuxFleshInterior_Params() void LuxFleshInterior_Link_Params(FleshInterior_Vars_t &info)\
{																								\
info.m_nFleshTexture = FLESHINTERIORTEXTURE;													 \
info.m_nFleshNoiseTexture = FLESHINTERIORNOISETEXTURE;											  \
info.m_nFleshBorderTexture1D = FLESHBORDERTEXTURE1D;											   \
info.m_nFleshNormalTexture = FLESHNORMALTEXTURE;												    \
info.m_nFleshSubsurfaceTexture = FLESHSUBSURFACETEXTURE;										 	 \
info.m_nFleshCubeTexture = FLESHCUBETEXTURE;													 	  \
info.m_nBorderNoiseScale = FLESHBORDERNOISESCALE;												 	   \
info.m_nDebugForceFleshOn = FLESHDEBUGFORCEFLESHON;											 			\
info.m_nEffectCenterRadius1 = FLESHEFFECTCENTERRADIUS1;													 \
info.m_nEffectCenterRadius2 = FLESHEFFECTCENTERRADIUS2;													  \
info.m_nEffectCenterRadius3 = FLESHEFFECTCENTERRADIUS3;													   \
info.m_nEffectCenterRadius4 = FLESHEFFECTCENTERRADIUS4;														\
info.m_nSubsurfaceTint = FLESHSUBSURFACETINT;																 \
info.m_nBorderWidth = FLESHBORDERWIDTH;																		  \
info.m_nBorderSoftness = FLESHBORDERSOFTNESS;																   \
info.m_nBorderTint = FLESHBORDERTINT;																			\
info.m_nGlobalOpacity = FLESHGLOBALOPACITY;																		 \
info.m_nGlossBrightness = FLESHGLOSSBRIGHTNESS;																	  \
info.m_nScrollSpeed = FLESHSCROLLSPEED;																			   \
info.m_nTime = TIME;																								\
}//------------------------------------------------------------------------------------------------------------------\

#endif