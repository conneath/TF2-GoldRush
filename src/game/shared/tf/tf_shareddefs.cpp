//====== Copyright � 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_shareddefs.h"
#include "KeyValues.h"
#include "takedamageinfo.h"
#include "tf_gamerules.h"

//-----------------------------------------------------------------------------
// Teams.
//-----------------------------------------------------------------------------
const char *g_aTeamNames[TF_TEAM_COUNT] =
{
	"Unassigned",
	"Spectator",
	"Red",
	"Blue"
};

color32 g_aTeamColors[TF_TEAM_COUNT] = 
{
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 },
	{ 255, 0, 0, 0 },
	{ 0, 0, 255, 0 }
};

//-----------------------------------------------------------------------------
// Classes.
//-----------------------------------------------------------------------------

const char *g_aPlayerClassNames[] =
{
	"#TF_Class_Name_Undefined",
	"#TF_Class_Name_Scout",
	"#TF_Class_Name_Sniper",
	"#TF_Class_Name_Soldier",
	"#TF_Class_Name_Demoman",
	"#TF_Class_Name_Medic",
	"#TF_Class_Name_HWGuy",
	"#TF_Class_Name_Pyro",
	"#TF_Class_Name_Spy",
	"#TF_Class_Name_Engineer"
};

const char *g_aPlayerClassNames_NonLocalized[] =
{
	"Undefined",
	"Scout",
	"Sniper",
	"Soldier",
	"Demoman",
	"Medic",
	"Heavy",
	"Pyro",
	"Spy",
	"Engineer"
};

int GetClassIndexFromString( const char* pClassName, int nLastClassIndex/*=TF_LAST_NORMAL_CLASS*/ )
{
	for ( int i = TF_FIRST_NORMAL_CLASS; i <= nLastClassIndex; ++i )
	{
		// compare first N characters to allow matching both "heavy" and "heavyweapons"
		int classnameLength = V_strlen( g_aPlayerClassNames_NonLocalized[i] );

		if ( V_strlen( pClassName ) < classnameLength )
			continue;

		if ( !V_strnicmp( g_aPlayerClassNames_NonLocalized[i], pClassName, classnameLength ) )
		{
			return i;
		}
	}

	return TF_CLASS_UNDEFINED;
}

int iRemapIndexToClass[TF_CLASS_MENU_BUTTONS] =
{
	0,
		TF_CLASS_SCOUT,
		TF_CLASS_SOLDIER,
		TF_CLASS_PYRO,
		TF_CLASS_DEMOMAN,
		TF_CLASS_HEAVYWEAPONS,
		TF_CLASS_ENGINEER,
		TF_CLASS_MEDIC,
		TF_CLASS_SNIPER,
		TF_CLASS_SPY,
		0,
		0,
		TF_CLASS_RANDOM
};

int GetRemappedMenuIndexForClass( int iClass )
{
	int iIndex = 0;

	for ( int i = 0; i < TF_CLASS_MENU_BUTTONS; i++ )
	{
		if ( iRemapIndexToClass[i] == iClass )
		{
			iIndex = i;
			break;
		}
	}

	return iIndex;
}

int condition_to_attribute_translation[] =
{
	TF_COND_BURNING,					// 1 (1<<0)
	TF_COND_AIMING,						// 2 (1<<1)
	TF_COND_ZOOMED,						// 4 (1<<2)
	TF_COND_DISGUISING,					// 8 (...)
	TF_COND_DISGUISED,					// 16
	TF_COND_STEALTHED,					// 32
	TF_COND_INVULNERABLE,				// 64
	TF_COND_TELEPORTED,					// 128
	TF_COND_TAUNTING,					// 256
	TF_COND_INVULNERABLE_WEARINGOFF,	// 512
	TF_COND_STEALTHED_BLINK,			// 1024
	TF_COND_SELECTED_TO_TELEPORT,		// 2048
	TF_COND_CRITBOOSTED,				// 4096
	//TF_COND_TMPDAMAGEBONUS,				// 8192
	//TF_COND_FEIGN_DEATH,				// 16384
	//TF_COND_PHASE,						// 32768
	TF_COND_STUNNED,					// 65536
	TF_COND_HEALTH_BUFF,				// 131072
	TF_COND_HEALTH_OVERHEALED,			// 262144
	//TF_COND_URINE,						// 524288
	//TF_COND_ENERGY_BUFF,				// 1048576

	TF_COND_LAST				// sentinel value checked against when iterating
};

