//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_HUD_OBJECTIVESTATUS_H
#define TF_HUD_OBJECTIVESTATUS_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_controls.h"
#include "tf_imagepanel.h"
#include "tf_hud_flagstatus.h"
#include "tf_hud_escort.h"
#include "tf_time_panel.h"
#include "hud_controlpointicons.h"
#include "GameEventListener.h"

#define MAX_BOSS_STUN_SKILL_SHOTS 3

//-----------------------------------------------------------------------------
// Purpose:  Parent panel for the various objective displays
//-----------------------------------------------------------------------------
class CTFHudObjectiveStatus : public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTFHudObjectiveStatus, vgui::EditablePanel );

public:
	CTFHudObjectiveStatus( const char *pElementName );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void Reset();
	virtual void Think();
	virtual bool ShouldDraw() OVERRIDE;

	virtual int GetRenderGroupPriority( void ) { return 60; }	// higher than build menus

	CControlPointProgressBar *GetControlPointProgressBar( void );

private:

	void	SetVisiblePanels( void );

private:

	float					m_flNextThink;

	CTFHudFlagObjectives	*m_pFlagPanel;
	CTFHudTimeStatus* m_pTimePanel;

	CHudControlPointIcons	*m_pControlPointIconsPanel;
	CControlPointProgressBar *m_pControlPointProgressBar;
	CTFHudEscort			*m_pEscortPanel;
	CTFHudMultipleEscort	*m_pMultipleEscortPanel;
};

#endif	// TF_HUD_OBJECTIVESTATUS_H
