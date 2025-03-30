//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: HUD Target ID element
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "tf_hud_target_id.h"
#include "c_tf_playerresource.h"
#include "iclientmode.h"
#include "vgui/ILocalize.h"
#include "c_baseobject.h"
#include "c_team.h"
#include "tf_gamerules.h"
#include "tf_hud_statpanel.h"
#if defined( REPLAY_ENABLED )
#include "replay/iclientreplaycontext.h"
#include "replay/ireplaymoviemanager.h"
#include "replay/ienginereplay.h"
#endif // REPLAY_ENABLED
#include "tf_weapon_bonesaw.h"
#include "sourcevr/isourcevirtualreality.h"
#include "tf_revive.h"
#include "tf_logic_robot_destruction.h"
#include "entity_capture_flag.h"
#include "vgui_avatarimage.h"

#include "VGuiMatSurface/IMatSystemSurface.h"
#include "renderparm.h"

#include "tf_dropped_weapon.h"
#include "econ/econ_item_description.h"
#include "inputsystem/iinputsystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar cl_hud_minmode;

DECLARE_HUDELEMENT( CMainTargetID );
DECLARE_HUDELEMENT( CSpectatorTargetID );
DECLARE_HUDELEMENT( CSecondaryTargetID );

using namespace vgui;

enum
{
	SPECTATOR_TARGET_ID_NORMAL = 0,
	SPECTATOR_TARGET_ID_BOTTOM_LEFT,
	SPECTATOR_TARGET_ID_BOTTOM_CENTER,
	SPECTATOR_TARGET_ID_BOTTOM_RIGHT,
};

ConVar tf_hud_target_id_alpha( "tf_hud_target_id_alpha", "100", FCVAR_ARCHIVE, "Alpha value of target id background, default 100" );
ConVar tf_hud_target_id_offset( "tf_hud_target_id_offset", "0", FCVAR_ARCHIVE, "RES file Y offset for target id" );
ConVar tf_hud_target_id_show_avatars( "tf_hud_target_id_show_avatars", "2", FCVAR_ARCHIVE, "Display Steam avatars on TargetID when using floating health icons.  1 = everyone, 2 = friends only." );


bool ShouldHealthBarBeVisible( CBaseEntity *pTarget, CTFPlayer *pLocalPlayer )
{
	if ( !pTarget || !pLocalPlayer )
		return false;

	if ( pTarget->IsHealthBarVisible() )
		return true;

	if ( !pTarget->IsPlayer() )
		return false;

	int iHideEnemyHealth = 0;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pLocalPlayer, iHideEnemyHealth, hide_enemy_health );
	if ( ( iHideEnemyHealth > 0 ) && !pLocalPlayer->InSameTeam( pTarget ) )
		return false;

	if ( pLocalPlayer->IsPlayerClass( TF_CLASS_SPY ) )
		return true;

	if ( pLocalPlayer->InSameTeam( pTarget ) )
		return true;

	if ( pLocalPlayer->InSameDisguisedTeam( pTarget ) )
		return true;

	int iSeeEnemyHealth = 0;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pLocalPlayer, iSeeEnemyHealth, see_enemy_health )
	if ( iSeeEnemyHealth )
		return true;
	
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTargetID::CTargetID( const char *pElementName ) :
	CHudElement( pElementName ), BaseClass( NULL, pElementName )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_hFont = g_hFontTrebuchet24;
	m_flLastChangeTime = 0;
	m_iLastEntIndex = 0;
	m_nOriginalY = 0;
	m_bArenaPanelVisible = false;

	SetHiddenBits( HIDEHUD_MISCSTATUS | HIDEHUD_TARGET_ID );

	m_pTargetNameLabel = NULL;
	m_pTargetDataLabel = NULL;
	m_pBGPanel = NULL;
	m_pMoveableIcon = NULL;
	m_pMoveableSymbolIcon = NULL;
	m_pMoveableIconBG = NULL;
	m_pMoveableKeyLabel = NULL;
	m_pTargetHealth = new CTFSpectatorGUIHealth( this, "SpectatorGUIHealth" );
	m_pTargetAmmoIcon = NULL;
	m_pTargetKillStreakIcon = NULL;
	m_bLayoutOnUpdate = false;

	m_iLastScannedEntIndex = 0;
	m_pAvatarImage = NULL;

	RegisterForRenderGroup( "mid" );
	RegisterForRenderGroup( "commentary" );

	m_iRenderPriority = 5;

	ListenForGameEvent( "show_class_layout" );
	RegisterForRenderGroup( "arena_target_id" );
}

