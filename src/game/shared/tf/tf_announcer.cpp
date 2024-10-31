//=============================================================================//
//
// Purpose: 
//
//=============================================================================//
#include "cbase.h"
#include "tf_announcer.h"
#include "tf_gamerules.h"

#ifdef CLIENT_DLL
#include <engine/IEngineSound.h>
#include "hud_macros.h"
#endif

const char *CTFAnnouncer::m_aAnnouncerSounds[TF_ANNOUNCERTYPE_COUNT][TF_ANNOUNCER_MESSAGE_COUNT] =
{
	// Helen
	{
		"Hud.PointCaptured",
		"Game.Stalemate",
		"Game.SuddenDeath",
		"Game.YourTeamWon",
		"Game.YourTeamLost",
		"Game.Overtime",
		"Announcer.RoundBegins60Seconds",
		"Announcer.RoundBegins30Seconds",
		"Announcer.RoundBegins10Seconds",
		"Announcer.RoundBegins5Seconds",
		"Announcer.RoundBegins4Seconds",
		"Announcer.RoundBegins3Seconds",
		"Announcer.RoundBegins2Seconds",
		"Announcer.RoundBegins1Seconds",
		"Announcer.RoundEnds5minutes",
		"Announcer.RoundEnds60seconds",
		"Announcer.RoundEnds30seconds",
		"Announcer.RoundEnds10seconds",
		"Announcer.RoundEnds5seconds",
		"Announcer.RoundEnds4seconds",
		"Announcer.RoundEnds3seconds",
		"Announcer.RoundEnds2seconds",
		"Announcer.RoundEnds1seconds",
		"Announcer.TimeAdded",
		"Announcer.TimeAwardedForTeam",
		"Announcer.TimeAddedForEnemy",
		"CaptureFlag.EnemyStolen",
		"CaptureFlag.EnemyDropped",
		"CaptureFlag.EnemyCaptured",
		"CaptureFlag.EnemyReturned",
		"CaptureFlag.TeamStolen",
		"CaptureFlag.TeamDropped",
		"CaptureFlag.TeamCaptured",
		"CaptureFlag.TeamReturned",
		"AttackDefend.EnemyStolen",
		"AttackDefend.EnemyDropped",
		"AttackDefend.EnemyCaptured",
		"AttackDefend.EnemyReturned",
		"AttackDefend.TeamStolen",
		"AttackDefend.TeamDropped",
		"AttackDefend.TeamCaptured",
		"AttackDefend.TeamReturned",
		"Invade.EnemyStolen",
		"Invade.EnemyDropped",
		"Invade.EnemyCaptured",
		"Invade.TeamStolen",
		"Invade.TeamDropped",
		"Invade.TeamCaptured",
		"Invade.FlagReturned",
		"Announcer.SD_TheirTeamHasFlag",
		"Announcer.SD_TheirTeamDroppedFlag",
		"Announcer.SD_TheirTeamCapped",
		"Announcer.SD_OurTeamHasFlag",
		"Announcer.SD_OurTeamDroppedFlag",
		"Announcer.SD_OurTeamCapped",
		"Announcer.SD_FlagReturned",
		"Announcer.SD_RoundStart",
		"Announcer.ControlPointContested",
		"Announcer.ControlPointContested_Neutral",
		"Announcer.Cart.WarningAttacker",
		"Announcer.Cart.WarningDefender",
		"Announcer.Cart.FinalWarningAttacker",
		"Announcer.Cart.FinalWarningDefender",
		"Announcer.AM_RoundStartRandom",
		"Announcer.AM_FirstBloodRandom",
		"Announcer.AM_FirstBloodFast",
		"Announcer.AM_FirstBloodFinally",
		"Announcer.AM_CapEnabledRandom",
		"Announcer.AM_FlawlessVictory01",
		"Announcer.AM_FlawlessVictoryRandom",
		"Announcer.AM_FlawlessDefeatRandom",
		"Announcer.AM_TeamScrambleRandom",
		"Announcer.AM_LastManAlive",
		"Announcer.Success",
		"Announcer.Failure",
		"Announcer.SecurityAlert",
		"Announcer.Dom_LeadGained",
		"Announcer.Dom_LeadLost",
		"Announcer.Dom_TeamGettingClose",
		"Announcer.Dom_EnemyGettingClose",
		"Announcer.VIP_TheirTeamHasFlag",
		"Announcer.VIP_TheirTeamDroppedFlag",
		"Announcer.VIP_TheirTeamCapped",
		"Announcer.VIP_OurTeamHasFlag",
		"Announcer.VIP_OurTeamDroppedFlag",
		"Announcer.VIP_OurTeamCapped",
		"Announcer.VIP_FlagReturned",
		"Announcer.VIP_VIPDeath",
		"Announcer.VIP_OurVIPDeath"
	},
};

CTFAnnouncer g_TFAnnouncer( "Announcer" );

#ifdef CLIENT_DLL
static void __MsgFunc_AnnouncerSpeak( bf_read &msg )
{
	int iMessage = msg.ReadShort();
	g_TFAnnouncer.Speak( iMessage );
}
#endif

CTFAnnouncer::CTFAnnouncer( const char *pszName ) : CAutoGameSystem( pszName )
{
}


bool CTFAnnouncer::Init( void )
{
#ifdef CLIENT_DLL
	HOOK_MESSAGE( AnnouncerSpeak );
#endif

	return true;
}


void CTFAnnouncer::LevelInitPreEntity( void )
{
#ifdef GAME_DLL
	for ( int i = 0; i < TF_ANNOUNCERTYPE_COUNT; i++ )
	{
		for ( int j = 0; j < TF_ANNOUNCER_MESSAGE_COUNT; j++ )
		{
			const char *pszSound = m_aAnnouncerSounds[i][j];
			if ( !pszSound || !pszSound[0] )
				continue;

			CBaseEntity::PrecacheScriptSound( pszSound );
		}
	}
#endif
}


const char *CTFAnnouncer::GetSoundForMessage( int iMessage )
{
	int iType = TF_ANNOUNCERTYPE_HELEN;

	return m_aAnnouncerSounds[iType][iMessage];
}

#ifdef GAME_DLL


void CTFAnnouncer::Speak( IRecipientFilter &filter, int iMessage )
{
	UserMessageBegin( filter, "AnnouncerSpeak" );
	WRITE_SHORT( iMessage );
	MessageEnd();
}


void CTFAnnouncer::Speak( int iMessage )
{
	CReliableBroadcastRecipientFilter filter;
	Speak( filter, iMessage );
}


void CTFAnnouncer::Speak( CBasePlayer *pPlayer, int iMessage )
{
	CSingleUserRecipientFilter filter( pPlayer );
	filter.MakeReliable();
	Speak( filter, iMessage );
}


void CTFAnnouncer::Speak( int iTeam, int iMessage )
{
	CTeamRecipientFilter filter( iTeam, true );
	Speak( filter, iMessage );
}

#else


void CTFAnnouncer::Speak( int iMessage )
{
	const char *pszSound = GetSoundForMessage( iMessage );
	if ( !pszSound || !pszSound[0] )
		return;

	CLocalPlayerFilter filter;
	C_BaseEntity::EmitSound( filter, SOUND_FROM_LOCAL_PLAYER, pszSound );
}

#endif
