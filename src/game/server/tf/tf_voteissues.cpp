//=============================================================================//
//
// Purpose:
//
//=============================================================================//
#include "cbase.h"
#include "tf_shareddefs.h"
#include "tf_voteissues.h"
#include "tf_gamerules.h"
#include "tf_gamestats.h"
#include "tf_team.h"

//-----------------------------------------------------------------------------
//
// Purpose: Kick vote
//
//-----------------------------------------------------------------------------
ConVar sv_vote_issue_kick_allowed( "sv_vote_issue_kick_allowed", "1", FCVAR_NONE, "Can players call votes to kick players from the server?" );
ConVar sv_vote_kick_ban_duration( "sv_vote_kick_ban_duration", "20", FCVAR_NONE, "The number of minutes a vote ban should last. (0 = Disabled)" );
ConVar sv_vote_issue_kick_namelock_duration( "sv_vote_issue_kick_namelock_duration", "120", FCVAR_NONE, "How long to prevent kick targets from changing their name (in seconds)." );

ConVar tf2c_vote_issue_change_civilian_allowed( "tf2c_vote_issue_change_civilian_allowed", "1", FCVAR_NONE, "Can players call votes to pick a new civilian from the server?" );
ConVar tf2c_vote_issue_change_civilian_teleport( "tf2c_vote_issue_change_civilian_teleport", "0", FCVAR_NONE, "Keep Civilian position instead of resetting it to spawn?" );

CKickIssue::CKickIssue( const char *pszTypeString ) : CBaseIssue( pszTypeString )
{
	Init();
}


void CKickIssue::Init()
{
	m_szPlayerName[0] = '\0';
	m_szSteamID[0] = '\0';
	m_iReason = 0;
	m_SteamID.Clear();
	m_hPlayerTarget = NULL;
	SetYesNoVoteCount( 0, 0, 0 );
}


const char *CKickIssue::GetDisplayString( void )
{
	switch ( m_iReason )
	{
		case KICK_REASON_IDLE:
			return "#TF_vote_kick_player_idle";
		case KICK_REASON_SCAMMING:
			return "#TF_vote_kick_player_scamming";
		case KICK_REASON_CHEATING:
			return "#TF_vote_kick_player_cheating";
		default:
			return "#TF_vote_kick_player_other";
	}
}


const char *CKickIssue::GetVotePassedString( void )
{
	if ( sv_vote_kick_ban_duration.GetInt() > 0 && m_SteamID.IsValid() )
		return "#TF_vote_passed_ban_player";

	return "#TF_vote_passed_kick_player";
}


const char *CKickIssue::GetDetailsString()
{
	CBasePlayer *pPlayer = m_hPlayerTarget.Get();
	if ( pPlayer )
		return pPlayer->GetPlayerName();

	if ( m_szPlayerName[0] != '\0' )
		return m_szPlayerName;

	return "Unnamed";
}


void CKickIssue::ListIssueDetails( CBasePlayer *pForWhom )
{
	if ( IsEnabled() )
	{
		char szBuf[64];
		V_sprintf_safe( szBuf, "callvote %s <userID>\n", GetTypeString() );
		ClientPrint( pForWhom, HUD_PRINTCONSOLE, szBuf );
	}
}


bool CKickIssue::IsEnabled()
{
	return sv_vote_issue_kick_allowed.GetBool();
}

void CKickIssue::NotifyGC( bool a2 )
{
	return;
}


int CKickIssue::PrintLogData()
{
	return 0;
}


int GetKickBanPlayerReason( const char *pszReason )
{
	if ( !V_strncmp( pszReason, "other", 5 ) )
		return CKickIssue::KICK_REASON_NONE;

	if ( !V_strncmp( pszReason, "cheating", 8 ) )
		return CKickIssue::KICK_REASON_CHEATING;

	if ( !V_strncmp( pszReason, "idle", 4 ) )
		return CKickIssue::KICK_REASON_IDLE;

	if ( !V_strncmp( pszReason, "scamming", 8 ) )
		return CKickIssue::KICK_REASON_SCAMMING;

	return CKickIssue::KICK_REASON_NONE;
}


