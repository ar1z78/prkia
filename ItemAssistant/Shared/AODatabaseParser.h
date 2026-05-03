#ifndef AODatabaseParser_h__
#define AODatabaseParser_h__

#include <memory>
#include <fstream>
#include <shared/AODB.h>
#include <string>
#include <sqlite3/sqlite3.h>
#include <map>
#include <vector>
#include <cstdint>

class AODatabaseParser
{
public:
	struct Exception : public std::exception {
		Exception(std::string const& what) : std::exception(what.c_str()) {}
	};

	// Updated to take a single string (the path to rdb.db)
	AODatabaseParser(std::string const& rdbPath);
	~AODatabaseParser();

	// Retrieves the item by its Resource ID (formerly 'offset')
	// UPDATED: Now returns std::shared_ptr
	std::shared_ptr<ao_item> GetItem(unsigned int id, const char* tableName) const;

protected:
	// These are kept for compatibility but will be empty in the .cpp
	void EnsureFileOpen(uint64_t offset) const;
	void OpenFileFromOffset(uint64_t offset) const;

private:
	// This is our new SQLite connection for PRK RDB
	mutable sqlite3* m_rdb;
	// UPDATED: Standard way to manage a buffer array
	mutable std::unique_ptr<char[]> m_buffer;

	// Legacy variables kept so other files don't break during compilation
	typedef std::map<uint64_t, std::string> FileOffsetToStringMap;
	FileOffsetToStringMap m_file_offsets;
	mutable uint64_t m_current_file_offset;
	mutable uint64_t m_current_file_size;
	mutable std::ifstream m_file;
};

#endif // AODatabaseParser_h__

