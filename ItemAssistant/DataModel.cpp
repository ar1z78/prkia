#include "StdAfx.h"
#include "DataModel.h"
#include "DBManager.h"
#include "Shared/SQLite.h"
#include <vector>
#include <string>

namespace aoia { namespace sv {

    DataModel::DataModel(sqlite::IDBPtr db)
        : m_db(db)
    {
        // Sets up the map between column-index and title.
        m_columnTitles.clear();
        m_columnTitles[0] = _T("Toon");
        m_columnTitles[1] = _T("Levels");
        m_columnTitles[2] = _T("AI Levels");
        m_columnTitles[3] = _T("Credits");
        m_columnTitles[4] = _T("VP");
        m_columnTitles[5] = _T("Spec");

        // Set ut a list of the statids each column should be bound to. (Hardcoded to skip "toon name" column further down.)
        m_statids = { 54, 169, 61, 669, 182 };

        std::tostringstream statids;
        for (unsigned int i = 0; i < m_statids.size(); ++i) {
            if (i > 0) {
                statids << ", ";
            }
            statids << m_statids.at(i);
        }

        sqlite::ITablePtr toons = m_db->ExecTable(STREAM2STR("SELECT charid, charname FROM tToons"));
        for (unsigned int i = 0; i < toons->Rows(); ++i)
        {
            DataModelItem item;
            item.charid = (unsigned int)_ttoi(from_ascii_copy(toons->Data(i, 0)).c_str());
            item.name = from_ascii_copy(toons->Data(i, 1));

            sqlite::ITablePtr stats = m_db->ExecTable(STREAM2STR("SELECT statid, statvalue FROM tToonStats WHERE charid = " << item.charid << " AND statid IN("<< statids.str() << ")"));
            for (unsigned int j = 0; j < stats->Rows(); ++j)
            {
                unsigned int id = (unsigned int)_ttoi(from_ascii_copy(stats->Data(j, 0)).c_str());
                item.stats[id] = from_ascii_copy(stats->Data(j, 1));
            }

            m_items.push_back(item);
        }
    }

    DataModel::~DataModel()
    {
    }

    unsigned int DataModel::getColumnCount() const
    {
        if (m_items.empty()) {
            return 0;
        }

        return m_columnTitles.size();
    }

    std::tstring DataModel::getColumnName( unsigned int index ) const
    {
        if (m_items.empty()) {
            return _T("");
        }

        std::map<unsigned int, std::tstring>::const_iterator it = m_columnTitles.find(index);
        if (it != m_columnTitles.end())
        {
            return it->second;
        }

        LOG("aoia::sv::DataModel::getColumnName() : Invalid index supplied.");
        return _T("");
    }

    unsigned int DataModel::getItemCount() const
    {
        if (m_items.empty()) {
            return 0;
        }

        return m_items.size();
    }

    std::tstring DataModel::getItemProperty( unsigned int index, unsigned int column ) const
    {
        if (m_items.empty()) {
            return _T("");
        }

        DataModelItem const& item = m_items.at(index);

        if (column == 0) {
            return item.name;
        }
        --column;

        std::map<unsigned int, std::tstring>::const_iterator it = item.stats.find(m_statids.at(column));
        if (it != item.stats.end()) {
            return it->second;
        }
        return _T("-");
    }

}}  // end of namespace
