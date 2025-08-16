#ifndef IRPC_H_INCLUDED
#define IRPC_H_INCLUDED

#include <SimpleSockets.h>
#include <timing.h>

#include <vector>
#include <map>
#include <list>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <type_traits>
#include <cassert>
#include <functional>
#include <memory>

//! Interface for RPC Values
class IRPCValue{

	public:
	
	enum Type {NIL, BOOLEAN, FLOAT, INTEGER, STRING, ARRAY, OBJECT, UNKNOWN, TYPE_COUNT};
	
	static constexpr IRPCValue::Type typeId = IRPCValue::UNKNOWN;

	virtual Type getType() const{
		return UNKNOWN;
	}
	
	virtual ~IRPCValue(){}

};

//! Representation of null value
class NULLValue : public IRPCValue{
	
	public:
	
	static constexpr IRPCValue::Type typeId = IRPCValue::NIL;
	
	Type getType() const{return typeId;}
	
};

//! Representation of scalar values (e.g. BOOLEAN, FLOAT, INTEGER, STRING, typedefs see below)
template <typename TValueType, IRPCValue::Type TRPCType>
class ScalarValue : public IRPCValue{

	public:
	
	using scalar_type = TValueType;
	
	TValueType value;
	
	static constexpr IRPCValue::Type typeId = TRPCType;
	
	Type getType() const{return typeId;}
	
	ScalarValue(const TValueType& value):value(value){}
	
	template<typename TInputValueType>
	static ScalarValue<TValueType, TRPCType>* create(TInputValueType value){
		return new ScalarValue<TValueType, TRPCType>(value);
	}
	
	template<typename TNativeValue>
	static TNativeValue createNative(IRPCValue* rpcValue);
	
	template<typename TNativeValue>
	static bool checkSignature(IRPCValue* rpcValue);
	
};

typedef ScalarValue<bool, IRPCValue::BOOLEAN> BooleanValue;
typedef ScalarValue<double, IRPCValue::FLOAT> FloatValue;
typedef ScalarValue<int64_t, IRPCValue::INTEGER> IntegerValue;
typedef ScalarValue<std::string, IRPCValue::STRING> StringValue;

//! auto scalar conversion
template <typename TValueType, IRPCValue::Type TRPCType>
template<typename TNativeValue>
TNativeValue ScalarValue<TValueType, TRPCType>::createNative(IRPCValue* rpcValue){
	auto rpcType = rpcValue->getType();
	if(rpcType==IRPCValue::BOOLEAN){
		return ((BooleanValue*)rpcValue)->value;
	}else if(rpcType==IRPCValue::FLOAT){
		return ((FloatValue*)rpcValue)->value;
	}else if(rpcType==IRPCValue::INTEGER){
		return ((IntegerValue*)rpcValue)->value;
	}
	assert(false);
	return TNativeValue();
}

//! string scalars cannot be automatically converted
template <>
template<typename TNativeValue>
TNativeValue ScalarValue<std::string, IRPCValue::STRING>::createNative(IRPCValue* rpcValue){
	auto rpcType = rpcValue->getType();
	if(rpcType==IRPCValue::STRING){
		return ((StringValue*)rpcValue)->value;
	}
	assert(false);
	return TNativeValue();
}

template <typename TValueType, IRPCValue::Type TRPCType>
template<typename TNativeValue>
bool ScalarValue<TValueType, TRPCType>::checkSignature(IRPCValue* rpcValue){
	auto rpcType = rpcValue->getType();
	return rpcType==IRPCValue::BOOLEAN || rpcType==IRPCValue::FLOAT || rpcType==IRPCValue::INTEGER;//booleans, floats and integers are auto convertible
}

template <>
template<typename TNativeValue>
bool ScalarValue<std::string, IRPCValue::STRING>::checkSignature(IRPCValue* rpcValue){
	auto rpcType = rpcValue->getType();
	return rpcType==IRPCValue::STRING;
}

//! Representation of arrays
class ArrayValue : public IRPCValue{

	public:
	
	std::vector<IRPCValue*> values;
	
