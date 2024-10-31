//========= ShiroDkxtro2's --ACROHS Ultimate Shaders Project-- ============//
//
//	Initial D.	:	20.01.2023 DMY
//	Last Change :	24.05.2023 DMY
//
//	Purpose of this File :	Define Macros commonly used by other shaders.
//							Actual Function Declarations in the .cpp!
//
//===========================================================================//

int ComputeEnvMapMode(bool EnvMap, bool EnvMapMask, bool BaseAlphaEnvMapMask, bool NormalMapAlphaEnvMapMask);
int ComputeEnvMapMode(bool EnvMap, bool EnvMapMask, bool BaseAlphaEnvMapMask);
int	CheckSeamless(bool bIsSeamless);

bool bStaticLightVertex(LightState_t LightState);

void SetHWMorphVertexShaderState(IShaderDynamicAPI *pShaderAPI, int nDimConst, int nSubrectConst, VertexTextureSampler_t morphSampler);

// Everything below is Macros
//===========================================================================//
// Helper Macro for flashlight bordercolor support
//===========================================================================//

#define Flashlight_BorderColorSupportCheck()\
if (g_pHardwareConfig->SupportsBorderColor())\
{\
	params[FLASHLIGHTTEXTURE]->SetStringValue("effects/flashlight_border");\
}\
else\
{\
	params[FLASHLIGHTTEXTURE]->SetStringValue("effects/flashlight001");\
}

//===========================================================================//
// Helper Macros for compacting the everexpanding amounts of If-Statements
//===========================================================================//

// Check if a parameter is defined
#define IsParamDefined(Var)\
(params[Var]->IsDefined())

// We set $Bumpmap to be $NormalTexture.
// The Engine checks $Bumpmap for a ton of things and to disable our features...
// Too bad We don't have Bumpmap defined by the time it checks c:
// This is required for Model Lightmapping on Bumped models
#define Bumpmap_to_NormalTexture(BumpVar, NormalVar)\
if(params[BumpVar]->IsDefined())\
{\
	params[NormalVar]->SetStringValue(params[BumpVar]->GetStringValue());\
	params[BumpVar]->SetUndefined();\
}

// The original helper function checks for != -1, this one doesn't.
#define FloatParameterDefault(FloatVar, DefaultValue)\
if(!params[FloatVar]->IsDefined())\
{\
	params[FloatVar]->SetFloatValue(DefaultValue);\
}

// The original helper function checks for != -1, this one doesn't.
#define IntParameterDefault(IntVar, DefaultValue)\
if(!params[IntVar]->IsDefined())\
{\
	params[IntVar]->SetIntValue(DefaultValue);\
}

// The original helper function checks for != -1, this one doesn't.
// Sets a Vec2 Var to a given value
#define Vec2ParameterDefault(Vec2Var, x, y)\
if(!params[Vec2Var]->IsDefined())\
{\
	params[Vec2Var]->SetVecValue( x, y);\
}


// The original helper function checks for != -1, this one doesn't.
// Sets a Vec3 Var to a given value
#define Vec3ParameterDefault(Vec3Var, x, y, z)\
if(!params[Vec3Var]->IsDefined())\
{\
	params[Vec3Var]->SetVecValue( x, y, z);\
}

// The original helper function checks for != -1, this one doesn't.
// Sets a Vec4 Var to a given value
#define Vec4ParameterDefault(Vec4Var, x, y, z, w)\
if(!params[Vec4Var]->IsDefined())\
{\
	params[Vec4Var]->SetVecValue( x, y, z, w);\
}

// Loads a texture var if its defined
#define LoadTextureWithCheck(var, flags)\
if (params[var]->IsDefined())\
{\
	pShader->LoadTexture(var, flags);\
}

// Load a Bumpmap, if defined, using the LoadBumpMap Function
#define LoadNormalTexture(var)\
if (g_pConfig->UseBumpmapping() && params[var]->IsDefined())\
{\
	pShader->LoadBumpMap(var);\
}

// Loads the Bumpmap and sets the BUMPED_LIGHTMAP flag
#define LoadNormalTextureLightmapFlag(var)\
if (g_pConfig->UseBumpmapping() && params[var]->IsDefined())\
{\
	pShader->LoadBumpMap(var);\
	SET_FLAGS2(MATERIAL_VAR2_LIGHTING_BUMPED_LIGHTMAP);\
}

