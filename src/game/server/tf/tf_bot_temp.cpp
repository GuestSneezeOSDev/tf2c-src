//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Basic BOT handling.
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "player.h"
#include "tf_player.h"
#include "tf_gamerules.h"
#include "in_buttons.h"
#include "movehelper_server.h"
#include "tf_playerclass_shared.h"
#include "datacache/imdlcache.h"
#include "tf_team.h"

void ClientPutInServer( edict_t *pEdict, const char *playername );
void Bot_Think( CTFPlayer *pBot );

ConVar bot_forcefireweapon( "bot_forcefireweapon", "", 0, "Force bots with the specified weapon to fire." );
ConVar bot_forceattack2( "bot_forceattack2", "0", 0, "When firing, use attack2." );
ConVar bot_forceattackon( "bot_forceattackon", "1", 0, "When firing, don't tap fire, hold it down." );
ConVar bot_flipout( "bot_flipout", "0", 0, "When on, all bots fire their guns." );
ConVar bot_defend( "bot_defend", "0", 0, "Set to a team number, and that team will all keep their combat shields raised." );
ConVar bot_changeclass( "bot_changeclass", "0", 0, "Force all bots to change to the specified class." );
ConVar bot_dontmove( "bot_dontmove", "1" );
ConVar bot_saveme( "bot_saveme", "0", FCVAR_CHEAT );
static ConVar bot_mimic( "bot_mimic", "0", 0, "Bot uses usercmd of player by index." );
static ConVar bot_mimic_yaw_offset( "bot_mimic_yaw_offset", "180", 0, "Offsets the bot yaw." );
ConVar bot_selectweaponslot( "bot_selectweaponslot", "-1", FCVAR_CHEAT, "set to weapon slot that bot should switch to." );
ConVar bot_randomnames( "bot_randomnames", "0", FCVAR_CHEAT );
ConVar bot_jump( "bot_jump", "0", FCVAR_CHEAT, "Force all bots to repeatedly jump." );

static int BotNumber = 1;

typedef struct
{
	bool			backwards;

	float			nextturntime;
	bool			lastturntoright;

	float			nextstrafetime;
	float			sidemove;

	QAngle			forwardAngle;
	QAngle			lastAngles;
	
	float			m_flJoinTeamTime;
	int				m_WantedTeam;
	int				m_WantedClass;

	bool m_bWasDead;
	float m_flDeadTime;
} botdata_t;

static botdata_t g_BotData[ MAX_PLAYERS ];

static const char *g_aBotNames[] =
{
	"Bot",
	"This is a medium Bot",
	"This is a super long bot name that is too long for the game to allow",
	"Another bot",
	"Yet more Bot names, medium sized",
	"B",
};

//-----------------------------------------------------------------------------
// Purpose: Create a new Bot and put it in the game.
// Output : Pointer to the new Bot, or NULL if there's no free clients.
//-----------------------------------------------------------------------------
CBasePlayer *BotPutInServer( bool bFrozen, int iTeam, int iClass, const char *pszCustomName )
{
	char botname[64];
	if ( pszCustomName && pszCustomName[0] )
	{
		V_strcpy_safe( botname, pszCustomName );
	}
	else if ( bot_randomnames.GetBool() )
	{
		V_strcpy_safe( botname, g_aBotNames[RandomInt( 0, ARRAYSIZE( g_aBotNames ) - 1 )] );
	}
	else
	{
		V_sprintf_safe( botname, "Bot%02d", BotNumber );
	}

	edict_t *pEdict = engine->CreateFakeClient( botname );
	if ( !pEdict )
	{
		Msg( "Failed to create Bot.\n" );
		return NULL;
	}

	// Allocate a CBasePlayer for the bot, and call spawn
	//ClientPutInServer( pEdict, botname );
	CTFPlayer *pPlayer = ( (CTFPlayer *)CBaseEntity::Instance( pEdict ) );
	pPlayer->ClearFlags();
	pPlayer->AddFlag( FL_CLIENT | FL_FAKECLIENT );

	pPlayer->SetAsBasicBot();

	if ( bFrozen )
		pPlayer->AddEFlags( EFL_BOT_FROZEN );

	BotNumber++;

	botdata_t *pBot = &g_BotData[pPlayer->entindex() - 1];
	pBot->m_bWasDead = false;
	pBot->m_WantedTeam = iTeam;
	pBot->m_WantedClass = iClass;
	pBot->m_flJoinTeamTime = gpGlobals->curtime + 0.3;

	return pPlayer;
}


