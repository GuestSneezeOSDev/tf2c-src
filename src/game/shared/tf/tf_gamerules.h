//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: The TF Game rules object
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================

#ifndef TF_GAMERULES_H
#define TF_GAMERULES_H

#ifdef _WIN32
#pragma once
#endif


#include "teamplay_gamerules.h"
#include "tf_round_timer.h"
#include "convar.h"
#include "gamevars_shared.h"
#include "GameEventListener.h"
#include "tf_gamestats_shared.h"
#include "tf_shareddefs.h"

#ifdef CLIENT_DLL
#include "c_tf_player.h"
#else
#include "tf_control_point.h"
#include "tf_player.h"
#endif

#ifdef CLIENT_DLL

#define CTFGameRules C_TFGameRules
#define CTFGameRulesProxy C_TFGameRulesProxy

#else

class CHealthKit;
class CTeamTrainWatcher;
class CTFBotRoster;

extern ConVar mp_stalemate_timelimit;

#endif

extern ConVar tf_avoidteammates;
extern ConVar tf_avoidteammates_pushaway;
extern ConVar mp_stalemate_meleeonly;
extern ConVar tf_arena_use_queue;

extern ConVar tf2c_loadinginfo_mapname;
extern ConVar tf2c_loadinginfo_gametype;
extern ConVar tf2c_loadinginfo_fourteam;

extern Vector g_TFClassViewVectors[];

class CTFGameRules;

//-----------------------------------------------------------------------------
// Round states
//-----------------------------------------------------------------------------
enum gamerules_roundstate_t
{
	// initialize the game, create teams
	GR_STATE_INIT = 0,

	//Before players have joined the game. Periodically checks to see if enough players are ready
	//to start a game. Also reverts to this when there are no active players
	GR_STATE_PREGAME,

	//The game is about to start, wait a bit and spawn everyone
	GR_STATE_STARTGAME,

	//All players are respawned, frozen in place
	GR_STATE_PREROUND,

	//Round is on, playing normally
	GR_STATE_RND_RUNNING,

	//Someone has won the round
	GR_STATE_TEAM_WIN,

	//Noone has won, manually restart the game, reset scores
	GR_STATE_RESTART,

	//Noone has won, restart the game
	GR_STATE_STALEMATE,

	//Game is over, showing the scoreboard etc
	GR_STATE_GAME_OVER,

	//Game is in a bonus state, transitioned to after a round ends
	GR_STATE_BONUS,

	//Game is awaiting the next wave/round of a multi round experience
	GR_STATE_BETWEEN_RNDS,

	GR_NUM_ROUND_STATES
};

enum {
	WINREASON_NONE = 0,
	WINREASON_ALL_POINTS_CAPTURED,
	WINREASON_OPPONENTS_DEAD,
	WINREASON_FLAG_CAPTURE_LIMIT,
	WINREASON_DEFEND_UNTIL_TIME_LIMIT,
	WINREASON_STALEMATE,
	WINREASON_TIMELIMIT,
	WINREASON_WINLIMIT,
	WINREASON_WINDIFFLIMIT,
	WINREASON_RD_REACTOR_CAPTURED,
	WINREASON_RD_CORES_COLLECTED,
	WINREASON_RD_REACTOR_RETURNED,
	// TF2C
	WINREASON_VIP_ESCAPED,
	WINREASON_ROUNDSCORELIMIT,
	WINREASON_VIP_KILLED,
	
	WINREASON_COUNT
};

enum stalemate_reasons_t
{
	STALEMATE_JOIN_MID,
	STALEMATE_TIMER,
	STALEMATE_SERVER_TIMELIMIT,

	NUM_STALEMATE_REASONS,
};


#if defined(TF_CLIENT_DLL) || defined(TF_DLL)

/// Info about a player in a PVE game or any other mode that we
/// might eventually decide to use the lobby system for.
struct LobbyPlayerInfo_t
{
	int m_nEntNum; //< Index of player (1...MAX_PLAYERS), or 0 if the guy is in the lobby but not yet known to us
	CUtlString m_sPlayerName; //< Player display name
	CSteamID m_steamID; //< Steam ID of the player
	int m_iTeam; //< Team selection.
	bool m_bInLobby; //< Is this guy in the lobby?
	bool m_bConnected; //< Is this a bot?
	bool m_bBot; //< Is this a bot?
	bool m_bSquadSurplus; //< Did he present a voucher to get surplus for his squad
};

#endif

//-----------------------------------------------------------------------------
// Purpose: Per-state data
//-----------------------------------------------------------------------------
class CGameRulesRoundStateInfo
{
public:
	gamerules_roundstate_t	m_iRoundState;
	const char				*m_pStateName;

	void ( CTFGameRules::*pfnEnterState )( );	// Init and deinit the state.
	void ( CTFGameRules::*pfnLeaveState )( );
	void ( CTFGameRules::*pfnThink )( );	// Do a PreThink() in this state.
};

#ifdef GAME_DLL
class CTFRadiusDamageInfo
{
public:
	CTFRadiusDamageInfo();

	bool	ApplyToEntity( CBaseEntity *pEntity );

public:
	CTakeDamageInfo info;
	Vector m_vecSrc;
	float m_flRadius;
	float m_flSelfDamageRadius;
	int m_iClassIgnore;
	CBaseEntity *m_pEntityIgnore;
	bool m_bStockSelfDamage;
};

class CTFRadiusHealingInfo
{
public:
	CTFRadiusHealingInfo();

	bool	ApplyToEntity(CBaseEntity *pEntity);

public:
	CBaseEntity		*m_hPlayerResponsible;
	float			m_flHealingAmountRadial;
	float			m_flHealingAmountDirect;
	float			m_flOverhealMultiplier;		// A multiplicative penalty applied to overheal dealt.
	Vector			m_vecSrc;
	float			m_flRadius;
	float			m_flSelfHealRadius;
	int				m_iClassIgnore;
	CBaseEntity		*m_hEntityIgnore;
	CBaseEntity		*m_hEntityDirectlyHit;
	bool			m_bDirectHitWasEnemy;
	bool			m_bUseFalloffCalcs;
	ETFCond			m_condConditionsToApply;
	float			m_flConditionDuration;

	// If a lingering heal effect is desired, specify so with these params:
	bool			m_bApplyResidualHeal;
	float			m_flResidualHealRate;
	float			m_flResidualHealDuration;

