#ifndef LRUCache_H_INCLUDED
#define LRUCache_H_INCLUDED

#include <list>
#include <map>

//! a LRU (Least Recently Used) Cache
template <class A, class B>
class LRUCache{

	private:

	typedef std::list<A> List;
	typedef std::pair<B, typename List::iterator> Pair;
	typedef std::map<A, Pair > Map;

	Map cache;//index->(element, iterator in list)
	List lrulist;//List of indices (the least recently used element is the front element)
	int lrulistsize;//size for O(1) for some STL implementations
	int maxelements;

	unsigned int cacheMissCounter;//Zähler für Optimierungszwecke
	
	//! removes all elements which cause an oversize
	void removeOversize(){
		while(lrulistsize>maxelements){
			deleteElement(lrulist.front());
			lrulist.pop_front();
			lrulistsize--;
		}
	}

	public:

	typedef typename std::map<A, std::pair<B, typename std::list<A>::iterator> >::iterator iterator;

	LRUCache(int maxElements){
		cacheMissCounter=0;
		lrulistsize=0;
		maxelements=maxElements;
	}

	iterator begin(){
		return cache.begin();
	}

	iterator end(){
		return cache.end();
	}

	A& getKeyFromIterator(iterator& it){
		return it->first;
	}

	B& getValueFromIterator(iterator& it){
		return it->second.first;
	}

	int getMaxCacheSize(){
		return maxelements;
	}

	void setMaxCacheSize(int me){
		maxelements = me;
		removeOversize();
	}

	//! O(1)
	int getCachedElementCount(){
		return lrulistsize;
	}
	
	//! O(logn)
	void deleteElement(A delidx){
		typename Map::iterator it = cache.find(delidx);
		if(it != cache.end()){
			delete it->second.first;
			cache.erase(it);
		}
	}

	//! O(logn)
	void addElement(A idx, B ele){
		lrulist.push_back(idx);
		lrulistsize++;
		typename List::iterator itlist = lrulist.end();
		--itlist;
		cache[idx] = std::make_pair(ele, itlist);
		removeOversize();
	}

	//! returns element or NULL if element not available; O(logn)
	B getElement(A idx){
		typename Map::iterator it = cache.find(idx);
		if(it != cache.end()){//es ist im cache
			lrulist.erase(it->second.second);
			lrulist.push_back(idx);
			it->second.second = lrulist.end();
			--(it->second.second);
			return it->second.first;
		}else{
			cacheMissCounter++;
			return NULL;
		}
	}

	//! Useful for performance optimization
	unsigned int getCacheMissCount(){
		return cacheMissCounter;
	}

	//! O(n)
	void clear(){
		typename Map::iterator it = cache.begin();
		while(it != cache.end()){
			delete (it->second.first);
			++it;
		}
		cache.clear();
		lrulist.clear();
		lrulistsize = 0;
	}

	~LRUCache(){
		clear();
	}

};

#endif
