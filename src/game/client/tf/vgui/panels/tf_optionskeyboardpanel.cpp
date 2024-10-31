//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
//===========================================================================//
#include "cbase.h"

#include "tf_optionskeyboardpanel.h"
#include "tf_mainmenu.h"
#include "controls/tf_vcontrolslistpanel.h"
#include "controls/tf_advbutton.h"

#include <vgui_controls/Button.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/ListPanel.h>
#include <vgui_controls/QueryBox.h>

#include <vgui/Cursor.h>
#include <vgui/IVGui.h>
#include <vgui/ISurface.h>
#include "tier1/KeyValues.h"
#include "tier1/convar.h"
#include <vgui/KeyCode.h>
#include <vgui/MouseCode.h>
#include <vgui/ISystem.h>
#include <vgui/IInput.h>

#include "filesystem.h"
#include "tier1/utlbuffer.h"
#include "IGameUIFuncs.h"
#include <vstdlib/IKeyValuesSystem.h>
#include "tier2/tier2.h"
#include "inputsystem/iinputsystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFOptionsKeyboardPanel::CTFOptionsKeyboardPanel(vgui::Panel *parent, const char *panelName) : CTFDialogPanelBase(parent, panelName)
{
	bParsed = false;
	memset( m_Bindings, 0, sizeof( m_Bindings ) );

	// create the key bindings list
	CreateKeyBindingList();
	// Store all current key bindings
	SaveCurrentBindings();

	m_pSetBindingButton = new CTFButton( this, "ChangeKeyButton", "" );
	m_pClearBindingButton = new CTFButton( this, "ClearKeyButton", "" );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFOptionsKeyboardPanel::~CTFOptionsKeyboardPanel()
{
	DeleteSavedBindings();
}

//-----------------------------------------------------------------------------
// Purpose: sets background color & border
//-----------------------------------------------------------------------------
void CTFOptionsKeyboardPanel::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	LoadControlSettings("Resource/UI/main_menu/OptionsKeyboard.res");
	//Msg("APPLIED!\n");
	m_pSetBindingButton->SetEnabled(false);
	m_pClearBindingButton->SetEnabled(false);

	// Parse default descriptions
	if (!bParsed)
		ParseActionDescriptions();

	m_pKeyBindList->InvalidateLayout(false, true);
}

//-----------------------------------------------------------------------------
// Purpose: reloads current keybinding
//-----------------------------------------------------------------------------
void CTFOptionsKeyboardPanel::OnResetData()
{
	BaseClass::OnResetData();

	// Populate list based on current settings
	FillInCurrentBindings();
	if ( IsVisible() )
	{
		m_pKeyBindList->SetSelectedItem(0);
	}
}

//-----------------------------------------------------------------------------
// Purpose: saves out keybinding changes
//-----------------------------------------------------------------------------
void CTFOptionsKeyboardPanel::OnApplyChanges()
{
	BaseClass::OnApplyChanges();

	ApplyAllBindings();
}

//-----------------------------------------------------------------------------
// Purpose: saves out keybinding changes
//-----------------------------------------------------------------------------
void CTFOptionsKeyboardPanel::OnSetDefaults()
{
	BaseClass::OnSetDefaults();

	// Restore defaults from default keybindings file
	FillInDefaultBindings();
	m_pKeyBindList->RequestFocus();
}

//-----------------------------------------------------------------------------
// Purpose: Create key bindings list control
//-----------------------------------------------------------------------------
void CTFOptionsKeyboardPanel::CreateKeyBindingList()
{
	// Create the control
	m_pKeyBindList = new vcontrolslistpanel(this, "listpanel_keybindlist");
}

//-----------------------------------------------------------------------------
// Purpose: binds double-clicking or hitting enter in the keybind list to changing the key
//-----------------------------------------------------------------------------
void CTFOptionsKeyboardPanel::OnKeyCodeTyped(vgui::KeyCode code)
{
	if (code == KEY_ENTER)
	{
		OnCommand("ChangeKey");
	}
	else
	{
		BaseClass::OnKeyCodeTyped(code);
	}
}