// Loads a cubemap if var is defined.
#define LoadEnvMap(var, flags)\
if (params[var]->IsDefined())\
{\
	pShader->LoadCubeMap(var, flags);\
}

// Is the texture loaded?
// Put a ; if you don't have && at the end!
#define IsTextureLoaded(TextureVar)\
(params[TextureVar]->IsTexture())

//===========================================================================//
// Helper Macros for Getting values from parameters
//===========================================================================//

// Get Param as Bool
// ShiroDkxtro2 : note that values above 0 will automatically be interpreted as true
#define GetBoolParamValue(Var)\
params[Var]->GetIntValue() != 0

// Get Param as Integer
#define GetIntParamValue(Var)\
params[Var]->GetIntValue()

// Get Param as Float
#define GetFloatParamValue(Var)\
params[Var]->GetFloatValue()

// Get Two Values of a Vector Parameter
#define GetVec2ParamValue(Var, OutputVec)\
params[Var]->GetVecValue(OutputVec, 2)

// Get Three Values of a Vector Parameter
#define GetVec3ParamValue(Var, OutputVec)\
params[Var]->GetVecValue(OutputVec, 3)

// Get Four Values of a Vector Parameter
#define GetVec4ParamValue(Var, OutputVec)\
params[Var]->GetVecValue(OutputVec, 4)

//===========================================================================//
// Helper Macros for Setting values for parameters
//===========================================================================//

// Set Param as Bool
// Use SetIntParamValue(Var, 1) instead

// Set Param as Integer
#define SetIntParamValue(Var, Value)\
params[Var]->SetIntValue(Value);

// Set Param as Float
#define SetFloatParamValue(Var, Value)\
params[Var]->SetFloatValue(Value);

// Set Two Values of a Vector Parameter
#define SetVec2ParamValue(Var, OutputVec)\
params[Var]->SetVecValue(OutputVec, 2);

// Set Three Values of a Vector Parameter
#define SetVec3ParamValue(Var, OutputVec)\
params[Var]->SetVecValue(OutputVec, 3);

// Set Four Values of a Vector Parameter
#define SetVec4ParamValue(Var, OutputVec)\
params[Var]->SetVecValue(OutputVec, 4);

//===========================================================================//
// Helper Macros for enabling Samplers
//===========================================================================//
/*


this thing is evil
pShader->SetInitialShadowState();\
*/

// Enables Flashlightsamplers and set FogToBlack() so that fog doesn't become more bright under the flashlight.
#define EnableFlashlightSamplers(bHasFlashlight, SAMPLER_SHADOWDEPTH, SAMPLER_RANDOMROTATION, SAMPLER_COOKIE, Var)\
if (bHasFlashlight)\
{\
	pShaderShadow->EnableAlphaWrites(false);\
	pShaderShadow->EnableDepthWrites(false);\
	pShaderShadow->EnableTexture(SAMPLER_SHADOWDEPTH, true);\
	pShaderShadow->SetShadowDepthFiltering(SAMPLER_SHADOWDEPTH);\
	pShaderShadow->EnableSRGBRead(SAMPLER_SHADOWDEPTH, false);\
	pShaderShadow->EnableTexture(SAMPLER_RANDOMROTATION, true);\
	pShaderShadow->EnableTexture(SAMPLER_COOKIE, true);\
	pShaderShadow->EnableSRGBRead(SAMPLER_COOKIE, true);\
	pShader->SetAdditiveBlendingShadowState(Var, true);\
	pShader->FogToBlack();\
}\
else\
{\
	pShader->DefaultFog();\
	pShader->SetDefaultBlendingShadowState(Var, true);\
}

// Enable a Sampler and whether or not it should do sRGB read
#define EnableSampler(SamplerName, sRGB)\
{\
	pShaderShadow->EnableTexture(SamplerName, true);\
	pShaderShadow->EnableSRGBRead(SamplerName, sRGB);\
}

// Enable a Sampler. But first check if a condition is true. Also whether or not sRGB Read
#define EnableSamplerWithCheck(boolean, SamplerName, sRGB)\
if (boolean)\
{\
	pShaderShadow->EnableTexture(SamplerName, true);\
	pShaderShadow->EnableSRGBRead(SamplerName, sRGB);\
}

