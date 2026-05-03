#include "stdafx.h"
#include "StringUtil.h"
#include "InvTreeItems.h"
#include "InventoryView.h"
#include "Shared\TextFormat.h"
#include "RenameBackpackDlg.h"
#include <AOManager.h>
#include "DBManager.h"    // For g_DBManager
#include <shared/IDB.h>   // For ExecTable definition
#include <string>



SqlTreeViewItemBase::SqlTreeViewItemBase(InventoryView* pOwner)
    : m_pOwner(pOwner) {}


SqlTreeViewItemBase::~SqlTreeViewItemBase() {}


void SqlTreeViewItemBase::SetOwner(InventoryView* pOwner)
{
    m_pOwner = pOwner;
}


unsigned int SqlTreeViewItemBase::AppendMenuCmd(HMENU hMenu, unsigned int firstID, WTL::CTreeItem item) const
{
    return firstID;
}


bool SqlTreeViewItemBase::HandleMenuCmd(unsigned int commandID, WTL::CTreeItem item)
{
    return false;
}


void SqlTreeViewItemBase::SetLabel(std::tstring const& newLabel) {}


bool SqlTreeViewItemBase::SortChildren() const
{
    return false;
}


/***************************************************************************/
/** Container Tree View Item                                              **/
/***************************************************************************/

ContainerTreeViewItem::ContainerTreeViewItem(sqlite::IDBPtr db, aoia::IContainerManagerPtr containerManager, InventoryView* pOwner, unsigned int charid, unsigned int containerid, std::tstring const& constraints, std::tstring const& label)
    : m_db(db)
    , m_containerManager(containerManager)
    , m_charid(charid)
    , m_containerid(containerid)
    , m_constraints(constraints)
    , SqlTreeViewItemBase(pOwner)
{
    if (label.empty())
    {
        std::tstringstream str;
        std::tstring containerName = m_containerManager->GetContainerName(m_charid, m_containerid);

        if (containerName.empty())
        {
            str << "Backpack: " << m_containerid;
        }
        else
        {
            str << containerName;
        }

        m_label = str.str();
    }
    else
    {
        m_label = label;
    }
}


ContainerTreeViewItem::~ContainerTreeViewItem() {}


void ContainerTreeViewItem::OnSelected()
{
    std::tstringstream sql;
    sql << _T("owner = ") << m_charid << _T(" AND parent ");
	
    if (m_containerid == 0)
    {
        sql << _T("> 3 AND parent NOT IN (SELECT DISTINCT children FROM tItems)");
    }
    else
    {
        sql << _T(" = ") << m_containerid;
    }

    if (!m_constraints.empty())
    {
        sql << _T(" AND ") << m_constraints;
    }

    m_pOwner->UpdateListView(sql.str());
}


bool ContainerTreeViewItem::CanEdit() const
{
    return false;
}


std::tstring ContainerTreeViewItem::GetLabel() const
{
    return m_label;
}


bool ContainerTreeViewItem::HasChildren() const
{
    bool result = false;
    // Check for Bank (1), Inventory (2), Shop (3) ...
    if (m_containerid >= 1 && m_containerid <= 3)
    {
        std::tstringstream sql;
        sql << _T("SELECT children FROM tItems WHERE children > 0 AND parent = ") << m_containerid 
            << _T(" AND owner = ") << m_charid;

        // Existence check: Only count this as a child if the ID actually contains items
        sql << _T(" AND children IN (SELECT DISTINCT parent FROM tItems WHERE owner = ") << m_charid << _T(")");

        // Existing constraints (like slot > 63) will now work perfectly
        if (!m_constraints.empty())
        {
            sql << _T(" AND ") << m_constraints;
        }

        sql << _T(" LIMIT 1");

        g_DBManager.Lock();
        sqlite::ITablePtr pT = m_db->ExecTable(sql.str());
        g_DBManager.UnLock();

        if (pT != NULL)
        {
            result = pT->Rows() > 0;
        }
    }
    return result;
}


