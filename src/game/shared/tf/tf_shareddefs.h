//====== Copyright © 1996-2006, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================
#ifndef TF_SHAREDDEFS_H
#define TF_SHAREDDEFS_H
#ifdef _WIN32
#pragma once
#endif

#include "shareddefs.h"
#include "mp_shareddefs.h"
#include "takedamageinfo.h" //already gets leaked into client so

// Using MAP_DEBUG mode?
#ifdef MAP_DEBUG
	#define MDEBUG(x) x
#else
	#define MDEBUG(x)
#endif

#define TF_PLAYER_SCRIPED_SEQUENCES 0

//-----------------------------------------------------------------------------
// Teams.
//-----------------------------------------------------------------------------
enum
{
	TF_TEAM_RED = LAST_SHARED_TEAM + 1,
	TF_TEAM_BLUE,
	TF_TEAM_GREEN,
	TF_TEAM_YELLOW,
	TF_TEAM_COUNT,

	// Used exclusively for global disguises
	// We don't count it as part of the total team count
	TF_TEAM_GLOBAL,

	TF_TEAM_PVE_DEFENDERS = TF_TEAM_RED,
	TF_TEAM_PVE_INVADERS = TF_TEAM_BLUE,
	TF_TEAM_PVE_INVADERS_GIANTS = TF_TEAM_BLUE
};

enum
{
	TF_HOLIDAY_BIRTHDAY = 1,
	TF_HOLIDAY_HALLOWEEN,
	TF_HOLIDAY_WINTER
};

//-----------------------------------------------------------------------------
// Player Ranks.
//-----------------------------------------------------------------------------
enum
{
	TF_RANK_INVALID = -1,

	TF_RANK_NONE,
	TF_RANK_PLAYTESTER		= ( 1 << 0 ),
	TF_RANK_CONTRIBUTOR		= ( 1 << 1 ),
	TF_RANK_DEVELOPER		= ( 1 << 2 ),

	TF_RANK_COUNT			= ( TF_RANK_PLAYTESTER | TF_RANK_CONTRIBUTOR | TF_RANK_DEVELOPER )
};

#define TF_TEAM_AUTOASSIGN (TF_TEAM_COUNT + 1 )
#define TF_ORIGINAL_TEAM_COUNT ( TF_TEAM_BLUE + 1 )

extern const char *g_aTeamNames_Localized[TF_TEAM_COUNT];
extern const char *g_aTeamNames[TF_TEAM_COUNT];
extern const char *g_aTeamNamesShort[TF_TEAM_COUNT];
extern const char *g_aTeamUpperNamesShort[TF_TEAM_COUNT];
extern const char *g_aTeamLowerNames[TF_TEAM_COUNT];
extern const char *g_aTeamUpperNames[TF_TEAM_COUNT];
extern const char *g_aTeamUpperNamesShort[TF_TEAM_COUNT];

extern Vector g_aTeamParticleColors[TF_TEAM_COUNT];
extern color32 g_aTeamColors[TF_TEAM_COUNT];

template<typename Functor>
void ForEachTeamName( Functor &&func, const char **pNames = g_aTeamLowerNames )
{
	for ( int i = FIRST_GAME_TEAM; i < TF_TEAM_COUNT; i++ )
	{
		func( pNames[i] );
	}
}

inline const char *GetTeamSuffix( int iTeam, const char **pNames = g_aTeamLowerNames, const char *pszNeutralName = NULL )
{
	// Most effects and textures don't have versions for Unassigned and Spectator teams for obvious reasons so we need a fallback.
	// This is also used for HUD since texture artists can't seem to agree on what suffix should be used for unassigned CPs and such.
	if ( iTeam < FIRST_GAME_TEAM && pszNeutralName )
		return pszNeutralName;

	return pNames[iTeam];
}

inline const char *ConstructTeamParticle( const char *pszFormat, int iTeam, const char **pNames = g_aTeamLowerNames )
{
	// "red" by default because people will spawn unassigned buildings through console and then complain about missing particles.
	static char szParticleName[128];
	V_sprintf_safe( szParticleName, pszFormat, GetTeamSuffix( iTeam, pNames, "red" ) );
	return szParticleName;
}

inline void PrecacheTeamParticles( const char *pszFormat, const char **pNames = g_aTeamLowerNames )
{
	ForEachTeamName( [=]( const char *pszTeam )
	{
		char szParticle[128];
		V_snprintf( szParticle, sizeof( szParticle ), pszFormat, pszTeam );
		PrecacheParticleSystem( szParticle );
	}, pNames );
}

inline const char *ConstructTeamString( const char *pszFormat, int iTeam, const char **pNames = g_aTeamLowerNames )
{
	return ConstructTeamParticle( pszFormat, iTeam, pNames );
}

#ifdef CLIENT_DLL
inline const char *GetTeamMedalString( int iTeam, int iRank )
{
	if ( iTeam < FIRST_GAME_TEAM )
		return NULL;

	if ( iRank <= TF_RANK_NONE || iRank > TF_RANK_COUNT )
		return NULL;

	const char *pszMedalType;
	if ( iRank & TF_RANK_DEVELOPER )
	{
		pszMedalType = "dev";
	}
	else if ( iRank & TF_RANK_CONTRIBUTOR )
	{
		pszMedalType = "contributor";
	}
	else if ( iRank & TF_RANK_PLAYTESTER )
	{
		pszMedalType = "tester";
	}
	else
		return NULL;

	return VarArgs( "../hud/medal_%s_%s", pszMedalType, g_aTeamLowerNames[iTeam] );
}
#endif

inline int GetTeamSkin( int iTeam )
{
	switch ( iTeam )
	{
		case TF_TEAM_RED:
			return 0;
		case TF_TEAM_BLUE:
			return 1;
		case TF_TEAM_GREEN:
			return 2;
		case TF_TEAM_YELLOW:
			return 3;
#ifdef TF2C_BETA
		case TF_TEAM_GLOBAL:
			return 4;
#endif
		default:
			return 0;
	}
}

#define CONTENTS_REDTEAM	CONTENTS_TEAM1
#define CONTENTS_BLUETEAM	CONTENTS_TEAM2
#define CONTENTS_GREENTEAM	CONTENTS_UNUSED
#define CONTENTS_YELLOWTEAM	CONTENTS_UNUSED6
			
// Team roles
enum 
{
	TEAM_ROLE_NONE = 0,
	TEAM_ROLE_DEFENDERS,
	TEAM_ROLE_ATTACKERS,

	NUM_TEAM_ROLES,
};

//-----------------------------------------------------------------------------
// CVar replacements
//-----------------------------------------------------------------------------
#define TF_DAMAGE_CRIT_CHANCE				0.02f
#define TF_DAMAGE_CRIT_CHANCE_RAPID			0.02f
#define TF_DAMAGE_CRIT_DURATION_RAPID		2.0f
#define TF_DAMAGE_CRIT_CHANCE_MELEE			0.15f

#define TF_DAMAGE_CRITMOD_MAXTIME			20
#define TF_DAMAGE_CRITMOD_MINTIME			2
#define TF_DAMAGE_CRITMOD_DAMAGE			800
#define TF_DAMAGE_CRITMOD_MAXMULT			4

#define TF_DAMAGE_CRIT_MULTIPLIER			3.0f
#define TF_DAMAGE_MINICRIT_MULTIPLIER		1.35f


//-----------------------------------------------------------------------------
// TF-specific viewport panels
//-----------------------------------------------------------------------------
#define PANEL_MAPINFO				"mapinfo"
#define PANEL_ROUNDINFO				"roundinfo"
#define PANEL_FOURTEAMSCOREBOARD	"fourteamscoreboard"
#define PANEL_FOURTEAMSELECT		"fourteamselect"
#define PANEL_ARENA_TEAM			"arenateampanel"

// File we'll save our list of viewed intro movies in.
#define MOVIES_FILE					"viewed.res"

//-----------------------------------------------------------------------------
// Player Classes.
//-----------------------------------------------------------------------------
#define TF_CLASS_COUNT			( TF_CLASS_COUNT_ALL - 1 )

#define TF_FIRST_NORMAL_CLASS	( TF_CLASS_UNDEFINED + 1 )
#define TF_LAST_NORMAL_CLASS	( TF_CLASS_CIVILIAN - 1 )