//-----------------------------------------------------------------------------
// Gametypes.
//-----------------------------------------------------------------------------
const char *g_aGameTypeNames[] =
{
	"Undefined",
	"#Gametype_CTF",
	"#Gametype_CP",
	"#Gametype_Escort"
};

//-----------------------------------------------------------------------------
// Weapon Types
//-----------------------------------------------------------------------------
const char* g_AnimSlots[] =
{
	"primary",
	"secondary",
	"melee",
	"grenade",
	"building",
	"pda",
	"item1",
	"item2",
	"MELEE_ALLCLASS",
	"secondary2",
	"primary2"
};

const char* g_LoadoutSlots[] =
{
	"primary",
	"secondary",
	"melee",
	"pda",
	"pda2",
	"building"
};

//-----------------------------------------------------------------------------
// Ammo.
//-----------------------------------------------------------------------------
const char *g_aAmmoNames[] =
{
	"DUMMY AMMO",
	"TF_AMMO_PRIMARY",
	"TF_AMMO_SECONDARY",
	"TF_AMMO_METAL",
	"TF_AMMO_GRENADES1",
	"TF_AMMO_GRENADES2"
};

struct pszWpnEntTranslationListEntry
{
	const char* weapon_name;
	const char* padding;
	const char* weapon_scout;
	const char* weapon_sniper;
	const char* weapon_soldier;
	const char* weapon_demoman;
	const char* weapon_medic;
	const char* weapon_heavyweapons;
	const char* weapon_pyro;
	const char* weapon_spy;
	const char* weapon_engineer;
};
static pszWpnEntTranslationListEntry pszWpnEntTranslationList[] =
{
	"tf_weapon_shotgun",			// Base weapon to translate
	NULL,
	"tf_weapon_shotgun_primary",	// Scout
	"tf_weapon_shotgun_primary",	// Sniper
	"tf_weapon_shotgun_soldier",	// Soldier
	"tf_weapon_shotgun_primary",	// Demoman
	"tf_weapon_shotgun_primary",	// Medic
	"tf_weapon_shotgun_hwg",		// Heavy
	"tf_weapon_shotgun_pyro",		// Pyro
	"tf_weapon_shotgun_primary",	// Spy
	"tf_weapon_shotgun_primary",	// Engineer

	"tf_weapon_pistol",				// Base weapon to translate
	NULL,
	"tf_weapon_pistol_scout",		// Scout
	"tf_weapon_pistol",				// Sniper
	"tf_weapon_pistol",				// Soldier
	"tf_weapon_pistol",				// Demoman
	"tf_weapon_pistol",				// Medic
	"tf_weapon_pistol",				// Heavy
	"tf_weapon_pistol",				// Pyro
	"tf_weapon_pistol",				// Spy
	"tf_weapon_pistol",				// Engineer
};