bool CKickIssue::CreateVoteDataFromDetails( const char *pszDetails )
{
	int iPlayerID = 0;

	const char *pch;
	pch = strrchr( pszDetails, ' ' );
	if ( pch )
	{
		m_iReason = GetKickBanPlayerReason( pch + 1 );

		CUtlString string( pszDetails, pch + 1 - pszDetails );
		iPlayerID = atoi( string );
	}
	else
	{
		iPlayerID = atoi( pszDetails );
	}

	CBasePlayer *pPlayer = UTIL_PlayerByUserId( iPlayerID );
	if ( pPlayer )
	{
		m_hPlayerTarget = pPlayer;
		return true;
	}

	return false;
}


bool CKickIssue::CanCallVote( int nEntIndex, const char *pszDetails, vote_create_failed_t &nFailCode, int &nTime )
{
	if ( !CBaseIssue::CanCallVote( nEntIndex, pszDetails, nFailCode, nTime ) )
		return false;

	Init();

	if ( !IsEnabled() )
	{
		nFailCode = VOTE_FAILED_ISSUE_DISABLED;
		return false;
	}

	// Get the player and kick reason.
	if ( !CreateVoteDataFromDetails( pszDetails ) )
	{
		nFailCode = VOTE_FAILED_PLAYERNOTFOUND;
		return false;
	}

	CBasePlayer *pPlayer = m_hPlayerTarget.Get();
	Assert( pPlayer );

	// Can't votekick special bots.
	if ( pPlayer->IsHLTV() || pPlayer->IsReplay() )
		return false;

	// Can't votekick spectators.
	// TF2C: Stop malicious players from dodging votekick by staying in spectator
	int iTeam = pPlayer->GetTeamNumber();
	//if ( iTeam < FIRST_GAME_TEAM )
	//	return false;

	// Can't kick a player with RCON access or listen server host.
	if ( ( !engine->IsDedicatedServer() && pPlayer->entindex() == 1 ) || pPlayer->IsAutoKickDisabled() )
	{
		nFailCode = VOTE_FAILED_CANNOT_KICK_ADMIN;
		return false;
	}

	// Can't votekick developers.
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
	if ( pTFPlayer && pTFPlayer->PlayerIsDeveloper() )
	{
		return false;
	}

	if ( nEntIndex != DEDICATED_SERVER )
	{
		CBasePlayer *pCaller = UTIL_PlayerByIndex( nEntIndex );
		if ( !pCaller )
			return false;

		// Can only call votekick on teammates.
		if ( pCaller->GetTeamNumber() != iTeam )
			return false;
	}

	if ( !pPlayer->IsFakeClient() && ( !pPlayer->GetSteamID( &m_SteamID ) || !m_SteamID.IsValid() ) )
		return false;

#if 0
	if ( TFGameRules()->IsMannVsMachineMode() )
	{
		// Live TF2 has MvM specific fail codes here...
	}
#endif

	return true;
}


void CKickIssue::OnVoteStarted()
{
	CBasePlayer *pPlayer = GetTargetPlayer();
	if ( pPlayer )
	{
		// Remember their name and Steam ID string.
		// Live TF2 uses CSteamID::Render but we can't do that because we're missing some required library.
		V_strcpy_safe( m_szPlayerName, pPlayer->GetPlayerName() );
		V_strcpy( m_szSteamID, engine->GetPlayerNetworkIDString( pPlayer->edict() ) );

		if ( sv_vote_issue_kick_namelock_duration.GetFloat() > 0.0f )
		{
			g_voteController->AddPlayerToNameLockedList(
				m_SteamID,
				sv_vote_issue_kick_namelock_duration.GetFloat(),
				pPlayer->GetUserID() );
		}

		// Have the target player automatically vote No.
		g_voteController->TryCastVote( pPlayer->entindex(), "Option2" );
	}
}