std::vector<MFTreeViewItem*> ContainerTreeViewItem::GetChildren() const
{
    std::vector<MFTreeViewItem*> result;
    if (m_containerid == 1 || m_containerid == 2)
    {
        // Init contents from DB. Only select children IDs that are actually used as parents by other items
        std::tstringstream sql;
        sql << _T("SELECT DISTINCT children FROM tItems ")
            << _T("WHERE children > 0 AND parent = ") << m_containerid 
            << _T(" AND owner = ") << m_charid
            << _T(" AND children IN (SELECT DISTINCT parent FROM tItems WHERE owner = ") << m_charid << _T(")");

        if (!m_constraints.empty())
        {
            sql << _T(" AND ") << m_constraints;
        }

        g_DBManager.Lock();
        sqlite::ITablePtr pT = m_db->ExecTable(sql.str());
        g_DBManager.UnLock();

        if (pT != NULL)
        {
            for (size_t i = 0; i < pT->Rows(); ++i)
            {
                unsigned int contid = (unsigned int)_ttoi(from_ascii_copy(pT->Data(i, 0)).c_str());
                result.push_back(new ContainerTreeViewItem(m_db, m_containerManager, m_pOwner, m_charid, contid));
            }
        }
    }
    return result;
}



unsigned int ContainerTreeViewItem::AppendMenuCmd(HMENU hMenu, unsigned int firstID, WTL::CTreeItem item) const
{
    if (m_containerid != 0)
    {
		m_commands[firstID] = SqlTreeViewItemBase::CMD_DELETE;
		AppendMenu(hMenu, MF_STRING, firstID++, _T("Delete Items From DB"));

		// Can't rename inventory / bank /etc
		if (m_containerid >= 1024) {
			m_commands[firstID] = SqlTreeViewItemBase::CMD_RENAME;
			AppendMenu(hMenu, MF_STRING, firstID++, _T("Rename backpack"));
		}
    }
    return firstID;
}
inline bool exists_test(const std::tstring& name) {
	if (FILE *file = _wfopen(name.c_str(), _T("r"))) {
		fclose(file);
		return true;
	} else {
		return false;
	}   
}