	bool			m_bSelfHeal;			// I healed myself.... today....
};

struct KillingWeaponData_t
{
	KillingWeaponData_t()
	{
		szWeaponName[0] = '\0';
		szWeaponLogName[0] = '\0';
		iWeaponID = TF_WEAPON_NONE;
	}

	char szWeaponName[128];
	char szWeaponLogName[128];
	ETFWeaponID iWeaponID;
};
#endif

class CTFGameRulesProxy : public CGameRulesProxy, public CGameEventListener
{
public:
	DECLARE_CLASS( CTFGameRulesProxy, CGameRulesProxy );
	DECLARE_NETWORKCLASS();

#ifdef GAME_DLL
	DECLARE_DATADESC();

	CTFGameRulesProxy();
	~CTFGameRulesProxy();

	friend class CTFGameRules;

	virtual void FireGameEvent( IGameEvent *event );

	void	InputSetStalemateOnTimelimit( inputdata_t &inputdata );
	void	InputSetRedTeamRespawnWaveTime( inputdata_t &inputdata );
	void	InputSetBlueTeamRespawnWaveTime( inputdata_t &inputdata );
	void	InputSetGreenTeamRespawnWaveTime( inputdata_t &inputdata );
	void	InputSetYellowTeamRespawnWaveTime( inputdata_t &inputdata );
	void	InputAddRedTeamRespawnWaveTime( inputdata_t &inputdata );
	void	InputAddBlueTeamRespawnWaveTime( inputdata_t &inputdata );
	void	InputAddGreenTeamRespawnWaveTime( inputdata_t &inputdata );
	void	InputAddYellowTeamRespawnWaveTime( inputdata_t &inputdata );
	void	InputSetRedTeamGoalString( inputdata_t &inputdata );
	void	InputSetBlueTeamGoalString( inputdata_t &inputdata );
	void	InputSetGreenTeamGoalString( inputdata_t &inputdata );
	void	InputSetYellowTeamGoalString( inputdata_t &inputdata );
	void	InputSetRedTeamRole( inputdata_t &inputdata );
	void	InputSetBlueTeamRole( inputdata_t &inputdata );
	void	InputSetGreenTeamRole( inputdata_t &inputdata );
	void	InputSetYellowTeamRole( inputdata_t &inputdata );
	void	InputSetRequiredObserverTarget( inputdata_t &inputdata );
	void	InputAddRedTeamScore( inputdata_t &inputdata );
	void	InputAddBlueTeamScore( inputdata_t &inputdata );
	void	InputAddGreenTeamScore( inputdata_t &inputdata );
	void	InputAddYellowTeamScore( inputdata_t &inputdata );
	void	InputSetRedTeamScore( inputdata_t &inputdata );
	void	InputSetBlueTeamScore( inputdata_t &inputdata );
	void	InputSetGreenTeamScore( inputdata_t &inputdata );
	void	InputSetYellowTeamScore( inputdata_t &inputdata );

	void	InputSetRedKothClockActive( inputdata_t &inputdata );
	void	InputSetBlueKothClockActive( inputdata_t &inputdata );
	void	InputSetGreenKothClockActive( inputdata_t &inputdata );
	void	InputSetYellowKothClockActive( inputdata_t &inputdata );
	void	SetKothClockActive( int iActiveTeam, inputdata_t &inputdata );

	void	InputSetCTFCaptureBonusTime( inputdata_t &inputdata );

	void	InputPlayVO( inputdata_t &inputdata );
	void	InputPlayVORed( inputdata_t &inputdata );
	void	InputPlayVOBlue( inputdata_t &inputdata );
	void	InputPlayVOGreen( inputdata_t &inputdata );
	void	InputPlayVOYellow( inputdata_t &inputdata );

	void	InputHandleMapEvent( inputdata_t &inputdata );
	void	InputSetRoundRespawnFreezeEnabled( inputdata_t &inputdata );

	void	Activate();

private:
	int		m_iHud_Type;
	bool	m_bFourTeamMode;
	bool	m_bCTF_Overtime;
	int		m_iPointLimit;

	COutputEvent m_OnWonByTeam1;
	COutputEvent m_OnWonByTeam2;
	COutputEvent m_OnWonByTeam3;
	COutputEvent m_OnWonByTeam4;
	COutputEvent m_Team1PlayersChanged;
	COutputEvent m_Team2PlayersChanged;
	COutputEvent m_Team3PlayersChanged;
	COutputEvent m_Team4PlayersChanged;
	COutputEvent m_OnStateEnterBetweenRounds;
	COutputEvent m_OnStateEnterPreRound;
	COutputEvent m_OnStateExitPreRound;
	COutputEvent m_OnStateEnterRoundRunning;

	COutputEvent m_OnOvertime;
	COutputEvent m_OnSuddenDeath;

#endif

	//----------------------------------------------------------------------------------
	// Client specific
#ifdef CLIENT_DLL
public:
	virtual void	OnPreDataChanged( DataUpdateType_t updateType );
	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual void	FireGameEvent( IGameEvent *event ) {}
#endif // CLIENT_DLL
};

struct PlayerRoundScore_t
{
	int iPlayerIndex;	// player index
	int iRoundScore;	// how many points scored this round
	int	iTotalScore;	// total points scored across all rounds
	int	iKills;
	int iDeaths;
};

struct ArenaPlayerRoundScore_t
{
	int iPlayerIndex;	// player index
	int iRoundScore;	// how many points scored this round
	int iDamage;
	int iHealing;
	int iLifeTime;
	int iKills;
};

#define MAX_TEAMGOAL_STRING		256

class CTFGameRules : public CTeamplayRules, public CGameEventListener
{
	DECLARE_CLASS( CTFGameRules, CTeamplayRules );
	DECLARE_NETWORKCLASS_NOBASE();

public:
	CTFGameRules();

	enum
	{
		HALLOWEEN_SCENARIO_DOOMSDAY
	};

	// Damage Queries.
	bool			Damage_IsTimeBased( int iDmgType );			// Damage types that are time-based.
	bool			Damage_ShowOnHUD( int iDmgType );				// Damage types that have client HUD art.
	bool			Damage_ShouldNotBleed( int iDmgType );			// Damage types that don't make the player bleed.
	// TEMP:
	int				Damage_GetTimeBased( void );
	int				Damage_GetShowOnHud( void );
	int				Damage_GetShouldNotBleed( void );

