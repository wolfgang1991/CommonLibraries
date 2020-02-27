#ifndef THREADING_H_INCLUDED
#define THREADING_H_INCLUDED

// When grown up this is going to be an abstraction layer for threads (useful for platforms where posix threads are not available e.g. Windows)

#include <list>
#include <set>
#include <cstdint>
#include <pthread.h>//TODO other platforms without pthreads

typedef pthread_t Thread;
typedef pthread_mutex_t Mutex;

#define lockMutex(M) pthread_mutex_lock(&M)
#define unlockMutex(M) pthread_mutex_unlock(&M)
#define initMutex(M) pthread_mutex_init(&M, NULL)
#define deleteMutex(M) pthread_mutex_destroy(&M)
#define joinThread(T) pthread_join(T, NULL)==0

class ThreadPool;

//! stackSize in bytes (<0 => default)
bool createThread(Thread& outThread, void* (*start_routine)(void*), void* arg, bool joinable = false, int32_t stackSize = -1);

class PooledThread{

	private:
	
	Thread thread;
	
	Mutex mutex;
	void* (*start_routine)(void*);
	void* start_data;
	bool mustExit;
	
	static void* run(void* data);
	
	public:
	
	PooledThread();
	
	~PooledThread();
	
	//! must be called at beginning
	bool startIdle(ThreadPool* pool);
	
	//! true if successful
	bool startThreadedFunction(void* (*start_routine)(void*), void* start_data);
	
	//! exits the thread and deletes itself
	void exit();

};

class ThreadPool{
	friend PooledThread;

	private:
	
	Mutex mutex;
	std::list<PooledThread*> idleThreads;
	std::set<PooledThread*> runningThreads;
	
	//! called by PooledThread who finished it's calculation
	void returnThread(PooledThread* thread);
	
	public:
	
	ThreadPool(uint32_t initialThreadCount = 0);
	
	~ThreadPool();
	
	PooledThread* startThreadedFunction(void* (*start_routine)(void*), void* start_data);
	
	bool hasIdleThreads();
	
	bool hasRunningThreads();
	
};

#endif
