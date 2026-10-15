#ifndef UCMAP_H
#define UCMAP_H

#include <cstdint>

//! default hash function usable for integers (template specialization required for other types)
template<typename TKey>
uint32_t calculateUCMapIndex(const TKey& key, uint32_t arraySize){
	//return key%arraySize;
	uint32_t key32 = key;
	return (key32*(key32+3)) % arraySize;
}

template <typename TKey, typename TValue>
class DummyValueRemove{ public: void OnValueRemove(const TKey& key, const TValue& value){} };

//! Fixed-size hash map with compile-time array size and no heap allocations
//! If a new element is added (e.g. by operator[]) and the map is full, the least recently used element is removed
//! TReturnCallback::OnValueRemove(TKey& key, TValue& value) called when an element is removed from the map due to being the least recently used.
template<typename TKey, typename TValue, uint32_t arraySize, typename TSignedIndex = int32_t, typename TReturnCallback = DummyValueRemove<TKey, TValue>>
class UCMap{
	static_assert(arraySize>0, "UCMap requires arraySize > 0");

	struct Entry{
		TKey key;
		TValue value;
		TSignedIndex nextInBucket;
		TSignedIndex previousUsage;
		TSignedIndex nextUsage;
		bool used;
	};

	static constexpr TSignedIndex invalidIndex = -1;

	Entry entries[arraySize];
	TSignedIndex bucketHeads[arraySize];
	TSignedIndex unusedStack[arraySize];
	uint32_t unusedStackPtr;
	TSignedIndex mostRecentlyUsed;
	TSignedIndex leastRecentlyUsed;

	TReturnCallback* cbk = nullptr;

	void resetEntryLinks(TSignedIndex entryIndex){
		entries[entryIndex].nextInBucket = invalidIndex;
		entries[entryIndex].previousUsage = invalidIndex;
		entries[entryIndex].nextUsage = invalidIndex;
	}

	TSignedIndex findEntryIndex(const TKey& key) const{
		uint32_t bucketIndex = calculateUCMapIndex(key, arraySize);
		TSignedIndex entryIndex = bucketHeads[bucketIndex];
		while(entryIndex!=invalidIndex){
			if(entries[entryIndex].used && entries[entryIndex].key==key){
				return entryIndex;
			}
			entryIndex = entries[entryIndex].nextInBucket;
		}
		return invalidIndex;
	}

	void unlinkUsage(TSignedIndex entryIndex){
		Entry& entry = entries[entryIndex];
		if(entry.previousUsage!=invalidIndex){
			entries[entry.previousUsage].nextUsage = entry.nextUsage;
		}else{
			mostRecentlyUsed = entry.nextUsage;
		}
		if(entry.nextUsage!=invalidIndex){
			entries[entry.nextUsage].previousUsage = entry.previousUsage;
		}else{
			leastRecentlyUsed = entry.previousUsage;
		}
		entry.previousUsage = invalidIndex;
		entry.nextUsage = invalidIndex;
	}

	void linkAsMostRecentlyUsed(TSignedIndex entryIndex){
		Entry& entry = entries[entryIndex];
		entry.previousUsage = invalidIndex;
		entry.nextUsage = mostRecentlyUsed;
		if(mostRecentlyUsed!=invalidIndex){
			entries[mostRecentlyUsed].previousUsage = entryIndex;
		}else{
			leastRecentlyUsed = entryIndex;
		}
		mostRecentlyUsed = entryIndex;
	}

	void moveToMostRecentlyUsed(TSignedIndex entryIndex){
		if(mostRecentlyUsed==entryIndex){
			return;
		}
		unlinkUsage(entryIndex);
		linkAsMostRecentlyUsed(entryIndex);
	}

	void removeFromBucket(TSignedIndex entryIndex){
		if(cbk) cbk->OnValueRemove(entries[entryIndex].key, entries[entryIndex].value);
		uint32_t bucketIndex = calculateUCMapIndex(entries[entryIndex].key, arraySize);
		TSignedIndex currentIndex = bucketHeads[bucketIndex];
		TSignedIndex previousIndex = invalidIndex;
		while(currentIndex!=invalidIndex){
			if(currentIndex==entryIndex){
				if(previousIndex==invalidIndex){
					bucketHeads[bucketIndex] = entries[currentIndex].nextInBucket;
				}else{
					entries[previousIndex].nextInBucket = entries[currentIndex].nextInBucket;
				}
				entries[currentIndex].nextInBucket = invalidIndex;
				return;
			}
			previousIndex = currentIndex;
			currentIndex = entries[currentIndex].nextInBucket;
		}
	}

