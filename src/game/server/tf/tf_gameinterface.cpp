//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "gameinterface.h"
#include "mapentities.h"
#include "filesystem.h"
#include "steam/steam_api.h"
#include "tier0/icommandline.h"
#include "tier3/tier3.h"
#include "vgui/ILocalize.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// -------------------------------------------------------------------------------------------- //
// Mod-specific CServerGameClients implementation.
// -------------------------------------------------------------------------------------------- //
void CServerGameClients::GetPlayerLimits( int& minplayers, int& maxplayers, int &defaultMaxPlayers ) const
{
	minplayers = 2;  // Force multiplayer.
	maxplayers = MAX_PLAYERS - 1; // MAX_PLAYERS is not a power of 2 (see shareddefs.h), need to fix it up here...
	defaultMaxPlayers = 24;
}

// -------------------------------------------------------------------------------------------- //
// Mod-specific CServerGameDLL implementation.
// ------------------------------------------------------------------------------------------- //
void CServerGameDLL::LevelInit_ParseAllEntities( const char *pMapEntities )
{
}

// These classes aren't needed elsewhere.
class CSearchPath
{
	char		m_szData[24];
};

// VPK Data
class CPackedStore
{
	char		m_szData[12];

public:
	char		m_pszFileBaseName[MAX_PATH];
	char		m_pszFullPathName[MAX_PATH];

};

// Directories and VPKs
class CPath
{
public:
	CPath()
	{
		m_iOrder = -1;
		m_szName = "";
		m_szPathID = "GAME";
	}

	int			m_iOrder;
	const char	*m_szName;
	const char	*m_szPathID;

};

