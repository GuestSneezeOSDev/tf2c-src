//====== Copyright © 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_shareddefs.h"
#include "KeyValues.h"
#include "takedamageinfo.h"
#include "tf_gamerules.h"
#include "utlbuffer.h"
#include "filesystem.h"

//-----------------------------------------------------------------------------
// Teams.
//-----------------------------------------------------------------------------
const char *g_aTeamNames_Localized[TF_TEAM_COUNT] =
{
	"#TF_Unassigned",
	"#TF_Spectators",
	"#TF_RedTeam_Name",
	"#TF_BlueTeam_Name",
	"#TF_GreenTeam_Name",
	"#TF_YellowTeam_Name"
};

const char *g_aTeamNames[TF_TEAM_COUNT] =
{
	"Unassigned",
	"Spectator",
	"Red",
	"Blue",
	"Green",
	"Yellow"
};

// These two arrays should always be used when constructing team specific names (e.g. particle names).
const char *g_aTeamNamesShort[TF_TEAM_COUNT] =
{
	"unassigned",
	"spectator",
	"red",
	"blu",
	"grn",
	"ylw",
};

const char *g_aTeamUpperNamesShort[TF_TEAM_COUNT] =
{
	"UNASSIGNED",
	"SPECTATOR",
	"RED",
	"BLU",
	"GRN",
	"YLW",
};

const char *g_aTeamLowerNames[TF_TEAM_COUNT] =
{
	"unassigned",
	"spectator",
	"red",
	"blue",
	"green",
	"yellow",
};

const char *g_aTeamUpperNames[TF_TEAM_COUNT] =
{
	"UNASSIGNED",
	"SPECTATOR",
	"RED",
	"BLUE",
	"GREEN",
	"YELLOW",
};

const char *g_aPlayerNamesGeneric[TF_TEAM_COUNT] =
{
	"#TF_Mercenary_Name",
	"#TF_Mercenary_Name",
	"#TF_RedMercenary_Name",
	"#TF_BlueMercenary_Name",
	"#TF_GreenMercenary_Name",
	"#TF_YellowMercenary_Name",
};

Vector g_aTeamParticleColors[TF_TEAM_COUNT] =
{
	Vector( 1, 1, 1 ),
	Vector( 1, 1, 1 ),
	Vector( 1.0f, 0.2f, 0.15f ),
	Vector( 0.2f, 0.4f, 1.0f ),
	Vector( 0.2f, 0.7f, 0.15f ),
	Vector( 1.0f, 0.7f, 0.2f ),
};

color32 g_aTeamColors[TF_TEAM_COUNT] = 
{
	{ 0, 0, 0, 0 },		// Unassigned
	{ 0, 0, 0, 0 },		// Spectator
	{ 255, 0, 0, 0 },	// Red
	{ 0, 0, 255, 0 },	// Blue
	{ 0, 255, 0, 0 },	// Green
	{ 255, 255, 0, 0 }	// Yellow
};

//-----------------------------------------------------------------------------
// Classes.
//-----------------------------------------------------------------------------

const char *g_aPlayerClassNames[TF_CLASS_COUNT_ALL] =
{
	"#TF_Class_Name_Undefined",
	"#TF_Class_Name_Scout",
	"#TF_Class_Name_Sniper",
	"#TF_Class_Name_Soldier",
	"#TF_Class_Name_Demoman",
	"#TF_Class_Name_Medic",
	"#TF_Class_Name_HWGuy",
	"#TF_Class_Name_Pyro",
	"#TF_Class_Name_Spy",
	"#TF_Class_Name_Engineer",
	"#TF_Class_Name_Civilian",
};

const char *g_aPlayerClassEmblems[TF_CLASS_COUNT_ALL + ( TF_CLASS_COUNT_ALL - 1 )] =
{
	// Alive
	"../hud/leaderboard_class_scout",
	"../hud/leaderboard_class_sniper",
	"../hud/leaderboard_class_soldier",
	"../hud/leaderboard_class_demo",
	"../hud/leaderboard_class_medic",
	"../hud/leaderboard_class_heavy",
	"../hud/leaderboard_class_pyro",
	"../hud/leaderboard_class_spy",
	"../hud/leaderboard_class_engineer",
	"../hud/leaderboard_class_civilian",

	// Dead
	"../hud/leaderboard_class_scout_d",
	"../hud/leaderboard_class_sniper_d",
	"../hud/leaderboard_class_soldier_d",
	"../hud/leaderboard_class_demo_d",
	"../hud/leaderboard_class_medic_d",
	"../hud/leaderboard_class_heavy_d",
	"../hud/leaderboard_class_pyro_d",
	"../hud/leaderboard_class_spy_d",
	"../hud/leaderboard_class_engineer_d",
	"../hud/leaderboard_class_civilian_d",
};

const char *g_aPlayerClassEmblemsAlt[TF_CLASS_COUNT_ALL + ( TF_CLASS_COUNT_ALL - 1 )] =
{
	// Alive
	"class_icons/class_icon_orange_scout",
	"class_icons/class_icon_orange_sniper",
	"class_icons/class_icon_orange_soldier",
	"class_icons/class_icon_orange_demo",
	"class_icons/class_icon_orange_medic",
	"class_icons/class_icon_orange_heavy",
	"class_icons/class_icon_orange_pyro",
	"class_icons/class_icon_orange_spy",
	"class_icons/class_icon_orange_engineer",
	"class_icons/class_icon_orange_civilian",

	// Dead
	"class_icons/class_icon_orange_scout_d",
	"class_icons/class_icon_orange_sniper_d",
	"class_icons/class_icon_orange_soldier_d",
	"class_icons/class_icon_orange_demo_d",
	"class_icons/class_icon_orange_medic_d",
	"class_icons/class_icon_orange_heavy_d",
	"class_icons/class_icon_orange_pyro_d",
	"class_icons/class_icon_orange_spy_d",
	"class_icons/class_icon_orange_engineer_d",
	"class_icons/class_icon_orange_civilian_d",
};

const char *g_aPlayerClassNames_NonLocalized[TF_CLASS_COUNT_ALL] =
{
	"Undefined",
	"Scout",
	"Sniper",
	"Soldier",
	"Demoman",
	"Medic",
	"Heavy",
	"Pyro",
	"Spy",
	"Engineer",
	"Civilian",
};

const char *g_aRawPlayerClassNamesShort[TF_CLASS_COUNT_ALL] =
{
	"undefined",
	"scout",
	"sniper",
	"soldier",
	"demo",
	"medic",
	"heavy",
	"pyro",
	"spy",
	"engineer",
	"civilian",
};