void CKickIssue::OnVoteFailed( int iEntityHoldingVote )
{
	CBaseIssue::OnVoteFailed( iEntityHoldingVote );
	SetYesNoVoteCount( 0, 0, 0 );
	PrintLogData();
	NotifyGC( false );
}


void CKickIssue::ExecuteCommand()
{
	PrintLogData();

	CBasePlayer *pPlayer = GetTargetPlayer();
	if ( pPlayer )
	{
		// Kick them.
		engine->ServerCommand( UTIL_VarArgs( "kickid %d %s;", pPlayer->GetUserID(), "#TF_Vote_kicked" ) );
	}

	// Also ban their Steam ID if enabled.
	if ( sv_vote_kick_ban_duration.GetInt() > 0 && m_SteamID.IsValid() )
	{
		engine->ServerCommand( UTIL_VarArgs( "banid %d %s;", sv_vote_kick_ban_duration.GetInt(), m_szSteamID ) );
		g_voteController->AddPlayerToKickWatchList( m_SteamID, 60.0f * sv_vote_kick_ban_duration.GetFloat() );
	}
}


bool CKickIssue::IsTeamRestrictedVote()
{
	return true;
}


CBasePlayer *CKickIssue::GetTargetPlayer( void )
{
	CBasePlayer *pPlayer = m_hPlayerTarget.Get();
	if ( !pPlayer && m_SteamID.IsValid() )
	{
		pPlayer = UTIL_PlayerBySteamID( m_SteamID );
	}

	return pPlayer;
}


//-----------------------------------------------------------------------------
//
// Purpose: Restart map vote
//
//-----------------------------------------------------------------------------
ConVar sv_vote_issue_restart_game_allowed( "sv_vote_issue_restart_game_allowed", "0", FCVAR_NONE, "Can players call votes to restart the game?" );
ConVar sv_vote_issue_restart_game_cooldown( "sv_vote_issue_restart_game_cooldown", "300", FCVAR_NONE, "Minimum time before another restart vote can occur (in seconds)." );

CRestartGameIssue::CRestartGameIssue( const char *pszTypeString ) : CBaseIssue( pszTypeString )
{
}


const char *CRestartGameIssue::GetDisplayString( void )
{
	return "#TF_vote_restart_game";
}


const char *CRestartGameIssue::GetVotePassedString( void )
{
	return "#TF_vote_passed_restart_game";
}


void CRestartGameIssue::ListIssueDetails( CBasePlayer *pForWhom )
{
	if ( IsEnabled() )
	{
		ListStandardNoArgCommand( pForWhom, GetTypeString() );
	}
}


bool CRestartGameIssue::IsEnabled( void )
{
	return sv_vote_issue_restart_game_allowed.GetBool();
}


bool CRestartGameIssue::CanCallVote( int nEntIndex, const char *pszDetails, vote_create_failed_t &nFailCode, int &nTime )
{
	if ( CBaseIssue::CanCallVote( nEntIndex, pszDetails, nFailCode, nTime ) == false )
		return false;

	if ( !IsEnabled() )
	{
		nFailCode = VOTE_FAILED_ISSUE_DISABLED;
		return false;
	}

	return true;
}

extern ConVar mp_restartgame;


void CRestartGameIssue::ExecuteCommand( void )
{
	if ( sv_vote_issue_restart_game_cooldown.GetFloat() > 0.0f )
	{
		m_flNextCallTime = gpGlobals->curtime + sv_vote_issue_restart_game_cooldown.GetFloat();
	}

	mp_restartgame.SetValue( 1 );
}


//-----------------------------------------------------------------------------
//
// Purpose: Change map vote.
//
//-----------------------------------------------------------------------------
ConVar sv_vote_issue_changelevel_allowed( "sv_vote_issue_changelevel_allowed", "0", FCVAR_NONE, "Can players call votes to change levels?" );

CChangeLevelIssue::CChangeLevelIssue( const char *pszTypeString ) : CBaseIssue( pszTypeString )
{
}


const char *CChangeLevelIssue::GetDisplayString( void )
{
	return "#TF_vote_changelevel";
}


