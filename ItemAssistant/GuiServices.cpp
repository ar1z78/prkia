#include "StdAfx.h"
#include "GuiServices.h"
#include "Version.h"
#include "ntray.h"


namespace aoia
{
    GuiServices::GuiServices(std::shared_ptr<CTrayNotifyIcon> trayIcon)
        : m_trayIcon(trayIcon) {}


    GuiServices::~GuiServices() {}

    void GuiServices::ShowTrayIconBalloon(std::tstring const& message) const
    {
        m_trayIcon->SetBalloonDetails(message.c_str(), _T("PRK Item Assistant++"), CTrayNotifyIcon::Info, 5000);
    }


	void GuiServices::ShowHelp(std::tstring const& topic) const
	{
		// 1. Find any existing HH window and close it
		HWND hWndExisting = FindWindow(_T("HH Parent"), NULL);
		if (hWndExisting) {
			PostMessage(hWndExisting, WM_CLOSE, 0, 0);
		}

		// 2. Get the path
		TCHAR szPath[MAX_PATH];
		GetModuleFileName(NULL, szPath, MAX_PATH);
		PathRemoveFileSpec(szPath);
		PathAppend(szPath, _T("help\\help.html"));

		// 3. Build the parameters
		std::tstringstream params;
		params << _T("\"") << szPath;
		if (!topic.empty()) {
			params << _T("#") << topic;
		}

		// 4. Open fresh instance
		ShellExecute(NULL, _T("open"), _T("hh.exe"), params.str().c_str(), NULL, SW_NORMAL);
	}



    void GuiServices::OpenURL(std::tstring const& url) const
    {
        ShellExecute(NULL, _T("open"), url.c_str(), NULL, NULL, SW_NORMAL);
    }

}