const char *g_aRawPlayerClassNames[TF_CLASS_COUNT_ALL] =
{
	"undefined",
	"scout",
	"sniper",
	"soldier",
	"demoman",
	"medic",
	"heavyweapons",
	"pyro",
	"spy",
	"engineer",
	"civilian",
};

//-----------------------------------------------------------------------------
// Gametypes.
//-----------------------------------------------------------------------------
const char *g_aGameTypeNames[TF_GAMETYPE_COUNT] =
{
	"Undefined",
	"#Gametype_CTF",
	"#Gametype_CP",
	"#Gametype_Escort",
	"#Gametype_Arena",
	"#Gametype_RobotDestruction",
	"#GameType_Passtime",
	"#GameType_PlayerDestruction",
	"#Gametype_MVM",
	"#Gametype_VIP",
};

//-----------------------------------------------------------------------------
// Weapon Types
//-----------------------------------------------------------------------------
const char *g_AnimSlots[] = // correlates to ETFWeaponType
{
	"PRIMARY",
	"SECONDARY",
	"MELEE",
	"GRENADE",
	"BUILDING",
	"PDA",
	"ITEM1",
	"ITEM2",
	"HEAD",
	"MISC",
	"MELEE_ALLCLASS",
	"SECONDARY2",
	"PRIMARY2"
};

const char *g_LoadoutSlots[] = // correlates to ETFLoadoutSlot
{
	"PRIMARY",
	"SECONDARY",
	"MELEE",
	"UTILITY",
	"BUILDING",
	"PDA",
	"PDA2",
	"HEAD",
	"MISC",
	"ACTION",
	"TAUNT"
};

//-----------------------------------------------------------------------------
// Ammo.
//-----------------------------------------------------------------------------
const char *g_aAmmoNames[] =
{
	"DUMMY AMMO",
	"TF_AMMO_PRIMARY",
	"TF_AMMO_SECONDARY",
	"TF_AMMO_METAL",
	"TF_AMMO_GRENADES1",
	"TF_AMMO_GRENADES2"
};

struct pszWpnEntTranslationListEntry
{
	const char *weapon_name;
	const char *padding;
	const char *weapon_scout;
	const char *weapon_sniper;
	const char *weapon_soldier;
	const char *weapon_demoman;
	const char *weapon_medic;
	const char *weapon_heavyweapons;
	const char *weapon_pyro;
	const char *weapon_spy;
	const char *weapon_engineer;
	const char *weapon_civilian;
};

static pszWpnEntTranslationListEntry pszWpnEntTranslationList[] =
{
	{
		"tf_weapon_shotgun",				// Base weapon to translate
		NULL,
		"tf_weapon_shotgun_primary",		// Scout
		"tf_weapon_shotgun_primary",		// Sniper
		"tf_weapon_shotgun_soldier",		// Soldier
		"tf_weapon_shotgun_primary",		// Demoman
		"tf_weapon_shotgun_primary",		// Medic
		"tf_weapon_shotgun_hwg",			// Heavy
		"tf_weapon_shotgun_pyro",			// Pyro
		"tf_weapon_shotgun_primary",		// Spy
		"tf_weapon_shotgun_primary",		// Engineer
		"tf_weapon_shotgun_primary",		// Civilian
	},
	{
		"tf_weapon_pistol",					// Base weapon to translate
		NULL,
		"tf_weapon_pistol_scout",			// Scout
		"tf_weapon_pistol",					// Sniper
		"tf_weapon_pistol",					// Soldier
		"tf_weapon_pistol",					// Demoman
		"tf_weapon_pistol",					// Medic
		"tf_weapon_pistol",					// Heavy
		"tf_weapon_pistol",					// Pyro
		"tf_weapon_pistol",					// Spy
		"tf_weapon_pistol",					// Engineer
		"tf_weapon_pistol",					// Civilian
	},
	{
		"tf_weapon_shovel",					// Base weapon to translate
		NULL,
		"tf_weapon_shovel",					// Scout
		"tf_weapon_shovel",					// Sniper
		"tf_weapon_shovel",					// Soldier
		"tf_weapon_bottle",					// Demoman
		"tf_weapon_shovel",					// Medic
		"tf_weapon_shovel",					// Heavy
		"tf_weapon_shovel",					// Pyro
		"tf_weapon_shovel",					// Spy
		"tf_weapon_shovel",					// Engineer
		"tf_weapon_shovel",					// Civilian
	},
	{
		"tf_weapon_bottle",					// Base weapon to translate
		NULL,
		"tf_weapon_bottle",					// Scout
		"tf_weapon_bottle",					// Sniper
		"tf_weapon_shovel",					// Soldier
		"tf_weapon_bottle",					// Demoman
		"tf_weapon_bottle",					// Medic
		"tf_weapon_bottle",					// Heavy
		"tf_weapon_bottle",					// Pyro
		"tf_weapon_bottle",					// Spy
		"tf_weapon_bottle",					// Engineer
		"tf_weapon_bottle",					// Civilian
	},
	{
		"saxxy",							// Base weapon to translate
		NULL,
		"tf_weapon_bat",					// Scout
		"tf_weapon_club",					// Sniper
		"tf_weapon_shovel",					// Soldier
		"tf_weapon_bottle",					// Demoman
		"tf_weapon_bonesaw",				// Medic
		"tf_weapon_fireaxe",				// Heavy
		"tf_weapon_fireaxe",				// Pyro
		"tf_weapon_knife",					// Spy
		"tf_weapon_wrench",					// Engineer
		"tf_weapon_umbrella",				// Civilian
	},
	{
		"tf_weapon_throwable",				// Base weapon to translate
		NULL,
		"tf_weapon_throwable",				// Scout
		"tf_weapon_throwable",				// Sniper
		"tf_weapon_throwable",				// Soldier
		"tf_weapon_throwable",				// Demoman
		"tf_weapon_throwable",				// Medic
		"tf_weapon_throwable",				// Heavy
		"tf_weapon_throwable",				// Pyro
		"tf_weapon_throwable",				// Spy
		"tf_weapon_throwable",				// Engineer
		"tf_weapon_throwable",				// Civilian
	},
	{
		"tf_weapon_parachute",				// Base weapon to translate
		NULL,
		"tf_weapon_parachute_secondary",	// Scout
		"tf_weapon_parachute_secondary",	// Sniper
		"tf_weapon_parachute_primary",		// Soldier
		"tf_weapon_parachute_secondary",	// Demoman
		"tf_weapon_parachute_secondary",	// Medic
		"tf_weapon_parachute_secondary",	// Heavy
		"tf_weapon_parachute_secondary",	// Pyro
		"tf_weapon_parachute_secondary",	// Spy
		0,									// Engineer
		"tf_weapon_parachute_secondary",	// Civilian
	},
	{
		"tf_weapon_revolver",				// Base weapon to translate
		NULL,
		"tf_weapon_revolver",				// Scout
		"tf_weapon_revolver",				// Sniper
		"tf_weapon_revolver",				// Soldier
		"tf_weapon_revolver",				// Demoman
		"tf_weapon_revolver",				// Medic
		"tf_weapon_revolver",				// Heavy
		"tf_weapon_revolver",				// Pyro
		"tf_weapon_revolver",				// Spy
		"tf_weapon_revolver_secondary",		// Engineer
		"tf_weapon_revolver",				// Civilian
	}
};

