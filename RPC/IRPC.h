#ifndef IRPC_H_INCLUDED
#define IRPC_H_INCLUDED

#include <SimpleSockets.h>

#include <vector>
#include <map>
#include <list>
#include <unordered_map>
#include <type_traits>
#include <cassert>

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
	static TNativeValue createNative(ScalarValue<TValueType, TRPCType>* rpcValue){
		return rpcValue->value;
	}
	
};

typedef ScalarValue<bool, IRPCValue::BOOLEAN> BooleanValue;
typedef ScalarValue<double, IRPCValue::FLOAT> FloatValue;
typedef ScalarValue<int64_t, IRPCValue::INTEGER> IntegerValue;
typedef ScalarValue<std::string, IRPCValue::STRING> StringValue;

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
	static TNativeValue createNative(ArrayValue* rpcValue);

};

//! Representation of Objects (Key -> Value pairs)
class ObjectValue : public IRPCValue{

	public:
	
	//! key -> value
	std::map<std::string, IRPCValue*> values;
	
	static constexpr IRPCValue::Type typeId = IRPCValue::OBJECT;
	
	Type getType() const{return typeId;}
	
	ObjectValue(const std::initializer_list<std::map<std::string, IRPCValue*>::value_type>& initList):values(initList){}
	
	ObjectValue(){}
	
	~ObjectValue(){
		for(auto it = values.begin(); it != values.end(); ++it){
			delete it->second;
		}
	}
	
	//! Factory function for arbitrary containers with key (string) and values (any type), implementation see below
	template<typename TNativeValue>
	static ObjectValue* create(const TNativeValue& ctr);

	template<typename TNativeValue>
	static TNativeValue createNative(ObjectValue* rpcValue);

};

// The following traits are used to identify native types with rpc types: ------------------

template <typename T>
using is_bool = std::is_same<T, bool>;

template <typename T>
struct is_integer{static constexpr bool value = std::is_same<T, int8_t>::value || std::is_same<T, int16_t>::value || std::is_same<T, int32_t>::value || std::is_same<T, int64_t>::value || std::is_same<T, uint8_t>::value || std::is_same<T, uint16_t>::value || std::is_same<T, uint32_t>::value || std::is_same<T, uint64_t>::value;};

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
	assert(rpcValue->getType()==native_to_rpc_type<TNativeValue>::typeId || native_to_rpc_type<TNativeValue>::typeId==IRPCValue::UNKNOWN);
	return native_to_rpc_type<TNativeValue, true>::template createNative<TNativeValue>(static_cast<native_to_rpc_type<TNativeValue>*>(rpcValue));
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

template<typename TNativeValue>
TNativeValue ArrayValue::createNative(ArrayValue* rpcValue){
	TNativeValue res;
	reserveIfApplicable<TNativeValue>(res, rpcValue->values.size());
	for(IRPCValue* value : rpcValue->values){
		res.push_back(createNativeValue<typename TNativeValue::value_type>(value));
	}
	return res;
}

template<typename TNativeValue>
TNativeValue ObjectValue::createNative(ObjectValue* rpcValue){
	TNativeValue res;
	for(auto it = rpcValue->values.begin(); it != rpcValue->values.end(); ++it){
		res[it->first] = createNativeValue<typename TNativeValue::mapped_type>(it->second);
	}
	return res;
}

// Interfaces: ------------------------------

//! Interface for Remote Procedure Callers
class IRemoteProcedureCaller{

	public:
	
	virtual ~IRemoteProcedureCaller(){}
	
	//! results need to be deleted inside this method
	virtual void OnProcedureResult(IRPCValue* results, uint32_t id) = 0;
	
	//! called in case of a severe error instead of a result
	virtual void OnProcedureError(int32_t errorCode, const std::string& errorMessage, IRPCValue* errorData, uint32_t id){
		delete errorData;
	}

};

//! Interface for Remote Procedure Call receiver
class IRemoteProcedureCallReceiver{

	public:
	
	virtual ~IRemoteProcedureCallReceiver(){}
	
	//! values and result need to be deleted by the caller of this method
	virtual IRPCValue* callProcedure(const std::string& procedure, const std::vector<IRPCValue*>& values) = 0;

};

//! Interface for remote procedure calls
class IRPC{

	public:

	//! true if connected and procedure call successfully sent or added to an internal send queue, caller can be NULL if no response is expected
	//! the id is optional for identifying the result in OnProcedureResult it is NOT the id from JSON RPC (there should be a id translation for (caller,caller_id) <-> json_rpc_id in case of json rpc
	//! deleteValues: true if this method shall delete the values if no longer needed, if false they must be deleted manually
	virtual bool callRemoteProcedure(const std::string& procedure, const std::vector<IRPCValue*>& values, IRemoteProcedureCaller* caller = NULL, uint32_t id = 0, bool deleteValues = true) = 0;
	
	//! register as call receiver
	virtual void registerCallReceiver(const std::string& procedure, IRemoteProcedureCallReceiver* receiver) = 0;
	
	//! unregisters the call receiver, if receiver!=NULL: it is only unregistered if the current registration matches the receiver (useful to avoid to accidentally unregister other receivers), if NULL any receiver can be unregistered
	virtual void unregisterCallReceiver(const std::string& procedure, IRemoteProcedureCallReceiver* receiver = NULL) = 0;
	
	//! Needs to be called before a IRemoteProcedureCaller is deleted. Otherwise it might be used after deleting it.
	virtual void removeProcedureCaller(IRemoteProcedureCaller* caller) = 0;
	
	virtual ~IRPC(){}

};

class IMetaProtocolHandler{

	public:
	
	virtual ~IMetaProtocolHandler(){}
	
	//! true if target protocol has been negotiated successfully
	//! this function can be called by another thread
	virtual bool tryNegotiate(ISocket* socket) = 0;

};

//! Interface class for RPC Clients
class IRPCClient : public IRPC{

	public:
	
	enum ClientState{NOT_CONNECTED, CONNECTING, CONNECTED, CONNECTION_ERROR, STATE_COUNT};
	
	virtual ~IRPCClient(){}
	
	//! Connects (async) and sends the RPC and API level, pingTimeout and connectTimeout in ms
	virtual void connect(const IIPAddress& address, uint32_t pingSendPeriod, uint32_t pingTimeout, uint32_t connectTimeout, IMetaProtocolHandler* metaProtocolHandler = NULL) = 0;
	
	//! Synchronization / result processing / call processing etc...
	virtual void update() = 0;
	
	//! NOT_CONNECTED if ping timeout
	virtual ClientState getState() const = 0;
	
	virtual bool isConnected() const = 0;
	
	virtual void disconnect() = 0;
	
	//! gets the time of the last reception in seconds (compatible to getSecs() output)
	virtual double getLastReceiveTime() = 0;
	
};

#endif