	static constexpr IRPCValue::Type typeId = IRPCValue::ARRAY;
	
	Type getType() const{return typeId;}
	
	ArrayValue(const std::initializer_list<IRPCValue*>& initList):values(initList){}
	
	ArrayValue(){}
	
	~ArrayValue(){
		for(uint32_t i=0; i<values.size(); i++){
			delete values[i];
		}
	}
	
	//! Factory function for arbitrary containers, implementation see below
	template<typename TNativeValue>
	static ArrayValue* create(const TNativeValue& ctr);
	
	template<typename TNativeValue>
	static TNativeValue createNative(IRPCValue* rpcValue);
	
	template<typename TNativeValue>
	static bool checkSignature(IRPCValue* rpcValue);

};

//! Representation of Objects (Key -> Value pairs)
class ObjectValue : public IRPCValue{

	public:
	
	//! key -> value
	std::unordered_map<std::string, IRPCValue*> values;
	
	static constexpr IRPCValue::Type typeId = IRPCValue::OBJECT;
	
	Type getType() const{return typeId;}
	
	ObjectValue(const std::initializer_list<std::map<std::string, IRPCValue*>::value_type>& initList):values(initList){}
	
	ObjectValue(){}
	
	~ObjectValue(){
		for(auto it = values.begin(); it != values.end(); ++it){
			delete it->second;
		}
	}
	
	IRPCValue* get(const std::string& key){
		auto it = values.find(key);
		if(it==values.end()){
			return NULL;
		}else{
			return it->second;
		}
	}
	
	//! Factory function for arbitrary containers with key (string) and values (any type), implementation see below
	template<typename TNativeValue>
	static ObjectValue* create(const TNativeValue& ctr);

	template<typename TNativeValue>
	static TNativeValue createNative(IRPCValue* rpcValue);
	
	template<typename TNativeValue>
	static bool checkSignature(IRPCValue* rpcValue);

};

// The following traits are used to identify native types with rpc types: ------------------

template <typename T>
using is_bool = std::is_same<T, bool>;

template <typename T>
struct is_integer{static constexpr bool value = std::is_same<T, int8_t>::value || std::is_same<T, int16_t>::value || std::is_same<T, int32_t>::value || std::is_same<T, int64_t>::value || std::is_same<T, uint8_t>::value || std::is_same<T, uint16_t>::value || std::is_same<T, uint32_t>::value || std::is_same<T, uint64_t>::value;};

static_assert(!is_integer<bool>::value, "");

template <typename T>
struct is_float{static constexpr bool value = std::is_same<T, float>::value || std::is_same<T, double>::value;};

template <typename T>
using is_string = std::is_same<T, std::string>;

template<typename T>
struct is_scalar{static constexpr bool value = is_bool<T>::value || is_integer<T>::value || is_float<T>::value || is_string<T>::value;};

//! Currently list and vector supported, in case more types shall be supported additional traits have to be added here
template<typename T>
struct is_array : public std::false_type {};

template<typename T> 
struct is_array<std::vector<T>> : public std::true_type {};

template<typename T> 
struct is_array<std::list<T>> : public std::true_type {};

template<typename T> 
struct is_array<std::set<T>> : public std::true_type {};

template<typename T> 
struct is_array<std::unordered_set<T>> : public std::true_type {};

template<typename T, std::size_t N> 
struct is_array<std::array<T,N>> : public std::true_type {};

//! Currently map and unordered_map supported, in case more types shall be supported additional traits have to be added here
template<typename T>
struct is_object : public std::false_type {};

template<typename K, typename V> 
struct is_object<std::map<K, V>> : public std::true_type {};

template<typename K, typename V> 
struct is_object<std::unordered_map<K, V>> : public std::true_type {};