//-----------------------------------------------------------------------------
// Weapons.
//-----------------------------------------------------------------------------
const char *g_aWeaponNames[] =
{
	"TF_WEAPON_NONE",
	"TF_WEAPON_BAT",
	"TF_WEAPON_BOTTLE", 
	"TF_WEAPON_FIREAXE",
	"TF_WEAPON_CLUB",
	"TF_WEAPON_CROWBAR",
	"TF_WEAPON_KNIFE",
	"TF_WEAPON_FISTS",
	"TF_WEAPON_SHOVEL",
	"TF_WEAPON_WRENCH",
	"TF_WEAPON_BONESAW",
	"TF_WEAPON_SHOTGUN_PRIMARY",
	"TF_WEAPON_SHOTGUN_SOLDIER",
	"TF_WEAPON_SHOTGUN_HWG",
	"TF_WEAPON_SHOTGUN_PYRO",
	"TF_WEAPON_SCATTERGUN",
	"TF_WEAPON_SNIPERRIFLE",
	"TF_WEAPON_MINIGUN",
	"TF_WEAPON_SMG",
	"TF_WEAPON_SYRINGEGUN_MEDIC",
	"TF_WEAPON_TRANQ",
	"TF_WEAPON_ROCKETLAUNCHER",
	"TF_WEAPON_GRENADELAUNCHER",
	"TF_WEAPON_PIPEBOMBLAUNCHER",
	"TF_WEAPON_FLAMETHROWER",
	"TF_WEAPON_GRENADE_NORMAL",
	"TF_WEAPON_GRENADE_CONCUSSION",
	"TF_WEAPON_GRENADE_NAIL",
	"TF_WEAPON_GRENADE_MIRV",
	"TF_WEAPON_GRENADE_MIRV_DEMOMAN",
	"TF_WEAPON_GRENADE_NAPALM",
	"TF_WEAPON_GRENADE_GAS",
	"TF_WEAPON_GRENADE_EMP",
	"TF_WEAPON_GRENADE_CALTROP",
	"TF_WEAPON_GRENADE_PIPEBOMB",
	"TF_WEAPON_GRENADE_SMOKE_BOMB",
	"TF_WEAPON_GRENADE_HEAL",
	"TF_WEAPON_PISTOL",
	"TF_WEAPON_PISTOL_SCOUT",
	"TF_WEAPON_REVOLVER",
	"TF_WEAPON_NAILGUN",
	"TF_WEAPON_PDA",
	"TF_WEAPON_PDA_ENGINEER_BUILD",
	"TF_WEAPON_PDA_ENGINEER_DESTROY",
	"TF_WEAPON_PDA_SPY",
	"TF_WEAPON_BUILDER",
	"TF_WEAPON_MEDIGUN",
	"TF_WEAPON_GRENADE_MIRVBOMB",
	"TF_WEAPON_FLAMETHROWER_ROCKET",
	"TF_WEAPON_GRENADE_DEMOMAN",
	"TF_WEAPON_SENTRY_BULLET",
	"TF_WEAPON_SENTRY_ROCKET",
	"TF_WEAPON_DISPENSER",
	"TF_WEAPON_INVIS",
	"TF_WEAPON_FLAG",
	"TF_WEAPON_FLAREGUN",
	"TF_WEAPON_LUNCHBOX",
	"TF_WEAPON_COMPOUND_BOW",
	"TF_WEAPON_SHOTGUN_BUILDING_RESCUE",

	// Add new weapons after this.
	"TF_WEAPON_HUNTERRIFLE",
	"TF_WEAPON_COILGUN",
	"TF_WEAPON_UMBRELLA",
	"TF_WEAPON_CUBEMAP",
	"TF_WEAPON_TASER",
	"TF_WEAPON_AAGUN",
	"TF_WEAPON_RUSSIANROULETTE",
	"TF_WEAPON_BEACON",
	"TF_WEAPON_DOUBLESHOTGUN",
	"TF_WEAPON_THROWABLE_BRICK",
	"TF_WEAPON_GRENADE_MIRV2",
	"TF_WEAPON_DETONATOR",
	"TF_WEAPON_THROWINGKNIFE",
	"TF_WEAPON_RIOT_SHIELD",
	"TF_WEAPON_PAINTBALLRIFLE",
	"TF_WEAPON_BRIMSTONELAUNCHER",
#ifdef TF2C_BETA
	"TF_WEAPON_PILLSTREAK",
	"TF_WEAPON_HEALLAUNCHER",
	"TF_WEAPON_ANCHOR",
	"TF_WEAPON_CYCLOPS",
	"TF_WEAPON_LEAPKNIFE",
#endif

	// End marker, do not add below here.
	"TF_WEAPON_COUNT",
};

