#include "NamedPipes.h"

#include <iostream>
#include <cstdint>

#if !SIMPLESOCKETS_WIN
#error Named pipes are windows only.
#endif

int main(){
	
	const uint32_t bufSize = 512;
	char buf[bufSize];
	
	std::string reply("This is the answer.");
	
	NamedPipeServer s("\\\\.\\pipe\\TestPipe", true);
	
	std::cout << "Waiting for connection ..." << std::endl;
	while(s.connect()){
		std::cout << "Connected" << std::endl;
		uint32_t received;
		while((received = s.recv(buf, bufSize, true))>0){//in blocking mode it receives always sth when connected
			std::cout << std::string(buf, received) << std::flush;
			s.send(reply.c_str(), reply.size());
		}
		std::cout << "isConnected: " << (int)s.isConnected() << std::endl;
	}
	
	return 0;
}
