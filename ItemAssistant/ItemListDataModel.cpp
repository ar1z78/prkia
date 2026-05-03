#include "StdAfx.h"
#include "ItemListDataModel.h"
#include "DBManager.h"
#include <AOManager.h>

namespace aoia 
{

//    ItemListDataModel::ItemListDataModel(sqlite::IDBPtr db, IContainerManagerPtr containerManager, 
	ItemListDataModel::ItemListDataModel(sqlite::IDBPtr db, IContainerManagerPtr containerManager,
		aoia::ISettingsPtr settings, std::tstring const& predicate, unsigned int sortColumnIndex, bool sortAscending)
		: m_db(db)
		, m_containerManager(containerManager)
		, m_settings(settings) // Initialize here
	{
		runQuery(predicate, sortColumnIndex, sortAscending);
	}

	// Constructor 2
	ItemListDataModel::ItemListDataModel(sqlite::IDBPtr db, IContainerManagerPtr containerManager,
		aoia::ISettingsPtr settings, std::set<unsigned int> const& aoids, unsigned int sortColumnIndex, bool sortAscending)
		: m_db(db)
		, m_containerManager(containerManager)
		, m_settings(settings) // Initialize here
	{
        std::tstringstream aoid_array;
        for (std::set<unsigned int>::const_iterator it = aoids.begin(); it != aoids.end(); ++it)
        {
            if (it != aoids.begin())
            {
                aoid_array << ",";
            }
            aoid_array << *it;
        }

        std::tstring predicate = _T("A.aoid IN (") + aoid_array.str() + _T(")");
        runQuery(predicate, sortColumnIndex, sortAscending);
    }


    ItemListDataModel::~ItemListDataModel()
    {
    }


	void ItemListDataModel::runQuery(std::tstring const& predicate, int sortColumn, bool sortAscending)
	{
		// Calculate Multiplier for Selling
		SellerSettings s = AOManager::instance().getSellerSettings();
		// Now use the struct members:
		int clSkill = s.clSkill;
		bool isOmni = s.isOmni;
		bool isTrader = s.isTrader;

		double sellMultiplier = 0.04; //Neut or Clan Terminal
		if (isOmni) sellMultiplier = 0.06; // NL Miiir shop or Omni Terminal
		if (isTrader) sellMultiplier = 0.07; //Trader Terminal

		// Add 1% (0.01) for every 40 points of CL.
		double skillBonus = (clSkill / 40.0);
		if (skillBonus > 17.0) skillBonus = 17.0;

		m_cachedMultiplier = sellMultiplier + (skillBonus / 100.0 * sellMultiplier);

		// 2. Build Optimized SQL
		std::tstring sql =
			_T("SELECT ")
			_T("    AL.name, I.ql, A.ivalue, T.charname, I.parent, ")
			_T("    (SELECT CASE ") // Sub-query for container name stays, but JOINs are better for prices
			_T("        WHEN I2.parent = 1 THEN 'Bank' ")
			_T("        WHEN I2.parent = 2 AND I2.slot > 63 THEN 'Inventory' ")
			_T("        WHEN I2.parent = 2 AND I2.slot < 16 THEN 'Equipped (Weapons)' ")
			_T("        WHEN I2.parent = 2 AND I2.slot >= 16 AND I2.slot < 32 THEN 'Equipped (Cloth)' ")
			_T("        WHEN I2.parent = 2 AND I2.slot >= 32 AND I2.slot < 47 THEN 'Equipped (Implants)' ")
			_T("        WHEN I2.parent = 2 AND I2.slot >= 47 AND I2.slot < 64 THEN 'Equipped (Social)' ")
			_T("        WHEN I2.parent = 3 THEN 'Shop' ")
			_T("     END FROM tItems I2 WHERE I2.children = I.parent) AS container, ")
			_T("    T.charid, I.itemidx, A.aoid, I.keylow, I.keyhigh, ")
			_T("    AL.ivalue AS vLow, AH.ivalue AS vHigh, AL.ql AS qlLow, AH.ql AS qlHigh ") // New Pricing Columns
			_T("FROM tItems I ")
			_T("JOIN aodb.tblAO A ON I.keyhigh = A.aoid ")
			_T("JOIN tToons T ON I.owner = T.charid ")
			_T("LEFT JOIN aodb.tblAO AL ON I.keylow = AL.aoid ")  // Join for Low Price
			_T("LEFT JOIN aodb.tblAO AH ON I.keyhigh = AH.aoid "); // Join for High Price

		if (!predicate.empty()) {
			sql += _T(" WHERE ") + predicate;
		}



        std::tstring sort_predicate;
        if (sortColumn > -1 && sortColumn < COL_COUNT)
        {
            sort_predicate = _T(" ORDER BY ");
            sort_predicate += STREAM2STR(sortColumn + 1);
        }

        if (!sort_predicate.empty())
        {
            sort_predicate += sortAscending ? _T(" ASC ") : _T(" DESC ");
            sql += sort_predicate;
        }

        m_result = m_db->ExecTable(sql);
        m_lastQuery = sql;
        m_lastPredicate = predicate;
    }


    unsigned int ItemListDataModel::getColumnCount() const
    {
        if (!m_result)
        {
            return 0;
        }

        return COL_COUNT;
    }


    std::tstring ItemListDataModel::getColumnName(unsigned int index) const
    {
        if (!m_result)
        {
            return _T("");
        }

        switch (index)
        {
        case COL_ITEM_NAME:
            return _T("Item Name");
        case COL_ITEM_QL:
            return _T("QL");
        case COL_ITEM_VALUE:
            return _T("Sell Value");
        case COL_TOON_NAME:
            return _T("Character");
        case COL_BACKPACK_NAME:
            return _T("Backpack");
        case COL_BACKPACK_LOCATION:
            return _T("Location");
        default:
            return _T("");
        }
    }