int g_aWeaponDamageTypes[] =
{
	DMG_GENERIC,																				// TF_WEAPON_NONE
	DMG_CLUB,																					// TF_WEAPON_BAT,
	DMG_CLUB,																					// TF_WEAPON_BOTTLE, 
	DMG_SLASH,																					// TF_WEAPON_FIREAXE,
	DMG_SLASH,																					// TF_WEAPON_CLUB,
	DMG_CLUB,																					// TF_WEAPON_CROWBAR,
	DMG_SLASH,																					// TF_WEAPON_KNIFE,
	DMG_CLUB,																					// TF_WEAPON_FISTS,
	DMG_CLUB,																					// TF_WEAPON_SHOVEL,
	DMG_CLUB,																					// TF_WEAPON_WRENCH,
	DMG_SLASH,																					// TF_WEAPON_BONESAW,
	DMG_BULLET | DMG_BUCKSHOT | DMG_USEDISTANCEMOD,												// TF_WEAPON_SHOTGUN_PRIMARY,
	DMG_BULLET | DMG_BUCKSHOT | DMG_USEDISTANCEMOD,												// TF_WEAPON_SHOTGUN_SOLDIER,
	DMG_BULLET | DMG_BUCKSHOT | DMG_USEDISTANCEMOD,												// TF_WEAPON_SHOTGUN_HWG,
	DMG_BULLET | DMG_BUCKSHOT | DMG_USEDISTANCEMOD,												// TF_WEAPON_SHOTGUN_PYRO,
	DMG_BULLET | DMG_BUCKSHOT | DMG_USEDISTANCEMOD,												// TF_WEAPON_SCATTERGUN,
	DMG_BULLET | DMG_USE_HITLOCATIONS,															// TF_WEAPON_SNIPERRIFLE,
	DMG_BULLET | DMG_USEDISTANCEMOD,															// TF_WEAPON_MINIGUN,
	DMG_BULLET | DMG_USEDISTANCEMOD,															// TF_WEAPON_SMG,
	DMG_BULLET | DMG_USEDISTANCEMOD | DMG_NOCLOSEDISTANCEMOD /*| DMG_PREVENT_PHYSICS_FORCE*/,	// TF_WEAPON_SYRINGEGUN_MEDIC,
	DMG_BULLET | DMG_PREVENT_PHYSICS_FORCE,														// TF_WEAPON_TRANQ,
	DMG_BLAST | DMG_HALF_FALLOFF | DMG_USEDISTANCEMOD,											// TF_WEAPON_ROCKETLAUNCHER,
	DMG_BLAST | DMG_HALF_FALLOFF | DMG_USEDISTANCEMOD,											// TF_WEAPON_GRENADELAUNCHER,
	DMG_BLAST | DMG_HALF_FALLOFF | DMG_NOCLOSEDISTANCEMOD,										// TF_WEAPON_PIPEBOMBLAUNCHER,
	DMG_IGNITE | DMG_PREVENT_PHYSICS_FORCE,														// TF_WEAPON_FLAMETHROWER,
	DMG_BLAST | DMG_HALF_FALLOFF,																// TF_WEAPON_GRENADE_NORMAL,
	DMG_SONIC | DMG_HALF_FALLOFF,																// TF_WEAPON_GRENADE_CONCUSSION,
	DMG_BULLET | DMG_HALF_FALLOFF,																// TF_WEAPON_GRENADE_NAIL,
	DMG_BLAST | DMG_HALF_FALLOFF,																// TF_WEAPON_GRENADE_MIRV,
	DMG_BLAST | DMG_HALF_FALLOFF,																// TF_WEAPON_GRENADE_MIRV_DEMOMAN,
	DMG_BURN | DMG_RADIUS_MAX,																	// TF_WEAPON_GRENADE_NAPALM,
	DMG_POISON | DMG_HALF_FALLOFF,																// TF_WEAPON_GRENADE_GAS,
	DMG_BLAST | DMG_HALF_FALLOFF | DMG_PREVENT_PHYSICS_FORCE,									// TF_WEAPON_GRENADE_EMP,
	DMG_GENERIC,																				// TF_WEAPON_GRENADE_CALTROP,
	DMG_BLAST | DMG_HALF_FALLOFF | DMG_NOCLOSEDISTANCEMOD,										// TF_WEAPON_GRENADE_PIPEBOMB,
	DMG_GENERIC,																				// TF_WEAPON_GRENADE_SMOKE_BOMB,
	DMG_GENERIC,																				// TF_WEAPON_GRENADE_HEAL
	DMG_BULLET | DMG_USEDISTANCEMOD,															// TF_WEAPON_PISTOL,
	DMG_BULLET | DMG_USEDISTANCEMOD,															// TF_WEAPON_PISTOL_SCOUT,
	DMG_BULLET | DMG_USEDISTANCEMOD,															// TF_WEAPON_REVOLVER,
	DMG_BULLET | DMG_USEDISTANCEMOD | DMG_PREVENT_PHYSICS_FORCE,								// TF_WEAPON_NAILGUN,
	DMG_BULLET,																					// TF_WEAPON_PDA,
	DMG_BULLET,																					// TF_WEAPON_PDA_ENGINEER_BUILD,
	DMG_BULLET,																					// TF_WEAPON_PDA_ENGINEER_DESTROY,
	DMG_BULLET,																					// TF_WEAPON_PDA_SPY,
	DMG_BULLET,																					// TF_WEAPON_BUILDER
	DMG_BULLET,																					// TF_WEAPON_MEDIGUN
	DMG_BLAST | DMG_HALF_FALLOFF,																// TF_WEAPON_GRENADE_MIRVBOMB
	DMG_BLAST | DMG_IGNITE | DMG_RADIUS_MAX,													// TF_WEAPON_FLAMETHROWER_ROCKET
	DMG_BLAST | DMG_HALF_FALLOFF,																// TF_WEAPON_GRENADE_DEMOMAN
	DMG_GENERIC,																				// TF_WEAPON_SENTRY_BULLET
	DMG_GENERIC | DMG_USEDISTANCEMOD,															// TF_WEAPON_SENTRY_ROCKET
	DMG_GENERIC,																				// TF_WEAPON_DISPENSER
	DMG_GENERIC,																				// TF_WEAPON_INVIS
	DMG_CLUB,																					// TF_WEAPON_FLAG
	DMG_IGNITE,																					// TF_WEAPON_FLAREGUN,
	DMG_GENERIC,																				// TF_WEAPON_LUNCHBOX,
	DMG_BULLET | DMG_USE_HITLOCATIONS,															// TF_WEAPON_COMPOUND_BOW
	DMG_BULLET | DMG_USEDISTANCEMOD,															// TF_WEAPON_SHOTGUN_BUILDING_RESCUE

	// Add new weapons here.
	DMG_BULLET | DMG_USE_HITLOCATIONS | DMG_USEDISTANCEMOD,										// TF_WEAPON_HUNTERRIFLE
	DMG_BULLET,																					// TF_WEAPON_COILGUN
	DMG_CLUB,																					// TF_WEAPON_UMBRELLA
	DMG_GENERIC,																				// TF_WEAPON_CUBEMAP
	DMG_CLUB,																					// TF_WEAPON_TASER
	DMG_BLAST | DMG_HALF_FALLOFF | DMG_USEDISTANCEMOD,											// TF_WEAPON_AAGUN
	DMG_CLUB,																					// TF_WEAPON_RUSSIANROULETTE
	DMG_SLASH,																					// TF_WEAPON_BEACON
	DMG_BULLET | DMG_BUCKSHOT | DMG_USEDISTANCEMOD,												// TF_WEAPON_DOUBLESHOTGUN
	DMG_CLUB,																					// TF_WEAPON_THROWABLE_BRICK
	DMG_BLAST | DMG_HALF_FALLOFF,																// TF_WEAPON_GRENADE_MIRV2
	DMG_BLAST | DMG_HALF_FALLOFF,																// TF_WEAPON_DETONATOR
	DMG_BULLET,																					// TF_WEAPON_THROWINGKNIFE
	DMG_CLUB,																					// TF_WEAPON_RIOT_SHIELD
	DMG_BULLET | DMG_PREVENT_PHYSICS_FORCE,														// TF_WEAPON_PAINTBALLRIFLE
	DMG_BLAST | DMG_HALF_FALLOFF | DMG_USEDISTANCEMOD,											// TF_WEAPON_BRIMSTONELAUNCHER
#ifdef TF2C_BETA
	DMG_BLAST | DMG_HALF_FALLOFF | DMG_USEDISTANCEMOD,											// TF_WEAPON_PILLSTREAK
	DMG_BLAST | DMG_HALF_FALLOFF,																// TF_WEAPON_HEALLAUNCHER
	DMG_CLUB,																					// TF_WEAPON_ANCHOR
	DMG_BLAST | DMG_HALF_FALLOFF | DMG_USEDISTANCEMOD,											// TF_WEAPON_CYCLOPS
	DMG_CLUB,																					// TF_WEAPON_LEAPKNIFE
#endif
	// This is a special entry that must match with TF_WEAPON_COUNT
	// to protect against updating the weapon list without updating this list.
	TF_DMG_SENTINEL_VALUE
};

