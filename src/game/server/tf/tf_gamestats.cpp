//====== Copyright � 1996-2006, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "tf_gamerules.h"
#include "tf_gamestats.h"
#include "tf_obj_sentrygun.h"
#include "tf_obj_dispenser.h"
#include "tf_obj_sapper.h"
#include "usermessages.h"
#include "player_resource.h"
#include "tf_team.h"
#include "hl2orange.spa.h"
#include "NextBotManager.h"

#include "tf_randomizer_manager.h"

// Must run with -gamestats to be able to turn on/off stats with ConVar below.
static ConVar tf_stats_track( "tf_stats_track", "1", FCVAR_NONE, "Turn on//off tf stats tracking." );
static ConVar tf_stats_verbose( "tf_stats_verbose", "0", FCVAR_NONE, "Turn on//off verbose logging of stats." );

extern ConVar tf2c_nemesis_relationships;

CTFGameStats CTF_GameStats;

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  :  - 
//-----------------------------------------------------------------------------
CTFGameStats::CTFGameStats()
{
	gamestats = this;
	Clear();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
// Input  :  - 
//-----------------------------------------------------------------------------
CTFGameStats::~CTFGameStats()
{
	Clear();
}

//-----------------------------------------------------------------------------
// Purpose: Clear out game stats
// Input  :  - 
//-----------------------------------------------------------------------------
void CTFGameStats::Clear( void )
{
	m_reportedStats.Clear();
	Q_memset( m_aPlayerStats, 0, sizeof( m_aPlayerStats ) );
	CBaseGameStats::Clear();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTFGameStats::StatTrackingEnabledForMod( void )
{
	// Don't track during extreme modes.
	if ( GetRandomizerManager()->RandomizerFlags() != TF_RANDOMIZER_OFF )
		return false;

	return tf_stats_track.GetBool();
}

//-----------------------------------------------------------------------------
// Purpose: Loads previously saved game stats from file
//-----------------------------------------------------------------------------
bool CTFGameStats::LoadFromFile( void )
{
	// We deliberately don't load from previous file.  That's data we've already
	// reported, and for TF stats we don't want to re-accumulate data, just
	// keep sending fresh stuff to server.
	return false;
}


void CTFGameStats::AppendCustomDataToSaveBuffer( CUtlBuffer &SaveBuffer )
{
	m_reportedStats.AppendCustomDataToSaveBuffer( SaveBuffer );
	// clear stats since we've now reported these
	m_reportedStats.Clear();
}


void CTFGameStats::LoadCustomDataFromBuffer( CUtlBuffer &LoadBuffer )
{
	m_reportedStats.LoadCustomDataFromBuffer( LoadBuffer );
}


bool CTFGameStats::Init( void )
{
	return true;
}


void CTFGameStats::Event_LevelInit( void )
{
	CBaseGameStats::Event_LevelInit();

	ClearCurrentGameData();

	// Get the host ip and port.
	int nIPAddr = 0;
	short nPort = 0;
	ConVar *hostip = cvar->FindVar( "hostip" );
	if ( hostip )
	{
		nIPAddr = hostip->GetInt();
	}			

	ConVar *hostport = cvar->FindVar( "hostport" );
	if ( hostport )
	{
		nPort = hostport->GetInt();
	}			

	m_reportedStats.m_pCurrentGame->Init( STRING( gpGlobals->mapname ), nIPAddr, nPort, gpGlobals->curtime );

	TF_Gamestats_LevelStats_t *map = m_reportedStats.FindOrAddMapStats( STRING( gpGlobals->mapname ) );
	map->Init( STRING( gpGlobals->mapname ), nIPAddr, nPort, gpGlobals->curtime );
}


void CTFGameStats::Event_LevelShutdown( float flElapsed )
{
	if ( m_reportedStats.m_pCurrentGame )
	{
		flElapsed = gpGlobals->curtime - m_reportedStats.m_pCurrentGame->m_flRoundStartTime;
		m_reportedStats.m_pCurrentGame->m_Header.m_iTotalTime += (int) flElapsed;
	}

	// add current game data in to data for this level
	AccumulateGameData();

	CBaseGameStats::Event_LevelShutdown( flElapsed );
}

//-----------------------------------------------------------------------------
// Purpose: Resets all stats for this player
//-----------------------------------------------------------------------------
void CTFGameStats::ResetPlayerStats( CTFPlayer *pPlayer )
{
	PlayerStats_t &stats = m_aPlayerStats[pPlayer->entindex()];
	// reset the stats on this player
	stats.Reset();
	// reset the matrix of who killed whom with respect to this player
	ResetKillHistory( pPlayer );
	// let the client know to reset its stats
	SendStatsToPlayer( pPlayer, STATMSG_RESET );
}

//-----------------------------------------------------------------------------
// Purpose: Resets the kill history for this player
//-----------------------------------------------------------------------------
void CTFGameStats::ResetKillHistory( CTFPlayer *pPlayer, bool bTeammatesOnly /*= false*/ )
{
	int iPlayerIndex = pPlayer->entindex();

	// for every other player, set all all the kills with respect to this player to 0
	for ( int i = 0; i < ARRAYSIZE( m_aPlayerStats ); i++ )
	{
		if ( bTeammatesOnly )
		{
			CBasePlayer *pOther = UTIL_PlayerByIndex( i );
			if ( pOther && pPlayer->IsEnemy( pOther ) )
				continue;
		}

		PlayerStats_t &statsOther = m_aPlayerStats[i];
		statsOther.statsKills.iNumKilled[iPlayerIndex] = 0;
		statsOther.statsKills.iNumKilledBy[iPlayerIndex] = 0;
		statsOther.statsKills.iNumKilledByUnanswered[iPlayerIndex] = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Resets per-round stats for all players
//-----------------------------------------------------------------------------
void CTFGameStats::ResetRoundStats()
{
	for ( int i = 0; i < ARRAYSIZE( m_aPlayerStats ); i++ )
	{		
		m_aPlayerStats[i].statsCurrentRound.Reset();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Increments specified stat for specified player by specified amount
//-----------------------------------------------------------------------------
void CTFGameStats::IncrementStat( CTFPlayer *pPlayer, TFStatType_t statType, int iValue )
{
	PlayerStats_t &stats = m_aPlayerStats[pPlayer->entindex()];
	stats.statsCurrentLife.m_iStat[statType] += iValue;
	stats.statsCurrentRound.m_iStat[statType] += iValue;
	stats.statsAccumulated.m_iStat[statType] += iValue;

	// if this stat should get sent to client, mark it as dirty
	if ( ShouldSendToClient( statType ) )
	{
		stats.iStatsChangedBits |= 1 << ( statType - TFSTAT_FIRST );
	}
}


void CTFGameStats::SendStatsToPlayer( CTFPlayer *pPlayer, int iMsgType )
{
	int entidx = pPlayer->entindex();
	if (entidx <= 0)
	{
		return;
	}
	PlayerStats_t &stats = m_aPlayerStats[pPlayer->entindex()];

	int iSendBits = stats.iStatsChangedBits;
	switch ( iMsgType )
	{
		case STATMSG_PLAYERDEATH:
		case STATMSG_PLAYERRESPAWN:
		case STATMSG_GAMEEND:
			// Calc player score from this life.
			AccumulateAndResetPerLifeStats( pPlayer );
			iSendBits = stats.iStatsChangedBits;
			break;
		case STATMSG_RESET:
			// this is a reset message, no need to send any stat values with it
			iSendBits = 0;
			break;
		case STATMSG_UPDATE:
			// if nothing changed, no need to send a message
			if ( iSendBits == 0 )
				return;
			break;
		case STATMSG_PLAYERSPAWN:
			// do a full update at player spawn
			for ( int i = TFSTAT_FIRST; i < TFSTAT_MAX; i++ )
			{
				iSendBits |= ( 1 << ( i - TFSTAT_FIRST ) );
			}
			break;
		default:
			Assert( false );
	}

	int iStat = TFSTAT_FIRST;
	CSingleUserRecipientFilter filter( pPlayer );
	UserMessageBegin( filter, "PlayerStatsUpdate" );
	WRITE_BYTE( pPlayer->GetPlayerClass()->GetClassIndex() );		// write the class
	WRITE_BYTE( iMsgType );											// type of message
	WRITE_LONG( iSendBits );										// write the bit mask of which stats follow in the message

	// write all the stats specified in the bit mask
	while ( iSendBits > 0 )
	{
		if ( iSendBits & 1 )
		{
			WRITE_LONG( stats.statsAccumulated.m_iStat[iStat] );
		}
		iSendBits >>= 1;
		iStat ++;
	}
	MessageEnd();

	stats.iStatsChangedBits = 0;
	stats.m_flTimeLastSend = gpGlobals->curtime;

	if ( iMsgType == STATMSG_PLAYERDEATH || iMsgType == STATMSG_GAMEEND )
	{
		// max sentry kills is different from other stats, it is a max value and can span player lives.  Reset it to zero so 
		// it doesn't get re-reported in the next life unless the sentry stays alive and gets more kills.
		pPlayer->SetMaxSentryKills( 0 );
		Event_MaxSentryKills( pPlayer, 0 );
	}
}


void CTFGameStats::AccumulateAndResetPerLifeStats( CTFPlayer *pPlayer )
{
	int iClass = pPlayer->GetPlayerClass()->GetClassIndex();

	PlayerStats_t &stats = m_aPlayerStats[pPlayer->entindex()];

	// add score from previous life and reset current life stats
	int iScore = TFGameRules()->CalcPlayerScore( &stats.statsCurrentLife );
	if ( m_reportedStats.m_pCurrentGame != NULL )
	{
		m_reportedStats.m_pCurrentGame->m_aClassStats[iClass].iScore += iScore;
	}
	stats.statsCurrentRound.m_iStat[TFSTAT_POINTSSCORED] += iScore;
	stats.statsAccumulated.m_iStat[TFSTAT_POINTSSCORED] += iScore;
	stats.statsCurrentLife.Reset();

	if ( iScore != 0 )
	{
		stats.iStatsChangedBits |= 1 << ( TFSTAT_POINTSSCORED - TFSTAT_FIRST );
	}
}


void CTFGameStats::Event_PlayerConnected( CBasePlayer *pPlayer )
{
	ResetPlayerStats( ToTFPlayer( pPlayer ) );
}


void CTFGameStats::Event_PlayerDisconnected( CBasePlayer *pPlayer )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
	if ( !pTFPlayer )
		return;

	ResetPlayerStats( pTFPlayer );

	if ( pPlayer->IsAlive() )
	{
		int iClass = pTFPlayer->GetPlayerClass()->GetClassIndex();
		if ( m_reportedStats.m_pCurrentGame != NULL )
		{
			m_reportedStats.m_pCurrentGame->m_aClassStats[iClass].iTotalTime += (int) ( gpGlobals->curtime - pTFPlayer->GetSpawnTime() );
		}
	}
}


void CTFGameStats::Event_PlayerChangedClass( CTFPlayer *pPlayer )
{
}


void CTFGameStats::Event_PlayerSpawned( CTFPlayer *pPlayer )
{	
	// if player is spawning as a member of valid team, increase the spawn count for his class
	int iTeam = pPlayer->GetTeamNumber();
	int iClass = pPlayer->GetPlayerClass()->GetClassIndex();
	if ( TEAM_UNASSIGNED != iTeam && TEAM_SPECTATOR != iTeam )
	{
		if ( m_reportedStats.m_pCurrentGame != NULL )
		{
			m_reportedStats.m_pCurrentGame->m_aClassStats[iClass].iSpawns++;
		}
	}

	TF_Gamestats_LevelStats_t *map = m_reportedStats.m_pCurrentGame;
	if ( !map )
		return;

	// calculate peak player count on each team
	for ( iTeam = FIRST_GAME_TEAM; iTeam < GetNumberOfTeams(); iTeam++ )
	{
		CTFTeam *pTeam = GetGlobalTFTeam( iTeam );

		int iPlayerCount = pTeam->GetNumPlayers();
		if ( iPlayerCount > map->m_iPeakPlayerCount[iTeam] )
		{
			map->m_iPeakPlayerCount[iTeam] = iPlayerCount;
		}
	}

	if ( iClass >= TF_FIRST_NORMAL_CLASS && iClass < TF_CLASS_COUNT_ALL )
	{
		SendStatsToPlayer( pPlayer, STATMSG_PLAYERSPAWN );
	}
}

	
void CTFGameStats::Event_PlayerForceRespawn( CTFPlayer *pPlayer )
{
	if ( pPlayer->IsAlive() && !TFGameRules()->PrevRoundWasWaitingForPlayers() )
	{		
		// send stats to player
		SendStatsToPlayer( pPlayer, STATMSG_PLAYERRESPAWN );

		// if player is alive before respawn, add time from this life to class stat
		int iClass = pPlayer->GetPlayerClass()->GetClassIndex();
		if ( m_reportedStats.m_pCurrentGame != NULL )
		{
			m_reportedStats.m_pCurrentGame->m_aClassStats[iClass].iTotalTime += (int)( gpGlobals->curtime - pPlayer->GetSpawnTime() );
		}
	}
}


void CTFGameStats::Event_PlayerLeachedHealth( CTFPlayer *pPlayer, bool bDispenserHeal, float amount ) 
{
	// UNDONE: Not used by any TF2C achievements.
	/*if ( !bDispenserHeal )
	{
		// If this was a heal by enemy medic and the first such heal that the server is aware of for this player,
		// send an achievement event to client.  On the client, it will award achievement if player doesn't have it yet
		PlayerStats_t &stats = m_aPlayerStats[pPlayer->entindex()];
		if ( 0 == stats.statsAccumulated.m_iStat[TFSTAT_HEALTHLEACHED] )
		{
			CSingleUserRecipientFilter filter( pPlayer );
			UserMessageBegin( filter, "AchievementEvent" );
				WRITE_BYTE( ACHIEVEMENT_TF_GET_HEALED_BYENEMY );
			MessageEnd();
		}
	}*/

	IncrementStat( pPlayer, TFSTAT_HEALTHLEACHED, (int)amount );
}


void CTFGameStats::Event_PlayerHealedOther( CTFPlayer *pPlayer, float iAmount ) 
{
		IncrementStat( pPlayer, TFSTAT_HEALING, (int)iAmount );
}


void CTFGameStats::Event_PlayerBlockedDamage( CTFPlayer *pPlayer, int iAmount ) 
{
	IncrementStat( pPlayer, TFSTAT_DAMAGE_BLOCKED, iAmount );
}


void CTFGameStats::Event_AssistKill( CTFPlayer *pAttacker, CBaseEntity *pVictim )
{
	// increment player's stat
	IncrementStat( pAttacker, TFSTAT_KILLASSISTS, 1 );

	// increment reported class stats
	int iClass = pAttacker->GetPlayerClass()->GetClassIndex();
	if ( m_reportedStats.m_pCurrentGame != NULL )
	{
		m_reportedStats.m_pCurrentGame->m_aClassStats[iClass].iAssists++;
	}

	if ( pVictim->IsPlayer() )
	{
		// keep track of how many times every player kills every other player
		CTFPlayer *pPlayerVictim = ToTFPlayer( pVictim );
		TrackKillStats( pAttacker, pPlayerVictim );
	}
}


void CTFGameStats::Event_PlayerInvulnerable( CTFPlayer *pPlayer ) 
{
	IncrementStat( pPlayer, TFSTAT_INVULNS, 1 );
}


void CTFGameStats::Event_PlayerCreatedBuilding( CTFPlayer *pPlayer, CBaseObject *pBuilding )
{
	// sappers are buildings from the code's point of view but not from the player's, don't count them
	CObjectSapper *pSapper = dynamic_cast<CObjectSapper *>( pBuilding );
	if ( pSapper )
		return;

	IncrementStat( pPlayer, TFSTAT_BUILDINGSBUILT, 1 );
}


void CTFGameStats::Event_PlayerDestroyedBuilding( CTFPlayer *pPlayer, CBaseObject *pBuilding )
{
	// sappers are buildings from the code's point of view but not from the player's, don't count them
	CObjectSapper *pSapper = dynamic_cast<CObjectSapper *>( pBuilding );
	if ( pSapper )
		return;

	IncrementStat( pPlayer, TFSTAT_BUILDINGSDESTROYED, 1 );
}


void CTFGameStats::Event_AssistDestroyBuilding( CTFPlayer *pPlayer, CBaseObject *pBuilding )
{
	// sappers are buildings from the code's point of view but not from the player's, don't count them
	CObjectSapper *pSapper = dynamic_cast<CObjectSapper *>( pBuilding );
	if ( pSapper )
		return;

	IncrementStat( pPlayer, TFSTAT_KILLASSISTS, 1 );
}


void CTFGameStats::Event_Headshot( CTFPlayer *pKiller )
{
	IncrementStat( pKiller, TFSTAT_HEADSHOTS, 1 );
}


void CTFGameStats::Event_Backstab( CTFPlayer *pKiller )
{
	IncrementStat( pKiller, TFSTAT_BACKSTABS, 1 );
}


void CTFGameStats::Event_PlayerUsedTeleport( CTFPlayer *pTeleportOwner, CTFPlayer *pTeleportingPlayer )
{
	// We don't count the builder's teleports
	if ( pTeleportOwner != pTeleportingPlayer )
	{
		IncrementStat( pTeleportOwner, TFSTAT_TELEPORTS, 1 );
	}
}


void CTFGameStats::Event_PlayerUsedJumppad( CTFPlayer *pJumppadOwner, CTFPlayer *pJumpingPlayer )
{
	// We don't count the builder's jumppads
	if (pJumppadOwner != pJumpingPlayer)
	{
		IncrementStat(pJumppadOwner, TFSTAT_JUMPPAD_JUMPS, 1);
	}
}


void CTFGameStats::Event_PlayerFiredWeapon( CTFPlayer *pPlayer, bool bCritical ) 
{
	// If normal gameplay state, track weapon stats. 
	if ( TFGameRules()->State_Get() == GR_STATE_RND_RUNNING )
	{
		CTFWeaponBase *pTFWeapon = pPlayer->GetActiveTFWeapon();
		if ( pTFWeapon )
		{
			// record shots fired in reported per-weapon stats
			ETFWeaponID iWeaponID = pTFWeapon->GetWeaponID();

			if ( m_reportedStats.m_pCurrentGame != NULL )
			{
				TF_Gamestats_WeaponStats_t *pWeaponStats = &m_reportedStats.m_pCurrentGame->m_aWeaponStats[iWeaponID];
				pWeaponStats->iShotsFired++;
				if ( bCritical )
				{
					pWeaponStats->iCritShotsFired++;
				}
			}

			pPlayer->OnMyWeaponFired( pTFWeapon );
			TheNextBots().OnWeaponFired( pPlayer, pTFWeapon );
		}
	}

	IncrementStat( pPlayer, TFSTAT_SHOTS_FIRED, 1 );
}


void CTFGameStats::Event_PlayerDamage( CBasePlayer *pBasePlayer, const CTakeDamageInfo &info, int iDamageTaken )
{
	CTFPlayer *pTarget = ToTFPlayer( pBasePlayer );
	CTFPlayer *pAttacker = ToTFPlayer( info.GetAttacker() );
	if ( !pAttacker )
		return;

	// don't count damage to yourself
	if ( pTarget == pAttacker )
		return;
	
	IncrementStat( pAttacker, TFSTAT_DAMAGE, iDamageTaken );

	TF_Gamestats_LevelStats_t::PlayerDamageLump_t damage;
	Vector killerOrg = vec3_origin;

	// set the location where the target was hit
	const Vector &org = pTarget->GetAbsOrigin();
	damage.nTargetPosition[0] = static_cast<int>( org.x );
	damage.nTargetPosition[1] = static_cast<int>( org.y );
	damage.nTargetPosition[2] = static_cast<int>( org.z );

	// set the class of the attacker
	if ( TFGameRules()->IsSentryDamager( info ) )
	{
		killerOrg = info.GetWeapon()->GetAbsOrigin();
		damage.iAttackClass = TF_CLASS_ENGINEER;
	} 
	else
	{
		if ( pAttacker )
		{
			damage.iAttackClass = pAttacker->GetPlayerClass()->GetClassIndex();
			killerOrg = pAttacker->GetAbsOrigin();
		}
		else
		{
			damage.iAttackClass = TF_CLASS_UNDEFINED;
			killerOrg = org;
		}
	}

	// find the weapon the killer used
	damage.iWeapon = (short)GetWeaponFromDamage( info );

	// If normal gameplay state, track weapon stats. 
	if ( TFGameRules()->State_Get() == GR_STATE_RND_RUNNING && damage.iWeapon != TF_WEAPON_NONE )
	{
		// record hits & damage in reported per-weapon stats
		if ( m_reportedStats.m_pCurrentGame != NULL )
		{
			TF_Gamestats_WeaponStats_t *pWeaponStats = &m_reportedStats.m_pCurrentGame->m_aWeaponStats[damage.iWeapon];
			pWeaponStats->iHits++;
			pWeaponStats->iTotalDamage += iDamageTaken;

			// Try and figure out where the damage is coming from
			Vector vecDamageOrigin = info.GetReportedPosition();
			// If we didn't get an origin to use, try using the attacker's origin
			if ( vecDamageOrigin == vec3_origin )
			{
				vecDamageOrigin = killerOrg;
			}

			if ( vecDamageOrigin != vec3_origin )
			{
				pWeaponStats->iHitsWithKnownDistance++;
				int iDistance = (int) vecDamageOrigin.DistTo( pBasePlayer->GetAbsOrigin() );
//				Msg( "Damage distance: %d\n", iDistance );
				pWeaponStats->iTotalDistance += iDistance;
			}
		}
	}

	Assert( damage.iAttackClass != TF_CLASS_UNDEFINED );

	// record the time the damage occurred
	damage.fTime = gpGlobals->curtime;

	// store the attacker's position
	damage.nAttackerPosition[0] = static_cast<int>( killerOrg.x );
	damage.nAttackerPosition[1] = static_cast<int>( killerOrg.y );
	damage.nAttackerPosition[2] = static_cast<int>( killerOrg.z );

	// set the class of the target
	damage.iTargetClass = pTarget->GetPlayerClass()->GetClassIndex();

	Assert( damage.iTargetClass != TF_CLASS_UNDEFINED );

	// record the damage done
	damage.iDamage = info.GetDamage();

	// record if it was a crit
	damage.iCrit = ( ( info.GetDamageType() & DMG_CRITICAL ) != 0 );

	// record if it was a kill
	damage.iKill = ( pTarget->GetHealth() <= 0 );

	// add it to the list of damages
	if ( m_reportedStats.m_pCurrentGame != NULL )
	{
		m_reportedStats.m_pCurrentGame->m_aPlayerDamage.AddToTail( damage );
	}	
}


void CTFGameStats::Event_PlayerDamageAssist( CBasePlayer *pProvider, int iBonusDamage )
{
	CTFPlayer *pPlayerProvider = static_cast<CTFPlayer *>( pProvider );
	if ( pPlayerProvider )
	{
		IncrementStat( pPlayerProvider, TFSTAT_DAMAGE_ASSIST, iBonusDamage );
	}
}


void CTFGameStats::Event_PlayerKilledOther( CBasePlayer *pAttacker, CBaseEntity *pVictim, const CTakeDamageInfo &info )
{
	// This also gets called when the victim is a building.  That gets tracked separately as building destruction, don't count it here
	if ( !pVictim->IsPlayer() )
		return;

	CTFPlayer *pPlayerAttacker = static_cast<CTFPlayer *>( pAttacker );
	IncrementStat( pPlayerAttacker, TFSTAT_KILLS, 1 );

	// keep track of how many times every player kills every other player
	CTFPlayer *pPlayerVictim = ToTFPlayer( pVictim );
	TrackKillStats( pAttacker, pPlayerVictim );
	
	int iClass = pPlayerAttacker->GetPlayerClass()->GetClassIndex();
	if ( m_reportedStats.m_pCurrentGame != NULL )
	{
		m_reportedStats.m_pCurrentGame->m_aClassStats[iClass].iKills++;
	}
}


void CTFGameStats::Event_PlayerSuicide( CBasePlayer *pPlayer )
{
	CTFPlayer *pTFVictim = static_cast<CTFPlayer *>( pPlayer );
	IncrementStat( pTFVictim, TFSTAT_SUICIDES, 1 );
}


void CTFGameStats::Event_RoundEnd( int iWinningTeam, bool bFullRound, float flRoundTime, bool bWasSuddenDeathWin )
{
	TF_Gamestats_LevelStats_t *map = m_reportedStats.m_pCurrentGame;
	Assert( map );
	if ( !map )
		return;

	m_reportedStats.m_pCurrentGame->m_Header.m_iTotalTime += (int) flRoundTime;
	m_reportedStats.m_pCurrentGame->m_flRoundStartTime = gpGlobals->curtime;

	// only record full rounds, not mini-rounds
	if ( !bFullRound )
		return;

	map->m_Header.m_iRoundsPlayed++;
	switch ( iWinningTeam )
	{
		case TF_TEAM_RED:
			map->m_Header.m_iRedWins++;
			if ( bWasSuddenDeathWin )
			{
				map->m_Header.m_iRedSuddenDeathWins++;
			}
			break;
		case TF_TEAM_BLUE:
			map->m_Header.m_iBlueWins++;
			if ( bWasSuddenDeathWin )
			{
				map->m_Header.m_iBlueSuddenDeathWins++;
			}
			break;
		case TF_TEAM_GREEN:
			map->m_Header.m_iGreenWins++;
			if ( bWasSuddenDeathWin )
			{
				map->m_Header.m_iGreenSuddenDeathWins++;
			}
			break;
		case TF_TEAM_YELLOW:
			map->m_Header.m_iYellowWins++;
			if ( bWasSuddenDeathWin )
			{
				map->m_Header.m_iYellowSuddenDeathWins++;
			}
			break;
		case TEAM_UNASSIGNED:
			map->m_Header.m_iStalemates++;
			break;
		default:
			Assert( false );
			break;
	}

	// add current game data in to data for this level
	AccumulateGameData();
}


void CTFGameStats::Event_PlayerCapturedPoint( CTFPlayer *pPlayer )
{
	// increment player stats
	IncrementStat( pPlayer, TFSTAT_CAPTURES, 1 );
	// increment reported stats
	int iClass = pPlayer->GetPlayerClass()->GetClassIndex();
	if ( m_reportedStats.m_pCurrentGame != NULL )
	{
		m_reportedStats.m_pCurrentGame->m_aClassStats[iClass].iCaptures++;
	}	
}


void CTFGameStats::Event_PlayerDefendedPoint( CTFPlayer *pPlayer )
{
	IncrementStat( pPlayer, TFSTAT_DEFENSES, 1 );
}


void CTFGameStats::Event_PlayerDominatedOther( CTFPlayer *pAttacker )
{
	IncrementStat( pAttacker, TFSTAT_DOMINATIONS, 1 );
}


void CTFGameStats::Event_PlayerRevenge( CTFPlayer *pAttacker )
{
	IncrementStat( pAttacker, TFSTAT_REVENGE, 1 );
}


void CTFGameStats::Event_MaxSentryKills( CTFPlayer *pAttacker, int iMaxKills )
{
	// Max sentry kills is a little different from other stats, it is the most kills from
	// any single sentry the player builds during his lifetime.  It does not increase monotonically
	// so this is a little different than the other stat code.
	PlayerStats_t &stats = m_aPlayerStats[pAttacker->entindex()];
	int iCur = stats.statsCurrentRound.m_iStat[TFSTAT_MAXSENTRYKILLS];
	if ( iCur != iMaxKills )
	{
		stats.statsCurrentRound.m_iStat[TFSTAT_MAXSENTRYKILLS] = iMaxKills;
		stats.iStatsChangedBits |= ( 1 << ( TFSTAT_MAXSENTRYKILLS - TFSTAT_FIRST ) );
	}
}


void CTFGameStats::Event_PlayerKilled( CBasePlayer *pPlayer, const CTakeDamageInfo &info )
{
	Assert( pPlayer );
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );

	IncrementStat( pTFPlayer, TFSTAT_DEATHS, 1 );

	TF_Gamestats_LevelStats_t::PlayerDeathsLump_t death;
	Vector killerOrg;

	// set the location where the target died
	const Vector &org = pPlayer->GetAbsOrigin();
	death.nPosition[ 0 ] = static_cast<int>( org.x );
	death.nPosition[ 1 ] = static_cast<int>( org.y );
	death.nPosition[ 2 ] = static_cast<int>( org.z );

	// set the class of the attacker
	CTFPlayer *pScorer = ToTFPlayer( info.GetAttacker() );

	if ( TFGameRules()->IsSentryDamager( info ) )
	{
		killerOrg = info.GetWeapon()->GetAbsOrigin();
		death.iAttackClass = TF_CLASS_ENGINEER;
	}
	else
	{		
		if ( pScorer )
		{
			death.iAttackClass = pScorer->GetPlayerClass()->GetClassIndex();
			killerOrg = pScorer->GetAbsOrigin();
		}
		else
		{
			death.iAttackClass = TF_CLASS_UNDEFINED;
			killerOrg = org;

			// Environmental death.
			IncrementStat( pTFPlayer, TFSTAT_ENV_DEATHS, 1 );
		}
	}

	SendStatsToPlayer( pTFPlayer, STATMSG_PLAYERDEATH );

	// set the class of the target
	death.iTargetClass = pTFPlayer->GetPlayerClass()->GetClassIndex();

	// find the weapon the killer used
	death.iWeapon = (short)GetWeaponFromDamage( info );

	// calculate the distance to the killer
	death.iDistance = static_cast<unsigned short>( ( killerOrg - org ).Length() );

	// add it to the list of deaths
	TF_Gamestats_LevelStats_t *map = m_reportedStats.m_pCurrentGame;
	if ( map )
	{
		map->m_aPlayerDeaths.AddToTail( death );
		int iClass = pTFPlayer->GetPlayerClass()->GetClassIndex();

		if ( m_reportedStats.m_pCurrentGame != NULL )
		{
			m_reportedStats.m_pCurrentGame->m_aClassStats[iClass].iDeaths++;
			m_reportedStats.m_pCurrentGame->m_aClassStats[iClass].iTotalTime += (int) ( gpGlobals->curtime - pTFPlayer->GetSpawnTime() );
		}
	}
}


void CTFGameStats::Event_PlayerAwardBonusPoints( CTFPlayer *pPlayer, CBaseEntity *pAwarder, int iAmount )
{
	IncrementStat( pPlayer, TFSTAT_BONUS_POINTS, iAmount );

	if ( pAwarder )
	{
		CSingleUserRecipientFilter filter( pPlayer );
		filter.MakeReliable();

		UserMessageBegin( filter, "PlayerBonusPoints" );
			WRITE_BYTE( iAmount );
			WRITE_BYTE( pPlayer->entindex() );
			WRITE_SHORT( pAwarder->entindex() );
		MessageEnd();
	}
}


void CTFGameStats::Event_GameEnd( void )
{
	// Calculate score and send out stats to everyone.
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pPlayer && pPlayer->IsAlive() )
		{
			AccumulateAndResetPerLifeStats( pPlayer );
			SendStatsToPlayer( pPlayer, STATMSG_GAMEEND );
		}
	}
}


void CTFGameStats::Event_PlayerScoresEscortPoints( CTFPlayer *pPlayer, int iAmount )
{
	IncrementStat( pPlayer, TFSTAT_CAPTURES, iAmount );
}


void CTFGameStats::Event_PlayerMovedToSpectators( CTFPlayer *pPlayer )
{
	if ( pPlayer && pPlayer->IsAlive() )
	{
		// Send out their stats.
		SendStatsToPlayer( pPlayer, STATMSG_GAMEEND );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Per-frame handler
//-----------------------------------------------------------------------------
void CTFGameStats::FrameUpdatePostEntityThink()
{
	// see if any players have stat changes we need to send
	for( int iPlayerIndex = 1 ; iPlayerIndex <= MAX_PLAYERS; iPlayerIndex++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );
		if ( pPlayer && pPlayer->IsConnected() && pPlayer->IsAlive() )
		{
			PlayerStats_t &stats = m_aPlayerStats[pPlayer->entindex()];
			// if there are any updated stats for this player and we haven't sent a stat update for this player in the last second,
			// send one now.
			if ( ( stats.iStatsChangedBits > 0 ) && ( gpGlobals->curtime >= stats.m_flTimeLastSend + 1.0f ) )
			{
				SendStatsToPlayer( pPlayer, STATMSG_UPDATE );
			}						
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Adds data from current game into accumulated data for this level.
//-----------------------------------------------------------------------------
void CTFGameStats::AccumulateGameData()
{
	// find or add a bucket for this level
	TF_Gamestats_LevelStats_t *map = m_reportedStats.FindOrAddMapStats( STRING( gpGlobals->mapname ) );
	// get current game data
	TF_Gamestats_LevelStats_t *game = m_reportedStats.m_pCurrentGame;
	if ( !map || !game )
		return;
	
	// Sanity-check that this looks like real game play -- must have minimum # of players on both teams,
	// minimum time and some damage to players must have occurred
	if ( ( game->m_iPeakPlayerCount[TF_TEAM_RED] >= 3  ) && ( game->m_iPeakPlayerCount[TF_TEAM_BLUE] >= 3 ) &&
		( game->m_Header.m_iTotalTime >= 4 * 60 ) && ( game->m_aPlayerDamage.Count() > 0 ) ) 
	{
		// if this looks like real game play, add it to stats
		map->Accumulate( game );
	}
		
	ClearCurrentGameData();
}

//-----------------------------------------------------------------------------
// Purpose: Clears data for current game
//-----------------------------------------------------------------------------
// there is a leak somewhere here...
void CTFGameStats::ClearCurrentGameData()
{
	if ( m_reportedStats.m_pCurrentGame )
	{
		delete m_reportedStats.m_pCurrentGame;
 	}
	m_reportedStats.m_pCurrentGame = new TF_Gamestats_LevelStats_t;
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether this stat should be sent to the client
//-----------------------------------------------------------------------------
bool CTFGameStats::ShouldSendToClient( TFStatType_t statType )
{
	switch ( statType )
	{
		// don't need to send these
		case TFSTAT_SHOTS_HIT:
		case TFSTAT_SHOTS_FIRED:
		case TFSTAT_SUICIDES:
		case TFSTAT_ENV_DEATHS:
			return false;
		default:
			return true;
	}
}

struct MapNameAndPlaytime_t
{
	const char *mapname;
	int timeplayed;
};

static int SortMapPlaytime( const MapNameAndPlaytime_t *map1, const MapNameAndPlaytime_t *map2 )
{
	return ( map1->timeplayed - map2->timeplayed );
}


void CTFGameStats::GetVoteData( const char *pszVoteIssue, int iNumOptions, CUtlStringList &optionList )
{
	if ( V_strcmp( pszVoteIssue, "NextLevel" ) == 0 )
	{
		// Always add the next map in the cycle.
		char szNextMap[MAX_MAP_NAME];
		TFGameRules()->GetNextLevelName( szNextMap, MAX_MAP_NAME );
		optionList.CopyAndAddToTail( szNextMap );
		iNumOptions--;

		if ( !g_pStringTableServerMapCycle )
			return;

		// Load the map list from the cycle.
		int iStringIndex = g_pStringTableServerMapCycle->FindStringIndex( "ServerMapCycle" );
		if ( iStringIndex == INVALID_STRING_INDEX )
			return;

		int nLength = 0;
		const char *pszMapCycle = (const char *)g_pStringTableServerMapCycle->GetStringUserData( iStringIndex, &nLength );
		if ( !pszMapCycle || !pszMapCycle[0] )
			return;

		CUtlStringList mapList;
		V_SplitString( pszMapCycle, "\n", mapList );

		CUtlVector<MapNameAndPlaytime_t> mapPlaytimeList;

		for ( const char *pszMapName : mapList )
		{
			// Skip the current map.
			if ( V_strcmp( pszMapName, STRING( gpGlobals->mapname ) ) == 0 )
				continue;

			// Skip the next map as we've already included it.
			if ( V_strcmp( pszMapName, szNextMap ) == 0 )
				continue;

			MapNameAndPlaytime_t &mapPlaytime = mapPlaytimeList[mapPlaytimeList.AddToTail()];
			mapPlaytime.mapname = pszMapName;

			// See if we have stats on this map.
			int idx = m_reportedStats.m_dictMapStats.Find( pszMapName );
			if ( idx != m_reportedStats.m_dictMapStats.InvalidIndex() )
			{
				mapPlaytime.timeplayed = m_reportedStats.m_dictMapStats[idx].m_Header.m_iTotalTime;
			}
			else
			{
				mapPlaytime.timeplayed = 0;
			}
		}

		// Sort them by playtime in ascending order and add randomness for equal maps.
		mapPlaytimeList.Shuffle();
		mapPlaytimeList.Sort( SortMapPlaytime );

		// Add the desired number of maps to the options list.
		for ( int i = 0; i < mapPlaytimeList.Count() && i < iNumOptions; i++ )
		{
			optionList.CopyAndAddToTail( mapPlaytimeList[i].mapname );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Updates the stats of who has killed whom
//-----------------------------------------------------------------------------
void CTFGameStats::TrackKillStats( CBasePlayer *pAttacker, CBasePlayer *pVictim )
{
	int iPlayerIndexAttacker = pAttacker->entindex();
	int iPlayerIndexVictim = pVictim->entindex();

	PlayerStats_t &statsAttacker = m_aPlayerStats[iPlayerIndexAttacker];
	PlayerStats_t &statsVictim = m_aPlayerStats[iPlayerIndexVictim];

	statsVictim.statsKills.iNumKilledBy[iPlayerIndexAttacker]++;
	statsVictim.statsKills.iNumKilledByUnanswered[iPlayerIndexAttacker]++;
	statsAttacker.statsKills.iNumKilled[iPlayerIndexVictim]++;
	statsAttacker.statsKills.iNumKilledByUnanswered[iPlayerIndexVictim] = 0;

}

struct PlayerStats_t *CTFGameStats::FindPlayerStats( CBasePlayer *pPlayer )
{
	return &m_aPlayerStats[pPlayer->entindex()];
}


static void CC_ListDeaths( const CCommand &args )
{
	TF_Gamestats_LevelStats_t *map = CTF_GameStats.m_reportedStats.m_pCurrentGame;
	if ( !map )
		return;

	for( int i = 0; i < map->m_aPlayerDeaths.Count(); i++ )
	{
		Msg( "%s killed %s with %s at (%d,%d,%d), distance %d\n",
            g_aPlayerClassNames_NonLocalized[ map->m_aPlayerDeaths[ i ].iAttackClass ],
			g_aPlayerClassNames_NonLocalized[map->m_aPlayerDeaths[i].iTargetClass],
			WeaponIdToAlias( (ETFWeaponID)map->m_aPlayerDeaths[ i ].iWeapon ), 
			map->m_aPlayerDeaths[ i ].nPosition[ 0 ],
			map->m_aPlayerDeaths[ i ].nPosition[ 1 ],
			map->m_aPlayerDeaths[ i ].nPosition[ 2 ],
			map->m_aPlayerDeaths[ i ].iDistance );
	}

	Msg( "\n---------------------------------\n\n" );

	for( int i = 0; i < map->m_aPlayerDamage.Count(); i++ )
	{
		Msg( "%.2f : %s at (%d,%d,%d) caused %d damage to %s with %s at (%d,%d,%d)%s%s\n",
			map->m_aPlayerDamage[ i ].fTime,
			g_aPlayerClassNames_NonLocalized[map->m_aPlayerDamage[i].iAttackClass],
			map->m_aPlayerDamage[ i ].nAttackerPosition[ 0 ],
			map->m_aPlayerDamage[ i ].nAttackerPosition[ 1 ],
			map->m_aPlayerDamage[ i ].nAttackerPosition[ 2 ],
			map->m_aPlayerDamage[ i ].iDamage,
			g_aPlayerClassNames_NonLocalized[map->m_aPlayerDamage[i].iTargetClass],
			WeaponIdToAlias( (ETFWeaponID)map->m_aPlayerDamage[ i ].iWeapon ), 
			map->m_aPlayerDamage[ i ].nTargetPosition[ 0 ],
			map->m_aPlayerDamage[ i ].nTargetPosition[ 1 ],
			map->m_aPlayerDamage[ i ].nTargetPosition[ 2 ],
			map->m_aPlayerDamage[ i ].iCrit ? ", CRIT!" : "",
			map->m_aPlayerDamage[ i ].iKill ? ", KILL" : ""	);
	}

	Msg( "\n---------------------------------\n\n" );
	Msg( "listed %d deaths\n", map->m_aPlayerDeaths.Count() );
	Msg( "listed %d damages\n\n", map->m_aPlayerDamage.Count() );
}

static ConCommand listDeaths("listdeaths", CC_ListDeaths, "lists player deaths", FCVAR_CHEAT );