//-----------------------------------------------------------------------------
// Weapons.
//-----------------------------------------------------------------------------
const char *g_aWeaponNames[] =
{
	"TF_WEAPON_NONE",
	"TF_WEAPON_BAT",
	"TF_WEAPON_BOTTLE", 
	"TF_WEAPON_FIREAXE",
	"TF_WEAPON_CLUB",
	"TF_WEAPON_CROWBAR",
	"TF_WEAPON_KNIFE",
	"TF_WEAPON_FISTS",
	"TF_WEAPON_SHOVEL",
	"TF_WEAPON_WRENCH",
	"TF_WEAPON_BONESAW",
	"TF_WEAPON_SHOTGUN_PRIMARY",
	"TF_WEAPON_SHOTGUN_SOLDIER",
	"TF_WEAPON_SHOTGUN_HWG",
	"TF_WEAPON_SHOTGUN_PYRO",
	"TF_WEAPON_SCATTERGUN",
	"TF_WEAPON_SNIPERRIFLE",
	"TF_WEAPON_MINIGUN",
	"TF_WEAPON_SMG",
	"TF_WEAPON_SYRINGEGUN_MEDIC",
	"TF_WEAPON_TRANQ",
	"TF_WEAPON_ROCKETLAUNCHER",
	"TF_WEAPON_GRENADELAUNCHER",
	"TF_WEAPON_PIPEBOMBLAUNCHER",
	"TF_WEAPON_FLAMETHROWER",
	"TF_WEAPON_GRENADE_NORMAL",
	"TF_WEAPON_GRENADE_CONCUSSION",
	"TF_WEAPON_GRENADE_NAIL",
	"TF_WEAPON_GRENADE_MIRV",
	"TF_WEAPON_GRENADE_MIRV_DEMOMAN",
	"TF_WEAPON_GRENADE_NAPALM",
	"TF_WEAPON_GRENADE_GAS",
	"TF_WEAPON_GRENADE_EMP",
	"TF_WEAPON_GRENADE_CALTROP",
	"TF_WEAPON_GRENADE_PIPEBOMB",
	"TF_WEAPON_GRENADE_SMOKE_BOMB",
	"TF_WEAPON_GRENADE_HEAL",
	"TF_WEAPON_PISTOL",
	"TF_WEAPON_PISTOL_SCOUT",
	"TF_WEAPON_REVOLVER",
	"TF_WEAPON_NAILGUN",
	"TF_WEAPON_PDA",
	"TF_WEAPON_PDA_ENGINEER_BUILD",
	"TF_WEAPON_PDA_ENGINEER_DESTROY",
	"TF_WEAPON_PDA_SPY",
	"TF_WEAPON_BUILDER",
	"TF_WEAPON_MEDIGUN",
	"TF_WEAPON_GRENADE_MIRVBOMB",
	"TF_WEAPON_FLAMETHROWER_ROCKET",
	"TF_WEAPON_GRENADE_DEMOMAN",
	"TF_WEAPON_SENTRY_BULLET",
	"TF_WEAPON_SENTRY_ROCKET",
	"TF_WEAPON_DISPENSER",
	"TF_WEAPON_INVIS",
	"TF_WEAPON_FLAREGUN",
	"TF_WEAPON_LUNCHBOX",

	"TF_WEAPON_COUNT",	// end marker, do not add below here
};