	float			GetLastRoundStateChangeTime( void ) const { return m_flLastRoundStateChangeTime; }
	float			m_flLastRoundStateChangeTime;

	// Data accessors
	inline gamerules_roundstate_t State_Get( void ) { return m_iRoundState; }
	bool			IsInWaitingForPlayers( void ) { return m_bInWaitingForPlayers; }
	bool			InRoundRestart( void ) { return State_Get() == GR_STATE_PREROUND; }
	bool			InStalemate( void ) { return State_Get() == GR_STATE_STALEMATE && !IsInArenaMode(); }
	bool			InStalemateMeleeOnly( void ) { return ( InStalemate() && mp_stalemate_meleeonly.GetBool() ); }
	bool			RoundHasBeenWon( void ) { return State_Get() == GR_STATE_TEAM_WIN; }
	
	int				GetFarthestOwnedControlPoint( int iTeam, bool bWithSpawnpoints );

	// Return the value of this player towards capturing a point
	int				GetCaptureValueForPlayer( CBasePlayer *pPlayer );
	bool			TeamMayCapturePoint( int iTeam, int iPointIndex );
	bool			PlayerMayCapturePoint( CBasePlayer *pPlayer, int iPointIndex, char *pszReason = NULL, int iMaxReasonLength = 0 );
	bool			PlayerMayBlockPoint( CBasePlayer *pPlayer, int iPointIndex, char *pszReason = NULL, int iMaxReasonLength = 0 );

	// Return false if players aren't allowed to cap points at this time (i.e. in WaitingForPlayers)
	bool			PointsMayBeCaptured( void );
	void			SetLastCapPointChanged( int iIndex ) { m_iLastCapPointChanged = iIndex; }
	int				GetLastCapPointChanged( void ) { return m_iLastCapPointChanged; }

	float			GetNextRespawnWave( int iTeam, CBasePlayer *pPlayer );
	bool			HasPassedMinRespawnTime( CBasePlayer *pPlayer );
	void			LevelInitPostEntity( void );
	float			GetRespawnTimeScalar( int iTeam );
	float			GetRespawnWaveMaxLength( int iTeam, bool bScaleWithNumPlayers = true );
	bool			ShouldRespawnQuickly( CBasePlayer *pPlayer ) { return false; }
	float			GetMinTimeWhenPlayerMaySpawn( CBasePlayer *pPlayer );

	int				GetWinningTeam( void ) { return m_iWinningTeam; }
	int				GetWinReason() { return m_iWinReason; }
	bool			InOvertime( void ) { return m_bInOvertime; }
	bool			InSetup( void ) { return m_bInSetup; }

	bool			SwitchedTeamsThisRound( void ) { return m_bSwitchedTeamsThisRound; }
	bool			ShouldBalanceTeams( void );
	bool			WouldChangeUnbalanceTeams( int iNewTeam, int iCurrentTeam );
	bool			AreTeamsUnbalanced( int &iHeaviestTeam, int &iLightestTeam );
	bool			IsInTournamentMode( void );
	bool			IsInHighlanderMode( void );
	bool			IsInPreMatch( void ) { return ( IsInTournamentMode() && IsInWaitingForPlayers() ); }
	bool			IsWaitingForTeams( void ) { return m_bAwaitingReadyRestart; }
	bool			IsInStopWatch( void ) { return m_bStopWatch; }
	int				GetStopWatchState( void ) const { return m_bStopWatch; } // TODO
	void			SetInStopWatch( bool bState ) { m_bStopWatch = bState; }
	void			StopWatchModeThink( void ) { }

	bool IsTeamReady( int iTeamNumber )
	{
		return m_bTeamReady[iTeamNumber];
	}

	bool IsPlayerReady( int iIndex )
	{
		return m_bPlayerReady[iIndex];
	}

	void			HandleTeamScoreModify( int iTeam, int iScore ) {  }

	float			GetRoundRestartTime( void ) { return m_flRestartRoundTime; }
	int				GetBonusRoundTime( bool bFinal = false );

#if defined( TF_CLIENT_DLL ) || defined( TF_DLL )
	// Get list of all the players, including those in the lobby but who have
	// not yet joined.
	void GetAllPlayersLobbyInfo( CUtlVector<LobbyPlayerInfo_t> &vecPlayers, bool bIncludeBots = false );

	// Get list of players who are on the defending team now, or are likely
	// to end up on the defending team (not yet connected or assigned a team)
	void GetPotentialPlayersLobbyPlayerInfo( CUtlVector<LobbyPlayerInfo_t> &vecLobbyPlayers, bool bIncludeBots = false );
#endif

	void			SetAllowBetweenRounds( bool bValue ) { m_bAllowBetweenRounds = bValue; }
	bool			HaveCheatsBeenEnabledDuringLevel( void ) { return m_bCheatsEnabledDuringLevel; }

	static int		CalcPlayerScore( RoundStats_t *pRoundStats );
	static int		CalcPlayerSupportScore( RoundStats_t *pRoundStats, int iPlayerIndex );

	bool			IsBirthday( void );
	bool			IsHolidayActive( int eHoliday ) const;
	bool			IsHolidayMap( int eHoliday );

	const unsigned char *GetEncryptionKey( void ) { return GetTFEncryptionKey(); }

	// This also exists for the client to have access to (Discord Rich Presence).
	bool			IsValveMap( void );

	virtual void RegisterScriptFunctions( void );

#ifdef GAME_DLL
public:
	bool			TimerMayExpire( void );
	void			HandleSwitchTeams( void );
	void			HandleScrambleTeams( void );

	// Override this to prevent removal of game specific entities that need to persist
	bool			RoundCleanupShouldIgnore( CBaseEntity *pEnt );
	bool			ShouldCreateEntity( const char *pszClassName );

	void			RunPlayerConditionThink( void );
	void			FrameUpdatePostEntityThink();

	// Called when a new round is being initialized
	void			SetupOnRoundStart( void );

	// Called when a new round is off and running
	void			SetupOnRoundRunning( void );

	void			PreRound_End( void );

	// Called before a new round is started (so the previous round can end)
	void			PreviousRoundEnd( void );

	// Send the team scores down to the client
	void			SendTeamScoresEvent( void ) { return; }

