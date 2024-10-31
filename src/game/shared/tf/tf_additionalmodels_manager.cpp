//=============================================================================//
//
// Purpose: Spawns addition props on specified maps.
//
//=============================================================================//
#include "cbase.h"
#include "filesystem.h"
#include "time.h"
#include <KeyValues.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CTFAdditionalModelsManager : public CAutoGameSystemPerFrame
{
public:
	CTFAdditionalModelsManager( const char *pszName );
	~CTFAdditionalModelsManager();

	// IGameSystem
	virtual bool Init( void );
	virtual void Shutdown( void );
	virtual void LevelInitPostEntity( void );
	virtual void LevelShutdownPostEntity( void );

	// Gets called each frame.
	virtual void Update( float flFrameTime );

#ifndef CLIENT_DLL
	void ParseAdditionalModels( KeyValues *pSubKey, CBaseEntity *pOwner = NULL );
	void ParseEntity( KeyValues *pSubKey, bool bAnimated = false, CBaseEntity *pOwner = NULL );

	const char* GetEventName( void ) { return "normal"; }

	KeyValues* pkvFile;
#endif
	
} g_AdditionalModelsManager( "CTFAdditionalModelsManager" );

CTFAdditionalModelsManager *GetAdditionalModelsManager()
{
	return &g_AdditionalModelsManager;
}

CTFAdditionalModelsManager::CTFAdditionalModelsManager( const char *pszName ) : CAutoGameSystemPerFrame( pszName )
{
}

CTFAdditionalModelsManager::~CTFAdditionalModelsManager()
{
}


bool CTFAdditionalModelsManager::Init( void )
{
	return true;
}


void CTFAdditionalModelsManager::Shutdown( void )
{
}


void CTFAdditionalModelsManager::LevelInitPostEntity( void )
{
#ifndef CLIENT_DLL
	if (pkvFile)
	{
		pkvFile->deleteThis();
		pkvFile = nullptr;
	}
	pkvFile = new KeyValues( "AdditionalModels" );
	if ( pkvFile->LoadFromFile( filesystem, "scripts/additional_models.txt", "MOD" ) )
	{
		DevMsg( "Parsing additional models...\n" );

		// Add support for events later.
		ParseAdditionalModels( pkvFile->FindKey( STRING( gpGlobals->mapname ) ) );
	}
#endif
}


void CTFAdditionalModelsManager::LevelShutdownPostEntity( void )
{
#ifndef CLIENT_DLL
	if (pkvFile)
	{
		pkvFile->deleteThis();
		pkvFile = nullptr;
	}
#endif
}


void CTFAdditionalModelsManager::Update( float flFrameTime )
{
}

#ifndef CLIENT_DLL

void CTFAdditionalModelsManager::ParseAdditionalModels( KeyValues *pSubKey, CBaseEntity *pOwner /*= NULL*/ )
{
	if ( pSubKey )
	{
		KeyValues *pModelType = pSubKey->GetFirstSubKey();
		while ( pModelType )
		{
			if ( !stricmp( pModelType->GetName(), "animated" ) )
			{
				ParseEntity( pModelType, true, pOwner );
			}
			else if ( !stricmp( pModelType->GetName(), "static" ) )
			{
				ParseEntity( pModelType, false, pOwner );
			}

			pModelType = pModelType->GetNextKey();
		}
	}
}


