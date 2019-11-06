//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

// vgui_SpectatorPanel.h: interface for the SpectatorPanel class.
//
//////////////////////////////////////////////////////////////////////

#ifndef SPECTATORPANEL_H
#define SPECTATORPANEL_H

#include <VGUI_Panel.h>
#include <VGUI_Label.h>
#include <VGUI_Button.h>

using namespace vgui;

#define SPECTATOR_PANEL_CMD_NONE 0

#define SPECTATOR_PANEL_CMD_OPTIONS      1
#define SPECTATOR_PANEL_CMD_PREVPLAYER   2
#define SPECTATOR_PANEL_CMD_NEXTPLAYER   3
#define SPECTATOR_PANEL_CMD_HIDEMENU     4
#define SPECTATOR_PANEL_CMD_TOGGLE_INSET 5
#define SPECTATOR_PANEL_CMD_CAMERA       6

#define TEAM_NUMBER 2

class SpectatorPanel : public Panel  //, public vgui::CDefaultInputSignal
{
public:
	SpectatorPanel(int x, int y, int wide, int tall);
	virtual ~SpectatorPanel();

	void ActionSignal(int cmd);

	// InputSignal overrides.
public:
	void Initialize();
	void Update();



public:
	void EnableInsetView(bool isEnabled);
	void ShowMenu(bool isVisible);


	ColorButton* m_OptionButton = nullptr;
	//	CommandButton     *	m_HideButton = nullptr;
	ColorButton* m_PrevPlayerButton = nullptr;
	ColorButton* m_NextPlayerButton = nullptr;
	ColorButton* m_CamButton = nullptr;

	CTransparentPanel* m_TopBorder = nullptr;
	CTransparentPanel* m_BottomBorder = nullptr;

	ColorButton* m_InsetViewButton = nullptr;

	Label* m_BottomMainLabel = nullptr;
	CImageLabel* m_TimerImage = nullptr;
	Label* m_CurrentTime = nullptr;
	Label* m_ExtraInfo = nullptr;
	Panel* m_Separator = nullptr;

	Label* m_TeamScores[TEAM_NUMBER] = { 0 };

	CImageLabel* m_TopBanner = nullptr;

	bool m_menuVisible = false;
	bool m_insetVisible = false;
};



class CSpectatorHandler_Command : public ActionSignal
{
private:
	SpectatorPanel* m_pFather = nullptr;
	int m_cmd = 0;

public:
	CSpectatorHandler_Command(SpectatorPanel* panel, int cmd)
	{
		m_pFather = panel;
		m_cmd = cmd;
	}

	virtual void actionPerformed(Panel* panel)
	{
		m_pFather->ActionSignal(m_cmd);
	}
};


#endif  // !defined SPECTATORPANEL_H
