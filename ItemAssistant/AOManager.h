#pragma warning(disable: 4251)

#ifndef AOMANAGER_H
#define AOMANAGER_H

#include <ItemAssistantCore.h>
#include <Shared/UnicodeSupport.h>
#include <Shared/Singleton.h>
#include <settings/ISettings.h>
#include <vector>
#include <exception>

struct SellerSettings {
	int clSkill;
	bool isOmni;
	bool isTrader;
	std::tstring colorLow;
	std::tstring colorMed;
	std::tstring colorHigh;
	int priceMax;
};

class AOManager
{
    SINGLETON(AOManager);
public:
    ~AOManager();
	aoia::ISettingsPtr getSettings() const { return m_settings; }
    struct ITEMASSISTANTCORE_API AOManagerException : public std::exception {
        AOManagerException(std::tstring const& message) : std::exception(to_ascii_copy(message).c_str()) {}
    };

    void SettingsManager(aoia::ISettingsPtr settings);
    std::tstring getAOFolder() const;
	std::tstring getAOPrefsFolder() const;
	std::tstring getAOPrefsFolderManual() const;
	bool getAutoPrefs() const;
	void setAutoPrefs(bool);
	bool getShowCreds() const;
	void setShowCreds(bool);



	SellerSettings getSellerSettings() const;
	void setSellerSettings(const SellerSettings& s);

	bool getMinToTaskbar() const;
	void setMinToTaskbar(bool);
    std::vector<std::tstring> getAccountNames() const;
	std::tstring getAccountName(unsigned int charid) const;
	bool getShowInfo() const;
	void setShowInfo(bool);
//	bool getSellerStats() const; // For your next step
//	void setSellerStats(bool);   // For your next step
    bool getFindToggle() const;
    void setFindToggle(bool);

    bool getRecordStats() const;
    void setRecordStats(bool);

private:
    mutable std::tstring m_aofolder;
    mutable std::tstring m_prefsfolder;
	mutable std::tstring m_autoprefs;
	mutable std::tstring m_showcreds;
	mutable std::tstring m_minToTaskbar;
    aoia::ISettingsPtr m_settings;
	mutable std::tstring m_showInfo;
//	mutable std::tstring m_sellerStats; // For your next step
    mutable std::tstring m_recordStats;
    mutable std::tstring m_findToggle;
};

#endif // AOMANAGER_H
