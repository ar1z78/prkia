#include "DataQueue.h"
#include <cstring> // For memcpy

DataItem::DataItem(unsigned long _type, unsigned int _size, char* _data)
	: m_type(_type)
	, m_size(_size)
{
	// CRITICAL for VS2013: Since m_data is std::shared_ptr<char>, 
	// we must pass std::default_delete<char[]>() so it uses delete[] and not delete.
	m_data.reset(new char[_size], std::default_delete<char[]>());
	memcpy(m_data.get(), _data, _size);
}

unsigned long DataItem::type() const
{
	return m_type;
}

char* DataItem::data() const
{
	return m_data.get();
}

unsigned int DataItem::size() const
{
	return m_size;
}

/************************************************************************/

DataQueue::DataQueue()
{
}

DataQueue::~DataQueue()
{
}

void DataQueue::push(DataItemPtr item)
{
	// Replaced boost::lock_guard with std::lock_guard
	std::lock_guard<std::mutex> guard(m_mutex);
	m_queue.push(item);
}

DataItemPtr DataQueue::pop()
{
	// Replaced boost::lock_guard with std::lock_guard
	std::lock_guard<std::mutex> guard(m_mutex);

	if (m_queue.empty()) {
		return nullptr;
	}

	DataItemPtr pData = m_queue.front();
	m_queue.pop();
	return pData;
}

// Thread-safe empty check
bool DataQueue::empty() const
{
	std::lock_guard<std::mutex> guard(m_mutex);
	return m_queue.empty();
}