//===========================================================================//
// Helper Macros for setting up utility things like VertexFormat
//===========================================================================//

// We only need one texcoord, in the default float2 size
// We also need the position, surface normal
#define GetVertexShaderFormat_Brush()\
unsigned int flags = VERTEX_POSITION | VERTEX_NORMAL;\
pShaderShadow->VertexShaderVertexFormat(flags, 2, 0, 0);

// We only need one texcoord, in the default float2 size
// We also need the position, surface normal and Color
#define GetVertexShaderFormat_Displacement()\
unsigned int flags = VERTEX_POSITION | VERTEX_NORMAL | VERTEX_COLOR;\
pShaderShadow->VertexShaderVertexFormat(flags, 2, 0, 0);\

#define GetVertexShaderFormat_Unlit()\
unsigned int flags = VERTEX_POSITION | VERTEX_NORMAL | VERTEX_COLOR | VERTEX_FORMAT_COMPRESSED;\
int pTexCoordDim[3] = { 2, 0, 3 };\
pShaderShadow->VertexShaderVertexFormat(flags, 3, pTexCoordDim, 4);

// We need the position and surface normal, AND compression
// We also need three texcoords, all in the default float2 size
#define GetVertexShaderFormat_Model()\
unsigned int flags = VERTEX_POSITION | VERTEX_NORMAL | VERTEX_FORMAT_COMPRESSED;\
pShaderShadow->VertexShaderVertexFormat(flags, 1, 0, 0);

#define GetVertexShaderFormat_Decal()\
unsigned int flags = VERTEX_FORMAT_COMPRESSED | VERTEX_COLOR | VERTEX_POSITION;\
int pTexCoordDim[3] = { 2, 2, 3 };\
pShaderShadow->VertexShaderVertexFormat(flags, 3, pTexCoordDim, 0);

// Required for Morphing
#define GetVertexShaderFormat_ModelMorphed()\
unsigned int flags = VERTEX_POSITION | VERTEX_NORMAL | VERTEX_FORMAT_COMPRESSED;\
int pTexCoordDim[3] = { 2, 0, 3 };\
pShaderShadow->VertexShaderVertexFormat(flags, 3, pTexCoordDim, 4);

// We need whatever we need. Does not ask for Vertex_Color
#define GetVertexShaderFormat_Both(bIsModel);\
if (bIsModel)\
{\
	unsigned int flags = VERTEX_POSITION | VERTEX_NORMAL | VERTEX_FORMAT_COMPRESSED;\
	pShaderShadow->VertexShaderVertexFormat(flags, 1, 0, 0);\
}\
else\
{\
	unsigned int flags = VERTEX_POSITION | VERTEX_NORMAL;\
	pShaderShadow->VertexShaderVertexFormat(flags, 3, 0, 0);\
}

// Evaluate Blendrequirements
#define IsOpaque(BoolName, Var, Alphatested)\
BoolName = false;\
BlendType_t nBlendType = pShader->EvaluateBlendRequirements(Var, true);\
BoolName = (nBlendType != BT_BLENDADD) && (nBlendType != BT_BLEND) && !Alphatested;

//===========================================================================//
// Helper Macros for Binding Textures
//===========================================================================//

// Bind a texture. But first make sure it exists.
#define BindTextureWithCheck(bTexture, SAMPLER, TextureVar, FrameVar)\
if(bTexture)\
{\
	pShader->BindTexture(SAMPLER, TextureVar, FrameVar);\
}

// Bind a texture. No If-statement...
#define BindTextureWithoutCheck(SAMPLER, TextureVar, FrameVar)\
	pShader->BindTexture(SAMPLER, TextureVar, FrameVar);

// Bind a texture. Make sure it exists, otherwise bind a StandardTexture
#define BindTextureWithCheckAndFallback(bTexture, SAMPLER, TextureVar, FrameVar, StandardTexture)\
if(bTexture)\
{\
	pShader->BindTexture(SAMPLER, TextureVar, FrameVar);\
}\
else\
{\
	pShaderAPI->BindStandardTexture(SAMPLER, StandardTexture);\
}

// Bind a StandardTexture of choice.
#define BindTextureStandard(SAMPLER, StandardTexture)\
pShaderAPI->BindStandardTexture(SAMPLER, StandardTexture);

