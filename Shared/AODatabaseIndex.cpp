#include "StdAfx.h"
#include "AODatabaseIndex.h"
#include <windows.h>
#include <algorithm>

AODatabaseIndex::AODatabaseIndex(std::string const& index_file, std::set<ResourceType>& types)
{
    ReadIndexFile(index_file, types);
}

AODatabaseIndex::~AODatabaseIndex()
{
}

std::vector<unsigned int> AODatabaseIndex::GetOffsets(ResourceType type) const
{
    std::vector<unsigned int> retval;
    ResourceTypeMap::const_iterator resourceIt = m_record_index.find(type);
    if (resourceIt != m_record_index.end())
    {
        for (IdOffsetMap::const_iterator it = resourceIt->second.begin(); it != resourceIt->second.end(); ++it)
        {
            retval.insert(retval.end(), it->second.begin(), it->second.end());
        }
        std::sort(retval.begin(), retval.end());
    }
    return retval;
}

void AODatabaseIndex::ReadIndexFile(std::string filename, std::set<ResourceType>& types)
{
    HANDLE hFile = CreateFileA(filename.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return;

    HANDLE hMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (hMap == NULL) {
        CloseHandle(hFile);
        return;
    }

    const char* pBegin = (const char*)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
    if (pBegin == NULL) {
        CloseHandle(hMap);
        CloseHandle(hFile);
        return;
    }

    // Get file size to calculate the end pointer
    LARGE_INTEGER fileSize;
    GetFileSizeEx(hFile, &fileSize);
    const char* pEnd = pBegin + fileSize.QuadPart;

    const char* current_pos = pBegin;

    // Extract some info from the header
    // Info found by tdb @ AODevs.
    unsigned int data_start = *(unsigned int*)(current_pos + 0x48);
    unsigned int block_size = *(unsigned int*)(current_pos + 0x0c);

    // Skip to data_start
    current_pos = pBegin + data_start;

    while (current_pos + block_size <= pEnd)
    {
        unsigned int next_block = ReadIndexBlock(current_pos, std::min<const char*>(pEnd, current_pos + block_size), types);

        if (next_block > 0)
        {
            current_pos = pBegin + next_block;
        }
        else
        {
            break;
        }
    }

    // Cleanup
    UnmapViewOfFile(pBegin);
    CloseHandle(hMap);
    CloseHandle(hFile);
}

unsigned int AODatabaseIndex::ReadIndexBlock(const char* pos, const char* end, std::set<ResourceType>& types)
{
    // Index blocks are stored in a double linked list. 
    // Extract pointer to next block.
    unsigned int next_block = *(unsigned int*)pos;
    // Extract pointer to previous block.
    unsigned int prev_block = *(unsigned int*)(pos + 4);

    // Number of active record indexes in this block
    unsigned short count = *(unsigned short*)(pos + 8);

    // Skip block header
    pos += 0x12 + 10;
    
    const unsigned int* int_pos = (unsigned int*)pos;
    while (count-- > 0 && (const char*)(int_pos + 4) <= end) // Standardized boundary check
    {
        int_pos++; // skip ignored
        unsigned int offset = *int_pos++;
        unsigned int resource_type = _byteswap_ulong(*int_pos++);
        unsigned int resource_id = _byteswap_ulong(*int_pos++);

        if (types.empty() || types.find((ResourceType)resource_type) != types.end())
        {
            m_record_index[(ResourceType)resource_type][resource_id].insert(offset);
        }
    }

    return next_block;
}