// Handler for the "bot" command.
CON_COMMAND_F( bot, "Add a bot.", FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	// The bot command uses switches like command-line switches.
	// -count <count> tells how many bots to spawn.
	// -team <index> selects the bot's team. Default is -1 which chooses randomly.
	//	Note: if you do -team !, then it 
	// -class <index> selects the bot's class. Default is -1 which chooses randomly.
	// -frozen prevents the bots from running around when they spawn in.
	// -name sets the bot's name

	// Look at -count.
	int count = args.FindArgInt( "-count", 1 );
	count = clamp( count, 1, 16 );

	if ( args.FindArg( "-all" ) )
		count = 9;

	// Look at -frozen.
	bool bFrozen = !!args.FindArg( "-frozen" );
		
	// Ok, spawn all the bots.
	while ( --count >= 0 )
	{
		// What class do they want?
		int iClass = RandomInt( TF_FIRST_NORMAL_CLASS, TF_LAST_NORMAL_CLASS );
		char const *pVal = args.FindArg( "-class" );
		if ( pVal )
		{
			for ( int i = TF_FIRST_NORMAL_CLASS; i < TF_CLASS_COUNT_ALL; i++ )
			{
				if ( V_stricmp( pVal, g_aPlayerClassNames_NonLocalized[i] ) == 0 )
				{
					iClass = i;
					break;
				}
			}
		}
		if ( args.FindArg( "-all" ) )
			iClass = 9 - count;

		int iTeam = TEAM_UNASSIGNED;
		pVal = args.FindArg( "-team" );
		if ( pVal )
		{
			if ( V_stricmp( pVal, "random" ) == 0 )
			{
				iTeam = RandomInt( FIRST_GAME_TEAM, GetNumberOfTeams() - 1 );
			}
			else
			{
				for ( int i = 0; i < GetNumberOfTeams(); i++ )
				{
					if ( V_stricmp( pVal, g_aTeamNames[i] ) == 0 )
					{
						iTeam = i;
						break;
					}
				}
			}
		}

		char const *pName = args.FindArg( "-name" );

		BotPutInServer( bFrozen, iTeam, iClass, pName );
	}
}