//-----------------------------------------------------------------------------
// Purpose: command handler
//-----------------------------------------------------------------------------
void CTFOptionsKeyboardPanel::OnCommand( const char *command )
{
	if ( !stricmp( command, "Defaults" )  )
	{
		// Open a box asking if we want to restore defaults.
		QueryBox *pRestoreDefaultsQuery = new QueryBox("#GameUI_KeyboardSettings", "#GameUI_KeyboardSettingsText", guiroot->GetParent());
		if ( pRestoreDefaultsQuery )
		{
			pRestoreDefaultsQuery->AddActionSignalTarget(this);

			pRestoreDefaultsQuery->SetOKCommand(new KeyValues("Command", "command", "DefaultsOK"));

			pRestoreDefaultsQuery->DoModal();
		}
	}
	else if ( !stricmp(command, "DefaultsOK"))
	{
		// Restore defaults from default keybindings file.
		OnSetDefaults();
	}
	else if ( !m_pKeyBindList->IsCapturing() && !stricmp( command, "ChangeKey" ) )
	{
		m_pKeyBindList->StartCaptureMode(dc_blank);
	}
	else if ( !m_pKeyBindList->IsCapturing() && !stricmp( command, "ClearKey" ) )
	{
		OnKeyCodePressed(KEY_DELETE);
        m_pKeyBindList->RequestFocus();
	}
	else if ( !stricmp(command, "Advanced") )
	{
		OpenKeyboardAdvancedDialog();
	}
	else
	{
		BaseClass::OnCommand(command);
	}
}

const char *UTIL_Parse( const char *data, char *token, int sizeofToken )
{
	data = engine->ParseFile( data, token, sizeofToken );
	return data;
}
static char *UTIL_CopyString( const char *in )
{
	int len = strlen( in ) + 1;
	char *out = new char[ len ];
	Q_strncpy( out, in, len );
	return out;
}

#ifndef _XBOX
char *UTIL_va(char *format, ...)
{
	va_list		argptr;
	static char	string[4][1024];
	static int	curstring = 0;
	
	curstring = ( curstring + 1 ) % 4;

	va_start (argptr, format);
	V_vsprintf_safe( string[curstring], format, argptr );
	va_end (argptr);

	return string[curstring];  
}
#endif


void CTFOptionsKeyboardPanel::ParseActionDescriptions( void )
{
	char szBinding[256];
	char szDescription[256];

	KeyValues *item;

	// Load the default keys list
	CUtlBuffer buf( 0, 0, CUtlBuffer::TEXT_BUFFER );
	if ( !g_pFullFileSystem->ReadFile( "scripts/kb_act.lst", NULL, buf ) )
		return;

	const char *data = (const char*)buf.Base();

	int sectionIndex = 0;
	char token[512];
	while ( 1 )
	{
		data = UTIL_Parse( data, token, sizeof(token) );
		// Done.
		if ( strlen( token ) <= 0 )  
			break;

		V_strcpy_safe( szBinding, token );

		data = UTIL_Parse( data, token, sizeof(token) );
		if ( strlen(token) <= 0 )
		{
			break;
		}

		V_strcpy_safe(szDescription, token );

		// Skip '======' rows
		if ( szDescription[ 0 ] != '=' )
		{
			HFont Font = m_pKeyBindList->GetFont();
			// Flag as special header row if binding is "blank"
			if (!stricmp(szBinding, "blank"))
			{
				float width = m_pKeyBindList->GetWide();
				// add header item
				m_pKeyBindList->AddSection(++sectionIndex, szDescription);
				m_pKeyBindList->AddColumnToSection(sectionIndex, "Action", szDescription, SectionedListPanel::COLUMN_BRIGHT, width / 2.0);
				m_pKeyBindList->AddColumnToSection(sectionIndex, "Key", "#GameUI_KeyButton", SectionedListPanel::COLUMN_BRIGHT, width / 2.0);
			}
			else
			{
				// Create a new: blank item
				item = new KeyValues( "Item" );
				
				// fill in data
				item->SetString("Action", szDescription);
				item->SetString("Binding", szBinding);
				item->SetString("Key", "");

				// Add to list
				int itemID = m_pKeyBindList->AddItem(sectionIndex, item);
				m_pKeyBindList->SetItemFont(itemID, Font);
				item->deleteThis();
			}
		}
	}
	bParsed = true;
}

