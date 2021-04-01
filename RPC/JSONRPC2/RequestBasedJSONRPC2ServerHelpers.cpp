#include <RequestBasedJSONRPC2ServerHelpers.h>
#include <JSONParser.h>
#include <JSONRPC2Client.h>

#include <sstream>
#include <iostream>

static void handleEntity(std::stringstream& ss, IRPCValue* entity, const std::unordered_map<std::string, IRemoteProcedureCallReceiver*>& receivers){
	if(entity){
		if(entity->getType()==IRPCValue::ARRAY){
			ArrayValue* array = (ArrayValue*)entity;
			for(IRPCValue* r:array->values){
				handleEntity(ss, r, receivers);
			}
			array->values.clear();//clear before delete since they already have been deleted in the callback methods as required
			return;
		}else if(entity->getType()==IRPCValue::OBJECT){
			ObjectValue* o = (ObjectValue*)entity;
			if(StringValue* method = getObjectField<StringValue>(o, "method")){//requests and notifications
				IntegerValue* id = getObjectField<IntegerValue>(o, "id");//only integer ids allowed in our case
				//std::cout << "handling: " << (method->value) << std::endl;
				auto it = receivers.find(method->value);
				if(it != receivers.end()){
					ArrayValue* params = getObjectField<ArrayValue>(o, "params");
					IRPCValue* result = params?it->second->callProcedure(method->value, params->values):it->second->callProcedure(method->value, {});
					if(id){
						if(result){
							ss << convertRPCValueToJSONResult(*result, id->value);
							delete result;
						}else{//return null
							NULLValue nullVal;
							ss << convertRPCValueToJSONResult(nullVal, id->value);
						}
					}else{
						delete result;
					}
				}else if(id){
					ss << "{\"jsonrpc\": \"2.0\", \"error\": {\"code\":-32601, \"message\": \"Method not found\", \"data\": \"" << method->value << "\"}, \"id\": " << id->value << "}";
				}
			}else{
				std::cerr << "Error: Invalid / unsupported JSON-RPC: " << convertRPCValueToJSONString(*entity) << std::endl;
			}
		}else{
			std::cerr << "Error: Unexpected JSON-RPC type: " << convertRPCValueToJSONString(*entity) << std::endl;
		}
	}
}

std::string processJSONRPC2Request(const std::string& request, const std::unordered_map<std::string, IRemoteProcedureCallReceiver*>& receivers){
	std::stringstream ss;
	if(request.size()>=2){//at least {}
		JSONParser parser;
		for(uint32_t i=0; i<request.size(); i++){
			IJSONParser::State state = parser.parse(request[i], request[i+1]);//i+1 safe since request[request.size()]=='\0' (guranteed in c++11)
			if(state==IJSONParser::SUCCESS){
				IRPCValue* v = parser.stealResult();
				handleEntity(ss, v, receivers);
				delete v;
				parser.reset();
			}else if(state==IJSONParser::ERROR){
				parser.reset();
			}
		}
	}
	return ss.str();
}