#define	TF_CLASS_MENU_BUTTONS	( TF_CLASS_RANDOM + 1 )

enum
{
	TF_CLASS_UNDEFINED = 0,

	TF_CLASS_SCOUT,			// TF_FIRST_NORMAL_CLASS
    TF_CLASS_SNIPER,
    TF_CLASS_SOLDIER,
	TF_CLASS_DEMOMAN,
	TF_CLASS_MEDIC,
	TF_CLASS_HEAVYWEAPONS,
	TF_CLASS_PYRO,
	TF_CLASS_SPY,
	TF_CLASS_ENGINEER,		// TF_LAST_NORMAL_CLASS

	// Add any new classes after Engineer.
	// The following classes are not available in normal play.
	TF_CLASS_CIVILIAN,

	TF_CLASS_COUNT_ALL,

	TF_CLASS_RANDOM
};

extern const char *g_aPlayerClassNames[TF_CLASS_COUNT_ALL];					// Localized class names.
extern const char *g_aPlayerClassNames_NonLocalized[TF_CLASS_COUNT_ALL];	// Non-localized class names.
extern const char *g_aRawPlayerClassNamesShort[TF_CLASS_COUNT_ALL];
extern const char *g_aRawPlayerClassNames[TF_CLASS_COUNT_ALL];

extern const char *g_aPlayerClassEmblems[TF_CLASS_COUNT_ALL + ( TF_CLASS_COUNT_ALL - 1 )];
extern const char *g_aPlayerClassEmblemsAlt[TF_CLASS_COUNT_ALL + ( TF_CLASS_COUNT_ALL - 1 )];

//-----------------------------------------------------------------------------
// For entity_capture_flags to use when placed in the world.
//-----------------------------------------------------------------------------
enum ETFFlagType
{
	TF_FLAGTYPE_CTF = 0,
	TF_FLAGTYPE_ATTACK_DEFEND,
	TF_FLAGTYPE_TERRITORY_CONTROL,
	TF_FLAGTYPE_INVADE,
	TF_FLAGTYPE_RESOURCE_CONTROL,
	TF_FLAGTYPE_ROBOT_DESTRUCTION,
	TF_FLAGTYPE_RETRIEVE,
	TF_FLAGTYPE_KINGOFTHEHILL,
	TF_FLAGTYPE_VIP,
};

//-----------------------------------------------------------------------------
// For the game rules to determine which type of game we're playing.
//-----------------------------------------------------------------------------
enum ETFGameType
{
	TF_GAMETYPE_UNDEFINED = 0,
	TF_GAMETYPE_CTF,
	TF_GAMETYPE_CP,
	TF_GAMETYPE_ESCORT,
	TF_GAMETYPE_ARENA,
	TF_GAMETYPE_MVM,
	TF_GAMETYPE_RD,
	TF_GAMETYPE_PASSTIME,
	TF_GAMETYPE_PD,
	TF_GAMETYPE_VIP,
	TF_GAMETYPE_POWERSIEGE,
	TF_GAMETYPE_INFILTRATION,

	// TF_GAMETYPE_SIEGE, // Possibly for a future update.
	TF_GAMETYPE_COUNT
};
extern const char *g_aGameTypeNames[TF_GAMETYPE_COUNT];	// Localized gametype names.

//-----------------------------------------------------------------------------
// Items.
//-----------------------------------------------------------------------------
enum
{
	TF_ITEM_UNDEFINED = 0,
	TF_ITEM_CAPTURE_FLAG,
};

//-----------------------------------------------------------------------------
// Ammo.
//-----------------------------------------------------------------------------
enum
{
	TF_AMMO_DUMMY = 0,	// Dummy index to make the CAmmoDef indices correct for the other ammo types.
	TF_AMMO_PRIMARY,
	TF_AMMO_SECONDARY,
	TF_AMMO_METAL,
	TF_AMMO_GRENADES1,
	TF_AMMO_GRENADES2,
	TF_AMMO_COUNT
};

extern const char *g_aAmmoNames[];

enum EAmmoSource
{
	TF_AMMO_SOURCE_AMMOPACK = 0,	// Default, used for ammopacks.
	TF_AMMO_SOURCE_RESUPPLY,		// Maybe?
	TF_AMMO_SOURCE_DISPENSER,
	TF_AMMO_SOURCE_COUNT
};

enum
{
	TF_GIVEAMMO_MAX = -2,
	TF_GIVEAMMO_NONE = -1,
};

//-----------------------------------------------------------------------------
// Weapon Types (aka animation slots; see "anim_slot" in item schema).
//-----------------------------------------------------------------------------
enum ETFWeaponType
{
	TF_WPN_TYPE_PRIMARY = 0,
	TF_WPN_TYPE_SECONDARY,
	TF_WPN_TYPE_MELEE,
	TF_WPN_TYPE_GRENADE,
	TF_WPN_TYPE_BUILDING,
	TF_WPN_TYPE_PDA,
	TF_WPN_TYPE_ITEM1,
	TF_WPN_TYPE_ITEM2,
	TF_WPN_TYPE_HEAD, // Not sure why these two are here...
	TF_WPN_TYPE_MISC,
	TF_WPN_TYPE_MELEE_ALLCLASS,
	TF_WPN_TYPE_SECONDARY2,
	TF_WPN_TYPE_PRIMARY2,
	TF_WPN_TYPE_COUNT,

	TF_WPN_TYPE_INVALID  = -1,
	TF_WPN_TYPE_NOT_USED = -2,
};

//-----------------------------------------------------------------------------
// Loadout slots (aka item slots; see "item_slot" in item schema).
//-----------------------------------------------------------------------------
enum ETFLoadoutSlot
{
	TF_LOADOUT_SLOT_PRIMARY = 0,
	TF_LOADOUT_SLOT_SECONDARY,
	TF_LOADOUT_SLOT_MELEE,
	TF_LOADOUT_SLOT_UTILITY,
	TF_LOADOUT_SLOT_BUILDING,
	TF_LOADOUT_SLOT_PDA1,
	TF_LOADOUT_SLOT_PDA2,
	TF_LOADOUT_SLOT_HAT,
	TF_LOADOUT_SLOT_MISC,
	TF_LOADOUT_SLOT_ACTION,
	TF_LOADOUT_SLOT_TAUNT,
	TF_LOADOUT_SLOT_COUNT,

	TF_LOADOUT_SLOT_INVALID = -1,
};

#define TF_FIRST_COSMETIC_SLOT TF_LOADOUT_SLOT_HAT

extern const char *g_AnimSlots[];
extern const char *g_LoadoutSlots[];

//-----------------------------------------------------------------------------
// Weapons.
//-----------------------------------------------------------------------------
#define TF_PLAYER_WEAPON_COUNT		5
#define TF_PLAYER_GRENADE_COUNT		2
#define TF_PLAYER_BUILDABLE_COUNT	6

#define TF_WEAPON_PRIMARY_MODE		0
#define TF_WEAPON_SECONDARY_MODE	1