void CTFAdditionalModelsManager::ParseEntity( KeyValues *pSubKey, bool bAnimated /*= false*/, CBaseEntity *pOwner /*= NULL*/ )
{
	if ( pSubKey )
	{
		Vector vecOrigin( 0.0f, 0.0f, 0.0f );
		QAngle vecAngles( 0.0f, 0.0f, 0.0f );

		const char *pszModel = "models/error.mdl";
		int	nSkin = 0;
		float flScale = 1.0f;
		const char *pszSequence = "ref";

		CBaseAnimating *pAdditionalModel = static_cast< CBaseAnimating * >( CBaseEntity::CreateNoSpawn( "prop_dynamic", vecOrigin, vecAngles, pOwner ) );
		if ( pAdditionalModel )
		{
			KeyValues *pEntityDataKey = pSubKey->GetFirstSubKey();
			while ( pEntityDataKey )
			{
				if ( !stricmp( pEntityDataKey->GetName(), "model" ) )
				{
					pszModel = pEntityDataKey->GetString();
				}
				else if ( !stricmp( pEntityDataKey->GetName(), "skin" ) )
				{
					nSkin = pEntityDataKey->GetInt();
				}
				else if ( !stricmp( pEntityDataKey->GetName(), "scale" ) )
				{
					flScale = pEntityDataKey->GetFloat();
				}
				else if ( !stricmp( pEntityDataKey->GetName(), "attached_models" ) )
				{
					ParseAdditionalModels( pEntityDataKey, pAdditionalModel );
				}
				else if ( !stricmp( pEntityDataKey->GetName(), "modifiers" ) )
				{
					KeyValues *pModifierKey = pEntityDataKey->GetFirstSubKey();
					while ( pModifierKey )
					{
						bool bHidden = false;

						if ( !stricmp( pModifierKey->GetName(), "event" ) )
						{
							char *szToken = strtok( (char *)pModifierKey->GetString(), "," );
							while ( szToken != 0 )
							{
								if ( !stricmp( szToken, GetEventName() ) )
								{
									bHidden = false;
									break;
								}

								szToken = strtok( 0, "," );
								bHidden = true;
							}
						}
						else // Time related modifiers
						{
							time_t ltime = time( 0 );
							const time_t *ptime = &ltime;
							struct tm *today = localtime( ptime );

							if ( today )
							{
								if ( !stricmp( pModifierKey->GetName(), "second" ) )
								{
									char *szToken = strtok( (char *)pModifierKey->GetString(), "," );
									while ( szToken != 0 )
									{
										if ( ( today->tm_sec + 1 ) == atoi( szToken ) )
										{
											bHidden = false;
											break;
										}

										szToken = strtok( 0, "," );
										bHidden = true;
									}
								}
								else if ( !stricmp( pModifierKey->GetName(), "minute" ) )
								{
									char *szToken = strtok( (char *)pModifierKey->GetString(), "," );
									while ( szToken != 0 )
									{
										if ( ( today->tm_min + 1 ) == atoi( szToken ) )
										{
											bHidden = false;
											break;
										}

										szToken = strtok( 0, "," );
										bHidden = true;
									}
								}
								else if ( !stricmp( pModifierKey->GetName(), "hour" ) )
								{
									char *szToken = strtok( (char *)pModifierKey->GetString(), "," );
									while ( szToken != 0 )
									{
										if ( ( today->tm_hour + 1 ) == atoi( szToken ) )
										{
											bHidden = false;
											break;
										}

										szToken = strtok( 0, "," );
										bHidden = true;
									}
								}
								else if ( !stricmp( pModifierKey->GetName(), "day" ) )
								{
									char *szToken = strtok( (char *)pModifierKey->GetString(), "," );
									while ( szToken != 0 )
									{
										if ( today->tm_mday == atoi( szToken ) )
										{
											bHidden = false;
											break;
										}

										szToken = strtok( 0, "," );
										bHidden = true;
									}
								}
								else if ( !stricmp( pModifierKey->GetName(), "month" ) )
								{
									
									char *szToken = strtok( (char *)pModifierKey->GetString(), "," );
									while ( szToken != 0 )
									{
										if ( ( today->tm_mon + 1 ) == atoi( szToken ) )
										{
											bHidden = false;
											break;
										}

										szToken = strtok( 0, "," );
										bHidden = true;
									}
								}
								else if ( !stricmp( pModifierKey->GetName(), "weekday" ) )
								{
									char *szToken = strtok( (char *)pModifierKey->GetString(), "," );
									while ( szToken != 0 )
									{
										if ( today->tm_wday == atoi( szToken ) )
										{
											bHidden = false;
											break;
										}

										szToken = strtok( 0, "," );
										bHidden = true;
									}
								}
							}
						}

						if ( bHidden )
						{
							UTIL_Remove( pAdditionalModel );
							return;
						}

						pModifierKey = pModifierKey->GetNextKey();
					}
				}
				else
				{
					if ( bAnimated && !stricmp( pEntityDataKey->GetName(), "sequence" ) )
					{
						pszSequence = pEntityDataKey->GetString();
					}
					else if ( !pOwner )
					{
						if ( !stricmp( pEntityDataKey->GetName(), "origin" ) )
						{
							UTIL_StringToVector( vecOrigin.Base(), pEntityDataKey->GetString() );
						}
						else if ( !stricmp( pEntityDataKey->GetName(), "angles" ) )
						{
							UTIL_StringToVector( vecAngles.Base(), pEntityDataKey->GetString() );
						}
					}
				}

				pEntityDataKey = pEntityDataKey->GetNextKey();
			}

			CBaseEntity::PrecacheModel( pszModel );

			pAdditionalModel->SetAbsOrigin( vecOrigin );
			pAdditionalModel->SetAbsAngles( vecAngles );
		
			pAdditionalModel->SetName( MAKE_STRING( "map_additional_model" ) );
			pAdditionalModel->SetModelName( AllocPooledString( pszModel ) );
			pAdditionalModel->SetModel( pszModel );
			pAdditionalModel->SetModelScale( flScale );
			pAdditionalModel->m_nSkin = nSkin;

			DispatchSpawn( pAdditionalModel );

			pAdditionalModel->SetSolidFlags( FSOLID_NOT_SOLID );

			if ( pOwner )
			{
				pAdditionalModel->FollowEntity( pOwner, true );
				pAdditionalModel->SetOwnerEntity( pOwner );
			}

			if ( bAnimated )
			{
				pAdditionalModel->SetSequence( pAdditionalModel->LookupSequence( pszSequence ) );
				pAdditionalModel->ResetSequenceInfo();

				if ( pAdditionalModel->IsUsingClientSideAnimation() )
				{
					pAdditionalModel->ResetClientsideFrame();
				}
			}
		}
	}
}
#endif