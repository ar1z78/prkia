#include "StdAfx.h"
#include "AOManager.h"
#include <Shared/UnicodeSupport.h>
#include <Shared/FileUtils.h>
#include <Shlobj.h>
#include <iomanip>
#include <memory> 
#include <algorithm>

inline bool FileExists(const std::tstring& path) {
	DWORD dwAttrib = GetFileAttributes(path.c_str());
	return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

// Replicate the 32-bit Boost 1.53 Range Hash for strings
std::size_t legacy_boost_hash(const std::string& s) {
	std::size_t seed = 0;
	for (std::string::const_iterator it = s.begin(); it != s.end(); ++it) {
		// This is the "Magic" formula used by old Boost:
		seed ^= static_cast<std::size_t>(*it) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	}
	return seed;
}

using namespace aoia;


SINGLETON_IMPL(AOManager);


AOManager::AOManager()
{
}


AOManager::AOManager(const AOManager&)
{
}


AOManager::~AOManager()
{
}


std::tstring AOManager::getAOFolder() const
{
	assert(m_settings);

	if (m_aofolder.empty())
	{
		std::tstring AODir; // tpath is now std::tstring
		bool requestFolder = true;

		// 1. Get AO folder from settings
		std::tstring dir_setting = m_settings->getValue(_T("AOPath"));
		if (!dir_setting.empty())
		{
			AODir = dir_setting;
			// Use Win32 FileExists check
			if (FileExists(AODir + _T("\\anarchyonline.exe")))
			{
				requestFolder = false;
			}
		}

		if (requestFolder)
		{
			AODir = BrowseForFolder(NULL, _T("Please locate the AO directory:"));
			if (AODir.empty()) {
				return _T("");
			}

			if (!FileExists(AODir + _T("\\anarchyonline.exe"))) {
				MessageBox(NULL, _T("The specified directory doesn't contain a valid Anarchy Online installation."),
					_T("Error - AO Item Assistant++"), MB_OK | MB_ICONERROR);
				return _T("");
			}

			// 2. Store the new AO directory in the settings. 
			// Removed .native() because AODir is already a string.
			m_settings->setValue(_T("AOPath"), AODir);
		}

		m_aofolder = AODir;
	}

	return m_aofolder;
}



std::tstring AOManager::getAOPrefsFolder() const
{
	assert(m_settings);

	if (m_prefsfolder.empty()) {
		std::tstring dir_setting = m_settings->getValue(_T("AOPrefsPath"));
		if (!dir_setting.empty()) {
			m_prefsfolder = dir_setting;
		}
		else {
			std::tstring tAOFolder = getAOFolder();
			if (tAOFolder.empty()) return _T("");

			TCHAR tAppData[MAX_PATH];
			SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, tAppData);

			// 1. Prepare Root string (Parent of C:\win\client is C:\win)
			size_t last_slash = tAOFolder.find_last_of(_T("\\/"));
			std::tstring tAORoot = (last_slash == std::string::npos) ? tAOFolder : tAOFolder.substr(0, last_slash);

			// 2. NORMALIZE (Boost generic_string() logic)
			// Forward slashes, NO trailing slash
			std::replace(tAORoot.begin(), tAORoot.end(), '\\', '/');
			if (!tAORoot.empty() && tAORoot.back() == '/') {
				tAORoot.pop_back();
			}

			// 3. CALCULATE THE LEGACY HASH
			// Use the custom function instead of boost::hash<std::string>
			std::size_t nHash = legacy_boost_hash(to_ascii_copy(tAORoot));

			// 4. CONVERT TO HEX
			std::stringstream sAOHashDir;
			sAOHashDir << std::hex << nHash;

			// 5. BUILD FINAL PATH
			std::tstring path = tAppData;
			path += _T("\\Funcom\\Anarchy Online\\");
			path += from_ascii_copy(sAOHashDir.str()); // This should now be "5d4f750" (97843024)
			path += _T("\\") + tAOFolder.substr(last_slash + 1) + _T("\\Prefs");

			m_prefsfolder = path;

		}
	}
	return m_prefsfolder;
}


