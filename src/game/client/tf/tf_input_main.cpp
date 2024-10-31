//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: TF2 specific input handling
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "input.h"
#include "cam_thirdperson.h"
#include "c_tf_player.h"

ConVar tf2c_thirdperson( "tf2c_thirdperson", "0", FCVAR_USERINFO | FCVAR_ARCHIVE, "Enables/Disables third person. Only works if tf2c_allow_thirdperson is 1 on the server." );
ConVar tf_tauntcam_yaw( "tf_tauntcam_yaw", "0", FCVAR_CHEAT );
ConVar tf_tauntcam_pitch( "tf_tauntcam_pitch", "0", FCVAR_CHEAT );
ConVar tf_tauntcam_dist( "tf_tauntcam_dist", "150", FCVAR_CHEAT );

extern ConVar cam_idealdist;
extern ConVar cam_idealdistright;
extern ConVar cam_idealdistup;

extern const ConVar *sv_cheats;

//-----------------------------------------------------------------------------
// Purpose: TF Input interface
//-----------------------------------------------------------------------------
class CTFInput : public CInput
{
public:
	CTFInput()
	{
		m_bCheatThirdPerson = false;
	}

	virtual void CAM_Think( void )
	{
		C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( !pPlayer )
			return;

		// If we're in cheat third person mode and sv_cheats are disabled snap back to first person.
		if ( CAM_IsThirdPerson() && m_bCheatThirdPerson && sv_cheats && !sv_cheats->GetBool() )
		{
			if ( pPlayer->InThirdPersonShoulder() )
			{
				m_ShoulderCameraData.m_flPitch = 0.0f;
				m_ShoulderCameraData.m_flYaw = 0.0f;
				m_ShoulderCameraData.m_flDist = TF_CAMERA_DIST;
				m_ShoulderCameraData.m_flLag = 4.0f;

				QAngle angCamera;
				engine->GetViewAngles( angCamera );

				Vector vecOffset( TF_CAMERA_DIST, TF_CAMERA_DIST_RIGHT, TF_CAMERA_DIST_UP );
				// Flip the angle if viewmodels are flipped.
				if ( pPlayer->ShouldFlipViewModel() )
				{
					vecOffset.y *= -1.0f;
				}

				g_ThirdPersonManager.SetDesiredCameraOffset( vecOffset );
				CAM_SetCameraThirdData( &m_ShoulderCameraData, angCamera );
				m_bCheatThirdPerson = false;
			}
			else
			{
				CAM_ToFirstPerson();
			}
		}

		if ( !m_bCheatThirdPerson && !pPlayer->InTauntCam() )
		{
			// Maintain proper shoulder third person camera state.
			if ( pPlayer->InThirdPersonShoulder() )
			{
				if ( !CAM_IsThirdPerson() )
				{
					CAM_ToThirdPerson();
				}
			}
			else
			{
				if ( CAM_IsThirdPerson() )
				{
					CAM_ToFirstPerson();
				}
			}
		}

		CInput::CAM_Think();
	}

	virtual void CAM_ToThirdPerson( void )
	{
		CInput::CAM_ToThirdPerson();

		C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( !pPlayer )
			return;

		g_ThirdPersonManager.SetOverridingThirdPerson( true );

		if ( pPlayer->InThirdPersonShoulder() )
		{
			m_ShoulderCameraData.m_flPitch = 0.0f;
			m_ShoulderCameraData.m_flYaw = 0.0f;
			m_ShoulderCameraData.m_flDist = TF_CAMERA_DIST;
			m_ShoulderCameraData.m_flLag = 4.0f;

			QAngle angCamera;
			engine->GetViewAngles( angCamera );

			Vector vecOffset( TF_CAMERA_DIST, TF_CAMERA_DIST_RIGHT, TF_CAMERA_DIST_UP );
			// Flip the angle if viewmodels are flipped.
			if ( cl_flipviewmodels.GetBool() )
			{
				vecOffset.y *= -1.0f;
			}

			g_ThirdPersonManager.SetDesiredCameraOffset( vecOffset );
			CAM_SetCameraThirdData( &m_ShoulderCameraData, angCamera );
			m_bCheatThirdPerson = false;

			pPlayer->ThirdPersonSwitch( true );
		}
		else if ( pPlayer->InTauntCam() )
		{
			m_TauntCameraData.m_flPitch = tf_tauntcam_pitch.GetFloat();
			m_TauntCameraData.m_flYaw = tf_tauntcam_yaw.GetFloat();
			m_TauntCameraData.m_flDist = tf_tauntcam_dist.GetFloat();
			m_TauntCameraData.m_flLag = 4.0f;
			m_TauntCameraData.m_vecHullMin.Init( -9.0f, -9.0f, -9.0f );
			m_TauntCameraData.m_vecHullMax.Init( 9.0f, 9.0f, 9.0f );

			QAngle angCamera;
			engine->GetViewAngles( angCamera );

			g_ThirdPersonManager.SetDesiredCameraOffset( Vector( tf_tauntcam_dist.GetFloat(), 0.0f, 0.0f ) );
			CAM_SetCameraThirdData( &m_TauntCameraData, angCamera );
			m_bCheatThirdPerson = false;

			pPlayer->ThirdPersonSwitch( true );
		}
		else
		{
			g_ThirdPersonManager.SetDesiredCameraOffset( Vector( cam_idealdist.GetFloat(), cam_idealdistright.GetFloat(), cam_idealdistup.GetFloat() ) );
			m_bCheatThirdPerson = true;
		}
	}

	virtual void CAM_ToFirstPerson( void )
	{
		CInput::CAM_ToFirstPerson();

		CAM_SetCameraThirdData( NULL, vec3_angle );
		m_bCheatThirdPerson = false;
	}

private:
	CameraThirdData_t m_TauntCameraData;
	CameraThirdData_t m_ShoulderCameraData;
	bool m_bCheatThirdPerson;
};

static CTFInput g_Input;

// Expose this interface
IInput *input = ( IInput * )&g_Input;

