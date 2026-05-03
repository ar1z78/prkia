#include "StdAfx.h"
#include "resource.h"
#include "ProgressDialog.h"

CProgressDialog::CProgressDialog(unsigned int overallMax, unsigned taskMax)
    : m_overallMax(overallMax)
    , m_overallCurrent(0)
    , m_taskMax(taskMax)
    , m_taskCurrent(0)
    , m_isCanceled(false)
    , m_syncCount(2) // Equivalent to m_barrier(2)
{
    m_lines.resize(3);
    Begin();    // Launch thread.

    // Manual Barrier Wait
    std::unique_lock<std::mutex> lock(m_syncMutex);
    if (--m_syncCount > 0) {
        m_syncCond.wait(lock, [this] { return m_syncCount == 0; });
    } else {
        m_syncCond.notify_all();
    }
}

CProgressDialog::~CProgressDialog()
{
    End();
}

void CProgressDialog::setText(unsigned int line, std::tstring const& text)
{
    unsigned int ctrlId = 0;
    switch(line)
    {
        case 0: ctrlId = IDC_DESCRIPTION1; break;
        case 1: ctrlId = IDC_DESCRIPTION2; break;
        case 2: ctrlId = IDC_DESCRIPTION3; break;
        default: assert(false); return; // Unknown line
    }

    m_lines[line] = text;
    if (::IsWindow(m_hWnd)) {
        SetDlgItemText(ctrlId, text.c_str());
    }
}

void CProgressDialog::setTaskProgress(unsigned int completed, unsigned int max)
{
    if (max != 0) { m_taskMax = max; }

    CProgressBarCtrl progressCtrl = GetDlgItem(IDC_PROGRESS1);
    if (::IsWindow(progressCtrl.m_hWnd)) {
        progressCtrl.SetRange(0, 10000);
        progressCtrl.SetPos(completed * 10000 / (m_taskMax > 0 ? m_taskMax : 1));
    }
}

void CProgressDialog::setOverallProgress(unsigned int completed, unsigned int max)
{
    if (max != 0) { m_overallMax = max; }

    CProgressBarCtrl progressCtrl = GetDlgItem(IDC_PROGRESS2);
    if (::IsWindow(progressCtrl.m_hWnd)) {
        progressCtrl.SetRange(0, 10000);
        progressCtrl.SetPos(completed * 10000 / (m_overallMax > 0 ? m_overallMax : 1));
    }
}

DWORD CProgressDialog::ThreadProc()
{
    BOOL bRet;
    MSG msg;

    ::PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE); // Should create the message queue for this thread.

    Create(NULL);
    ShowWindow(SW_SHOWDEFAULT);

    // Manual Barrier Wait (Signal UI is ready)
    {
        std::unique_lock<std::mutex> lock(m_syncMutex);
        if (--m_syncCount > 0) {
            m_syncCond.wait(lock, [this] { return m_syncCount == 0; });
        } else {
            m_syncCond.notify_all();
        }
    }

    while(IsRunning())
    {
        bRet = ::GetMessage(&msg, NULL, 0, 0);

        if(bRet == -1) {
            continue; // error, don't process
        }
        else if(!bRet) {
            break; // WM_QUIT, exit message loop
        }

        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }

    ShowWindow(SW_HIDE);
    DestroyWindow();

    return 0;
}

LRESULT CProgressDialog::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    SetDlgItemText(IDC_DESCRIPTION1, _T(""));
    SetDlgItemText(IDC_DESCRIPTION2, _T(""));
    SetDlgItemText(IDC_DESCRIPTION3, _T(""));
    setOverallProgress(0);
    setTaskProgress(0);
    return 0;
}

LRESULT CProgressDialog::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    m_isCanceled = true;
    SetDlgItemText(IDC_DESCRIPTION1, _T("Aborting..."));
    GetDlgItem(IDCANCEL).EnableWindow(FALSE);
    return 0;
}
