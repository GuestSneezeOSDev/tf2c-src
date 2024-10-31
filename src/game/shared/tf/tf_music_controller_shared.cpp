//=============================================================================//
//
// Purpose:
//
//=============================================================================//
#include "cbase.h"
#include "tf_music_controller_shared.h"
#include <filesystem.h>

#ifdef CLIENT_DLL
#include "c_tf_music_controller.h"
#else
#include "tf_music_controller.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
#define CTFMusicController C_TFMusicController
#endif

const char *g_aMusicSegmentNames[TF_MUSIC_NUM_SEGMENTS] =
{
	"loop",
	"waiting",
	"ending",
};

void CTFMusicController::ParseMusicData( const char *pszFile )
{
	KeyValues *pTrackKeys = new KeyValues( "Music" );
	if ( !pTrackKeys->LoadFromFile( filesystem, pszFile, "MOD" ) )
	{
		Warning( "C_TFMusicController: Failed to load music data from %s!\n", pszFile );
		pTrackKeys->deleteThis();
		return;
	}

	for ( KeyValues *pData = pTrackKeys->GetFirstSubKey(); pData != NULL; pData = pData->GetNextKey() )
	{
		int iTrackID = atoi( pData->GetName() );
		if ( iTrackID == 0 )
			continue;

		MusicData_t musicData;

		V_strcpy_safe( musicData.szName, pData->GetString( "name", "" ) );
		V_strcpy_safe( musicData.szComposer, pData->GetString( "composer", "" ) );

		KeyValues *pSegmentsData = pData->FindKey( "segments" );

		if ( pSegmentsData )
		{
			for ( int i = 0; i < TF_MUSIC_NUM_SEGMENTS; i++ )
			{
				V_strcpy_safe( musicData.aSegments[i], pSegmentsData->GetString( g_aMusicSegmentNames[i], "" ) );
			}
		}

		m_Tracks.Insert( iTrackID, musicData );
	}

	pTrackKeys->deleteThis();
}
