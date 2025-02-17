//====== Copyright � 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose: Shared player code.
//
//=============================================================================
#ifndef TF_PLAYER_SHARED_H
#define TF_PLAYER_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#include "networkvar.h"
#include "tf_shareddefs.h"
#include "tf_weaponbase.h"
#include "basegrenade_shared.h"

// Client specific.
#ifdef CLIENT_DLL
class C_TFPlayer;
#include "baseobject_shared.h"
// Server specific.
#else
#include "tf_obj.h"
class CTFPlayer;
#endif

//=============================================================================
//
// Tables.
//

// Client specific.
#ifdef CLIENT_DLL

	EXTERN_RECV_TABLE( DT_TFPlayerShared );

// Server specific.
#else

	EXTERN_SEND_TABLE( DT_TFPlayerShared );

#endif

struct stun_struct_t
{
	CHandle<CTFPlayer> hPlayer;
	float flDuration;
	float flExpireTime;
	float flStartFadeTime;
	float flStunAmount;
	int	iStunFlags;
};

//=============================================================================

#define PERMANENT_CONDITION		-1

// Damage storage for crit multiplier calculation
class CTFDamageEvent
{
	DECLARE_EMBEDDED_NETWORKVAR()

public:
	float flDamage;
	float flTime;
	bool bKill;
};

//=============================================================================
//
// Shared player class.
//
class CTFPlayerShared
{
public:

// Client specific.
#ifdef CLIENT_DLL

	friend class C_TFPlayer;
	typedef C_TFPlayer OuterClass;
	DECLARE_PREDICTABLE();

// Server specific.
#else

	friend class CTFPlayer;
	typedef CTFPlayer OuterClass;

#endif
	
	DECLARE_EMBEDDED_NETWORKVAR()
	DECLARE_CLASS_NOBASE( CTFPlayerShared );

	// Initialization.
	CTFPlayerShared();
	void Init( OuterClass *pOuter );

	// State (TF_STATE_*).
	int		GetState() const					{ return m_nPlayerState; }
	void	SetState( int nState )				{ m_nPlayerState = nState; }
	bool	InState( int nState )				{ return ( m_nPlayerState == nState ); }

	// Condition (TF_COND_*).
	int		GetCond() const						{ return m_nPlayerCond; }
	void	SetCond( int nCond )				{ m_nPlayerCond = nCond; }
	void	AddCond( int nCond, float flDuration = PERMANENT_CONDITION );
	void	RemoveCond( int nCond );
	bool	InCond( int nCond );
	void	RemoveAllCond( CTFPlayer *pPlayer );
	void	OnConditionAdded( int nCond );
	void	OnConditionRemoved( int nCond );
	void	ConditionThink( void );
	float	GetConditionDuration( int nCond );

	void	ConditionGameRulesThink( void );

	void	InvisibilityThink( void );

	int		GetMaxBuffedHealth( void );

#ifdef CLIENT_DLL
	// This class only receives calls for these from C_TFPlayer, not
	// natively from the networking system
	virtual void OnPreDataChanged( void );
	virtual void OnDataChanged( void );

	// check the newly networked conditions for changes
	void	UpdateConditions( void );

	void	UpdateCritBoostEffect( void );
#endif

	bool	IsCritBoosted( void );
	bool	IsInvulnerable( void );
	bool	IsStealthed( void );