const char *CChangeLevelIssue::GetVotePassedString( void )
{
	return "#TF_vote_passed_changelevel";
}


const char *CChangeLevelIssue::GetDetailsString( void )
{
	return CBaseIssue::GetDetailsString();
}


void CChangeLevelIssue::ListIssueDetails( CBasePlayer *pForWhom )
{
	if ( IsEnabled() )
	{
		char szBuf[64];
		V_sprintf_safe( szBuf, "callvote %s <mapname>\n", GetTypeString() );
		ClientPrint( pForWhom, HUD_PRINTCONSOLE, szBuf );
	}
}


bool CChangeLevelIssue::IsEnabled( void )
{
	return sv_vote_issue_changelevel_allowed.GetBool();
}


bool CChangeLevelIssue::IsYesNoVote( void )
{
	return true;
}


bool CChangeLevelIssue::CanTeamCallVote( int iTeam )
{
	return true;
}

bool VotableMap( const char *pszMapName )
{
	char szBuf[MAX_MAP_NAME];
	V_strcpy_safe( szBuf, pszMapName );

	return ( engine->FindMap( szBuf, MAX_MAP_NAME ) == IVEngineServer::eFindMap_Found );
}


bool CChangeLevelIssue::CanCallVote( int nEntIndex, const char *pszDetails, vote_create_failed_t &nFailCode, int &nTime )
{
	if ( !CBaseIssue::CanCallVote( nEntIndex, pszDetails, nFailCode, nTime ) )
		return false;

	if ( !IsEnabled() )
	{
		nFailCode = VOTE_FAILED_ISSUE_DISABLED;
		return false;
	}

	if ( !Q_strcmp( pszDetails, "" ) )
	{
		nFailCode = VOTE_FAILED_MAP_NAME_REQUIRED;
		return false;
	}
	else
	{
		if ( !VotableMap( pszDetails ) )
		{
			nFailCode = VOTE_FAILED_MAP_NOT_FOUND;
			return false;
		}

		if ( MultiplayRules() && !MultiplayRules()->IsMapInMapCycle( pszDetails ) )
		{
			nFailCode = VOTE_FAILED_MAP_NOT_VALID;
			return false;
		}
	}

	return true;
}


void CChangeLevelIssue::ExecuteCommand( void )
{
	engine->ChangeLevel( GetDetailsString(), NULL );
}


//-----------------------------------------------------------------------------
//
// Purpose: Next map vote
//
//-----------------------------------------------------------------------------
ConVar sv_vote_issue_nextlevel_allowed( "sv_vote_issue_nextlevel_allowed", "1", FCVAR_NONE, "Can players call votes to set the next level?" );
ConVar sv_vote_issue_nextlevel_choicesmode( "sv_vote_issue_nextlevel_choicesmode", "0", FCVAR_NONE, "Present players with a list of lowest playtime maps to choose from?" );
ConVar sv_vote_issue_nextlevel_allowextend( "sv_vote_issue_nextlevel_allowextend", "1", FCVAR_NONE, "Allow players to extend the current map?" );
ConVar sv_vote_issue_nextlevel_prevent_change( "sv_vote_issue_nextlevel_prevent_change", "1", FCVAR_NONE, "Not allowed to vote for a nextlevel if one has already been set." );

CNextLevelIssue::CNextLevelIssue( const char *pszTypeString ) : CBaseIssue( pszTypeString )
{
}


const char *CNextLevelIssue::GetDisplayString( void )
{
	if ( IsInMapChoicesMode() )
		return "#TF_vote_nextlevel_choices";

	return "#TF_vote_nextlevel";
}


const char *CNextLevelIssue::GetVotePassedString( void )
{
	if ( sv_vote_issue_nextlevel_allowextend.GetBool() )
	{
		if ( !V_strcmp( GetDetailsString(), "Extend current Map" ) )
			return "#TF_vote_passed_nextlevel_extend";
	}

	return "#TF_vote_passed_nextlevel";
}