    unsigned int ItemListDataModel::getItemCount() const
    {
        if (!m_result)
        {
            return 0;
        }

        return m_result->Rows();
    }

	
	
	bool ItemListDataModel::getItemContainerId(unsigned int index, unsigned int& charid, unsigned int& containerid) const {
		if (!m_result || index >= m_result->Rows()) {
			return false;
		}

		std::string containerData = m_result->Data(index, COL_BACKPACK_NAME);
		std::string charData = m_result->Data(index, 6);

		// atoi returns 0 on failure. We check if it's 0 but the string wasn't "0" to detect errors.
		containerid = (unsigned int)atoi(containerData.c_str());
		charid = (unsigned int)atoi(charData.c_str());

		// Basic validation: if IDs are 0 and strings aren't "0", something went wrong.
		if ((containerid == 0 && containerData != "0") || (charid == 0 && charData != "0")) {
			return false;
		}

		return true;
	}

	void ItemListDataModel::RefreshCachedSettings() {
		// Re-run the multiplier logic we put in runQuery
		SellerSettings s = AOManager::instance().getSellerSettings();

		// Now use the struct members:
		int clSkill = s.clSkill;
		bool isOmni = s.isOmni;
		bool isTrader = s.isTrader;

		double sellMultiplier = 0.04;
		if (isOmni) sellMultiplier = 0.06;
		if (isTrader) sellMultiplier = 0.07;

		double skillBonus = (clSkill / 40.0);
		if (skillBonus > 17.0) skillBonus = 17.0;

		m_cachedMultiplier = sellMultiplier + (skillBonus / 100.0 * sellMultiplier);
	}

	std::tstring ItemListDataModel::getItemProperty(unsigned int index, unsigned int column) const
	{
		if (!m_result) {
			return _T("");
		}

		switch (column)
		{
		case COL_ITEM_VALUE:
		{
			// 1. Fetch raw database values
			long long vBase = _tstoi64(from_ascii_copy(m_result->Data(index, 11)).c_str());
			long long vNext = _tstoi64(from_ascii_copy(m_result->Data(index, 12)).c_str());
			int qlBase = _tstoi(from_ascii_copy(m_result->Data(index, 13)).c_str());
			int qlNext = _tstoi(from_ascii_copy(m_result->Data(index, 14)).c_str());
			int qlCurr = _tstoi(from_ascii_copy(m_result->Data(index, 1)).c_str());

			//if (vBase <= 0) return _T("---");

			// 2. Perform Quadratic Interpolation on raw base values
			double currentBaseValue = (double)vBase;
			int qlSpan = qlNext - qlBase;

			if (qlSpan > 0 && qlCurr > qlBase)
			{
				double k = (double)(vNext - vBase) / (double)(qlSpan * qlSpan);
				double dist = (double)(qlCurr - qlBase);
				currentBaseValue = (double)vBase + (k * dist * dist);
			}

			// 3. Multiply by m_cachedMultiplier and truncate (base it)
			// We use (long long) to truncate toward zero instead of rounding.
			long long sellPrice = (long long)(currentBaseValue * m_cachedMultiplier);
			std::tstringstream ss;
			ss << sellPrice;
			/*
			// 4. Format for display
			if (sellPrice <= 0) return _T("---");
			if (sellPrice >= 100000) ss << _T("= ") << sellPrice << _T(" =");
			else if (sellPrice < 1000) ss << _T("x ") << sellPrice << _T(" x");
			else ss << _T("- ") << sellPrice << _T(" -");
*/
			return ss.str();
		}

		case COL_BACKPACK_NAME:
		{
			unsigned int id = (unsigned int)_ttoi(from_ascii_copy(m_result->Data(index, COL_BACKPACK_NAME)).c_str());
			if (id <= 10) {
				return _T("");
			}
			// T.charid is at index 6 in SQL
			unsigned int charid = (unsigned int)_ttoi(from_ascii_copy(m_result->Data(index, 6)).c_str());
			return m_containerManager->GetContainerName(charid, id);
		}

		default:
			// For all other columns (Name, QL, etc.), just return the raw data
			return from_ascii_copy(m_result->Data(index, column));
		}
	}



    unsigned int ItemListDataModel::getItemId(unsigned int index) const
    {
        if (!m_result)
        {
            return 0;
        }

        // The AOID is selected as column index 7 in the query
        return (unsigned int)atoi(m_result->Data(index, 8).c_str());
    }


    unsigned int ItemListDataModel::getItemIndex(unsigned int rowIndex) const
    {
        if (!m_result)
        {
            return 0;
        }

        // The owned-item-index is selected as column index 6 in the query
        return (unsigned int)atoi(m_result->Data(rowIndex, 7).c_str());
    }


    void ItemListDataModel::sortData(unsigned int columnIndex, bool ascending)
    {
        // TODO: Add support for sorting the backpack column properly.
        runQuery(m_lastPredicate, columnIndex, ascending);
        signalCollectionUpdated();
    }


    void ItemListDataModel::DeleteItems( std::set<unsigned int> const& ids )
    {
        g_DBManager.Lock();
        g_DBManager.DeleteItems(ids);
        g_DBManager.UnLock();

        runQueryAgain();
        signalCollectionUpdated();
    }


    void ItemListDataModel::runQueryAgain()
    {
        if (m_lastQuery.empty())
        {
            m_result.reset();
            return;
        }

        m_result = m_db->ExecTable(m_lastQuery);
    }

}