//===========================================================================//
// Helper Macros for setting up constant registers
//===========================================================================//

#define f4Empty(Name)\
float Name[4] = {1,1,1,1};

// ShiroDkxtro2 : This is the same as MapBases implementation for consistency. But I removed the for loops
// I'd rather avoid loops during runtime when there is a static alternative.
// Maybe the compiler optimises it away? I certainly wouldn't know.
// What I do know is that it can't unoptimise this.
#define SetUpPCC(bHasParallaxCorrection, VarEnvMapOrigin, VarObb1, VarObb2, VarObb3, cMatrix, cOrigin)	\
if (bHasParallaxCorrection)																				\
{																										\
	float EnvMapOrigin[4] = { 0, 0, 0, 0 };																\
	params[VarEnvMapOrigin]->GetVecValue(EnvMapOrigin, 3);												\
																										\
	float* Vecs[3];																						\
	Vecs[0] = const_cast<float*>(params[VarObb1]->GetVecValue());										\
	Vecs[1] = const_cast<float*>(params[VarObb2]->GetVecValue());										\
	Vecs[2] = const_cast<float*>(params[VarObb3]->GetVecValue());										\
	float Matrix[4][4];																					\
	Matrix[0][0] = Vecs[0][0];	Matrix[1][0] = Vecs[1][0];												\
	Matrix[2][0] = Vecs[2][0];	Matrix[0][1] = Vecs[0][1];												\
	Matrix[1][1] = Vecs[1][1];	Matrix[2][1] = Vecs[2][1];												\
	Matrix[0][2] = Vecs[0][2];	Matrix[1][2] = Vecs[1][2];												\
	Matrix[2][2] = Vecs[2][2];	Matrix[0][3] = Vecs[0][3];												\
	Matrix[1][3] = Vecs[1][3];	Matrix[2][3] = Vecs[2][3];												\
	Matrix[3][0] = Matrix[3][1] = Matrix[3][2] = 0;														\
	Matrix[3][3] = 1;																					\
	pShaderAPI->SetPixelShaderConstant(cMatrix, &Matrix[0][0], 4);										\
	pShaderAPI->SetPixelShaderConstant(cOrigin, EnvMapOrigin);											\
}//-----------------------------------------------------------------------------------------------------\

