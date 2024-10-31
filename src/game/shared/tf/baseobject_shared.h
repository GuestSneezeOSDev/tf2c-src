//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef BASEOBJECT_SHARED_H
#define BASEOBJECT_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_shareddefs.h"

#if defined( CLIENT_DLL )
#define CBaseObject C_BaseObject
#endif

class CBaseObject;
typedef CHandle<CBaseObject>	ObjectHandle;
struct BuildPoint_t
{
	// If this is true, then objects are parented to the attachment point instead of 
	// parented to the entity's abs origin + angles. That way, they'll move if the 
	// attachment point animates.
	bool			m_bPutInAttachmentSpace;	

	int				m_iAttachmentNum;
	ObjectHandle	m_hObject;
	bool			m_bValidObjects[ OBJ_LAST ];
};

#define TF_OBJ_GROUND_CLEARANCE	32

// Max number of objects a team can have
#define MAX_OBJECTS_PER_PLAYER	4
//#define MAX_OBJECTS_PER_TEAM	128

//--------------------------------------------------------------------------
// OBJECT FLAGS
//--------------------------------------------------------------------------
enum
{
	OF_ALLOW_REPEAT_PLACEMENT = 0x01,
	OF_MUST_BE_BUILT_ON_ATTACHMENT = 0x02,
	OF_IS_CART_OBJECT = 0x04, //I'm not sure what the exact name is, but live tf2 uses it for the payload bomb dispenser object

	OF_BIT_COUNT = 4
};

//--------------------------------------------------------------------------
// Builder "weapon" states
//--------------------------------------------------------------------------
enum
{
	BS_IDLE = 0,
	BS_SELECTING,
	BS_PLACING,
	BS_PLACING_INVALID
};

//--------------------------------------------------------------------------
// BUILDING
//--------------------------------------------------------------------------
// Build checks will return one of these for a player
enum
{
	CB_CAN_BUILD,			// Player is allowed to build this object
	CB_CANNOT_BUILD,		// Player is not allowed to build this object
	CB_LIMIT_REACHED,		// Player has reached the limit of the number of these objects allowed
	CB_NEED_RESOURCES,		// Player doesn't have enough resources to build this object
	CB_NEED_ADRENALIN,		// Commando doesn't have enough adrenalin to build a rally flag
	CB_UNKNOWN_OBJECT,		// Error message, tried to build unknown object
};

//-------------------------
// Shared Sentry State
//-------------------------
enum
{
	SENTRY_STATE_INACTIVE = 0,
	SENTRY_STATE_SEARCHING,
	SENTRY_STATE_ATTACKING,
	SENTRY_STATE_UPGRADING,

	SENTRY_NUM_STATES,
};

#define SENTRYGUN_EYE_OFFSET_LEVEL_1	Vector( 0, 0, 32 )
#define SENTRYGUN_EYE_OFFSET_LEVEL_2	Vector( 0, 0, 40 )
#define SENTRYGUN_EYE_OFFSET_LEVEL_3	Vector( 0, 0, 46 )
#define SENTRYGUN_MAX_SHELLS_1			150
#define SENTRYGUN_MAX_SHELLS_2			200
#define SENTRYGUN_MAX_SHELLS_3			200
#define SENTRYGUN_MAX_ROCKETS			20
#define SENTRYGUN_MAX_RANGE				1100.0f

// Dispenser's maximum carrying capability
#define DISPENSER_MAX_METAL_AMMO 400

//-------------------------
// Shared Teleporter State
//-------------------------
enum
{
	TELEPORTER_STATE_BUILDING = 0,				// Building, not active yet
	TELEPORTER_STATE_IDLE,						// Does not have a matching teleporter yet
	TELEPORTER_STATE_READY,						// Found match, charged and ready
	TELEPORTER_STATE_SENDING,					// Teleporting a player away
	TELEPORTER_STATE_RECEIVING,
	TELEPORTER_STATE_RECEIVING_RELEASE,
	TELEPORTER_STATE_RECHARGING,				// Waiting for recharge
	TELEPORTER_STATE_UPGRADING
};

//-------------------------
// Shared Jumppad State
//-------------------------
enum
{
	JUMPPAD_STATE_INACTIVE = 0,				// Inactive
	JUMPPAD_STATE_READY,					// charged and ready
	JUMPPAD_STATE_UNDERWATER,				// disabled by being underwater
};

extern float g_flTeleporterRechargeTimes[];
extern float g_flDispenserAmmoRates[];
extern float g_flDispenserHealRates[];

// Shared header file for players
#if defined( CLIENT_DLL )
#include "c_baseobject.h"
#else
#include "tf_obj.h"
#endif

#endif // BASEOBJECT_SHARED_H
