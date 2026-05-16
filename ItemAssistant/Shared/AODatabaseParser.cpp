#include "stdafx.h"
#include "AODatabaseParser.h"
#include "AOItemParser.h"
#include <algorithm>
#include <memory> // For std::shared_ptr and std::make_shared


#define BUFFER_SIZE 1024*1024

/*struct DataBaseRecordHeader
{
    unsigned int record_size;
    unsigned int payload_size;
    unsigned int payload_type;
}; */


// Constructor now takes the path to rdb.db instead of a list of .dat files
AODatabaseParser::AODatabaseParser(std::string const& rdbPath)
	: m_rdb(NULL)
	, m_current_file_offset(0) // Still need to initialize these to avoid warnings
	, m_current_file_size(0)
{ // <--- THIS BRACE WAS MISSING

	m_buffer.reset(new char[BUFFER_SIZE]); // Creating a buffer for record parsing.

	// In PRK, rdbPath should be the path to "cd_image/rdb.db"
	if (sqlite3_open(rdbPath.c_str(), &m_rdb) != SQLITE_OK)
	{
		throw Exception("Unable to open PRK rdb.db");
	}
}


AODatabaseParser::~AODatabaseParser()
{
    if (m_rdb)
    {
        sqlite3_close(m_rdb);
    }
}


// In the new logic, 'offset' is actually the Item ID from the RDB
std::shared_ptr<ao_item> AODatabaseParser::GetItem(unsigned int id, const char* tableName) const
{
    std::shared_ptr<ao_item> retval;

    // Query the BLOB for the specific ID
    sqlite3_stmt* stmt;
    // We build the SQL string with the provided table name
    std::string sql = "SELECT Data FROM ";
    sql += tableName;
    sql += " WHERE ID = ? LIMIT 1;";
    
    if (sqlite3_prepare_v2(m_rdb, sql.c_str(), -1, &stmt, NULL) != SQLITE_OK)
	{
        return retval;
    }

    sqlite3_bind_int(stmt, 1, id);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        // 1. Get the pointer and size
        const void* blob_ptr = sqlite3_column_blob(stmt, 0); // Use 0 if Data is the first column
        int blob_size = sqlite3_column_bytes(stmt, 0);


        if (blob_size > BUFFER_SIZE)
        {
            sqlite3_finalize(stmt);
            return retval;
        }
        // Copy the BLOB into the existing buffer to reuse AOItemParser logic
            memcpy(m_buffer.get(), blob_ptr, blob_size);

        try
        {
            // We pass the buffer to your existing AOItemParser
            // 1. Let the parser handle the blob data
             retval.reset(new AOItemParser(m_buffer.get(), blob_size, id));

        }
        catch (std::exception &e)
        {
            sqlite3_finalize(stmt);
            throw Exception(e.what());
        }
    }

    sqlite3_finalize(stmt);
    return retval;
}

// These functions are no longer needed for SQLite, keep them empty to avoid errors
void AODatabaseParser::EnsureFileOpen(uint64_t offset) const {}
void AODatabaseParser::OpenFileFromOffset(uint64_t offset) const {}