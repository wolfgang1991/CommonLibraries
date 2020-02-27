#ifndef UniqueIdentifierGenerator_H_INCLUDED
#define UniqueIdentifierGenerator_H_INCLUDED

#include <list>

template<typename T>
class UniqueIdentifierGenerator{
	
	private:
	
	T maxId;
	std::list<T> returnedIds;
	
	public:
	
	UniqueIdentifierGenerator(T initalStartId = static_cast<T>(0)):maxId(initalStartId){}
	
	//! gets a unique id (shall be returned after it is no longer used)
	T getUniqueId(){
		T res;
		if(returnedIds.empty()){
			res = maxId;
			maxId++;
		}else{
			res = returnedIds.front();
			returnedIds.pop_front();
		}
		return res;
	}
	
	//! returns a no longer used unique id
	void returnId(T id){
		returnedIds.push_back(id);
	}
	
};

#endif
