//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "variant_t.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void variant_t::SetEntity( CBaseEntity *val ) 
{ 
	eVal = val;
	fieldType = FIELD_EHANDLE; 
}

const char* variant_t::GetDebug()
{
	/*
	case FIELD_BOOLEAN:		*((bool *)data) = bVal != 0;		break;
	case FIELD_CHARACTER:	*((char *)data) = iVal;				break;
	case FIELD_SHORT:		*((short *)data) = iVal;			break;
	case FIELD_INTEGER:		*((int *)data) = iVal;				break;
	case FIELD_STRING:		*((string_t *)data) = iszVal;		break;
	case FIELD_FLOAT:		*((float *)data) = flVal;			break;
	case FIELD_COLOR32:		*((color32 *)data) = rgbaVal;		break;

	case FIELD_VECTOR:
	case FIELD_POSITION_VECTOR:
	{
		((float *)data)[0] = vecVal[0];
		((float *)data)[1] = vecVal[1];
		((float *)data)[2] = vecVal[2];
		break;
	}

	case FIELD_EHANDLE:		*((EHANDLE *)data) = eVal;			break;
	case FIELD_CLASSPTR:	*((CBaseEntity **)data) = eVal;		break;
	*/

	const char* fieldtype = "unknown";
	switch (FieldType())
	{
	case FIELD_VOID:			fieldtype = "Void"; break;
	case FIELD_FLOAT:			fieldtype = "Float"; break;
	case FIELD_STRING:			fieldtype = "String"; break;
	case FIELD_INTEGER:			fieldtype = "Integer"; break;
	case FIELD_BOOLEAN:			fieldtype = "Boolean"; break;
	case FIELD_EHANDLE:			fieldtype = "Entity"; break;
	case FIELD_CLASSPTR:		fieldtype = "EntityPtr"; break;
	case FIELD_POSITION_VECTOR:
	case FIELD_VECTOR:			fieldtype = "Vector"; break;
	case FIELD_CHARACTER:		fieldtype = "Character"; break;
	case FIELD_SHORT:			fieldtype = "Short"; break;
	case FIELD_COLOR32:			fieldtype = "Color32"; break;
	default:					fieldtype = UTIL_VarArgs("unknown: %i", FieldType());
	}
	return UTIL_VarArgs("%s (%s)", String(), fieldtype);
}

void variant_t::SetScriptVariant( ScriptVariant_t &var )
{
	switch ( FieldType() )
	{
		case FIELD_VOID:		var = NULL; break;
		case FIELD_INTEGER:		var = iVal; break;
		case FIELD_FLOAT:		var = flVal; break;
		case FIELD_STRING:		var = STRING( iszVal ); break;
		case FIELD_POSITION_VECTOR:
		case FIELD_VECTOR:		var = reinterpret_cast<Vector *>( &flVal ); break; // HACKHACK
		case FIELD_BOOLEAN:		var = bVal; break;
		case FIELD_EHANDLE:		var = ToHScript( eVal ); break;
		case FIELD_CLASSPTR:	var = ToHScript( eVal ); break;
		case FIELD_SHORT:		var = (short)iVal; break;
		case FIELD_CHARACTER:	var = (char)iVal; break;
		case FIELD_COLOR32:
		{
			Color *clr = new Color( rgbaVal.r, rgbaVal.g, rgbaVal.b, rgbaVal.a );
			var = g_pScriptVM->RegisterInstance( clr );
			break;
		}
		default:				var = ToString(); break;
	}
}