enum ETFWeaponID
{
	TF_WEAPON_NONE = 0,
	TF_WEAPON_BAT,
	TF_WEAPON_BOTTLE,
	TF_WEAPON_FIREAXE,
	TF_WEAPON_CLUB,
	TF_WEAPON_CROWBAR,
	TF_WEAPON_KNIFE,
	TF_WEAPON_FISTS,
	TF_WEAPON_SHOVEL,
	TF_WEAPON_WRENCH,
	TF_WEAPON_BONESAW,
	TF_WEAPON_SHOTGUN_PRIMARY,
	TF_WEAPON_SHOTGUN_SOLDIER,
	TF_WEAPON_SHOTGUN_HWG,
	TF_WEAPON_SHOTGUN_PYRO,
	TF_WEAPON_SCATTERGUN,
	TF_WEAPON_SNIPERRIFLE,
	TF_WEAPON_MINIGUN,
	TF_WEAPON_SMG,
	TF_WEAPON_SYRINGEGUN_MEDIC,
	TF_WEAPON_TRANQ,
	TF_WEAPON_ROCKETLAUNCHER,
	TF_WEAPON_GRENADELAUNCHER,
	TF_WEAPON_PIPEBOMBLAUNCHER,
	TF_WEAPON_FLAMETHROWER,
	TF_WEAPON_GRENADE_NORMAL,
	TF_WEAPON_GRENADE_CONCUSSION,
	TF_WEAPON_GRENADE_NAIL,
	TF_WEAPON_GRENADE_MIRV,
	TF_WEAPON_GRENADE_MIRV_DEMOMAN,
	TF_WEAPON_GRENADE_NAPALM,
	TF_WEAPON_GRENADE_GAS,
	TF_WEAPON_GRENADE_EMP,
	TF_WEAPON_GRENADE_CALTROP,
	TF_WEAPON_GRENADE_PIPEBOMB,
	TF_WEAPON_GRENADE_SMOKE_BOMB,
	TF_WEAPON_GRENADE_HEAL,
	TF_WEAPON_PISTOL,
	TF_WEAPON_PISTOL_SCOUT,
	TF_WEAPON_REVOLVER,
	TF_WEAPON_NAILGUN,
	TF_WEAPON_PDA,
	TF_WEAPON_PDA_ENGINEER_BUILD,
	TF_WEAPON_PDA_ENGINEER_DESTROY,
	TF_WEAPON_PDA_SPY,
	TF_WEAPON_BUILDER,
	TF_WEAPON_MEDIGUN,
	TF_WEAPON_GRENADE_MIRVBOMB,
	TF_WEAPON_FLAMETHROWER_ROCKET,
	TF_WEAPON_GRENADE_DEMOMAN,
	TF_WEAPON_SENTRY_BULLET,
	TF_WEAPON_SENTRY_ROCKET,
	TF_WEAPON_DISPENSER,
	TF_WEAPON_INVIS,
	TF_WEAPON_FLAG,
	TF_WEAPON_FLAREGUN,
	TF_WEAPON_LUNCHBOX,
	TF_WEAPON_COMPOUND_BOW,
	TF_WEAPON_SHOTGUN_BUILDING_RESCUE,

	// Add new weapons after this.
	TF_WEAPON_HUNTERRIFLE,
	TF_WEAPON_COILGUN,
	TF_WEAPON_UMBRELLA,
	TF_WEAPON_CUBEMAP,
	TF_WEAPON_TASER,
	TF_WEAPON_AAGUN,
	TF_WEAPON_RUSSIANROULETTE,
	TF_WEAPON_BEACON,
	TF_WEAPON_DOUBLESHOTGUN,
	TF_WEAPON_THROWABLE_BRICK,
	TF_WEAPON_GRENADE_MIRV2,
	TF_WEAPON_DETONATOR,
	TF_WEAPON_THROWINGKNIFE,
	TF_WEAPON_RIOT_SHIELD,
	TF_WEAPON_PAINTBALLRIFLE,
	TF_WEAPON_BRIMSTONELAUNCHER,
#ifdef TF2C_BETA
	TF_WEAPON_PILLSTREAK,
	TF_WEAPON_HEALLAUNCHER,
	TF_WEAPON_ANCHOR,
	TF_WEAPON_CYCLOPS,
	TF_WEAPON_LEAPKNIFE,
#endif
	TF_WEAPON_COUNT
};

enum ETFDetonateModes {
	TF_DETMODE_DEFAULT = 0,
	TF_DETMODE_WHATIS1MEANTTOBE = 1,
	TF_DETMODE_FIZZLEONWORLD,		// 2
	TF_DETMODE_EXPLODEONWORLD		// 3
};

extern const char *g_aWeaponNames[];
extern int g_aWeaponDamageTypes[];

ETFWeaponID GetWeaponId( const char *pszWeaponName );
#ifdef GAME_DLL
ETFWeaponID GetWeaponFromDamage( const CTakeDamageInfo &info );
#endif
int GetBuildableId( const char *pszBuildableName );

const char *WeaponIdToAlias( ETFWeaponID iWeapon );
const char *WeaponIdToClassname( ETFWeaponID iWeapon );
bool ClassOwnsWeapon( int iClass, ETFWeaponID iWeapon );
const char *TranslateWeaponEntForClass( const char *pszName, int iClass );

bool WeaponID_IsSniperRifle( int iWeaponID );
bool WeaponID_IsLunchbox( int iWeaponID );

// There 3 projectile bases in TF2:
// 1) CTFBaseRocket: Flies in a straight line or with an arch, explodes on contact.
// 2) CTFWeaponBaseGrenadeProj: Physics object, explodes on timer.
// 3) CTFBaseProjectile: Used for nails, flies with an arch, does damage on contact.
// 4) CTFFlameEntity: This one kind of stands on its own, it's not actually a base but we're deriving it from CBaseProjectile for consistency.
// They all have different behavior and accessors so it's important to distinguish them easily.
enum
{
	TF_PROJECTILE_BASE_ROCKET,
	TF_PROJECTILE_BASE_GRENADE,
	TF_PROJECTILE_BASE_NAIL,
	TF_PROJECTILE_BASE_FLAME,
};

enum ProjectileType_t
{
	TF_PROJECTILE_NONE,
	TF_PROJECTILE_BULLET,
	TF_PROJECTILE_ROCKET,
	TF_PROJECTILE_PIPEBOMB,
	TF_PROJECTILE_PIPEBOMB_REMOTE,
	TF_PROJECTILE_SYRINGE,
	TF_PROJECTILE_FLARE,
	TF_PROJECTILE_JAR,
	TF_PROJECTILE_ARROW,
	TF_PROJECTILE_FLAME_ROCKET,
	TF_PROJECTILE_JAR_MILK,
	TF_PROJECTILE_HEALING_BOLT,
	TF_PROJECTILE_ENERGY_BALL,
	TF_PROJECTILE_ENERGY_RING,
	TF_PROJECTILE_PIPEBOMB_REMOTE_PRACTICE,
	TF_PROJECTILE_CLEAVER,
	TF_PROJECTILE_STICKY_BALL,
	TF_PROJECTILE_CANNONBALL,
	TF_PROJECTILE_BUILDING_REPAIR_BOLT,
	TF_PROJECTILE_FESTIVE_ARROW,
	TF_PROJECTILE_THROWABLE,
	TF_PROJECTILE_SPELLFIREBALL,
	TF_PROJECTILE_FESTIVE_URINE,
	TF_PROJECTILE_FESTIVE_HEALING_BOLT,
	TF_PROJECTILE_BREADMONSTER_JARATE,
	TF_PROJECTILE_BREADMONSTER_MADMILK,
	TF_PROJECTILE_GRAPPLINGHOOK,
	TF_PROJECTILE_SENTRY_ROCKET,
	TF_PROJECTILE_BREAD_MONSTER,

	// Add new projectiles here.
	TF_PROJECTILE_NAIL,
	TF_PROJECTILE_DART,
	TF_PROJECTILE_MIRV,
	TF_PROJECTILE_COIL,
	TF_PROJECTILE_BRICK,
	TF_PROJECTILE_THROWINGKNIFE,
	TF_PROJECTILE_PAINTBALL,
#ifdef TF2C_BETA
	TF_PROJECTILE_HEALGRENADEIMPACT,
	TF_PROJECTILE_UBERGENERATOR,
	TF_PROJECTILE_CYCLOPSGRENADE,
#endif
	TF_NUM_PROJECTILES
};

extern const char *g_szProjectileNames[];

enum
{
	TF_NAIL_SYRINGE,
	TF_NAIL_NORMAL,
	TF_NAIL_COUNT,
};

//-----------------------------------------------------------------------------
// TF Player Condition.
//-----------------------------------------------------------------------------

// Burning!
#define TF_BURNING_FREQUENCY		0.5f

// Bleeding!
#define TF_BLEEDING_FREQUENCY		0.5f
#define TF_BLEEDING_DMG				4

