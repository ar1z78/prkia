#include "StdAfx.h"
#include "Tabframe.h"

using namespace aoia;
using namespace Parsers;

static BOOL CALLBACK SetChildFont(HWND hWnd, LPARAM lParam) {
	::SendMessage(hWnd, WM_SETFONT, (WPARAM)lParam, TRUE);
	return TRUE;
}

TabFrame::TabFrame(sqlite::IDBPtr db, aoia::IContainerManagerPtr containerManager, aoia::IGuiServicesPtr gui, aoia::ISettingsPtr settings)
  : m_toobarVisibility(true)
  , m_activeTabIndex(-1)
  , m_inventoryView(db, containerManager, gui, settings)
  , m_summaryView(db, gui)
//  , m_playershopView(gui)
  , m_patternView(db, containerManager, gui, settings)
//  , m_recipesView(db, containerManager, gui, settings)
  , m_identifyView(db, containerManager, gui)
{
}


TabFrame::~TabFrame()
{
}


LRESULT TabFrame::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    RECT defRect = { 0, 0, 640, 480 };

    // Create the tab control
	// Remove WS_CLIPCHILDREN from the tab control creation
	m_tabCtrl.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TCS_TABS | TCS_MULTILINE);
	ModifyStyle(0, WS_CLIPCHILDREN);

    // Create each tab view
    m_summaryView.Create(*this, defRect, 0, WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
    AddTab(m_summaryView.m_hWnd, _T("Summary"), &m_summaryView);
    m_viewPlugins.push_back(&m_summaryView);

    m_inventoryView.Create(*this, defRect, 0, WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
    AddTab(m_inventoryView.m_hWnd, _T("Inventory"), &m_inventoryView);
    m_viewPlugins.push_back(&m_inventoryView);

    m_patternView.Create(*this, defRect, 0, WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
    AddTab(m_patternView.m_hWnd, _T("Pattern Matcher"), &m_patternView);
    m_viewPlugins.push_back(&m_patternView);

/*
    m_recipesView.Create(*this, defRect, 0, WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
    AddTab(m_recipesView.m_hWnd, _T("Recipes"), &m_recipesView);
    m_viewPlugins.push_back(&m_recipesView);

    m_playershopView.Create(*this, defRect, 0, WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
    AddTab(m_playershopView.m_hWnd, _T("Playershop Monitor"), &m_playershopView);
    m_viewPlugins.push_back(&m_playershopView);
    m_playershopView.StartMonitoring();
*/
    m_identifyView.Create(*this, defRect, 0, WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
    AddTab(m_identifyView.m_hWnd, _T("Identify"), &m_identifyView);
    m_viewPlugins.push_back(&m_identifyView);

#ifdef _DEBUG
    m_msgView.Create(*this, defRect, 0, WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
    AddTab(m_msgView.m_hWnd, _T("Messages (Debug)"), &m_msgView);
    m_viewPlugins.push_back(&m_msgView);
#endif

    // Display first tab
    DisplayTab(m_summaryView.m_hWnd);

	// Get the standard Windows UI font handle
	HFONT hGuiFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

	// Apply it to the TabFrame and all its children (tabs, buttons, etc.)
	::SendMessage(m_hWnd, WM_SETFONT, (WPARAM)hGuiFont, TRUE);
	EnumChildWindows(m_hWnd, SetChildFont, (LPARAM)hGuiFont);
    bHandled = TRUE;
    return 0;
}


LRESULT TabFrame::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    int cx = GET_X_LPARAM(lParam);
    int cy = GET_Y_LPARAM(lParam);

    // Resize tab control to fill the parent window
    if (m_tabCtrl.IsWindow())
    {
        m_tabCtrl.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
        ResizeTabContent();
    }

    bHandled = TRUE;
    return 0;
}


void TabFrame::ResizeTabContent()
{
	if (!m_tabCtrl.IsWindow()) return;

	RECT rect;
	m_tabCtrl.GetWindowRect(&rect); // Get screen coordinates
	ScreenToClient(&rect);         // Convert to TabFrame (parent) coordinates

	// AdjustRect shrinks the rect to the area below the tab buttons
	m_tabCtrl.AdjustRect(FALSE, &rect);

	int nSel = m_tabCtrl.GetCurSel();
	if (nSel != -1 && nSel < (int)m_tabs.size()) {
		// Move the child view to the adjusted position
		::MoveWindow(m_tabs[nSel].hWnd, rect.left, rect.top,
			rect.right - rect.left, rect.bottom - rect.top, TRUE);

		// Ensure the active window is at the top of the Z-order
		::SetWindowPos(m_tabs[nSel].hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	}
}




LRESULT TabFrame::OnSelChange(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
	int nSel = m_tabCtrl.GetCurSel();

	// 1. Hide all tab windows first
	for (auto& tab : m_tabs) {
		::ShowWindow(tab.hWnd, SW_HIDE);
	}

	// 2. Show only the active one and resize it to fit
	if (nSel != -1) {
		::ShowWindow(m_tabs[nSel].hWnd, SW_SHOW);
		ResizeTabContent(); // Ensure it fills the tab area
	}

	return 0;
}



void TabFrame::AddTab(HWND hWnd, const std::tstring& title, aoia::IPluginView* pView)
{
    TabItem item;
    item.hWnd = hWnd;
    item.title = title;
    item.pView = pView;

    m_tabs.push_back(item);

    // Add tab to control
    TCITEM tie;
    tie.mask = TCIF_TEXT;
    tie.pszText = (LPTSTR)title.c_str();
    tie.cchTextMax = (int)title.length();

    m_tabCtrl.InsertItem(m_tabs.size() - 1, &tie);
}


void TabFrame::DisplayTab(HWND hWnd)
{
    IPluginView* oldplugin = GetActivePluginView();
    
    int nTab = -1;
    for (size_t i = 0; i < m_tabs.size(); i++)
    {
        if (m_tabs[i].hWnd == hWnd)
        {
            nTab = (int)i;
            break;
        }
    }

    if (nTab < 0)
        return;

    m_activeTabIndex = nTab;
    m_tabCtrl.SetCurSel(nTab);

    IPluginView* newplugin = GetActivePluginView();

    if (oldplugin)
    {
        oldplugin->disconnect(m_statusTextSignalConnection);
        oldplugin->OnActive(false);
        ::ShowWindow(oldplugin->GetWindow(), SW_HIDE);
    }

    if (newplugin)
    {
        newplugin->OnActive(true);
        ::ShowWindow(newplugin->GetWindow(), SW_SHOW);

        // Update statusbar
        m_statusBar.SetText(0, newplugin->GetStatusText().c_str());
        m_statusTextSignalConnection = newplugin->connectStatusChanged([this]() { 
            this->onStatusChanged(); 
        });
    }

    ResizeTabContent();
}


HWND TabFrame::GetActiveView() const
{
    if (m_activeTabIndex >= 0 && m_activeTabIndex < (int)m_tabs.size())
    {
        return m_tabs[m_activeTabIndex].hWnd;
    }
    return NULL;
}


IPluginView* TabFrame::GetActivePluginView()
{
    if (m_activeTabIndex >= 0 && m_activeTabIndex < (int)m_tabs.size())
    {
        return m_tabs[m_activeTabIndex].pView;
    }
    return NULL;
}


void TabFrame::onStatusChanged()
{
    IPluginView* plugin = GetActivePluginView();
    if (plugin)
    {
        m_statusBar.SetText(0, plugin->GetStatusText().c_str());
    }
}


void TabFrame::OnAOServerMessage(AOMessageBase &msg)
{
    IPluginView* active = GetActivePluginView();
    if (active)
    {
        active->OnAOServerMessage(msg);
    }
    
    for (unsigned int i = 0; i < m_viewPlugins.size(); i++)
    {
        if (m_viewPlugins[i] != active)
        {
            m_viewPlugins[i]->OnAOServerMessage(msg);
        }
    }
}


void TabFrame::OnAOClientMessage(AOClientMessageBase &msg)
{
    for (unsigned int i = 0; i < m_viewPlugins.size(); i++)
    {
        m_viewPlugins[i]->OnAOClientMessage(msg);
    }
}


BOOL TabFrame::PreTranslateMsg(MSG* pMsg)
{
	int nSel = m_tabCtrl.GetCurSel();
	if (nSel != -1 && nSel < (int)m_tabs.size())
	{
		// This is the CRITICAL hand-off for keyboard shortcuts
		if (m_tabs[nSel].pView && m_tabs[nSel].pView->PreTranslateMsg(pMsg))
			return TRUE;
	}
	return FALSE;
}



