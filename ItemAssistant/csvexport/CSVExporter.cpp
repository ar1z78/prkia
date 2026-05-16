#include "stdafx.h"
#include "CSVExporter.h"
#include "StringUtil.h"

namespace aoia
{
    CSVExporter::CSVExporter()
    {
    }


    CSVExporter::~CSVExporter()
    {
    }


    void CSVExporter::DoExport(std::tostream &os, IDataGridModelPtr data) const
    {
        std::tstring separator = _T("\n");

        // Write the column headers
        for (unsigned int column_index = 0; column_index < data->getColumnCount(); ++column_index)
        {
            if (column_index > 0)
            {
                os << _T(",");
            }
            os << data->getColumnName(column_index);
        }
        os << separator;
        
        // Write one row for each item to export.
        for (unsigned int row_index = 0; row_index < data->getItemCount(); ++row_index)
        {
            if (row_index > 0)
            {
                os << separator;
            }

            // Dump all the columns from the datamodel.
            for (unsigned int column_index = 0; column_index < data->getColumnCount(); ++column_index)
            {
                if (column_index > 0)
                {
                    os << _T(",");
                }
                std::tstring cell_value = data->getItemProperty(row_index, column_index);
                if (cell_value.find(_T(",")) != std::tstring::npos)
                {
                    // If the value of the cell contains a comma, we need to encapsulate the text in " symbols.
                    os << EnsureEncapsulation(cell_value);
                }
                else 
                {
                    os << cell_value;
                }
            }
        }
    }


    std::tstring CSVExporter::EnsureEncapsulation(std::tstring const& field) const
    {
        std::tstring retval;
        // Using standard StringUtil helpers (GCC/VS compatible)
        if (!StringUtil::starts_with(field, _T("\"")))
        {
            retval = _T("\"");
        }
        retval += field;
        if (!StringUtil::ends_with(field, _T("\"")))
        {
            retval += _T("\"");
        }
        return retval;
    }


}