//-----------------------------------------------------------------------------
// Purpose: Setup
//-----------------------------------------------------------------------------
void CTargetID::Reset( void )
{
	m_pTargetHealth->Reset();

	vgui::IScheme *pScheme = vgui::scheme()->GetIScheme( GetScheme() );
	if ( pScheme )
	{
		m_LabelColorDefault = pScheme->GetColor( "Label.TextColor", Color( 255, 255, 255, 255 ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTargetID::FireGameEvent( IGameEvent * event )
{
	const char *eventName = event->GetName();

	if ( FStrEq( "show_class_layout", eventName ) )
	{
		if ( TFGameRules() && TFGameRules()->IsInArenaMode() && GetLocalPlayerTeam() > LAST_SHARED_TEAM )
		{
			m_bArenaPanelVisible = event->GetBool( "show", false );
		}
		else
		{
			m_bArenaPanelVisible = false;
		}

		InvalidateLayout( true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Find out which player to pull an avatar image from.  pTFPlayer is the player under the crosshair.
//-----------------------------------------------------------------------------
C_TFPlayer *CTargetID::GetTargetForSteamAvatar( C_TFPlayer *pTFPlayer )
{
	if ( !tf_hud_target_id_show_avatars.GetBool() )
		return NULL;

	if ( !pTFPlayer || ( g_TF_PR && g_TF_PR->IsFakePlayer( pTFPlayer->entindex() ) ) )
		return NULL;

	C_TFPlayer *pTFLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pTFLocalPlayer )
		return NULL;

	// Save room when healing or being healed
	if ( pTFLocalPlayer->IsPlayerClass( TF_CLASS_MEDIC ) && pTFLocalPlayer->MedicGetHealTarget() == pTFPlayer )
		return NULL;
	
	C_TFPlayer *pTFHealer = NULL;
	float flHealerChargeLevel = -1.f;
	pTFLocalPlayer->GetHealer( &pTFHealer, &flHealerChargeLevel );
	if ( pTFHealer && pTFHealer->entindex() == m_iTargetEntIndex )
		return NULL;

	if ( pTFPlayer->IsPlayerClass( TF_CLASS_SPY ) && pTFPlayer->m_Shared.InCond( TF_COND_DISGUISED ) )
	{
		C_TFPlayer *pDisguiseTarget = pTFPlayer->m_Shared.GetDisguiseTarget();
		if ( pDisguiseTarget && ( pTFLocalPlayer->InSameTeam( pDisguiseTarget ) || pDisguiseTarget == pTFLocalPlayer ) )
		{
			// Bots don't (currently) have avatars.
			if ( pDisguiseTarget->IsBot() )
				return NULL;

			if ( tf_hud_target_id_show_avatars.GetInt() == 2 && !pTFLocalPlayer->IsPlayerOnSteamFriendsList( pDisguiseTarget ) )
				return NULL;

			return pDisguiseTarget;
		}
	}

	if ( pTFLocalPlayer->IsPlayerOnSteamFriendsList( pTFPlayer ) )
		return pTFPlayer;

	if ( tf_hud_target_id_show_avatars.GetInt() == 1 )
		return pTFPlayer;

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTargetID::ApplySchemeSettings( vgui::IScheme *scheme )
{
	LoadControlSettings( "resource/UI/TargetID.res" );

	BaseClass::ApplySchemeSettings( scheme );

	m_pTargetNameLabel = dynamic_cast<Label *>(FindChildByName("TargetNameLabel"));
	m_pTargetDataLabel = dynamic_cast<Label *>(FindChildByName("TargetDataLabel"));
	m_pBGPanel =  dynamic_cast<CTFImagePanel *> ( FindChildByName("TargetIDBG") );
	m_pMoveableSubPanel = dynamic_cast<vgui::EditablePanel *> ( FindChildByName("MoveableSubPanel") );
	if ( m_pMoveableSubPanel )
	{
		m_pMoveableIcon = dynamic_cast<CIconPanel *> ( m_pMoveableSubPanel->FindChildByName("MoveableIcon") );
		m_pMoveableSymbolIcon = dynamic_cast<vgui::ImagePanel *> ( m_pMoveableSubPanel->FindChildByName("MoveableSymbolIcon") );
		m_pMoveableIconBG = dynamic_cast<CIconPanel *> ( m_pMoveableSubPanel->FindChildByName("MoveableIconBG") );
		m_pMoveableKeyLabel = dynamic_cast<Label *>( m_pMoveableSubPanel->FindChildByName("MoveableKeyLabel") );
	}
	m_hFont = scheme->GetFont( "TargetID", true );
	m_pTargetAmmoIcon = dynamic_cast<vgui::ImagePanel *>( FindChildByName( "AmmoIcon" ) );
	m_pTargetKillStreakIcon = dynamic_cast<vgui::ImagePanel *>( FindChildByName( "KillStreakIcon" ) );
	m_pAvatarImage = dynamic_cast< CAvatarImagePanel* >( FindChildByName( "AvatarImage" ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTargetID::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	m_iRenderPriority = inResourceData->GetInt( "priority" );

	int x;
	GetPos( x, m_nOriginalY );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTargetID::GetRenderGroupPriority( void )
{
	return m_iRenderPriority;
}

//-----------------------------------------------------------------------------
// Purpose: clear out string etc between levels
//-----------------------------------------------------------------------------
void CTargetID::VidInit()
{
	CHudElement::VidInit();

	m_flLastChangeTime = 0;
	m_iLastEntIndex = 0;
}

bool CTargetID::IsValidIDTarget( int nEntIndex, float flOldTargetRetainFOV, float &flNewTargetRetainFOV )
{
	bool bReturn = false;
	flNewTargetRetainFOV = 0.0f;

	C_TFPlayer *pLocalTFPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalTFPlayer )
		return false;


	if ( nEntIndex )
	{
		C_BaseEntity *pEnt = cl_entitylist->GetEnt( nEntIndex );
		if ( pEnt )
		{
			Vector vDiff = pEnt->EyePosition() - pLocalTFPlayer->EyePosition();
			float flDist;
			flDist = VectorNormalize( vDiff );

			if ( flOldTargetRetainFOV != 0.0f )
			{
				// It has a FOV that maintains previous targets
				Vector vForward;
				pLocalTFPlayer->EyeVectors( &vForward );

				float fAngle = 1.0f - vDiff.Dot( vForward );
				fAngle = RemapVal( fAngle, 0.0f, 1.0f, 0.0f, 90.0f );

				if ( fAngle > flOldTargetRetainFOV )
				{
					return false;
				}
			}

			C_TFPlayer *pPlayer = ToTFPlayer( pEnt );

			int iHideEnemyHealth = 0;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pLocalTFPlayer, iHideEnemyHealth, hide_enemy_health );

			bool bInSameTeam = pLocalTFPlayer->InSameDisguisedTeam( pEnt );	
			bool bSpy = pLocalTFPlayer->IsPlayerClass( TF_CLASS_SPY ) && iHideEnemyHealth == 0;

			if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
			{
				// We don't want to show health bars to the spy in MVM because it's distracting
				bSpy = false;

				// Are we disguised as the enemy?
				if ( pLocalTFPlayer->m_Shared.InCond( TF_COND_DISGUISED ) && pLocalTFPlayer->m_Shared.GetDisguiseTeam() != pLocalTFPlayer->GetTeamNumber() )
				{
					// Get the target's apparent team
					int iTheirApparentTeam = pEnt->GetTeamNumber();
					if ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) )
					{
						iTheirApparentTeam = pPlayer->m_Shared.GetDisguiseTeam();
					}

					// Are we disguised as they appear?
					if ( pLocalTFPlayer->m_Shared.GetDisguiseTeam() == iTheirApparentTeam )
					{
						// Don't show the health
						bInSameTeam = false;
					}
				}
			}

			bool bSpectator = pLocalTFPlayer->GetTeamNumber() == TEAM_SPECTATOR;
			int iSeeEnemyHealth = 0;
			bool bStealthed = false;
			bool bHealthBarVisible = ShouldHealthBarBeVisible( pEnt, pLocalTFPlayer );
			bool bShow = bHealthBarVisible;

			if ( pPlayer )
			{
				if ( pPlayer->m_Shared.IsStealthed() )
				{
					bStealthed = true;
					bHealthBarVisible = false;
					bShow = false;
				}

				if ( !bStealthed )
				{
					CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pLocalTFPlayer, iSeeEnemyHealth, see_enemy_health );
				}

				bool bMaintainInFOV = !pLocalTFPlayer->InSameTeam( pEnt );

				if ( bHealthBarVisible )
				{
					bool bEnemyPlayer = pPlayer->GetTeamNumber() != pLocalTFPlayer->GetTeamNumber();
					bool bEnemyMiniBoss = pPlayer->IsMiniBoss() && bEnemyPlayer;
					bShow = bEnemyMiniBoss;

					if ( bShow )
					{
						bMaintainInFOV = false;

						// Minibosses keep the health indicator up within a small FOV until a different valid target is selected
						// The FOV needs to grow exponentially when a target is getting near
						if ( bEnemyMiniBoss )
						{
							bMaintainInFOV = true;
						}
					}
				}

				if ( bMaintainInFOV )
				{
					const float flMaxDist = 800.0f;
					float fInterp = RemapVal( flMaxDist - MIN( flDist, flMaxDist ), 0.0f, flMaxDist, 0.0f, 1.0f );
					fInterp *= fInterp;
					flNewTargetRetainFOV = fInterp * 13.0f + 0.75f;
				}

				bReturn = ( bSpectator || pLocalTFPlayer->InSameTeam( pEnt ) || ( ( bInSameTeam || bSpy || iSeeEnemyHealth ) && !bStealthed ) );
			}


			if ( pEnt->IsBaseObject() && ( bInSameTeam || bSpy ) )
			{
				bReturn = true;
			}
			else if ( pEnt->IsVisibleToTargetID() )
			{
				bReturn = true;
			}
		}
	}

	return bReturn;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTargetID::ShouldDraw( void )
{
	if ( !CHudElement::ShouldDraw() )
	{
		return false;
	}

	if ( TFGameRules() && TFGameRules()->ShowMatchSummary() )
	{
		return false;
	}

	C_TFPlayer *pLocalTFPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalTFPlayer )
	{
		return false;
	}

	if ( pLocalTFPlayer->IsTaunting() )
	{
		return false;
	}

	// Get our target's ent index
	m_iTargetEntIndex = CalculateTargetIndex(pLocalTFPlayer);
	if ( !m_iTargetEntIndex )
	{
		if ( m_flTargetRetainFOV == 0.0f )
		{
			// Check to see if we should clear our ID
			if ( m_flLastChangeTime && ( gpGlobals->curtime > m_flLastChangeTime ) )
			{
				m_flLastChangeTime = 0;
				m_iLastEntIndex = 0;
			}
			else
			{
				// Keep re-using the old one
				m_iTargetEntIndex = m_iLastEntIndex;
			}
		}
	}
	else
	{
		m_flLastChangeTime = gpGlobals->curtime;

		if ( m_iTargetEntIndex != m_iLastScannedEntIndex )
		{
			m_iLastScannedEntIndex = m_iTargetEntIndex;
		}
	}

	float flTargetRetainFOV = 0.0f;
	bool bReturn = IsValidIDTarget( m_iTargetEntIndex, 0.0f, flTargetRetainFOV );

	if ( !bReturn )
	{
		m_iLastEntIndex = 0;
	}
	else
	{
		if ( !IsVisible() || (m_iTargetEntIndex != m_iLastEntIndex) )
		{
			m_iLastEntIndex = m_iTargetEntIndex;
			m_bLayoutOnUpdate = true;
			m_flTargetRetainFOV = flTargetRetainFOV;
			if ( m_pAvatarImage )
			{
				m_pAvatarImage->SetVisible( false );
			}
		}

		UpdateID();
	}

	return bReturn;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTargetID::PerformLayout( void )
{
	int iXIndent = YRES( 5 );
	int iXPostdent = YRES( 10 );
	int iWidth = m_pTargetHealth->GetWide() + iXIndent + iXPostdent;

	int iTextW, iTextH;
	int iDataW, iDataH;
	m_pTargetNameLabel->GetContentSize( iTextW, iTextH );
	m_pTargetDataLabel->GetContentSize( iDataW, iDataH );
	iWidth += max( iTextW, iDataW );

	if ( m_pBGPanel )
	{
		m_pBGPanel->SetSize( iWidth, GetTall() );
	}

	// Put the moveable icon to the right hand of our panel
	if ( m_pMoveableSubPanel && m_pMoveableSubPanel->IsVisible() )
	{
		if ( m_pMoveableKeyLabel && m_pMoveableIcon && m_pMoveableSymbolIcon && m_pMoveableIconBG )
		{
			m_pMoveableKeyLabel->SizeToContents();

			int iIndent = XRES( 4 );
			int iMoveWide = MAX( XRES( 16 ) + m_pMoveableKeyLabel->GetWide() + iIndent, (m_pMoveableIcon->GetWide()) + iIndent + XRES( 8 ) );
			m_pMoveableKeyLabel->SetWide( iMoveWide );
			m_pMoveableSubPanel->SetSize( iMoveWide, GetTall() );
			m_pMoveableSubPanel->SetPos( iWidth - iIndent, 0 );

			int x, y;
			m_pMoveableKeyLabel->GetPos( x, y );
			m_pMoveableSymbolIcon->SetPos( (iMoveWide - m_pMoveableSymbolIcon->GetWide()) * 0.5, y - m_pMoveableSymbolIcon->GetTall() );
			m_pMoveableSymbolIcon->GetPos( x, y );
			m_pMoveableIcon->SetPos( (iMoveWide - m_pMoveableIcon->GetWide()) * 0.5, y - m_pMoveableIcon->GetTall() );
			m_pMoveableIconBG->SetSize( m_pMoveableSubPanel->GetWide(), m_pMoveableSubPanel->GetTall() );
		}
	}

	if ( m_pMoveableSubPanel && m_pMoveableSubPanel->IsVisible() )
	{
		// Now add our extra width to the total size
		iWidth += m_pMoveableSubPanel->GetWide();
	}

	SetSize( iWidth, GetTall() );

	int iX, iY;
	GetPos( iX, iY );
	SetPos( (ScreenWidth() - iWidth) * 0.5, iY );

};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTargetID::CalculateTargetIndex( C_TFPlayer *pLocalTFPlayer ) 
{ 
	int iIndex = pLocalTFPlayer->GetIDTarget(); 

	// If our target entity is already in our secondary ID, don't show it in primary.
	CSecondaryTargetID *pSecondaryID = GET_HUDELEMENT( CSecondaryTargetID );
	if ( pSecondaryID && pSecondaryID != this && pSecondaryID->GetTargetIndex() == iIndex )
	{
		iIndex = 0;
	}

	return iIndex;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTargetID::UpdateID( void )
{
	wchar_t sIDString[ MAX_ID_STRING ] = L"";
	wchar_t sDataString[ MAX_ID_STRING ] = L"";

	C_TFPlayer *pLocalTFPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalTFPlayer )
		return;

	// Default the labels' colors
	Color colorName = m_LabelColorDefault;
	Color colorData = m_LabelColorDefault;

	// Get our target's ent index
	// Is this an entindex sent by the server?
	if ( m_iTargetEntIndex )
	{
		C_BaseEntity *pEnt = cl_entitylist->GetEnt( m_iTargetEntIndex );
		if ( !pEnt )
			return;

		bool bShowHealth = false;
		float flHealth = 0;
		float flMaxHealth = 1;
		int iMaxBuffedHealth = 0;
		int iTargetTeam = pEnt->GetTeamNumber();
		const char *pszActionCommand = NULL;
		const char *pszActionIcon = NULL;

		m_pTargetHealth->SetBuilding( false );
		m_pTargetHealth->SetLevel( -1 );

		// Some entities we always want to check, cause the text may change
		// even while we're looking at it
		// Is it a player?
		if ( IsPlayerIndex( m_iTargetEntIndex ) )
		{
			const char *printFormatString = NULL;
			wchar_t wszPlayerName[ MAX_PLAYER_NAME_LENGTH ];
			bool bDisguisedTarget = false;
			bool bDisguisedEnemy = false;

			C_TFPlayer *pPlayer = static_cast<C_TFPlayer*>( pEnt );
			if ( !pPlayer )
				return;

			C_TFPlayer *pDisguiseTarget = NULL;
			g_pVGuiLocalize->ConvertANSIToUnicode( pPlayer->GetPlayerName(), wszPlayerName, sizeof(wszPlayerName) );

			// determine if the target is a disguised spy (either friendly or enemy)
			if ( pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) && // they're disguised
				//!pPlayer->m_Shared.InCond( TF_COND_DISGUISING ) && // they're not in the process of disguising
				!pPlayer->m_Shared.IsStealthed() ) // they're not cloaked
			{
				bDisguisedTarget = true;
				pDisguiseTarget = pPlayer->m_Shared.GetDisguiseTarget();

				if ( pLocalTFPlayer->InSameTeam( pEnt ) == false )
				{
					iTargetTeam = pPlayer->m_Shared.GetDisguiseTeam();
				}
			}

			if ( bDisguisedTarget )
			{
				// is the target a disguised enemy spy?
				if ( pPlayer->IsEnemyPlayer() )
				{
					if ( pDisguiseTarget )
					{
						bDisguisedEnemy = true;
						// change the player name
						g_pVGuiLocalize->ConvertANSIToUnicode( pDisguiseTarget->GetPlayerName(), wszPlayerName, sizeof(wszPlayerName) );
						// change the team  / team color
					}
				}
			}

			bool bInSameTeam = pLocalTFPlayer->InSameDisguisedTeam( pEnt );
			bool bSpy = pLocalTFPlayer->IsPlayerClass( TF_CLASS_SPY );
			bool bMedic = pLocalTFPlayer->IsPlayerClass( TF_CLASS_MEDIC );
			bool bHeavy = pLocalTFPlayer->IsPlayerClass( TF_CLASS_HEAVYWEAPONS );

			// See if the player wants to fill in the data string
			bool bIsAmmoData = false;
			bool bIsKillStreakData = false;
			pPlayer->GetTargetIDDataString( bDisguisedTarget, sDataString, sizeof(sDataString), bIsAmmoData, bIsKillStreakData );
			if ( pLocalTFPlayer->GetTeamNumber() == TEAM_SPECTATOR || bInSameTeam || bSpy || bDisguisedEnemy || bMedic || bHeavy )
			{
				printFormatString = "#TF_playerid_sameteam";
				bShowHealth = true;
			}
			else if ( pLocalTFPlayer->m_Shared.GetState() == TF_STATE_DYING )
			{
				// We're looking at an enemy who killed us.
				printFormatString = "#TF_playerid_diffteam";
				bShowHealth = true;
			}			

			if ( bShowHealth )
			{
				if ( g_TF_PR )
				{
					if ( bDisguisedEnemy )
					{
						flHealth = (float)pPlayer->m_Shared.GetDisguiseHealth();
						flMaxHealth = (float)pPlayer->m_Shared.GetDisguiseMaxHealth();
						iMaxBuffedHealth = pPlayer->m_Shared.GetDisguiseMaxBuffedHealth();
					}
					else
					{
						flHealth = (float)pPlayer->GetHealth();
						flMaxHealth = g_TF_PR->GetMaxHealth( m_iTargetEntIndex );
						iMaxBuffedHealth = pPlayer->m_Shared.GetMaxBuffedHealth();
					}
				}
				else
				{
					bShowHealth = false;
				}
			}

			if ( printFormatString )
			{
				const wchar_t *pszPrepend = GetPrepend();
				if ( !pszPrepend || !pszPrepend[0] )
				{
					pszPrepend = L"";
				}
				g_pVGuiLocalize->ConstructString_safe( sIDString, g_pVGuiLocalize->Find(printFormatString), 2, pszPrepend, wszPlayerName );
			}

			// Show target's clip state to attached medics
			bool bShowClipInfo = bIsAmmoData &&
								 sDataString[0] && 
								 ToTFPlayer( pLocalTFPlayer->MedicGetHealTarget() ) == pPlayer;
			if ( m_pTargetAmmoIcon && m_pTargetAmmoIcon->IsVisible() != bShowClipInfo )
			{
				m_pTargetAmmoIcon->SetVisible( bShowClipInfo );
			}

			bool bShowKillStreak = bIsKillStreakData && sDataString[0];
			if ( m_pTargetKillStreakIcon && m_pTargetKillStreakIcon->IsVisible() != bShowKillStreak )
			{
				m_pTargetKillStreakIcon->SetVisible( bShowKillStreak );
			}
		}
		else	
		{
			// see if it is an object
			if ( pEnt->IsBaseObject() )
			{
				C_BaseObject *pObj = assert_cast<C_BaseObject *>( pEnt );

				pObj->GetTargetIDString( sIDString, sizeof(sIDString), false );
				pObj->GetTargetIDDataString( sDataString, sizeof(sDataString) );
				bShowHealth = true;
				flHealth = pObj->GetHealth();
				flMaxHealth = pObj->GetMaxHealth();
				m_pTargetHealth->SetBuilding( true );
				
				if ( m_pTargetKillStreakIcon )
				{
					m_pTargetKillStreakIcon->SetVisible( false );
				}

				// Switch the icon to the right object
				if ( pObj->GetBuilder() == pLocalTFPlayer )
				{
					int iObj = pObj->GetType();

					if ( iObj >= OBJ_DISPENSER && iObj <= OBJ_SENTRYGUN )
					{
						if ( pLocalTFPlayer->CanPickupBuilding(pObj) )
						{
							pszActionCommand = "+attack2";
						}

						
						switch ( iObj )
						{
						default:
						case OBJ_DISPENSER:
							pszActionIcon = "obj_status_dispenser";
							break;
						case OBJ_TELEPORTER:
							{
								pszActionIcon = (pObj->GetObjectMode() == MODE_TELEPORTER_ENTRANCE) ? "obj_status_tele_entrance" : "obj_status_tele_exit";
							}
							break;
						case OBJ_SENTRYGUN:
							{
								int iLevel = pObj->GetUpgradeLevel();
								if ( iLevel == 3 )
								{
									pszActionIcon =  "obj_status_sentrygun_3";
								}
								else
								{
									pszActionIcon = (iLevel == 2) ? "obj_status_sentrygun_2" : "obj_status_sentrygun_1";
								}
							}
							break;
						}
					}
				}
			}
			// Generic
			else if ( pEnt->IsVisibleToTargetID() )
			{
				CCaptureFlag *pFlag = dynamic_cast< CCaptureFlag * >( pEnt );
				if ( pFlag && pFlag->GetPointValue() > 0 )
				{
					bShowHealth = false;
					g_pVGuiLocalize->ConvertANSIToUnicode( CFmtStr("%d Points", pFlag->GetPointValue() ), sIDString, sizeof(sIDString) );
				}
				else
				{
					CTFDroppedWeapon *pDroppedWeapon = dynamic_cast< CTFDroppedWeapon * >( pEnt );
					if ( pDroppedWeapon )
					{
						CEconItemView* pDroppedEconItem = pDroppedWeapon->GetItem();
						if ( pLocalTFPlayer->GetDroppedWeaponInRange() != NULL )
						{
							pszActionIcon = "obj_weapon_pickup";
							pszActionCommand = "+use_action_slot_item";
						}
	
						if ( FStrEq( pDroppedEconItem->GetStaticData()->GetItemClass(), "tf_weapon_medigun" ) )
						{
							wchar_t wszChargeLevel[10];
							_snwprintf( wszChargeLevel, ARRAYSIZE( wszChargeLevel ) - 1, L"%.0f", pDroppedWeapon->GetChargeLevel() * 100 );
							wszChargeLevel[ARRAYSIZE( wszChargeLevel ) - 1] = '\0';

							g_pVGuiLocalize->ConstructString_safe( sIDString, L"%s1 (%s2%)", 2, CEconItemLocalizedFullNameGenerator( GLocalizationProvider(), pDroppedEconItem->GetItemDefinition(), pDroppedEconItem->GetItemQuality() ).GetFullName(), wszChargeLevel );
						}
						else
						{
							g_pVGuiLocalize->ConstructString_safe( sIDString, L"%s1", 1, CEconItemLocalizedFullNameGenerator( GLocalizationProvider(), pDroppedEconItem->GetItemDefinition(), pDroppedEconItem->GetItemQuality() ).GetFullName() );
						}

						locchar_t wszPlayerName [128];
						CBasePlayer *pOwner = GetPlayerByAccountID( pDroppedEconItem->GetAccountID() );
						// Bots will not work here, so don't fill this out.
						if ( pOwner )
						{
							g_pVGuiLocalize->ConvertANSIToUnicode( pOwner->GetPlayerName(), wszPlayerName, sizeof(wszPlayerName) );
							g_pVGuiLocalize->ConstructString_safe( sDataString, g_pVGuiLocalize->Find( "#TF_WhoDropped" ), 1, wszPlayerName );

							// Get the rarity color
							vgui::IScheme *pScheme = vgui::scheme()->GetIScheme( GetScheme() );
							if ( pScheme )
							{
								const char* pszColorName = GetItemSchema()->GetRarityColor( pDroppedEconItem->GetItemDefinition()->GetRarity() );
								pszColorName = pszColorName ? pszColorName : "TanLight";
								colorName = pScheme->GetColor( pszColorName, Color( 255, 255, 255, 255 ) );
							}
						}
					}
					else if ( pLocalTFPlayer->InSameTeam( pEnt ) )
					{
						bShowHealth = true;
						flHealth = pEnt->GetHealth();
						flMaxHealth = pEnt->GetMaxHealth();
						iMaxBuffedHealth = pEnt->GetMaxHealth();

						// Display respawn timer on revive markers by hacking bountymode's player level display
						if ( !pEnt->IsPlayer() )
						{
							CTFReviveMarker *pMarker = dynamic_cast< CTFReviveMarker* >( pEnt );
							if ( pMarker && pMarker->GetOwner() )
							{
								float flRespawn = TFGameRules()->GetNextRespawnWave( pMarker->GetTeamNumber(), pMarker->GetOwner() ) - gpGlobals->curtime;
								m_pTargetHealth->SetLevel( (int)flRespawn );

								g_pVGuiLocalize->ConvertANSIToUnicode( pMarker->GetOwner()->GetPlayerName(), sIDString, sizeof(sIDString) );
							}
						}
					}
				}
			}
		}

		// Setup health icon
		if ( !pEnt->IsAlive() && ( pEnt->IsPlayer() || pEnt->IsBaseObject() ) )
		{
			flHealth = 0;	// fixup for health being 1 when dead
		}

		m_pTargetHealth->SetHealth( flHealth, flMaxHealth, iMaxBuffedHealth );
		m_pTargetHealth->SetVisible( true );
		if ( m_pMoveableSubPanel )
		{
			bool bShowActionKey = pszActionCommand != NULL;
			if ( m_pMoveableSubPanel->IsVisible() != bShowActionKey )
			{
				m_pMoveableSubPanel->SetVisible( bShowActionKey );
				m_bLayoutOnUpdate = true;
			}

			if ( m_pMoveableSubPanel->IsVisible() )
			{
				const char *pBoundKey = nullptr;
				if ( pszActionCommand && ::input->IsSteamControllerActive() )
				{
					auto origin = g_pInputSystem->GetSteamControllerActionOrigin( *pszActionCommand == '+' ? pszActionCommand + 1 : pszActionCommand, GAME_ACTION_SET_FPSCONTROLS );
					if ( origin != k_EControllerActionOrigin_None )
					{
						auto pSteamController = g_pInputSystem->SteamControllerInterface();
						pBoundKey = pSteamController ? pSteamController->GetStringForActionOrigin( origin ) : "";
					}

				}
				if ( !pBoundKey )
				{
					pBoundKey = engine->Key_LookupBinding( pszActionCommand );
				}
				m_pMoveableSubPanel->SetDialogVariable( "movekey", pBoundKey );
			}

			if ( m_pMoveableIcon )
			{
				if ( pszActionIcon )
				{
					m_pMoveableIcon->SetIcon( pszActionIcon );
				}
				m_pMoveableIcon->SetVisible( pszActionIcon != NULL );
			}
		}

		if ( m_pAvatarImage && pEnt->IsPlayer() )
		{
			C_BasePlayer *pTFTarget = GetTargetForSteamAvatar( ToTFPlayer( pEnt ) );

			bool bShowAvatar = ( pTFTarget ) ? true : false;
			if ( m_pAvatarImage->IsVisible() != bShowAvatar )
			{
				m_pAvatarImage->SetVisible( bShowAvatar );
				if ( bShowAvatar )
				{
					m_pAvatarImage->SetPlayer( pTFTarget );
					m_pAvatarImage->SetShouldDrawFriendIcon( false );
					m_pAvatarImage->SetAlpha( tf_hud_target_id_alpha.GetInt() );
				}
			}
		}

		if ( m_pTargetNameLabel && m_pTargetDataLabel )
		{
			int iNameW, iDataW, iIgnored;
			m_pTargetNameLabel->GetContentSize( iNameW, iIgnored );
			m_pTargetDataLabel->GetContentSize( iDataW, iIgnored );

			// Target name
			if ( sIDString[0] )
			{
				sIDString[ ARRAYSIZE(sIDString)-1 ] = '\0';
				m_pTargetNameLabel->SetVisible(true);
				m_pTargetNameLabel->SetFgColor( colorName );

				// TODO: Support	if( hud_centerid.GetInt() == 0 )
				SetDialogVariable( "targetname", sIDString );
			}
			else
			{
				m_pTargetNameLabel->SetVisible(false);
				m_pTargetNameLabel->SetText("");
			}

			// Extra target data
			if ( sDataString[0] )
			{
				sDataString[ ARRAYSIZE(sDataString)-1 ] = '\0';
				m_pTargetDataLabel->SetVisible(true);
				m_pTargetDataLabel->SetFgColor( colorData );

				SetDialogVariable( "targetdata", sDataString );
			}
			else
			{
				m_pTargetDataLabel->SetVisible(false);
				m_pTargetDataLabel->SetText("");
			}

			int iPostNameW, iPostDataW;
			m_pTargetNameLabel->GetContentSize( iPostNameW, iIgnored );
			m_pTargetDataLabel->GetContentSize( iPostDataW, iIgnored );

			if ( m_pBGPanel )
			{
				m_pBGPanel->SetBGTeam( iTargetTeam );
				m_pBGPanel->UpdateBGImage();
				m_pBGPanel->SetAlpha( tf_hud_target_id_alpha.GetInt() );
			}

			if ( m_bLayoutOnUpdate || (iPostDataW != iDataW) || (iPostNameW != iNameW) )
			{
				InvalidateLayout( true );
				m_bLayoutOnUpdate = false;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSecondaryTargetID::CSecondaryTargetID( const char *pElementName ) : CTargetID( pElementName )
{
	m_wszPrepend[0] = '\0';

	RegisterForRenderGroup( "mid" );

	m_bWasHidingLowerElements = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSecondaryTargetID::ShouldDraw( void )
{
	bool bDraw = BaseClass::ShouldDraw();

	if ( bDraw )
	{
		if ( !m_bWasHidingLowerElements )
		{
			HideLowerPriorityHudElementsInGroup( "mid" );
			m_bWasHidingLowerElements = true;
		}
	}
	else 
	{
		if ( m_bWasHidingLowerElements )
		{
			UnhideLowerPriorityHudElementsInGroup( "mid" );
			m_bWasHidingLowerElements = false;
		}
	}

	return bDraw;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CSecondaryTargetID::CalculateTargetIndex( C_TFPlayer *pLocalTFPlayer )
{
	// If we're a medic & we're healing someone, target him.
	CBaseEntity *pHealTarget = pLocalTFPlayer->MedicGetHealTarget();
	if ( pHealTarget )
	{
		if ( pHealTarget->entindex() != m_iTargetEntIndex )
		{
			g_pVGuiLocalize->ConstructString_safe( m_wszPrepend, g_pVGuiLocalize->Find("#TF_playerid_healtarget" ), 0 );
		}
		return pHealTarget->entindex();
	}

	// If we have a healer, target him.
	C_TFPlayer *pHealer;
	float flHealerChargeLevel;
	pLocalTFPlayer->GetHealer( &pHealer, &flHealerChargeLevel );
	if ( pHealer )
	{
		if ( pHealer->entindex() != m_iTargetEntIndex )
		{
			g_pVGuiLocalize->ConstructString_safe( m_wszPrepend, g_pVGuiLocalize->Find("#TF_playerid_healer" ), 0 );
		}
		return pHealer->entindex();
	}

	if ( m_iTargetEntIndex )
	{
		m_wszPrepend[0] = '\0';
	}
	return 0;
}

// Separately declared versions of the hud element for alive and dead so they
// can have different positions

bool CMainTargetID::ShouldDraw( void )
{
	C_TFPlayer *pLocalTFPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalTFPlayer )
		return false;

	if ( pLocalTFPlayer->GetObserverMode() > OBS_MODE_NONE )
		return false;

	return BaseClass::ShouldDraw();
}

bool CSpectatorTargetID::ShouldDraw( void )
{
	C_TFPlayer *pLocalTFPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalTFPlayer )
		return false;

	if ( pLocalTFPlayer->GetObserverMode() <= OBS_MODE_NONE ||
		 pLocalTFPlayer->GetObserverMode() == OBS_MODE_FREEZECAM )
		return false;

	// Hide panel for freeze-cam screenshot?
	extern bool IsTakingAFreezecamScreenshot();
	extern ConVar hud_freezecamhide;
	
	if ( IsTakingAFreezecamScreenshot() && hud_freezecamhide.GetBool() )
		return false;

#if defined( REPLAY_ENABLED )
	if ( g_pEngineClientReplay->IsPlayingReplayDemo() )
		return false;
#endif

	return BaseClass::ShouldDraw();
}

int	CSpectatorTargetID::CalculateTargetIndex( C_TFPlayer *pLocalTFPlayer ) 
{ 
	int iIndex = BaseClass::CalculateTargetIndex( pLocalTFPlayer );

#if defined( REPLAY_ENABLED )
	// Don't execute this if we're watching a replay
	if ( ( !g_pEngineClientReplay || !g_pEngineClientReplay->IsPlayingReplayDemo() ) && pLocalTFPlayer->GetObserverMode() == OBS_MODE_IN_EYE && pLocalTFPlayer->GetObserverTarget() )
#else
	if ( pLocalTFPlayer->GetObserverMode() == OBS_MODE_IN_EYE && pLocalTFPlayer->GetObserverTarget() )
#endif
	{
		iIndex = pLocalTFPlayer->GetObserverTarget()->entindex();
	}

	return iIndex;
}