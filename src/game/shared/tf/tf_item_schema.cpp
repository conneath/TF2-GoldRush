//================================================================
// Goldrush Item Schema - powered by conneath industries ltd co tm
// credit to the tf2c team for making the original econ system
// that every tf2 mod seems to use these days (including us)
//================================================================
#include "cbase.h"
#include "tf_item_schema.h"
#include "tf_item_system.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=============================================================================
// CEconItemAttribute
//=============================================================================

BEGIN_NETWORK_TABLE_NOBASE( CEconItemAttribute, DT_EconItemAttribute )
#ifdef CLIENT_DLL
RecvPropInt( RECVINFO( m_iAttributeDefinitionIndex ) ),
RecvPropFloat( RECVINFO( value ) ),
RecvPropString( RECVINFO( value_string ) ),
RecvPropString( RECVINFO( attribute_class ) ),
#else
SendPropInt( SENDINFO( m_iAttributeDefinitionIndex ) ),
SendPropFloat( SENDINFO( value ) ),
SendPropString( SENDINFO( value_string ) ),
SendPropString( SENDINFO( attribute_class ) ),
#endif
END_NETWORK_TABLE()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconItemAttribute::Init( int iIndex, float flValue, const char* pszAttributeClass /*= NULL*/ )
{
	m_iAttributeDefinitionIndex = iIndex;
	value = flValue;
	value_string.GetForModify()[0] = '\0';

	if ( pszAttributeClass )
	{
		V_strncpy( attribute_class.GetForModify(), pszAttributeClass, sizeof( attribute_class ) );
	}
	else
	{
		EconAttributeDefinition* pAttribDef = GetStaticData();
		if ( pAttribDef )
		{
			V_strncpy( attribute_class.GetForModify(), pAttribDef->attribute_class, sizeof( attribute_class ) );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconItemAttribute::Init( int iIndex, const char* pszValue, const char* pszAttributeClass /*= NULL*/ )
{
	m_iAttributeDefinitionIndex = iIndex;
	value = 0.0f;
	V_strncpy( value_string.GetForModify(), pszValue, sizeof( value_string ) );

	if ( pszAttributeClass )
	{
		V_strncpy( attribute_class.GetForModify(), pszAttributeClass, sizeof( attribute_class ) );
	}
	else
	{
		EconAttributeDefinition* pAttribDef = GetStaticData();
		if ( pAttribDef )
		{
			V_strncpy( attribute_class.GetForModify(), pAttribDef->attribute_class, sizeof( attribute_class ) );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
EconAttributeDefinition* CEconItemAttribute::GetStaticData( void )
{
	return GetItemSchema()->GetAttributeDefinition( m_iAttributeDefinitionIndex );
}


//=============================================================================
// CEconItemDefinition
//=============================================================================

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
EconItemVisuals* CEconItemDefinition::GetVisuals( int iTeamNum /*= TEAM_UNASSIGNED*/ )
{
	if ( iTeamNum > LAST_SHARED_TEAM && iTeamNum < TF_TEAM_COUNT )
	{
		return &visual[iTeamNum];
	}

	return &visual[TEAM_UNASSIGNED];
}

int CEconItemDefinition::GetNumAttachedModels( int iTeam )
{
	EconItemVisuals* pVisuals = GetVisuals( iTeam );
	Assert( pVisuals );
	if ( pVisuals )
		return pVisuals->m_AttachedModels.Count();

	return 0;
}


attachedmodel_t* CEconItemDefinition::GetAttachedModelData( int iTeam, int iIdx )
{
	EconItemVisuals* pVisuals = GetVisuals( iTeam );
	Assert( pVisuals );
	if ( iTeam < FIRST_GAME_TEAM || iTeam >= TF_TEAM_COUNT || !pVisuals )
		return NULL;

	Assert( iIdx < pVisuals->m_AttachedModels.Count() );
	if ( iIdx >= pVisuals->m_AttachedModels.Count() )
		return NULL;

	return &pVisuals->m_AttachedModels[iIdx];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CEconItemDefinition::GetLoadoutSlot( int iClass /*= TF_CLASS_UNDEFINED*/ )
{
	if ( iClass && item_slot_per_class[iClass] != -1 )
	{
		return item_slot_per_class[iClass];
	}

	return item_slot;
}

//-----------------------------------------------------------------------------
// Purpose: Find an attribute with the specified class.
//-----------------------------------------------------------------------------
CEconItemAttribute* CEconItemDefinition::IterateAttributes( string_t strClass )
{
	// Returning the first attribute found.
	for ( int i = 0; i < attributes.Count(); i++ )
	{
		CEconItemAttribute* pAttribute = &attributes[i];

		if ( pAttribute->m_strAttributeClass == strClass )
		{
			return pAttribute;
		}
	}

	return NULL;
}