int g_aWeaponDamageTypes[] =
{
	DMG_GENERIC,	// TF_WEAPON_NONE
	DMG_CLUB,		// TF_WEAPON_BAT,
	DMG_CLUB,		// TF_WEAPON_BOTTLE, 
	DMG_CLUB,		// TF_WEAPON_FIREAXE,
	DMG_CLUB,		// TF_WEAPON_CLUB,
	DMG_CLUB,		// TF_WEAPON_CROWBAR,
	DMG_SLASH,		// TF_WEAPON_KNIFE,
	DMG_CLUB,		// TF_WEAPON_FISTS,
	DMG_CLUB,		// TF_WEAPON_SHOVEL,
	DMG_CLUB,		// TF_WEAPON_WRENCH,
	DMG_SLASH,		// TF_WEAPON_BONESAW,
	DMG_BUCKSHOT | DMG_USEDISTANCEMOD,	// TF_WEAPON_SHOTGUN_PRIMARY,
	DMG_BUCKSHOT | DMG_USEDISTANCEMOD,	// TF_WEAPON_SHOTGUN_SOLDIER,
	DMG_BUCKSHOT | DMG_USEDISTANCEMOD,	// TF_WEAPON_SHOTGUN_HWG,
	DMG_BUCKSHOT | DMG_USEDISTANCEMOD,	// TF_WEAPON_SHOTGUN_PYRO,
	DMG_BUCKSHOT | DMG_USEDISTANCEMOD,  // TF_WEAPON_SCATTERGUN,
	DMG_BULLET | DMG_USE_HITLOCATIONS,	// TF_WEAPON_SNIPERRIFLE,
	DMG_BULLET | DMG_USEDISTANCEMOD,		// TF_WEAPON_MINIGUN,
	DMG_BULLET | DMG_USEDISTANCEMOD,		// TF_WEAPON_SMG,
	DMG_BULLET | DMG_USEDISTANCEMOD | DMG_NOCLOSEDISTANCEMOD | DMG_PREVENT_PHYSICS_FORCE,		// TF_WEAPON_SYRINGEGUN_MEDIC,
	DMG_BULLET | DMG_USEDISTANCEMOD | DMG_PREVENT_PHYSICS_FORCE | DMG_PARALYZE,		// TF_WEAPON_TRANQ,
	DMG_BLAST | DMG_HALF_FALLOFF | DMG_USEDISTANCEMOD,		// TF_WEAPON_ROCKETLAUNCHER,
	DMG_BLAST | DMG_HALF_FALLOFF | DMG_USEDISTANCEMOD,		// TF_WEAPON_GRENADELAUNCHER,
	DMG_BLAST | DMG_HALF_FALLOFF,		// TF_WEAPON_PIPEBOMBLAUNCHER,
	DMG_IGNITE | DMG_PREVENT_PHYSICS_FORCE | DMG_PREVENT_PHYSICS_FORCE,		// TF_WEAPON_FLAMETHROWER,
	DMG_BLAST | DMG_HALF_FALLOFF,		// TF_WEAPON_GRENADE_NORMAL,
	DMG_SONIC | DMG_HALF_FALLOFF,		// TF_WEAPON_GRENADE_CONCUSSION,
	DMG_BULLET | DMG_HALF_FALLOFF,		// TF_WEAPON_GRENADE_NAIL,
	DMG_BLAST | DMG_HALF_FALLOFF,		// TF_WEAPON_GRENADE_MIRV,
	DMG_BLAST | DMG_HALF_FALLOFF,		// TF_WEAPON_GRENADE_MIRV_DEMOMAN,
	DMG_BURN | DMG_RADIUS_MAX,		// TF_WEAPON_GRENADE_NAPALM,
	DMG_POISON | DMG_HALF_FALLOFF,		// TF_WEAPON_GRENADE_GAS,
	DMG_BLAST | DMG_HALF_FALLOFF | DMG_PREVENT_PHYSICS_FORCE,		// TF_WEAPON_GRENADE_EMP,
	DMG_GENERIC,	// TF_WEAPON_GRENADE_CALTROP,
	DMG_BLAST | DMG_HALF_FALLOFF,		// TF_WEAPON_GRENADE_PIPEBOMB,
	DMG_GENERIC,	// TF_WEAPON_GRENADE_SMOKE_BOMB,
	DMG_GENERIC,	// TF_WEAPON_GRENADE_HEAL
	DMG_BULLET | DMG_USEDISTANCEMOD,		// TF_WEAPON_PISTOL,
	DMG_BULLET | DMG_USEDISTANCEMOD,		// TF_WEAPON_PISTOL_SCOUT,
	DMG_BULLET | DMG_USEDISTANCEMOD,		// TF_WEAPON_REVOLVER,
	DMG_BULLET | DMG_USEDISTANCEMOD | DMG_NOCLOSEDISTANCEMOD | DMG_PREVENT_PHYSICS_FORCE,		// TF_WEAPON_NAILGUN,
	DMG_BULLET,		// TF_WEAPON_PDA,
	DMG_BULLET,		// TF_WEAPON_PDA_ENGINEER_BUILD,
	DMG_BULLET,		// TF_WEAPON_PDA_ENGINEER_DESTROY,
	DMG_BULLET,		// TF_WEAPON_PDA_SPY,
	DMG_BULLET,		// TF_WEAPON_BUILDER
	DMG_BULLET,		// TF_WEAPON_MEDIGUN
	DMG_BLAST,		// TF_WEAPON_GRENADE_MIRVBOMB
	DMG_BLAST | DMG_IGNITE | DMG_RADIUS_MAX,		// TF_WEAPON_FLAMETHROWER_ROCKET
	DMG_BLAST | DMG_HALF_FALLOFF,					// TF_WEAPON_GRENADE_DEMOMAN
	DMG_GENERIC,	// TF_WEAPON_SENTRY_BULLET
	DMG_GENERIC,	// TF_WEAPON_SENTRY_ROCKET
	DMG_GENERIC,	// TF_WEAPON_DISPENSER
	DMG_GENERIC,	// TF_WEAPON_INVIS
	DMG_BULLET | DMG_IGNITE,		// TF_WEAPON_FLAREGUN
	DMG_GENERIC,	// TF_WEAPON_LUNCHBOX (ugh...)
	// This is a special entry that must match with TF_WEAPON_COUNT
	// to protect against updating the weapon list without updating this list
	TF_DMG_SENTINEL_VALUE
};