//-----------------------------------------------------------------------------
// Purpose: Search current data set for item which has the specified binding string
// Input  : *binding - string to find
// Output : KeyValues or NULL on failure
//-----------------------------------------------------------------------------
KeyValues *CTFOptionsKeyboardPanel::GetItemForBinding( const char *binding )
{
	static int bindingSymbol = KeyValuesSystem()->GetSymbolForString("Binding");

	// Loop through all items
	for (int i = 0; i < m_pKeyBindList->GetItemCount(); i++)
	{
		KeyValues *item = m_pKeyBindList->GetItemData(m_pKeyBindList->GetItemIDFromRow(i));
		if ( !item )
			continue;

		KeyValues *bindingItem = item->FindKey(bindingSymbol);
		const char *bindString = bindingItem->GetString();

		// Check the "Binding" key
		if (!stricmp(bindString, binding))
			return item;
	}
	// Didn't find it
	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Bind the specified keyname to the specified item
// Input  : *item - Item to which to add the key
//			*keyname - The key to be added
//-----------------------------------------------------------------------------
void CTFOptionsKeyboardPanel::AddBinding( KeyValues *item, const char *keyname )
{
	// See if it's already there as a binding
	if ( !stricmp( item->GetString( "Key", "" ), keyname ) )
		return;

	// Make sure it doesn't live anywhere
	RemoveKeyFromBindItems( item, keyname );

	const char *binding = item->GetString( "Binding", "" );

	// Loop through all the key bindings and set all entries that have
	// the same binding. This allows us to have multiple entries pointing 
	// to the same binding.
	for (int i = 0; i < m_pKeyBindList->GetItemCount(); i++)
	{
		KeyValues *curitem = m_pKeyBindList->GetItemData(m_pKeyBindList->GetItemIDFromRow(i));
		if ( !curitem )
			continue;

		const char *curbinding = curitem->GetString( "Binding", "" );

		if (!stricmp(curbinding, binding))
		{
			curitem->SetString( "Key", keyname );
			m_pKeyBindList->InvalidateItem(i);
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Remove all keys from list
//-----------------------------------------------------------------------------
void CTFOptionsKeyboardPanel::ClearBindItems( void )
{
	for (int i = 0; i < m_pKeyBindList->GetItemCount(); i++)
	{
		KeyValues *item = m_pKeyBindList->GetItemData(m_pKeyBindList->GetItemIDFromRow(i));
		if ( !item )
			continue;

		// Reset keys
		item->SetString( "Key", "" );

		m_pKeyBindList->InvalidateItem(i);
	}

	m_pKeyBindList->InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Remove all instances of the specified key from bindings
//-----------------------------------------------------------------------------
void CTFOptionsKeyboardPanel::RemoveKeyFromBindItems( KeyValues *org_item, const char *key )
{
	Assert( key && key[ 0 ] );
	if ( !key || !key[ 0 ] )
		return;

	int len = Q_strlen( key );
	char *pszKey = new char[len + 1];

	if ( !pszKey )
		return;

	Q_memcpy( pszKey, key, len+1 );

	for (int i = 0; i < m_pKeyBindList->GetItemCount(); i++)
	{
		KeyValues *item = m_pKeyBindList->GetItemData(m_pKeyBindList->GetItemIDFromRow(i));
		if ( !item )
			continue;

		// If it's bound to the primary: then remove it
		if ( !stricmp( pszKey, item->GetString( "Key", "" ) ) )
		{
			bool bClearEntry = true;

			if ( org_item )
			{
				// Only clear it out if the actual binding isn't the same. This allows
				// us to have the same key bound to multiple entries in the keybinding list
				// if they point to the same command.
				const char *org_binding = org_item->GetString( "Binding", "" );
				const char *binding = item->GetString( "Binding", "" );
				if ( !stricmp( org_binding, binding ) )
				{
					bClearEntry = false;
				}
			}

			if ( bClearEntry )
			{
				item->SetString( "Key", "" );
				m_pKeyBindList->InvalidateItem(i);
			}
		}
	}

	delete [] pszKey;

	// Make sure the display is up to date
	m_pKeyBindList->InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Ask the engine for all bindings and set up the list
//-----------------------------------------------------------------------------
void CTFOptionsKeyboardPanel::FillInCurrentBindings( void )
{
	// reset the unbind list
	// we only unbind keys used by the normal config items (not custom binds)
	m_KeysToUnbind.RemoveAll();

	// Clear any current settings
	ClearBindItems();

	bool bJoystick = false;
	ConVarRef var( "joystick" );
	if ( var.IsValid() )
	{
		bJoystick = var.GetBool();
	}

	for ( int i = 0; i < BUTTON_CODE_LAST; i++ )
	{
		// Look up binding
		const char *binding = gameuifuncs->GetBindingForButtonCode( (ButtonCode_t)i );
		if ( !binding )
			continue;

		// See if there is an item for this one?
		KeyValues *item = GetItemForBinding( binding );
		if ( item )
		{
			// Bind it by name
			const char *keyName = g_pInputSystem->ButtonCodeToString( (ButtonCode_t)i );

			// Already in list, means user had two keys bound to this item.  We'll only note the first one we encounter
			char const *currentKey = item->GetString( "Key", "" );
			if ( currentKey && currentKey[ 0 ] )
			{
				ButtonCode_t currentBC = (ButtonCode_t)gameuifuncs->GetButtonCodeForBind( currentKey );

				// If we're using a joystick, joystick bindings override keyboard ones
				bool bShouldOverride = bJoystick && IsJoystickCode((ButtonCode_t)i) && !IsJoystickCode(currentBC);

				if ( !bShouldOverride )
					continue;

				// Remove the key we're about to override from the unbinding list
				m_KeysToUnbind.FindAndRemove( currentKey );
			}

			AddBinding( item, keyName );

			// remember to apply unbinding of this key when we apply
			m_KeysToUnbind.AddToTail( keyName );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Clean up memory used by saved bindings
//-----------------------------------------------------------------------------
void CTFOptionsKeyboardPanel::DeleteSavedBindings( void )
{
	for ( int i = 0; i < BUTTON_CODE_LAST; i++ )
	{
		if ( m_Bindings[ i ].binding )
	{
			delete[] m_Bindings[ i ].binding;
		m_Bindings[ i ].binding = NULL;
	}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Copy all bindings into save array
//-----------------------------------------------------------------------------
void CTFOptionsKeyboardPanel::SaveCurrentBindings( void )
{
    DeleteSavedBindings();
	for (int i = 0; i < BUTTON_CODE_LAST; i++)
	{
		const char *binding = gameuifuncs->GetBindingForButtonCode( (ButtonCode_t)i );
		if ( !binding || !binding[0])
			continue;

		// Copy the binding string
		m_Bindings[ i ].binding = UTIL_CopyString( binding );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Tells the engine to bind a key
//-----------------------------------------------------------------------------
void CTFOptionsKeyboardPanel::BindKey( const char *key, const char *binding )
{
	engine->ClientCmd_Unrestricted( VarArgs( "bind \"%s\" \"%s\"\n", key, binding ) );
}

//-----------------------------------------------------------------------------
// Purpose: Tells the engine to unbind a key
//-----------------------------------------------------------------------------
void CTFOptionsKeyboardPanel::UnbindKey( const char *key )
{
	engine->ClientCmd_Unrestricted( VarArgs( "unbind \"%s\"\n", key ) );
}

//-----------------------------------------------------------------------------
// Purpose: Go through list and bind specified keys to actions
//-----------------------------------------------------------------------------
void CTFOptionsKeyboardPanel::ApplyAllBindings( void )
{
	// unbind everything that the user unbound
	{for (int i = 0; i < m_KeysToUnbind.Count(); i++)
	{
		UnbindKey(m_KeysToUnbind[i].String());
	}}
	m_KeysToUnbind.RemoveAll();

	// free binding memory
    DeleteSavedBindings();

	for (int i = 0; i < m_pKeyBindList->GetItemCount(); i++)
	{
		KeyValues *item = m_pKeyBindList->GetItemData(m_pKeyBindList->GetItemIDFromRow(i));
		if ( !item )
			continue;

		// See if it has a binding
		const char *binding = item->GetString( "Binding", "" );
		if ( !binding || !binding[ 0 ] )
			continue;

		const char *keyname;
		
		// Check main binding
		keyname = item->GetString( "Key", "" );
		if ( keyname && keyname[ 0 ] )
		{
			// Tell the engine
			BindKey( keyname, binding );
            ButtonCode_t code = g_pInputSystem->StringToButtonCode( keyname );
            if ( code != BUTTON_CODE_INVALID )
			{
				m_Bindings[ code ].binding = UTIL_CopyString( binding );
			}
		}
	}

	// Now exec their custom bindings
	engine->ClientCmd_Unrestricted( "exec userconfig.cfg\nhost_writeconfig\n" );
}

//-----------------------------------------------------------------------------
// Purpose: Read in defaults from game's default config file and populate list 
//			using those defaults
//-----------------------------------------------------------------------------
void CTFOptionsKeyboardPanel::FillInDefaultBindings( void )
{
	FileHandle_t fh = g_pFullFileSystem->Open( "cfg/config_default.cfg", "rb" );
	if (fh == FILESYSTEM_INVALID_HANDLE)
		return;

	int size = g_pFullFileSystem->Size(fh);
	CUtlBuffer buf( 0, size, CUtlBuffer::TEXT_BUFFER );
	g_pFullFileSystem->Read( buf.Base(), size, fh );
	g_pFullFileSystem->Close(fh);

	// Clear out all current bindings
	ClearBindItems();

	const char *data = (const char*)buf.Base();

	// loop through all the binding
	while ( data != NULL )
	{
		char cmd[64];
		data = UTIL_Parse( data, cmd, sizeof(cmd) );
		if ( strlen( cmd ) <= 0 )
			break;

		if ( !stricmp(cmd, "bind") )
		{
			// Key name
			char szKeyName[256];
			data = UTIL_Parse( data, szKeyName, sizeof(szKeyName) );
			if ( strlen( szKeyName ) <= 0 )
				break; // Error

			char szBinding[256];
			data = UTIL_Parse( data, szBinding, sizeof(szBinding) );
			if ( strlen( szKeyName ) <= 0 )  
				break; // Error

			// Find item
			KeyValues *item = GetItemForBinding( szBinding );
			if ( item )
			{
				// Bind it
				AddBinding( item, szKeyName );
			}
		}
	}
	
	PostActionSignal(new KeyValues("ApplyButtonEnable"));

	// Make sure console and escape key are always valid
    KeyValues *item = GetItemForBinding( "toggleconsole" );
    if (item)
    {
        // Bind it
        AddBinding( item, "`" );
    }
    item = GetItemForBinding( "cancelselect" );
    if (item)
    {
        // Bind it
        AddBinding( item, "ESCAPE" );
    }
}

//-----------------------------------------------------------------------------
// Purpose: User clicked on item: remember where last active row/column was
//-----------------------------------------------------------------------------
void CTFOptionsKeyboardPanel::ItemSelected(int itemID)
{
	m_pKeyBindList->SetItemOfInterest(itemID);

	if (m_pKeyBindList->IsItemIDValid(itemID))
	{
		// find the details, see if we should be enabled/clear/whatever
		m_pSetBindingButton->SetEnabled(true);

		KeyValues *kv = m_pKeyBindList->GetItemData(itemID);
		if (kv)
		{
			const char *key = kv->GetString("Key", NULL);
			if (key && *key)
			{
				m_pClearBindingButton->SetEnabled(true);
			}
			else
			{
				m_pClearBindingButton->SetEnabled(false);
			}

			if (kv->GetInt("Header"))
			{
				m_pSetBindingButton->SetEnabled(false);
			}
		}
	}
	else
	{
		m_pSetBindingButton->SetEnabled(false);
		m_pClearBindingButton->SetEnabled(false);
	}
}

//-----------------------------------------------------------------------------
// Purpose: called when the capture has finished
//-----------------------------------------------------------------------------
void CTFOptionsKeyboardPanel::Finish( ButtonCode_t code )
{
	int r = m_pKeyBindList->GetItemOfInterest();

	// Retrieve clicked row and column
	m_pKeyBindList->EndCaptureMode( dc_arrow );

	// Find item for this row
	KeyValues *item = m_pKeyBindList->GetItemData(r);
	if ( item )
	{
		// Handle keys: but never rebind the escape key
		// Esc just exits bind mode silently
		if ( code != BUTTON_CODE_NONE && code != KEY_ESCAPE && code != BUTTON_CODE_INVALID )
		{
			// Bind the named key
			AddBinding( item, g_pInputSystem->ButtonCodeToString( code ) );
			PostActionSignal( new KeyValues( "ApplyButtonEnable" ) );	
		}

		m_pKeyBindList->InvalidateItem(r);
	}

	m_pSetBindingButton->SetEnabled(true);
	m_pClearBindingButton->SetEnabled(true);
}

//-----------------------------------------------------------------------------
// Purpose: Scans for captured key presses
//-----------------------------------------------------------------------------
void CTFOptionsKeyboardPanel::OnThink()
{
	BaseClass::OnThink();

	if ( m_pKeyBindList->IsCapturing() )
	{
		ButtonCode_t code = BUTTON_CODE_INVALID;
		if ( engine->CheckDoneKeyTrapping( code ) )
		{
			Finish( code );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Check for enter key and go into keybinding mode if it was pressed
//-----------------------------------------------------------------------------
void CTFOptionsKeyboardPanel::OnKeyCodePressed(vgui::KeyCode code)
{
	// Enter key pressed and not already trapping next key/button press
	if ( !m_pKeyBindList->IsCapturing() )
	{
		// Grab which item was set as interesting
		int r = m_pKeyBindList->GetItemOfInterest();

		// Check that it's visible
		int x, y, w, h;
		bool visible = m_pKeyBindList->GetCellBounds(r, 1, x, y, w, h);
		if (visible)
		{
			if ( KEY_DELETE == code )
			{
				// find the current binding and remove it
				KeyValues *kv = m_pKeyBindList->GetItemData(r);

				const char *key = kv->GetString("Key", NULL);
				if (key && *key)
				{
					RemoveKeyFromBindItems(NULL, key);
				}

				m_pClearBindingButton->SetEnabled(false);
				m_pKeyBindList->InvalidateItem(r);
				PostActionSignal(new KeyValues("ApplyButtonEnable"));

				// message handled, don't pass on
				return;
			}
		}
	}

	// Allow base class to process message instead
	BaseClass::OnKeyCodePressed( code );
}


//-----------------------------------------------------------------------------
// Purpose: advanced keyboard settings dialog
//-----------------------------------------------------------------------------
class CTFOptionsKeyboardPanelAdvancedDlg : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE( CTFOptionsKeyboardPanelAdvancedDlg, vgui::Frame );
public:
	CTFOptionsKeyboardPanelAdvancedDlg( vgui::VPANEL hParent ) : BaseClass( NULL, NULL )
	{
		// parent is ignored, since we want look like we're steal focus from the parent (we'll become modal below)

		SetTitle("#GameUI_KeyboardAdvanced_Title", true);
		SetSize( 280, 140 );
		LoadControlSettings( "resource/OptionsSubKeyboardAdvancedDlg.res" );
		MoveToCenterOfScreen();
		SetSizeable( false );
		SetDeleteSelfOnClose( true );
	}

	virtual void Activate()
	{
		BaseClass::Activate();

		input()->SetAppModalSurface(GetVPanel());

		// reset the data
		ConVarRef con_enable( "con_enable" );
		if ( con_enable.IsValid() )
		{
			SetControlInt("ConsoleCheck", con_enable.GetInt() ? 1 : 0);
		}

		ConVarRef hud_fastswitch( "hud_fastswitch" );
		if ( hud_fastswitch.IsValid() )
		{
			SetControlInt("FastSwitchCheck", hud_fastswitch.GetInt() ? 1 : 0);
		}
	}

	virtual void OnApplyData()
	{
		// apply data
		ConVarRef con_enable( "con_enable" );
		con_enable.SetValue( GetControlInt( "ConsoleCheck", 0 ) );

		ConVarRef hud_fastswitch( "hud_fastswitch" );
		hud_fastswitch.SetValue( GetControlInt( "FastSwitchCheck", 0 ) );
	}

	virtual void OnCommand( const char *command )
	{
		if ( !stricmp(command, "OK") )
		{
			// apply the data
			OnApplyData();
			Close();
		}
		else
		{
			BaseClass::OnCommand( command );
		}
	}

	void OnKeyCodeTyped(KeyCode code)
	{
		// force ourselves to be closed if the escape key it pressed
		if (code == KEY_ESCAPE)
		{
			Close();
		}
		else
		{
			BaseClass::OnKeyCodeTyped(code);
		}
	}
};

//-----------------------------------------------------------------------------
// Purpose: Open advanced keyboard options
//-----------------------------------------------------------------------------
void CTFOptionsKeyboardPanel::OpenKeyboardAdvancedDialog()
{
	if (!m_OptionsSubKeyboardAdvancedDlg.Get())
	{
		m_OptionsSubKeyboardAdvancedDlg = new CTFOptionsKeyboardPanelAdvancedDlg(GetVParent());
	}
	m_OptionsSubKeyboardAdvancedDlg->Activate();
}