//! native_to_rpc_type is used by createRPCValue and in createNativeValue.
//! current mapping is: bool -> BooleanValue; (uint|int)_(8|16|32|64) -> IntegerValue; double|float -> FloatValue; std::string -> StringValue; (list|vector)<..> -> ArrayValue; (map|unordered_map)<std::string, ..> -> ObjectValue
//! In case no match is found either IRPCValue is returned except if chooseNativeIfNoMatch is true => TNativeValue. This makes it possible to extend it for custom objects or arrays with different member types. For this just create a class with two static methods: create and createNative.
template<typename TNativeValue, bool chooseNativeIfNoMatch = false>
using native_to_rpc_type = typename std::conditional<is_bool<TNativeValue>::value, BooleanValue,
										typename std::conditional<is_integer<TNativeValue>::value, IntegerValue,
											typename std::conditional<is_float<TNativeValue>::value, FloatValue,
												typename std::conditional<is_string<TNativeValue>::value, StringValue,
													typename std::conditional<is_array<TNativeValue>::value, ArrayValue, 
														typename std::conditional<is_object<TNativeValue>::value, ObjectValue, 
															typename std::conditional<chooseNativeIfNoMatch, TNativeValue, IRPCValue>::type>::type>::type>::type>::type>::type>::type;


// Functions to convert native and rpc values: ------------------------

//! creates a new rpc value from a native value
//! even complicated types work out of the box, provided each component type is convertible e.g. createRPCValue(std::map<std::string, std::vector<std::map<std::string, double>>>());
template<typename TNativeValue>
IRPCValue* createRPCValue(const TNativeValue& value){
	return native_to_rpc_type<TNativeValue, true>::template create<TNativeValue>(value);
}

//! Creates a default native value from a RPC type
//! even complicated types work out of the box, provided each component type is convertible e.g. createNativeValue<std::vector<std::list<int>>>(...)
//! Attention: The true underlying data structure of rpcValue must match the native signature. Otherwise there will be invalid casts and bad memory access.
template<typename TNativeValue>
TNativeValue createNativeValue(IRPCValue* rpcValue){
	assert(rpcValue->getType()==native_to_rpc_type<TNativeValue>::typeId || native_to_rpc_type<TNativeValue>::typeId==IRPCValue::UNKNOWN || is_scalar<TNativeValue>::value);
	return native_to_rpc_type<TNativeValue, true>::template createNative<TNativeValue>(rpcValue);
}

//! checks if a native value can be safely created from the given rpc value
//! if custom objects are involved they must implement the checkSignature method
template<typename TNativeValue>
bool checkSignature(IRPCValue* rpcValue){
	return native_to_rpc_type<TNativeValue, true>::template checkSignature<TNativeValue>(rpcValue);
}

template<typename TNativeValue>
ArrayValue* ArrayValue::create(const TNativeValue& ctr){
	ArrayValue* res = new ArrayValue();
	res->values.reserve(ctr.size());
	for(const typename TNativeValue::value_type& value : ctr){
		res->values.push_back(createRPCValue<typename TNativeValue::value_type>(value));
	}
	return res;
}

template<typename TNativeValue>
ObjectValue* ObjectValue::create(const TNativeValue& ctr){
	ObjectValue* res = new ObjectValue();
	for(typename TNativeValue::const_iterator it = ctr.begin(); it != ctr.end(); ++it){
		res->values[it->first] = createRPCValue<typename TNativeValue::mapped_type>(it->second);
	}
	return res;
}

//! allocates memory if container applicable
template<typename T>
void reserveIfApplicable(T& ctr, size_t size){}

template<typename T>
void reserveIfApplicable(std::vector<T>& ctr, size_t size){
	ctr.reserve(size);
}

template<typename T>
void insertByContainer(T& ctr, typename T::value_type&& value){
	ctr.push_back(value);
}

template<typename T>
void insertByContainer(std::set<T>& ctr, T&& value){
	ctr.insert(value);
}

template<typename TNativeValue>
TNativeValue ArrayValue::createNative(IRPCValue* rpcValue){
	TNativeValue res;
	reserveIfApplicable<TNativeValue>(res, ((ArrayValue*)rpcValue)->values.size());
	for(IRPCValue* value : ((ArrayValue*)rpcValue)->values){
		insertByContainer(res, createNativeValue<typename TNativeValue::value_type>(value));//res.push_back(createNativeValue<typename TNativeValue::value_type>(value));
	}
	return res;
}

