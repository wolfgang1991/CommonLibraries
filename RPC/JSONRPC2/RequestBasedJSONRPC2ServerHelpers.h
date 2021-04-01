#ifndef RequestBasedJSONRPC2ServerHelpers_H_INCLUDED
#define RequestBasedJSONRPC2ServerHelpers_H_INCLUDED

#include <IRPC.h>

//! depending on the underlying protocol implementation, this function might be called concurrently, also the receiver will be called concurrently then
//! returns the response, at least c++11 required
//! receivers: probably concurrently (therefore constant) map: methode name -> call receiver
std::string processJSONRPC2Request(const std::string& request, const std::unordered_map<std::string, IRemoteProcedureCallReceiver*>& receivers);

#endif
