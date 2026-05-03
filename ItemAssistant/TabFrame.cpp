#include "StdAfx.h"
#include "Tabframe.h"

using namespace aoia;
using namespace Parsers;


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
    m_tabCtrl.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | TCS_TABS | TCS_MULTILINE);

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
    if (m_tabCtrl.IsWindow() && m_activeTabIndex >= 0)
    {
        RECT rcTab;
        m_tabCtrl.GetClientRect(&rcTab);
        
        // Adjust for tab item area
        m_tabCtrl.AdjustRect(FALSE, &rcTab);

        // Resize active tab window
        if (m_activeTabIndex < (int)m_tabs.size())
        {
            ::SetWindowPos(m_tabs[m_activeTabIndex].hWnd, NULL, 
                rcTab.left, rcTab.top, 
                rcTab.right - rcTab.left, 
                rcTab.bottom - rcTab.top, 
                SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }
}


LRESULT TabFrame::OnSelChange(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
{
    int nTab = m_tabCtrl.GetCurSel();
    if (nTab >= 0 && nTab < (int)m_tabs.size())
    {
        DisplayTab(m_tabs[nTab].hWnd);
    }

    bHandled = TRUE;
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

        // Assign new toolbar
        m_activeViewToolbar.Detach();
        m_activeViewToolbar.Attach(newplugin->GetToolbar());

        int nBandIndex = m_rebarControl.IdToIndex(ATL_IDW_BAND_FIRST + 1);  // toolbar is 2nd added band
        if (nBandIndex < 0)
        {
            // Insert new band
            WTL::CFrameWindowImplBase<>::AddSimpleReBarBandCtrl(m_rebarControl, m_activeViewToolbar, ATL_IDW_BAND_FIRST + 1, NULL, TRUE);
            m_rebarControl.ShowBand(m_rebarControl.IdToIndex(ATL_IDW_BAND_FIRST + 1), m_toobarVisibility);
            m_rebarControl.LockBands(true);
        }
        else
        {
            // Replace band
            REBARBANDINFO rbbi;
            ZeroMemory(&rbbi, sizeof(REBARBANDINFO));
            rbbi.cbSize = sizeof(REBARBANDINFO);
            rbbi.fMask = RBBIM_CHILD;
            rbbi.hwndChild = m_activeViewToolbar.m_hWnd;

            m_rebarControl.SetBandInfo(nBandIndex, &rbbi);
        }

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


void TabFrame::SetToolbarVisibility(bool visible)
{
    m_toobarVisibility = visible;
    m_rebarControl.ShowBand(m_rebarControl.IdToIndex(ATL_IDW_BAND_FIRST + 1), m_toobarVisibility);
}
