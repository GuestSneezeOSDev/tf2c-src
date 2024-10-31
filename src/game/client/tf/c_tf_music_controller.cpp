//=============================================================================//
//
// Purpose:
//
//=============================================================================//
#include "cbase.h"
#include "c_tf_music_controller.h"
#include "tf_gamerules.h"
#include <vgui_controls/EditablePanel.h>
#include "hudelement.h"
#include "iclientmode.h"
#include <vgui/IVGui.h>
#include <vgui_controls/AnimationController.h>
#include <vgui_controls/TextImage.h>
#include "tf_controls.h"
#include "tf_hud_freezepanel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

C_TFMusicController *g_pMusicController = NULL;

static void MusicToggle( IConVar *var, const char *pOldValue, float flOldValue )
{
	if ( !g_pMusicController )
		return;

	ConVar *pCvar = (ConVar *)var;
	g_pMusicController->ToggleMusicEnabled( pCvar->GetBool() );
}

ConVar tf2c_music( "tf2c_music", "0", FCVAR_ARCHIVE, "Enable music in Deathmatch.", MusicToggle );
ConVar tf2c_music_volume( "tf2c_music_volume", "1.0", FCVAR_ARCHIVE, NULL, true, 0, true, 1 );


IMPLEMENT_CLIENTCLASS_DT_NOBASE( C_TFMusicController, DT_TFMusicController, CTFMusicController )
	RecvPropInt( RECVINFO( m_iTrack ) ),
	RecvPropBool( RECVINFO( m_bShouldPlay ) ),
END_RECV_TABLE()


C_TFMusicController::C_TFMusicController()
{
	SetDefLessFunc( m_Tracks );

	m_iTrack = 1;
	m_bShouldPlay = false;

	m_iPlayingTrack = 0;
	m_bPlaying = false;
	m_bPlayingEnding = false;
	m_bInWaiting = false;
}

