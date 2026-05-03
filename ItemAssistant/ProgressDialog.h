#pragma once

#include <Shared/Thread.h>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <string>

class CProgressDialog
    : public CDialogImpl<CProgressDialog>
    , protected Thread
{
public:
    enum { IDD = IDD_DUAL_PROGRESS };

    CProgressDialog(unsigned int overallMax, unsigned int taskMax);
    virtual ~CProgressDialog(void);

    BEGIN_MSG_MAP(CProgressDialog)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
    END_MSG_MAP()

    void setText(unsigned int line, std::tstring const& text);
    void setTaskProgress(unsigned int completed, unsigned int max = 0);
    void setOverallProgress(unsigned int completed, unsigned int max = 0);

    bool userCanceled() const { return m_isCanceled; }

    virtual DWORD ThreadProc();

protected:
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnCloseCmd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

private:
    unsigned int m_overallMax;
    unsigned int m_overallCurrent;
    unsigned int m_taskMax;
    unsigned int m_taskCurrent;
    bool m_isCanceled;
    std::vector<std::tstring> m_lines;

    std::mutex              m_syncMutex;
    std::condition_variable m_syncCond;
    int                     m_syncCount; 
};