const char *CNextLevelIssue::GetDetailsString()
{
	return CBaseIssue::GetDetailsString();
}


void CNextLevelIssue::ListIssueDetails( CBasePlayer *pForWhom )
{
	if ( IsEnabled() )
	{
		char szBuf[64];
		V_sprintf_safe( szBuf, "callvote %s <mapname>\n", GetTypeString() );
		ClientPrint( pForWhom, HUD_PRINTCONSOLE, szBuf );
	}
}


bool CNextLevelIssue::IsEnabled( void )
{
	return sv_vote_issue_nextlevel_allowed.GetBool();
}


bool CNextLevelIssue::IsYesNoVote( void )
{
	return !IsInMapChoicesMode();
}


bool CNextLevelIssue::CanTeamCallVote( int iTeam )
{
	return true;
}


bool CNextLevelIssue::CanCallVote( int nEntIndex, const char *pszDetails, vote_create_failed_t &nFailCode, int &nTime )
{
	if ( !CBaseIssue::CanCallVote( nEntIndex, pszDetails, nFailCode, nTime ) )
		return false;

	if ( sv_vote_issue_nextlevel_choicesmode.GetBool() && nEntIndex == DEDICATED_SERVER )
	{
		// Invokes a UI down stream.
		if ( !Q_strcmp( pszDetails, "" ) )
			return true;

		return false;
	}
	
	if ( !IsEnabled() )
	{
		nFailCode = VOTE_FAILED_ISSUE_DISABLED;
		return false;
	}

	if ( !Q_strcmp( pszDetails, "" ) )
	{
		nFailCode = VOTE_FAILED_MAP_NAME_REQUIRED;
		return false;
	}
	else
	{
		if ( !VotableMap( pszDetails ) )
		{
			nFailCode = VOTE_FAILED_MAP_NOT_FOUND;
			return false;
		}

		if ( MultiplayRules() && !MultiplayRules()->IsMapInMapCycle( pszDetails ) )
		{
			nFailCode = VOTE_FAILED_MAP_NOT_VALID;
			return false;
		}
	}

	if ( sv_vote_issue_nextlevel_prevent_change.GetBool() )
	{
		if ( nextlevel.GetString() && *nextlevel.GetString() )
		{
			nFailCode = VOTE_FAILED_NEXTLEVEL_SET;
			return false;
		}
	}

	return true;
}


int CNextLevelIssue::GetNumberVoteOptions( void )
{
	return IsInMapChoicesMode() ? 5 : 2;
}


float CNextLevelIssue::GetQuorumRatio( void )
{
	if ( IsInMapChoicesMode() )
		return 0.1f;

	return CBaseIssue::GetQuorumRatio();
}


void CNextLevelIssue::ExecuteCommand( void )
{
	if ( !V_strcmp( GetDetailsString(), "Extend current Map" ) )
	{
		TFGameRules()->ExtendCurrentMap();
	}
	else
	{
		nextlevel.SetValue( GetDetailsString() );
	}
}


bool CNextLevelIssue::GetVoteOptions( CUtlVector <const char*> &vecNames )
{
	if ( !IsInMapChoicesMode() )
		return CBaseIssue::GetVoteOptions( vecNames );

	int iMapsNeeded = GetNumberVoteOptions();
	if ( sv_vote_issue_nextlevel_allowextend.GetBool() )
	{
		iMapsNeeded--;
	}

	// Get 4/5 maps with the smallest playtime.
	m_MapList.PurgeAndDeleteElements();
	CTF_GameStats.GetVoteData( "NextLevel", iMapsNeeded, m_MapList );
	Assert( m_MapList.Count() != 0 );

	for ( const char *pszName : m_MapList )
	{
		vecNames.AddToTail( pszName );
	}

	// We don't want to show a vote with less than 2 options.
	if ( sv_vote_issue_nextlevel_allowextend.GetBool() || vecNames.Count() < 2 )
	{
		vecNames.AddToTail( "Extend current Map" );
	}

	return true;
}


