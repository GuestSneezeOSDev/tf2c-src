//========= ShiroDkxtro2's --ACROHS Ultimate Shaders Project-- ============//
//
//	Description :	What is this?
//					This is a total rewrite of the SDK Shaders.
//					Under the prefix of lux_ shaders.
//					The goal is to make the most possible combinations work,
//					add new parameters and functionality and...
//					Most importantly.-Decrease Compile times. A lot.
//					For ACROHS specifically, we also add CSM support.
//					This nukes ps20b and below. No Linux support.
//
//	Initial D.	:	20.01.2023 DMY
//	Last Change :	14.02.2023 DMY
//
//	Purpose of this File :	Define what features should be used
//							Define If we use sdk2013 MP or SP
//							Define ACROHS specific features
//							Define Utility and Debug features/tools
//
//	How to use :	Uncomment things you don't want!
//					Disabling something will usually not require a recompile
//					You can still build the dll with it being deactivated.
//					Recompiling the shaders, will remove it from them though.
//===========================================================================//
//====		What SDK the shaders will be running on.				=========//
//====		NOTE: If you are NOT on MP, you are on SP!				=========//
//===========================================================================//

	//#define SDK2013MP

// Commenting this will automatically undefine all ACROHS specific Shader features.
#define ACROHS

//===========================================================================//
//====	Features that are present on ( but not limited to ) MapBase =========//
//====	NOTE: Features like Radial Fog require a Shader Recompile	=========//
//===========================================================================//
// NOTE:	This will automatically define features already present in Mapbase
//			Not having this will only disable it on the dll.
//			If you want to not compile the shaders with these features.
//			Comment everything else below.
//#define MAPBASE_FEATURED

// This requires a Shader Recompile!
#define RADIALFOG

// This will make Ropes use the Spline shader instead.
// Mapbase forces you to use Splineropes! If you don't have this on Mapbase,
// Ropes won't render at all.
#define SPLINEROPES

// Parallax Correction for Cubemaps
#define PARALLAXCORRECTEDCUBEMAPS

// Treesway support for model shaders
// NOTE: This define is not called TREESWAY because,
// the parameter name is the same and that.. confuses Visual Studio... :/?
// Anyways, Its TREESWAYING now! 
#define TREESWAYING

//===========================================================================//
//====	  Features to use, Undefine if you don't want a feature		=========//
//===========================================================================//

// Parallax Mapping using the red channel of the $ParallaxMap texture
// This is a mix between Parallax Interval 
// Define name is very specific, don't change it.
#define PARALLAX_MAPPING

// Specific to the PBR Shader!
#define DEFAULTMRAO

// The next 4 are for debugging/development builds and not release builds!
// Makes shader a few nanoseconds or so faster to compute on the dll.

// mat_fullbright 2 support
#define DEBUG_FULLBRIGHT2

// Display MRAO maps on PBR using the mat_mrao ConVar
#define DEBUG_MRAO

// Display Envmapmask, Phongmask, etc using the mat_texturemaps ConVar
#define DEBUG_TEXTUREVIEW

// only on MP, if you define this and use SP it will be undefine later.
// Displays debug Luxel texture for models and brushes.
#define DEBUG_LUXELS

// Only featured on MP, however will NOT be undefined! Manually usable on SP
// Allows models to use sources native system for lightmaps on models
#define MODEL_LIGHTMAPPING

// ShiroDkxtro2 :
// This was deprecated since 2007, according to the VDC anyways 
// However the PBR shader uses $emissiontexture
// And this is way cheaper, faster and less convoluted than $EmissiveBlend
// So I brought it back! Thank me later that you don't have to use $detailblendmode 5 for this...
// Btw its called SELFILLUMTEXTURING because if you use SELFILLUMTEXTURE, VS will get confused on param declarations? :/?
#define SELFILLUMTEXTURING

// Because you might not want this...
//#define BRUSH_PHONG

//===========================================================================//
//========= Oddly specific things you might want to change back	 ============//
//===========================================================================//

// ShiroDkxtro2 Note :	Using this will make the Shaders use the HDTV standard (Rec. 709)
//						For getting the average luminance of a texture's rgb values.
//						This is used for the greyscale image from $EnvMapSaturation and for $BaseMapLuminancePhongMask
//						When not defined it will use the NTSC Standard for Analog Television instead.
//						The first one is more modern and therefore I decided to enable this by default.
//						Rec. 709 Lum Coefficients are float3(0.2126f, 0.7152f, 0.0722f)
//						NTSC St. Lum Coefficients are float3(0.2990f, 0.5870f, 0.1140f)
//
// This requires a Shader Recompile!
#define HDTV_LUM_COEFFICIENTS

//===========================================================================//
//========= SPECIAL-ACROHS's CSM is closed source, this isn't!	 ============//
//===========================================================================//
//#define SPP_CSM

//===========================================================================//
//========= Specific to ACROHS, Undefine if you're NOT on ACROHS ============//
//===========================================================================//
//#define ACROHS_CSM

//	Make PBR Usable.
#define PHYSICALLY_BASED_RENDERING
//	This is for PBR: Allows the display of MRAO Channels using mat_mrao 1-3..
//	It can also display specular/glossiness... Idk why I named it that(??)
#define DEBUG_MRAO

//===========================================================================//
//========= Utility Disable -- Don't need that? Compile faster!! ============//
//===========================================================================//

// Have you ever seen someone use this in a mod?!
// Toon Shader when
#define LIGHTWARP

// No Flashlight?
#define PROJECTEDTEXTURES

// RIP DetailTextures?
#define DETAILTEXTURING

// You are Sellface?! mat_specular 0?? SPEED
// Note you must undefine PARALLAXCORRECTEDCUBEMAPS and CUBEMAPS_FRESNEL manually,
// To have them actually gone. ( And to save extra compile time )
#define CUBEMAPS

// The I don't like Phong #define
#define PHONG_REFLECTIONS

// My husband left me! Now I only want to compile shaders faster! UNCOMMENT HERE
#define CUBEMAPS_FRESNEL

// Tinting via base alpha? Just finalise in the texture or something. Faster compiles again!
#define BASETINTINGVIAALPHA

// Not using $halflambert anyways? Faster Compiletimes..
#define HALFLAMBERTIAN






























































//===========================================================================//
// EVERYTHING BELOW, MUST STAY BELOW LINE 200 - IMPORTANT FOR SHADER DEFINES //
//===========================================================================//


// These will be undefine because it will cause errors when trying to compile on SP. Explanation behind the undefine.
#ifndef SDK2013MP
#undef DEBUG_LUXELS // The BINDSTANDARDTEXTURE cannot bind a Standardtexture that doesn't exist on SP.
#endif

// Mapbase Needs this
#ifdef MAPBASE_FEATURED
#define SPLINEROPES
#define PARALLAXCORRECTEDCUBEMAPS
#define TREESWAYING
#endif

// Not on ACROHS? Sucks to be you!
#ifndef ACROHS
#undef ACROHS_CSM
#else
#undef SPP_CSM // Local implementation is superior. There is no reason to use this.
#endif

#if defined(SELFILLUMTEXTURING)

#endif

#ifndef CUBEMAPS
#undef CUBEMAPS_FRESNEL
#undef PARALLAXCORRECTEDCUBEMAPS
#endif

/*
	My todo list :
	
	LightMappedReflective Shader
	MonitorScreen shader from Mapbase
	
	ScreenSpaceGeneral/Engine_post_dx9 <--- Ask Blixibon what the ____ this is?
	
	FLESH ( interior pass )
	
	Put Emissive Scroll and Cloak Blendpass stuff somewhere...
*/