const char *g_szProjectileNames[] =
{
	"",
	"projectile_bullet",
	"projectile_rocket",
	"projectile_pipe",
	"projectile_pipe_remote",
	"projectile_syringe",
	"projectile_flare",
	"projectile_jar",
	"projectile_arrow",
	"projectile_flame_rocket",
	"projectile_jar_milk",
	"projectile_healing_bolt",
	"projectile_energy_ball",
	"projectile_energy_ring",
	"projectile_pipe_remote_practice",
	"projectile_cleaver",
	"projectile_sticky_ball",
	"projectile_cannonball",
	"projectile_building_repair_bolt",
	"projectile_festive_arrow",
	"projectile_throwable",
	"projectile_spellfireball",
	"projectile_festive_urine",
	"projectile_festive_healing_bolt",
	"projectfile_breadmonster_jarate",
	"projectfile_breadmonster_madmilk",
	"projectile_grapplinghook",
	"projectile_sentry_rocket",
	"projectile_bread_monster",

	// Add new projectiles here.
	"projectile_nail",
	"projectile_dart",
	"projectile_mirv",
	"projectile_coil",
	"projectile_brick",
	"projectile_throwingknife",
	"projectile_paintball",
#ifdef TF2C_BETA
	"projectile_grenade_healimpact",
	"projectile_generator_uber",
	"projectile_grenade_cyclops",
#endif
};

COMPILE_TIME_ASSERT(ARRAYSIZE(g_szProjectileNames) == TF_NUM_PROJECTILES);

const char *g_pszHintMessages[] =
{
	"#Hint_spotted_a_friend",
	"#Hint_spotted_an_enemy",
	"#Hint_killing_enemies_is_good",
	"#Hint_out_of_ammo",
	"#Hint_turn_off_hints",
	"#Hint_pickup_ammo",
	"#Hint_Cannot_Teleport_With_Flag",
	"#Hint_Cannot_Cloak_With_Flag",
	"#Hint_Cannot_Disguise_With_Flag",
	"#Hint_Cannot_Attack_While_Cloaked",
	"#Hint_ClassMenu",

	// Grenades
	"#Hint_gren_caltrops",
	"#Hint_gren_concussion",
	"#Hint_gren_emp",
	"#Hint_gren_gas",
	"#Hint_gren_mirv",
	"#Hint_gren_nail",
	"#Hint_gren_napalm",
	"#Hint_gren_normal",

	// Altfires
	"#Hint_altfire_sniperrifle",
	"#Hint_altfire_flamethrower",
	"#Hint_altfire_grenadelauncher",
	"#Hint_altfire_pipebomblauncher",
	"#Hint_altfire_rotate_building",

	// Soldier
	"#Hint_Soldier_rpg_reload",

	// Engineer
	"#Hint_Engineer_use_wrench_onown",
	"#Hint_Engineer_use_wrench_onother",
	"#Hint_Engineer_use_wrench_onfriend",
	"#Hint_Engineer_build_sentrygun",
	"#Hint_Engineer_build_dispenser",
	"#Hint_Engineer_build_teleporters",
	"#Hint_Engineer_pickup_metal",
	"#Hint_Engineer_repair_object",
	"#Hint_Engineer_metal_to_upgrade",
	"#Hint_Engineer_upgrade_sentrygun",

	"#Hint_object_has_sapper",

	"#Hint_object_your_object_sapped",
	"#Hint_enemy_using_dispenser",
	"#Hint_enemy_using_tp_entrance",
	"#Hint_enemy_using_tp_exit",
};

const char *g_pszArrowModels[] = 
{
	"models/weapons/w_models/w_arrow.mdl",
};
COMPILE_TIME_ASSERT( ARRAYSIZE( g_pszArrowModels ) == TF_ARROW_MODEL_COUNT );


ETFWeaponID GetWeaponId( const char *pszWeaponName )
{
	// If this doesn't match, you need to add missing weapons to the array.
	COMPILE_TIME_ASSERT( ARRAYSIZE( g_aWeaponNames ) == ( TF_WEAPON_COUNT + 1 ) );

	for ( int iWeapon = 0, iWeaponCount = ARRAYSIZE( g_aWeaponNames ); iWeapon < iWeaponCount; ++iWeapon )
	{
		if ( !Q_stricmp( pszWeaponName, g_aWeaponNames[iWeapon] ) )
			return (ETFWeaponID)iWeapon;
	}

	return TF_WEAPON_NONE;
}



const char *WeaponIdToAlias( ETFWeaponID iWeapon )
{
	// If this doesn't match, you need to add missing weapons to the array.
	COMPILE_TIME_ASSERT( ARRAYSIZE( g_aWeaponNames ) == ( TF_WEAPON_COUNT + 1 ) );

	if ( iWeapon >= ARRAYSIZE( g_aWeaponNames ) || iWeapon < 0 )
		return NULL;

	return g_aWeaponNames[iWeapon];
}

//-----------------------------------------------------------------------------
// Purpose: Entity classnames need to be in lower case. Use this whenever
// you're spawning a weapon.
//-----------------------------------------------------------------------------
const char *WeaponIdToClassname( ETFWeaponID iWeapon )
{
	// If this doesn't match, you need to add missing weapons to the array.
	COMPILE_TIME_ASSERT( ARRAYSIZE( g_aWeaponNames ) == ( TF_WEAPON_COUNT + 1 ) );

	if ( iWeapon >= ARRAYSIZE( g_aWeaponNames ) || iWeapon < 0 )
		return NULL;

	static char szEntName[256];
	V_strcpy_safe( szEntName, g_aWeaponNames[iWeapon] );
	V_strlower( szEntName );
	return szEntName;
}


