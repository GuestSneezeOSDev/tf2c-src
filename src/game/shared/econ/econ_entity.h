//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//

#ifndef ECON_ENTITY_H
#define ECON_ENTITY_H

#ifdef _WIN32
#pragma once
#endif

#if defined( CLIENT_DLL )
#define CEconEntity C_EconEntity
#endif

#include "ihasattributes.h"
#include "econ_item_view.h"
#include "attribute_manager.h"

class C_ViewmodelAttachmentModel;

struct wearableanimplayback_t
{
	int iStub;
};

enum
{
	VMTYPE_NONE = -1,	// Hasn't been set yet. We should never have this.
	VMTYPE_HL2 = 0,		// HL2-Type vmodels. Hands, weapon and anims are in the same model. (Used in HL1, HL2, CS:S, DOD:S, pretty much any old-gen Valve game)
	VMTYPE_TF2,			// TF2-Type cmodels. Hands are a separate model, anims are in the hands model. (Only used in live TF2)
	VMTYPE_L4D			// L4D-Type vmodels. Hands are a separate model, anims are in the weapon model. (Used in L4D, L4D2, Portal 2, CS:GO)
};

#ifdef CLIENT_DLL
// Additional attachments.
struct AttachedModelData_t
{
	const model_t *m_pModel;
	int m_iModelDisplayFlags;
};
#endif

//-----------------------------------------------------------------------------
// Purpose: BaseCombatWeapon is derived from this in live tf2.
//-----------------------------------------------------------------------------
class CEconEntity : public CBaseAnimating, public IHasAttributes
{
	DECLARE_CLASS( CEconEntity, CBaseAnimating );
	DECLARE_NETWORKCLASS();
	DECLARE_ENT_SCRIPTDESC( CEconEntity );

#ifdef CLIENT_DLL
	DECLARE_PREDICTABLE();
#endif

public:
	CEconEntity();
	~CEconEntity();

#ifdef CLIENT_DLL
	virtual void	OnPreDataChanged( DataUpdateType_t );
	virtual void	OnDataChanged( DataUpdateType_t );
	virtual bool	OnFireEvent( C_BaseViewModel *pViewModel, const Vector& origin, const QAngle& angles, int event, const char *options );

	// Attachments
	virtual bool	AttachmentModelsShouldBeVisible( void ) { return true; }
	virtual void	UpdateAttachmentModels( void );
	virtual void	ViewModelAttachmentBlending( CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask, C_ViewmodelAttachmentModel *pAttachment );
	virtual bool	OnInternalDrawModel( ClientModelRenderInfo_t *pInfo );
#endif

	virtual void	UpdateOnRemove( void );

	virtual void	GiveTo( CBaseEntity *pEntity );

	virtual int		TranslateViewmodelHandActivity( int iActivity ) { return iActivity; }

	virtual void	PlayAnimForPlaybackEvent( wearableanimplayback_t iPlayback ) {};

	void			UpdatePlayerModelToClass( void );
	static void		UpdateWeaponBodygroups( CBasePlayer *pPlayer, bool bForce = false );
	virtual bool	UpdateBodygroups( CBasePlayer *pOwner, bool bForce );

	virtual void	SetItem( CEconItemView const &newItem );
	CEconItemView	*GetItem() { return m_AttributeManager.GetItem(); }
	CEconItemView const *GetItem() const { return m_AttributeManager.GetItem(); }
	virtual bool	HasItemDefinition( void ) const;
	virtual int		GetItemID( void );

	virtual CAttributeManager	*GetAttributeManager( void ) { return &m_AttributeManager; }
	virtual CAttributeContainer	*GetAttributeContainer( void ) { return &m_AttributeManager; }
	virtual CBaseEntity			*GetAttributeOwner( void ) { return NULL; }
	virtual CAttributeList		*GetAttributeList( void ) { return GetItem()->GetAttributeList(); }
	virtual void				ReapplyProvision( void );
	void						InitializeAttributes( void );

	void			AddAttribute( char const *pszAttribName, float flValue, float flDuration );
	void			RemoveAttribute( char const *pszAttributeName );
	void			ScriptReapplyProvision() { ReapplyProvision(); }

protected:
	EHANDLE			m_hOldOwner;

private:
	CNetworkVarEmbedded( CAttributeContainer, m_AttributeManager );
	
#ifdef CLIENT_DLL
public:
	CUtlVector<AttachedModelData_t>	m_vecAttachedModels;
#endif

};
#endif