bool CNextLevelIssue::IsInMapChoicesMode( void )
{
	return ( !GetDetailsString()[0] && sv_vote_issue_nextlevel_choicesmode.GetBool() );
}


//-----------------------------------------------------------------------------
//
// Purpose: Extend map vote
//
//-----------------------------------------------------------------------------
ConVar sv_vote_issue_extendlevel_allowed( "sv_vote_issue_extendlevel_allowed", "1", FCVAR_NONE,  "Can players call votes to set the next level?" );
ConVar sv_vote_issue_extendlevel_quorum( "sv_vote_issue_extendlevel_quorum", "0.6", FCVAR_NONE, "What is the ratio of voters needed to reach quorum?" );

CExtendLevelIssue::CExtendLevelIssue( const char *pszTypeString ) : CBaseIssue( pszTypeString )
{
}


const char *CExtendLevelIssue::GetDisplayString( void )
{
	return "#TF_vote_extendlevel";
}


const char *CExtendLevelIssue::GetVotePassedString( void )
{
	return "#TF_vote_passed_nextlevel_extend";
}


void CExtendLevelIssue::ListIssueDetails( CBasePlayer *pForWhom )
{
	if ( IsEnabled() )
	{
		ListStandardNoArgCommand( pForWhom, GetTypeString() );
	}
}


bool CExtendLevelIssue::IsEnabled( void )
{
	return sv_vote_issue_extendlevel_allowed.GetBool();
}


bool CExtendLevelIssue::CanCallVote( int nEntIndex, const char *pszDetails, vote_create_failed_t &nFailCode, int &nTime )
{
	if ( !CBaseIssue::CanCallVote( nEntIndex, pszDetails, nFailCode, nTime ) )
		return false;

	if ( !IsEnabled() )
	{
		nFailCode = VOTE_FAILED_ISSUE_DISABLED;
		return false;
	}

	return true;
}


float CExtendLevelIssue::GetQuorumRatio( void )
{
	return sv_vote_issue_extendlevel_quorum.GetFloat();
}


void CExtendLevelIssue::ExecuteCommand( void )
{
	TFGameRules()->ExtendCurrentMap();
}


//-----------------------------------------------------------------------------
//
// Purpose: Restart map vote
//
//-----------------------------------------------------------------------------
ConVar sv_vote_issue_scramble_teams_allowed( "sv_vote_issue_scramble_teams_allowed", "1", FCVAR_NONE, "Can players call votes to scramble the teams?" );
ConVar sv_vote_issue_scramble_teams_cooldown( "sv_vote_issue_scramble_teams_cooldown", "1200", FCVAR_NONE, "Minimum time before another scramble vote can occur (in seconds)." );

CScrambleTeams::CScrambleTeams( const char *pszTypeString ) : CBaseIssue( pszTypeString )
{
}


const char *CScrambleTeams::GetDisplayString( void )
{
	return "#TF_vote_scramble_teams";
}


const char *CScrambleTeams::GetVotePassedString( void )
{
	return "#TF_vote_passed_scramble_teams";
}


void CScrambleTeams::ListIssueDetails( CBasePlayer *pForWhom )
{
	if ( IsEnabled() )
	{
		ListStandardNoArgCommand( pForWhom, GetTypeString() );
	}
}


bool CScrambleTeams::IsEnabled( void )
{
	return sv_vote_issue_scramble_teams_allowed.GetBool();
}


bool CScrambleTeams::CanCallVote( int nEntIndex, const char *pszDetails, vote_create_failed_t &nFailCode, int &nTime )
{
	if ( CBaseIssue::CanCallVote( nEntIndex, pszDetails, nFailCode, nTime ) == false )
		return false;

	if ( !IsEnabled() )
	{
		nFailCode = VOTE_FAILED_ISSUE_DISABLED;
		return false;
	}

	if ( TeamplayGameRules()->ShouldScrambleTeams() )
	{
		// Scramble already scheduled.
		nFailCode = VOTE_FAILED_SCRAMBLE_IN_PROGRESS;
		return false;
	}

	return true;
}