bool ClassOwnsWeapon( int iClass, ETFWeaponID iWeapon )
{
	for ( int i = 0; i < TF_PLAYER_WEAPON_COUNT; i++ )
	{
		if ( GetPlayerClassData( iClass )->m_aWeapons[i] == iWeapon )
			return true;
	}

	return false;
}


const char *TranslateWeaponEntForClass( const char *pszName, int iClass )
{
	if ( pszName )
	{
		for ( int i = 0; i < ARRAYSIZE( pszWpnEntTranslationList ); i++ )
		{
			if ( !V_stricmp( pszName, pszWpnEntTranslationList[i].weapon_name ) )
				return ( (const char **)&( pszWpnEntTranslationList[i] ) )[1 + iClass];
		}
	}
	return pszName;
}

#ifdef GAME_DLL

ETFWeaponID GetWeaponFromDamage( const CTakeDamageInfo &info )
{
	KillingWeaponData_t weaponData;
	TFGameRules()->GetKillingWeaponName( info, NULL, weaponData );
	return weaponData.iWeaponID;
}
#endif

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool WeaponID_IsSniperRifle( int iWeaponID )
{
	return iWeaponID == TF_WEAPON_SNIPERRIFLE/* || iWeaponID == TF_WEAPON_SNIPERRIFLE_DECAP || iWeaponID == TF_WEAPON_SNIPERRIFLE_CLASSIC*/;
}

bool WeaponID_IsLunchbox( int iWeaponID )
{
	return iWeaponID == TF_WEAPON_LUNCHBOX/* || iWeaponID == TF_WEAPON_LUNCHBOX_DRINK*/;
}

//-----------------------------------------------------------------------------
// Conditions stuff.
//-----------------------------------------------------------------------------
ETFCond g_eAttributeConditions[] =
{
	TF_COND_BURNING,
	TF_COND_AIMING,
	TF_COND_ZOOMED,
	TF_COND_DISGUISING,
	TF_COND_DISGUISED,
	TF_COND_STEALTHED,
	TF_COND_INVULNERABLE,
	TF_COND_TELEPORTED,
	TF_COND_TAUNTING,
	TF_COND_INVULNERABLE_WEARINGOFF,
	TF_COND_STEALTHED_BLINK,
	TF_COND_SELECTED_TO_TELEPORT,
	TF_COND_CRITBOOSTED,
	TF_COND_TMPDAMAGEBONUS,
	TF_COND_FEIGN_DEATH,
	TF_COND_PHASE,
	TF_COND_STUNNED,
	TF_COND_HEALTH_BUFF,
	TF_COND_HEALTH_OVERHEALED,
	TF_COND_URINE,
	TF_COND_ENERGY_BUFF,				// 1048576
	TF_COND_LAST
};

ETFCond g_eDebuffConditions[] =
{
	TF_COND_BURNING,
	TF_COND_URINE,
	TF_COND_BLEEDING,
	TF_COND_MAD_MILK,
	TF_COND_TRANQUILIZED,
	TF_COND_LAST
};


// when changing priority make sure same priority is applied for CTFGameRules::ApplyOnDamageModifyRules crit transformations order
ETFCond g_eAttackerAssistConditions[] =
{
	TF_COND_CRITBOOSTED,			// Highest priority.
	TF_COND_DAMAGE_BOOST,
 	TF_COND_CIV_DEFENSEBUFF,
	TF_COND_CIV_SPEEDBUFF,
	TF_COND_RESISTANCE_BUFF,
	TF_COND_HEALTH_BUFF,
	TF_COND_LAUNCHED,
	TF_COND_JUMPPAD_ASSIST,		// Lowest priority.
	TF_COND_LAST
};

// when changing priority make sure same priority is applied for CTFGameRules::ApplyOnDamageModifyRules crit transformations order
ETFCond g_eVictimAssistConditions[] =
{
	TF_COND_SUPERMARKEDFORDEATH,	// Highest priority.
	TF_COND_MARKEDFORDEATH,
	TF_COND_TRANQUILIZED,
	TF_COND_MIRV_SLOW, // Lowest priority.
	TF_COND_LAST
};

bool ConditionExpiresFast( ETFCond nCond )
{
	// Damaging conds.
	if ( nCond == TF_COND_BURNING ||
		nCond == TF_COND_BLEEDING )
		return true;

	// Liquids.
	if ( nCond == TF_COND_URINE ||
		nCond == TF_COND_MAD_MILK )
		return true;

	// Tranq.
	if ( nCond == TF_COND_TRANQUILIZED )
		return true;

	return false;
}


//-----------------------------------------------------------------------------
// Mediguns.
//-----------------------------------------------------------------------------
MedigunEffects_t g_MedigunEffects[] =
{
	{ TF_COND_INVULNERABLE, TF_COND_INVULNERABLE_WEARINGOFF, "TFPlayer.InvulnerableOn", "TFPlayer.InvulnerableOff" },
	{ TF_COND_CRITBOOSTED, TF_COND_LAST, "TFPlayer.CritBoostOn", "TFPlayer.CritBoostOff" },
	{ TF_COND_RADIAL_UBER, TF_COND_LAST, "TFPlayer.InvulnerableOn", "TFPlayer.InvulnerableOff" },
	{ TF_COND_MEGAHEAL, TF_COND_LAST, "TFPlayer.QuickFixInvulnerableOn", "TFPlayer.MegaHealOff" },
	{ TF_COND_MEDIGUN_UBER_BULLET_RESIST, TF_COND_LAST, "WeaponMedigun_Vaccinator.InvulnerableOn", "WeaponMedigun_Vaccinator.InvulnerableOff" },
	{ TF_COND_MEDIGUN_UBER_BLAST_RESIST, TF_COND_LAST, "WeaponMedigun_Vaccinator.InvulnerableOn", "WeaponMedigun_Vaccinator.InvulnerableOff" },
	{ TF_COND_MEDIGUN_UBER_FIRE_RESIST, TF_COND_LAST, "WeaponMedigun_Vaccinator.InvulnerableOn", "WeaponMedigun_Vaccinator.InvulnerableOff" },
};

static CUtlStringList s_BotNames;