	// Send the end of round info displayed in the win panel
	void			SendWinPanelInfo( void );
	void			SendArenaWinPanelInfo( void );

	// Setup spawn points for the current round before it starts
	void			SetupSpawnPointsForRound( void );

	// Called when a round has entered stalemate mode (timer has run out)
	void			SetupOnStalemateStart( void );
	void			SetupOnStalemateEnd( void );
	void			SetSetup( bool bSetup );

	bool			ShouldGoToBonusRound( void ) { return false; }
	void			SetupOnBonusStart( void ) { return; }
	void			SetupOnBonusEnd( void ) { return; }
	void			BonusStateThink( void ) { return; }

	void			BetweenRounds_Start( void ) { return; }
	void			BetweenRounds_End( void ) { return; }
	void			BetweenRounds_Think( void ) { return; }

	bool			PrevRoundWasWaitingForPlayers() { return m_bPrevRoundWasWaitingForPlayers; }

	bool			ShouldScorePerRound( void );

	bool			CheckNextLevelCvar( bool bAllowEnd = true );

	void			RestartTournament( void );

	bool			TournamentModeCanEndWithTimelimit( void ) { return true; }

public:
	void			State_Transition( gamerules_roundstate_t newState );

	void			RespawnPlayers( bool bForceRespawn, bool bTeam = false, int iTeam = TEAM_UNASSIGNED );

	void			SetForceMapReset( bool reset );

	void			SetRoundToPlayNext( string_t strName ){ m_iszRoundToPlayNext = strName; }
	string_t		GetRoundToPlayNext( void ){ return m_iszRoundToPlayNext; }
	void			AddPlayedRound( string_t strName );
	bool			IsPreviouslyPlayedRound( string_t strName );
	string_t		GetLastPlayedRound( void );

	void			SetWinningTeam( int team, int iWinReason, bool bForceMapReset = true, bool bSwitchTeams = false, bool bDontAddScore = false, bool bFinal = false ) OVERRIDE;
	void			SetStalemate( int iReason, bool bForceMapReset = true, bool bSwitchTeams = false );

	void			SetRoundOverlayDetails( void );

	float			GetWaitingForPlayersTime( void ) { return mp_waitingforplayers_time.GetFloat(); }
	void			ShouldResetScores( bool bResetTeam, bool bResetPlayer ){ m_bResetTeamScores = bResetTeam; m_bResetPlayerScores = bResetPlayer; }
	void			ShouldResetRoundsPlayed( bool bResetRoundsPlayed ){ m_bResetRoundsPlayed = bResetRoundsPlayed; }

	void			SetFirstRoundPlayed( string_t strName ){ m_iszFirstRoundPlayed = strName; }
	string_t		GetFirstRoundPlayed(){ return m_iszFirstRoundPlayed; }

	void			SetTeamRespawnWaveTime( int iTeam, float flValue );
	void			AddTeamRespawnWaveTime( int iTeam, float flValue );
	void			FillOutTeamplayRoundWinEvent( IGameEvent *event );

	void			SetStalemateOnTimelimit( bool bStalemate ) { m_bAllowStalemateAtTimelimit = bStalemate; }
	
	void			SetCTFOvertime( bool bEnabled ) { m_bCTF_Overtime = bEnabled; }

	bool			IsGameUnderTimeLimit( void );

	CTeamRoundTimer *GetActiveRoundTimer( void );

	void			HandleTimeLimitChange( void );

	void SetTeamReadyState( bool bState, int iTeam )
	{
		m_bTeamReady.Set( iTeam, bState );
	}

	void SetPlayerReadyState( int iIndex, bool bState )
	{
		m_bPlayerReady.Set( iIndex, bState );
	}
	void			ResetPlayerAndTeamReadyState( void );

	void			PlayTrainCaptureAlert( CTeamControlPoint *pPoint, bool bFinalPointInMap );

	void			PlaySpecialCapSounds( int iCappingTeam, CTeamControlPoint *pPoint );

	bool			PlayThrottledAlert( int iTeam, const char *sound, float fDelayBeforeNext );

	void			BroadcastSound( int iTeam, const char *sound, int iAdditionalSoundFlags = 0 );
	int				GetRoundsPlayed( void ) { return m_nRoundsPlayed; }

	void			RecalculateControlPointState( void );

	bool			ShouldSkipAutoScramble( void );

	bool			ShouldWaitToStartRecording( void ){ return IsInWaitingForPlayers(); }

	void			SetOvertime( bool bOvertime );

	void			Activate();

	bool			AllowDamage( CBaseEntity *pVictim, const CTakeDamageInfo &info );

protected:
	void			CheckChatText( CBasePlayer *pPlayer, char *pText );
	void			CheckChatForReadySignal( CBasePlayer *pPlayer, const char *chatmsg );

	SERVERONLY_EXPORT void	SetInWaitingForPlayers( bool bWaitingForPlayers );
	void			CheckWaitingForPlayers( void );
	bool			AllowWaitingForPlayers( void ) { return true; }
	void			CheckRestartRound( void );
	bool			CheckTimeLimit( bool bAllowEnd = true );
	bool			CheckWinLimit( bool bAllowEnd = true );
	bool			CheckMaxRounds( bool bAllowEnd = true );
	int				GetWinsRemaining( void );
	int				GetRoundsRemaining( void );

	void			CheckReadyRestart( void );
#if defined( TF_CLIENT_DLL ) || defined( TF_DLL )
	bool			AreLobbyPlayersOnTeamReady( int iTeam );
	bool			AreLobbyPlayersConnected( void );
#endif

	bool			CanChangelevelBecauseOfTimeLimit( void );
	bool			CanGoToStalemate( void );

	// State machine handling
	void			State_Enter( gamerules_roundstate_t newState );						// Initialize the new state.
	void			State_Leave();														// Cleanup the previous state.
	void			State_Think();														// Update the current state.
	static CGameRulesRoundStateInfo *State_LookupInfo( gamerules_roundstate_t state );	// Find the state info for the specified state.

	// State Functions
	void			State_Enter_INIT( void );
	void			State_Think_INIT( void );

	void			State_Enter_PREGAME( void );
	void			State_Think_PREGAME( void );

	void			State_Enter_STARTGAME( void );
	void			State_Think_STARTGAME( void );