CON_COMMAND( bot_kick, "Remove a bot by name, or an entire team( \"red\" or \"blue\" ), or all bots( \"all\" )." )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if ( args.ArgC() < 2 )
		return;

	int iTeam = TEAM_INVALID;

	if ( V_stricmp( args[1], "all" ) == 0 )
	{
		iTeam = TEAM_ANY;
	}
	else
	{
		for ( int i = 0; i < GetNumberOfTeams(); i++ )
		{
			if ( V_stricmp( args[1], g_aTeamNames[i] ) == 0 )
			{
				iTeam = i;
				break;
			}
		}
	}

	if ( iTeam == TEAM_INVALID )
	{
		CBasePlayer *pVictim = UTIL_PlayerByName( args[1] );
		if ( pVictim && pVictim->IsBot() )
		{
			engine->ServerCommand( UTIL_VarArgs( "kickid %d\n", pVictim->GetUserID() ) );
		}
	}
	else
	{
		CUtlVector<CBasePlayer *> vecPlayers;
		CollectPlayers( &vecPlayers, iTeam );

		for ( int i = 0; i < vecPlayers.Count(); i++ )
		{
			CBasePlayer *pPlayer = vecPlayers[i];
			if ( pPlayer->IsBot() )
			{
				engine->ServerCommand( UTIL_VarArgs( "kickid %d\n", pPlayer->GetUserID() ) );
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Run through all the Bots in the game and let them think.
//-----------------------------------------------------------------------------
void Bot_RunAll( void )
{
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

		if ( pPlayer && (pPlayer->GetFlags() & FL_FAKECLIENT) && pPlayer->IsBasicBot() && !pPlayer->MyNextBotPointer() )
		{
			Bot_Think( pPlayer );
		}
	}
}

bool RunMimicCommand( CUserCmd& cmd )
{
	if ( bot_mimic.GetInt() <= 0 )
		return false;

	if ( bot_mimic.GetInt() > gpGlobals->maxClients )
		return false;

	
	CBasePlayer *pPlayer = UTIL_PlayerByIndex( bot_mimic.GetInt()  );
	if ( !pPlayer )
		return false;

	if ( !pPlayer->GetLastUserCommand() )
		return false;

	cmd = *pPlayer->GetLastUserCommand();
	cmd.viewangles[YAW] += bot_mimic_yaw_offset.GetFloat();
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Simulates a single frame of movement for a player
// Input  : *fakeclient - 
//			*viewangles - 
//			forwardmove - 
//			sidemove - 
//			upmove - 
//			buttons - 
//			impulse - 
//			msec - 
// Output : 	virtual void
//-----------------------------------------------------------------------------
static void RunPlayerMove( CTFPlayer *fakeclient, const QAngle& viewangles, float forwardmove, float sidemove, float upmove, unsigned short buttons, byte impulse, float frametime )
{
	if ( !fakeclient )
		return;

	CUserCmd cmd;

	// Store off the globals.. they're gonna get whacked
	float flOldFrametime = gpGlobals->frametime;
	float flOldCurtime = gpGlobals->curtime;

	float flTimeBase = gpGlobals->curtime + gpGlobals->frametime - frametime;
	fakeclient->SetTimeBase( flTimeBase );

	Q_memset( &cmd, 0, sizeof( cmd ) );

	if ( !RunMimicCommand( cmd ) )
	{
		VectorCopy( viewangles, cmd.viewangles );
		cmd.forwardmove = forwardmove;
		cmd.sidemove = sidemove;
		cmd.upmove = upmove;
		cmd.buttons = buttons;
		cmd.impulse = impulse;
		cmd.random_seed = random->RandomInt( 0, 0x7fffffff );
	}

	/*
	if ( bot_dontmove.GetBool() )
	{
		cmd.forwardmove = 0;
		cmd.sidemove = 0;
		cmd.upmove = 0;
	}
	*/

	MoveHelperServer()->SetHost( fakeclient );
	fakeclient->PlayerRunCommand( &cmd, MoveHelperServer() );

	// save off the last good usercmd
	fakeclient->SetLastUserCommand( cmd );

	// Clear out any fixangle that has been set
	fakeclient->pl.fixangle = FIXANGLE_NONE;

	// Restore the globals..
	gpGlobals->frametime = flOldFrametime;
	gpGlobals->curtime = flOldCurtime;
}

//-----------------------------------------------------------------------------
// Purpose: Run this Bot's AI for one frame.
//-----------------------------------------------------------------------------
void Bot_Think( CTFPlayer *pBot )
{
	// Make sure we stay being a bot
	pBot->AddFlag( FL_FAKECLIENT );

	botdata_t *botdata = &g_BotData[ ENTINDEX( pBot->edict() ) - 1 ];

	QAngle vecViewAngles;
	float forwardmove = 0.0;
	float sidemove = botdata->sidemove;
	float upmove = 0.0;
	unsigned short buttons = 0;
	byte  impulse = 0;
	float frametime = gpGlobals->frametime;

	vecViewAngles = pBot->EyeAngles();

	MDLCACHE_CRITICAL_SECTION();

	// Create some random values
	if ( pBot->GetTeamNumber() == TEAM_UNASSIGNED && gpGlobals->curtime > botdata->m_flJoinTeamTime )
	{
		const char *pszTeam = NULL;
		switch ( botdata->m_WantedTeam )
		{
		case TF_TEAM_RED:
			pszTeam = "red";
			break;
		case TF_TEAM_BLUE:
			pszTeam = "blue";
			break;
		case TF_TEAM_GREEN:
			pszTeam = "green";
			break;
		case TF_TEAM_YELLOW:
			pszTeam = "yellow";
			break;
		case TEAM_SPECTATOR:
			pszTeam = "spectator";
			break;
		default:
			pszTeam = "auto";
			break;
		}
		pBot->HandleCommand_JoinTeam( pszTeam );
	}
	else if ( pBot->GetTeamNumber() != TEAM_UNASSIGNED && pBot->IsPlayerClass( TF_CLASS_UNDEFINED, true ) )
	{
		// If they're on a team but haven't picked a class, choose a random class..
		pBot->HandleCommand_JoinClass( GetPlayerClassData( botdata->m_WantedClass )->m_szClassName );
	}
	else if ( pBot->IsAlive() && (pBot->GetSolid() == SOLID_BBOX) )
	{
		trace_t trace;

		botdata->m_bWasDead = false;

		if ( bot_saveme.GetInt() > 0 )
		{
			pBot->SaveMe();
			bot_saveme.SetValue( bot_saveme.GetInt() - 1 );
		}

		if ( !pBot->IsEFlagSet(EFL_BOT_FROZEN) )
		{
			if ( !bot_dontmove.GetBool() )
			{
				forwardmove = 600 * ( botdata->backwards ? -1 : 1 );
				if ( botdata->sidemove != 0.0f )
				{
					forwardmove *= random->RandomFloat( 0.1, 1.0f );
				}
			}
			else
			{
				forwardmove = 0;
			}

			if ( bot_jump.GetBool() && pBot->GetFlags() & FL_ONGROUND )
			{
				buttons |= IN_JUMP;
			}
		}

		if ( !pBot->IsEFlagSet(EFL_BOT_FROZEN) && !bot_dontmove.GetBool() )
		{
			Vector vecEnd;
			Vector forward;

			QAngle angle;
			float angledelta = 15.0;

			int maxtries = (int)360.0/angledelta;

			if ( botdata->lastturntoright )
			{
				angledelta = -angledelta;
			}

			angle = pBot->GetLocalAngles();

			Vector vecSrc;
			while ( --maxtries >= 0 )
			{
				AngleVectors( angle, &forward );

				vecSrc = pBot->GetLocalOrigin() + Vector( 0, 0, 36 );

				vecEnd = vecSrc + forward * 10;

				UTIL_TraceHull( vecSrc, vecEnd, VEC_HULL_MIN, VEC_HULL_MAX, 
					MASK_PLAYERSOLID, pBot, COLLISION_GROUP_NONE, &trace );

				if ( trace.fraction == 1.0 )
				{
					if ( gpGlobals->curtime < botdata->nextturntime )
					{
						break;
					}
				}

				angle.y += angledelta;

				if ( angle.y > 180 )
					angle.y -= 360;
				else if ( angle.y < -180 )
					angle.y += 360;

				botdata->nextturntime = gpGlobals->curtime + 2.0;
				botdata->lastturntoright = random->RandomInt( 0, 1 ) == 0 ? true : false;

				botdata->forwardAngle = angle;
				botdata->lastAngles = angle;

			}


			if ( gpGlobals->curtime >= botdata->nextstrafetime )
			{
				botdata->nextstrafetime = gpGlobals->curtime + 1.0f;

				if ( random->RandomInt( 0, 5 ) == 0 )
				{
					botdata->sidemove = -600.0f + 1200.0f * random->RandomFloat( 0, 2 );
				}
				else
				{
					botdata->sidemove = 0;
				}
				sidemove = botdata->sidemove;

				if ( random->RandomInt( 0, 20 ) == 0 )
				{
					botdata->backwards = true;
				}
				else
				{
					botdata->backwards = false;
				}
			}

			pBot->SetLocalAngles( angle );
			vecViewAngles = angle;
		}

		if ( bot_selectweaponslot.GetInt() >= 0 )
		{
			int slot = bot_selectweaponslot.GetInt();

			CBaseCombatWeapon *pWpn = pBot->Weapon_GetSlot( slot );
			if ( pWpn )
			{
				pBot->Weapon_Switch( pWpn );
			}

			bot_selectweaponslot.SetValue( -1 );
		}

		// Is my team being forced to defend?
		if ( bot_defend.GetInt() == pBot->GetTeamNumber() )
		{
			buttons |= IN_ATTACK2;
		}
		// If bots are being forced to fire a weapon, see if I have it
		else if ( bot_forcefireweapon.GetString() )
		{
			// Manually look through weapons to ignore subtype			
			CBaseCombatWeapon *pWeapon = NULL;
			const char *pszWeapon = bot_forcefireweapon.GetString();
			for ( int i = 0; i < pBot->WeaponCount(); i++ ) 
			{
				if ( pBot->GetWeapon( i ) && FClassnameIs( pBot->GetWeapon( i ), pszWeapon ) )
				{
					pWeapon = pBot->GetWeapon( i );
					break;
				}
			}

			if ( pWeapon )
			{
				// Switch to it if we don't have it out
				CBaseCombatWeapon *pActiveWeapon = pBot->GetActiveWeapon();

				// Switch?
				if ( pActiveWeapon != pWeapon )
				{
					pBot->Weapon_Switch( pWeapon );
				}
				else
				{
					// Start firing
					// Some weapons require releases, so randomise firing
					if ( bot_forceattackon.GetBool() || (RandomFloat(0.0,1.0) > 0.5) )
					{
						buttons |= IN_ATTACK;
					}

					if ( bot_forceattack2.GetBool() )
					{
						buttons |= IN_ATTACK2;
					}
				}
			}
		}

		if ( bot_flipout.GetInt() )
		{
			if ( bot_forceattackon.GetBool() || (RandomFloat(0.0,1.0) > 0.5) )
			{
				buttons |= bot_forceattack2.GetBool() ? IN_ATTACK2 : IN_ATTACK;
			}
		}
	}
	else
	{
		// Wait for Reinforcement wave
		if ( !pBot->IsAlive() )
		{
			if ( botdata->m_bWasDead )
			{
				// Wait for a few seconds before respawning.
				if ( gpGlobals->curtime - botdata->m_flDeadTime > 3 )
				{
					// Respawn the bot
					buttons |= IN_JUMP;
				}
			}
			else
			{
				// Start a timer to respawn them in a few seconds.
				botdata->m_bWasDead = true;
				botdata->m_flDeadTime = gpGlobals->curtime;
			}
		}
	}

	if ( bot_flipout.GetInt() == 2 )
	{

		QAngle angOffset = RandomAngle( -1, 1 );

		botdata->lastAngles += angOffset;

		for ( int i = 0 ; i < 2; i++ )
		{
			if ( fabs( botdata->lastAngles[ i ] - botdata->forwardAngle[ i ] ) > 15.0f )
			{
				if ( botdata->lastAngles[ i ] > botdata->forwardAngle[ i ] )
				{
					botdata->lastAngles[ i ] = botdata->forwardAngle[ i ] + 15;
				}
				else
				{
					botdata->lastAngles[ i ] = botdata->forwardAngle[ i ] - 15;
				}
			}
		}

		botdata->lastAngles[ 2 ] = 0;

		pBot->SetLocalAngles( botdata->lastAngles );
		vecViewAngles = botdata->lastAngles;
	}
	else if ( bot_flipout.GetInt() == 3 )
	{
		botdata->lastAngles.x = sin( gpGlobals->curtime + pBot->entindex() ) * 90;
		botdata->lastAngles.y = AngleNormalize( ( gpGlobals->curtime * 1.7 + pBot->entindex() ) * 45 );
		botdata->lastAngles.z = 0.0;

		float speed = 300; // sin( gpGlobals->curtime / 1.7 + pBot->entindex() ) * 600;
		forwardmove = sin( gpGlobals->curtime + pBot->entindex() ) * speed;
		//sidemove = cos( gpGlobals->curtime * 2.3 + pBot->entindex() ) * speed;
		sidemove = cos( gpGlobals->curtime + pBot->entindex() ) * speed;

		
		if (sin(gpGlobals->curtime ) < -0.5)
		{
			buttons |= IN_DUCK;
		}
		else if (sin(gpGlobals->curtime ) < 0.5)
		{
			buttons |= IN_WALK;
		}
		

		pBot->SetLocalAngles( botdata->lastAngles );
		vecViewAngles = botdata->lastAngles;

		// no shooting
		buttons &= ~ (IN_ATTACK2 | IN_ATTACK);
	}

	// Fix up the m_fEffects flags
	// pBot->PostClientMessagesSent();

	RunPlayerMove( pBot, vecViewAngles, forwardmove, sidemove, upmove, buttons, impulse, frametime );
}

//------------------------------------------------------------------------------
// Purpose: sends the specified command from a bot
//------------------------------------------------------------------------------
void cc_bot_sendcommand( const CCommand &args )
{
	if ( args.ArgC() < 3 )
	{
		Msg( "Too few parameters.  Usage: bot_command <bot name> <command string...>\n" );
		return;
	}

	// get the bot's player object
	CBasePlayer *pPlayer = UTIL_PlayerByName( args[1] );
	if ( !pPlayer )
	{
		Msg( "No bot with name %s\n", args[1] );
		return;
	}	
	const char *commandline = args.GetCommandString();

	// find the rest of the command line past the bot index
	commandline = strstr( commandline, args[2] );
	Assert( commandline );

	int iSize = Q_strlen(commandline) + 1;
	char *pBuf = (char *)malloc(iSize);
	Q_snprintf( pBuf, iSize, "%s", commandline );

	if ( pBuf[iSize-2] == '"' )
	{
		pBuf[iSize-2] = '\0';
	}

	// make a command object with the intended command line
	CCommand command;
	command.Tokenize( pBuf );

	// send the command
	TFGameRules()->ClientCommand( pPlayer, command );
}
static ConCommand bot_sendcommand( "bot_command", cc_bot_sendcommand, "<bot id> <command string...>.  Sends specified command on behalf of specified bot", FCVAR_CHEAT );

//------------------------------------------------------------------------------
// Purpose: sends the specified command from a bot
//------------------------------------------------------------------------------
void cc_bot_kill( const CCommand &args )
{
	// get the bot's player object
	CBasePlayer *pPlayer = UTIL_PlayerByName( args[1] );

	if ( pPlayer )
	{
		pPlayer->CommitSuicide();
	}
}

static ConCommand bot_kill( "bot_kill", cc_bot_kill, "<bot id>.  Kills bot.", FCVAR_CHEAT );

CON_COMMAND_F( bot_changeteams, "Make all bots change teams", FCVAR_CHEAT )
{
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

		if ( pPlayer && (pPlayer->GetFlags() & FL_FAKECLIENT) )
		{
			int iTeam = pPlayer->GetTeamNumber();
			int iNewTeam = iTeam;

			do
			{
				iNewTeam = RandomInt( FIRST_GAME_TEAM, GetNumberOfTeams() - 1 );
			} while ( iTeam == iNewTeam );

			pPlayer->ChangeTeam( iNewTeam );
		}
	}
}

CON_COMMAND_F( bot_refill, "Refill all bot ammo counts", FCVAR_CHEAT )
{
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

		if ( pPlayer && pPlayer->IsFakeClient() )
		{
			pPlayer->GiveAmmo( 1000, TF_AMMO_PRIMARY );
			pPlayer->GiveAmmo( 1000, TF_AMMO_SECONDARY );
			pPlayer->GiveAmmo( 1000, TF_AMMO_METAL );
			pPlayer->TakeHealth( 999, DMG_GENERIC );
		}
	}
}

CON_COMMAND_F( bot_whack, "Deliver lethal damage from player to specified bot", FCVAR_CHEAT )
{
	if ( args.ArgC() < 2 )
	{
		Msg( "Too few parameters.  Usage: bot_whack <bot name>\n" );
		return;
	}
	
	// get the bot's player object
	CBasePlayer *pBot = UTIL_PlayerByName( args[1] );
	if ( !pBot )
	{
		Msg( "No bot with name %s\n", args[1] );
		return;
	}

	CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( 1 ) );
	CTakeDamageInfo info( pBot, pTFPlayer, 1000, DMG_BULLET );
	info.SetInflictor( pTFPlayer->GetActiveTFWeapon() );
	pBot->TakeDamage( info );	
}