void ContainerTreeViewItem::HandleMenuCmdRename(WTL::CTreeItem item) {
	if (m_containerid < 1024)
		return; // Bank, inventory, etc..


	std::tstring prefs = AOManager::instance().getAOPrefsFolder();
	std::tstring accname = AOManager::instance().getAccountName(m_charid);
	if (accname.length() > 0) {

		std::tstringstream path;
		path << prefs << _T("\\") << accname << _T("\\Char") 
			<<m_charid << _T("\\Containers\\Container_51017x") << m_containerid << _T(".xml");


		std::tstring strpath = path.str();
		if (exists_test(strpath)) {



			RenameBackpackDlg dlg;
			dlg.setCurrentName(m_containerManager->GetContainerName(m_charid, m_containerid));
			dlg.DoModal();

			std::tstring bagname = dlg.getBackpackName();

			if (dlg.getSuccess()) {
				// Rename to new name
				if (bagname.length() > 0) {
					std::ifstream t(strpath);
					std::tstring data;

					t.seekg(0, std::ios::end);   
					//data.reserve(t.tellg());
					data.reserve(static_cast<unsigned int>(t.tellg()));
					t.seekg(0, std::ios::beg);

					data.assign((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());


					std::tstring startStr = _T("name=\"container_name\" value='");
					int startPos = data.find(startStr.c_str());
					int endPos = data.find(_T("'"), startStr.length() + startPos);
					std::replace( bagname.begin(), bagname.end(), '\'', '_');
					// Spirit's version was double-quoting based on previous code logic, but StringUtil::xml_encode handles the entities correctly.
					std::tstring bagnameEscaped = _T("&quot;") + StringUtil::xml_encode(bagname) + _T("&quot;");

					std::tofstream outfile(strpath);
					if (startPos == std::tstring::npos) {
						startStr = _T("<Archive code=\"0\">");
						startPos = data.find(startStr.c_str());

						outfile << data.substr(0, startPos + startStr.length()).c_str();
						outfile << _T("\n    <String name=\"container_name\" value='");
						outfile << bagnameEscaped.c_str();
						outfile << _T("' />");
						outfile << data.substr(startPos + startStr.length()).c_str();
					}
					else {
						outfile << data.substr(0, startPos + startStr.length()).c_str();
						outfile << bagnameEscaped.c_str();
						outfile << data.substr(endPos).c_str();
					}
					outfile.close();

					WTL::CTreeItem parent = item.GetParent();
					parent.Expand(TVE_COLLAPSE | TVE_COLLAPSERESET);
					parent.Expand();
					
				}

				// Default name
				else {
					std::ifstream t(strpath);
					std::tstring data;

					t.seekg(0, std::ios::end);   
					//data.reserve(t.tellg());
					data.reserve(static_cast<unsigned int>(t.tellg()));
					t.seekg(0, std::ios::beg);
					data.assign((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());

					std::tstring startStr = _T("<String name=\"container_name");
					int startPos = data.find(startStr.c_str());
					int endPos = data.find(_T(">"), startStr.length() + startPos);


					std::tofstream outfile(strpath);
					if (startPos != std::tstring::npos) {
						outfile << data.substr(0, startPos).c_str();
						outfile << data.substr(endPos + 1).c_str();
					}
					outfile.close();

					WTL::CTreeItem parent = item.GetParent();
					parent.Expand(TVE_COLLAPSE | TVE_COLLAPSERESET);
					parent.Expand();
				}
			}
		}
		else {
			RenameBackpackDlg dlg;
			dlg.setCurrentName(m_containerManager->GetContainerName(m_charid, m_containerid));
			dlg.DoModal();

			if (dlg.getSuccess()) {
				std::tstring bagname = dlg.getBackpackName();

				// Rename to new name
				if (bagname.length() > 0) {
					std::replace( bagname.begin(), bagname.end(), '\'', '_');
					// Spirit's version was double-quoting based on previous code logic, but StringUtil::xml_encode handles the entities correctly.
					std::tstring bagnameEscaped = _T("&quot;") + StringUtil::xml_encode(bagname) + _T("&quot;");
					std::tofstream outfile(strpath);
					outfile << _T("<Archive code=\"0\">\n<String name=\"container_name\" value='");
					outfile << bagnameEscaped.c_str();
					outfile << _T("' />\n</Archive>");
					outfile.close();


					WTL::CTreeItem parent = item.GetParent();
					parent.Expand(TVE_COLLAPSE | TVE_COLLAPSERESET);
					parent.Expand();
				}
			}
		}
	}
}
bool ContainerTreeViewItem::HandleMenuCmd(unsigned int commandID, WTL::CTreeItem item) {
    if (m_commands.find(commandID) != m_commands.end())
    {
        switch (m_commands[commandID])
        {
		case SqlTreeViewItemBase::CMD_RENAME:
			HandleMenuCmdRename(item);
			break;

        case SqlTreeViewItemBase::CMD_DELETE:
            {
                g_DBManager.Lock();
                m_db->Begin();
                std::tstringstream sql;
                sql << _T("DELETE FROM tItems WHERE parent = ") << m_containerid <<
                    _T("; DELETE FROM tItems WHERE children = ") << m_containerid;
                if (m_db->Exec(sql.str()))
                {
                    m_db->Commit();
                }
                else
                {
                    m_db->Rollback();
                }
                g_DBManager.UnLock();
            }
            break;
        default:
            break;
        }
    }
    else {
		// Input: F2
		if (commandID == 4) {
			HandleMenuCmdRename(item);
			return true;
		}

        return false;
    }
    return true;
}


bool ContainerTreeViewItem::CanDelete() const
{
    return true;
}


bool ContainerTreeViewItem::SortChildren() const
{
    return true;
}


/***************************************************************************/
/** Character Tree View Item                                              **/
/***************************************************************************/

CharacterTreeViewItem::CharacterTreeViewItem(sqlite::IDBPtr db, aoia::IContainerManagerPtr containerManager, InventoryView* pOwner, unsigned int charid)
    : m_db(db)
    , m_containerManager(containerManager)
    , m_charid(charid)
    , SqlTreeViewItemBase(pOwner)
{
    g_DBManager.Lock();
    m_label = g_DBManager.GetToonName(charid);
    g_DBManager.UnLock();

    if (m_label.empty())
    {
        std::tstringstream str;
        str << charid;
        m_label = str.str();
    }
}


CharacterTreeViewItem::~CharacterTreeViewItem() {}


void CharacterTreeViewItem::OnSelected()
{
    std::tstringstream str;
    str << _T("owner = ") << m_charid;
    m_pOwner->UpdateListView(str.str());
}


bool CharacterTreeViewItem::CanEdit() const
{
    return false;
}


std::tstring CharacterTreeViewItem::GetLabel() const
{
    g_DBManager.Lock();
    std::tstring result = g_DBManager.GetToonName(m_charid);
    g_DBManager.UnLock();

    if (result.empty())
    {
        std::tstringstream str;
        str << m_charid;
        result = str.str();
    }

    std::tstring tCredsFmt(_T(""));
    if (AOManager::instance().getShowCreds())
    {
        g_DBManager.Lock();
        sqlite::ITablePtr pCreds = m_db->ExecTable(STREAM2STR("SELECT statvalue FROM tToonStats WHERE charid = " << (unsigned int)m_charid << " AND statid = 61"));
        g_DBManager.UnLock();

		__int64 lCreds = 0;
		if (pCreds && pCreds->Rows() > 0) {
			std::string data = pCreds->Data(0, 0);
			if (!data.empty()) {
				try {
					lCreds = std::stoll(data);
				}
				catch (...) {
					lCreds = 0;
				}
			}
		}
        
        if (lCreds < 0) lCreds = 0;

        TextFormat fT(3, ",");
        tCredsFmt = _T(" :: ") + fT.FormatLongLongToString(lCreds) + _T("cr");
    }
    
    return result + tCredsFmt;
}



void CharacterTreeViewItem::SetLabel(std::tstring const& newLabel)
{
    g_DBManager.Lock();
    g_DBManager.SetToonName(m_charid, newLabel);
    g_DBManager.UnLock();
}


bool CharacterTreeViewItem::HasChildren() const
{
    return true;
}


std::vector<MFTreeViewItem*> CharacterTreeViewItem::GetChildren() const
{
    std::vector<MFTreeViewItem*> result;

    result.push_back(new ContainerTreeViewItem(m_db, m_containerManager, m_pOwner, m_charid, 1, _T(""), _T("Bank"))); // bank
    result.push_back(new ContainerTreeViewItem(m_db, m_containerManager, m_pOwner, m_charid, 2, _T("slot > 63"), _T("Inventory"))); // inventory
    result.push_back(new ContainerTreeViewItem(m_db, m_containerManager, m_pOwner, m_charid, 2, _T("slot < 16"), _T("Weapons"))); // Weapons tab
    result.push_back(new ContainerTreeViewItem(m_db, m_containerManager, m_pOwner, m_charid, 2, _T("slot >= 16 AND slot < 32"), _T("Cloth"))); // Armor tab
    result.push_back(new ContainerTreeViewItem(m_db, m_containerManager, m_pOwner, m_charid, 2, _T("slot >= 32 AND slot < 47"), _T("Implants"))); // Implants tab
    result.push_back(new ContainerTreeViewItem(m_db, m_containerManager, m_pOwner, m_charid, 2, _T("slot >= 47 AND slot < 64"), _T("Social"))); // Social tab
    //result.push_back(new ContainerTreeViewItem(m_db, m_containerManager, m_pOwner, m_charid, 3, _T(""), _T("Shop"))); // Player shop
#ifdef DEBUG
    result.push_back(new ContainerTreeViewItem(m_db, m_containerManager, m_pOwner, m_charid, 0, _T(""), _T("Unknown"))); // Unknown
#endif

    return result;
}


unsigned int CharacterTreeViewItem::AppendMenuCmd(HMENU hMenu, unsigned int firstID, WTL::CTreeItem item) const
{
    m_commands[firstID] = SqlTreeViewItemBase::CMD_DELETE;
    AppendMenu(hMenu, MF_STRING, firstID++, _T("Delete Toon"));
    m_commands[firstID] = SqlTreeViewItemBase::CMD_EXPORT;
    AppendMenu(hMenu, MF_STRING, firstID++, _T("Export Items..."));
    return firstID;
}


bool CharacterTreeViewItem::HandleMenuCmd(unsigned int commandID, WTL::CTreeItem item)
{
    if (m_commands.find(commandID) != m_commands.end())
    {
        switch (m_commands[commandID])
        {
        case SqlTreeViewItemBase::CMD_DELETE:
            {
                bool ok = true;
                g_DBManager.Lock();
                m_db->Begin();
                {
                    std::tstringstream sql;
                    sql << _T("DELETE FROM tItems WHERE owner = ") << m_charid;
                    ok = m_db->Exec(sql.str());
                }
                if (ok)
                {
                    std::tstringstream sql;
                    sql << _T("DELETE FROM tToons WHERE charid = ") << m_charid;
                    ok = m_db->Exec(sql.str());
                }
                if (ok)
                {
                    m_db->Commit();
                }
                else
                {
                    m_db->Rollback();
                }
                g_DBManager.UnLock();
            }
            break;

        case SqlTreeViewItemBase::CMD_EXPORT:
            {
                std::tstringstream str;
                str << _T("owner = ") << m_charid;
                m_pOwner->exportToCSV(str.str());
            }
            break;

        default:
            break;
        }
    }
    else
    {
        return false;
    }
    return true;
}


bool CharacterTreeViewItem::CanDelete() const
{
    return true;
}


/***************************************************************************/
/** Account Tree View Item                                                **/
/***************************************************************************/

AccountTreeViewItem::AccountTreeViewItem(sqlite::IDBPtr db, aoia::IContainerManagerPtr containerManager, InventoryView* pOwner, std::tstring accountName)
    : m_db(db)
    , m_containerManager(containerManager)
    , SqlTreeViewItemBase(pOwner)
    , m_label(accountName)
{
}


AccountTreeViewItem::~AccountTreeViewItem()
{
}


void AccountTreeViewItem::OnSelected() 
{
}


bool AccountTreeViewItem::CanEdit() const
{
    return false;
}


std::tstring AccountTreeViewItem::GetLabel() const
{
	std::tstring accPath = AOManager::instance().getAOPrefsFolder();
	if (accPath.empty()) return _T("Account :: ") + m_label;

	accPath += _T("\\") + m_label;
	unsigned __int64 lCreds = 0;

	WIN32_FIND_DATA findData;
	std::tstring searchPath = accPath + _T("\\*");
	HANDLE hFind = FindFirstFile(searchPath.c_str(), &findData);

	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			// Skip "." and ".." and non-directories
			if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
			if (findData.cFileName[0] == '.') continue;

			std::tstring subfolderName = findData.cFileName;

			if (subfolderName.length() < 5 || subfolderName == _T("Browser")) {
				continue;
			}

			try
			{
				// subfolderName is "Char12345", so we skip the first 4 chars
				unsigned int charID = (unsigned int)_ttoi(subfolderName.substr(4).c_str());

				if (charID != 0)
				{
					g_DBManager.Lock();
					// Query character name
					sqlite::ITablePtr pToon = m_db->ExecTable(STREAM2STR("SELECT charname FROM tToons WHERE charid = " << charID));

					if (pToon->Rows() > 0 && !pToon->Data(0, 0).empty())
					{
						// Query Credits (Stat 61)
						sqlite::ITablePtr pCreds = m_db->ExecTable(STREAM2STR("SELECT statvalue FROM tToonStats WHERE charid = " << charID << " AND statid = 61"));

						if (pCreds->Rows() > 0 && !pCreds->Data(0, 0).empty()) {
							lCreds += atoll(pCreds->Data(0, 0).c_str()); // Use _ttoll for __int64
						}
					}
					g_DBManager.UnLock();
				}
			}
			catch (...) {
				continue;
			}
		} while (FindNextFile(hFind, &findData));
		FindClose(hFind);
	}

	std::tstring tCredsFmt(_T(""));
	if (AOManager::instance().getShowCreds())
	{
		TextFormat fT(3, ",");
		tCredsFmt = _T(" :: ") + fT.FormatLongLongToString(lCreds) + _T("cr");
	}

	return _T("Account :: ") + m_label + tCredsFmt;
}