	void			State_Enter_PREROUND( void );
	void			State_Leave_PREROUND( void );
	void			State_Think_PREROUND( void );

	void			State_Enter_RND_RUNNING( void );
	void			State_Think_RND_RUNNING( void );
	void			State_Leave_RND_RUNNING( void );

	void			State_Enter_TEAM_WIN( void );
	void			State_Think_TEAM_WIN( void );

	void			State_Enter_RESTART( void );
	void			State_Think_RESTART( void );

	void			State_Enter_STALEMATE( void );
	void			State_Think_STALEMATE( void );
	void			State_Leave_STALEMATE( void );

	void			State_Enter_BONUS( void );
	void			State_Think_BONUS( void );
	void			State_Leave_BONUS( void );

	void			State_Enter_BETWEEN_RNDS( void );
	void			State_Leave_BETWEEN_RNDS( void );
	void			State_Think_BETWEEN_RNDS( void );

	// mp_scrambleteams_auto
	void			ResetTeamsRoundWinTracking( void );

protected:
	void			InitTeams( void );

	int				CountActivePlayers( void );

	void			RoundRespawn( void );
	void			CleanUpMap( void );
	void			CheckRespawnWaves( void );
	void			BalanceTeams( bool bRequireSwitcheesToBeDead );
	void			ResetScores( void );
	void			ResetMapTime( void );

	void			PlayStartRoundVoice( void );
	void			PlayWinSong( int team );
	void			PlayStalemateSong( void );
	void			PlaySuddenDeathSong( void );

	void			RespawnTeam( int iTeam ) { RespawnPlayers( false, true, iTeam ); }

	void			InternalHandleTeamWin( int iWinningTeam );

	bool			MapHasActiveTimer( void );
	void			CreateTimeLimitTimer( void );

	float			GetLastMajorEventTime( void ) OVERRIDE { return m_flLastTeamWin; }

	static int		PlayerRoundScoreSortFunc( const PlayerRoundScore_t *pRoundScore1, const PlayerRoundScore_t *pRoundScore2 );
	static int		ArenaPlayerRoundScoreSortFunc( const ArenaPlayerRoundScore_t *pRoundScore1, const ArenaPlayerRoundScore_t *pRoundScore2 );
#endif // GAME_DLL

public:
	// Collision and Damage rules.
	bool			ShouldCollide( int collisionGroup0, int collisionGroup1 );

	int				GetTimeLeft( void );

	// Get the view vectors for this mod.
	const CViewVectors *GetViewVectors() const;

	void			FireGameEvent( IGameEvent *event );

	const char		*GetGameTypeName( void ) { return g_aGameTypeNames[m_nGameType]; }
	int				GetGameType( void ) { return m_nGameType; }

	bool			FlagsMayBeCapped( void );

	const char		*GetTeamGoalString( int iTeam );

	int				GetHudType( void ) { return m_nHudType; };

	bool			IsConnectedUserInfoChangeAllowed( CBasePlayer *pPlayer );
	bool			AllowThirdPersonCamera( void );
	CBaseCombatWeapon *GetNextBestWeapon( CBaseCombatCharacter *pPlayer, CBaseCombatWeapon *pCurrentWeapon );

	bool			IsFourTeamGame( void ) { return m_bFourTeamMode; }
	void			SetFourTeamGame( bool bEnabled ) { m_bFourTeamMode = bEnabled; }

	bool			IsMannVsMachineMode( void ) { return false; };
	bool			IsInArenaMode( void ) { return m_bArenaMode; }
	bool			IsInArenaQueueMode( void ) { return IsInArenaMode() && tf_arena_use_queue.GetBool() && !IsFourTeamGame(); }
	bool			IsInKothMode( void ) { return m_bPlayingKoth; }
	bool			IsHalloweenScenario( int iEventType ) { return false; }
	bool			IsPVEModeActive( void ) { return false; }
	bool			IsCompetitiveMode( void ) { return m_bCompetitiveMode; }
	bool			IsInHybridCTF_CPMode( void ) { return m_bPlayingHybrid_CTF_CP; }
	bool			IsInSpecialDeliveryMode( void ) { return m_bPlayingSpecialDeliveryMode; }
	bool			IsInMedievalMode( void ) { return m_bPlayingMedieval; }
	bool			IsVIPMode( void ) { return m_nGameType == TF_GAMETYPE_VIP; }
	bool			IsInDominationMode( void ) { return m_bPlayingDomination; }
	bool			IsInTerritorialDominationMode( void ) { return m_bPlayingTerritorialDomination; }
	// bool			IsInSiegeMode( void ) { return m_nGameType == TF_GAMETYPE_SIEGE; } // Possibly for a future update.
	bool			ShouldWaitForPlayersInPregame( void ) { return IsInArenaMode(); }

	int				GetPointLimit( void ){ return m_iPointLimit; };

	bool			HasMultipleTrains( void ){ return m_bMultipleTrains; }

	//Training Mode
	bool			IsInTraining( void ) { return false; }
	bool			IsInItemTestingMode( void ) { return false; }

	CTeamRoundTimer *GetKothTimer( int iTeam );
	CTeamRoundTimer *GetWaitingForPlayersTimer( void ) { return m_hWaitingForPlayersTimer; }
	CTeamRoundTimer *GetStalemateTimer( void ) { return m_hStalemateTimer; }
	CTeamRoundTimer *GetTimeLimitTimer( void ) { return m_hTimeLimitTimer; }
	CTeamRoundTimer *GetRoundTimer( void ) { return m_hRoundTimer; }

	int				GetRoundState() const { return m_iRoundState; }
#ifdef CLIENT_DLL
	void			SetRoundState( int iRoundState );
	void			OnPreDataChanged( DataUpdateType_t updateType );
	void			OnDataChanged( DataUpdateType_t updateType );
	void			HandleOvertimeBegin();
	void			GetTeamGlowColor( int nTeam, float &r, float &g, float &b );

	bool			ShouldShowTeamGoal( void );

	const char		*GetVideoFileForMap( bool bWithExtension = true );

	// TODO: Make a list of weather particles, this solution will simply disable all particles on a map when this command is active.
	virtual bool	AllowMapParticleEffect( const char *pszParticleEffect );
	bool			AllowWeatherParticles( void );