template<typename TNativeValue>
TNativeValue ObjectValue::createNative(IRPCValue* rpcValue){
	TNativeValue res;
	for(auto it = ((ObjectValue*)rpcValue)->values.begin(); it != ((ObjectValue*)rpcValue)->values.end(); ++it){
		res[it->first] = createNativeValue<typename TNativeValue::mapped_type>(it->second);
	}
	return res;
}

template<typename TNativeValue>
bool ArrayValue::checkSignature(IRPCValue* rpcValue){
	if(rpcValue->getType()==IRPCValue::ARRAY){
		bool res = true;
		for(IRPCValue* value : ((ArrayValue*)rpcValue)->values){
			res = res && ::checkSignature<typename TNativeValue::value_type>(value);
		}
		return res;
	}
	return false;
}

template<typename TNativeValue>
bool ObjectValue::checkSignature(IRPCValue* rpcValue){
	if(rpcValue->getType()==IRPCValue::OBJECT){
		bool res = true;
		for(auto it = ((ObjectValue*)rpcValue)->values.begin(); it != ((ObjectValue*)rpcValue)->values.end(); ++it){
			res = res && ::checkSignature<typename TNativeValue::mapped_type>(it->second);
		}
		return res;
	}
	return false;
}

//! checks only the first "layer"
inline bool hasValidRPCSignature(const std::vector<IRPCValue*>& values, const std::vector<IRPCValue::Type>& types){
	if(types.size()!=values.size()){return false;}
	for(uint32_t i=0; i<values.size(); i++){
		if(values[i]->getType()!=types[i]){return false;}
	}
	return true;
}

inline bool hasRPCArrayValidFieldTypes(ArrayValue* array, IRPCValue::Type type){
	for(IRPCValue* v : array->values){
		if(v->getType()!=type){return false;}
	}
	return true;
}

// Interfaces: ------------------------------

//! Interface for Remote Procedure Callers
class IRemoteProcedureCaller{

	public:
	
	virtual ~IRemoteProcedureCaller(){}
	
	//! results need to be deleted inside this method
	virtual void OnProcedureResult(IRPCValue* results, uint32_t id) = 0;
	
	//! called in case of a severe error instead of a result
	//! errorData needs to be deleted in this method
	virtual void OnProcedureError(int32_t errorCode, const std::string& errorMessage, IRPCValue* errorData, uint32_t id){
		delete errorData;
	}

};

class IRPC;

//! Interface for Remote Procedure Call receiver
class IRemoteProcedureCallReceiver{

	public:
	
	virtual ~IRemoteProcedureCallReceiver(){}
	
	//! called before callProcedure to notify about the RPC interface
	virtual void OnSetRPC(IRPC* rpc){}
	
	//! values and result need to be deleted by the caller of this method
	virtual IRPCValue* callProcedure(const std::string& procedure, const std::vector<IRPCValue*>& values) = 0;

};

//! Adapter to use simple lambdas as call receivers
class LambdaCallReceiver : public IRemoteProcedureCallReceiver{
	
	public:
	
	using CallProcedureFunction = std::function<IRPCValue*(const std::string&,const std::vector<IRPCValue*>&)>;
	
	CallProcedureFunction f;
	
	LambdaCallReceiver():f([](const std::string&,const std::vector<IRPCValue*>&){return (IRPCValue*)(NULL);}){}
	
	LambdaCallReceiver(const CallProcedureFunction& f):f(f){}
	
	IRPCValue* callProcedure(const std::string& procedure, const std::vector<IRPCValue*>& values) override{
		return f(procedure, values);
	}
	
};

//! Interface for remote procedure calls
class IRPC{

	public:

	//! true if procedure call successfully sent or added to an internal send queue, caller can be NULL if no response is expected
	//! the id is optional for identifying the result in OnProcedureResult it is NOT the id from JSON RPC (there should be a id translation for (caller,caller_id) <-> json_rpc_id in case of json rpc
	//! deleteValues: true if this method shall delete the values if no longer needed, if false they must be deleted manually
	virtual bool callRemoteProcedure(const std::string& procedure, const std::vector<IRPCValue*>& values, IRemoteProcedureCaller* caller = NULL, uint32_t id = 0, bool deleteValues = true) = 0;
	