bool AccountTreeViewItem::HasChildren() const
{
    return true;
}


std::vector<MFTreeViewItem*> AccountTreeViewItem::GetChildren() const
{
	std::vector<MFTreeViewItem*> result;

	std::tstring accountPath = AOManager::instance().getAOPrefsFolder();
	if (accountPath.empty()) return result;

	accountPath += _T("\\") + m_label;

	WIN32_FIND_DATA findData;
	std::tstring searchPath = accountPath + _T("\\*");
	HANDLE hFind = FindFirstFile(searchPath.c_str(), &findData);

	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			// Skip non-directories and system folders
			if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
			if (_tcscmp(findData.cFileName, _T(".")) == 0 || _tcscmp(findData.cFileName, _T("..")) == 0) continue;

			std::tstring subfolderName = findData.cFileName;

			// Same validation logic as before
			if (subfolderName.length() < 5 || subfolderName == _T("Browser")) {
				continue;
			}

			try
			{
				// Convert "Char12345" -> 12345
				unsigned int charID = (unsigned int)_ttoi(subfolderName.substr(4).c_str());

				if (charID != 0)
				{
					g_DBManager.Lock();
					// Check if the toon exists in our database
					sqlite::ITablePtr pToon = m_db->ExecTable(STREAM2STR("SELECT charname FROM tToons WHERE charid = " << charID));
					g_DBManager.UnLock();

					if (pToon->Rows() > 0 && !pToon->Data(0, 0).empty()) {
						result.push_back(new CharacterTreeViewItem(m_db, m_containerManager, m_pOwner, charID));
					}
				}
			}
			catch (...) {
				continue;
			}
		} while (FindNextFile(hFind, &findData));
		FindClose(hFind);
	}

	return result;
}