	virtual bool	AllowMapVisionFilterShaders( void ) { return true; }
	virtual const char *TranslateEffectForVisionFilter( const char *pchEffectType, const char *pchEffectName ) { return pchEffectName; }

	virtual void	ModifySentChat( char *pBuf, int iBufSize );
#else

	~CTFGameRules();

	void LevelShutdown();

	bool			ClientCommand( CBaseEntity *pEdict, const CCommand &args );
	void			Think();

	void			GoToIntermission( void );
	bool			CheckCapsPerRound();

	bool			FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker, const CTakeDamageInfo &info );

	virtual bool	FShouldSwitchWeapon( CBasePlayer *pPlayer, CBaseCombatWeapon *pWeapon ) { return false; }

	// Spawing rules.
	CBaseEntity		*GetPlayerSpawnSpot( CBasePlayer *pPlayer );
	bool			IsSpawnPointValid( CBaseEntity *pSpot, CBasePlayer *pPlayer, bool bIgnorePlayers );

	virtual int		ItemShouldRespawn( CItem *pItem );
	float			FlItemRespawnTime( CItem *pItem );
	Vector			VecItemRespawnSpot( CItem *pItem );
	QAngle			VecItemRespawnAngles( CItem *pItem );
	bool			CanHaveItem( CBasePlayer *pPlayer, CItem *pItem );
	void			PlayerGotItem( CBasePlayer *pPlayer, CItem *pItem );

	const char		*GetChatFormat( bool bTeamOnly, CBasePlayer *pPlayer );
	void			ClientSettingsChanged( CBasePlayer *pPlayer );
	void			GetTaggedConVarList( KeyValues *pCvarTagList );
	void			ClientCommandKeyValues( edict_t *pEntity, KeyValues *pKeyValues );
	void			ChangePlayerName( CTFPlayer *pPlayer, const char *pszNewName );

	void			OnNavMeshLoad();

	VoiceCommandMenuItem_t *VoiceCommand( CBaseMultiplayerPlayer *pPlayer, int iMenu, int iItem );

	int				GetAutoAimMode() { return AUTOAIM_NONE; }

	bool			CanHaveAmmo( CBaseCombatCharacter *pPlayer, int iAmmoIndex );

	const char		*GetGameDescription( void );

	// Line trace rules.
	float			WeaponTraceEntity( CBaseEntity *pEntity, const Vector &vecStart, const Vector &vecEnd, unsigned int mask, trace_t *ptr );

	// Sets up g_pPlayerResource.
	void			CreateStandardEntities();

	void			PlayerKilled( CBasePlayer *pVictim, const CTakeDamageInfo &info );
	void			DeathNotice( CBasePlayer *pVictim, const CTakeDamageInfo &info );
	CBasePlayer		*GetDeathScorer( CBaseEntity *pKiller, CBaseEntity *pInflictor, CBaseEntity *pVictim );
	void			LogLifeStats(CTFPlayer* pTFPlayer);

	void			CalcDominationAndRevenge( CTFPlayer *pAttacker, CTFPlayer *pVictim, bool bIsAssist, int *piDeathFlags );

	void			GetKillingWeaponName( const CTakeDamageInfo &info, CTFPlayer *pVictim, KillingWeaponData_t &weaponData );
	CTFPlayer		*GetAssister( CTFPlayer *pVictim, CTFPlayer *pScorer, const CTakeDamageInfo &info, CBaseEntity *&pAssisterWeapon );

	bool			ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen );
	void			ClientDisconnected( edict_t *pClient );

	void			RadiusDamage( CTFRadiusDamageInfo &radiusInfo );
	void			RadiusDamage( const CTakeDamageInfo &info, const Vector &vecSrc, float flRadius, int iClassIgnore, CBaseEntity *pEntityIgnore );
	void			RadiusHeal( CTFRadiusHealingInfo &radiusInfo, int nMaxPlayers = -1, bool bDivideHealing = false );
	
	bool			ApplyOnDamageModifyRules( CTakeDamageInfo &info, CBaseEntity *pVictimBaseEntity, bool bAllowDamage );

	struct DamageModifyExtras_t
	{
		bool bIgniting;
		bool bSelfBlastDmg;
		bool bPlayDamageReductionSound;
		CUtlVector<CTFPlayer*> vecPlayDamageReductionSoundFullPlayers; // Is used only within a single frame function so mere pointers
	};
	float			ApplyOnDamageAliveModifyRules( const CTakeDamageInfo &info, CBaseEntity *pVictimBaseEntity, DamageModifyExtras_t& outParams );

	float			FlPlayerFallDamage( CBasePlayer *pPlayer );

	bool			FlPlayerFallDeathDoesScreenFade( CBasePlayer *pl ) { return false; }

	bool			UseSuicidePenalty() { return false; }

	int				GetPreviousRoundWinners( void ) { return m_iPreviousRoundWinners; }

	void			SendHudNotification( IRecipientFilter &filter, HudNotification_t iType, int iTeam = TEAM_UNASSIGNED );
	void			SendHudNotification( IRecipientFilter &filter, const char *pszText, const char *pszIcon, int iTeam = TEAM_UNASSIGNED );

	void			ShowRoundInfoPanel( CTFPlayer *pPlayer = NULL ); // NULL pPlayer means show the panel to everyone

	bool			CanChangeClassInStalemate( void );

	void			SetHudType( int iHudType ) { m_nHudType = iHudType; }

	void			SetTeamGoalString( int iTeam, const char *pszGoal );

	void			HandleCTFCaptureBonus( int iTeam );

	void			Arena_RunTeamLogic();
	void			Arena_PrepareNewPlayerQueue( bool bStreakReached );
	void			Arena_CleanupPlayerQueue();
	void			Arena_ResetLosersScore( bool bStreakReached );
	void			Arena_SendPlayerNotifications();
	void			Arena_ClientDisconnect( const char *pszPlayerName );
	void			Arena_NotifyTeamSizeChange();
	int				Arena_PlayersNeededForMatch();

	bool			IsPlayerInQueue( CTFPlayer *pPlayer ) const;
	void			AddPlayerToQueue( CTFPlayer *pPlayer );
	void			AddPlayerToQueueHead( CTFPlayer *pPlayer );
	void			RemovePlayerFromQueue( CTFPlayer *pPlayer );

	int				GetClassLimit( int iDesiredClassIndex, int iTeam );
	bool			CanPlayerChooseClass( CBasePlayer *pPlayer, int iDesiredClassIndex );

	// Speaking, vcds, voice commands.
	void			InitCustomResponseRulesDicts();
	void			ShutdownCustomResponseRulesDicts();

	int				PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget );

	void			SetKothTimer( CTeamRoundTimer *pTimer, int iTeam );

	void			ManageServerSideVoteCreation( void );
	void			StartNextMapVote( void );
	void			ScheduleNextMapVote( void );
	void			ExtendCurrentMap( void );

	CBaseEntity		*GetRequiredObserverTarget( void ) { return m_hRequiredObserverTarget.Get(); }
	void			SetRequiredObserverTarget( CBaseEntity *pEntity ) { m_hRequiredObserverTarget = pEntity; }

	void			SetPreRoundFreezeTimeEnabled( bool bEnabled ) { m_bPreRoundFreezeTime = bEnabled; }

	bool			IsSentryDamager( const CTakeDamageInfo &info );

	void			SendLoadingScreenInfo( void );

	void			AssignTeamVIPs( void );
	CTFPlayer		*AssignVIP( int iTeam, CTFPlayer *pSkipPlayer = NULL );
	void			ResetVIPHistoryOnTeam( int iTeam );
	void			OnVIPLeft( CTFPlayer *pPlayer );
	void			OnVIPKilled( CTFPlayer *pPlayer, CTFPlayer *pKiller = NULL, CTFPlayer *pAssister = NULL, const CTakeDamageInfo &info = CTakeDamageInfo() );
	void			OnVIPEscaped( CTFPlayer *pPlayer, bool bWin );

	bool			Domination_RunLogic( void );

	int				GetAssignedHumanTeam();

