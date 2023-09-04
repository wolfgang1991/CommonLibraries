#include "Threading.h"
#include "timing.h"

#include <iostream>

struct ThreadPoolInfo{
	ThreadPool* pool;
	PooledThread* thread;
};

bool createThread(Thread& outThread, void* (*start_routine)(void*), void* arg, bool joinable, int32_t stackSize){
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	if(joinable){
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	}else{
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	}
	if(stackSize>0){
		pthread_attr_setstacksize(&attr, stackSize);
	}
	int res = pthread_create(&outThread, &attr, start_routine, arg);
	pthread_attr_destroy(&attr);
	return res==0;
}

PooledThread::PooledThread(){
	initMutex(mutex);
	start_routine = NULL;
	start_data = NULL;
	mustExit = false;
}

bool PooledThread::startIdle(ThreadPool* pool){
	return createThread(thread, PooledThread::run, new ThreadPoolInfo{pool, this}, false);
}

void PooledThread::exit(){
	lockMutex(mutex);
	mustExit = true;
	unlockMutex(mutex);
}
	
PooledThread::~PooledThread(){
	deleteMutex(mutex);
}
	
void* PooledThread::run(void* data){
	ThreadPoolInfo* info = (ThreadPoolInfo*)data;
	bool running = true;
	while(running){
		lockMutex(info->thread->mutex);
		running = !info->thread->mustExit;
		void* (*start_routine)(void*) = info->thread->start_routine;
		void* start_data = info->thread->start_data;
		unlockMutex(info->thread->mutex);
		if(running && start_routine!=NULL){
			start_routine(start_data);
			lockMutex(info->thread->mutex);
			info->thread->start_routine = NULL;
			info->thread->start_data = NULL;
			running = !info->thread->mustExit;
			unlockMutex(info->thread->mutex);
			if(running){
				info->pool->returnThread(info->thread);
			}
		}
		delay(1);
	}
	delete info->thread;
	delete info;
	return NULL;
}

bool PooledThread::startThreadedFunction(void* (*start_routine)(void*), void* start_data){
	bool success = false;
	lockMutex(mutex);
	if(this->start_routine==NULL){
		success = true;
		this->start_routine = start_routine;
		this->start_data = start_data;
	}
	unlockMutex(mutex);
	return success;
}

ThreadPool::ThreadPool(uint32_t initialThreadCount){
	initMutex(mutex);
	lockMutex(mutex);
	for(uint32_t i=0; i<initialThreadCount; i++){
		idleThreads.push_back(new PooledThread());
		if(!idleThreads.back()->startIdle(this)){
			delete idleThreads.back();
			idleThreads.pop_back();
		}
	}
	unlockMutex(mutex);
}
	
ThreadPool::~ThreadPool(){
	lockMutex(mutex);
	for(auto it=idleThreads.begin(); it!=idleThreads.end(); ++it){
		(*it)->exit();
	}
	for(auto it=runningThreads.begin(); it!=runningThreads.end(); ++it){
		(*it)->exit();
	}
	unlockMutex(mutex);
	delay(10);//give some time before deleting the mutex in case a thread is stuck at returnThread
	deleteMutex(mutex);
}
	
PooledThread* ThreadPool::startThreadedFunction(void* (*start_routine)(void*), void* start_data){
	lockMutex(mutex);
	PooledThread* pt = NULL;
	if(idleThreads.empty()){
		//std::cout << "new Thread" << std::endl;
		pt = new PooledThread();
		if(!pt->startIdle(this)){
			delete pt;
			unlockMutex(mutex);
			return NULL;
		}
	}else{
		//std::cout << "reuse Thread" << std::endl;
		pt = idleThreads.back();
		idleThreads.pop_back();
	}
	pt->startThreadedFunction(start_routine, start_data);
	runningThreads.insert(pt);
	unlockMutex(mutex);
	return pt;
}

void ThreadPool::returnThread(PooledThread* thread){
	//std::cout << "returnThread" << std::endl;
	lockMutex(mutex);
	auto it = runningThreads.find(thread);
	if(it!=runningThreads.end()){runningThreads.erase(it);}
	idleThreads.push_back(thread);
	unlockMutex(mutex);
}

bool ThreadPool::hasIdleThreads(){
	lockMutex(mutex);
	bool idle = !idleThreads.empty();
	unlockMutex(mutex);
	return idle;
}

bool ThreadPool::hasRunningThreads(){
	lockMutex(mutex);
	bool running = !runningThreads.empty();
	unlockMutex(mutex);
	return running;
}
