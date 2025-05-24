//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "hudelement.h"
#include "iclientmode.h"
#include <KeyValues.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui_controls/AnimationController.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui/IVGui.h>
#include <vgui/ISurface.h>
#include <vgui/IImage.h>
#include <vgui_controls/Label.h>

#include "c_playerresource.h"
#include "teamplay_round_timer.h"
#include "utlvector.h"
#include "entity_capture_flag.h"
#include "c_tf_player.h"
#include "c_team.h"
#include "c_tf_team.h"
#include "c_team_objectiveresource.h"
#include "tf_hud_objectivestatus.h"
#include "tf_spectatorgui.h"
#include "teamplayroundbased_gamerules.h"
#include "tf_gamerules.h"
#include "tf_hud_freezepanel.h"
#include "c_func_capture_zone.h"
#include "clientmode_shared.h"
#include "tf_hud_mediccallers.h"
#include "view.h"
#include "prediction.h"

using namespace vgui;

DECLARE_BUILD_FACTORY( CTFArrowPanel );
DECLARE_BUILD_FACTORY( CTFFlagStatus );

ConVar tf_rd_flag_ui_mode( "tf_rd_flag_ui_mode", "3", FCVAR_DEVELOPMENTONLY, "When flags are stolen and not visible: 0 = Show outlines (glows), 1 = Show most valuable enemy flag (icons), 2 = Show all enemy flags (icons), 3 = Show all flags (icons)." );

extern ConVar tf_flag_caps_per_round;