private:
	int				DefaultFOV( void ) { return 75; }

#endif

private:
	// Server specific
#ifdef GAME_DLL
	CGameRulesRoundStateInfo	*m_pCurStateInfo;			// Per-state data 
	float						m_flStateTransitionTime;	// Timer for round states

	float						m_flWaitingForPlayersTimeEnds;
	

	float						m_flNextPeriodicThink;
	bool						m_bChangeLevelOnRoundEnd;

	bool						m_bResetTeamScores;
	bool						m_bResetPlayerScores;
	bool						m_bResetRoundsPlayed;

	// Stalemate
	float						m_flStalemateStartTime;

	bool						m_bForceMapReset; // should the map be reset when a team wins and the round is restarted?
	bool						m_bPrevRoundWasWaitingForPlayers;	// was the previous map reset after a waiting for players period

	string_t					m_iszRoundToPlayNext;
	CUtlVector<string_t>		m_iszPreviousRounds; // we'll store the two previous rounds so we won't play them again right away if there are other rounds that can be played first
	string_t					m_iszFirstRoundPlayed; // store the first round played after a full restart so we can pick a different one next time if we have other options

	float						m_flOriginalTeamRespawnWaveTime[MAX_TEAMS];

	bool						m_bAllowStalemateAtTimelimit;
	bool						m_bChangelevelAfterStalemate;

	float						m_flRoundStartTime;		// time the current round started
	float						m_flNewThrottledAlertTime;		// time that we can play another throttled alert

	bool						m_bCTF_Overtime;

	int							m_nRoundsPlayed;
	bool						m_bUseAddScoreAnim;

	gamerules_roundstate_t		m_prevState;

	bool						m_bPlayerReadyBefore[MAX_PLAYERS + 1];	// Test to see if a player has hit ready before

	float						m_flLastTeamWin;

	float						m_flStartBalancingTeamsAt;
	float						m_flNextBalanceTeamsTime;
	bool						m_bPrintedUnbalanceWarning;
	float						m_flFoundUnbalancedTeamsTime;

	float						m_flAutoBalanceQueueTimeEnd;
	int							m_nAutoBalanceQueuePlayerIndex;
	int							m_nAutoBalanceQueuePlayerScore;

	//----------------------------------------------------------------------------------

	CUtlVector<CHandle<CHealthKit>> m_hDisabledHealthKits;

	char						m_szMostRecentCappers[MAX_PLAYERS + 1];	// list of players who made most recent capture.  Stored as string so it can be passed in events.
	int							m_iNumCaps[TF_TEAM_COUNT];				// # of captures ever by each team during a round

	int							SetCurrentRoundStateBitString();
	void						SetMiniRoundBitMask( int iMask );
	int							m_iPrevRoundState;	// bit string representing the state of the points at the start of the previous miniround
	int							m_iCurrentRoundState;
	int							m_iCurrentMiniRoundMask;

	bool						m_bFirstBlood;
	int							m_iArenaTeamCount;
	float						m_flNextArenaNotify;
	CUtlVector<CHandle<CTFPlayer>> m_ArenaPlayerQueue;

	bool						m_bVoteMapOnNextRound;
	bool						m_bVotedForNextMap;
	int							m_iMaxRoundsBonus;
	int							m_iWinLimitBonus;

	float						m_flNextDominationThink;
	float						m_flNextVoteThink;

	bool						m_bNeedSteamServerCheck;

	EHANDLE						m_hRequiredObserverTarget;
	bool						m_bPreRoundFreezeTime;

	KeyValues					*m_pAuthData = NULL;
#endif
	// End server specific

	// End server specific
	//----------------------------------------------------------------------------------

	//----------------------------------------------------------------------------------
	// Client specific
#ifdef CLIENT_DLL
	bool						m_bOldInWaitingForPlayers;
	bool						m_bOldInOvertime;
	bool						m_bOldInSetup;
#endif // CLIENT_DLL

public:
	float						m_flStopWatchTotalTime;
	int							m_iLastCapPointChanged;

	//----------------------------------------------------------------------------------

	bool						m_bControlSpawnsPerTeam[MAX_TEAMS][MAX_CONTROL_POINTS];
	int							m_iPreviousRoundWinners;

	int							m_iBirthdayMode;
	int							m_iHolidayMode;

#ifdef GAME_DLL
	float						m_flCTFBonusTime;
	float						m_flTimerMayExpireAt;
#endif

private:
	bool						m_bAllowBetweenRounds;