//-----------------------------------------------------------------------------
// Purpose: Parses GameInfo.txt to see if external mounts are requested.
//-----------------------------------------------------------------------------
void ApplyFilesystemModifications( void )
{
	// Experimental stuff, enable sometime in the future when ready.
#ifdef STAGING_ONLY
	if ( !CommandLine()->FindParm( "-nocontentmounting" ) && !engine->IsDedicatedServer() )
	{
		bool bNoSteam = false;

		// TODO: Have this execute after a game directory is added using ( "resource/%s_%language%.txt", szGamePath ).
		g_pVGuiLocalize->AddFile( "resource/tf_%language%.txt", "GAME", true );

		g_pFullFileSystem->AsyncFinishAll();

		// Pull from 'GameInfo.txt'.
		KeyValues *pMainFile = new KeyValues( "GameInfo" );
		pMainFile->LoadFromFile( g_pFullFileSystem, "GameInfo.txt", "GAME" );
		if ( pMainFile )
		{
			// Hop into the 'FileSystem' section.
			KeyValues *pFileSystemInfo = pMainFile->FindKey( "FileSystem" );
			if ( pFileSystemInfo )
			{
				// Poke around in the filesystem for a reference to the paths it holds.
				CUtlVector<CSearchPath> *m_SearchPaths = &*(CUtlVector<CSearchPath> *)( (uint8 *)g_pFullFileSystem + 72 );
				FOR_EACH_SUBKEY( pFileSystemInfo, pFileSystemKey )
				{
					const char *szFileSystemKeyName = pFileSystemKey->GetName();

					// Parse additional content.
					if ( !strcmp( szFileSystemKeyName, "SteamAdditionalContent" ) )
					{
						if ( !steamapicontext || !steamapicontext->SteamApps() )
						{
							// No Steam, can't retrieve anything...
							bNoSteam = true;
							break;
						}

						int iAppID = -1;
						char szSubDir[MAX_PATH];
						bool bAbsolutePath = false;

						// Parse everything and queue it up.
						CUtlVector<CPath> pPaths;
						FOR_EACH_SUBKEY( pFileSystemKey, pAdditionalContentKey )
						{
							const char *pszContentKeyName = pAdditionalContentKey->GetName();
							if ( !strcmp( pszContentKeyName, "AppId" ) )
							{
								iAppID = pAdditionalContentKey->GetInt();
							}
							// Relativity should work.
							else if ( !strcmp( pszContentKeyName, "Dir" ) )
							{
								V_strncpy( szSubDir, pAdditionalContentKey->GetString(), sizeof( szSubDir ) );
								bAbsolutePath = true;
							}
							// Dedicated can't go into sub-directories of Steam installs.
							else if ( !bAbsolutePath && !strcmp( pszContentKeyName, "SubDir" ) )
							{
								V_strncpy( szSubDir, pAdditionalContentKey->GetString(), sizeof( szSubDir ) );
							}
							else if ( !strcmp( pszContentKeyName, "Path" ) )
							{
								CPath pPath;
								FOR_EACH_SUBKEY( pAdditionalContentKey, pPathKey )
								{
									const char *pszPathName = pPathKey->GetName();
									if ( !strcmp( pszPathName, "Offset" ) )
									{
										pPath.m_iOrder = pPathKey->GetInt();
									}
									else if ( !strcmp( pszPathName, "Name" ) )
									{
										pPath.m_szName = pPathKey->GetString();
									}
									else if ( !strcmp( pszPathName, "PathID" ) )
									{
										pPath.m_szPathID = pPathKey->GetString();
									}
								}

								pPaths.AddToHead( pPath );
							}
						}

						// If our AppID is valid, then go through the paths we've queued up, parse, and inject into the filesystem.
						if ( iAppID != -1 )
						{
							char szSourcePath[MAX_PATH];
							steamapicontext->SteamApps()->GetAppInstallDir( iAppID, szSourcePath, sizeof( szSourcePath ) );

							if ( szSubDir[0] )
							{
								if ( !bAbsolutePath )
									V_strncat( szSourcePath, "\\", sizeof( szSourcePath ) );

								V_strncat( szSourcePath, szSubDir, sizeof( szSourcePath ) );
							}
							else
							{
								DevWarning( "Invalid directory path in 'AdditionalContent'...\n" );

								// Cleanup.
								pPaths.Purge();
								continue;
							}

							char szGamePath[MAX_PATH];
							V_strcpy_safe( szGamePath, szSourcePath );

							// Go through all of the paths we have queued up to be mounted.
							for ( int i = 0; i < pPaths.Count(); i++ )
							{
								// Bail if we have some invalid data for this index.
								if ( !V_strcmp( pPaths[i].m_szName, "" ) || pPaths[i].m_iOrder < 0 )
									continue;

								char szPathID[32];
						
								V_strcpy_safe( szPathID, pPaths[i].m_szPathID );

								char szPath[MAX_PATH];
								char *szPathIDToken = strtok( szPathID, "+" );
								while ( szPathIDToken != 0 )
								{
									// Sticking this inside of the while loop because 'strtok()' is messing the path up.
									V_strncpy( szPath, szSourcePath, sizeof( szPath ) );
									V_strncat( szPath, "\\", sizeof( szPath ) );
									V_strncat( szPath, pPaths[i].m_szName, sizeof( szPath ) );

									g_pFullFileSystem->AddSearchPath( szPath, szPathIDToken );
									if ( g_pFullFileSystem->GetSearchPath( szPathIDToken, true, szPath, MAX_PATH ) != 0 )
									{
										int iOffset = m_SearchPaths->Count() - pPaths[i].m_iOrder;
										if ( m_SearchPaths->IsValidIndex( iOffset ) )
										{
											CSearchPath pOffsetPath = m_SearchPaths->Element( m_SearchPaths->Count() - 1 );
											m_SearchPaths->Remove( m_SearchPaths->Count() - 1 );
											m_SearchPaths->InsertBefore( iOffset, pOffsetPath );
										}
										else
										{
											DevWarning( "Directory path hit an invalid offset, leaving it where it is." );
										}
									}

									szPathIDToken = strtok( 0, "+" );
								}
							}

							// The actual directory will have the lowest file priority.
							if ( szGamePath[0] )
							{
								g_pFullFileSystem->AddSearchPath( szGamePath, "GAME" );
							}
						}

						// Cleanup.
						pPaths.Purge();
					}
				}
			}
		}

		pMainFile->deleteThis();
	
		if ( bNoSteam )
		{
			DevWarning( "Unable to retrieve some content from Steam.\n" );
		}
	}
#endif
}