const char *g_szProjectileNames[] =
{
	"",
	"projectile_bullet",
	"projectile_rocket",
	"projectile_pipe",
	"projectile_pipe_remote",
	"projectile_syringe",
	"projectile_flare",
};

// these map to the projectiles named in g_szProjectileNames
int g_iProjectileWeapons[] = 
{
	TF_WEAPON_NONE,
	TF_WEAPON_PISTOL,
	TF_WEAPON_ROCKETLAUNCHER,
	TF_WEAPON_PIPEBOMBLAUNCHER,
	TF_WEAPON_GRENADELAUNCHER,
	TF_WEAPON_SYRINGEGUN_MEDIC,
	TF_WEAPON_FLAREGUN
};

const char *g_pszHintMessages[] =
{
	"#Hint_spotted_a_friend",
	"#Hint_spotted_an_enemy",
	"#Hint_killing_enemies_is_good",
	"#Hint_out_of_ammo",
	"#Hint_turn_off_hints",
	"#Hint_pickup_ammo",
	"#Hint_Cannot_Teleport_With_Flag",
	"#Hint_Cannot_Cloak_With_Flag",
	"#Hint_Cannot_Disguise_With_Flag",
	"#Hint_Cannot_Attack_While_Cloaked",
	"#Hint_ClassMenu",

// Grenades
	"#Hint_gren_caltrops",
	"#Hint_gren_concussion",
	"#Hint_gren_emp",
	"#Hint_gren_gas",
	"#Hint_gren_mirv",
	"#Hint_gren_nail",
	"#Hint_gren_napalm",
	"#Hint_gren_normal",

// Altfires
	"#Hint_altfire_sniperrifle",
	"#Hint_altfire_flamethrower",
	"#Hint_altfire_grenadelauncher",
	"#Hint_altfire_pipebomblauncher",
	"#Hint_altfire_rotate_building",

// Soldier
	"#Hint_Soldier_rpg_reload",

// Engineer
	"#Hint_Engineer_use_wrench_onown",
	"#Hint_Engineer_use_wrench_onother",
	"#Hint_Engineer_use_wrench_onfriend",
	"#Hint_Engineer_build_sentrygun",
	"#Hint_Engineer_build_dispenser",
	"#Hint_Engineer_build_teleporters",
	"#Hint_Engineer_pickup_metal",
	"#Hint_Engineer_repair_object",
	"#Hint_Engineer_metal_to_upgrade",
	"#Hint_Engineer_upgrade_sentrygun",

	"#Hint_object_has_sapper",

	"#Hint_object_your_object_sapped",
	"#Hint_enemy_using_dispenser",
	"#Hint_enemy_using_tp_entrance",
	"#Hint_enemy_using_tp_exit",
};

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int GetWeaponId( const char *pszWeaponName )
{
	// if this doesn't match, you need to add missing weapons to the array
	assert( ARRAYSIZE( g_aWeaponNames ) == ( TF_WEAPON_COUNT + 1 ) );

	for ( int iWeapon = 0; iWeapon < ARRAYSIZE( g_aWeaponNames ); ++iWeapon )
	{
		if ( !Q_stricmp( pszWeaponName, g_aWeaponNames[iWeapon] ) )
			return iWeapon;
	}

	return TF_WEAPON_NONE;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *WeaponIdToAlias( int iWeapon )
{
	// if this doesn't match, you need to add missing weapons to the array
	assert( ARRAYSIZE( g_aWeaponNames ) == ( TF_WEAPON_COUNT + 1 ) );

	if ( ( iWeapon >= ARRAYSIZE( g_aWeaponNames ) ) || ( iWeapon < 0 ) )
		return NULL;

	return g_aWeaponNames[iWeapon];
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *WeaponIdToClassname( int iWeapon )
{
	const char *pszAlias = WeaponIdToAlias( iWeapon );
	if ( pszAlias == NULL )
		return NULL;

	static char szClassname[128];
	V_strncpy( szClassname, pszAlias, sizeof( szClassname ) );
	V_strlower( szClassname );

	return szClassname;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char* TranslateWeaponEntForClass( const char* pszName, int iClass )
{
	if ( pszName )
	{
		for ( int i = 0; i < ARRAYSIZE( pszWpnEntTranslationList ); i++ )
		{
			if ( V_stricmp( pszName, pszWpnEntTranslationList[i].weapon_name ) == 0 )
			{
				return ((const char**)&(pszWpnEntTranslationList[i]))[1 + iClass];
			}
		}
	}
	return pszName;
}

#ifdef GAME_DLL

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int GetWeaponFromDamage( const CTakeDamageInfo &info )
{
	int iWeapon = TF_WEAPON_NONE;

	// Work out what killed the player, and send a message to all clients about it
	//const char *killer_weapon_name = TFGameRules()->GetKillingWeaponName( info, NULL );
	const char* killer_weapon_name = "";

	// Find the killer & the scorer
	CBaseEntity* pInflictor = info.GetInflictor();
	CBaseEntity* pKiller = info.GetAttacker();
	CBasePlayer* pScorer = TFGameRules()->GetDeathScorer( pKiller, pInflictor, NULL );

	// find the weapon the killer used

	if ( pScorer )	// Is the killer a client?
	{
		if ( pInflictor )
		{
			if ( pInflictor == pScorer )
			{
				// If the inflictor is the killer,  then it must be their current weapon doing the damage
				if ( pScorer->GetActiveWeapon() )
				{
					killer_weapon_name = pScorer->GetActiveWeapon()->GetClassname();
				}
			}
			else
			{
				killer_weapon_name = STRING( pInflictor->m_iClassname );  // it's just that easy
			}
		}
	}
	else if ( pInflictor )
	{
		killer_weapon_name = STRING( pInflictor->m_iClassname );
	}

	if ( !Q_strnicmp( killer_weapon_name, "tf_projectile", 13 ) )
	{
		for( int i = 0; i < ARRAYSIZE( g_szProjectileNames ); i++ )
		{
			if ( !Q_stricmp( &killer_weapon_name[ 3 ], g_szProjectileNames[ i ] ) )
			{
				iWeapon = g_iProjectileWeapons[ i ];
				break;
			}
		}
	}
	else
	{
		int iLen = Q_strlen( killer_weapon_name );

		// strip off _projectile from projectiles shot from other projectiles
		if ( ( iLen < 256 ) && ( iLen > 11 ) && !Q_stricmp( &killer_weapon_name[ iLen - 11 ], "_projectile" ) )
		{
			char temp[ 256 ];
			Q_strcpy( temp, killer_weapon_name );
			temp[ iLen - 11 ] = 0;

			// set the weapon used
			iWeapon = GetWeaponId( temp );
		}
		else
		{
			// set the weapon used
			iWeapon = GetWeaponId( killer_weapon_name );
		}
	}

	return iWeapon;
}

#endif

// ------------------------------------------------------------------------------------------------ //
// CObjectInfo tables.
// ------------------------------------------------------------------------------------------------ //

CObjectInfo::CObjectInfo( char *pObjectName )
{
	m_pObjectName = pObjectName;
	m_pClassName = NULL;
	m_flBuildTime = -9999;
	m_nMaxObjects = -9999;
	m_Cost = -9999;
	m_CostMultiplierPerInstance = -999;
	m_flUpgradeDuration = -999;
	m_UpgradeCost = -9999;
	m_MaxUpgradeLevel = -9999;
	m_pBuilderWeaponName = NULL;
	m_pBuilderPlacementString = NULL;
	m_SelectionSlot = -9999;
	m_SelectionPosition = -9999;
	m_bSolidToPlayerMovement = false;
	m_pIconActive = NULL;
	m_pIconInactive = NULL;
	m_pViewModel = NULL;
	m_pPlayerModel = NULL;
	m_iDisplayPriority = 0;
	m_bVisibleInWeaponSelection = true;
	m_pExplodeSound = NULL;
	m_pExplosionParticleEffect = NULL;
	m_bAutoSwitchTo = false;
}


CObjectInfo::~CObjectInfo()
{
	delete [] m_pClassName;
	delete [] m_pStatusName;
	delete [] m_pBuilderWeaponName;
	delete [] m_pBuilderPlacementString;
	delete [] m_pIconActive;
	delete [] m_pIconInactive;
	delete [] m_pViewModel;
	delete [] m_pPlayerModel;
	delete [] m_pExplodeSound;
	delete [] m_pExplosionParticleEffect;
}

CObjectInfo g_ObjectInfos[OBJ_LAST] =
{
	CObjectInfo( "OBJ_DISPENSER" ),
	CObjectInfo( "OBJ_TELEPORTER_ENTRANCE" ),
	CObjectInfo( "OBJ_TELEPORTER_EXIT" ),
	CObjectInfo( "OBJ_SENTRYGUN" ),
	CObjectInfo( "OBJ_ATTACHMENT_SAPPER" ),
};

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int GetBuildableId( const char *pszBuildableName )
{
	for ( int iBuildable = 0; iBuildable < OBJ_LAST; ++iBuildable )
	{
		if ( !Q_stricmp( pszBuildableName, g_ObjectInfos[iBuildable].m_pObjectName ) )
			return iBuildable;
	}

	return OBJ_LAST;
}


bool AreObjectInfosLoaded()
{
	return g_ObjectInfos[0].m_pClassName != NULL;
}


void LoadObjectInfos( IBaseFileSystem *pFileSystem )
{
	const char *pFilename = "scripts/objects.txt";

	// Make sure this stuff hasn't already been loaded.
	Assert( !AreObjectInfosLoaded() );

	KeyValues *pValues = new KeyValues( "Object descriptions" );
	if ( !pValues->LoadFromFile( pFileSystem, pFilename, "GAME" ) )
	{
		Error( "Can't open %s for object info.", pFilename );
		pValues->deleteThis();
		return;
	}

	// Now read each class's information in.
	for ( int iObj=0; iObj < ARRAYSIZE( g_ObjectInfos ); iObj++ )
	{
		CObjectInfo *pInfo = &g_ObjectInfos[iObj];
		KeyValues *pSub = pValues->FindKey( pInfo->m_pObjectName );
		if ( !pSub )
		{
			Error( "Missing section '%s' from %s.", pInfo->m_pObjectName, pFilename );
			pValues->deleteThis();
			return;
		}

		// Read all the info in.
		if ( (pInfo->m_flBuildTime = pSub->GetFloat( "BuildTime", -999 )) == -999 ||
			(pInfo->m_nMaxObjects = pSub->GetInt( "MaxObjects", -999 )) == -999 ||
			(pInfo->m_Cost = pSub->GetInt( "Cost", -999 )) == -999 ||
			(pInfo->m_CostMultiplierPerInstance = pSub->GetFloat( "CostMultiplier", -999 )) == -999 ||
			(pInfo->m_flUpgradeDuration = pSub->GetFloat( "UpgradeDuration", -999 )) == -999 ||
			(pInfo->m_UpgradeCost = pSub->GetInt( "UpgradeCost", -999 )) == -999 ||
			(pInfo->m_MaxUpgradeLevel = pSub->GetInt( "MaxUpgradeLevel", -999 )) == -999 ||
			(pInfo->m_SelectionSlot = pSub->GetInt( "SelectionSlot", -999 )) == -999 ||
			(pInfo->m_SelectionPosition = pSub->GetInt( "SelectionPosition", -999 )) == -999 )
		{
			Error( "Missing data for object '%s' in %s.", pInfo->m_pObjectName, pFilename );
			pValues->deleteThis();
			return;
		}

		pInfo->m_pClassName = ReadAndAllocStringValue( pSub, "ClassName", pFilename );
		pInfo->m_pStatusName = ReadAndAllocStringValue( pSub, "StatusName", pFilename );
		pInfo->m_pBuilderWeaponName = ReadAndAllocStringValue( pSub, "BuilderWeaponName", pFilename );
		pInfo->m_pBuilderPlacementString = ReadAndAllocStringValue( pSub, "BuilderPlacementString", pFilename );
		pInfo->m_bSolidToPlayerMovement = pSub->GetInt( "SolidToPlayerMovement", 0 ) ? true : false;
		pInfo->m_pIconActive = ReadAndAllocStringValue( pSub, "IconActive", pFilename );
		pInfo->m_pIconInactive = ReadAndAllocStringValue( pSub, "IconInactive", pFilename );
		pInfo->m_pViewModel = ReadAndAllocStringValue( pSub, "Viewmodel", pFilename );
		pInfo->m_pPlayerModel = ReadAndAllocStringValue( pSub, "Playermodel", pFilename );
		pInfo->m_iDisplayPriority = pSub->GetInt( "DisplayPriority", 0 );
		pInfo->m_pHudStatusIcon = ReadAndAllocStringValue( pSub, "HudStatusIcon", pFilename );
		pInfo->m_bVisibleInWeaponSelection = ( pSub->GetInt( "VisibleInWeaponSelection", 1 ) > 0 );
		pInfo->m_pExplodeSound = ReadAndAllocStringValue( pSub, "ExplodeSound", pFilename );
		pInfo->m_pExplosionParticleEffect = ReadAndAllocStringValue( pSub, "ExplodeEffect", pFilename );
		pInfo->m_bAutoSwitchTo = ( pSub->GetInt( "autoswitchto", 0 ) > 0 );

		pInfo->m_iMetalToDropInGibs = pSub->GetInt( "MetalToDropInGibs", 0 );
	}

	pValues->deleteThis();
}


const CObjectInfo* GetObjectInfo( int iObject )
{
	Assert( iObject >= 0 && iObject < OBJ_LAST );
	Assert( AreObjectInfosLoaded() );
	return &g_ObjectInfos[iObject];
}

ConVar tf_cheapobjects( "tf_cheapobjects","0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Set to 1 and all objects will cost 0" );

//-----------------------------------------------------------------------------
// Purpose: Return the cost of another object of the specified type
//			If bLast is set, return the cost of the last built object of the specified type
// 
// Note: Used to contain logic from tf2 that multiple instances of the same object
//       cost different amounts. See tf2/game_shared/tf_shareddefs.cpp for details
//-----------------------------------------------------------------------------
int CalculateObjectCost( int iObjectType )
{
	if ( tf_cheapobjects.GetInt() )
	{
		return 0;
	}

	int iCost = GetObjectInfo( iObjectType )->m_Cost;

	return iCost;
}

//-----------------------------------------------------------------------------
// Purpose: Calculate the cost to upgrade an object of a specific type
//-----------------------------------------------------------------------------
int	CalculateObjectUpgrade( int iObjectType, int iObjectLevel )
{
	// Max level?
	if ( iObjectLevel >= GetObjectInfo( iObjectType )->m_MaxUpgradeLevel )
		return 0;

	int iCost = GetObjectInfo( iObjectType )->m_UpgradeCost;
	for ( int i = 0; i < (iObjectLevel - 1); i++ )
	{
		iCost *= OBJECT_UPGRADE_COST_MULTIPLIER_PER_LEVEL;
	}

	return iCost;
}

//-----------------------------------------------------------------------------
// Purpose: Return true if the specified class is allowed to build the specified object type
//-----------------------------------------------------------------------------
bool ClassCanBuild( int iClass, int iObjectType )
{
	/*
	for ( int i = 0; i < OBJ_LAST; i++ )
	{
		// Hit the end?
		if ( g_TFClassInfos[iClass].m_pClassObjects[i] == OBJ_LAST )
			return false;

		// Found it?
		if ( g_TFClassInfos[iClass].m_pClassObjects[i] == iObjectType )
			return true;
	}

	return false;
	*/

	return ( iClass == TF_CLASS_ENGINEER );
}

const unsigned char* GetTFEncryptionKey( void )
{
	return (unsigned char*)"E2NcUkG2";
}