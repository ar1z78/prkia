#include "StdAfx.h"
#include "ServerSelectorDialog.h"
#include "namehelper.h"

ServerSelectorDialog::ServerSelectorDialog()
    : m_choice(1)
{
}


LRESULT ServerSelectorDialog::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    CenterWindow(GetParent());
	// Update radio button labels from settings
	SetDlgItemText(IDC_AUNO, aoia::GetServerName(1).c_str());
	SetDlgItemText(IDC_JAYDEE, aoia::GetServerName(2).c_str());
	SetDlgItemText(IDC_XYPHOS, aoia::GetServerName(3).c_str());
    CheckDlgButton(IDC_AUNO, 1);
    return TRUE;
}


LRESULT ServerSelectorDialog::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    if (IsDlgButtonChecked(IDC_AUNO))
    {
        m_choice = 1;
    }
    else if (IsDlgButtonChecked(IDC_JAYDEE))
    {
        m_choice = 2;
    }
    else if (IsDlgButtonChecked(IDC_XYPHOS))
    {
        m_choice = 3;
    }
    else 
    {
        m_choice = 0;
    }

    EndDialog(wID);
    return 0;
}


unsigned int ServerSelectorDialog::getChoice() const
{
    return m_choice;
}