void CScrambleTeams::ExecuteCommand( void )
{
	if ( sv_vote_issue_scramble_teams_cooldown.GetFloat() > 0.0f )
	{
		m_flNextCallTime = gpGlobals->curtime + sv_vote_issue_scramble_teams_cooldown.GetFloat();
	}

	engine->ServerCommand( "mp_scrambleteams 2;" );
}

// Team Fortress 2 Classic

// VIP

CChangeCivilian::CChangeCivilian( const char *pszTypeString ) : CBaseIssue( pszTypeString )
{
	Init();
}


void CChangeCivilian::Init()
{
	m_szPlayerName[0] = '\0';
	m_szSteamID[0] = '\0';
	m_SteamID.Clear();
	m_hPlayerTarget = NULL;
	m_iCallerID = -1;
	m_iCallingTeam = TEAM_UNASSIGNED;
	SetYesNoVoteCount( 0, 0, 0 );
}


const char *CChangeCivilian::GetDisplayString( void )
{
	CBasePlayer *pPlayer = GetTargetPlayer();
	if ( m_iCallerID != -1 )
	{
		CBasePlayer *pCaller = UTIL_PlayerByIndex( m_iCallerID );
		if ( pCaller == pPlayer )
			return "#TF_vote_change_civilian_caller";
	}

	return "#TF_vote_change_civilian";
}


const char *CChangeCivilian::GetVotePassedString( void )
{
	return "#TF_vote_passed_change_civilian";
}


const char *CChangeCivilian::GetDetailsString()
{
	CBasePlayer *pPlayer = m_hPlayerTarget.Get();
	if ( pPlayer )
		return pPlayer->GetPlayerName();

	if ( m_szPlayerName[0] != '\0' )
		return m_szPlayerName;

	return "Unnamed";
}


void CChangeCivilian::ListIssueDetails( CBasePlayer *pForWhom )
{
	if ( IsEnabled() )
	{
		char szBuf[64];
		V_sprintf_safe( szBuf, "callvote %s <userID>\n", GetTypeString() );
		ClientPrint( pForWhom, HUD_PRINTCONSOLE, szBuf );
	}
}


bool CChangeCivilian::IsEnabled()
{
	CTFGameRules *pTFGameRules = TFGameRules();
	return ( pTFGameRules && pTFGameRules->IsVIPMode() ) && tf2c_vote_issue_change_civilian_allowed.GetBool();
}

void CChangeCivilian::NotifyGC( bool a2 )
{
	return;
}


int CChangeCivilian::PrintLogData()
{
	return 0;
}


bool CChangeCivilian::CreateVoteDataFromDetails( const char *pszDetails )
{
	int iPlayerID = 0;

	const char *pch;
	pch = strrchr( pszDetails, ' ' );
	if ( pch )
	{
		CUtlString string( pszDetails, pch + 1 - pszDetails );
		iPlayerID = atoi( string );
	}
	else
	{
		iPlayerID = atoi( pszDetails );
	}

	CBasePlayer *pPlayer = UTIL_PlayerByUserId( iPlayerID );
	if ( pPlayer )
	{
		m_hPlayerTarget = pPlayer;
		return true;
	}

	return false;
}


