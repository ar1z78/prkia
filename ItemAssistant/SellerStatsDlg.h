#pragma once
#include "resource.h"
#include <iomanip>
#include "AOManager.h"

class CSellerStatsDlg : public CDialogImpl<CSellerStatsDlg> {
public:
    enum { IDD = IDD_SELLER_STATS };

    int m_cl;
    bool m_omni, m_trader;
    // New variables to hold the settings
    std::tstring m_colorLow, m_colorMed, m_colorHigh;
    int m_priceMax;
	aoia::ISettingsPtr m_settings;



    BEGIN_MSG_MAP(CSellerStatsDlg)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_ID_HANDLER(IDOK, OnOk)
        COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
        // Handle color preview updates
        COMMAND_RANGE_HANDLER(IDC_PRICE_COLOR_LOW, IDC_PRICE_COLOR_HIGH, OnColorChange)
        MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
    END_MSG_MAP()

    LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL&) {
		
		SellerSettings s = AOManager::instance().getSellerSettings();
		
        SetDlgItemInt(IDC_CL_SKILL, m_cl);
        CheckDlgButton(IDC_OMNI, m_omni ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(IDC_TRADER, m_trader ? BST_CHECKED : BST_UNCHECKED);
        
        // Load existing hex values and max price
        SetDlgItemText(IDC_PRICE_COLOR_LOW, m_colorLow.c_str());
        SetDlgItemText(IDC_PRICE_COLOR_MED, m_colorMed.c_str());
        SetDlgItemText(IDC_PRICE_COLOR_HIGH, m_colorHigh.c_str());
        SetDlgItemInt(IDC_PRICE_MAX_HIGH, m_priceMax);

        CenterWindow(GetParent());
        return TRUE;
    }

    LRESULT OnColorChange(WORD, WORD, HWND, BOOL&) {
        // Redraw the swatches when text changes
        Invalidate(); 
        return 0;
    }

    LRESULT OnDrawItem(UINT, WPARAM, LPARAM lParam, BOOL& bHandled) {
        LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
        TCHAR buf[16];
        UINT editID = 0;

        // Map the static swatch ID to the corresponding edit box ID
        if (lpdis->CtlID == IDC_SAMPLE_LOW) editID = IDC_PRICE_COLOR_LOW;
        else if (lpdis->CtlID == IDC_SAMPLE_MED) editID = IDC_PRICE_COLOR_MED;
        else if (lpdis->CtlID == IDC_SAMPLE_HIGH) editID = IDC_PRICE_COLOR_HIGH;

        if (editID != 0) {
            GetDlgItemText(editID, buf, 16);
            COLORREF col = ParseHex(buf);
            WTL::CDCHandle dc(lpdis->hDC);
            dc.FillSolidRect(&lpdis->rcItem, col);
            dc.FrameRect(&lpdis->rcItem, (HBRUSH)GetStockObject(BLACK_BRUSH));
            return TRUE;
        }
        bHandled = FALSE;
        return 0;
    }

	LRESULT OnOk(WORD, WORD, HWND, BOOL&) {
		// 1. Create the struct object 's'
		SellerSettings s;

		// 2. Fill the struct with values from the dialog controls
		s.clSkill = GetDlgItemInt(IDC_CL_SKILL);
		s.isOmni = IsDlgButtonChecked(IDC_OMNI) == BST_CHECKED;
		s.isTrader = IsDlgButtonChecked(IDC_TRADER) == BST_CHECKED;
		s.priceMax = GetDlgItemInt(IDC_PRICE_MAX_HIGH);

		TCHAR buf[16];
		GetDlgItemText(IDC_PRICE_COLOR_LOW, buf, 16);  s.colorLow = buf;
		GetDlgItemText(IDC_PRICE_COLOR_MED, buf, 16);  s.colorMed = buf;
		GetDlgItemText(IDC_PRICE_COLOR_HIGH, buf, 16); s.colorHigh = buf;

		// 3. Now 's' is defined, so we can save it
		AOManager::instance().setSellerSettings(s);

		// Update local variables for the MessageBox logic
		m_cl = s.clSkill;

		// Breakpoint logic...
		int remainder = m_cl % 40;
		if (remainder != 0) {
			if (m_cl >= 680) {
				MessageBox(_T("Your seller already reached max CL skill (680) for selling items."), _T("Max CL skill"), MB_OK | MB_ICONINFORMATION);
			}
			else {
				int needed = 40 - remainder;
				std::tstringstream ss;
				ss << _T("Your seller needs ") << needed << _T(" more CL skill for next breakpoint.");
				MessageBox(ss.str().c_str(), _T("CL Breakpoint"), MB_OK | MB_ICONINFORMATION);
			}
		}
		::SendMessage(GetTopLevelWindow(), WM_SETTINGCHANGE, 0, 0);
		EndDialog(IDOK);
		return 0;
	}


    LRESULT OnCancel(WORD, WORD, HWND, BOOL&) { EndDialog(IDCANCEL); return 0; }

private:
    COLORREF ParseHex(const TCHAR* sz) {
        TCHAR* end;
        unsigned long val = _tcstoul(sz, &end, 16);
        // Swap RGB to BGR for Windows COLORREF
        return RGB((val >> 16) & 0xFF, (val >> 8) & 0xFF, val & 0xFF);
    }
};
