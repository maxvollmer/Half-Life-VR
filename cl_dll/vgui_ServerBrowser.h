//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#ifndef ServerBrowser_H
#define ServerBrowser_H

#include <VGUI_Panel.h>

namespace vgui
{
	class Button;
	class TablePanel;
	class HeaderPanel;
}  // namespace vgui

class CTransparentPanel;
class CommandButton;

// Scoreboard positions
#define SB_X_INDENT (20 * ((float)ScreenHeight / 640))
#define SB_Y_INDENT (20 * ((float)ScreenHeight / 480))

class ServerBrowser : public CTransparentPanel
{
private:
	HeaderPanel* _headerPanel = nullptr;
	TablePanel* _tablePanel = nullptr;

	CommandButton* _connectButton = nullptr;
	CommandButton* _refreshButton = nullptr;
	CommandButton* _broadcastRefreshButton = nullptr;
	CommandButton* _stopButton = nullptr;
	CommandButton* _sortButton = nullptr;
	CommandButton* _cancelButton = nullptr;

	CommandButton* _pingButton = nullptr;

public:
	ServerBrowser(int x, int y, int wide, int tall);

public:
	virtual void setSize(int wide, int tall);
};



#endif
