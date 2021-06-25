#include "JSONRPC2Client.h"

#include <timing.h>
#include <StringHelpers.h>
#include <JSONParser.h>

#include <cmath>
#include <cstring>
#include <iostream>
#include <csignal>

#ifdef DEBUG
#define PRINT_COMMUNICATION
#endif

using namespace std;

std::string convertRPCValueToJSONResult(const IRPCValue& value, uint32_t jsonId){
	std::stringstream ss;
	ss << "{\"jsonrpc\": \"2.0\", \"result\": " << convertRPCValueToJSONString(value) << ", \"id\": " << jsonId << "}\n";
	return ss.str();
}

std::string makeJSONRPCRequest(const std::string& procedure, const std::vector<IRPCValue*>& values, uint32_t* jsonId){
	std::stringstream ss;
	ss << "{\"jsonrpc\": \"2.0\", \"method\": " << escapeAndQuoteJSONString(procedure) << ", \"params\": [";
	for(uint32_t i=0; i<values.size(); i++){
		if(i>0){ss << ",";}
		ss << convertRPCValueToJSONString(*(values[i]));
	}
	ss << "]";
	if(jsonId){
		ss << ", \"id\": " << *jsonId;
	}
	ss << "}\n";//newline for better readability
	//std::cout << ss.str() << std::endl;//for debug purposes
	return ss.str();
}

static const char* jsonPing = "{\"jsonrpc\": \"2.0\", \"method\": \"rc:ping\", \"params\": [], \"id\": 0}\n";
static const uint32_t jsonPingSize = strlen(jsonPing);