CON_COMMAND_F( bot_teleport, "Teleport the specified bot to the specified position & angles.\n\tFormat: bot_teleport <bot name> <X> <Y> <Z> <Pitch> <Yaw> <Roll>", FCVAR_CHEAT )
{
	const char* botname = args[1];

	if ( args.ArgC() < 2 )
	{
		CUtlVector< CBasePlayer* > botcandidates;

		CBasePlayer* pPlayer;
		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			pPlayer = ToBasePlayer(UTIL_PlayerByIndex(i));

			if (!pPlayer)
				continue;

			if (!pPlayer->IsFakeClient())
				continue;

			botcandidates.AddToTail(pPlayer);
		}

		if ( botcandidates.Count() > 0 )
		{
			int iRandom = RandomInt(0, botcandidates.Count() - 1);
			botname = botcandidates[iRandom]->GetPlayerName();
		}
	}

	if ( !botname )
	{
		Msg("No bot specified. bot_teleport <bot name> <X> <Y> <Z> <Pitch> <Yaw> <Roll>\n");
		return;
	}

	// get the bot's player object
	CBasePlayer *pBot = UTIL_PlayerByName( botname );
	if ( !pBot )
	{
		Msg( "No bot with name %s\n", botname );
		return;
	}

	Vector vecPos( vec3_origin );
	QAngle vecAng( vec3_angle );

	if ( args.ArgC() >= 5 )
	{
		vecPos = Vector( atof( args[2] ), atof( args[3] ), atof( args[4] ) );

		if ( args.ArgC() >= 8 )
		{
			vecAng = QAngle( atof( args[5] ), atof( args[6] ), atof( args[7] ) );
		}
	}
	else
	{
		CBasePlayer* pPlayer = UTIL_GetCommandClient();
		trace_t tr;
		Vector forward;
		pPlayer->EyeVectors( &forward );
		UTIL_TraceLine( pPlayer->EyePosition(), pPlayer->EyePosition() + forward * MAX_TRACE_LENGTH, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr );
		if ( tr.fraction != 1.0 )
		{
			vecPos = tr.endpos;
		}
	}

	pBot->Teleport( &vecPos, &vecAng, NULL );
}