	//! register as call receiver
	virtual void registerCallReceiver(const std::string& procedure, IRemoteProcedureCallReceiver* receiver) = 0;
	
	//! unregisters the call receiver, if receiver!=NULL: it is only unregistered if the current registration matches the receiver (useful to avoid to accidentally unregister other receivers), if NULL any receiver can be unregistered
	virtual void unregisterCallReceiver(const std::string& procedure, IRemoteProcedureCallReceiver* receiver = NULL) = 0;
	
	//! Needs to be called before a IRemoteProcedureCaller is deleted. Otherwise it might be used after deleting it.
	virtual void removeProcedureCaller(IRemoteProcedureCaller* caller) = 0;
	
	//! Synchronization / result processing / call processing etc...
	virtual void update() = 0;
	
	virtual ~IRPC(){}

};

class IMetaProtocolHandler{

	public:
	
	virtual ~IMetaProtocolHandler(){}
	
	//! true if target protocol has been negotiated successfully
	//! this function can be called by another thread
	virtual bool tryNegotiate(ICommunicationEndpoint* socket) = 0;
	
	//! returns true if a compression feature of the underlying protocol shall be used (e.g. ZSocket), this flag may be determined during protocol negotiation
	virtual bool useCompression() const{return false;}

};

//! Interface for minimal RPC clients (IRPC with connection state information)
class IMinimalRPCClient : public IRPC{
	
	public:
	
	enum ClientState{NOT_CONNECTED, CONNECTING, CONNECTED, CONNECTION_ERROR, STATE_COUNT};
	
	virtual ~IMinimalRPCClient(){}
	
	//! NOT_CONNECTED if ping timeout
	virtual ClientState getState() const = 0;
	
	virtual bool isConnected() const{
		ClientState state = getState();
		return state==CONNECTING || state==CONNECTED;
	}
	
	virtual void disconnect() = 0;
	
};

//! Interface class for RPC Clients
class IRPCClient : public IMinimalRPCClient{

	public:
	
	virtual ~IRPCClient(){}
	
	//! Connects (async) and sends the RPC and API level, pingTimeout and connectTimeout in ms
	virtual void connect(const IIPAddress& address, uint32_t pingSendPeriod, uint32_t pingTimeout, uint32_t connectTimeout, IMetaProtocolHandler* metaProtocolHandler = NULL) = 0;
	
	//! blocks until all pending stuff as been sent
	virtual void flush() = 0;
	
};

inline std::shared_ptr<IRPCValue> blockForIRPCResult(const std::function<void(IRPC* rpc, IRemoteProcedureCaller* caller, uint32_t id)>& rpcFunction, IRPC* rpc, uint32_t id){
	class : public IRemoteProcedureCaller{
		public:
		bool done = false;
		uint32_t funcID;
		std::shared_ptr<IRPCValue> result;
		void OnProcedureResult(IRPCValue* results, uint32_t id){
			done = id==funcID;
			result = std::shared_ptr<IRPCValue>(results);
		}
		virtual void OnProcedureError(int32_t errorCode, const std::string& errorMessage, IRPCValue* errorData, uint32_t id){
			done = id==funcID;
			delete errorData;
		}
	} cbk;
	cbk.funcID = id;
	rpcFunction(rpc, &cbk, id);
	while(!cbk.done){
		rpc->update();
		delay(1);
	}
	return cbk.result;
}

// Helper macros for user defined types:
// Example:
/*
class Foo{

	public:
	
	std::string name;
	std::vector<int64_t> position;
	
	CREATE_BEGIN(Foo)
		FILL_FIELD(name)
		FILL_FIELD(position)
	CREATE_END
	
	CREATE_NATIVE_BEGIN(Foo)
		FILL_NATIVE_FIELD(name)
		static const std::vector<int64_t> defaultPosition{-1,-1,-1};
		FILL_NATIVE_FIELD_IF_AVAILABLE(position, defaultPosition)
	CREATE_NATIVE_END
	
	CHECK_SIGNATURE_BEGIN(Foo)
		CHECK_FIELD_SIGNATURE(name, false)
		CHECK_FIELD_SIGNATURE(position, true)
	CHECK_SIGNATURE_END
	
};
*/