std::tstring AOManager::getAOPrefsFolderManual() const
{
	assert(m_settings);

	// 1. tpath is now a std::tstring, so we don't need bfs::
	std::tstring prefsDir;
	prefsDir = BrowseForFolder(NULL, _T("Please locate the AO prefs folder:"));

	if (prefsDir.empty()) {
		return _T("");
	}

	// 2. Use our Win32 FileExists helper and string concatenation
	if (!FileExists(prefsDir + _T("\\Prefs.xml"))) {
		MessageBox(NULL, _T("The specified directory doesn't seem to contain valid Anarchy Online prefs."),
			_T("Error - AO Item Assistant++"), MB_OK | MB_ICONERROR);
		return _T("");
	}

	// Store the new AO Prefs directory in the settings
	m_prefsfolder = prefsDir;
	m_settings->setValue(_T("AOPrefsPath"), m_prefsfolder);

	return m_prefsfolder;
}

SellerSettings AOManager::getSellerSettings() const {
	SellerSettings s;
	s.clSkill = _ttoi(m_settings->getValue(_T("skill")).c_str());
	s.isOmni = m_settings->getValue(_T("omni")) == _T("yes");
	s.isTrader = m_settings->getValue(_T("trader")) == _T("yes");

	s.colorLow = m_settings->getValue(_T("Price.ColorLow"));
	s.colorMed = m_settings->getValue(_T("Price.ColorMed"));
	s.colorHigh = m_settings->getValue(_T("Price.ColorHigh"));

	std::tstring maxStr = m_settings->getValue(_T("Price.MaxHigh"));
	s.priceMax = maxStr.empty() ? 100000 : _ttoi(maxStr.c_str());

	// Set defaults if settings are empty
	if (s.colorLow.empty())  s.colorLow = _T("808080");
	if (s.colorMed.empty())  s.colorMed = _T("00FF00");
	if (s.colorHigh.empty()) s.colorHigh = _T("FFD700");

	return s;
}

void AOManager::setSellerSettings(const SellerSettings& s) {
	m_settings->setValue(_T("skill"), STREAM2STR(s.clSkill));
	m_settings->setValue(_T("omni"), s.isOmni ? _T("yes") : _T("no"));
	m_settings->setValue(_T("trader"), s.isTrader ? _T("yes") : _T("no"));
	m_settings->setValue(_T("Price.ColorLow"), s.colorLow);
	m_settings->setValue(_T("Price.ColorMed"), s.colorMed);
	m_settings->setValue(_T("Price.ColorHigh"), s.colorHigh);
	m_settings->setValue(_T("Price.MaxHigh"), STREAM2STR(s.priceMax));

}




bool AOManager::getAutoPrefs() const
{
	if (m_settings->getValue(_T("AOPrefsPath")).empty())  // Get prefs folder from settings
		return true;

	return false;
}


void AOManager::setAutoPrefs(bool bAutoPrefs)
{
    m_prefsfolder = _T("");
    m_settings->setValue(_T("AOPrefsPath"), m_prefsfolder);
}


bool AOManager::getShowCreds() const
{
	if (m_showcreds.empty())
	{
		m_showcreds = m_settings->getValue(_T("ShowCreds"));   // Get ShowCreds from settings
		if (m_showcreds.empty())
		{
			m_showcreds = _T("yes");
			m_settings->setValue(_T("ShowCreds"), m_showcreds);
			return true;
		}
	}

	if (m_showcreds == _T("yes"))
		return true;
	else
		return false;
}


void AOManager::setShowCreds(bool bShowCreds)
{
	if (bShowCreds)
		m_showcreds = _T("yes");
	else
		m_showcreds = _T("no");
    
	m_settings->setValue(_T("ShowCreds"), m_showcreds);
}

// --- Info Handlers ---
bool AOManager::getShowInfo() const {
    if (m_showInfo.empty()) {
        m_showInfo = m_settings->getValue(_T("ShowInfo"));
        if (m_showInfo.empty()) {
            m_showInfo = _T("no"); // Default
            m_settings->setValue(_T("ShowInfo"), m_showInfo);
        }
    }
    return (m_showInfo == _T("yes"));
}

