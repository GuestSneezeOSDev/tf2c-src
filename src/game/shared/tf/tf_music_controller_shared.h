#ifndef TF_MUSIC_CONTROLLER_SHARED_H
#define TF_MUSIC_CONTROLLER_SHARED_H

#ifdef _WIN32
#pragma once
#endif

#define TF_MUSIC_DATA_FILE "scripts/deathmatch/music_tracks.txt"

enum
{
	TF_MUSIC_LOOP = 0,
	TF_MUSIC_WAITING,
	TF_MUSIC_ENDING,
	TF_MUSIC_NUM_SEGMENTS,
};

struct MusicData_t
{
	MusicData_t()
	{
		szName[0] = '\0';
		szComposer[0] = '\0';

		memset( aSegments, 0, sizeof( aSegments ) );
	}

	char szName[128];
	char szComposer[128];

	char aSegments[TF_MUSIC_NUM_SEGMENTS][MAX_PATH];
};

extern const char *g_aMusicSegmentNames[TF_MUSIC_NUM_SEGMENTS];

#endif // TF_MUSIC_CONTROLLER_SHARED_H