void* JSONRPC2Client::clientMain(void* p){
	JSONRPC2Client* client = (JSONRPC2Client*)p;
	const double pingTimeout = ((double)client->pingTimeout)/1000.0;
	const bool sendPing = client->pingSendPeriod != PING_DISABLE_SEND_PERIOD;
	const double pingSendPeriod = ((double)client->pingSendPeriod)/1000.0;
	if(client->socket==NULL){
		client->socket = connectSocketForAddressList(std::list<IIPAddress*>(1, client->address), client->connectTimeout);
	}
	bool runThread = client->socket!=NULL;
	if(client->metaProtocolHandler!=NULL && runThread){
		runThread = client->metaProtocolHandler->tryNegotiate(client->socket);
	}
	lockMutex(client->mutexSync);
	client->state = runThread?CONNECTED:CONNECTION_ERROR;
	unlockMutex(client->mutexSync);
	const uint32_t bufferSize = 4096;
	char* buffer = new char[bufferSize];
	int32_t curlyBracketCount = 0, squareBracketCount = 0;
	int state = 0;//0:start, 1:{ first read/parsing json rpc, 2: inside string, 3: inside string but escape character read
	char lastChar = '\0';
	double lastReceived = getSecs();
	double lastPingSent = lastReceived-pingTimeout;
	while(runThread){
		double t = getSecs();
		//Synchronization:
		lockMutex(client->mutexSync);
		client->lastReceived = lastReceived;
		runThread = !client->syncExit;
		client->clientToSend.splice(client->clientToSend.end(), client->syncToSend);
		client->syncToReceive.splice(client->syncToReceive.end(), client->clientToReceive);
		client->areTherePendingSends = !client->clientToSend.empty();
		unlockMutex(client->mutexSync);
		//Send stuff
		if(!client->clientToSend.empty()){
			lastPingSent = t;
			for(auto it = client->clientToSend.begin(); it != client->clientToSend.end(); ++it){
				#ifdef PRINT_COMMUNICATION
				std::cout << "sending: " << *it << std::endl;
				#endif
				client->socket->send(it->c_str(), it->size());
			}
			client->clientToSend.clear();
			lockMutex(client->mutexSync);
			client->areTherePendingSends = false;
			unlockMutex(client->mutexSync);
		}
		//Receive stuff & Timeout:
		uint32_t read = client->socket->recv(buffer, bufferSize);
		if(read==0 && t-lastReceived>pingTimeout){
			runThread = false;
			std::cout << "Ping Timeout: " << (t-lastReceived) << "s" << std::endl;
		}
		while(read>0){
			#ifdef PRINT_COMMUNICATION
			std::cout << "raw: " << std::string(buffer, read) << std::endl << std::flush;
			#endif
			for(uint32_t i=0; i<read; i++){
				char c = buffer[i];
				//std::cout << c << " state: " << state << " curlyBracketCount: " << curlyBracketCount << " squareBracketCount: " << squareBracketCount << std::endl;
				switch(state){
					case 0:{
						if(c=='{'){
							state = 1;
							lastChar = '{';
							curlyBracketCount++;
						}else if(c=='['){
							state = 1;
							lastChar = '[';
							squareBracketCount++;
						}
						break;
					}case 1:{
						client->parser->parse(lastChar, c);
						lastChar = c;
						switch(c){
							case '{':
								curlyBracketCount++;
								break;
							case '[':
								squareBracketCount++;
								break;
							case '}':
								curlyBracketCount--;
								break;
							case ']':
								squareBracketCount--;
								break;
							case '\"':
								state = 2;
								break;
						}
						if(curlyBracketCount==0 && squareBracketCount==0){
							IJSONParser::State s = client->parser->parse(lastChar, '\0');
							if(s==IJSONParser::SUCCESS){
								client->clientToReceive.push_back(client->parser->stealResult());
							}else{
								std::cerr << "Error while parsing JSON." << std::endl;
							}
							client->parser->reset();
							state = 0;
						}
						break;
					}case 2:{
						client->parser->parse(lastChar, c);
						lastChar = c;
						switch(c){
							case '\\':
								state = 3;
								break;
							case '\"':
								state = 1;
								break;
						}
						break;
					}case 3:{
						client->parser->parse(lastChar, c);
						lastChar = c;
						state = 2;
						break;
					}
				}
			}
			read = client->socket->recv(buffer, bufferSize);
			lastReceived = t = getSecs();
			if(sendPing && t-lastPingSent>pingSendPeriod){lastPingSent = t; client->socket->send(jsonPing, jsonPingSize);}//Send ping if required
			lockMutex(client->mutexSync);
			client->lastReceived = lastReceived;
			unlockMutex(client->mutexSync);
		}
		if(sendPing && t-lastPingSent>pingSendPeriod){lastPingSent = t; client->socket->send(jsonPing, jsonPingSize);}//Send ping if required
		delay(1);
	}
	std::cout << "JSONRPC2Client Thread exiting..." << std::endl;
	delete[] buffer;
	if(client->socket!=NULL){delete client->socket;}
	delete client->address;
	client->clientToSend.clear();
	deleteAllElements(client->clientToReceive);
	client->clientToReceive.clear();
	lockMutex(client->mutexSync);
	client->mustJoin = true;
	client->state = NOT_CONNECTED;
	client->syncToSend.clear();
	deleteAllElements(client->syncToReceive);
	client->syncToReceive.clear();
	unlockMutex(client->mutexSync);
	return NULL;
}

JSONRPC2Client::JSONRPC2Client(){
	mustJoin = areTherePendingSends = syncExit = false;
	initMutex(mutexSync);
	parser = new JSONParser();
	socket = NULL;
	metaProtocolHandler = NULL;
	syncedState = state = IRPCClient::NOT_CONNECTED;
	maxJsonId = 1;//0 is reserved for ping
}

JSONRPC2Client::~JSONRPC2Client(){
	jsonId2Caller.clear();
	receivers.clear();//robustness against forgotten deregistrations before delete
	disconnect();
	delete parser;
	deleteAllElements(mainToReceive);
}

bool JSONRPC2Client::callRemoteProcedure(const std::string& procedure, const std::vector<IRPCValue*>& values, IRemoteProcedureCaller* caller, uint32_t id, bool deleteValues){
	bool success = isConnected();
	if(success){
		if(caller!=NULL){
			uint32_t jsonId;
			if(reusableIds.empty()){
				maxJsonId++;
				jsonId = maxJsonId;
			}else{
				jsonId = reusableIds.back();
				reusableIds.pop_back();
			}
			jsonId2Caller[jsonId] = std::make_pair(caller, id);
			mainToSend.push_back(makeJSONRPCRequest(procedure, values, &jsonId));
		}else{
			mainToSend.push_back(makeJSONRPCRequest(procedure, values, NULL));
		}
	}else{
		std::cout << "no connection: " << makeJSONRPCRequest(procedure, values, NULL) << std::endl;
	}
	if(deleteValues){
		deleteAllElements(values);
	}
	return success;
}