	void	Disguise( int nTeam, int nClass );
	void	CompleteDisguise( void );
	void	RemoveDisguise( void );
	void	FindDisguiseTarget( void );
	int		GetDisguiseTeam( void )				{ return m_nDisguiseTeam; }
	int		GetDisguiseClass( void ) 			{ return m_nDisguiseClass; }
	int		GetDesiredDisguiseClass( void )		{ return m_nDesiredDisguiseClass; }
	int		GetDesiredDisguiseTeam( void )		{ return m_nDesiredDisguiseTeam; }
	EHANDLE GetDisguiseTarget( void ) 	
	{
#ifdef CLIENT_DLL
		if ( m_iDisguiseTargetIndex == TF_DISGUISE_TARGET_INDEX_NONE )
			return NULL;
		return cl_entitylist->GetNetworkableHandle( m_iDisguiseTargetIndex );
#else
		return m_hDisguiseTarget.Get();
#endif
	}
	int		GetDisguiseHealth( void )			{ return m_iDisguiseHealth; }
	void	SetDisguiseHealth( int iDisguiseHealth );

#ifdef CLIENT_DLL
	void	OnDisguiseChanged( void );
	void	RecalcDisguiseWeapon( void );
	int		GetDisguiseWeaponModelIndex( void ) { return m_iDisguiseWeaponModelIndex; }
	CTFWeaponInfo *GetDisguiseWeaponInfo( void );
#endif

#ifdef GAME_DLL
	void	Heal( CTFPlayer *pPlayer, float flAmount, bool bDispenserHeal = false );
	void	StopHealing( CTFPlayer *pPlayer );
	void	RecalculateInvuln( bool bInstantRemove = false, bool bCritboost = false );
	int		FindHealerIndex( CTFPlayer *pPlayer );
	EHANDLE	GetFirstHealer();

	//void	IncrementArenaNumChanges( void ) { m_nArenaNumChanges++; }
	//void	ResetArenaNumChanges( void ) { m_nArenaNumChanges = 0; }

	bool	AddToSpyCloakMeter( float val, bool bForce = false );

#endif

	bool HealerIsDispenser( int index );
	CBaseEntity* GetHealerByIndex( int index );
	int		GetNumHealers( void ) { return m_nNumHealers; }

	void				RecalculatePlayerBodygroups( bool bForce = false );
	void				RestorePlayerBodygroups( void );

	void	Burn( CTFPlayer *pPlayer, CTFWeaponBase* pWeapon = NULL );

	// Weapons.
	CTFWeaponBase *GetActiveTFWeapon() const;

	// Utility.
	bool	IsAlly( CBaseEntity *pEntity );

	// Separation force
	bool	IsSeparationEnabled( void ) const	{ return m_bEnableSeparation; }
	void	SetSeparation( bool bEnable )		{ m_bEnableSeparation = bEnable; }
	const Vector &GetSeparationVelocity( void ) const { return m_vSeparationVelocity; }
	void	SetSeparationVelocity( const Vector &vSeparationVelocity ) { m_vSeparationVelocity = vSeparationVelocity; }

	void	FadeInvis( float flInvisFadeTime );
	float	GetPercentInvisible( void );
	void	NoteLastDamageTime( int nDamage );
	void	OnSpyTouchedByEnemy( void );
	float	GetLastStealthExposedTime( void ) { return m_flLastStealthExposeTime; }

	int		GetDesiredPlayerClassIndex( void );

	float	GetSpyCloakMeter() const		{ return m_flCloakMeter; }
	void	SetSpyCloakMeter( float val ) { m_flCloakMeter = val; }

	bool	IsJumping( void ) { return m_bJumping; }
	void	SetJumping( bool bJumping );
	bool    IsAirDashing( void ) { return m_bAirDash; }
	void    SetAirDash( bool bAirDash );

	void	DebugPrintConditions( void );

	float	GetStealthNoAttackExpireTime( void );

	void	SetPlayerDominated( CTFPlayer *pPlayer, bool bDominated );
	bool	IsPlayerDominated( int iPlayerIndex );
	bool	IsPlayerDominatingMe( int iPlayerIndex );
	void	SetPlayerDominatingMe( CTFPlayer *pPlayer, bool bDominated );

	// hauling
	bool	IsCarryingObject( void )		const { return m_bCarryingObject; }
	CBaseObject* GetCarriedObject( void )	const { return m_hCarriedObject.Get(); }
	void	SetCarriedObject( CBaseObject* pObj );

