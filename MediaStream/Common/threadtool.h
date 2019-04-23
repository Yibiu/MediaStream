#pragma once
#include <stdint.h>
#include <thread>


/**
* @brief:
* Thread class
*/
class CThread
{
public:
	CThread()
	{
		_thread_ptr = NULL;
	}
	virtual ~CThread() {}

	bool start()
	{
		_thread_ptr = new std::thread(thread_proc_internal, this);
		if (NULL == _thread_ptr)
			return false;

		return true;
	}
	void stop()
	{
		if (NULL != _thread_ptr) {
			_thread_ptr->join();
			delete _thread_ptr;
			_thread_ptr = NULL;
		}
	}

	virtual void run() = 0;

	static void thread_proc_internal(void *param)
	{
		CThread *this_ptr = (CThread *)param;
		if (NULL != this_ptr)
			this_ptr->run();
	}

protected:
	std::thread *_thread_ptr;
};