// Because this is ugly to have around all the time.
// We won't set any of the 0-31 constant registers
// So we can use them for what their names say they are...
// NOTE: This is everything that both models and brushes can use.
// for Ambient Cubes, see SetupAdditionalConstantRegisters()
// c1, c2, c11, c12, c13, c14, c15, c28
// TODO: FIXME: the current flashlight check here binds the cookie twice.
// I assume the second one is more correct as it uses the flashlightstate's texture instead of some crappy loaded one
// Thats probably how the changable flashlight texture thing works. Find out whats truly the case here...!
#define SetupStockConstantRegisters(bFlashlight, SAMPLER_COOKIE, SAMPLER_RANDOMROT, VarFlashlightTexture, VarFlashlightTextureFrame, bFlashlightShadows)\
float color[4] = { 1.0, 1.0, 1.0, 1.0 };																												\
pShader->ComputeModulationColor(color);																													\
float flLScale = pShaderAPI->GetLightMapScaleFactor();																									\
color[0] *= flLScale; color[1] *= flLScale; color[2] *= flLScale;																						\
pShaderAPI->SetPixelShaderConstant(PSREG_DIFFUSE_MODULATION, color, 1);																					\
pShaderAPI->SetPixelShaderFogParams(PSREG_FOG_PARAMS);																									\
float vEyePos_SpecExponent[4] = { 0, 0, 0, 4 };																											\
pShaderAPI->GetWorldSpaceCameraPosition(vEyePos_SpecExponent);																							\
pShaderAPI->SetPixelShaderConstant(PSREG_EYEPOS_SPEC_EXPONENT, vEyePos_SpecExponent, 1);																\
if (bFlashlight)																																		\
{																																						\
pShader->BindTexture(SAMPLER_COOKIE, VarFlashlightTexture, VarFlashlightTextureFrame);																	\
VMatrix worldToTexture2;																																\
ITexture* pFlashlightDepthTexture;																														\
FlashlightState_t state = pShaderAPI->GetFlashlightStateEx(worldToTexture2, &pFlashlightDepthTexture);													\
bFlashlightShadows = state.m_bEnableShadows && (pFlashlightDepthTexture != NULL);																		\
SetFlashLightColorFromState(state, pShaderAPI, PSREG_FLASHLIGHT_COLOR);																					\
if (pFlashlightDepthTexture && g_pConfig->ShadowDepthTexture() && state.m_bEnableShadows)																\
{																																						\
	pShader->BindTexture(SAMPLER_SHADOWDEPTH, pFlashlightDepthTexture, 0);																				\
	pShaderAPI->BindStandardTexture(SAMPLER_RANDOMROT, TEXTURE_SHADOW_NOISE_2D);																		\
}																																						\
	VMatrix worldToTexture;																																\
	float atten[4], pos[4], tweaks[4];																													\
	const FlashlightState_t& flashlightState = pShaderAPI->GetFlashlightState(worldToTexture);															\
	SetFlashLightColorFromState(flashlightState, pShaderAPI, PSREG_FLASHLIGHT_COLOR);																	\
	pShader->BindTexture(SAMPLER_COOKIE, flashlightState.m_pSpotlightTexture, flashlightState.m_nSpotlightTextureFrame); \
	atten[0] = flashlightState.m_fConstantAtten;																										\
	atten[1] = flashlightState.m_fLinearAtten;																											\
	atten[2] = flashlightState.m_fQuadraticAtten;																										\
	atten[3] = flashlightState.m_FarZ;																													\
	pShaderAPI->SetPixelShaderConstant(PSREG_FLASHLIGHT_ATTENUATION, atten, 1);																			\
	pos[0] = flashlightState.m_vecLightOrigin[0];																										\
	pos[1] = flashlightState.m_vecLightOrigin[1];																										\
	pos[2] = flashlightState.m_vecLightOrigin[2];																										\
	pShaderAPI->SetPixelShaderConstant(PSREG_FLASHLIGHT_POSITION_RIM_BOOST, pos, 1);																	\
	pShaderAPI->SetPixelShaderConstant(PSREG_FLASHLIGHT_TO_WORLD_TEXTURE, worldToTexture.Base(), 4);													\
	tweaks[0] = ShadowFilterFromState(flashlightState);																									\
	tweaks[1] = ShadowAttenFromState(flashlightState);																									\
	pShader->HashShadow2DJitter(flashlightState.m_flShadowJitterSeed, &tweaks[2], &tweaks[3]);															\
	pShaderAPI->SetPixelShaderConstant(PSREG_ENVMAP_TINT__SHADOW_TWEAKS, tweaks, 1);																	\
}//-----------------------------------------------------------------------------------------------------------------------------------------------------\

// c04, c20-25
#define SetupAdditionalConstantRegisters()\
pShaderAPI->SetPixelShaderStateAmbientLightCube(PSREG_AMBIENT_CUBE, !lightState.m_bAmbientLight);\
pShaderAPI->CommitPixelShaderLighting(PSREG_LIGHT_INFO_ARRAY);

// Macro for setting up additional Dynamic Combo Things FOR BRUSHES
#define SetupDynamicComboVariables(iFogIndex, bWriteDepthToAlpha, bWriteWaterFogToAlpha, bIsFullyOpaque)\
MaterialFogMode_t fogType = pShaderAPI->GetSceneFogMode();												\
iFogIndex = (fogType == MATERIAL_FOG_LINEAR_BELOW_FOG_Z) ? 1 : 0;										\
if (bIsFullyOpaque)																						\
{																										\
	bWriteDepthToAlpha = pShaderAPI->ShouldWriteDepthToDestAlpha();										\
	bWriteWaterFogToAlpha = (fogType == MATERIAL_FOG_LINEAR_BELOW_FOG_Z);								\
	AssertMsg(!(bWriteDepthToAlpha && bWriteWaterFogToAlpha),											\
		"Can't write two values to alpha at the same time.");											\
}//-----------------------------------------------------------------------------------------------------\

// Macro for setting up additional Dynamic Combo Things FOR MODELS
#define SetupDynamicComboVariablesModels()\
int numBones = pShaderAPI->GetCurrentNumBones();