static void LoadBotNames()
{
	Assert( s_BotNames.IsEmpty() );
	
	CUtlBuffer buf( 0, 0, CUtlBuffer::TEXT_BUFFER );
	if ( !filesystem->ReadFile( "scripts/tfbot_names.txt", "GAME", buf ) ) {
		Warning( "Couldn't open/read TFBot name list file!\n" );
		return;
	}
	
	char line[0x100];
	while ( true ) {
		buf.GetLine( line, sizeof( line ) );
		if ( !buf.IsValid() ) break;
		
		Q_StripPrecedingAndTrailingWhitespace( line );
		if ( strlen( line ) != 0 ) {
			s_BotNames.CopyAndAddToTail( line );
		}
	}
	
	if ( s_BotNames.IsEmpty() ) {
		Warning( "No names found in the TFBot name list file!\n" );
	}
}

const char *GetRandomBotName()
{
	if ( s_BotNames.IsEmpty() ) {
		LoadBotNames();
		if ( s_BotNames.IsEmpty() ) {
			s_BotNames.CopyAndAddToTail( "TFBot" );
		}
	}
	
	static int idx = RandomInt( 0, s_BotNames.Count() - 1 );
	
	const char *name = s_BotNames[idx++];
	
	if ( idx >= s_BotNames.Count() ) {
		idx = 0;
	}
	
	return name;
}

const char *GetSeededRandomBotName( int iSeed )
{
	if ( s_BotNames.IsEmpty() ) {
		LoadBotNames();
		if ( s_BotNames.IsEmpty() ) {
			s_BotNames.CopyAndAddToTail( "TFBot" );
		}
	}
	
	RandomSeed( iSeed );
	static int idx = RandomInt( 0, s_BotNames.Count() - 1 );
	
	const char *name = s_BotNames[idx++];
	
	if ( idx >= s_BotNames.Count() ) {
		idx = 0;
	}
	
	return name;
}

// ------------------------------------------------------------------------------------------------ //
// CObjectInfo tables.
// ------------------------------------------------------------------------------------------------ //

CObjectInfo::CObjectInfo( const char *pObjectName )
{
	m_pObjectName = pObjectName;
	m_pClassName = NULL;
	m_flBuildTime = -9999;
	m_nMaxObjects = -9999;
	m_Cost = -9999;
	m_CostMultiplierPerInstance = -999;
	m_UpgradeCost = -9999;
	m_flUpgradeDuration = -9999;
	m_MaxUpgradeLevel = -9999;
	m_pBuilderWeaponName = NULL;
	m_pBuilderPlacementString = NULL;
	m_SelectionSlot = -9999;
	m_SelectionPosition = -9999;
	m_bSolidToPlayerMovement = false;
	m_pIconActive = NULL;
	m_pIconInactive = NULL;
	m_pIconMenu = NULL;
	m_pViewModel = NULL;
	m_pPlayerModel = NULL;
	m_iDisplayPriority = 0;
	m_bVisibleInWeaponSelection = true;
	m_pExplodeSound = NULL;
	m_pExplosionParticleEffect = NULL;
	m_bAutoSwitchTo = false;
	m_pUpgradeSound = NULL;
}


CObjectInfo::~CObjectInfo()
{
	delete[] m_pClassName;
	delete[] m_pStatusName;
	delete[] m_pBuilderWeaponName;
	delete[] m_pBuilderPlacementString;
	delete[] m_pIconActive;
	delete[] m_pIconInactive;
	delete[] m_pIconMenu;
	delete[] m_pViewModel;
	delete[] m_pPlayerModel;
	delete[] m_pExplodeSound;
	delete[] m_pExplosionParticleEffect;
	delete[] m_pUpgradeSound;
}

CObjectInfo g_ObjectInfos[OBJ_LAST] =
{
	CObjectInfo( "OBJ_DISPENSER" ),
	CObjectInfo( "OBJ_TELEPORTER" ),
	CObjectInfo( "OBJ_SENTRYGUN" ),
	CObjectInfo( "OBJ_ATTACHMENT_SAPPER" ),
	CObjectInfo( "OBJ_JUMPPAD" ),
	CObjectInfo( "OBJ_MINIDISPENSER" ),
	CObjectInfo( "OBJ_FLAMESENTRY" ),
};


int GetBuildableId( const char *pszBuildableName )
{
	for ( int iBuildable = 0; iBuildable < OBJ_LAST; ++iBuildable )
	{
		if ( !Q_stricmp( pszBuildableName, g_ObjectInfos[iBuildable].m_pObjectName ) )
			return iBuildable;
	}

	return OBJ_LAST;
}

bool AreObjectInfosLoaded()
{
	return g_ObjectInfos[0].m_pClassName != NULL;
}