// Most of these conds aren't actually implemented but putting them here for compatibility.
enum ETFCond
{
	TF_COND_INVALID = -1,
	TF_COND_AIMING = 0,		// Sniper aiming, Heavy minigun.
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
	TF_COND_OFFENSEBUFF,
	TF_COND_SHIELD_CHARGE,
	TF_COND_DEMO_BUFF,
	TF_COND_ENERGY_BUFF,
	TF_COND_RADIUSHEAL,
	TF_COND_HEALTH_BUFF,
	TF_COND_BURNING,
	TF_COND_HEALTH_OVERHEALED,
	TF_COND_URINE,
	TF_COND_BLEEDING,
	TF_COND_DEFENSEBUFF,
	TF_COND_MAD_MILK,
	TF_COND_MEGAHEAL,
	TF_COND_REGENONDAMAGEBUFF,
	TF_COND_MARKEDFORDEATH,
	TF_COND_NOHEALINGDAMAGEBUFF,
	TF_COND_SPEED_BOOST,
	TF_COND_CRITBOOSTED_PUMPKIN,
	TF_COND_CRITBOOSTED_USER_BUFF,
	TF_COND_CRITBOOSTED_DEMO_CHARGE,
	TF_COND_SODAPOPPER_HYPE,
	TF_COND_CRITBOOSTED_FIRST_BLOOD,
	TF_COND_CRITBOOSTED_BONUS_TIME,
	TF_COND_CRITBOOSTED_CTF_CAPTURE,
	TF_COND_CRITBOOSTED_ON_KILL,
	TF_COND_CANNOT_SWITCH_FROM_MELEE,
	TF_COND_DEFENSEBUFF_NO_CRIT_BLOCK,
	TF_COND_REPROGRAMMED,
	TF_COND_CRITBOOSTED_RAGE_BUFF,
	TF_COND_DEFENSEBUFF_HIGH,
	TF_COND_SNIPERCHARGE_RAGE_BUFF,
	TF_COND_DISGUISE_WEARINGOFF,
	TF_COND_MARKEDFORDEATH_SILENT,
	TF_COND_DISGUISED_AS_DISPENSER,
	TF_COND_SAPPED,
	TF_COND_INVULNERABLE_HIDE_UNLESS_DAMAGED,
	TF_COND_INVULNERABLE_USER_BUFF,
	TF_COND_HALLOWEEN_BOMB_HEAD,
	TF_COND_HALLOWEEN_THRILLER,
	TF_COND_RADIUSHEAL_ON_DAMAGE,
	TF_COND_CRITBOOSTED_CARD_EFFECT,
	TF_COND_INVULNERABLE_CARD_EFFECT,
	TF_COND_MEDIGUN_UBER_BULLET_RESIST,
	TF_COND_MEDIGUN_UBER_BLAST_RESIST,
	TF_COND_MEDIGUN_UBER_FIRE_RESIST,
	TF_COND_MEDIGUN_SMALL_BULLET_RESIST,
	TF_COND_MEDIGUN_SMALL_BLAST_RESIST,
	TF_COND_MEDIGUN_SMALL_FIRE_RESIST,
	TF_COND_STEALTHED_USER_BUFF,
	TF_COND_MEDIGUN_DEBUFF,
	TF_COND_STEALTHED_USER_BUFF_FADING,
	TF_COND_BULLET_IMMUNE,
	TF_COND_BLAST_IMMUNE,
	TF_COND_FIRE_IMMUNE,
	TF_COND_PREVENT_DEATH,
	TF_COND_MVM_BOT_STUN_RADIOWAVE,
	TF_COND_HALLOWEEN_SPEED_BOOST,
	TF_COND_HALLOWEEN_QUICK_HEAL,
	TF_COND_HALLOWEEN_GIANT,
	TF_COND_HALLOWEEN_TINY,
	TF_COND_HALLOWEEN_IN_HELL,
	TF_COND_HALLOWEEN_GHOST_MODE,
	TF_COND_MINICRITBOOSTED_ON_KILL,
	TF_COND_OBSCURED_SMOKE,
	TF_COND_PARACHUTE_ACTIVE,
	TF_COND_BLASTJUMPING,
	TF_COND_HALLOWEEN_KART,
	TF_COND_HALLOWEEN_KART_DASH,
	TF_COND_BALLOON_HEAD,
	TF_COND_MELEE_ONLY,
	TF_COND_SWIMMING_CURSE,
	TF_COND_FREEZE_INPUT,
	TF_COND_HALLOWEEN_KART_CAGE,
	TF_COND_DONOTUSE_0,
	TF_COND_RUNE_STRENGTH,
	TF_COND_RUNE_HASTE,
	TF_COND_RUNE_REGEN,
	TF_COND_RUNE_RESIST,
	TF_COND_RUNE_VAMPIRE,
	TF_COND_RUNE_REFLECT,
	TF_COND_RUNE_PRECISION,
	TF_COND_RUNE_AGILITY,
	TF_COND_GRAPPLINGHOOK,
	TF_COND_GRAPPLINGHOOK_SAFEFALL,
	TF_COND_GRAPPLINGHOOK_LATCHED,
	TF_COND_GRAPPLINGHOOK_BLEEDING,
	TF_COND_AFTERBURN_IMMUNE,
	TF_COND_RUNE_KNOCKOUT,
	TF_COND_RUNE_IMBALANCE,
	TF_COND_CRITBOOSTED_RUNE_TEMP,
	TF_COND_PASSTIME_INTERCEPTION,
	TF_COND_SWIMMING_NO_EFFECTS,
	TF_COND_PURGATORY,
	// Adding missing condition IDs even if they aren't used here to keep compitability with Live IDs
	TF_COND_RUNE_KING,
	TF_COND_RUNE_PLAGUE,
	TF_COND_RUNE_SUPERNOVA,
	TF_COND_PLAGUE,
	TF_COND_KING_BUFFED,
	TF_COND_TEAM_GLOWS,
	TF_COND_KNOCKED_INTO_AIR,
	TF_COND_COMPETITIVE_WINNER,
	TF_COND_COMPETITIVE_LOSER,
	TF_COND_HEALING_DEBUFF,
	TF_COND_PASSTIME_PENALTY_DEBUFF,
	TF_COND_GRAPPLED_TO_PLAYER,
	TF_COND_GRAPPLED_BY_PLAYER,
	TF_COND_PARACHUTE_DEPLOYED,
	TF_COND_GAS,
	TF_COND_BURNING_PYRO,
	TF_COND_ROCKETPACK,
	TF_COND_LOST_FOOTING,
	TF_COND_AIR_CURRENT,
	TF_COND_HALLOWEEN_HELL_HEAL,
	TF_COND_POWERUPMODE_DOMINANT,
	// Add new conditions here!
	TF_COND_TRANQUILIZED,
	TF_COND_AIRBLASTED,
	TF_COND_RESISTANCE_BUFF,
	TF_COND_DAMAGE_BOOST,
	TF_COND_LAUNCHED,	// Used by jumppad to allow stomping and take no fall damage
	TF_COND_CRITBOOSTED_HIDDEN,
	TF_COND_SUPERMARKEDFORDEATH,	// These two are added for detonator. Marked for Death but you take crits from melee too.
	TF_COND_SUPERMARKEDFORDEATH_SILENT,
	TF_COND_JUST_USED_JUMPPAD,	// Used by jumppad to ban players from using jumppad again way too soon
	TF_COND_SPEEDBOOST_DETONATOR,	// Used by detonator only right now // !!! foxysen detonator
	TF_COND_INVULNERABLE_SMOKE_BOMB,	// !!! foxysen speedwatch
	TF_COND_CIV_DEFENSEBUFF,
	TF_COND_CIV_SPEEDBUFF,
	TF_COND_MARKED_OUTLINE,
	TF_COND_RADIAL_UBER,		// Used by the NN to apply multiple CONDs on uber
	TF_COND_DEFLECT_BULLETS,
	TF_COND_TAKEBONUSHEALING,	// Causes the afflicted player to take extra health when healed
	TF_COND_DID_DISGUISE_BREAKING_ACTION,		// So sentries can shoot people even with sentries_no_aggro attribute
	TF_COND_JUMPPAD_ASSIST,	// Used during and a bit after jumppad landing to give Engineer assist and for achievement
	TF_COND_MIRV_SLOW,	// Used by MIRV on impact damage to give a longer lasting slow to players
    TF_COND_TELEPORTED_ALWAYS_SHOW, // teleported effect but always show it
	TF_COND_LAUNCHED_SELF, // performing a tf_weapon_doubleshotgun jump via apply_self_knockback
	TF_COND_LAST
};

#define TF_COND_JUMPPAD_ASSIST_DURATION 4.0f


#define TF_COND_TAKEBONUSHEALING_MULTIPLIER 1.5f

extern ETFCond g_eAttributeConditions[];

extern ETFCond g_eDebuffConditions[];

extern ETFCond g_eAttackerAssistConditions[];
extern ETFCond g_eVictimAssistConditions[];

bool ConditionExpiresFast( ETFCond nCond );