	TSignedIndex acquireEntryIndex(){
		if(unusedStackPtr > 0){
			unusedStackPtr--;
			return unusedStack[unusedStackPtr];
		}
		TSignedIndex entryIndex = leastRecentlyUsed;
		removeFromBucket(entryIndex);
		unlinkUsage(entryIndex);
		entries[entryIndex].used = false;
		resetEntryLinks(entryIndex);
		return entryIndex;
	}

	void insertEntry(TSignedIndex entryIndex, const TKey& key){
		uint32_t bucketIndex = calculateUCMapIndex(key, arraySize);
		Entry& entry = entries[entryIndex];
		entry.key = key;
		entry.value = TValue();
		entry.used = true;
		entry.nextInBucket = bucketHeads[bucketIndex];
		entry.previousUsage = invalidIndex;
		entry.nextUsage = invalidIndex;
		bucketHeads[bucketIndex] = entryIndex;
		linkAsMostRecentlyUsed(entryIndex);
	}

	public:

	UCMap(TReturnCallback* cbk = nullptr): mostRecentlyUsed(invalidIndex), leastRecentlyUsed(invalidIndex), cbk(cbk){
		for(uint32_t i=0; i<arraySize; i++){
			entries[i].used = false;
			resetEntryLinks(i);
			bucketHeads[i] = invalidIndex;
			unusedStack[i] = i;
		}
		unusedStackPtr = arraySize;
	}

	void setCallback(TReturnCallback* cbk){
		this->cbk = cbk;
	}

	TReturnCallback* getCallback() const{
		return cbk;
	}

	TValue& operator[](const TKey& key){
		TSignedIndex entryIndex = findEntryIndex(key);
		if(entryIndex!=invalidIndex){
			moveToMostRecentlyUsed(entryIndex);
			return entries[entryIndex].value;
		}
		entryIndex = acquireEntryIndex();
		insertEntry(entryIndex, key);
		return entries[entryIndex].value;
	}

	void erase(const TKey& key){
		TSignedIndex entryIndex = findEntryIndex(key);
		if(entryIndex != invalidIndex){
			removeFromBucket(entryIndex);
			unlinkUsage(entryIndex);
			entries[entryIndex].used = false;
			resetEntryLinks(entryIndex);
			unusedStack[unusedStackPtr] = entryIndex;
			unusedStackPtr++;
		}
	}

	bool empty() const{
		return unusedStackPtr == arraySize;
	}

	bool full() const{
		return unusedStackPtr == 0;
	}

	TSignedIndex size() const{
		return arraySize - unusedStackPtr;
	}

	TValue* getIfExists(const TKey& key){
		TSignedIndex entryIndex = findEntryIndex(key);
		if(entryIndex==invalidIndex){
			return NULL;
		}
		moveToMostRecentlyUsed(entryIndex);
		return &entries[entryIndex].value;
	}

	class iterator{
		friend class UCMap<TKey, TValue, arraySize, TSignedIndex, TReturnCallback>;

		UCMap<TKey, TValue, arraySize, TSignedIndex, TReturnCallback>* map;
		TSignedIndex entryIndex;

		public:

		iterator(UCMap<TKey, TValue, arraySize, TSignedIndex, TReturnCallback>* map, TSignedIndex startIndex)
			: map(map), entryIndex(startIndex) {}

		iterator() : map(nullptr), entryIndex(invalidIndex) {}

		bool operator!=(const iterator& other) const{
			return entryIndex != other.entryIndex;
		}

		iterator& operator++(){
			if(entryIndex != map->invalidIndex){
				entryIndex = map->entries[entryIndex].nextUsage;
			}
			return *this;
		}

		iterator& operator--(){
			if(entryIndex != map->invalidIndex){
				entryIndex = map->entries[entryIndex].previousUsage;
			}else{
				entryIndex = map->leastRecentlyUsed;
			}
			return *this;
		}

		TValue& operator*(){
			return map->entries[entryIndex].value;
		}

		bool operator==(const iterator& other) const{
			return entryIndex == other.entryIndex;
		}
	};

	iterator begin(){
		return iterator(this, mostRecentlyUsed);
	}

	iterator end(){
		return iterator(this, invalidIndex);
	}

	iterator erase(iterator it){
		TSignedIndex entryIndex = it.entryIndex;
		iterator next = iterator(this, entryIndex == invalidIndex ? invalidIndex : entries[entryIndex].nextUsage);
		if(entryIndex != invalidIndex){
			removeFromBucket(entryIndex);
			unlinkUsage(entryIndex);
			entries[entryIndex].used = false;
			resetEntryLinks(entryIndex);
			unusedStack[unusedStackPtr] = entryIndex;
			unusedStackPtr++;
		}
		return next;
	}

};

#endif