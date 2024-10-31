//========= ShiroDkxtro2's --ACROHS Ultimate Shaders Project-- ============//
//
//	Initial D.	:	07.02.2023 DMY
//	Last Change :	24.05.2023 DMY
//
//	Purpose of this File :	Declare ConVars used on all shaders.
//							If a ConVar from here is redefined, make sure
//							yours is an extern!
//
//===========================================================================//

// File in which you can determine what to use.
// We do not use preprocessor definitions for this because this is honestly faster and easier to set up.
#include "../ACROHS_Defines.h"

// All Shaders use the same struct and sampler definitions. This way multipass CSM doesn't need to re-setup the struct
#include "../ACROHS_Shared.h"
#include "CommonFunctions.h"

// The 'version' we track here is the amount of commits on the master LUX repo. .66 is 66 commits, which is the commit this was added.
ConVar lux_version("lux_version", "0.73"); // Commits + 1 actually. ( Staying ahead of the current commit count )

ConVar mat_fullbright("mat_fullbright", "0", FCVAR_CHEAT);
ConVar mat_printvalues("mat_printvalues", "0", FCVAR_CHEAT);
ConVar mat_oldshaders("mat_oldshaders", "0", FCVAR_RELOAD_MATERIALS);
ConVar mat_specular("mat_specular", "1", 0);

ConVar mat_nc("mat_nc", "", 0);
ConVar mat_nclf("mat_nclf", "0.0", 0);
ConVar mat_cc("mat_cc", "", 0);

#ifdef DEBUG_LUXELS
ConVar mat_luxels("mat_luxels", "0", FCVAR_CHEAT);
#endif

#ifdef LIGHTWARP
ConVar mat_disable_lightwarp("mat_disable_lightwarp", "0");
#endif

#ifdef MAPBASE_FEATURED
ConVar mat_specular_disable_on_missing("mat_specular_disable_on_missing", "0");
#endif

#ifdef BRUSH_PHONG
ConVar mat_enable_lightmapped_phong("mat_enable_lightmapped_phong", "1", FCVAR_ARCHIVE, "If 1, allow phong on world brushes. If 0, disallow. mat_force_lightmapped_phong does not work if this value is 0.");
ConVar mat_force_lightmapped_phong("mat_force_lightmapped_phong", "0", FCVAR_CHEAT, "Forces the use of phong on all LightmappedAdv textures, regardless of setting in VMT.");
ConVar mat_force_lightmapped_phong_boost("mat_force_lightmapped_phong_boost", "5.0", FCVAR_CHEAT);
ConVar mat_force_lightmapped_phong_exp("mat_force_lightmapped_phong_exp", "50.0", FCVAR_CHEAT);
#endif

ConVar r_rimlight("r_rimlight", "1", FCVAR_CHEAT);

#ifdef PHYSICALLY_BASED_RENDERING
ConVar mat_mrao("mat_mrao", "0", FCVAR_CHEAT);
#endif