//-----------------------------------------------------------------------------
// Mediguns.
//-----------------------------------------------------------------------------
enum
{
	TF_MEDIGUN_STOCK = 0,
	TF_MEDIGUN_KRITZKRIEG,
	/*TF_MEDIGUN_QUICKFIX,
	TF_MEDIGUN_VACCINATOR,
	TF_MEDIGUN_OVERHEALER,*/

	TF_MEDIGUN_COUNT
};

enum medigun_charge_types
{
	TF_CHARGE_NONE = -1,
	TF_CHARGE_INVULNERABLE = 0,
	TF_CHARGE_CRITBOOSTED,			// 1
	TF_CHARGE_PUSHBUFF,				// 2
	/*TF_CHARGE_MEGAHEAL,
	TF_CHARGE_BULLET_RESIST,
	TF_CHARGE_BLAST_RESIST,
	TF_CHARGE_FIRE_RESIST,*/

	TF_CHARGE_COUNT
};

// Healer type
enum HealerType {
	HEALER_TYPE_BEAM,		// Normal medigun type healing or ubering ; produces the pitch enveloped healsound
	HEALER_TYPE_BURST,		// 1-frame healers
	HEALER_TYPE_PASSIVE,	// Delivered as a passive effect with no input 
};

enum arrow_models
{
	MODEL_ARROW_REGULAR,

	TF_ARROW_MODEL_COUNT
};

extern const char *g_pszArrowModels[];

typedef struct
{
	ETFCond condition_enable;
	ETFCond condition_disable;
	const char *sound_enable;
	const char *sound_disable;
} MedigunEffects_t;

extern MedigunEffects_t g_MedigunEffects[];

//-----------------------------------------------------------------------------
// Class data
//-----------------------------------------------------------------------------
#define TF_REGEN_TIME			1.0f		// Number of seconds between each regen.
#define TF_REGEN_AMOUNT			3 			// Amount of health regenerated each Medic-regen.

//-----------------------------------------------------------------------------
// TF Player State.
//-----------------------------------------------------------------------------
enum 
{
	TF_STATE_ACTIVE = 0,		// Happily running around in the game.
	TF_STATE_WELCOME,			// First entering the server (shows level intro screen).
	TF_STATE_OBSERVER,			// Game observer mode.
	TF_STATE_DYING,				// Player is dying.
	TF_STATE_COUNT
};

//-----------------------------------------------------------------------------
// TF FlagInfo State.
//-----------------------------------------------------------------------------
enum
{
	TF_FLAGINFO_NONE = 0,
	TF_FLAGINFO_STOLEN,
	TF_FLAGINFO_DROPPED,
};

enum ETFFlagEventTypes
{
	TF_FLAGEVENT_PICKUP = 1,
	TF_FLAGEVENT_CAPTURE,
	TF_FLAGEVENT_DEFEND,
	TF_FLAGEVENT_DROPPED,
	TF_FLAGEVENT_RETURN,
};

//-----------------------------------------------------------------------------
// Assist-damage constants
//-----------------------------------------------------------------------------
#define TF_TIME_ASSIST_KILL				3.0f	// Time window for a recent damager to get credit for an assist for a kill.
#define TF_TIME_SUICIDE_KILL_CREDIT		10.0f	// Time window for a recent damager to get credit for a kill if target suicides.

//-----------------------------------------------------------------------------
// Domination/nemesis constants
//-----------------------------------------------------------------------------
#define TF_KILLS_DOMINATION				4		// # of unanswered kills to dominate another player.

//-----------------------------------------------------------------------------
// TF Hints
//-----------------------------------------------------------------------------
enum
{
	HINT_FRIEND_SEEN = 0,				// #Hint_spotted_a_friend
	HINT_ENEMY_SEEN,					// #Hint_spotted_an_enemy
	HINT_ENEMY_KILLED,					// #Hint_killing_enemies_is_good
	HINT_AMMO_EXHAUSTED,				// #Hint_out_of_ammo
	HINT_TURN_OFF_HINTS,				// #Hint_turn_off_hints
	HINT_PICKUP_AMMO,					// #Hint_pickup_ammo
	HINT_CANNOT_TELE_WITH_FLAG,			// #Hint_Cannot_Teleport_With_Flag
	HINT_CANNOT_CLOAK_WITH_FLAG,		// #Hint_Cannot_Cloak_With_Flag
	HINT_CANNOT_DISGUISE_WITH_FLAG,		// #Hint_Cannot_Disguise_With_Flag
	HINT_CANNOT_ATTACK_WHILE_CLOAKED,	// #Hint_Cannot_Attack_While_Cloaked
	HINT_CLASSMENU,						// #Hint_ClassMenu

	// Grenades.
	HINT_GREN_CALTROPS,					// #Hint_gren_caltrops
	HINT_GREN_CONCUSSION,				// #Hint_gren_concussion
	HINT_GREN_EMP,						// #Hint_gren_emp
	HINT_GREN_GAS,						// #Hint_gren_gas
	HINT_GREN_MIRV,						// #Hint_gren_mirv
	HINT_GREN_NAIL,						// #Hint_gren_nail
	HINT_GREN_NAPALM,					// #Hint_gren_napalm
	HINT_GREN_NORMAL,					// #Hint_gren_normal

	// Weapon alt-fires.
	HINT_ALTFIRE_SNIPERRIFLE,			// #Hint_altfire_sniperrifle
	HINT_ALTFIRE_FLAMETHROWER,			// #Hint_altfire_flamethrower
	HINT_ALTFIRE_GRENADELAUNCHER,		// #Hint_altfire_grenadelauncher
	HINT_ALTFIRE_PIPEBOMBLAUNCHER,		// #Hint_altfire_pipebomblauncher
	HINT_ALTFIRE_ROTATE_BUILDING,		// #Hint_altfire_rotate_building

	// Class specific.
	// Soldier
	HINT_SOLDIER_RPG_RELOAD,			// #Hint_Soldier_rpg_reload

	// Engineer
	HINT_ENGINEER_USE_WRENCH_ONOWN,		// "#Hint_Engineer_use_wrench_onown",
	HINT_ENGINEER_USE_WRENCH_ONOTHER,	// "#Hint_Engineer_use_wrench_onother",
	HINT_ENGINEER_USE_WRENCH_FRIEND,	// "#Hint_Engineer_use_wrench_onfriend",
	HINT_ENGINEER_BUILD_SENTRYGUN,		// "#Hint_Engineer_build_sentrygun"
	HINT_ENGINEER_BUILD_DISPENSER,		// "#Hint_Engineer_build_dispenser"
	HINT_ENGINEER_BUILD_TELEPORTERS,	// "#Hint_Engineer_build_teleporters"
	HINT_ENGINEER_PICKUP_METAL,			// "#Hint_Engineer_pickup_metal"
	HINT_ENGINEER_REPAIR_OBJECT,		// "#Hint_Engineer_repair_object"
	HINT_ENGINEER_METAL_TO_UPGRADE,		// "#Hint_Engineer_metal_to_upgrade"
	HINT_ENGINEER_UPGRADE_SENTRYGUN,	// "#Hint_Engineer_upgrade_sentrygun"

	HINT_OBJECT_HAS_SAPPER,				// "#Hint_object_has_sapper"

	HINT_OBJECT_YOUR_OBJECT_SAPPED,		// "#Hint_object_your_object_sapped"
	HINT_OBJECT_ENEMY_USING_DISPENSER,	// "#Hint_enemy_using_dispenser"
	HINT_OBJECT_ENEMY_USING_TP_ENTRANCE,// "#Hint_enemy_using_tp_entrance"
	HINT_OBJECT_ENEMY_USING_TP_EXIT,	// "#Hint_enemy_using_tp_exit"

	NUM_HINTS
};
extern const char *g_pszHintMessages[];

//--------------
// TF Specific damage flags
//--------------
//#define DMG_UNUSED				( DMG_LASTGENERICFLAG << 2 )
// We can't add anymore dmg flags, because we'd be over the 32 bit limit.
// So lets re-use some of the old dmg flags in TF
#define DMG_USE_HITLOCATIONS	( DMG_AIRBOAT )
#define DMG_HALF_FALLOFF		( DMG_RADIATION )
#define DMG_CRITICAL			( DMG_ACID )
#define DMG_RADIUS_MAX			( DMG_ENERGYBEAM )
#define DMG_IGNITE				( DMG_PLASMA )
#define DMG_USEDISTANCEMOD		( DMG_SLOWBURN )
#define DMG_NOCLOSEDISTANCEMOD	( DMG_POISON )
#define DMG_TRAIN				( DMG_VEHICLE )
#define DMG_SAWBLADE			( DMG_NERVEGAS )

