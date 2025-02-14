//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "tf_ammo_pack.h"
#include "tf_shareddefs.h"
#include "ammodef.h"
#include "tf_gamerules.h"
#include "explode.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//----------------------------------------------

// Network table.
IMPLEMENT_SERVERCLASS_ST( CTFAmmoPack, DT_AmmoPack )
	SendPropVector( SENDINFO( m_vecInitialVelocity ), -1, SPROP_NOSCALE ),
END_SEND_TABLE()

BEGIN_DATADESC( CTFAmmoPack )
	DEFINE_THINKFUNC( FlyThink ),
	DEFINE_ENTITYFUNC( PackTouch ),
END_DATADESC();

LINK_ENTITY_TO_CLASS( tf_ammo_pack, CTFAmmoPack );

PRECACHE_REGISTER( tf_ammo_pack );

void CTFAmmoPack::Spawn( void )
{
	Precache();
	SetModel( STRING( GetModelName() ) );
	BaseClass::Spawn();

	SetNextThink( gpGlobals->curtime + 0.75f );
	SetThink( &CTFAmmoPack::FlyThink );

	SetTouch( &CTFAmmoPack::PackTouch );

	m_flCreationTime = gpGlobals->curtime;

	// no pickup until flythink
	m_bAllowOwnerPickup = false;

	// no ammo to start
	memset( m_iAmmo, 0, sizeof(m_iAmmo) );

	// Die in 30 seconds
	SetContextThink( &CBaseEntity::SUB_Remove, gpGlobals->curtime + 30, "DieContext" );

	if ( IsX360() )
	{
		RemoveEffects( EF_ITEM_BLINK );
	}
}

void CTFAmmoPack::Precache( void )
{
}

CTFAmmoPack *CTFAmmoPack::Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, const char *pszModelName )
{
	CTFAmmoPack *pAmmoPack = static_cast<CTFAmmoPack*>( CBaseAnimating::CreateNoSpawn( "tf_ammo_pack", vecOrigin, vecAngles, pOwner ) );
	if ( pAmmoPack )
	{
		pAmmoPack->SetModelName( AllocPooledString( pszModelName ) );
		DispatchSpawn( pAmmoPack );
	}

	return pAmmoPack;
}

void CTFAmmoPack::SetInitialVelocity( Vector &vecVelocity )
{ 
	m_vecInitialVelocity = vecVelocity;
}

int CTFAmmoPack::GiveAmmo( int iCount, int iAmmoType )
{
	if (iAmmoType == -1 || iAmmoType >= TF_AMMO_COUNT )
	{
		Msg("ERROR: Attempting to give unknown ammo type (%d)\n", iAmmoType);
		return 0;
	}

	m_iAmmo[iAmmoType] += iCount;

	return iCount;
}

void CTFAmmoPack::FlyThink( void )
{
	m_bAllowOwnerPickup = true;
}

void CTFAmmoPack::PackTouch( CBaseEntity *pOther )
{
	Assert( pOther );

	if( !pOther->IsPlayer() )
		return;

	if( !pOther->IsAlive() )
		return;

	//Don't let the person who threw this ammo pick it up until it hits the ground.
	//This way we can throw ammo to people, but not touch it as soon as we throw it ourselves
	if( GetOwnerEntity() == pOther && m_bAllowOwnerPickup == false )
		return;

	CTFPlayer *pPlayer = ToTFPlayer( pOther );

	Assert( pPlayer );

	// The sandwich gives health instead of ammo
	if ( m_bHealthInstead )
	{
		// Let the sandwich fall to the ground for a bit so that people see it
		if ( !m_bAllowOwnerPickup )
			return;

		// Scouts get a little more, as a reference to the scout movie
		int iAmount = (pPlayer->IsPlayerClass( TF_CLASS_SCOUT )) ? 75 : 50;
		pPlayer->TakeHealth( iAmount, DMG_GENERIC );
		IGameEvent* event = gameeventmanager->CreateEvent( "player_healed" );
		if ( event )
		{
			event->SetInt( "priority", 1 );	// HLTV event priority
			event->SetInt( "patient", pPlayer->GetUserID() );
			event->SetInt( "healer", pPlayer->GetUserID() );
			event->SetInt( "amount", iAmount );
			//event->SetInt( "weapon_def_index", INVALID_ITEM_DEF_INDEX );
			gameeventmanager->FireEvent( event );
		}
		/* this is for scout achievement
		event = gameeventmanager->CreateEvent( "player_stealsandvich" );
		if ( event )
		{
			if ( ToTFPlayer( GetOwnerEntity() ) )
			{
				event->SetInt( "owner", ToTFPlayer( GetOwnerEntity() )->GetUserID() );
			}
			event->SetInt( "target", pPlayer->GetUserID() );
			gameeventmanager->FireEvent( event );
		}
		*/
		UTIL_Remove( this );
		return;
	}

	int iAmmoTaken = 0;

	int i;
	for ( i=0;i<TF_AMMO_COUNT;i++ )
	{
		iAmmoTaken += pPlayer->GiveAmmo( m_iAmmo[i], i );
	}

	// give them a chunk of cloak power
	if ( pPlayer->m_Shared.AddToSpyCloakMeter( 100.0f * 0.5 ) )
	{
		iAmmoTaken++;
	}

	if ( iAmmoTaken > 0 )
	{
		UTIL_Remove( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
unsigned int CTFAmmoPack::PhysicsSolidMaskForEntity( void ) const
{ 
	return BaseClass::PhysicsSolidMaskForEntity() | CONTENTS_DEBRIS;
}