void JSONRPC2Client::registerCallReceiver(const std::string& procedure, IRemoteProcedureCallReceiver* receiver){
	receivers[procedure] = receiver;
}

void JSONRPC2Client::unregisterCallReceiver(const std::string& procedure, IRemoteProcedureCallReceiver* receiver){
	auto it = receivers.find(procedure);
	if(it!=receivers.end()){
		if(receiver==NULL || receiver==it->second){
			receivers.erase(it);
		}
	}
}

void JSONRPC2Client::connect(const IIPAddress& address, uint32_t pingSendPeriod, uint32_t pingTimeout, uint32_t connectTimeout, IMetaProtocolHandler* metaProtocolHandler){
	disconnect();
	this->socket = NULL;
	this->pingSendPeriod = pingSendPeriod;
	this->pingTimeout = pingTimeout;
	this->connectTimeout = connectTimeout;
	this->address = address.createNewCopy();
	this->metaProtocolHandler = metaProtocolHandler;
	syncedState = state = IRPCClient::CONNECTING;
	mustJoin = syncExit = false;
	bool res = createThread(clientThread, JSONRPC2Client::clientMain, (void*)this, true);
	assert(res);
}

void JSONRPC2Client::useSocket(ISocket* socket, uint32_t pingTimeout, uint32_t pingSendPeriod){
	disconnect();
	this->socket = socket;
	this->pingTimeout = pingTimeout;
	this->pingSendPeriod = pingSendPeriod;
	this->connectTimeout = 0;
	this->address = NULL;
	this->metaProtocolHandler = NULL;
	syncedState = state = IRPCClient::CONNECTING;
	mustJoin = syncExit = false;
	bool res = createThread(clientThread, JSONRPC2Client::clientMain, (void*)this, true);
	assert(res);
}

IRPCValue* stealObjectField(ObjectValue* o, const std::string& key){
	auto it = o->values.find(key);
	if(it!=o->values.end()){
		IRPCValue* res = it->second;
		o->values.erase(it);
		return res;
	}
	return NULL;
}

IRPCValue* getObjectField(ObjectValue* o, const std::string& key, IRPCValue::Type type){
	auto it = o->values.find(key);
	if(it!=o->values.end()){
		return (type==IRPCValue::UNKNOWN || it->second->getType()==type)?it->second:NULL;
	}
	return NULL;
}

static inline bool hasJSONRPCField(ObjectValue* o){
	StringValue* s = getObjectField<StringValue>(o, "jsonrpc");
	if(s){
		return s->value.compare("2.0")==0;
	}
	return false;
}

