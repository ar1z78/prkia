#include "StdAfx.h"
#include "FindPanel.h"
#include "InventoryView.h"
#include "StringUtil.h"

using namespace aoia;

FindView::FindView(sqlite::IDBPtr db, aoia::ISettingsPtr settings)
    : m_db(db)
    , m_settings(settings)
    , m_lastQueryChar(-1)
    , m_lastQueryQlMin(-1)
    , m_lastQueryQlMax(-1)
    , m_pParent(NULL) {}


void FindView::SetParent(InventoryView* parent)
{
    m_pParent = parent;
}


LRESULT FindView::OnInitDialog(UINT/*uMsg*/, WPARAM/*wParam*/, LPARAM/*lParam*/, BOOL&/*bHandled*/)
{
    SetWindowText(_T("Find View"));
    updateCharList();
    DlgResize_Init(false, true, WS_CLIPCHILDREN);
    return 0;
}


LRESULT FindView::OnForwardMsg(UINT/*uMsg*/, WPARAM/*wParam*/, LPARAM lParam, BOOL&/*bHandled*/)
{
    LPMSG pMsg = (LPMSG)lParam;
    return this->PreTranslateMsg(pMsg);
}


BOOL FindView::PreTranslateMsg(MSG* pMsg)
{
    return IsDialogMessage(pMsg);
}


LRESULT FindView::OnEnChangeItemtext(WORD/*wNotifyCode*/, WORD/*wID*/, HWND/*hWndCtl*/, BOOL&/*bHandled*/)
{
    SetTimer(1, 1500);
    return 0;
}


LRESULT FindView::OnCbnSelChangeCharcombo(WORD/*wNotifyCode*/, WORD/*wID*/, HWND/*hWndCtl*/, BOOL&/*bHandled*/)
{
    UpdateFindQuery();
    return 0;
}


LRESULT FindView::OnCbnDropdown(WORD/*wNotifyCode*/, WORD/*wID*/, HWND/*hWndCtl*/, BOOL&/*bHandled*/)
{
    CComboBox toon_combo = GetDlgItem(IDC_CHARCOMBO);
    unsigned int char_id = toon_combo.GetItemData(toon_combo.GetCurSel());

    updateCharList();

    bool found = false;
    for (int i = 0; i < toon_combo.GetCount(); ++i)
    {
        unsigned int data = toon_combo.GetItemData(i);
        if (data == char_id)
        {
            toon_combo.SetCurSel(i);
            found = true;
            break;
        }
    }

    if (!found)
    {
        toon_combo.SetCurSel(0);
    }
    return 0;
}


LRESULT FindView::OnTimer(UINT wParam)
{
    if (wParam == 1)
    {
        UpdateFindQuery();
        KillTimer(1);
    }
    return 0;
}


void FindView::UpdateFindQuery()
{
    KillTimer(1);

    CComboBox cb = GetDlgItem(IDC_CHARCOMBO);
    CEdit eb = GetDlgItem(IDC_ITEMTEXT);
    CEdit qlmin = GetDlgItem(IDC_QLMIN);
    CEdit qlmax = GetDlgItem(IDC_QLMAX);

    unsigned int charid = 0;
    int item = cb.GetCurSel();
    if (item != CB_ERR)
    {
        charid = (unsigned int)cb.GetItemData(item);
    }

    TCHAR buffer[MAX_PATH];
    eb.GetWindowText(buffer, MAX_PATH);
    std::tstring text(buffer);
    StringUtil::trim_ao(text);

    int minql = -1;
    eb = qlmin; // reuse eb for convenience
    eb.GetWindowText(buffer, MAX_PATH);
    std::tstring qlminText(buffer);
    if (!qlminText.empty())
    {
        int val = _ttoi(qlminText.c_str());
        if (val != 0 || qlminText == _T("0")) { minql = val; }
    }

    int maxql = -1;
    eb = qlmax;
    eb.GetWindowText(buffer, MAX_PATH);
    std::tstring qlmaxText(buffer);
    if (!qlmaxText.empty())
    {
        int val = _ttoi(qlmaxText.c_str());
		// If it's a valid number (or explicitly "0"), update maxql.
		// Otherwise, it stays at the default value.
        if (val != 0 || qlmaxText == _T("0")) { maxql = val; }
    }

    // REMOVED 'text.size() > 2' so clearing the box triggers an update
    if (m_lastQueryText != text
        || m_lastQueryChar != charid
        || m_lastQueryQlMin != minql
        || m_lastQueryQlMax != maxql)
    {
        m_lastQueryText = text;
        m_lastQueryChar = charid;
        m_lastQueryQlMin = minql;
        m_lastQueryQlMax = maxql;

        std::vector<std::tstring> clauses;

        if (charid > 0) {
            clauses.push_back(STREAM2STR(_T("I.owner = ") << charid));
        }
        if (minql > -1) {
            clauses.push_back(STREAM2STR(_T("I.ql >= ") << minql));
        }
        if (maxql > -1) {
            clauses.push_back(STREAM2STR(_T("I.ql <= ") << maxql));
        }
        if (!text.empty()) {
            clauses.push_back(STREAM2STR(_T("keylow IN (SELECT aoid FROM aodb.tblAO WHERE name LIKE \"%") << text << _T("%\")")));
        }

        std::tstringstream sql;
        for (size_t i = 0; i < clauses.size(); ++i) {
            if (i > 0) sql << _T(" AND ");
            sql << clauses[i];
        }

        // If sql is empty, it resets the list to 'everything'
        m_pParent->UpdateListView(sql.str());
    }
}



void FindView::updateCharList()
{
    CComboBox cb = GetDlgItem(IDC_CHARCOMBO);

    cb.ResetContent();
    int item = cb.AddString(_T("-"));
    cb.SetItemData(item, 0);

    g_DBManager.Lock();
    sqlite::ITablePtr pT = m_db->ExecTable("SELECT DISTINCT charid FROM tToons T ORDER BY charname");
    g_DBManager.UnLock();

    if (pT != NULL)
    {
		for (unsigned int i = 0; i < pT->Rows(); i++)
		{
			std::string rowData = pT->Data(i, 0);
			
			// Check if the data is valid before converting
			if (rowData.empty()) {
				continue;
			}

			// _ttoi is safe and returns 0 on failure. 
			// Cast the char* data directly.
			unsigned int id = (unsigned int)atoi(rowData.c_str());

			if (id == 0 && rowData != "0") {
				// This is here because of a wierd but that appears to be SQLite's fault.
				LOG("Error in updateCharList(). Bad numeric data at row " << i << ".");
				continue;
			}

			g_DBManager.Lock();
			std::tstring name = g_DBManager.GetToonName(id);
			g_DBManager.UnLock();

			if (name.empty())
			{
				name = from_ascii_copy(rowData);
			}

			if ((item = cb.AddString(name.c_str())) != CB_ERR)
			{
				cb.SetItemData(item, id);
			}
		}

    }
}