bool CChangeCivilian::CanCallVote( int nEntIndex, const char *pszDetails, vote_create_failed_t &nFailCode, int &nTime )
{
	if ( !CBaseIssue::CanCallVote( nEntIndex, pszDetails, nFailCode, nTime ) )
		return false;

	Init();

	if ( !IsEnabled() )
	{
		nFailCode = VOTE_FAILED_ISSUE_DISABLED;
		return false;
	}

	// Get the player and reason.
	if ( !CreateVoteDataFromDetails( pszDetails ) )
	{
		nFailCode = VOTE_FAILED_PLAYERNOTFOUND;
		return false;
	}

	CBasePlayer *pPlayer = m_hPlayerTarget.Get();
	Assert( pPlayer );

	// Can't become a VIP again, dummy.
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
	if ( pTFPlayer && pTFPlayer->IsVIP() )
		return false;

	// Only the team that has the VIP can call this.
	CTFTeam *pTeam = GetGlobalTFTeam( pPlayer->GetTeamNumber() );
	if ( pTeam->GetVIP() == NULL )
	{
		nFailCode = VOTE_FAILED_TEAM_CANT_CALL;
		return false;
	}

	// Can't chose special bots.
	if ( pPlayer->IsHLTV() || pPlayer->IsReplay() )
		return false;

	// Can't choose spectators.
	if ( pPlayer->GetTeamNumber() < FIRST_GAME_TEAM )
		return false;

	if ( nEntIndex != DEDICATED_SERVER )
	{
		CBasePlayer *pCaller = UTIL_PlayerByIndex( nEntIndex );
		if ( !pCaller )
			return false;

		m_iCallerID = nEntIndex;

		// Can only choose teammates.
		int iTeam = pCaller->GetTeamNumber();
		if ( iTeam != pPlayer->GetTeamNumber() )
			return false;

		m_iCallingTeam = iTeam;
	}
	else
	{
		m_iCallingTeam = pPlayer->GetTeamNumber();
	}

	if ( !pPlayer->IsFakeClient() && ( !pPlayer->GetSteamID( &m_SteamID ) || !m_SteamID.IsValid() ) )
		return false;

	return true;
}


void CChangeCivilian::OnVoteStarted()
{
	CBasePlayer *pPlayer = GetTargetPlayer();
	if ( pPlayer )
	{
		// Remember their name and Steam ID string.
		// Live TF2 uses CSteamID::Render but we can't do that because we're missing some required library.
		V_strcpy_safe( m_szPlayerName, pPlayer->GetPlayerName() );
		V_strcpy( m_szSteamID, engine->GetPlayerNetworkIDString( pPlayer->edict() ) );
	}
}


void CChangeCivilian::OnVoteFailed( int iEntityHoldingVote )
{
	CBaseIssue::OnVoteFailed( iEntityHoldingVote );
	SetYesNoVoteCount( 0, 0, 0 );
	PrintLogData();
	NotifyGC( false );
}


void CChangeCivilian::ExecuteCommand()
{
	PrintLogData();

	CTFPlayer *pPlayer = ToTFPlayer( GetTargetPlayer() );
	if ( pPlayer )
	{
		int iTeam = pPlayer->GetTeamNumber();
		if ( iTeam == m_iCallingTeam )
		{
			CTFTeam *pTeam = GetGlobalTFTeam( iTeam );
			CTFPlayer *pVIPPlayer = pTeam ? pTeam->GetVIP() : NULL;
			if ( pVIPPlayer )
			{
				pVIPPlayer->ReturnFromVIP();
			}

			pPlayer->BecomeVIP();

			if ( pPlayer->IsAlive() )
			{
				pPlayer->ForceRespawn();
			}

			if ( pVIPPlayer && pVIPPlayer->IsAlive() )
			{
				if ( tf2c_vote_issue_change_civilian_teleport.GetBool() )
				{
					pPlayer->SetHealth( pVIPPlayer->GetHealth() );
					pPlayer->Teleport(  &pVIPPlayer->GetAbsOrigin(), 
										&pVIPPlayer->EyeAngles(), 
										&pVIPPlayer->GetAbsVelocity() );
				}

				pVIPPlayer->ForceRespawn();
			}

			return;
		}
	}
	
	Warning( "Vote target '%s' missing or on another team, the VIP will remain unchanged!\n", m_szPlayerName );
}


bool CChangeCivilian::IsTeamRestrictedVote()
{
	return true;
}


CBasePlayer *CChangeCivilian::GetTargetPlayer( void )
{
	CBasePlayer *pPlayer = m_hPlayerTarget.Get();
	if ( !pPlayer && m_SteamID.IsValid() )
	{
		pPlayer = UTIL_PlayerBySteamID( m_SteamID );
	}

	return pPlayer;
}