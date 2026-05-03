#ifndef DATAQUEUE_H
#define DATAQUEUE_H

#include <queue>
#include <memory>  // Replaces boost/shared_ptr and shared_array
#include <mutex>   // Replaces boost/thread

class DataItem
{
public:
	DataItem(unsigned long _type, unsigned int _size, char* _data);

	unsigned long type() const;
	char* data() const;
	unsigned int size() const;

private:
	// In VS2013, we use shared_ptr<char> with a custom deleter for arrays
	std::shared_ptr<char> m_data;
	unsigned int m_size;
	unsigned long m_type;
};

// DataItemPtr is now a standard shared pointer
typedef std::shared_ptr<DataItem> DataItemPtr;

/**
* This is a data queue class that supports multiple threads.
*/
class DataQueue
{
public:
	DataQueue();
	~DataQueue();

	/// This pushes a copy of the specified data on the queue.
	void push(DataItemPtr item);

	/// This pops the front of the queue.
	DataItemPtr pop();

	/// Returns true if no items on queue
	bool empty() const;

private:
	typedef std::queue<DataItemPtr> DataItemQueue;
	DataItemQueue m_queue;

	// Standard mutex
	mutable std::mutex m_mutex;
};

#endif // DATAQUEUE_H