	// Stuns
	stun_struct_t* GetActiveStunInfo( void ) const;
#ifdef GAME_DLL
	void				StunPlayer( float flTime, float flReductionAmount, int iStunFlags = TF_STUN_MOVEMENT, CTFPlayer* pAttacker = NULL );
#endif // GAME_DLL
	float				GetAmountStunned( int iStunFlags );
	int					GetStunFlags( void ) const { return GetActiveStunInfo() ? GetActiveStunInfo()->iStunFlags : 0; }
	float				GetStunExpireTime( void ) const { return GetActiveStunInfo() ? GetActiveStunInfo()->flExpireTime : 0; }
	void				UpdateClientsideStunSystem( void );

	// Movement stun state.
	bool				m_bStunNeedsFadeOut;
	float				m_flStunLerpTarget;
	float				m_flLastMovementStunChange;
#ifdef GAME_DLL
	CUtlVector <stun_struct_t> m_PlayerStuns;
#endif
	stun_struct_t		m_ActiveStunInfo;

	float				GetTauntRemoveTime( void ) const { return m_flTauntRemoveTime; }
private:

	void ImpactWaterTrace( trace_t &trace, const Vector &vecStart );

	void OnAddStealthed( void );
	void OnAddInvulnerable( void );
	void OnAddTeleported( void );
	void OnAddBurning( void );
	void OnAddDisguising( void );
	void OnAddDisguised( void );
	void OnAddCritBoost( void );
	void OnAddOverhealed( void );

	void OnRemoveZoomed( void );
	void OnRemoveBurning( void );
	void OnRemoveStealthed( void );
	void OnRemoveDisguised( void );
	void OnRemoveDisguising( void );
	void OnRemoveCritBoost( void );
	void OnRemoveOverhealed( void );
	void OnRemoveInvulnerable( void );
	void OnRemoveTeleported( void );
	void OnRemoveStunned( void );

	float GetCritMult( void );

#ifdef GAME_DLL
	void  UpdateCritMult( void );
	void  RecordDamageEvent( const CTakeDamageInfo &info, bool bKill );
	void  ClearDamageEvents( void ) { m_DamageEvents.Purge(); }
	int	  GetNumKillsInTime( float flTime );

	// Invulnerable.
	bool  IsProvidingInvuln( CTFPlayer *pPlayer, bool &bShouldbeCritboost );
	void  SetInvulnerable( bool bState, bool bInstant = false, bool bCritboost = false );
#endif

private:

	// Vars that are networked.
	CNetworkVar( int, m_nPlayerState );			// Player state.
	CNetworkVar( int, m_nPlayerCond );			// Player condition flags.
	float m_flCondExpireTimeLeft[TF_COND_LAST];		// Time until each condition expires

//TFTODO: What if the player we're disguised as leaves the server?
//...maybe store the name instead of the index?
	CNetworkVar( int, m_nDisguiseTeam );		// Team spy is disguised as.
	CNetworkVar( int, m_nDisguiseClass );		// Class spy is disguised as.
	EHANDLE m_hDisguiseTarget;					// Playing the spy is using for name disguise.
	CNetworkVar( int, m_iDisguiseTargetIndex );
	CNetworkVar( int, m_iDisguiseHealth );		// Health to show our enemies in player id
	CNetworkVar( int, m_nDesiredDisguiseClass );
	CNetworkVar( int, m_nDesiredDisguiseTeam );

	bool m_bEnableSeparation;		// Keeps separation forces on when player stops moving, but still penetrating
	Vector m_vSeparationVelocity;	// Velocity used to keep player seperate from teammates

	float m_flInvisibility;
	CNetworkVar( float, m_flInvisChangeCompleteTime );		// when uncloaking, must be done by this time
	float m_flLastStealthExposeTime;

	CNetworkVar( int, m_nNumHealers );

