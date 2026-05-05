#ifndef TABFRAME_H
#define TABFRAME_H

#include <PluginSDK/IPluginView.h>
#include <shared/Signals.h>
#include "InventoryView.h"
#include "PatternMatchView.h"
//#include "PlayershopView.h"
#include "IdentifyView.h"
//#include "RecipesView.h"
#include "SummaryView.h"
#include <vector>
#include <memory>

#ifdef _DEBUG
#include "AoMsgView.h"
#endif

// Structure to hold tab information
struct TabItem
{
    HWND hWnd;
    std::tstring title;
    aoia::IPluginView* pView;
};

class TabFrame
    : public CWindowImpl<TabFrame>
{
public:
    DECLARE_WND_CLASS(_T("TabFrame"))

    TabFrame(sqlite::IDBPtr db, aoia::IContainerManagerPtr containerManager, aoia::IGuiServicesPtr gui, aoia::ISettingsPtr settings);
    virtual ~TabFrame();

    BEGIN_MSG_MAP(TabFrame)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        NOTIFY_CODE_HANDLER(TCN_SELCHANGE, OnSelChange)
        CHAIN_MSG_MAP(CWindowImpl<TabFrame>)
    END_MSG_MAP()

    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSelChange(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

    aoia::IPluginView* GetActivePluginView();
    void OnAOServerMessage(Parsers::AOMessageBase &msg);
    void OnAOClientMessage(Parsers::AOClientMessageBase &msg);
    void SetToolBarPanel(HWND panel) { m_rebarControl.Attach(panel); }
    void SetStatusBar(HWND statusbar) { m_statusBar.Attach(statusbar); }

    /// Sets the state of the toolbar for the active view.
    void SetToolbarVisibility(bool visible);

    // Tab management methods
    void AddTab(HWND hWnd, const std::tstring& title, aoia::IPluginView* pView);
    void DisplayTab(HWND hWnd);
    HWND GetActiveView() const;
    WTL::CTabCtrl& GetTabCtrl() { return m_tabCtrl; }

protected:
    void onStatusChanged();
    void ResizeTabContent();

private:
    std::vector<TabItem> m_tabs;
    int m_activeTabIndex;

    std::vector<aoia::IPluginView*> m_viewPlugins;

    InventoryView       m_inventoryView;
#ifdef _DEBUG
    AoMsgView           m_msgView;
#endif
    PatternMatchView    m_patternView;
//	RecipesView			m_recipesView;
//    PlayershopView      m_playershopView;
    IdentifyView        m_identifyView;
    aoia::sv::SummaryView m_summaryView;

    WTL::CTabCtrl       m_tabCtrl;
    WTL::CReBarCtrl     m_rebarControl;
    WTL::CToolBarCtrl   m_activeViewToolbar;
    WTL::CStatusBarCtrl m_statusBar;

    bool m_toobarVisibility;

	aoia::SignalConnection m_statusTextSignalConnection;
};

#endif // TABFRAME_H