C_TFMusicController::~C_TFMusicController()
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TFMusicController::Spawn( void )
{
	Precache();
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TFMusicController::Precache( void )
{
	ParseMusicData( TF_MUSIC_DATA_FILE );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TFMusicController::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );

	m_bOldShouldPlay = m_bShouldPlay;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TFMusicController::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	// Don't allow multiple music controllers per map.
	if ( updateType == DATA_UPDATE_CREATED )
	{
		if ( !g_pMusicController )
		{
			g_pMusicController = this;

			ListenForGameEvent( "localplayer_changeteam" );
			ListenForGameEvent( "teamplay_update_timer" );
			ListenForGameEvent( "deathmatch_results" );
		}
	}
	else
	{
		if ( m_iTrack != m_iPlayingTrack )
		{
			// Stop the current track and start the new one.
			RestartMusic();
		}

		if ( m_bShouldPlay != m_bOldShouldPlay )
		{
			ToggleMusicEnabled( m_bShouldPlay );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TFMusicController::UpdateOnRemove( void )
{
	StopMusic( false );
	g_pMusicController = NULL;

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TFMusicController::ClientThink( void )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	Assert( m_pSound != NULL );

	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

	float flOldVolume = controller.SoundGetVolume( m_pSound );
	float flVolume = tf2c_music_volume.GetFloat();
	const float flDeltaTime = 0.5f;

	// Tone it down when player is dead.
	if ( !pPlayer->IsAlive() )
		flVolume *= 0.25f;

	if ( flVolume != flOldVolume )
	{
		controller.SoundChangeVolume( m_pSound, flVolume, flDeltaTime );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TFMusicController::FireGameEvent( IGameEvent *event )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	if ( V_strcmp( event->GetName(), "localplayer_changeteam" ) == 0 )
	{
		if ( pPlayer->GetTeamNumber() < FIRST_GAME_TEAM )
		{
			// Play ending cue if player switched to spec.
			StopMusic( true );
		}
		else
		{
			// Start music if player entered the battle.
			StartMusic();
		}
	}
	else if ( V_strcmp( event->GetName(), "teamplay_update_timer" ) == 0 )
	{
		// if "waiting for players" status changed, restart music.
		if ( m_bInWaiting != ShouldPlayWaitingMusic() )
		{
			RestartMusic();
		}
	}
	else if ( V_strcmp( event->GetName(), "deathmatch_results" ) == 0 )
	{
		StopMusic( true );
		m_bPlayingEnding = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TFMusicController::StartMusic( void )
{
	if ( m_bPlaying )
		return;

	m_bInWaiting = ShouldPlayWaitingMusic();

	if ( !CanPlayMusic() )
		return;

	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	m_iPlayingTrack = m_iTrack;

	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

	int index = m_Tracks.Find( m_iPlayingTrack );
	if ( index != m_Tracks.InvalidIndex() )
	{
		MusicData_t *pData = &m_Tracks[index];
		int iSegment = m_bInWaiting ? TF_MUSIC_WAITING : TF_MUSIC_LOOP;
		const char *pszSound = pData->aSegments[iSegment];

		if ( pszSound[0] != '\0' )
		{
			CLocalPlayerFilter filter;
			m_pSound = controller.SoundCreate( filter, entindex(), CHAN_STATIC, pszSound, SNDLVL_NONE );
			controller.Play( m_pSound, tf2c_music_volume.GetFloat(), 100 );

			if ( !m_bInWaiting )
			{
				//Msg( "Now playing: %s - %s\n", pData->szComposer, pData->szName );

				IGameEvent *event = gameeventmanager->CreateEvent( "song_started" );

				if ( event )
				{
					event->SetString( "name", pData->szName );
					event->SetString( "composer", pData->szComposer );

					gameeventmanager->FireEventClientSide( event );
				}
			}

			m_bPlaying = true;
			SetNextClientThink( CLIENT_THINK_ALWAYS );
		}
	}
	else
	{
		Warning( "C_TFMusicController: Attempted to play unknown track %d!\n", m_iPlayingTrack );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TFMusicController::StopMusic( bool bPlayEnding /*= false*/ )
{
	if ( !m_bPlaying )
		return;

	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

	if ( m_pSound )
	{
		controller.SoundDestroy( m_pSound );
		m_pSound = NULL;
	}

	if ( bPlayEnding && !m_bInWaiting )
	{
		// Play ending segment that corrensonds to the current track.
		int index = m_Tracks.Find( m_iPlayingTrack );
		if ( index != m_Tracks.InvalidIndex() )
		{
			const char *pszSound = m_Tracks[index].aSegments[TF_MUSIC_ENDING];

			if ( pszSound[0] != '\0' )
			{
				EmitSound_t params;
				params.m_pSoundName = pszSound;

				CSoundParameters scriptParams;
				if ( !Q_stristr( pszSound, ".wav" ) && !Q_stristr( pszSound, ".mp3" ) &&
					CBaseEntity::GetParametersForSound( pszSound, scriptParams, NULL ) )
				{
					// Scale volume from the script.
					params.m_flVolume = scriptParams.volume * tf2c_music_volume.GetFloat();
					params.m_nFlags |= SND_CHANGE_VOL;
				}
				else
				{
					params.m_nChannel = CHAN_STATIC;
					params.m_SoundLevel = SNDLVL_NONE;
					params.m_flVolume = tf2c_music_volume.GetFloat();
				}

				CLocalPlayerFilter filter;
				C_BaseEntity::EmitSound( filter, entindex(), params );
			}
		}
	}

	if ( !m_bInWaiting )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "song_stopped" );

		if ( event )
		{
			gameeventmanager->FireEventClientSide( event );
		}
	}

	m_bPlaying = false;
	SetNextClientThink( CLIENT_THINK_NEVER );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TFMusicController::RestartMusic( void )
{
	StopMusic( false );
	StartMusic();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TFMusicController::ToggleMusicEnabled( bool bEnable )
{
	if ( bEnable )
	{
		StartMusic();
	}
	else
	{
		StopMusic( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool C_TFMusicController::CanPlayMusic( void )
{
	// This must be the active controller.
	if ( g_pMusicController != this )
		return false;

	if ( !tf2c_music.GetBool() )
		return false;

	if ( !m_bShouldPlay )
		return false;

	// Don't play music for spectators.
	if ( GetLocalPlayerTeam() < FIRST_GAME_TEAM )
		return false;

	if ( TFGameRules()->State_Get() == GR_STATE_GAME_OVER )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool C_TFMusicController::ShouldPlayWaitingMusic( void )
{
	return ( TFGameRules()->IsInWaitingForPlayers() || TFGameRules()->InSetup() );
}


using namespace vgui;

class CTFHudMusicInfo : public CHudElement, public EditablePanel
{
public:
	DECLARE_CLASS_SIMPLE( CTFHudMusicInfo, EditablePanel );

	CTFHudMusicInfo( const char *pElementName );

	virtual void ApplySchemeSettings( IScheme *pScheme );
	virtual void FireGameEvent( IGameEvent *event );
	virtual bool ShouldDraw( void );
	virtual void LevelInit( void );
	virtual void OnTick( void );

	void ResetAnimation();

private:
	EditablePanel *m_pInfoPanel;
	CExLabel *m_pNameLabel;
	CExLabel *m_pComposerLabel;

	float m_flShowAt;
	float m_flHideAt;
};

DECLARE_HUDELEMENT( CTFHudMusicInfo );

CTFHudMusicInfo::CTFHudMusicInfo( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudMusicInfo" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pInfoPanel = new EditablePanel( this, "InfoPanel" );
	m_pNameLabel = new CExLabel( m_pInfoPanel, "NameLabel", "" );
	m_pComposerLabel = new CExLabel( m_pInfoPanel, "ComposerLabel", "" );

	m_flShowAt = 0.0f;
	m_flHideAt = 0.0f;

	ivgui()->AddTickSignal( GetVPanel() );

	ListenForGameEvent( "song_started" );
	ListenForGameEvent( "song_stopped" );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFHudMusicInfo::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/ui/HudMusicInfo.res" );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFHudMusicInfo::FireGameEvent( IGameEvent *event )
{
	if ( V_strcmp( event->GetName(), "song_started" ) == 0 )
	{
		m_pInfoPanel->SetDialogVariable( "songname", event->GetString( "name" ) );
		m_pInfoPanel->SetDialogVariable( "composername", event->GetString( "composer" ) );

		ResetAnimation();

		m_flShowAt = gpGlobals->curtime + 1.0f;
	}
	else if ( V_strcmp( event->GetName(), "song_stopped" ) == 0 )
	{
		ResetAnimation();
	}
	else
	{
		CHudElement::FireGameEvent( event );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFHudMusicInfo::ShouldDraw( void )
{
	if ( !g_pMusicController )
		return false;

	if ( !g_pMusicController->IsPlayingMusic() || g_pMusicController->IsInWaiting() )
		return false;

	if ( IsInFreezeCam() )
		return false;

	return CHudElement::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFHudMusicInfo::LevelInit( void )
{
	ResetAnimation();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFHudMusicInfo::OnTick( void )
{
	// Can't put this into HUD animations script file since we have to set the width through the code.
	if ( m_flShowAt != 0.0f && gpGlobals->curtime >= m_flShowAt )
	{
		// Calculate width.
		int nameWide, nameTall, composerWide, composerTall;
		m_pNameLabel->GetTextImage()->GetContentSize( nameWide, nameTall );
		m_pComposerLabel->GetTextImage()->GetContentSize( composerWide, composerTall );

		int iPanelWidth = Max( nameWide, composerWide ) + YRES( 10 );

		GetAnimationController()->RunAnimationCommand( m_pInfoPanel, "alpha", 255.0f, 0.0f, 0.5f, AnimationController::INTERPOLATOR_LINEAR );
		GetAnimationController()->RunAnimationCommand( m_pInfoPanel, "wide", iPanelWidth, 0.5f, 0.25f, AnimationController::INTERPOLATOR_LINEAR );

		GetAnimationController()->RunAnimationCommand( m_pNameLabel, "alpha", 255.0f, 0.65f, 0.1f, AnimationController::INTERPOLATOR_LINEAR );
		GetAnimationController()->RunAnimationCommand( m_pComposerLabel, "alpha", 255.0f, 0.65f, 0.1f, AnimationController::INTERPOLATOR_LINEAR );

		// Show it for 3 seconds.
		m_flHideAt = gpGlobals->curtime + 4.0f;
		m_flShowAt = 0.0f;
	}
	else if ( m_flHideAt != 0.0f && gpGlobals->curtime >= m_flHideAt )
	{
		GetAnimationController()->RunAnimationCommand( m_pNameLabel, "alpha", 0.0f, 0.0f, 0.25f, AnimationController::INTERPOLATOR_LINEAR );
		GetAnimationController()->RunAnimationCommand( m_pComposerLabel, "alpha", 0.0f, 0.0f, 0.25f, AnimationController::INTERPOLATOR_LINEAR );

		GetAnimationController()->RunAnimationCommand( m_pInfoPanel, "alpha", 0.0f, 0.5f, 0.25f, AnimationController::INTERPOLATOR_LINEAR );
		GetAnimationController()->RunAnimationCommand( m_pInfoPanel, "wide", GetTall(), 0.25f, 0.25f, AnimationController::INTERPOLATOR_LINEAR );

		m_flHideAt = 0.0f;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFHudMusicInfo::ResetAnimation( void )
{
	m_flShowAt = m_flHideAt = 0.0f;

	GetAnimationController()->CancelAnimationsForPanel( this );

	m_pInfoPanel->SetAlpha( 0 );
	m_pInfoPanel->SetWide( GetTall() );
	m_pNameLabel->SetAlpha( 0 );
	m_pComposerLabel->SetAlpha( 0 );
}
