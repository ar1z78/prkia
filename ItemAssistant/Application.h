#ifndef APPLICATION_H
#define APPLICATION_H

#include <settings/SettingsManager.h>

class CMainFrame;


class Application
{
public:
    Application();
    ~Application();

    bool init(std::tstring const& cmdLine);
    void destroy();
    int run(LPTSTR /*lpstrCmdLine*/ = NULL, int nCmdShow = SW_SHOWDEFAULT);

private:
    std::shared_ptr<CMainFrame> m_mainWindow;
    std::shared_ptr<aoia::SettingsManager> m_settings;
};


#endif // APPLICATION_H