#define TF_DMG_SENTINEL_VALUE	INT_MAX

// This can only ever be used on a TakeHealth call, since it re-uses a dmg flag that means something else.
#define HEAL_IGNORE_MAXHEALTH	( 1 << 1 )
#define HEAL_NOTIFY				( 1 << 2 )
#define HEAL_MAXBUFFCAP			( 1 << 3 )

// Special Damage types
enum ETFDmgCustom
{
	TF_DMG_CUSTOM_NONE = 0,
	TF_DMG_CUSTOM_HEADSHOT,
	TF_DMG_CUSTOM_BACKSTAB,
	TF_DMG_CUSTOM_BURNING,
	TF_DMG_WRENCH_FIX,
	TF_DMG_CUSTOM_MINIGUN,
	TF_DMG_CUSTOM_SUICIDE,
	TF_DMG_CUSTOM_TAUNTATK_HADOUKEN,
	TF_DMG_CUSTOM_BURNING_FLARE,
	TF_DMG_CUSTOM_TAUNTATK_HIGH_NOON,
	TF_DMG_CUSTOM_TAUNTATK_GRAND_SLAM,
	TF_DMG_CUSTOM_PENETRATE_MY_TEAM,
	TF_DMG_CUSTOM_PENETRATE_ALL_PLAYERS,
	TF_DMG_CUSTOM_TAUNTATK_FENCING,
	TF_DMG_CUSTOM_PENETRATE_NONBURNING_TEAMMATE,
	TF_DMG_CUSTOM_TAUNTATK_ARROW_STAB,
	TF_DMG_CUSTOM_TELEFRAG,
	TF_DMG_CUSTOM_BURNING_ARROW,
	TF_DMG_CUSTOM_FLYINGBURN,
	TF_DMG_CUSTOM_PUMPKIN_BOMB,
	TF_DMG_CUSTOM_DECAPITATION,
	TF_DMG_CUSTOM_TAUNTATK_GRENADE,
	TF_DMG_CUSTOM_BASEBALL,
	TF_DMG_CUSTOM_CHARGE_IMPACT,
	TF_DMG_CUSTOM_TAUNTATK_BARBARIAN_SWING,
	TF_DMG_CUSTOM_AIR_STICKY_BURST,
	TF_DMG_CUSTOM_DEFENSIVE_STICKY,
	TF_DMG_CUSTOM_PICKAXE,
	TF_DMG_CUSTOM_ROCKET_DIRECTHIT,
	TF_DMG_CUSTOM_TAUNTATK_UBERSLICE,
	TF_DMG_CUSTOM_PLAYER_SENTRY,
	TF_DMG_CUSTOM_STANDARD_STICKY,
	TF_DMG_CUSTOM_SHOTGUN_REVENGE_CRIT,
	TF_DMG_CUSTOM_TAUNTATK_ENGINEER_GUITAR_SMASH,
	TF_DMG_CUSTOM_BLEEDING,
	TF_DMG_CUSTOM_GOLD_WRENCH,
	TF_DMG_CUSTOM_CARRIED_BUILDING,
	TF_DMG_CUSTOM_COMBO_PUNCH,
	TF_DMG_CUSTOM_TAUNTATK_ENGINEER_ARM_KILL,
	TF_DMG_CUSTOM_FISH_KILL,
	TF_DMG_CUSTOM_TRIGGER_HURT,
	TF_DMG_CUSTOM_DECAPITATION_BOSS,
	TF_DMG_CUSTOM_STICKBOMB_EXPLOSION,
	TF_DMG_CUSTOM_AEGIS_ROUND,
	TF_DMG_CUSTOM_FLARE_EXPLOSION,
	TF_DMG_CUSTOM_BOOTS_STOMP,
	TF_DMG_CUSTOM_PLASMA,
	TF_DMG_CUSTOM_PLASMA_CHARGED,
	TF_DMG_CUSTOM_PLASMA_GIB,
	TF_DMG_CUSTOM_PRACTICE_STICKY,
	TF_DMG_CUSTOM_EYEBALL_ROCKET,
	TF_DMG_CUSTOM_HEADSHOT_DECAPITATION,
	TF_DMG_CUSTOM_TAUNTATK_ARMAGEDDON,
	TF_DMG_CUSTOM_FLARE_PELLET,
	TF_DMG_CUSTOM_CLEAVER,
	TF_DMG_CUSTOM_CLEAVER_CRIT,
	TF_DMG_CUSTOM_SAPPER_RECORDER_DEATH,
	TF_DMG_CUSTOM_MERASMUS_PLAYER_BOMB,
	TF_DMG_CUSTOM_MERASMUS_GRENADE,
	TF_DMG_CUSTOM_MERASMUS_ZAP,
	TF_DMG_CUSTOM_MERASMUS_DECAPITATION,
	TF_DMG_CUSTOM_CANNONBALL_PUSH,
	TF_DMG_CUSTOM_TAUNTATK_ALLCLASS_GUITAR_RIFF,
	TF_DMG_CUSTOM_THROWABLE,
	TF_DMG_CUSTOM_THROWABLE_KILL,
	TF_DMG_CUSTOM_SPELL_TELEPORT,
	TF_DMG_CUSTOM_SPELL_SKELETON,
	TF_DMG_CUSTOM_SPELL_MIRV,
	TF_DMG_CUSTOM_SPELL_METEOR,
	TF_DMG_CUSTOM_SPELL_LIGHTNING,
	TF_DMG_CUSTOM_SPELL_FIREBALL,
	TF_DMG_CUSTOM_SPELL_MONOCULUS,
	TF_DMG_CUSTOM_SPELL_BLASTJUMP,
	TF_DMG_CUSTOM_SPELL_BATS,
	TF_DMG_CUSTOM_SPELL_TINY,
	TF_DMG_CUSTOM_KART,
	TF_DMG_CUSTOM_GIANT_HAMMER,
	TF_DMG_CUSTOM_RUNE_REFLECT,
	TF_DMG_CUSTOM_DRAGONS_FURY_IGNITE,
	TF_DMG_CUSTOM_DRAGONS_FURY_BONUS_BURNING,
	TF_DMG_CUSTOM_SLAP_KILL,
	TF_DMG_CUSTOM_CROC,
	TF_DMG_CUSTOM_TAUNTATK_GASBLAST,
	TF_DMG_CUSTOM_AXTINGUISHER_BOOSTED,

	// TF2C dmg customs start here
	// -------------------------------------
	TF_DMG_BUILDING_CARRIED,
	TF_DMG_CUSTOM_TAUNTATK_MIRV,
	TF_DMG_CUSTOM_SUICIDE_DISINTEGRATE,
	TF_DMG_CUSTOM_BUDDHA,	// Damage that can't kill anyone, like a buddha cheatcode
	TF_DMG_CUSTOM_JUMPPAD_STOMP,
	TF_DMG_FALL_DAMAGE,	// Used to tell away fall damage that was done by actually falling from the one done by level triggers. Used to give fall damage immunity without breaking death pits.
	TF_DMG_EARTHQUAKE,
	TF_DMG_CUSTOM_GIB,
	TF_DMG_ANCHOR_AIRSHOTCRIT,
	TF_DMG_CYCLOPS_COMBO_MIRV,
	TF_DMG_CYCLOPS_COMBO_MIRV_BOMBLET,
	TF_DMG_CYCLOPS_COMBO_STICKYBOMB,
	TF_DMG_CYCLOPS_COMBO_PROXYMINE,
	TF_DMG_CYCLOPS_DELAYED,
	TF_DMG_CUSTOM_SUICIDE_BOIOING,
	TF_DMG_CUSTOM_SUICIDE_STOMP,
	TF_DMG_MIRV_DIRECT_HIT,

	TF_DMG_CUSTOM_END
};

inline bool IsDOTDmg(const CTakeDamageInfo& info)
{
	return (info.GetDamageType() & DMG_BURN || info.GetDamageCustom() == TF_DMG_CUSTOM_BLEEDING);
}

