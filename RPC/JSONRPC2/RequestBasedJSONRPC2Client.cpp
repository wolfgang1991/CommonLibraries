#include "RequestBasedJSONRPC2Client.h"
#include "JSONRPC2Client.h"

#include <UniqueIdentifierGenerator.h>
#include <Threading.h>
#include <timing.h>
#include <StringHelpers.h>

#include <curl/curl.h>

#include <limits>
#include <cassert>
#include <iostream>
#include <sstream>

static constexpr uint32_t invalidJSONId = std::numeric_limits<uint32_t>::max();

struct JSONRequest{
	std::string procedure;
	std::vector<IRPCValue*> values;
	uint32_t jsonId;
	bool deleteValues;
};

static const std::string requestSenderErrorMessage[IRequestSender::RESPONSE_TYPE_COUNT] = {
	"Success",
	"Sending of request unsuccessful.",
	"Bad response from server."
};

class RequestBasedJSONRPC2ClientPrivate{
	
	public:
	
	//main thread only
	Thread t;
	UniqueIdentifierGenerator<uint32_t> jsonIdGen;
	std::map<uint32_t, std::pair<IRemoteProcedureCaller*, uint32_t> > jsonId2Caller;//jsonId -> (Caller, idFromCaller)
	std::list<JSONRequest> mainToSend;
	std::list<IRPCValue*> mainToReceive;
	
	//sync
	Mutex m;
	bool mustExit;
	std::list<JSONRequest> syncToSend;
	std::list<IRPCValue*> syncToReceive;
	
	//rpc thread only
	IRequestSender* sender;
	uint32_t maxSendCount;
	
	RequestBasedJSONRPC2ClientPrivate(IRequestSender* sender, uint32_t autoRetries):sender(sender),maxSendCount(autoRetries+1){
		initMutex(m);
		mustExit = false;
		bool success = createThread(t, threadWrapper, this, true);
		assert(success);
	}
	
	~RequestBasedJSONRPC2ClientPrivate(){
		lockMutex(m);
		mustExit = true;
		unlockMutex(m);
		bool success = joinThread(t);
		assert(success);
		deleteMutex(m);
		deleteAllElements(syncToReceive);
		for(JSONRequest& r : syncToSend){deleteAllElements(r.values);}
		deleteAllElements(mainToReceive);
		for(JSONRequest& r : mainToSend){deleteAllElements(r.values);}
	}
	
	void threadMain(){
		std::list<JSONRequest> clientToSend;
		std::list<IRPCValue*> clientToReceive;
		JSONParser parser;
		bool running = true;
		while(running){
			lockMutex(m);
			running = !mustExit;
			clientToSend.splice(clientToSend.end(), syncToSend);
			syncToReceive.splice(syncToReceive.end(), clientToReceive);
			unlockMutex(m);
			if(running){
				if(!clientToSend.empty()){
					std::stringstream ss;
					for(JSONRequest& v : clientToSend){
						ss << makeJSONRPCRequest(v.procedure, v.values, v.jsonId==invalidJSONId?NULL:&v.jsonId);
						if(v.deleteValues){deleteAllElements(v.values);}
					}
					std::string toSend = ss.str();
					std::cout << "send: " << toSend << std::endl;//TODO for debugging
					IRequestSender::ResponseType t = IRequestSender::ERROR_NO_CONNECTION;
					std::string result;
					for(uint32_t i=0; i<maxSendCount && t==IRequestSender::ERROR_NO_CONNECTION; i++){
						t = sender->sendRequest(toSend, result);
					}
					if(t==IRequestSender::SUCCESS){
						parser.reset();
						for(uint32_t i=0; i<result.size(); i++){
							IJSONParser::State state = parser.parse(result[i], result[i+1]);//i+1 safe since result[result.size()]=='\0' (guranteed in c++11)
							if(state==IJSONParser::SUCCESS){
								clientToReceive.push_back(parser.stealResult());
								std::cout << "receive: " << convertRPCValueToJSONString(*clientToReceive.back()) << std::endl;//TODO for debugging
								parser.reset();
							}else if(state==IJSONParser::ERROR){
								parser.reset();
							}
						}
					}else{
						for(JSONRequest& v : clientToSend){
							if(v.jsonId!=invalidJSONId){
								clientToReceive.push_back(new ObjectValue({{"jsonrpc", new StringValue("2.0")}, {"id", new IntegerValue(v.jsonId)}, {"error", new ObjectValue({{"code", new IntegerValue(-32603)}, {"message", new StringValue(requestSenderErrorMessage[t])}})}}));
								//std::cout << "error: push_back: " << convertRPCValueToJSONString(*clientToReceive.back()) << std::endl;
							}
						}
					}
					clientToSend.clear();
				}else{
					delay(5);
				}
			}
		}
		deleteAllElements(clientToReceive);
		for(JSONRequest& r : clientToSend){deleteAllElements(r.values);}
	}
	
	static void* threadWrapper(void* data){
		((RequestBasedJSONRPC2ClientPrivate*)data)->threadMain();
		return NULL;
	}
	