void JSONRPC2Client::handleEntity(IRPCValue* entity){
	if(entity){
		//std::cout << "received: " << entity->to_json().dump() << std::endl;
		if(entity->getType()==IRPCValue::ARRAY){
			ArrayValue* array = (ArrayValue*)entity;
			for(IRPCValue* r:array->values){
				handleEntity(r);
			}
			array->values.clear();//clear before delete since they already have been deleted in the callback methods as required
			delete entity;
			return;
		}else if(entity->getType()==IRPCValue::OBJECT){
			ObjectValue* o = (ObjectValue*)entity;
			#ifndef DONT_CHECK_JSON_RPC_VERSION
			if(hasJSONRPCField(o)){
			#endif
				IRPCValue* result = stealObjectField(o, "result");
				ObjectValue* error = getObjectField<ObjectValue>(o, "error");
				if(result || error){//results and errors
					IntegerValue* id = getObjectField<IntegerValue>(o, "id");//only integer ids allowed in our case
					if(id){
						uint32_t jsonId = id->value;
						auto it = jsonId2Caller.find(jsonId);
						if(it != jsonId2Caller.end()){
							if(!result && error){
								IntegerValue* code = getObjectField<IntegerValue>(error, "code");
								StringValue* msg = getObjectField<StringValue>(error, "message");
								if(code && msg){
									it->second.first->OnProcedureError(code->value, msg->value, stealObjectField(error, "data"), it->second.second);
									reusableIds.push_back(jsonId);
									jsonId2Caller.erase(it);
									delete o;
									return;
								}
							}else if(result && !error){
								it->second.first->OnProcedureResult(result, it->second.second);
								reusableIds.push_back(jsonId);
								jsonId2Caller.erase(it);
								delete o;
								return;
							}
						}else{
							if(jsonId!=0){std::cerr << "Error: No RPC caller with json id: " << jsonId << " found." << std::endl;}//0 is ping
							delete result;
							delete o;
							return;
						}
					}
				}else if(StringValue* method = getObjectField<StringValue>(o, "method")){//requests and notifications
					IntegerValue* id = getObjectField<IntegerValue>(o, "id");//only integer ids allowed in our case
					auto it = receivers.find(method->value);
					if(it != receivers.end()){
						ArrayValue* params = getObjectField<ArrayValue>(o, "params");
						IRPCValue* result = params?it->second->callProcedure(method->value, params->values):it->second->callProcedure(method->value, {});
						if(id){
							if(result){
								mainToSend.push_back(convertRPCValueToJSONResult(*result, id->value));
								delete result;
							}else{//return null
								NULLValue nullVal;
								mainToSend.push_back(convertRPCValueToJSONResult(nullVal, id->value));
							}
						}else{
							delete result;
						}
						delete o;
						return;
					}else if(id){
						std::stringstream ss;
						ss << "{\"jsonrpc\": \"2.0\", \"error\": {\"code\":-32601, \"message\": \"Method not found\", \"data\": \"" << method->value << "\"}, \"id\": " << id->value << "}";
						mainToSend.push_back(ss.str());
						delete o;
						return;
					}
				}
				if(result){o->values["result"] = result;}//needs to be put back for error output
			#ifndef DONT_CHECK_JSON_RPC_VERSION
			}
			#endif
		}
		std::cerr << "Error: Invalid / unsupported JSON-RPC: " << convertRPCValueToJSONString(*entity) << std::endl;
		delete entity;
	}
}

void JSONRPC2Client::removeProcedureCaller(IRemoteProcedureCaller* caller){
	auto it = jsonId2Caller.begin();
	while(it != jsonId2Caller.end()){//inefficient, but ok since this method should be called when the caller is deleted which usually occurs at the end of the program
		if(it->second.first == caller){
			auto it2 = it; ++it2;
			reusableIds.push_back(it->second.second);
			jsonId2Caller.erase(it);
			it = it2;
		}else{
			++it;
		}
	}
}

void JSONRPC2Client::update(){
	//Sync
	lockMutex(mutexSync);
	syncedLastReceived = lastReceived;
	syncedState = state;
	syncToSend.splice(syncToSend.end(), mainToSend);
	mainToReceive.splice(mainToReceive.end(), syncToReceive);
	if(mustJoin){
		mustJoin = false;
		unlockMutex(mutexSync);
		bool res = joinThread(clientThread);
		assert(res);
	}else{
		unlockMutex(mutexSync);
	}
	//Process received stuff
	for(auto it = mainToReceive.begin(); it != mainToReceive.end(); ++it){
		//std::cout << "Handling: " << convertRPCValueToJSONString(**it) << std::endl;
		handleEntity(*it);
	}
	mainToReceive.clear();
}

IRPCClient::ClientState JSONRPC2Client::getState() const{
	return syncedState;
}

bool JSONRPC2Client::isConnected() const{
	IRPCClient::ClientState state = getState();
	return state==CONNECTING || state==CONNECTED;
}

void JSONRPC2Client::disconnect(){
	update();//required to get the current connection state
	if(isConnected()){
		lockMutex(mutexSync);
		syncExit = true;
		unlockMutex(mutexSync);
		bool res = joinThread(clientThread);
		assert(res);
		mustJoin = false;
	}
}

double JSONRPC2Client::getLastReceiveTime(){
	return syncedLastReceived;
}

void JSONRPC2Client::flush(){
	bool mustWait = true;
	while(mustWait){
		lockMutex(mutexSync);
		mustWait = areTherePendingSends || !syncToSend.empty();
		unlockMutex(mutexSync);
		if(mustWait){delay(10);}
	}
}
