#include "httplib.h"

#include <timing.h>
#include <RequestBasedJSONRPC2ServerHelpers.h>

#include <iostream>
#include <csignal>

volatile bool running = true;

void sigfunc(int sig){
	running = false;
}

class TestProcedureHandler : public IRemoteProcedureCallReceiver{

	public:
	
	const std::unordered_map<std::string, IRemoteProcedureCallReceiver*> registrations;
	
	enum ProcedureCalls{
		SUM,
		CALL_COUNT
	};

	TestProcedureHandler():registrations{{"moveXY",this}}{
	}
	
	int moveXY(int a, int b){
		return a+b;
	}

	IRPCValue* callProcedure(const std::string& procedure, const std::vector<IRPCValue*>& values){
		if(procedure.compare("moveXY")==0){
			return createRPCValue(moveXY(createNativeValue<int>(values[0]), createNativeValue<int>(values[1])));
		}
		return NULL;
	}

};

int main(int argc, char *argv[]){

	signal(SIGINT, sigfunc);
	
	TestProcedureHandler procHandler;
	
	httplib::Server svr;
	svr.set_payload_max_length(10*1000*1000);

	svr.Post("/rpc", [&procHandler](const httplib::Request& req, httplib::Response& res){
		std::cout << "raw received: " << req.body << std::endl;
		res.set_content(processJSONRPC2Request(req.body, procHandler.registrations), "application/json");
		std::cout << "replied: " << res.body << std::endl;
	});
	
	svr.Post("/test", [](const httplib::Request& req, httplib::Response& res){
		res.set_content(std::string("Request body:\n")+req.body, "text/plain");
	});
	
	svr.Get("/test", [](const httplib::Request& req, httplib::Response& res){
		res.set_content("TEST", "text/plain");
	});
	
	std::thread svrThread([&svr](){svr.listen("0.0.0.0", 34634);});
	
	while(running){
		delay(100);
	}
	
	svr.stop();
	svrThread.join();
	
	std::cout << "exit" << std::endl;
	
	return 0;
}