	void updateMainThread(){
		lockMutex(m);
		syncToSend.splice(syncToSend.end(), mainToSend);
		mainToReceive.splice(mainToReceive.end(), syncToReceive);
		unlockMutex(m);
		while(!mainToReceive.empty()){//loop via popping in case callback call update recursively
			IRPCValue* v = mainToReceive.front();
			mainToReceive.pop_front();
			if(v->getType()==IRPCValue::OBJECT){
				ObjectValue* o = (ObjectValue*)v;
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
									jsonIdGen.returnId(jsonId);
									jsonId2Caller.erase(it);
								}
							}else if(result && !error){
								it->second.first->OnProcedureResult(result, it->second.second);
								jsonIdGen.returnId(jsonId);
								jsonId2Caller.erase(it);
							}else{
								if(result){o->values["result"] = result;}//put back for proper error output and delete later on
								std::cerr << "Error: Invalid / unsupported JSON-RPC: " << convertRPCValueToJSONString(*o) << std::endl;
							}
						}else{
							if(jsonId!=0){std::cerr << "Error: No RPC caller with json id: " << jsonId << " found." << std::endl;}//0 is ping
							delete result;
						}
					}else{
						std::cerr << "Error: Missing id field: " << convertRPCValueToJSONString(*o) << std::endl;
						delete result;
					}
				}else{
					std::cerr << "Error: Missing result or error field: " << convertRPCValueToJSONString(*o) << std::endl;
				}
			}else{
				std::cerr << "Error: Unexpected return type: " << convertRPCValueToJSONString(*v) << std::endl;
			}
			delete v;
		}
	}
	
};

RequestBasedJSONRPC2Client::RequestBasedJSONRPC2Client(IRequestSender* sender, uint32_t autoRetries){
	p = new RequestBasedJSONRPC2ClientPrivate(sender, autoRetries);
}
	
RequestBasedJSONRPC2Client::~RequestBasedJSONRPC2Client(){
	delete p;
}

bool RequestBasedJSONRPC2Client::callRemoteProcedure(const std::string& procedure, const std::vector<IRPCValue*>& values, IRemoteProcedureCaller* caller, uint32_t id, bool deleteValues){
	if(caller!=NULL){
		uint32_t jsonId = p->jsonIdGen.getUniqueId();
		p->mainToSend.push_back(JSONRequest{procedure, values, jsonId, deleteValues});
		p->jsonId2Caller[jsonId] = std::make_pair(caller, id);
	}else{
		p->mainToSend.push_back(JSONRequest{procedure, values, invalidJSONId, deleteValues});
	}
	return true;
}

void RequestBasedJSONRPC2Client::registerCallReceiver(const std::string& procedure, IRemoteProcedureCallReceiver* receiver){
	std::cerr << "Unable to receive calls with request based JSON-RPC: " << procedure << std::endl;
}

void RequestBasedJSONRPC2Client::unregisterCallReceiver(const std::string& procedure, IRemoteProcedureCallReceiver* receiver){
	registerCallReceiver(procedure, receiver);
}

void RequestBasedJSONRPC2Client::removeProcedureCaller(IRemoteProcedureCaller* caller){
	auto it = p->jsonId2Caller.begin();
	while(it != p->jsonId2Caller.end()){//inefficient, but ok since this method should be called when the caller is deleted which usually occurs at the end of the program
		if(it->second.first == caller){
			auto it2 = it; ++it2;
			p->jsonIdGen.returnId(it->second.second);
			p->jsonId2Caller.erase(it);
			it = it2;
		}else{
			++it;
		}
	}
}

void RequestBasedJSONRPC2Client::update(){
	p->updateMainThread();
}

static void curl_assert(int returnCode){
	assert(returnCode==0);
}

size_t CURLRequestSender::write_callback(char* ptr, size_t size, size_t nmemb, void* userdata){
	if(nmemb>0){
		CURLRequestSender* s = (CURLRequestSender*)userdata;
		s->response << std::string(ptr, nmemb);
	}
	return nmemb;
}

size_t CURLRequestSender::header_callback(char* buffer, size_t size, size_t nitems, void* userdata){
	std::string line(buffer, nitems);
	if(isPrefixEqual(line, "Content-Type:")){
		((CURLRequestSender*)userdata)->isGoodResponse = line.find("application/json")!=std::string::npos;
	}
	return nitems;
}

CURLRequestSender::CURLRequestSender(const std::string& url, bool skipPeerVerification, bool skipHostnameVerification){
	curl = curl_easy_init();
	curl_assert(curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L));//for thread safety
	curl_assert(curl_easy_setopt(curl, CURLOPT_NOPROXY, "localhost,127.0.0.1"));//ipv6 doens't work in this list
	if(skipPeerVerification){
		curl_assert(curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L));
	}
	if(skipHostnameVerification){
		curl_assert(curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L));
	}
	curl_assert(curl_easy_setopt(curl, CURLOPT_HEADERDATA, this));
	curl_assert(curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback));
	curl_assert(curl_easy_setopt(curl, CURLOPT_WRITEDATA, this));
	curl_assert(curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback));
	curl_assert(curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 30L));//30 s
	curl_assert(curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 30L));//30 bytes/s
	curl_assert(curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L));//10s connect timeout
	curl_assert(curl_easy_setopt(curl, CURLOPT_URL, url.c_str()));
	curl_slist* hs = NULL;
	hs = curl_slist_append(hs, "Content-Type: application/json");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hs);
	this->hs = hs;
}
	
CURLRequestSender::~CURLRequestSender(){
	curl_easy_cleanup(curl);
	curl_slist_free_all((curl_slist*)hs);
}

IRequestSender::ResponseType CURLRequestSender::sendRequest(const std::string& toSend, std::string& result){
	isGoodResponse = true;
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, toSend.c_str());
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, toSend.size());
	CURLcode r = curl_easy_perform(curl);
	result = response.str();
	response.str("");
	return r==0?(isGoodResponse?(IRequestSender::SUCCESS):(IRequestSender::ERROR_BAD_RESPONSE)):(IRequestSender::ERROR_NO_CONNECTION);
}