unsigned int AccountTreeViewItem::AppendMenuCmd(HMENU hMenu, unsigned int firstID, WTL::CTreeItem item) const
{
    return firstID;
}


bool AccountTreeViewItem::HandleMenuCmd(unsigned int commandID, WTL::CTreeItem item)
{
    return false;
}


/***************************************************************************/
/** Account Root Tree View Item                                                **/
/***************************************************************************/

AccountRootTreeViewItem::AccountRootTreeViewItem(sqlite::IDBPtr db, aoia::IContainerManagerPtr containerManager, InventoryView* pOwner)
    : m_db(db)
    , m_containerManager(containerManager)
    , SqlTreeViewItemBase(pOwner)
{
}


AccountRootTreeViewItem::~AccountRootTreeViewItem()
{
}


void AccountRootTreeViewItem::OnSelected() 
{
	if (m_pOwner) {
		m_pOwner->UpdateListView(_T(""));
	}
}


bool AccountRootTreeViewItem::CanEdit() const
{
    return false;
}


std::tstring AccountRootTreeViewItem::GetLabel() const
{
	unsigned int iAccounts(0);
	std::tstring tRet(_T(":: "));
	unsigned __int64 lCreds(0);

	std::tstring prefsPath = AOManager::instance().getAOPrefsFolder();

	if (!prefsPath.empty())
	{
		WIN32_FIND_DATA findData;
		std::tstring searchPath = prefsPath + _T("\\*");
		HANDLE hFind = FindFirstFile(searchPath.c_str(), &findData);

		if (hFind != INVALID_HANDLE_VALUE)
		{
			do
			{
				// Skip non-directories and system folders (. and ..)
				if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
				if (_tcscmp(findData.cFileName, _T(".")) == 0 || _tcscmp(findData.cFileName, _T("..")) == 0) continue;

				std::tstring accName = findData.cFileName;

				if (accName != _T("Browser"))
				{
					// Construct the account path manually
					std::tstring fullAccPath = prefsPath + _T("\\") + accName;

					unsigned __int64 lHasCreds = AccountRootTreeViewItem::DoesAccountHaveToons(fullAccPath);
					if (lHasCreds != (unsigned __int64)-1)
					{
						lCreds += lHasCreds;
						iAccounts++;
					}
				}
			} while (FindNextFile(hFind, &findData));
			FindClose(hFind);
		}

		std::tstring tAcctWord = (iAccounts == 1) ? _T(" Account :: ") : _T(" Accounts :: ");

		// Replace lexical_cast with standard to_tstring or stringstream
		std::tstringstream ss;
		ss << iAccounts;
		tRet = tRet + ss.str() + tAcctWord;
	}

	// 2. Get number of logged toons from the DB
	g_DBManager.Lock();
	sqlite::ITablePtr pTC = m_db->ExecTable(_T("SELECT COUNT(DISTINCT charid) FROM tToons"));
	g_DBManager.UnLock();

	// Convert the narrow SQLite data to a wide T-string
	std::tstring tToonCount = from_ascii_copy(pTC->Data(0, 0));
	unsigned int iCount = (unsigned int)_ttoi(tToonCount.c_str());

	std::tstring tWord = (iCount == 1) ? _T(" Character ::") : _T(" Characters ::");

	std::tstring tCredsFmt(_T(""));
	if (AOManager::instance().getShowCreds())
	{
		TextFormat fT(3, ",");
		tCredsFmt = _T(" ") + fT.FormatLongLongToString(lCreds) + _T("cr ::");
	}

	return tRet + tToonCount + tWord + tCredsFmt;
}