private:
	CNetworkVar( gamerules_roundstate_t, m_iRoundState );
	CNetworkVar( bool, m_bInOvertime ); // Are we currently in overtime?
	CNetworkVar( bool, m_bInSetup ); // Are we currently in setup?
	CNetworkVar( bool, m_bSwitchedTeamsThisRound );
	CNetworkVar( int, m_iWinningTeam );				// Set before entering GR_STATE_TEAM_WIN
	CNetworkVar( int, m_iWinReason );
	CNetworkVar( bool, m_bInWaitingForPlayers );
	CNetworkVar( bool, m_bAwaitingReadyRestart );
	CNetworkVar( float, m_flRestartRoundTime );
	CNetworkVar( float, m_flMapResetTime );						// Time that the map was reset
	CNetworkArray( float, m_flNextRespawnWave, MAX_TEAMS );		// Minor waste, but cleaner code
	CNetworkArray( bool, m_bTeamReady, MAX_TEAMS );
	CNetworkVar( bool, m_bStopWatch );
	CNetworkVar( bool, m_bMultipleTrains ); // two trains in this map?
	CNetworkArray( bool, m_bPlayerReady, MAX_PLAYERS );
	CNetworkVar( bool, m_bCheatsEnabledDuringLevel );

	//----------------------------------------------------------------------------------

public:
	CNetworkArray( float, m_TeamRespawnWaveTimes, MAX_TEAMS );	// Time between each team's respawn wave

private:
	CNetworkVar( ETFGameType, m_nGameType ); // Type of game this map is (CTF, CP)
	CNetworkString( m_pszTeamGoalStringRed, MAX_TEAMGOAL_STRING );
	CNetworkString( m_pszTeamGoalStringBlue, MAX_TEAMGOAL_STRING );
	CNetworkString( m_pszTeamGoalStringGreen, MAX_TEAMGOAL_STRING );
	CNetworkString( m_pszTeamGoalStringYellow, MAX_TEAMGOAL_STRING );
	CNetworkVar( float, m_flCapturePointEnableTime );
	CNetworkVar( int, m_nHudType );
	CNetworkVar( bool, m_bPlayingKoth );
	CNetworkVar( bool, m_bPlayingMedieval );
	CNetworkVar( bool, m_bPlayingSpecialDeliveryMode );
	CNetworkVar( bool, m_bPlayingRobotDestructionMode );
	CNetworkVar( bool, m_bPlayingMannVsMachine );
	CNetworkVar( bool, m_bPlayingHybrid_CTF_CP );
	CNetworkVar( bool, m_bCompetitiveMode );
	CNetworkVar( bool, m_bPowerupMode );
	CNetworkVar( bool, m_bPlayingDomination );
	CNetworkVar( bool, m_bPlayingTerritorialDomination );
	CNetworkVar( bool, m_bFourTeamMode );
	CNetworkVar( bool, m_bArenaMode );
	CNetworkHandle( CTeamRoundTimer, m_hBlueKothTimer );
	CNetworkHandle( CTeamRoundTimer, m_hRedKothTimer );
	CNetworkHandle( CTeamRoundTimer, m_hGreenKothTimer );
	CNetworkHandle( CTeamRoundTimer, m_hYellowKothTimer );
	CNetworkHandle( CTeamRoundTimer, m_hWaitingForPlayersTimer );
	CNetworkHandle( CTeamRoundTimer, m_hStalemateTimer );
	CNetworkHandle( CTeamRoundTimer, m_hTimeLimitTimer );
	CNetworkHandle( CTeamRoundTimer, m_hRoundTimer );
	CNetworkVar( int, m_iMapTimeBonus );
	CNetworkVar( int, m_iPointLimit );

#ifdef GAME_DLL // TFBot stuff
public:
	bool CanBotChangeClass( CBasePlayer *pPlayer );
	bool CanBotChooseClass( CBasePlayer *pPlayer, int iDesiredClassIndex );

	void CollectCapturePoints( const CTFPlayer *pPlayer, CUtlVector<CTeamControlPoint *> *pPoints );
	void CollectDefendPoints( const CTFPlayer *pPlayer, CUtlVector<CTeamControlPoint *> *pPoints );

	CTeamTrainWatcher *GetPayloadToPush( int iTeam );
	CTeamTrainWatcher *GetPayloadToBlock( int iTeam );

private:
	void ResetBotRosters();

	void ResetBotPayloadInfo();

	CHandle<CTFBotRoster> m_hBotRosterRed;
	CHandle<CTFBotRoster> m_hBotRosterBlue;
	CHandle<CTFBotRoster> m_hBotRosterGreen;
	CHandle<CTFBotRoster> m_hBotRosterYellow;

	//bool m_bBotPayloadToPushInit = false;
	CHandle<CTeamTrainWatcher> m_hBotPayloadToPushRed;
	CHandle<CTeamTrainWatcher> m_hBotPayloadToPushBlue;
	CHandle<CTeamTrainWatcher> m_hBotPayloadToPushGreen;
	CHandle<CTeamTrainWatcher> m_hBotPayloadToPushYellow;

	//bool m_bBotPayloadToBlockInit = false;
	CHandle<CTeamTrainWatcher> m_hBotPayloadToBlockRed;
	CHandle<CTeamTrainWatcher> m_hBotPayloadToBlockBlue;
	CHandle<CTeamTrainWatcher> m_hBotPayloadToBlockGreen;
	CHandle<CTeamTrainWatcher> m_hBotPayloadToBlockYellow;

	CountdownTimer m_ctBotCountUpdate;
	CountdownTimer m_ctBotPayloadBlockUpdate;
public:
	CUtlVector<CHandle<CHealthKit>> m_hArenaDroppedTeamHealthKits;
	bool m_bArenaTeamHealthKitsTeamDied[TF_TEAM_COUNT];
#endif
};

//-----------------------------------------------------------------------------
// Gets us at the team fortress game rules
//-----------------------------------------------------------------------------
inline CTFGameRules *TFGameRules()
{
	return static_cast<CTFGameRules*>( g_pGameRules );
}

#ifdef GAME_DLL
bool EntityPlacementTest( CBaseEntity *pMainEnt, const Vector &vOrigin, Vector &outPos, bool bDropToGround );
bool PropDynamic_CollidesWithGrenades( CBaseEntity *pBaseEntity );
#endif

#ifdef CLIENT_DLL
void AddSubKeyNamed( KeyValues *pKeys, const char *pszName );
#endif

#endif // TF_GAMERULES_H