#define CREATE_BEGIN(NATIVE_TYPE) \
	template<typename TNativeValue> \
	static ObjectValue* create(const NATIVE_TYPE& nativeValue){ \
		static_assert(std::is_same<TNativeValue, NATIVE_TYPE>::value, ""); \
		ObjectValue* rpcValue = new ObjectValue();
	
#define CREATE_END \
		return rpcValue; \
	}
	
#define CHECK_SIGNATURE_BEGIN(NATIVE_TYPE) \
	template<typename TNativeValue> \
	static bool checkSignature(IRPCValue* rpcValue){ \
		static_assert(std::is_same<TNativeValue, NATIVE_TYPE>::value, ""); \
		if(rpcValue->getType()==IRPCValue::OBJECT){ \
			bool res = true;
	
#define CHECK_SIGNATURE_END \
			return res; \
		} \
		return false; \
	}

#define CHECK_FIELD_SIGNATURE(FIELD_NAME, IS_OK_IF_FIELD_MISSING) \
	CHECK_FIELD_SIGNATURE_ALIAS(FIELD_NAME, FIELD_NAME, IS_OK_IF_FIELD_MISSING)

#define CHECK_FIELD_SIGNATURE_ALIAS(ALIAS, FIELD_NAME, IS_OK_IF_FIELD_MISSING) \
	{ \
		ObjectValue* o = (ObjectValue*)rpcValue; \
		auto it = o->values.find(#ALIAS); \
		if(it!=o->values.end()){ \
			res = res && ::checkSignature<decltype(TNativeValue::FIELD_NAME)>(it->second); \
		}else{ \
			res = res && IS_OK_IF_FIELD_MISSING; \
		} \
	}

#define CREATE_NATIVE_BEGIN(NATIVE_TYPE) \
	template<typename TNativeValue> \
	static NATIVE_TYPE createNative(IRPCValue* rpcValue){ \
		static_assert(std::is_same<TNativeValue, NATIVE_TYPE>::value, ""); \
		NATIVE_TYPE nativeValue;
		
#define CREATE_NATIVE_END \
		return nativeValue; \
	}
	
#define FILL_FIELD_ALIAS(ALIAS, FIELD_NAME) \
	rpcValue->values[#ALIAS] = createRPCValue(nativeValue.FIELD_NAME);
	
#define FILL_FIELD(FIELD_NAME) \
	FILL_FIELD_ALIAS(FIELD_NAME, FIELD_NAME)

#define FILL_NATIVE_FIELD_ALIAS(ALIAS, FIELD_NAME) \
	nativeValue.FIELD_NAME = createNativeValue<decltype(nativeValue.FIELD_NAME)>(((ObjectValue*)rpcValue)->values[#ALIAS]);

#define FILL_NATIVE_FIELD(FIELD_NAME) \
	FILL_NATIVE_FIELD_ALIAS(FIELD_NAME, FIELD_NAME)
	
#define FILL_NATIVE_FIELD_ALIAS_IF_AVAILABLE(ALIAS, FIELD_NAME, DEFAULT_VALUE) \
	{ \
		ObjectValue* o = (ObjectValue*)rpcValue; \
		auto it = o->values.find(#ALIAS); \
		if(it!=o->values.end()){ \
			nativeValue.FIELD_NAME = createNativeValue<decltype(nativeValue.FIELD_NAME)>(it->second); \
		}else{ \
			nativeValue.FIELD_NAME = DEFAULT_VALUE; \
		} \
	}

#define FILL_NATIVE_FIELD_IF_AVAILABLE(FIELD_NAME, DEFAULT_VALUE) \
	FILL_NATIVE_FIELD_ALIAS_IF_AVAILABLE(FIELD_NAME, FIELD_NAME, DEFAULT_VALUE)

#endif