void AOManager::setShowInfo(bool bShow) {
    m_showInfo = bShow ? _T("yes") : _T("no");
    m_settings->setValue(_T("ShowInfo"), m_showInfo);
}

bool AOManager::getFindToggle() const {
    if (m_findToggle.empty()) {
        m_findToggle = m_settings->getValue(_T("FindToggle"));
        if (m_findToggle.empty()) {
			m_findToggle = _T("yes");
			m_settings->setValue(_T("FindToggle"), m_findToggle); }
    }
    return (m_findToggle == _T("yes"));
}

void AOManager::setFindToggle(bool bShow) {
    m_findToggle = bShow ? _T("yes") : _T("no");
    m_settings->setValue(_T("FindToggle"), m_findToggle);
}

bool AOManager::getRecordStats() const {
    if (m_recordStats.empty()) {
        m_recordStats = m_settings->getValue(_T("RecordStats"));
        if (m_recordStats.empty()) { 
            m_recordStats = _T("yes"); // Default to 'yes'
            m_settings->setValue(_T("RecordStats"), m_recordStats);
        }
    }
    return (m_recordStats == _T("yes"));
}

void AOManager::setRecordStats(bool bEnable) {
    m_recordStats = bEnable ? _T("yes") : _T("no");
    m_settings->setValue(_T("RecordStats"), m_recordStats);
}


bool AOManager::getMinToTaskbar() const
{
	if (m_minToTaskbar.empty())
	{
		m_minToTaskbar = m_settings->getValue(_T("Minimise To Taskbar"));   // Get it from settings
		if (m_minToTaskbar.empty())
		{
			m_minToTaskbar = _T("yes");
			m_settings->setValue(_T("Minimise To Taskbar"), m_minToTaskbar);
			return true;
		}
	}

	if (m_minToTaskbar == _T("yes"))
		return false;
	else
		return true;
}


void AOManager::setMinToTaskbar(bool bMinToTaskbar)
{
	if (bMinToTaskbar)
		m_minToTaskbar = _T("no");
	else
		m_minToTaskbar = _T("yes");
    
	m_settings->setValue(_T("Minimise To Taskbar"), m_minToTaskbar);
}

std::tstring AOManager::getAccountName(unsigned int charid) const {
	std::tstring charidStr;
	{
		std::tstringstream tmp;
		tmp << _T("Char") << charid;
		charidStr = tmp.str();
	}
	

	std::vector<std::tstring> accountNames = AOManager::getAccountNames() ;

	for (auto accountName : accountNames) {
		std::tstring path = AOManager::instance().getAOPrefsFolder() + _T("\\") + accountName + _T("\\*");


		WIN32_FIND_DATA findData;
		HANDLE hFind = FindFirstFileEx(path.c_str(), FindExInfoStandard, &findData, FindExSearchLimitToDirectories, NULL, 0);
		if (hFind != INVALID_HANDLE_VALUE) {
			do {
				if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
					&& (findData.cFileName[0] != NULL) 
					&& (findData.cFileName[0] != '.')
					&& (std::tstring(findData.cFileName).find(charidStr) != std::tstring::npos)
					)
				{		
					FindClose(hFind);
					hFind = NULL;
					return accountName;
				}
			}
			while (FindNextFile(hFind, &findData));
			if (hFind)
				FindClose(hFind);
		}
	}


	return _T("");
}
std::vector<std::tstring> AOManager::getAccountNames() const
{
    std::vector<std::tstring> result;

    std::tstring path = AOManager::instance().getAOPrefsFolder();
    path += _T("\\*");

    WIN32_FIND_DATA findData;

    HANDLE hFind = FindFirstFileEx(path.c_str(), FindExInfoStandard, &findData, FindExSearchLimitToDirectories, NULL, 0);

    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
                && (findData.cFileName[0] != NULL) 
                && (findData.cFileName[0] != '.')
				&& (std::tstring(findData.cFileName) != _T("Browser")))
            {
                result.push_back(std::tstring(findData.cFileName));
            }
        }
        while (FindNextFile(hFind, &findData));
        FindClose(hFind);
    }

    return result;
}


void AOManager::SettingsManager( aoia::ISettingsPtr settings )
{
    m_settings = settings;
}