inline bool IsTauntKillDmg(int iType)
{
	if (iType == TF_DMG_CUSTOM_TAUNTATK_GRAND_SLAM ||
		iType == TF_DMG_CUSTOM_TAUNTATK_HIGH_NOON ||
		iType == TF_DMG_CUSTOM_TAUNTATK_FENCING ||
		iType == TF_DMG_CUSTOM_TAUNTATK_ARROW_STAB ||
		iType == TF_DMG_CUSTOM_TAUNTATK_UBERSLICE ||
		iType == TF_DMG_CUSTOM_TAUNTATK_HADOUKEN ||
		iType == TF_DMG_CUSTOM_TAUNTATK_MIRV ||
		iType == TF_DMG_CUSTOM_TELEFRAG) // function right now is used only to skip damage modifiers, so added TELEGRAG for simplicity
		return true;

	return false;
}

enum
{
	TF_COLLISIONGROUP_GRENADES = LAST_SHARED_COLLISION_GROUP,
	TFCOLLISION_GROUP_OBJECT,
	TFCOLLISION_GROUP_OBJECT_SOLIDTOPLAYERMOVEMENT,
	TFCOLLISION_GROUP_COMBATOBJECT,
	TFCOLLISION_GROUP_ROCKETS,			// Solid to players, but not player movement. ensures touch calls are originating from rocket.
	TFCOLLISION_GROUP_RESPAWNROOMS,
	TFCOLLISION_GROUP_PUMPKINBOMBS,
	TFCOLLISION_GROUP_ROCKETS_NOTSOLID, // Same as TFCOLLISION_GROUP_ROCKETS but not solid to other rockets.
};

// Stun flags!
#define TF_STUN_NONE						0
#define TF_STUN_MOVEMENT					( 1 << 0 )
#define	TF_STUN_CONTROLS					( 1 << 1 )
#define TF_STUN_MOVEMENT_FORWARD_ONLY		( 1 << 2 )
#define TF_STUN_SPECIAL_SOUND				( 1 << 3 )
#define TF_STUN_DODGE_COOLDOWN				( 1 << 4 )
#define TF_STUN_NO_EFFECTS					( 1 << 5 )
#define TF_STUN_LOSER_STATE					( 1 << 6 )
#define TF_STUN_BY_TRIGGER					( 1 << 7 )
#define TF_STUN_BOTH						TF_STUN_MOVEMENT | TF_STUN_CONTROLS

enum EAttackBonusEffects_t
{
	kBonusEffect_None = 4,	// Must be 4, eeyup.
	kBonusEffect_Crit = 0,
	kBonusEffect_MiniCrit,

	kBonusEffect_Count,		// Must be 2nd to last
	
};

// Generalized Jump State.
#define TF_PLAYER_ROCKET_JUMPED		( 1 << 0 )
#define TF_PLAYER_STICKY_JUMPED		( 1 << 1 )
#define TF_PLAYER_ENEMY_BLASTED_ME	( 1 << 2 )

//--------------------------------------------------------------------------
// OBJECTS
//--------------------------------------------------------------------------
enum
{
	OBJ_DISPENSER = 0,
	OBJ_TELEPORTER,
	OBJ_SENTRYGUN,

	// Attachment objects.
	OBJ_ATTACHMENT_SAPPER,

	// TF2C objects
	OBJ_JUMPPAD,
	OBJ_MINIDISPENSER,
	OBJ_FLAMESENTRY,

	// If you add a new object, you need to add it to the g_ObjectInfos array 
	// in tf_shareddefs.cpp, and add it's data to the scripts/object.txt!
	OBJ_LAST,
};

#define OBJECT_MODE_NONE			0
#define TELEPORTER_TYPE_ENTRANCE	0
#define TELEPORTER_TYPE_EXIT		1
#define JUMPPAD_TYPE_A				0
#define JUMPPAD_TYPE_B				1

//--------------
// Scoring
//--------------

#define TF_SCORE_KILL							1
#define TF_SCORE_DEATH							0
#define TF_SCORE_CAPTURE						2
#define TF_SCORE_DEFEND							1
#define TF_SCORE_DESTROY_BUILDING				1
#define TF_SCORE_HEADSHOTS_PER_POINT			2
#define TF_SCORE_BACKSTAB						1
#define TF_SCORE_INVULN							1
#define TF_SCORE_REVENGE						1
#define TF_SCORE_KILL_ASSISTS_PER_POINT			2
#define TF_SCORE_TELEPORTS_PER_POINT			2	
#define TF_SCORE_HEAL_HEALTHUNITS_PER_POINT		600
#define TF_SCORE_DAMAGE_PER_POINT				600
#define TF_SCORE_BONUS_PER_POINT				4
#define TF_SCORE_JUMPPAD_JUMPS_PER_POINT		4	
#define TF_SCORE_DAMAGE_ASSIST_PER_POINT		400
#define TF_SCORE_DAMAGE_BLOCKED_PER_POINT		400

// Build animation events.
#define TF_OBJ_ENABLEBODYGROUP			6000
#define TF_OBJ_DISABLEBODYGROUP			6001
#define TF_OBJ_ENABLEALLBODYGROUPS		6002
#define TF_OBJ_DISABLEALLBODYGROUPS		6003
#define TF_OBJ_PLAYBUILDSOUND			6004

#define TF_AE_CIGARETTE_THROW			7000
#define TF_AE_FOOTSTEP					7001
#define TF_AE_WPN_EJECTBRASS			6002


const char *GetRandomBotName();
const char *GetSeededRandomBotName( int iSeed );


class CHudTexture;

class CObjectInfo
{
public:
	CObjectInfo( const char *pObjectName );
	CObjectInfo( const CObjectInfo& obj ) {}
	~CObjectInfo();

	// This is initialized by the code and matched with a section in objects.txt.
	const char	*m_pObjectName;

	// This stuff all comes from objects.txt.
	char	*m_pClassName;					// Code classname (in LINK_ENTITY_TO_CLASS).
	char	*m_pStatusName;					// Shows up when crosshairs are on the object.
	float	m_flBuildTime;
	int		m_nMaxObjects;					// Maximum number of objects per player.
	int		m_Cost;							// Base object resource cost.
	float	m_CostMultiplierPerInstance;	// Cost multiplier.
	int		m_UpgradeCost;					// Base object resource cost for upgrading.
	float	m_flUpgradeDuration;
	int		m_MaxUpgradeLevel;				// Max object upgrade level.
	char	*m_pBuilderWeaponName;			// Names shown for each object onscreen when using the builder weapon.
	char	*m_pBuilderPlacementString;		// String shown to player during placement of this object.
	int		m_SelectionSlot;				// Weapon selection slots for objects.
	int		m_SelectionPosition;			// Weapon selection positions for objects.
	bool	m_bSolidToPlayerMovement;
	bool	m_bUseItemInfo;
	char    *m_pViewModel;					// View model to show in builder weapon for this object.
	char    *m_pPlayerModel;				// World model to show attached to the player.
	int		m_iDisplayPriority;				// Priority for ordering in the hud display (higher is closer to top).
	bool	m_bVisibleInWeaponSelection;	// should show up and be selectable via the weapon selection?
	char	*m_pExplodeSound;				// gamesound to play when object explodes.
	char	*m_pExplosionParticleEffect;	// particle effect to play when object explodes.
	bool	m_bAutoSwitchTo;				// should we let players switch back to the builder weapon representing this?
	char	*m_pUpgradeSound;				// gamesound to play when upgrading.
	int		m_BuildCount;					// ???
	bool	m_bRequiresOwnBuilder;			// ???

	CUtlVector<const char *> m_AltModes;

	// HUD weapon selection menu icon (from hud_textures.txt).
	char	*m_pIconActive;
	char	*m_pIconInactive;
	char	*m_pIconMenu;

	// HUD building status icon.
	char	*m_pHudStatusIcon;

	// GIBS.
	int		m_iMetalToDropInGibs;
};

// Loads the objects.txt script.
class IBaseFileSystem;
void LoadObjectInfos( IBaseFileSystem *pFileSystem );

// Get a CObjectInfo from a TFOBJ_ define.
const CObjectInfo *GetObjectInfo( int iObject );

const unsigned char *GetTFEncryptionKey( void );

// Win panel styles.
enum
{
	WINPANEL_BASIC = 0,
};

