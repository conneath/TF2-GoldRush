//========= Copyright Valve Corporation, All rights reserved. ============//
//
// TF Arrow Projectile
//
//=============================================================================
#ifndef TF_PROJECTILE_ARROW_H
#define TF_PROJECTILE_ARROW_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_player.h"
#include "tf_weaponbase_rocket.h"
#include "iscorer.h"

class CTFProjectile_Arrow : public CTFBaseRocket, public IScorer
{
public:

	DECLARE_CLASS( CTFProjectile_Arrow, CTFBaseRocket );
	DECLARE_NETWORKCLASS();
	DECLARE_DATADESC();

	CTFProjectile_Arrow();
	~CTFProjectile_Arrow();

	// Creation.
	static CTFProjectile_Arrow *Create( const Vector &vecOrigin, const QAngle &vecAngles, const float fSpeed, const float fGravity, ProjectileType_t projectileType, CBaseEntity *pOwner = NULL, CBaseEntity *pScorer = NULL );	
	virtual void	InitArrow( const QAngle &vecAngles, const float fSpeed, const float fGravity, ProjectileType_t projectileType, CBaseEntity *pOwner = NULL, CBaseEntity *pScorer = NULL );	
	virtual void	Spawn();
	virtual void	Precache();
	virtual int		GetWeaponID( void ) const			{ return m_iWeaponId; }
	virtual int		GetProjectileType ( void ) const OVERRIDE;

	virtual bool	StrikeTarget( mstudiobbox_t *pBox, CBaseEntity *pOther );
	virtual void	OnArrowImpact( mstudiobbox_t *pBox, CBaseEntity *pOther, CBaseEntity *pAttacker );
	virtual bool	OnArrowImpactObject( CBaseEntity *pOther );
	bool			PositionArrowOnBone( mstudiobbox_t *pBox, CBaseAnimating *pOtherAnim );
	void			GetBoneAttachmentInfo( mstudiobbox_t *pBox, CBaseAnimating *pOtherAnim, Vector &bonePosition, QAngle &boneAngles, int &boneIndexAttached, int &physicsBoneIndex );

	void			ImpactThink( void );
	void			BuildingHealingArrow( CBaseEntity *pOther );

	int				GetArrowSkin() const;

	// IScorer interface
	virtual CBasePlayer *GetScorer( void );
	virtual CBasePlayer *GetAssistant( void ) { return NULL; }

	void			SetScorer( CBaseEntity *pScorer );
	void			SetCritical( bool bCritical ) { m_bCritical = bCritical; }

	virtual float	GetDamage();
	virtual bool	CanHeadshot();

	virtual void	OnArrowMissAllPlayers( void );
	virtual void	ArrowTouch( CBaseEntity *pOther );
	virtual void	CheckSkyboxImpact( CBaseEntity *pOther );
	bool			CheckRagdollPinned( const Vector &start, const Vector &vel, int boneIndexAttached, int physicsBoneIndex, CBaseEntity *pOther, int iHitGroup, int iVictim );
	virtual void	AdjustDamageDirection( const CTakeDamageInfo &info, Vector &dir, CBaseEntity *pEnt );
	void			ImpactSound( const char *pszSoundName, bool bLoudForAttacker = false );
	virtual void	BreakArrow();
	virtual void	ImpactTeamPlayer( CTFPlayer *pOther ) {}

	void			FadeOut( int iTime );
	void			RemoveThink();

	virtual const char * GetTrailParticleName( void );
	void			CreateTrail( void );
	void			RemoveTrail( void );

	virtual void	IncrementDeflected( void );
	
	virtual void	Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir );
	
	virtual bool	ShouldNotDetonate( void ) { return true; }
	bool			IsAlight() { return m_bArrowAlight; }
	void			SetArrowAlight( bool bAlight ) { m_bArrowAlight = bAlight; }
	virtual bool	IsDeflectable() OVERRIDE { return ( GetMoveType() != MOVETYPE_NONE ); }
	void			SetPenetrate( bool bPenetrate = false ) { m_bPenetrate = bPenetrate; SetSolidFlags( FSOLID_NOT_SOLID | FSOLID_TRIGGER ); }
	bool			CanPenetrate() const { return m_bPenetrate; }
	virtual bool	IsDestroyable( bool bOrbAttack = false ) OVERRIDE { return ( !bOrbAttack ? false : true ); }
	virtual bool	IsBreakable( void ) const { return true; }
	
	void SetApplyMilkOnHit() { m_bApplyMilkOnHit = true; }

private:

	CBaseHandle		m_Scorer;

	float			m_flImpactTime;
	Vector			m_vecImpactNormal;

	float			m_flTrailLife;
	EHANDLE			m_pTrail;

	bool			m_bStruckEnemy;

	CNetworkVar( bool, m_bArrowAlight );
	CNetworkVar( bool, m_bCritical );

	bool				m_bPenetrate;

	CNetworkVar( int, m_iProjectileType );
	int				m_iWeaponId;

	bool			m_bFiredWhileZoomed;

protected:
	CUtlVector< int >	m_HitEntities;
	float			m_flInitTime;

	bool			m_bApplyMilkOnHit;		// For Apothacary's Arrow which can sometimes be special
};

#endif	//TF_PROJECTILE_ARROW_H