void LoadObjectInfos( IBaseFileSystem *pFileSystem )
{
	const char *pFilename = "scripts/objects.txt";

	// Make sure this stuff hasn't already been loaded.
	Assert( !AreObjectInfosLoaded() );

	KeyValues *pValues = new KeyValues( "Object descriptions" );
	if ( !pValues->LoadFromFile( pFileSystem, pFilename, "GAME" ) )
	{
		Error( "Can't open %s for object info.", pFilename );
		pValues->deleteThis();
		return;
	}

	// Now read each class's information in.
	for ( int iObj = 0; iObj < ARRAYSIZE( g_ObjectInfos ); iObj++ )
	{
		CObjectInfo *pInfo = &g_ObjectInfos[iObj];
		KeyValues *pSub = pValues->FindKey( pInfo->m_pObjectName );
		if ( !pSub )
		{
			Error( "Missing section '%s' from %s.", pInfo->m_pObjectName, pFilename );
			pValues->deleteThis();
			return;
		}

		// Read all the info in.
		if ( ( pInfo->m_flBuildTime = pSub->GetFloat( "BuildTime", -999 ) ) == -999 ||
			( pInfo->m_nMaxObjects = pSub->GetInt( "MaxObjects", -999 ) ) == -999 ||
			( pInfo->m_Cost = pSub->GetInt( "Cost", -999 ) ) == -999 ||
			( pInfo->m_CostMultiplierPerInstance = pSub->GetFloat( "CostMultiplier", -999 ) ) == -999 ||
			( pInfo->m_UpgradeCost = pSub->GetInt( "UpgradeCost", -999 ) ) == -999 ||
			( pInfo->m_flUpgradeDuration = pSub->GetFloat( "UpgradeDuration", -999 ) ) == -999 ||
			( pInfo->m_MaxUpgradeLevel = pSub->GetInt( "MaxUpgradeLevel", -999 ) ) == -999 ||
			( pInfo->m_SelectionSlot = pSub->GetInt( "SelectionSlot", -999 ) ) == -999 ||
			( pInfo->m_BuildCount = pSub->GetInt( "BuildCount", -999 ) ) == -999 ||
			( pInfo->m_SelectionPosition = pSub->GetInt( "SelectionPosition", -999 ) ) == -999 )
		{
			Error( "Missing data for object '%s' in %s.", pInfo->m_pObjectName, pFilename );
			pValues->deleteThis();
			return;
		}

		pInfo->m_pClassName = ReadAndAllocStringValue( pSub, "ClassName", pFilename );
		pInfo->m_pStatusName = ReadAndAllocStringValue( pSub, "StatusName", pFilename );
		pInfo->m_pBuilderWeaponName = ReadAndAllocStringValue( pSub, "BuilderWeaponName", pFilename );
		pInfo->m_pBuilderPlacementString = ReadAndAllocStringValue( pSub, "BuilderPlacementString", pFilename );
		pInfo->m_bSolidToPlayerMovement = pSub->GetInt( "SolidToPlayerMovement", 0 ) ? true : false;
		pInfo->m_pIconActive = ReadAndAllocStringValue( pSub, "IconActive", pFilename );
		pInfo->m_pIconInactive = ReadAndAllocStringValue( pSub, "IconInactive", pFilename );
		pInfo->m_pIconMenu = ReadAndAllocStringValue( pSub, "IconMenu", pFilename );
		pInfo->m_bUseItemInfo = pSub->GetInt( "UseItemInfo", 0 ) ? true : false;
		pInfo->m_pViewModel = ReadAndAllocStringValue( pSub, "Viewmodel", pFilename );
		pInfo->m_pPlayerModel = ReadAndAllocStringValue( pSub, "Playermodel", pFilename );
		pInfo->m_iDisplayPriority = pSub->GetInt( "DisplayPriority", 0 );
		pInfo->m_pHudStatusIcon = ReadAndAllocStringValue( pSub, "HudStatusIcon", pFilename );
		pInfo->m_bVisibleInWeaponSelection = ( pSub->GetInt( "VisibleInWeaponSelection", 1 ) > 0 );
		pInfo->m_pExplodeSound = ReadAndAllocStringValue( pSub, "ExplodeSound", pFilename );
		pInfo->m_pUpgradeSound = ReadAndAllocStringValue( pSub, "UpgradeSound", pFilename );
		pInfo->m_pExplosionParticleEffect = ReadAndAllocStringValue( pSub, "ExplodeEffect", pFilename );
		pInfo->m_bAutoSwitchTo = ( pSub->GetInt( "autoswitchto", 0 ) > 0 );

		pInfo->m_iMetalToDropInGibs = pSub->GetInt( "MetalToDropInGibs", 0 );
		pInfo->m_bRequiresOwnBuilder = pSub->GetBool( "RequiresOwnBuilder", 0 );

		// PistonMiner: Added Object Mode key.
		KeyValues *pAltModes = pSub->FindKey( "AltModes" );
		if ( pAltModes )
		{
			// Load at most 4 object modes.
			for ( int i = 0; i < 4; ++i )
			{
				// Max size of 0x100.
				char altModeBuffer[256];
				V_sprintf_safe( altModeBuffer, "AltMode%d", i );
				KeyValues *pCurAltMode = pAltModes->FindKey( altModeBuffer );
				if ( !pCurAltMode )
					break;

				// Save logic here.
				pInfo->m_AltModes.AddToTail( ReadAndAllocStringValue( pCurAltMode, "StatusName", pFilename ) );
				pInfo->m_AltModes.AddToTail( ReadAndAllocStringValue( pCurAltMode, "ModeName", pFilename ) );
				pInfo->m_AltModes.AddToTail( ReadAndAllocStringValue( pCurAltMode, "IconMenu", pFilename ) );
			}
		}
	}

	pValues->deleteThis();
}


const CObjectInfo *GetObjectInfo( int iObject )
{
	Assert( iObject >= 0 && iObject < OBJ_LAST );
	Assert( AreObjectInfosLoaded() );
	return &g_ObjectInfos[iObject];
}


const unsigned char *GetTFEncryptionKey( void )
{ 
	return (unsigned char *)"E2NcUkG2"; 
}

const char *taunt_attack_name[TAUNTATK_COUNT] =
{
	"TAUNTATK_NONE",
	"TAUNTATK_PYRO_HADOUKEN",
	"TAUNTATK_HEAVY_EAT",
	"TAUNTATK_HEAVY_RADIAL_BUFF",
	"TAUNTATK_HEAVY_HIGH_NOON",
	"TAUNTATK_SCOUT_DRINK",
	"TAUNTATK_SCOUT_GRAND_SLAM",
	"TAUNTATK_MEDIC_INHALE",
	"TAUNTATK_SPY_FENCING_SLASH_A",
	"TAUNTATK_SPY_FENCING_SLASH_B",
	"TAUNTATK_SPY_FENCING_STAB",
	"TAUNTATK_RPS_KILL",
	"TAUNTATK_SNIPER_ARROW_STAB_IMPALE",
	"TAUNTATK_SNIPER_ARROW_STAB_KILL",
	"TAUNTATK_SOLDIER_GRENADE_KILL",
	"TAUNTATK_DEMOMAN_BARBARIAN_SWING",
	"TAUNTATK_MEDIC_UBERSLICE_IMPALE",
	"TAUNTATK_MEDIC_UBERSLICE_KILL",
	"TAUNTATK_FLIP_LAND_PARTICLE",
	"TAUNTATK_RPS_PARTICLE",
	"TAUNTATK_HIGHFIVE_PARTICLE",
	"TAUNTATK_ENGINEER_GUITAR_SMASH",
	"TAUNTATK_ENGINEER_ARM_IMPALE",
	"TAUNTATK_ENGINEER_ARM_KILL",
	"TAUNTATK_ENGINEER_ARM_BLEND",
	"TAUNTATK_SOLDIER_GRENADE_KILL_WORMSIGN",
	"TAUNTATK_SHOW_ITEM",
	"TAUNTATK_MEDIC_RELEASE_DOVES",
	"TAUNTATK_PYRO_ARMAGEDDON",
	"TAUNTATK_PYRO_SCORCHSHOT",
	"TAUNTATK_ALLCLASS_GUITAR_RIFF",
	"TAUNTATK_MEDIC_HEROIC_TAUNT",
};

int GetTauntAttackByName( const char *pszName )
{
	for ( int i = 0; i < TAUNTATK_COUNT; i++ )
	{
		if ( !V_stricmp( pszName, taunt_attack_name[i] ) )
			return i;
	}

	return TAUNTATK_NONE;
}

#ifdef STEAM_GROUP_CHECKPOINT
volatile uint64 *pulMaskGroup; // 0xAFB2423BBAA352DA
volatile uint64 ulGroupID; // 103582791461599320 ^ ulMaskGroup
#endif