bool AccountRootTreeViewItem::HasChildren() const
{
    return true;
}


std::vector<MFTreeViewItem*> AccountRootTreeViewItem::GetChildren() const
{
	std::vector<MFTreeViewItem*> result;

	std::tstring prefsPath = AOManager::instance().getAOPrefsFolder();
	if (prefsPath.empty()) {
		return result;
	}

	WIN32_FIND_DATA findData;
	std::tstring searchPath = prefsPath + _T("\\*");
	HANDLE hFind = FindFirstFile(searchPath.c_str(), &findData);

	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			// Filter for Directories, skipping "." and ".."
			if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
			if (_tcscmp(findData.cFileName, _T(".")) == 0 || _tcscmp(findData.cFileName, _T("..")) == 0) continue;

			std::tstring accName = findData.cFileName;

			if (accName != _T("Browser"))
			{
				// Construct the full path to the account folder
				std::tstring fullAccPath = prefsPath + _T("\\") + accName;

				// Check if account has valid characters (Update this function signature to take tstring)
				unsigned __int64 lCreds = AccountRootTreeViewItem::DoesAccountHaveToons(fullAccPath);

				// In the original code, they check if lCreds != 0 (meaning has toons)
				if (lCreds != (unsigned __int64)-1 && lCreds != 0)
				{
					result.push_back(new AccountTreeViewItem(m_db, m_containerManager, m_pOwner, accName));
				}
			}
		} while (FindNextFile(hFind, &findData));
		FindClose(hFind);
	}

	return result;
}