void AddSubKeyNamed( KeyValues *pKeys, const char *pszName );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFArrowPanel::CTFArrowPanel( Panel *parent, const char *name ) : vgui::Panel( parent, name )
{
	m_RedMaterial.Init( "hud/objectives_flagpanel_compass_red", TEXTURE_GROUP_VGUI ); 
	m_BlueMaterial.Init( "hud/objectives_flagpanel_compass_blue", TEXTURE_GROUP_VGUI ); 
	m_NeutralMaterial.Init( "hud/objectives_flagpanel_compass_grey", TEXTURE_GROUP_VGUI ); 
	m_NeutralRedMaterial.Init( "hud/objectives_flagpanel_compass_grey_with_red", TEXTURE_GROUP_VGUI ); 

	m_RedMaterialNoArrow.Init( "hud/objectives_flagpanel_compass_red_noArrow", TEXTURE_GROUP_VGUI ); 
	m_BlueMaterialNoArrow.Init( "hud/objectives_flagpanel_compass_blue_noArrow", TEXTURE_GROUP_VGUI ); 

	m_pMaterial = m_NeutralMaterial;
	m_bUseRed = false;
	m_flNextColorSwitch = 0.0f;

	ivgui()->AddTickSignal( GetVPanel(), 100 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFArrowPanel::OnTick( void )
{
	if ( !m_hEntity.Get() )
		return;

	C_BaseEntity *pEnt = m_hEntity.Get();
	m_pMaterial = m_NeutralMaterial;

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
	{
		if ( m_bUseRed )
		{
			m_pMaterial = m_NeutralRedMaterial;
		}
		else
		{
			m_pMaterial = m_NeutralMaterial;
		}

		if ( pEnt && TFGameRules()->GetMannVsMachineAlarmStatus() == true ) 
		{
			CCaptureFlag *pFlag = dynamic_cast<CCaptureFlag *>( pEnt );
			if ( pFlag && pFlag->IsStolen() )
			{
				if ( m_flNextColorSwitch < gpGlobals->curtime )
				{
					m_flNextColorSwitch = gpGlobals->curtime + 0.2f;
					m_bUseRed = !m_bUseRed;
				}
			}
			else
			{
				m_bUseRed = false;
			}
		}
		else
		{
			m_bUseRed = false;
		}
	}
	else
	{
		// figure out what material we need to use
		if ( pEnt->GetTeamNumber() == TF_TEAM_RED )
		{
			m_pMaterial = m_RedMaterial;

			if ( pLocalPlayer && ( pLocalPlayer->GetObserverMode() == OBS_MODE_IN_EYE ) )
			{
				// is our target a player?
				C_BaseEntity *pTargetEnt = pLocalPlayer->GetObserverTarget();
				if ( pTargetEnt && pTargetEnt->IsPlayer() )
				{
					// does our target have the flag and are they carrying the flag we're currently drawing?
					C_TFPlayer *pTarget = static_cast< C_TFPlayer* >( pTargetEnt );
					if ( pTarget->HasTheFlag() && ( pTarget->GetItem() == pEnt ) )
					{
						m_pMaterial = m_RedMaterialNoArrow;
					}
				}
			}
		}
		else if ( pEnt->GetTeamNumber() == TF_TEAM_BLUE )
		{
			m_pMaterial = m_BlueMaterial;

			if ( pLocalPlayer && ( pLocalPlayer->GetObserverMode() == OBS_MODE_IN_EYE ) )
			{
				// is our target a player?
				C_BaseEntity *pTargetEnt = pLocalPlayer->GetObserverTarget();
				if ( pTargetEnt && pTargetEnt->IsPlayer() )
				{
					// does our target have the flag and are they carrying the flag we're currently drawing?
					C_TFPlayer *pTarget = static_cast< C_TFPlayer* >( pTargetEnt );
					if ( pTarget->HasTheFlag() && ( pTarget->GetItem() == pEnt ) )
					{
						m_pMaterial = m_BlueMaterialNoArrow;
					}
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFArrowPanel::GetAngleRotation( void )
{
	float flRetVal = 0.0f;

	C_TFPlayer *pPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
	C_BaseEntity *pEnt = m_hEntity.Get();

	if ( pPlayer && pEnt )
	{
		QAngle vangles;
		Vector eyeOrigin;
		float zNear, zFar, fov;

		pPlayer->CalcView( eyeOrigin, vangles, zNear, zFar, fov );

		Vector vecFlag = pEnt->WorldSpaceCenter() - eyeOrigin;
		vecFlag.z = 0;
		vecFlag.NormalizeInPlace();

		Vector forward, right, up;
		AngleVectors( vangles, &forward, &right, &up );
		forward.z = 0;
		right.z = 0;
		forward.NormalizeInPlace();
		right.NormalizeInPlace();

		float dot = DotProduct( vecFlag, forward );
		float angleBetween = acos( dot );

		dot = DotProduct( vecFlag, right );

		if ( dot < 0.0f )
		{
			angleBetween *= -1;
		}

		flRetVal = RAD2DEG( angleBetween );
	}

	return flRetVal;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFArrowPanel::Paint()
{
	int x = 0;
	int y = 0;
	ipanel()->GetAbsPos( GetVPanel(), x, y );
	int nWidth = GetWide();
	int nHeight = GetTall();

	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->MatrixMode( MATERIAL_MODEL );
	pRenderContext->PushMatrix(); 

	VMatrix panelRotation;
	panelRotation.Identity();
	MatrixBuildRotationAboutAxis( panelRotation, Vector( 0, 0, 1 ), GetAngleRotation() );
//	MatrixRotate( panelRotation, Vector( 1, 0, 0 ), 5 );
	panelRotation.SetTranslation( Vector( x + nWidth/2, y + nHeight/2, 0 ) );
	pRenderContext->LoadMatrix( panelRotation );

	IMesh *pMesh = pRenderContext->GetDynamicMesh( true, NULL, NULL, m_pMaterial );

	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	meshBuilder.TexCoord2f( 0, 0, 0 );
	meshBuilder.Position3f( -nWidth/2, -nHeight/2, 0 );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.TexCoord2f( 0, 1, 0 );
	meshBuilder.Position3f( nWidth/2, -nHeight/2, 0 );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.TexCoord2f( 0, 1, 1 );
	meshBuilder.Position3f( nWidth/2, nHeight/2, 0 );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.TexCoord2f( 0, 0, 1 );
	meshBuilder.Position3f( -nWidth/2, nHeight/2, 0 );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();

	pMesh->Draw();
	pRenderContext->PopMatrix();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFArrowPanel::IsVisible( void )
{
	if( IsTakingAFreezecamScreenshot() )
		return false;

	return BaseClass::IsVisible();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFFlagStatus::CTFFlagStatus( Panel *parent, const char *name ) : EditablePanel( parent, name )
{
	m_pArrow = new CTFArrowPanel( this, "Arrow" );
	m_pStatusIcon = new CTFImagePanel( this, "StatusIcon" );
	m_pBriefcase = new CTFImagePanel( this, "Briefcase" );
	m_hEntity = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlagStatus::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	KeyValues *pConditions = NULL;

	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
	{
		pConditions = new KeyValues( "conditions" );

		if ( pConditions )
		{
			AddSubKeyNamed( pConditions, "if_mvm" );
		}
	}

	// load control settings...
	LoadControlSettings( "resource/UI/FlagStatus.res", NULL, NULL, pConditions );

	if ( pConditions )
	{
		pConditions->deleteThis();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFlagStatus::IsVisible( void )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pPlayer && pPlayer->GetObserverMode() == OBS_MODE_FREEZECAM )
		return false;

	return BaseClass::IsVisible();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlagStatus::UpdateStatus( void )
{
	if ( m_hEntity.Get() )
	{
		CCaptureFlag *pFlag = dynamic_cast<CCaptureFlag *>( m_hEntity.Get() );

		if ( pFlag )
		{
			const char *pszImage = "../hud/objectives_flagpanel_ico_flag_home";
			const char *pszBombImage = "../hud/bomb_dropped";

			if ( pFlag->IsDropped() )
			{
				pszImage = "../hud/objectives_flagpanel_ico_flag_dropped";
			}
			else if ( pFlag->IsStolen() )
			{
				pszImage = "../hud/objectives_flagpanel_ico_flag_moving";
				pszBombImage = "../hud/bomb_carried";
			}

			if ( m_pStatusIcon )
			{
				m_pStatusIcon->SetImage( pszImage );
			}

			if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() && m_pBriefcase )
			{
				m_pBriefcase->SetImage( pszBombImage );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFHudFlagObjectives::CTFHudFlagObjectives( Panel *parent, const char *name ) : EditablePanel( parent, name )
{
	m_pCarriedImage = NULL;
	m_pPlayingTo = NULL;
	m_bFlagAnimationPlayed = false;
	m_bCarryingFlag = false;
	m_pSpecCarriedImage = NULL;

	m_pRedFlag = new CTFFlagStatus( this, "RedFlag" );
	m_pBlueFlag = new CTFFlagStatus( this, "BlueFlag" );

	m_bPlayingHybrid_CTF_CP = false;
	m_bPlayingSpecialDeliveryMode = false;

	vgui::ivgui()->AddTickSignal( GetVPanel(), 250 );

	ListenForGameEvent( "flagstatus_update" );

	m_nNumValidFlags = -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFHudFlagObjectives::IsVisible( void )
{
	if( IsTakingAFreezecamScreenshot() )
		return false;

	return BaseClass::IsVisible();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudFlagObjectives::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	KeyValues *pConditions = NULL;

	bool bHybrid = TFGameRules() && TFGameRules()->IsPlayingHybrid_CTF_CP();
	bool bMVM = TFGameRules() && TFGameRules()->IsMannVsMachineMode();
	bool bSpecialDeliveryMode = TFGameRules() && TFGameRules()->IsPlayingSpecialDeliveryMode();

	int nNumFlags = 0;

	if ( m_pRedFlag && m_pRedFlag->GetEntity() != NULL )
	{
		nNumFlags++;
	}
	
	if ( m_pBlueFlag && m_pBlueFlag->GetEntity() != NULL )
	{
		nNumFlags++;
	}

	if ( nNumFlags == 2 && m_pRedFlag->GetEntity() == m_pBlueFlag->GetEntity() )
	{
		// They're both pointing at the same flag! There's really only 1
		nNumFlags = 1;
	}

	if ( nNumFlags == 0 )
	{
		pConditions = new KeyValues( "conditions" );
		if ( pConditions )
		{
			AddSubKeyNamed( pConditions, "if_no_flags" );
		}

		if ( bSpecialDeliveryMode )
		{
			AddSubKeyNamed( pConditions, "if_specialdelivery" );
		}
	}
	else
	{
		if ( bHybrid || ( nNumFlags == 1 ) || bMVM || bSpecialDeliveryMode )
		{
			pConditions = new KeyValues( "conditions" );
			if ( pConditions )
			{
				if ( bHybrid )
				{
					AddSubKeyNamed( pConditions, "if_hybrid" );
				}

				if ( nNumFlags == 1 || bSpecialDeliveryMode )
				{
					AddSubKeyNamed( pConditions, "if_hybrid_single" );
				}
				else if ( nNumFlags == 2 )
				{
					AddSubKeyNamed( pConditions, "if_hybrid_double" );
				}

				if ( bMVM )
				{
					AddSubKeyNamed( pConditions, "if_mvm" );
				}

				if ( bSpecialDeliveryMode )
				{
					AddSubKeyNamed( pConditions, "if_specialdelivery" );
				}
			}
		}
	}
	
	// load control settings...
	LoadControlSettings( "resource/UI/HudObjectiveFlagPanel.res", NULL, NULL, pConditions );

	m_pCarriedImage = dynamic_cast<ImagePanel *>( FindChildByName( "CarriedImage" ) );
	m_pPlayingTo = dynamic_cast<CExLabel *>( FindChildByName( "PlayingTo" ) );
	m_pPlayingToBG = FindChildByName( "PlayingToBG" );

	m_pCapturePoint = dynamic_cast<CTFArrowPanel *>( FindChildByName( "CaptureFlag" ) );

	m_pSpecCarriedImage = dynamic_cast<ImagePanel *>( FindChildByName( "SpecCarriedImage" ) );

	// outline is always on, so we need to init the alpha to 0
	vgui::Panel *pOutline = FindChildByName( "OutlineImage" );
	if ( pOutline )
	{
		pOutline->SetAlpha( 0 );
	}

	if ( pConditions )
	{
		pConditions->deleteThis();
	}

	UpdateStatus();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudFlagObjectives::Reset()
{
	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "FlagOutlineHide" );

	UpdateStatus();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudFlagObjectives::SetPlayingToLabelVisible( bool bVisible )
{
	if ( m_pPlayingTo && m_pPlayingToBG )
	{
		if ( m_pPlayingTo->IsVisible() != bVisible )
		{
			m_pPlayingTo->SetVisible( bVisible );
		}

		if ( m_pPlayingToBG->IsVisible() != bVisible )
		{
			m_pPlayingToBG->SetVisible( bVisible );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudFlagObjectives::OnTick()
{
	int nNumValidFlags = 0;

	// check that our blue panel still points at a valid flag
	if ( m_pBlueFlag && m_pBlueFlag->GetEntity() )
	{
		CCaptureFlag *pFlag = dynamic_cast< CCaptureFlag* >( m_pBlueFlag->GetEntity() );
		if ( !pFlag || pFlag->IsDisabled() )
		{
			m_pBlueFlag->SetEntity( NULL );
		}
	}

	// check that our red panel still points at a valid flag
	if ( m_pRedFlag && m_pRedFlag->GetEntity() )
	{
		CCaptureFlag *pFlag = dynamic_cast< CCaptureFlag* >( m_pRedFlag->GetEntity() );
		if ( !pFlag || pFlag->IsDisabled() )
		{
			m_pRedFlag->SetEntity( NULL );
		}
	}

	// iterate through the flags to set their position in our HUD
	for ( int i = 0; i<ICaptureFlagAutoList::AutoList().Count(); i++ )
	{
		CCaptureFlag *pFlag = static_cast< CCaptureFlag* >( ICaptureFlagAutoList::AutoList()[i] );

		if ( !pFlag->IsDisabled() || pFlag->IsVisibleWhenDisabled() )
		{
			if ( pFlag->GetTeamNumber() == TF_TEAM_RED )
			{
				if ( m_pRedFlag )
				{
					bool bNeedsUpdate = m_pRedFlag->GetEntity() != pFlag;
					m_pRedFlag->SetEntity( pFlag );
					if ( bNeedsUpdate )
					{
						UpdateStatus();
					}
				}
			}
			else if ( pFlag->GetTeamNumber() == TF_TEAM_BLUE )
			{
				if ( m_pBlueFlag )
				{
					bool bNeedsUpdate = m_pBlueFlag->GetEntity() != pFlag;
					m_pBlueFlag->SetEntity( pFlag );
					if ( bNeedsUpdate )
					{
						UpdateStatus();
					}
				}
			}
			else if ( pFlag->GetTeamNumber() == TEAM_UNASSIGNED )
			{
				if ( m_pBlueFlag && !m_pBlueFlag->GetEntity() )
				{
					m_pBlueFlag->SetEntity( pFlag );

					if ( !m_pBlueFlag->IsVisible() )
					{
						m_pBlueFlag->SetVisible( true );
					}

					if ( m_pRedFlag && m_pRedFlag->IsVisible()  )
					{
						m_pRedFlag->SetVisible( false );
					}
				}
				else if ( m_pRedFlag && !m_pRedFlag->GetEntity() )
				{
					// make sure both panels aren't pointing at the same entity
					if ( !m_pBlueFlag || ( pFlag != m_pBlueFlag->GetEntity() ) )
					{
						m_pRedFlag->SetEntity( pFlag );
							
						if ( !m_pRedFlag->IsVisible() )
						{
							m_pRedFlag->SetVisible( true );
						}
					}
				}
			}

			nNumValidFlags++;
		}
	}

	if ( m_nNumValidFlags != nNumValidFlags )
	{
		m_nNumValidFlags = nNumValidFlags;
		InvalidateLayout( false, true );
	}

	// are we playing captures for rounds?
	if ( !TFGameRules() || ( !TFGameRules()->IsPlayingHybrid_CTF_CP() && !TFGameRules()->IsPlayingSpecialDeliveryMode() && !TFGameRules()->IsMannVsMachineMode() ) )
	{
		if ( tf_flag_caps_per_round.GetInt() > 0 )
		{
			C_TFTeam *pTeam = GetGlobalTFTeam( TF_TEAM_BLUE );
			if ( pTeam )
			{
				SetDialogVariable( "bluescore", pTeam->GetFlagCaptures() );
			}

			pTeam = GetGlobalTFTeam( TF_TEAM_RED );
			if ( pTeam )
			{
				SetDialogVariable( "redscore", pTeam->GetFlagCaptures() );
			}

			SetPlayingToLabelVisible( true );
			SetDialogVariable( "rounds", tf_flag_caps_per_round.GetInt() );
		}
		else // we're just playing straight score
		{
			C_TFTeam *pTeam = GetGlobalTFTeam( TF_TEAM_BLUE );
			if ( pTeam )
			{
				SetDialogVariable( "bluescore", pTeam->Get_Score() );
			}

			pTeam = GetGlobalTFTeam( TF_TEAM_RED );
			if ( pTeam )
			{
				SetDialogVariable( "redscore", pTeam->Get_Score() );
			}

			SetPlayingToLabelVisible( false );
		}
	}

	// check the local player to see if they're spectating, OBS_MODE_IN_EYE, and the target entity is carrying the flag
	bool bSpecCarriedImage = false;
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pPlayer )
	{
		if ( pPlayer->GetObserverMode() == OBS_MODE_IN_EYE )
		{
			// does our target have the flag?
			C_BaseEntity *pEnt = pPlayer->GetObserverTarget();
			if ( pEnt && pEnt->IsPlayer() )
			{
				C_TFPlayer *pTarget = static_cast< C_TFPlayer* >( pEnt );
				if ( pTarget->HasTheFlag() )
				{
					bSpecCarriedImage = true;
					if ( pTarget->GetTeamNumber() == TF_TEAM_RED )
					{
						if ( m_pSpecCarriedImage )
						{
							m_pSpecCarriedImage->SetImage( "../hud/objectives_flagpanel_carried_blue" );
						}
					}
					else
					{
						if ( m_pSpecCarriedImage )
						{
							m_pSpecCarriedImage->SetImage( "../hud/objectives_flagpanel_carried_red" );
						}
					}
				}
			}
		}
	}

	if ( m_pSpecCarriedImage )
	{
		m_pSpecCarriedImage->SetVisible( bSpecCarriedImage );
	}

	if ( TFGameRules() )
	{
		if ( m_bPlayingHybrid_CTF_CP != TFGameRules()->IsPlayingHybrid_CTF_CP() )
		{
			m_bPlayingHybrid_CTF_CP = TFGameRules()->IsPlayingHybrid_CTF_CP();
			InvalidateLayout( false, true );
		}

		if ( m_bPlayingSpecialDeliveryMode != TFGameRules()->IsPlayingSpecialDeliveryMode() )
		{
			m_bPlayingSpecialDeliveryMode = TFGameRules()->IsPlayingSpecialDeliveryMode();
			InvalidateLayout( false, true );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudFlagObjectives::SetCarriedImage( const char *pchIcon )
{
	if ( m_pCarriedImage )
	{
		m_pCarriedImage->SetImage( pchIcon );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudFlagObjectives::UpdateStatus( C_BasePlayer *pNewOwner /*= NULL*/, C_BaseEntity *pFlagEntity /*= NULL*/ )
{
	C_TFPlayer *pLocalPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );

	// are we carrying a flag?
	CCaptureFlag *pPlayerFlag = NULL;
	if ( pLocalPlayer && pLocalPlayer->HasItem() && pLocalPlayer->GetItem()->GetItemID() == TF_ITEM_CAPTURE_FLAG )
	{
		if ( !pNewOwner || pNewOwner == pLocalPlayer )
		{
			pPlayerFlag = dynamic_cast< CCaptureFlag* >( pLocalPlayer->GetItem() );
		}
	}

	if ( !pPlayerFlag && pLocalPlayer && pLocalPlayer == pNewOwner )
	{
		pPlayerFlag = dynamic_cast< CCaptureFlag* >( pFlagEntity );
	}

	if ( pPlayerFlag )
	{
		m_bCarryingFlag = true;

		// make sure the panels are on, set the initial alpha values, 
		// set the color of the flag we're carrying, and start the animations
		if ( m_pBlueFlag && m_pBlueFlag->IsVisible() )
		{
			m_pBlueFlag->SetVisible( false );
		}

		if ( m_pRedFlag && m_pRedFlag->IsVisible() )
		{
			m_pRedFlag->SetVisible( false );
		}

		if ( m_pCarriedImage && !m_pCarriedImage->IsVisible() )
		{
			int nTeam;
			if ( pPlayerFlag->GetType() == TF_FLAGTYPE_ATTACK_DEFEND || 
				 pPlayerFlag->GetType() == TF_FLAGTYPE_TERRITORY_CONTROL || 
				 pPlayerFlag->GetType() == TF_FLAGTYPE_INVADE || 
				 pPlayerFlag->GetType() == TF_FLAGTYPE_RESOURCE_CONTROL )
			{
				nTeam = ( ( GetLocalPlayerTeam() == TF_TEAM_BLUE ) ? ( TF_TEAM_BLUE ) : ( TF_TEAM_RED ) );
			}
			else
			{
				// normal CTF behavior (carrying the enemy flag)
				nTeam = ( ( GetLocalPlayerTeam() == TF_TEAM_RED ) ? ( TF_TEAM_BLUE ) : ( TF_TEAM_RED ) );
			}


			char szImage[ MAX_PATH ];
			pPlayerFlag->GetHudIcon( nTeam, szImage, sizeof( szImage ) );

			SetCarriedImage( szImage );
			m_pCarriedImage->SetVisible( true );
		}

		if ( !m_bFlagAnimationPlayed )
		{
			m_bFlagAnimationPlayed = true;
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "FlagOutline" );
		}

		if ( m_pCapturePoint && !m_pCapturePoint->IsVisible() )
		{
			m_pCapturePoint->SetVisible( true );
		}

		if ( pLocalPlayer && m_pCapturePoint )
		{
			// go through all the capture zones and find ours
			for ( int i = 0; i<ICaptureZoneAutoList::AutoList().Count(); i++ )
			{
				C_CaptureZone *pCaptureZone = static_cast< C_CaptureZone* >( ICaptureZoneAutoList::AutoList()[i] );
				if ( !pCaptureZone->IsDormant() )
				{
					if ( pCaptureZone->GetTeamNumber() == pLocalPlayer->GetTeamNumber() && !pCaptureZone->IsDisabled() )
					{
						m_pCapturePoint->SetEntity( pCaptureZone );
					}
				}
			}
		}
	}
	else
	{
		// were we carrying the flag?
		if ( m_bCarryingFlag )
		{
			m_bCarryingFlag = false;
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "FlagOutline" );
		}

		m_bFlagAnimationPlayed = false;

		if ( m_pCarriedImage && m_pCarriedImage->IsVisible() )
		{
			m_pCarriedImage->SetVisible( false );
		}

		if ( m_pCapturePoint && m_pCapturePoint->IsVisible() )
		{
			m_pCapturePoint->SetVisible( false );
		}

		if ( m_pBlueFlag )
		{
			if ( m_pBlueFlag->GetEntity() != NULL )
			{
				if ( !m_pBlueFlag->IsVisible() )
				{
					m_pBlueFlag->SetVisible( true );
				}
				
				m_pBlueFlag->UpdateStatus();
			}
			else
			{
				if ( m_pBlueFlag->IsVisible() )
				{
					m_pBlueFlag->SetVisible( false );
				}
			}
		}

		if ( m_pRedFlag )
		{
			if ( m_pRedFlag->GetEntity() != NULL )
			{
				if ( !m_pRedFlag->IsVisible() )
				{
					m_pRedFlag->SetVisible( true );
				}

				m_pRedFlag->UpdateStatus();
			}
			else
			{
				if ( m_pRedFlag->IsVisible() )
				{
					m_pRedFlag->SetVisible( false );
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudFlagObjectives::FireGameEvent( IGameEvent *event )
{
	const char *eventName = event->GetName();

	if ( FStrEq( eventName, "flagstatus_update" ) )
	{
		int nVictimID = event->GetInt( "userid" );
		C_BasePlayer *pNewOwner = USERID2PLAYER( nVictimID );

		int nFlagEntIndex = event->GetInt( "entindex" );
		C_BaseEntity *pFlagEntity = ClientEntityList().GetEnt( nFlagEntIndex );

		UpdateStatus( pNewOwner, pFlagEntity );
	}
}

