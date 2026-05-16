#include "stdafx.h"
#include "CSVDataModel.h"
#include <assert.h>
#include "StringUtil.h"
#include <string>
namespace aoia {

    CSVDataModel::CSVDataModel(sqlite::IDBPtr db, IContainerManagerPtr bp, std::tstring const& predicate, std::tstring const& link_template)
        : m_db(db)
        , m_bp(bp)
        , m_linkTemplate(link_template)
    {
        assert(m_db);
        runQuery(predicate);
    }


    CSVDataModel::CSVDataModel( sqlite::IDBPtr db, IContainerManagerPtr bp, std::set<unsigned int> const& ids, std::tstring const& link_template )
        : m_db(db)
        , m_bp(bp)
        , m_linkTemplate(link_template)
    {
        assert(m_db);

		std::tstringstream idsStream;
		for (auto it = ids.begin(); it != ids.end(); ++it)
		{
			if (it != ids.begin()) { idsStream << _T(", "); }
			idsStream << *it;
		}

		std::tstring predicate = _T("I.itemidx IN(");
		predicate += idsStream.str();
		predicate += _T(")");


        runQuery(predicate);
    }


    CSVDataModel::~CSVDataModel()
    {
    }


    void CSVDataModel::runQuery(std::tstring const& predicate)
    {
        std::tstring sql = 
            _T("SELECT ")
            _T("    A.name, ")
            _T("    I.ql, ")
            _T("    T.charname, ")
            _T("    I.parent, ")
            _T("    CASE ")
            _T("        WHEN parent = 1 THEN 'Bank' ")
            _T("        WHEN parent = 2 AND slot > 63 THEN 'Inventory' ")
            _T("        WHEN parent = 2 AND slot < 16 THEN 'Equipped (Weapons)' ")
            _T("        WHEN parent = 2 AND slot >= 16 AND slot < 32 THEN 'Equipped (Cloth)' ")
            _T("        WHEN parent = 2 AND slot >= 32 AND slot < 47 THEN 'Equipped (Implants)' ")
            _T("        WHEN parent = 2 AND slot >= 47 AND slot < 64 THEN 'Equipped (Social)' ")
            _T("        WHEN parent = 3 THEN 'Shop' ")
            _T("        WHEN I.parent > 10 THEN (SELECT ")  // This resolves container for a items in backpacks.
            _T("            CASE ")
            _T("                WHEN I2.parent = 1 THEN 'Bank' ")
            _T("                WHEN I2.parent = 2 AND I2.slot > 63 THEN 'Inventory' ")
            _T("                WHEN I2.parent = 2 AND I2.slot < 16 THEN 'Equipped (Weapons)' ")
            _T("                WHEN I2.parent = 2 AND I2.slot >= 16 AND I2.slot < 32 THEN 'Equipped (Cloth)' ")
            _T("                WHEN I2.parent = 2 AND I2.slot >= 32 AND I2.slot < 47 THEN 'Equipped (Implants)' ")
            _T("                WHEN I2.parent = 2 AND I2.slot >= 47 AND I2.slot < 64 THEN 'Equipped (Social)' ")
            _T("                WHEN I2.parent = 3 THEN 'Shop' ")
            _T("            END ")
            _T("            FROM tItems I2 WHERE I2.children = I.parent) ")
            _T("    END AS container, ")
            _T("    I.keylow, ")
            _T("    I.keyhigh, ")
            _T("    I.parent, ")
            _T("    A.aoid, ")
            _T("    I.itemidx, ")
            _T("    I.owner ")
            _T("FROM ")
            _T("    tItems I JOIN aodb.tblAO A ON I.keyhigh = A.aoid JOIN tToons T ON I.owner = T.charid ");

        if (!predicate.empty())
        {
            sql += _T(" WHERE ");
            sql += predicate;
        }

        m_result = m_db->ExecTable(sql);
        m_lastPredicate = predicate;
    }


    unsigned int CSVDataModel::getColumnCount() const
    {
        if (!m_result)
        {
            return 0;
        }

        return COL_COUNT;
    }


    std::tstring CSVDataModel::getColumnName(unsigned int index) const
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
            return _T("Value");
        case COL_TOON_NAME:
            return _T("Character");
        case COL_BACKPACK_NAME:
            return _T("Backpack");
        case COL_BACKPACK_LOCATION:
            return _T("Location");
        case COL_LOW_ID:
            return _T("LowID");
        case COL_HIGH_ID:
            return _T("HighID");
        case COL_CONTAINER_ID:
            return _T("ContainerID");
        case COL_LINK:
            return _T("Link");
        default:
            return _T("");
        }
    }


    unsigned int CSVDataModel::getItemCount() const
    {
        if (!m_result)
        {
            return 0;
        }

        return m_result->Rows();
    }


    std::tstring CSVDataModel::getItemProperty(unsigned int index, unsigned int column) const
    {
        if (!m_result)
        {
            return _T("");
        }

        if (column == COL_BACKPACK_NAME)
        {
				unsigned int id = (unsigned int)_ttoi(from_ascii_copy(m_result->Data(index, COL_BACKPACK_NAME)).c_str());
				if (id <= 10) { return _T(""); } // If the item is in the bank or inventory we just display an empty name for the backpack.


				unsigned int charid = (unsigned int)_ttoi(from_ascii_copy(m_result->Data(index, COL_COUNT + 1)).c_str());
				return m_bp->GetContainerName(charid, id);
        }
        else if (column == COL_LINK)
        {
			std::tstring link(m_linkTemplate);
			StringUtil::replace_all(link, _T("%lowid%"), getItemProperty(index, COL_LOW_ID));
			StringUtil::replace_all(link, _T("%hiid%"), getItemProperty(index, COL_HIGH_ID));
			StringUtil::replace_all(link, _T("%ql%"), getItemProperty(index, COL_ITEM_QL));
			return link;
        }

        return from_ascii_copy(m_result->Data(index, column));
    }


    unsigned int CSVDataModel::getItemId(unsigned int index) const
    {
        if (!m_result)
        {
            return 0;
        }


		return (unsigned int)_ttoi(from_ascii_copy(m_result->Data(index, COL_COUNT - 1)).c_str());
    }


    unsigned int CSVDataModel::getItemIndex(unsigned int rowIndex) const
    {
        if (!m_result)
        {
            return 0;
        }

		return (unsigned int)_ttoi(from_ascii_copy(m_result->Data(rowIndex, COL_COUNT)).c_str());
    }


    void CSVDataModel::sortData(unsigned int columnIndex, bool ascending)
    {
    }

}