#define TF_DEATH_ANIMATION_TIME 2.0f

// Taunt attack types.
enum
{
	TAUNTATK_NONE,
	TAUNTATK_PYRO_HADOUKEN,
	TAUNTATK_HEAVY_EAT,
	TAUNTATK_HEAVY_RADIAL_BUFF,
	TAUNTATK_HEAVY_HIGH_NOON,
	TAUNTATK_SCOUT_DRINK,
	TAUNTATK_SCOUT_GRAND_SLAM,
	TAUNTATK_MEDIC_INHALE,
	TAUNTATK_SPY_FENCING_SLASH_A,
	TAUNTATK_SPY_FENCING_SLASH_B,
	TAUNTATK_SPY_FENCING_STAB,
	TAUNTATK_RPS_KILL,
	TAUNTATK_SNIPER_ARROW_STAB_IMPALE,
	TAUNTATK_SNIPER_ARROW_STAB_KILL,
	TAUNTATK_SOLDIER_GRENADE_KILL,
	TAUNTATK_DEMOMAN_BARBARIAN_SWING,
	TAUNTATK_MEDIC_UBERSLICE_IMPALE,
	TAUNTATK_MEDIC_UBERSLICE_KILL,
	TAUNTATK_FLIP_LAND_PARTICLE,
	TAUNTATK_RPS_PARTICLE,
	TAUNTATK_HIGHFIVE_PARTICLE,
	TAUNTATK_ENGINEER_GUITAR_SMASH,
	TAUNTATK_ENGINEER_ARM_IMPALE,
	TAUNTATK_ENGINEER_ARM_KILL,
	TAUNTATK_ENGINEER_ARM_BLEND,
	TAUNTATK_SOLDIER_GRENADE_KILL_WORMSIGN,
	TAUNTATK_SHOW_ITEM,
	TAUNTATK_MEDIC_RELEASE_DOVES,
	TAUNTATK_PYRO_ARMAGEDDON,
	TAUNTATK_PYRO_SCORCHSHOT,
	TAUNTATK_ALLCLASS_GUITAR_RIFF,
	TAUNTATK_MEDIC_HEROIC_TAUNT,

	// TF2c Specific ones go here
	TAUNTATK_DEMOMAN_MIRV_KILL,

	// Add new taunt attacks here.
	TAUNTATK_COUNT
};

extern const char *taunt_attack_name[TAUNTATK_COUNT];
int GetTauntAttackByName( const char *pszName );

typedef enum
{
	HUD_NOTIFY_YOUR_FLAG_TAKEN,
	HUD_NOTIFY_YOUR_FLAG_DROPPED,
	HUD_NOTIFY_YOUR_FLAG_RETURNED,
	HUD_NOTIFY_YOUR_FLAG_CAPTURED,

	HUD_NOTIFY_ENEMY_FLAG_TAKEN,
	HUD_NOTIFY_ENEMY_FLAG_DROPPED,
	HUD_NOTIFY_ENEMY_FLAG_RETURNED,
	HUD_NOTIFY_ENEMY_FLAG_CAPTURED,

	HUD_NOTIFY_TOUCHING_ENEMY_CTF_CAP,

	HUD_NOTIFY_NO_INVULN_WITH_FLAG,
	HUD_NOTIFY_NO_TELE_WITH_FLAG,

	HUD_NOTIFY_SPECIAL,

	// Add new notifications here.
	HUD_NOTIFY_ENEMY_GOT_UBER,	//before or after NUM_STOCK_NOTIFICATIONS which isn't used anywhere? !!! foxysen uber
	HUD_NOTIFY_NEUTRAL_FLAG_TAKEN,
	HUD_NOTIFY_NEUTRAL_FLAG_DROPPED,
	HUD_NOTIFY_NEUTRAL_FLAG_RETURNED,
	HUD_NOTIFY_NEUTRAL_FLAG_CAPTURED,
	HUD_NOTIFY_FLAG_CANNOT_CAPTURE,
	NUM_STOCK_NOTIFICATIONS
} HudNotification_t;

typedef enum
{
	SAPPING_NONE,
	SAPPING_PLACED,
	SAPPING_DONE
} sap_state_t;

#define TF_TRAIN_MAX_HILLS			5
#define TF_TRAIN_FLOATS_PER_HILL	2
#define TF_TRAIN_HILLS_ARRAY_SIZE	TF_TEAM_COUNT * TF_TRAIN_MAX_HILLS * TF_TRAIN_FLOATS_PER_HILL

#define TF_DEATH_DOMINATION				( 1 << 0 )	// Killer is dominating victim.
#define TF_DEATH_ASSISTER_DOMINATION	( 1 << 1 )	// Assister is dominating victim.
#define TF_DEATH_REVENGE				( 1 << 2 )	// Killer got revenge on victim.
#define TF_DEATH_ASSISTER_REVENGE		( 1 << 3 )	// Assister got revenge on victim.

// Unused death flags.
#define TF_DEATH_FIRST_BLOOD			( 1 << 4 )
#define TF_DEATH_FEIGN_DEATH			( 1 << 5 )
#define TF_DEATH_UNKNOWN				( 1 << 6 )	// Seemingly unused in Live TF2.
#define TF_DEATH_GIB					( 1 << 7 )
#define TF_DEATH_PURGATORY				( 1 << 8 )
#define TF_DEATH_MINIBOSS				( 1 << 9 )
#define TF_DEATH_AUSTRALIUM				( 1 << 10 )

// Ragdolls flags.
#define TF_RAGDOLL_GIB					( 1 << 0 )
#define TF_RAGDOLL_BURNING				( 1 << 1 )
#define TF_RAGDOLL_ONGROUND				( 1 << 2 )
#define TF_RAGDOLL_CHARGED				( 1 << 3 )
#define TF_RAGDOLL_ASH					( 1 << 4 )

#define HUD_ALERT_SCRAMBLE_TEAMS 0

// Custom contents mask used for tracing bullets in TF2.
#define MASK_TFSHOT ( MASK_SOLID | CONTENTS_HITBOX )

// Custom TF2 material indexes.
// If you add a new material here be sure to add its impact effect in tf_fx_impacts.cpp.
#define CHAR_TEX_TFSNOW 91

enum
{
	TF_ARENA_MESSAGE_NONE = -1,
	TF_ARENA_MESSAGE_CAREFUL = 0,
	TF_ARENA_MESSAGE_SITOUT,
	TF_ARENA_MESSAGE_NOPLAYERS,
};

enum
{
	TF_SCRAMBLEMODE_SCORETIME_RATIO = 0,
	TF_SCRAMBLEMODE_KILLDEATH_RATIO,
	TF_SCRAMBLEMODE_SCORE,
	TF_SCRAMBLEMODE_CLASS,
	TF_SCRAMBLEMODE_RANDOM,
	TF_SCRAMBLEMODE_COUNT
};

// Player attached models.
#define TF_SPY_MASK_MODEL "models/player/items/spy_mask.mdl"

#define TF_DISGUISE_TARGET_INDEX_NONE	( MAX_PLAYERS + 1 )
#define TF_PLAYER_INDEX_NONE			( MAX_PLAYERS + 1 )

#define TF_CAMERA_DIST 80
#define TF_CAMERA_DIST_RIGHT 20
#define TF_CAMERA_DIST_UP 0

#define IN_THIRDPERSON	( 1 << 30 )
#define IN_TYPING		( 1 << 31 )

const char *COM_GetModDirectory();

#ifdef STAGING_ONLY
#define TF2C_GAME_DIR "tf2classic-beta"
#else
#define TF2C_GAME_DIR COM_GetModDirectory()
#endif

// Agrimar: Putting this here so it could maybe help us clean up the DataTable code a tad in the future.
#ifdef CLIENT_DLL
#define BeginTable_NoBase( className, tableName ) BEGIN_RECV_TABLE_NOBASE( className, tableName )
#define EndTable() END_RECV_TABLE()
#define ReferenceTable( datatable ) REFERENCE_RECV_TABLE( datatable )
#else
#define BeginTable_NoBase( className, tableName ) BEGIN_SEND_TABLE_NOBASE( className, tableName )
#define EndTable() END_SEND_TABLE()
#define ReferenceTable( datatable ) REFERENCE_SEND_TABLE( datatable )
#endif
#endif // TF_SHAREDDEFS_H