	// Vars that are not networked.
	OuterClass			*m_pOuter;					// C_TFPlayer or CTFPlayer (client/server).

#ifdef GAME_DLL
	// Healer handling
	struct healers_t
	{
		EHANDLE	pPlayer;
		float	flAmount;
		bool	bDispenserHeal;
	};
	CUtlVector< healers_t >	m_aHealers;	
	float					m_flHealFraction;	// Store fractional health amounts
	float					m_flDisguiseHealFraction;	// Same for disguised healing

	float m_flInvulnerableOffTime;
#endif

	// Burn handling
	CHandle<CTFPlayer>		m_hBurnAttacker;
	// conn: This is used because most OnTakeDamage checks just use the currently held weapon if CTakeDamageInfo has no weapon set
	// which causes problems when holding the Axtinguisher after setting someone on fire.
	// Instead, we set the active weapon at time of burn start, and give it to CTakeDamageInfo.
	CHandle<CTFWeaponBase>	m_hBurnWeapon;
	CNetworkVar( int,		m_nNumFlames );
	float					m_flFlameBurnTime;
	float					m_flFlameRemoveTime;
	CNetworkVar( float,		m_flTauntRemoveTime );


	float m_flDisguiseCompleteTime;

	int	m_nOldConditions;
	int	m_nOldDisguiseClass;

	CNetworkVar( int, m_iDesiredPlayerClass );

	float m_flNextBurningSound;

	CNetworkVar( float, m_flCloakMeter );	// [0,100]

	CNetworkVar( bool, m_bJumping );
	CNetworkVar( bool, m_bAirDash );

	CNetworkVar( float, m_flStealthNoAttackExpire );
	CNetworkVar( float, m_flStealthNextChangeTime );

	CNetworkVar( int, m_iCritMult );

	CNetworkArray( bool, m_bPlayerDominated, MAX_PLAYERS+1 );		// array of state per other player whether player is dominating other players
	CNetworkArray( bool, m_bPlayerDominatingMe, MAX_PLAYERS+1 );	// array of state per other player whether other players are dominating this player
	
	// networking for stuns (all of this gets put into m_ActiveStunInfo by UpdateClientsideStunSystem(), don't use directly)
	CNetworkVar( float, m_flMovementStunTime );
	CNetworkVar( float, m_flStunEnd );
	CNetworkVar( int, m_iMovementStunAmount );
	CNetworkVar( unsigned char, m_iMovementStunParity );
	CNetworkHandle( CTFPlayer, m_hStunner );
	CNetworkVar( int, m_iStunFlags );
	CNetworkVar( int, m_iStunIndex );

	// hauling
	CNetworkHandle( CBaseObject, m_hCarriedObject );
	CNetworkVar( bool, m_bCarryingObject );

	CNetworkVar( int, m_nRestoreBody );
	//CNetworkVar( int, m_nRestoreDisguiseBody );
#ifdef GAME_DLL
	float	m_flNextCritUpdate;
	CUtlVector<CTFDamageEvent> m_DamageEvents;
#else
	int m_iDisguiseWeaponModelIndex;
	int m_iOldDisguiseWeaponModelIndex;
	CTFWeaponInfo *m_pDisguiseWeaponInfo;

	WEAPON_FILE_INFO_HANDLE	m_hDisguiseWeaponInfo;
#endif
};			   

#define TF_DEATH_DOMINATION				0x0001	// killer is dominating victim
#define TF_DEATH_ASSISTER_DOMINATION	0x0002	// assister is dominating victim
#define TF_DEATH_REVENGE				0x0004	// killer got revenge on victim
#define TF_DEATH_ASSISTER_REVENGE		0x0008	// assister got revenge on victim

extern const char *g_pszBDayGibs[22];

class CTargetOnlyFilter : public CTraceFilterSimple
{
public:
	CTargetOnlyFilter( CBaseEntity* pShooter, CBaseEntity* pTarget );
	virtual bool ShouldHitEntity( IHandleEntity* pHandleEntity, int contentsMask );
	CBaseEntity* m_pShooter;
	CBaseEntity* m_pTarget;
};

#endif // TF_PLAYER_SHARED_H
