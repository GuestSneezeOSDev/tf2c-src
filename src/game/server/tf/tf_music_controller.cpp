//=============================================================================//
//
// Purpose:
//
//=============================================================================//
#include "cbase.h"
#include "tf_music_controller.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_SERVERCLASS_ST_NOBASE( CTFMusicController, DT_TFMusicController )
	SendPropInt( SENDINFO( m_iTrack ) ),
	SendPropBool( SENDINFO( m_bShouldPlay ) ),
END_SEND_TABLE()

BEGIN_DATADESC( CTFMusicController )
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetTrack", InputSetTrack ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),

	DEFINE_KEYFIELD( m_iTrack, FIELD_INTEGER, "tracknum" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( tf_music_controller, CTFMusicController );


CTFMusicController::CTFMusicController()
{
	m_iTrack = -1;
	m_bShouldPlay = false;
	SetDefLessFunc( m_Tracks );
}

CTFMusicController::~CTFMusicController()
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFMusicController::Spawn( void )
{
	Precache();

	m_bShouldPlay = !HasSpawnFlags( SF_MUSIC_STARTSILENT );

	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFMusicController::Precache( void )
{
	BaseClass::Precache();

	ParseMusicData( TF_MUSIC_DATA_FILE );

	// Precache the current track.
	PrecacheTrack( m_iTrack );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CTFMusicController::UpdateTransmitState( void )
{
	// ALWAYS transmit to all clients.
	return SetTransmitState( FL_EDICT_ALWAYS );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFMusicController::InputSetTrack( inputdata_t &inputdata )
{
	int iNewTrack = inputdata.value.Int();

	if ( iNewTrack != m_iTrack )
	{
		m_iTrack = iNewTrack;

		// Cache the new track.
		PrecacheTrack( m_iTrack );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFMusicController::InputEnable( inputdata_t &inputdata )
{
	m_bShouldPlay = true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFMusicController::InputDisable( inputdata_t &inputdata )
{
	m_bShouldPlay = false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFMusicController::PrecacheTrack( int iTrack )
{
	int index = m_Tracks.Find( iTrack );

	if ( index != m_Tracks.InvalidIndex() )
	{
		MusicData_t *pData = &m_Tracks[index];

		for ( int i = 0; i < TF_MUSIC_NUM_SEGMENTS; i++ )
		{
			if ( pData->aSegments[i][0] != '\0' )
				PrecacheScriptSound( pData->aSegments[i] );
		}
	}
}