unsigned int AccountRootTreeViewItem::AppendMenuCmd(HMENU hMenu, unsigned int firstID, WTL::CTreeItem item) const
{
    return firstID;
}


bool AccountRootTreeViewItem::HandleMenuCmd(unsigned int commandID, WTL::CTreeItem item)
{
    return false;
}


unsigned __int64 AccountRootTreeViewItem::DoesAccountHaveToons(std::tstring accPath) const
{
	unsigned __int64 lCreds = 0;
	bool foundToon = false;

	// 1. Prepare Win32 Search
	WIN32_FIND_DATA findData;
	std::tstring searchPath = accPath + _T("\\*");
	HANDLE hFind = FindFirstFile(searchPath.c_str(), &findData);

	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			// Skip files, ".", and ".."
			if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
			if (_tcscmp(findData.cFileName, _T(".")) == 0 || _tcscmp(findData.cFileName, _T("..")) == 0) continue;

			std::tstring subfolderName = findData.cFileName;

			// Validation logic
			if (subfolderName.length() < 5 || subfolderName == _T("Browser")) {
				continue;
			}

			try
			{
				// Convert "Char12345" -> 12345
				unsigned int charID = (unsigned int)_ttoi(subfolderName.substr(4).c_str());

				if (charID != 0)
				{
					g_DBManager.Lock();
					// Check if toon exists in our DB
					// Change m_db->ExecTable(...) to:
					sqlite::ITablePtr pToon = g_DBManager.GetDatabase()->ExecTable(STREAM2STR("SELECT charname FROM tToons WHERE charid = " << charID));

					if (pToon->Rows() > 0 && !pToon->Data(0, 0).empty())
					{
						foundToon = true;
						// Get Credits (Stat 61)
						sqlite::ITablePtr pCreds = g_DBManager.GetDatabase()->ExecTable(STREAM2STR("SELECT statvalue FROM tToonStats WHERE charid = " << charID << " AND statid = 61"));

						if (pCreds->Rows() > 0 && !pCreds->Data(0, 0).empty()) {
							lCreds += atoll(pCreds->Data(0, 0).c_str());

						}
					}
					g_DBManager.UnLock();
				}
			}
			catch (...) {
				continue;
			}
		} while (FindNextFile(hFind, &findData));
		FindClose(hFind);
	}

	// Original logic check: if no toons found, return -1 (or 0 based on your usage)
	return foundToon ? lCreds : (unsigned __int64)-1;
}
