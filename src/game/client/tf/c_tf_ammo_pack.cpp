//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
//=============================================================================//
#include "cbase.h"
#include "c_baseanimating.h"
#include "engine/ivdebugoverlay.h"
#include "c_tf_player.h"
#include "engine/IEngineSound.h"
#include "soundenvelope.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class C_TFAmmoPack : public C_BaseAnimating, public ITargetIDProvidesHint
{
	DECLARE_CLASS( C_TFAmmoPack, C_BaseAnimating );

public:

	DECLARE_CLIENTCLASS();

	~C_TFAmmoPack();

	virtual int		DrawModel( int flags );
	virtual bool	OnInternalDrawModel( ClientModelRenderInfo_t *pInfo );
	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual bool	Interpolate( float currentTime );

	virtual CStudioHdr *OnNewModel( void );

	// Mirrors C_EconEntity
	CEconItemView *GetItem( void );

	// ITargetIDProvidesHint
public:
	virtual void	DisplayHintTo( C_BasePlayer *pPlayer );

private:
	// Looping sound emitted by dropped flamethrowers
	CNetworkVar( bool, m_bPilotLight );
	CSoundPatch *m_pPilotLightSound;

	// Mirrors C_EconEntity
protected:
	CEconItemView m_Item;

public:
	CUtlVector<AttachedModelData_t>	m_vecAttachedModels;

};

static ConVar tf_debug_weapontrail( "tf_debug_weapontrail", "0", FCVAR_CHEAT );
static ConVar tf2c_highlight_ammo( "tf2c_highlight_ammo", "1", FCVAR_ARCHIVE, "Enable to make dropped weapons (ammo packs) glow" );

// Network table.
IMPLEMENT_CLIENTCLASS_DT( C_TFAmmoPack, DT_AmmoPack, CTFAmmoPack )
	RecvPropBool( RECVINFO( m_bPilotLight ) ),
	RecvPropDataTable( RECVINFO_DT( m_Item ), 0, &REFERENCE_RECV_TABLE( DT_ScriptCreatedItem ) ),
END_RECV_TABLE()

void UpdateEconEntityAttachedModels( CEconItemView *pItem, CUtlVector<AttachedModelData_t> *vecAttachedModels, int iTeamNumber = FIRST_GAME_TEAM );
void DrawEconEntityAttachedModels( CBaseAnimating *pEnt, CUtlVector<AttachedModelData_t> *vecAttachedModels, const ClientModelRenderInfo_t *pInfo, int iMatchDisplayFlags );

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
C_TFAmmoPack::~C_TFAmmoPack()
{
	if ( m_pPilotLightSound )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pPilotLightSound );
		m_pPilotLightSound = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flags - 
// Output : int
//-----------------------------------------------------------------------------
int C_TFAmmoPack::DrawModel( int flags )
{
	// Debug!
	if ( tf_debug_weapontrail.GetBool() )
	{
		Msg( "Ammo Pack:: Position: (%f %f %f), Velocity (%f %f %f)\n", GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z, GetAbsVelocity().x, GetAbsVelocity().y, GetAbsVelocity().z );
		debugoverlay->AddBoxOverlay( GetAbsOrigin(), Vector( -2, -2, -2 ), Vector( 2, 2, 2 ), QAngle( 0, 0, 0 ), 255, 255, 0, 32, 5.0 );
	}

	return BaseClass::DrawModel( flags );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flags - 
// Output : ClientModelRenderInfo_t *
//-----------------------------------------------------------------------------
bool C_TFAmmoPack::OnInternalDrawModel( ClientModelRenderInfo_t *pInfo )
{
	if ( !BaseClass::OnInternalDrawModel( pInfo ) )
		return false;

	DrawEconEntityAttachedModels( this, &m_vecAttachedModels, pInfo, kAttachedModelDisplayFlag_WorldModel );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void C_TFAmmoPack::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	// Debug!
	if ( tf_debug_weapontrail.GetBool() )
	{
		Msg( "AbsOrigin (%f %f %f), LocalOrigin(%f %f %f)\n", GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z, GetLocalOrigin().x, GetLocalOrigin().y, GetLocalOrigin().z );
	}

	if ( updateType == DATA_UPDATE_CREATED )
	{ 
		// Debug!
		if ( tf_debug_weapontrail.GetBool() )
		{
			Msg( "Origin (%f %f %f)\n", GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z );
		}

		float flChangeTime = GetLastChangeTime( LATCH_SIMULATION_VAR );
		Vector vecCurOrigin = GetLocalOrigin();

		// Now stick our initial velocity into the interpolation history 
		CInterpolatedVar< Vector > &interpolator = GetOriginInterpolator();
		interpolator.ClearHistory();
		interpolator.AddToHead( flChangeTime - 0.15f, &vecCurOrigin, false );

		UpdateEconEntityAttachedModels( GetItem(), &m_vecAttachedModels, GetTeamNumber() );

		if ( tf2c_highlight_ammo.GetBool() )
		{
			AddEffects( EF_ITEM_BLINK );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : currentTime - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_TFAmmoPack::Interpolate( float currentTime )
{
	return BaseClass::Interpolate( currentTime );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void C_TFAmmoPack::DisplayHintTo( C_BasePlayer *pPlayer )
{
	C_TFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
	if ( pTFPlayer->IsPlayerClass( TF_CLASS_ENGINEER, true ) )
	{
		pTFPlayer->HintMessage( HINT_ENGINEER_PICKUP_METAL );
	}
	else
	{
		pTFPlayer->HintMessage( HINT_PICKUP_AMMO );
	}
}


CStudioHdr *C_TFAmmoPack::OnNewModel( void )
{
	CStudioHdr *hdr = BaseClass::OnNewModel();

	if ( m_bPilotLight )
	{
		// Create the looping pilot light sound
		const char *pilotlightsound = "Weapon_FlameThrower.PilotLoop";
		CLocalPlayerFilter filter;

		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		m_pPilotLightSound = controller.SoundCreate( filter, entindex(), pilotlightsound );

		controller.Play( m_pPilotLightSound, 1.0, 100 );
	}
	else
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pPilotLightSound );
		m_pPilotLightSound = NULL;
	}

	return hdr;
}


CEconItemView *C_TFAmmoPack::GetItem( void )
{
	return &m_Item;
}
