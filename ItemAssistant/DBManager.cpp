#include "StdAfx.h"
#include "dbmanager.h"
#include "shared/AODatabaseParser.h"
#include "shared/AODatabaseWriter.h"
#include "Shared/AODatabaseIndex.h"
#include "shared/FileUtils.h"
#include <sstream>
#include "ProgressDialog.h"
#include "AOManager.h"
#include "sqlite3/sqlite3.h"
#include <memory>
#include <windows.h>

inline bool FileExists(const std::tstring& path) {
	DWORD dwAttrib = GetFileAttributes(path.c_str());
	return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
		!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

#define CURRENT_DB_VERSION 8


DBManager::DBManager()
{
    m_db.reset(new sqlite::Db(Logger::instance().stream()));
}


DBManager::~DBManager()
{
}


bool DBManager::init(std::tstring dbfile)
{
	std::tstring aofolder = AOManager::instance().getAOFolder();
	if (aofolder.empty())
	{
		LOG("DBManager::init: Not a valid AO folder.");
		return false;
	}

	if (dbfile.empty())
	{
		dbfile = _T("ItemAssistant.db");
	}

	bool dbfileExists = FileExists(dbfile);

	if (!m_db->Init(dbfile))
	{
		// Note: I cleaned up the log macro to avoid potential stream issues
		std::tstring err = _T("DBManager::init: Unable to ");
		err += (dbfileExists ? _T("open") : _T("create"));
		err += _T(" database. [");
		err += dbfile;
		err += _T("]");
		LOG(to_ascii_copy(err).c_str());
		return false;
	}

	if (!dbfileExists)
	{
		createDBScheme();
	}

	unsigned int dbVersion = getDBVersion();
	if (dbVersion < CURRENT_DB_VERSION)
	{
		if (IDOK != MessageBox(NULL, _T("AO Item Assistant needs to update its database file to a newer version."),
			_T("Question - AO Item Assistant++"), MB_OKCANCEL | MB_ICONQUESTION))
		{
			return false;
		}
		updateDBVersion(dbVersion);
	}
	else if (dbVersion > CURRENT_DB_VERSION)
	{
		MessageBox(NULL,
			_T("AO Item Assistant has detected a too new version of its database file. You should upgrade the software to continue."),
			_T("Error - AO Item Assistant++"), MB_OK | MB_ICONERROR);
		return false;
	}

	// 2. Pass standard strings to the sync function
	if (!syncLocalItemsDB(_T("aoitems.db"), aofolder))
	{
		MessageBox(NULL, _T("AO Item Assistant cannot start without a valid item database."),
			_T("Error - AO Item Assistant++"), MB_OK | MB_ICONERROR);
		return false;
	}

	m_db->Exec(_T("ATTACH DATABASE \"aoitems.db\" AS aodb"));

	return true;
}



void DBManager::destroy()
{
    m_db->Term();
}

/**
* Check to see if we have a local database already.
* If it is, then check if it is up to date.
* If local database is missing or obsolete then recreate it.
* Return true if application has a local items database to run with, false otherwise.
*/
bool DBManager::syncLocalItemsDB(std::tstring const& localfile, std::tstring const& aofolder)
{
	bool hasLocalDB = false;
	FILETIME localWriteTime = { 0 };

	std::tstring local = localfile;
	std::tstring original = aofolder + _T("\\cd_image\\rdb.db");

	// Check if local DB exists and get its time
	WIN32_FILE_ATTRIBUTE_DATA localAttr;
	if (GetFileAttributesEx(local.c_str(), GetFileExInfoStandard, &localAttr))
	{
		hasLocalDB = true;
		localWriteTime = localAttr.ftLastWriteTime;
	}

	// Check if the original RDB exists
	if (!FileExists(original))
	{
		MessageBox(NULL, _T("Could not locate rdb.db in your AO folder. Please check your AO Path settings."), _T("Error"), MB_OK);
		return false;
	}

	if (hasLocalDB && getAODBSchemeVersion(localfile) == CURRENT_AODB_VERSION)
	{
		WIN32_FILE_ATTRIBUTE_DATA originalAttr;
		if (GetFileAttributesEx(original.c_str(), GetFileExInfoStandard, &originalAttr))
		{
			// If original is older or same as local, we are up to date
			if (CompareFileTime(&originalAttr.ftLastWriteTime, &localWriteTime) <= 0)
			{
				return true;
			}
		}

		int answer = ::MessageBox(NULL,
			_T("Your items database is out of date. Do you wish to update it now?\r\nAnswering 'NO' will continue using the old one."),
			_T("Question - AO Item Assistant++"), MB_ICONQUESTION | MB_YESNOCANCEL);

		if (answer == IDCANCEL) exit(0);
		else if (answer == IDNO) return true;
	}

	// 2. Prepare Temp File
	std::tstring tmpfile = _T("tmp_") + local;
	DeleteFile(tmpfile.c_str()); // Replace bfs::remove

	try
	{
		// 3. Extract items (Keep your SQLite logic as is, just use strings)
		std::string prk_rdb_path = to_ascii_copy(original);

		std::vector<unsigned int> item_offsets;
		std::vector<unsigned int> nano_offsets;

		sqlite3* rdb_conn;
		if (sqlite3_open(prk_rdb_path.c_str(), &rdb_conn) == SQLITE_OK) {
			sqlite3_stmt* stmt;
			if (sqlite3_prepare_v2(rdb_conn, "SELECT ID FROM rdb_1000020", -1, &stmt, NULL) == SQLITE_OK) {
				while (sqlite3_step(stmt) == SQLITE_ROW) item_offsets.push_back(sqlite3_column_int(stmt, 0));
			}
			sqlite3_finalize(stmt);
			if (sqlite3_prepare_v2(rdb_conn, "SELECT ID FROM rdb_1040005", -1, &stmt, NULL) == SQLITE_OK) {
				while (sqlite3_step(stmt) == SQLITE_ROW) nano_offsets.push_back(sqlite3_column_int(stmt, 0));
			}
			sqlite3_finalize(stmt);
			sqlite3_close(rdb_conn);
		}

		unsigned int itemCount = item_offsets.size();
		unsigned int nanoCount = nano_offsets.size();


		AODatabaseParser aodb(prk_rdb_path);
		AODatabaseWriter writer(to_ascii_copy(tmpfile), Logger::instance().stream());

		CProgressDialog dlg(itemCount + nanoCount, itemCount);
		dlg.SetWindowText(_T("Progress Dialog - Item Assistant"));
		dlg.setText(0, _T("Extracting data from the AO DB..."));
		dlg.setText(1, STREAM2STR("Finished " << 0 << " out of " << itemCount << " items."));
		dlg.setText(2, _T("Overall progress: 0%"));

		// Extract items
		std::shared_ptr<ao_item> item;
		unsigned int count = 0;
		writer.BeginWrite();
		for (std::vector<unsigned int>::iterator item_it = item_offsets.begin(); item_it != item_offsets.end(); ++item_it)
		{
			item = aodb.GetItem(*item_it, "rdb_1000020");
			count++;
			if (!item)
			{
				LOG(_T("Parsing item ") << count << _T(" at offset ") << *item_it << _T(" failed!"));
				continue;
			}
			writer.WriteItem(item);
			if (count % 1000 == 0) {
				if (dlg.userCanceled()) {
					return false;
				}
				dlg.setTaskProgress(count, itemCount);
				dlg.setText(1, STREAM2STR("Finished " << count << " out of " << itemCount << " items."));
				dlg.setOverallProgress(count, itemCount + nanoCount);
				dlg.setText(2, STREAM2STR("Overall progress: " << (count * 100) / max(1, itemCount + nanoCount) << "%"));
			}
			if (count % 10000 == 0) {
				writer.CommitItems();
				writer.BeginWrite();
			}
		}
		item.reset();
		dlg.setTaskProgress(count, itemCount);
		dlg.setText(1, STREAM2STR("Finished " << count << " out of " << itemCount << " items."));
		dlg.setOverallProgress(count, itemCount + nanoCount);
		dlg.setText(2, STREAM2STR("Overall progress: " << (count * 100) / max(1, itemCount + nanoCount) << "%"));
		writer.CommitItems();

		if (dlg.userCanceled())
		{
			return false;
		}

		// Extract nano programs
		std::shared_ptr<ao_item> nano;
		count = 0;
		writer.BeginWrite();
		for (std::vector<unsigned int>::iterator nano_it = nano_offsets.begin(); nano_it != nano_offsets.end(); ++nano_it)
		{
			nano = aodb.GetItem(*nano_it, "rdb_1040005");
			count++;
			if (!nano)
			{
				LOG(_T("Parsing nano ") << count << _T(" at offset ") << *nano_it << _T(" failed!"));
				continue;
			}
			writer.WriteItem(nano);
			if (count % 1000 == 0)
			{
				if (dlg.userCanceled())
				{
					return false;
				}
				dlg.setTaskProgress(count, nanoCount);
				dlg.setText(1, STREAM2STR("Finished " << count << " out of " << nanoCount << " nanos."));
				dlg.setOverallProgress(itemCount + count, itemCount + nanoCount);
				dlg.setText(2, STREAM2STR("Overall progress: " << ((itemCount + count) * 100) / max(1, itemCount +
					nanoCount) << "%"));
			}
			if (count % 10000 == 0)
			{
				writer.CommitItems();
				writer.BeginWrite();
			}
		}
		nano.reset();
		dlg.setTaskProgress(count, nanoCount);
		dlg.setText(1, STREAM2STR("Finished " << count << " out of " << nanoCount << " nanos."));
		dlg.setOverallProgress(itemCount + count, itemCount + nanoCount);
		dlg.setText(2, STREAM2STR("Overall progress: " << ((itemCount + count) * 100) / max(1, itemCount + nanoCount) << "%"));
		writer.CommitItems();
		
		if (dlg.userCanceled())
		{
			return false;
		}

		writer.PostProcessData();
		//force immediate redraw to close the dialog
		::InvalidateRect(dlg.m_hWnd, NULL, TRUE);
		::UpdateWindow(dlg.m_hWnd);
	}
	catch (std::exception& e)
	{
		LOG(_T("Error creating item database. ") << e.what());
		MessageBox(NULL, _T("Unable to parse the AO database."), _T("Error"), MB_OK | MB_ICONERROR);
		return false;
	}

	// 4. Final Swap
	DeleteFile(local.c_str());
	if (MoveFileEx(tmpfile.c_str(), local.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED)) {

		return true;
	}

	return false;
}


void DBManager::InsertItem(unsigned int keylow,
    unsigned int keyhigh,
    unsigned short ql,
    unsigned short flags,
    unsigned short stack,
    unsigned int parent,
    unsigned short slot,
    unsigned int children,
    unsigned int owner)
{
    std::tstringstream sql;
    sql << _T("INSERT INTO tItems (keylow, keyhigh, ql, flags, stack, parent, slot, children, owner) VALUES (")
        << (unsigned int) keylow      << _T(", ")
        << (unsigned int) keyhigh     << _T(", ")
        << (unsigned int) ql          << _T(", ")
        << (unsigned int) flags       << _T(", ")
        << (unsigned int) stack       << _T(", ")
        << (unsigned int) parent      << _T(", ")
        << (unsigned int) slot        << _T(", ")
        << (unsigned int) children    << _T(", ")
        << (unsigned int) owner       << _T(")");
    m_db->Exec(sql.str());
}


std::tstring DBManager::GetToonName(unsigned int charid) const
{
    std::tstring result;

    sqlite::ITablePtr pT = m_db->ExecTable(STREAM2STR("SELECT charid, charname FROM tToons WHERE charid = " << charid));

    if (pT != NULL && pT->Rows())
    {
        if (!pT->Data(0, 1).empty())
        {
            result = from_ascii_copy(pT->Data(0, 1));
        }
    }

    return result;
}


void DBManager::SetToonName(unsigned int charid, std::tstring const& newName)
{
    m_db->Begin();

    std::string asciiName = to_ascii_copy(newName);
	std::string sql = to_ascii_copy(STREAM2STR("INSERT OR REPLACE INTO tToons (charid, charname) VALUES (" << charid << ", '" << from_ascii_copy(asciiName) << "')"));
    //std::string sql = STREAM2STR("INSERT OR REPLACE INTO tToons (charid, charname) VALUES (" << charid << ", '" << asciiName << "')");

    if (!m_db->Exec(sql))
    {
        // Insert failed, try update
		sql = to_ascii_copy(STREAM2STR("UPDATE OR IGNORE tToons SET charname='" << from_ascii_copy(asciiName) << "' WHERE charid=" << charid));
        m_db->Exec(sql);
    }

    m_db->Commit();
}


void DBManager::SetToonShopId(unsigned int charid, unsigned int shopid)
{
    assert(charid != 0);
    assert(shopid != 0);

	std::string sql = to_ascii_copy(STREAM2STR("UPDATE OR IGNORE tToons SET shopid=" << shopid << " WHERE charid=" << charid));

    m_db->Begin();
    m_db->Exec(sql);
    m_db->Commit();
}


unsigned int DBManager::GetToonShopId(unsigned int charid) const
{
    assert(charid != 0);

    unsigned int result = 0;

    sqlite::ITablePtr pT = m_db->ExecTable(STREAM2STR("SELECT shopid FROM tToons WHERE charid = " << charid));
    if (pT != NULL && pT->Rows() > 0)
    {
        std::string data = pT->Data(0, 0);
        if (!data.empty())
        {
            // atoi is safe and handles non-numeric strings by returning 0
            result = (unsigned int)atoi(data.c_str());

            // Logic check: if it's 0 but the string wasn't "0", it's a bad parse
            if (result == 0 && data != "0")
            {
				LOG("Error in GetToonShopId(). Bad numeric data: " << from_ascii_copy(data));
				// Wierd.. lets debug!
                assert(false);
            }
        }
    }

    return result;
}


void DBManager::SetToonDimension(unsigned int charid, unsigned int dimensionid)
{
    assert(charid != 0);
    assert(dimensionid != 0);
    m_db->Begin();
    m_db->Exec(STREAM2STR("UPDATE OR IGNORE tToons SET dimensionid = " << dimensionid << " WHERE charid = " << charid));
    m_db->Commit();
}


unsigned int DBManager::GetToonDimension(unsigned int charid) const
{
    assert(charid != 0);

    unsigned int result = 0;

    sqlite::ITablePtr pT = m_db->ExecTable(STREAM2STR("SELECT dimensionid FROM tToons WHERE charid = " << charid));
    if (pT != NULL && pT->Rows() > 0)
    {
        std::string dataStr = pT->Data(0, 0);
		const char* data = dataStr.c_str();
		if (data != nullptr && data[0] != '\0')
        {
			result = (unsigned int)atoi(data);

            // Maintain your debug check for malformed data
            if (result == 0 && data != "0")
            {
                LOG("Error in GetToonDimension(). Bad numeric data: " << data);
				// Wierd.. lets debug!
                assert(false);
            }
        }
    }

    return result;
}



void DBManager::SetToonStats(unsigned int charid, StatMap const& stats)
{
    assert(charid != 0);
    TRACE("Updating stats on character id " << charid);

    m_db->Begin();
    m_db->Exec(STREAM2STR("DELETE FROM tToonStats WHERE charid = " << charid));
    for (StatMap::const_iterator it = stats.begin(); it != stats.end(); ++it)
    {
        unsigned int statid = it->first;
        unsigned int statvalue = it->second;
        m_db->Exec(STREAM2STR("INSERT OR IGNORE INTO tToonStats VALUES (" << charid << ", " << statid << ", " << statvalue << ")"));
    }
    m_db->Commit();
}


bool DBManager::GetDimensions(std::map<unsigned int, std::tstring> &dimensions) const
{
    sqlite::ITablePtr pT = m_db->ExecTable(STREAM2STR("SELECT dimensionid, dimensionname FROM tDimensions"));

    if (pT != NULL && pT->Rows() > 0)
    {
        for (unsigned int i = 0; i < pT->Rows(); ++i)
        {
            std::string idData = pT->Data(i, 0);
            unsigned int dimId = (unsigned int)atoi(idData.c_str());

            // Validate numeric data as we did in previous functions
            if (dimId == 0 && idData != "0")
            {
				LOG("Error in GetDimensions(). Bad numeric data: " << from_ascii_copy(idData));
				// Wierd.. lets debug!
                assert(false);
                continue;
            }

            std::tstring name = from_ascii_copy(pT->Data(i, 1));
            dimensions[dimId] = name;
        }
        return true;
    }
    return false;
}



unsigned int DBManager::FindNextAvailableContainerSlot(unsigned int charId, unsigned int containerId)
{
    assert(charId != 0);
    assert(containerId != 0);

    unsigned short posSlot = 0;

    if (containerId == 2)
        posSlot = 64; // start at slot 64 for inventory!

    sqlite::ITablePtr pT = m_db->ExecTable(STREAM2STR("SELECT slot FROM tItems WHERE parent = " << containerId << " AND slot >= " << posSlot
        << " AND owner = " << charId << " ORDER by slot ASC"));

    if (pT == NULL || !pT->Rows())
        return 0; // If empty, return the starting position (0 or 64)

    unsigned int count = pT->Rows();

    for (unsigned i = 0; i < count; i++)
    {
        // Safe conversion using atoi (Standard C++)
		unsigned short slotInDB = (unsigned short)atoi(pT->Data(i, 0).c_str());

        if (posSlot < slotInDB)
        {
            return posSlot; // Found a gap
        }

        posSlot++;
    }

    return posSlot; // Return the next slot after the last occupied one
}



//returns the properties value in the AO db for an item in a particular slot in a container.
unsigned int DBManager::GetItemProperties(unsigned int charId, unsigned int containerId, unsigned int slot)
{
    assert(charId != 0);
    assert(containerId != 0);
    // slot >= 0 is always true for unsigned, but keeping for logic consistency

    unsigned int result = 0;

    std::tstringstream sql;
    sql << _T("SELECT properties FROM tItems JOIN tblAO ON keylow = aoid WHERE owner = ") << charId
        << _T(" AND parent = ") << containerId << _T(" AND slot = ") << slot;
		
    //OutputDebugString(sql.str().c_str());
	
    sqlite::ITablePtr pT = m_db->ExecTable(sql.str());

    if (pT != NULL && pT->Rows() > 0)
    {
        std::string dataStr = pT->Data(0, 0);
const char* data = dataStr.c_str();
	if (data != nullptr && data[0] != '\0')
        {
			result = (unsigned int)atoi(data);
        }
    }

    return result;
}


//searches for items in containerIdToSearchIn with the same keylow and ql as the item specified
unsigned int DBManager::FindFirstItemOfSameType(unsigned int charId, unsigned int containerId, unsigned int slot, unsigned int containerIdToSearchIn)
{
    assert(charId != 0);
    assert(containerId != 0);
    assert(slot >= 0);
    assert(containerIdToSearchIn != 0);

    unsigned int result = 0;

    std::tstringstream sql;
    sql << _T("SELECT tTarget.slot FROM tItems tTarget, tItems tSource WHERE tSource.owner = ") << charId
        << _T(" AND tSource.parent = ") << containerId 
        << _T(" AND tSource.slot = ") << slot
        << _T(" AND tTarget.keylow = tSource.keylow AND tTarget.ql = tSource.ql")
        << _T(" AND tTarget.owner = ") << charId
        << _T(" AND tTarget.parent = ") << containerIdToSearchIn
        << _T(" ORDER BY tTarget.slot LIMIT 1");

    //OutputDebugString(sql.str().c_str());

    sqlite::ITablePtr pT = m_db->ExecTable(sql.str());

    if (pT != NULL && pT->Rows() > 0)
    {
        std::string dataStr = pT->Data(0, 0);
const char* data = dataStr.c_str();
	if (data != nullptr && data[0] != '\0')
        {
			result = (unsigned int)atoi(data);
        }
    }

    return result;
}



void DBManager::DeleteItems(std::set<unsigned int> const& ids) const
{
    if (ids.empty())
    {
        return;
    }

    std::tstringstream idList;
    for (auto it = ids.begin(); it != ids.end(); ++it)
    {
        if (it != ids.begin())
        {
            idList << _T(",");
        }
        idList << *it;
    }

    std::tstringstream sql;
    sql << _T("DELETE FROM tItems WHERE itemidx IN(") << idList.str() << _T(")");

    m_db->Exec(sql.str());
}



unsigned int DBManager::GetShopOwner(unsigned int shopid)
{
    assert(shopid != 0);
    unsigned int result = 0;

    sqlite::ITablePtr pT = m_db->ExecTable(STREAM2STR("SELECT charid FROM tToons WHERE shopid = " << shopid));

    if (pT != NULL && pT->Rows() > 0)
    {
        std::string dataStr = pT->Data(0, 0);
const char* data = dataStr.c_str();
	if (data != nullptr && data[0] != '\0')
        {
			result = (unsigned int)atoi(data);
        }
    }

    return result;
}


OwnedItemInfoPtr DBManager::GetOwnedItemInfo(unsigned int itemID)
{
    OwnedItemInfoPtr pRetVal(new OwnedItemInfo());

    std::tstringstream sql;
    sql << _T("SELECT tItems.keylow AS itemloid, tItems.keyhigh AS itemhiid, tItems.ql AS itemql, name AS itemname, ")
        << _T("(SELECT tToons.charname FROM tToons WHERE tToons.charid = owner) AS ownername, owner AS ownerid, ")
        << _T("parent AS containerid, tItems.flags ")
        << _T("FROM tItems JOIN tblAO ON keylow = aoid WHERE itemidx = ") << (int)itemID;

    sqlite::ITablePtr pT = m_db->ExecTable(sql.str());

    pRetVal->itemloid = from_ascii_copy(pT->Data(0, 0));
    pRetVal->itemhiid = from_ascii_copy(pT->Data(0, 1));
    pRetVal->itemql = from_ascii_copy(pT->Data(0, 2));
    pRetVal->itemname = from_ascii_copy(pT->Data(0, 3));
    pRetVal->ownername = from_ascii_copy(pT->Data(0, 4));
    pRetVal->ownerid = from_ascii_copy(pT->Data(0, 5));
    pRetVal->containerid = from_ascii_copy(pT->Data(0, 6));
	pRetVal->flags = (unsigned short)atoi(pT->Data(0, 7).c_str());

    return pRetVal;


    return pRetVal;
}


unsigned int DBManager::getAODBSchemeVersion(std::tstring const& filename) const
{
    unsigned int retval = 0;
    sqlite::Db db(Logger::instance().stream());

    if (db.Init(filename))
    {
        try
        {
            sqlite::ITablePtr pT = db.ExecTable(_T("SELECT Version FROM vSchemeVersion"));
            if (pT != NULL && pT->Rows() > 0)
            {
				retval = (unsigned int)atoi(pT->Data(0, 0).c_str());
            }
        }
        catch (sqlite::Db::QueryFailedException &/*e*/)
        {
            retval = 0;
        }
    }

    return retval;
}



unsigned int DBManager::getDBVersion() const
{
    unsigned int retval = 0;

    try 
    {
        sqlite::ITablePtr pT = m_db->ExecTable(_T("SELECT Version FROM vSchemeVersion"));
        if (pT != NULL && pT->Rows() > 0)
        {
			retval = (unsigned int)atoi(pT->Data(0, 0).c_str());
        }
    }
    catch (sqlite::Db::QueryFailedException &/*e*/)
    {
        retval = 0;
    }

    return retval;
}



void DBManager::updateDBVersion(unsigned int fromVersion) const
{
    switch (fromVersion)
    {
    case 0:
        {
            m_db->Begin();
            m_db->Exec(_T("CREATE TABLE tToons2 (charid, charname)"));
            m_db->Exec(_T("INSERT INTO tToons2 (charid, charname) SELECT charid, charname FROM tToons"));
            m_db->Exec(_T("DROP TABLE tToons"));
            m_db->Exec(_T("CREATE TABLE tToons (charid, charname)"));
            m_db->Exec(_T("CREATE UNIQUE INDEX iCharId ON tToons (charid)"));
            m_db->Exec(_T("INSERT INTO tToons (charid, charname) SELECT charid, charname FROM tToons2"));
            m_db->Exec(_T("DROP TABLE tToons2"));
            m_db->Commit();
        }
        // Dropthrough

    case 1:
        {
            m_db->Begin();
            m_db->Exec(_T("CREATE TABLE tToons2 (charid INTEGER NOT NULL PRIMARY KEY UNIQUE, charname VARCHAR)"));
            m_db->Exec(_T("INSERT INTO tToons2 (charid, charname) SELECT charid, charname FROM tToons"));
            m_db->Exec(_T("DROP TABLE tToons"));
            m_db->Exec(_T("CREATE TABLE tToons (charid INTEGER NOT NULL PRIMARY KEY UNIQUE, charname VARCHAR)"));
            m_db->Exec(_T("INSERT INTO tToons (charid, charname) SELECT charid, charname FROM tToons2"));
            m_db->Exec(_T("DROP TABLE tToons2"));
            m_db->Exec(_T("CREATE UNIQUE INDEX iCharId ON tToons (charid)"));
            m_db->Commit();
        }
        // Dropthrough

    case 2: // Update from v2 is the added shopid column in the toons table
        {
            m_db->Begin();
            m_db->Exec(_T("CREATE TABLE tToons2 (charid INTEGER NOT NULL PRIMARY KEY UNIQUE, charname VARCHAR, shopid INTEGER DEFAULT '0')"));
            m_db->Exec(_T("INSERT INTO tToons2 (charid, charname) SELECT charid, charname FROM tToons"));
            m_db->Exec(_T("DROP TABLE tToons"));
            m_db->Exec(_T("CREATE TABLE tToons (charid INTEGER NOT NULL PRIMARY KEY UNIQUE, charname VARCHAR, shopid INTEGER DEFAULT '0')"));
            m_db->Exec(_T("INSERT INTO tToons (charid, charname) SELECT charid, charname FROM tToons2"));
            m_db->Exec(_T("DROP TABLE tToons2"));
            m_db->Exec(_T("CREATE UNIQUE INDEX iCharId ON tToons (charid)"));
            m_db->Commit();
        }
        // Dropthrough

    case 3: // Update from v3 is the added flags column in tItems as well as the new dimension table and a new dimensionid column in tToons.
        {
            m_db->Begin();
            m_db->Exec(_T("CREATE TABLE tItems2 (itemidx INTEGER NOT NULL PRIMARY KEY ON CONFLICT REPLACE AUTOINCREMENT UNIQUE DEFAULT '1', keylow INTEGER, keyhigh INTEGER, ql INTEGER, flags INTEGER DEFAULT '0', stack INTEGER DEFAULT '1', parent INTEGER NOT NULL DEFAULT '2', slot INTEGER, children INTEGER, owner INTEGER NOT NULL)"));
            m_db->Exec(_T("INSERT INTO tItems2 (itemidx, keylow, keyhigh, ql, stack, parent, slot, children, owner) SELECT itemidx, keylow, keyhigh, ql, stack, parent, slot, children, owner FROM tItems"));
            m_db->Exec(_T("DROP TABLE tItems"));
            m_db->Exec(_T("CREATE TABLE tItems (itemidx INTEGER NOT NULL PRIMARY KEY ON CONFLICT REPLACE AUTOINCREMENT UNIQUE DEFAULT '1', keylow INTEGER, keyhigh INTEGER, ql INTEGER, flags INTEGER DEFAULT '0', stack INTEGER DEFAULT '1', parent INTEGER NOT NULL DEFAULT '2', slot INTEGER, children INTEGER, owner INTEGER NOT NULL)"));
            m_db->Exec(_T("INSERT INTO tItems (itemidx, keylow, keyhigh, ql, stack, parent, slot, children, owner) SELECT itemidx, keylow, keyhigh, ql, stack, parent, slot, children, owner FROM tItems2"));
            m_db->Exec(_T("DROP TABLE tItems2"));
            m_db->Exec(_T("CREATE TABLE tDimensions (dimensionid INTEGER NOT NULL PRIMARY KEY UNIQUE, dimensionname VARCHAR)"));
            m_db->Exec(_T("INSERT INTO tDimensions (dimensionid, dimensionname) VALUES (13, 'Rubi-Ka')"));
            m_db->Exec(_T("INSERT INTO tDimensions (dimensionid, dimensionname) VALUES (15, 'TestLive')"));
            m_db->Exec(_T("CREATE TABLE tToons2 (charid INTEGER NOT NULL PRIMARY KEY UNIQUE, charname VARCHAR, shopid INTEGER DEFAULT '0', dimensionid INTEGER DEFAULT '0')"));
            m_db->Exec(_T("INSERT INTO tToons2 (charid, charname, shopid) SELECT charid, charname, shopid FROM tToons"));
            m_db->Exec(_T("DROP TABLE tToons"));
            m_db->Exec(_T("CREATE TABLE tToons (charid INTEGER NOT NULL PRIMARY KEY UNIQUE, charname VARCHAR, shopid INTEGER DEFAULT '0', dimensionid INTEGER DEFAULT '0')"));
            m_db->Exec(_T("INSERT INTO tToons (charid, charname, shopid) SELECT charid, charname, shopid FROM tToons2"));
            m_db->Exec(_T("DROP TABLE tToons2"));
            m_db->Exec(_T("CREATE UNIQUE INDEX iCharId ON tToons (charid)"));
            m_db->Commit();
        }
        // Dropthrough

    case 4: // Updates from v4: Added toon-stats table.
        {
            m_db->Begin();
            m_db->Exec(_T("CREATE TABLE tToonStats (charid INTEGER NOT NULL, statid INTEGER NOT NULL, statvalue INTEGER NOT NULL, FOREIGN KEY (charid) REFERENCES tToons (charid))"));
            m_db->Exec(_T("CREATE UNIQUE INDEX charstatindex ON tToonStats (charid ASC, statid ASC)"));
            m_db->Commit();
        }
        // Dropthrough

    case 5: // Updates from v5: Added index to tItems
        {
            m_db->Begin();
            m_db->Exec("CREATE INDEX idx_titems_keylow ON tItems (keylow ASC)");
            m_db->Commit();
        }
        // Dropthrough

    case 6: // Updates from v6: Redid missing indexes from previous updates.
        {
            m_db->Begin();
            m_db->Exec("DROP INDEX IF EXISTS iOwner");
            m_db->Exec("DROP INDEX IF EXISTS iParent");
            m_db->Exec("CREATE INDEX idx_titems_owner ON tItems (owner)");
            m_db->Exec("CREATE INDEX idx_titems_parent ON tItems (parent)");
            m_db->Commit();
        }
        // Dropthrough

	case 7: // Updates from v7: Include 'Account' in tToons
		{
            m_db->Begin();
			m_db->Exec("DELETE * FROM tDimensions");
			m_db->Exec("INSERT INTO tDimensions (dimensionid, dimensionname) VALUES (13, 'Rubi-Ka')");
			m_db->Exec("INSERT INTO tDimensions (dimensionid, dimensionname) VALUES (15, 'TestLive')");
            m_db->Commit();
		}
		// Dropthrough

    default:
        m_db->Exec("DROP VIEW IF EXISTS vSchemeVersion");
        m_db->Exec(STREAM2STR(_T("CREATE VIEW vSchemeVersion AS SELECT '") << CURRENT_DB_VERSION << _T("' AS Version")));
        break;
    }
}


void DBManager::createDBScheme() const
{
    LOG("DBManager::createDBScheme: Creating a new database scheme from scratch.");

    m_db->Begin();
    m_db->Exec("CREATE TABLE tItems (itemidx INTEGER NOT NULL PRIMARY KEY ON CONFLICT REPLACE AUTOINCREMENT UNIQUE DEFAULT '1', keylow INTEGER, keyhigh INTEGER, ql INTEGER, flags INTEGER DEFAULT '0', stack INTEGER DEFAULT '1', parent INTEGER NOT NULL DEFAULT '2', slot INTEGER, children INTEGER, owner INTEGER NOT NULL)");
    m_db->Exec("CREATE INDEX idx_titems_keylow ON tItems (keylow ASC)");
    m_db->Exec("CREATE INDEX idx_titems_owner ON tItems (owner)");
    m_db->Exec("CREATE INDEX idx_titems_parent ON tItems (parent)");
    m_db->Exec("CREATE VIEW vBankItems AS SELECT * FROM tItems WHERE parent=1");
    m_db->Exec("CREATE VIEW vContainers AS SELECT * FROM tItems WHERE children > 0");
    m_db->Exec("CREATE VIEW vInvItems AS SELECT * FROM tItems WHERE parent=2");
    m_db->Exec("CREATE TABLE tToons (charid INTEGER NOT NULL PRIMARY KEY UNIQUE, charname VARCHAR, shopid INTEGER DEFAULT '0', dimensionid INTEGER DEFAULT '0')");
    m_db->Exec("CREATE UNIQUE INDEX iCharId ON tToons (charid)");
    m_db->Exec("CREATE TABLE tDimensions (dimensionid INTEGER NOT NULL PRIMARY KEY UNIQUE, dimensionname VARCHAR)");
    m_db->Exec("INSERT INTO tDimensions (dimensionid, dimensionname) VALUES (13, 'Rubi-Ka')");
    m_db->Exec("INSERT INTO tDimensions (dimensionid, dimensionname) VALUES (15, 'TestLive')");
    m_db->Exec("CREATE TABLE tToonStats (charid INTEGER NOT NULL, statid INTEGER NOT NULL, statvalue INTEGER NOT NULL, FOREIGN KEY (charid) REFERENCES tToons (charid))");
    m_db->Exec("CREATE UNIQUE INDEX charstatindex ON tToonStats (charid ASC, statid ASC)");
    m_db->Exec(STREAM2STR(_T("CREATE VIEW vSchemeVersion AS SELECT '") << CURRENT_DB_VERSION << _T("' AS Version")));
    m_db->Commit();
}
